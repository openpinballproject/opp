/*
 *===============================================================================
 *
 *                         OOOOOO
 *                       OOOOOOOOOO
 *      PPPPPPPPPPPPP   OOO      OOO   PPPPPPPPPPPPP
 *    PPPPPPPPPPPPPP   OOO        OOO   PPPPPPPPPPPPPP
 *   PPP         PPP   OOO        OOO   PPP         PPP
 *  PPP          PPP   OOO        OOO   PPP          PPP
 *  PPP          PPP   OOO        OOO   PPP          PPP
 *  PPP          PPP   OOO        OOO   PPP          PPP
 *   PPP         PPP   OOO        OOO   PPP         PPP
 *    PPPPPPPPPPPPPP   OOO        OOO   PPPPPPPPPPPPPP
 *     PPPPPPPPPPPPP   OOO        OOO   PPP
 *               PPP   OOO        OOO   PPP
 *               PPP   OOO        OOO   PPP
 *               PPP   OOO        OOO   PPP
 *               PPP    OOO      OOO    PPP
 *               PPP     OOOOOOOOOO     PPP
 *              PPPPP      OOOOOO      PPPPP
 *
 * @file:   rs232proc.c
 * @author: Hugh Spahr
 * @date:   12/06/2012
 *
 * @note:   Open Pinball Project
 *          Copyright� 2012-2015, Hugh Spahr
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *===============================================================================
 */
/**
 * This is the serial port processing for the Gen2 hardware.  It
 * uses the first serial port.
 *
 *===============================================================================
 */
 
#include "stdtypes.h"
#define RS232I_INSTANTIATE
#include "rs232intf.h"
#include "stdlintf.h"
#include "neointf.h"
#include "procdefs.h"         /* for EnableInterrupts macro */
#include "gen2glob.h"

#include "stm32f1xx_hal.h"

#define STDL_FILE_ID          2

#define TX_BUF_SIZE           0x10
#define RX_BUF_SIZE           0x400
#define MAX_RCV_CMD_LEN       0x34     /* Must include space for CRC8 */

typedef enum
{
   RS232_WAIT_FOR_CARD_ID     = 0x00,
   RS232_WAIT_FOR_CMD         = 0x01,
   RS232_STRIP_CMD            = 0x02,
   RS232_RCV_DATA_CMD         = 0x03,  /* Also strips the data */
   RS232_INVENTORY_CMD        = 0x04,  /* Special case since unknown length */
   RS232_NEO_COLOR_TBL        = 0x05,
   RS232_RCV_NEO_HDR          = 0x06,
   RS232_RCV_NEO_DATA         = 0x07,
} RS232_STATE_E;

typedef struct
{
   RS232_STATE_E              state;
   U8                         cmdLen;
   RS232I_CMD_E               currCmd;
   U8                         currIndex;
   U8                         crc8;
   U8                         *rcvHead_p;
   U8                         *rcvTail_p;
   U8                         rxBuf[RX_BUF_SIZE];
   U8                         rxTmpBuf[MAX_RCV_CMD_LEN];
} RS232_GLOB_T;

RS232_GLOB_T                  rs232_glob;

/* Prototypes */
void digital_set_solenoid_input(
   RS232I_SET_SOL_INP_E       inputIndex,
   U8                         solIndex);
void digital_set_kick_pwm(
   U8                         kickPwm,
   U8                         solIndex);
void digital_upd_sol_cfg(
   U32                        numSol);
void digital_upd_ind_sol_cfg(
   U32                        solNum);
void digital_upd_inp_cfg(
   U32                        numInp);
void digital_upd_ind_inp_cfg(
   U32                        inpNum);
void digital_set_inp_cfg(
   U32                        inpNum,
   RS232I_CFG_INP_TYPE_E      inpCfg);
void digital_convert_v0_sol_cfg_to_v1_solcfg(
   INT                        solIndex,
   RS232I_SOL_CFG_T           *cfg_p);

void rs232proc_rx_buffer(
   U8                          *rcv_p,
   U32                         length);
void rs232proc_bswap_copy_dest(
   U32                        *data_p,
   U8                         *dst_p);
void rs232proc_copy_dest(
   U8                         *src_p,
   U8                         *dst_p,
   UINT                       length);
void rs232proc_bswap_data_buf(
   U32                        *data_p,
   UINT                       numBytes);
void incand_proc_cmd(
   U8                         cmd,
   U32                        mask);
void stdlser_get_xmt_info(
   U8                         **data_pp,
   U16                        *numChar_p);
uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len);
void fade_update_rcv_cmd(
   U16                  offset,
   U16                  numBytes,
   U16                  fadeTime);
BOOL fade_update_rcv_data(
   U8                   data);

/*
 * ===============================================================================
 * 
 * Name: rs232proc_init
 * 
 * ===============================================================================
 */
/**
 * Initialize RS232 processing
 * 
 * Initialize serial port link.
 * 
 * @param   None 
 * @return  None
 * 
 * @pre     None 
 * @note    None
 * 
 * ===============================================================================
 */
void rs232proc_init(void) 
{
   /* Initialize the global structure */
   rs232_glob.state = RS232_WAIT_FOR_CARD_ID;
   rs232_glob.rcvHead_p = &rs232_glob.rxBuf[0];
   rs232_glob.rcvTail_p = &rs232_glob.rxBuf[0];
} /* End rs232proc_init */

/*
 * ===============================================================================
 * 
 * Name: rs232proc_task
 * 
 * ===============================================================================
 */
/**
 * Task for rs232 commands
 * 
 * @param   None 
 * @return  None
 * 
 * @pre     None 
 * @note    Verifying the CRC is only necessary for changing configuration
 *    commands.  The CRC will be verified for other commands, but commands are
 *    not ignored if that CRC is bad because the command response is already
 *    being sent.
 * 
 * ===============================================================================
 */
void rs232proc_task(void)
{
   U8                         data;
   U8                         txBuf[RS232I_MAX_TX_BUF_SZ];
   U8                         *src_p;
   U8                         *dest_p;
   UINT                       index;
   U32                        mask;
   U32                        tmpU32;
   uint8_t                    *xmtData_p;
   uint16_t                   xmtLength;

#define INCAND_CMD_OFFSET     0
#define INCAND_MASK_OFFSET    1
#define CONFIG_NUM_OFFSET     0
#define CONFIG_DATA_OFFSET    1
  
   /* Check if received a char */
   while (rs232_glob.rcvTail_p != rs232_glob.rcvHead_p)
   {
      data = *rs232_glob.rcvTail_p++;
      if (rs232_glob.rcvTail_p >= &rs232_glob.rxBuf[RX_BUF_SIZE])
      {
    	  rs232_glob.rcvTail_p = &rs232_glob.rxBuf[0];
      }
      switch (rs232_glob.state)
      {
         case RS232_WAIT_FOR_CARD_ID:
         {
            if (data == RS232I_INVENTORY)
            {
               rs232_glob.state = RS232_INVENTORY_CMD;
               (void)stdlser_xmt_data(&data, 1);
            }
            else if (data == RS232I_EOM)
            {
               (void)stdlser_xmt_data(&data, 1);
            }
            else if (data == CARD_ID_GEN2_CARD)
            {
               /* It is my command, it may or may not need stripped */
               rs232_glob.crc8 = 0xff;
               stdlser_calc_crc8(&rs232_glob.crc8, RS232I_CRC8_SZ, &data);
               rs232_glob.state = RS232_WAIT_FOR_CMD;
            }
            else
            {
               /* Lost synchronization.  Keep looking for a valid card ID */
            }
            break;
         }
         case RS232_WAIT_FOR_CMD:
         {
            rs232_glob.currIndex = 0;
            if (data < RS232I_NUM_CMDS)
            {
               /* Save the current command and the length */
               rs232_glob.currCmd = data;
               rs232_glob.cmdLen = CMD_LEN[data] + RS232I_CRC8_SZ;

               stdlser_calc_crc8(&rs232_glob.crc8, RS232I_CRC8_SZ, &data);
               txBuf[0] = CARD_ID_GEN2_CARD;
               txBuf[1] = data;

               switch (data)
               {
                  case RS232I_GET_SER_NUM:
                  {
                     rs232proc_bswap_copy_dest((U32 *)(&gen2g_persist_p->serNum), &txBuf[RS232I_CMD_HDR]);
                     rs232_glob.state = RS232_STRIP_CMD;
                     txBuf[6] = 0xff;
                     stdlser_calc_crc8(&txBuf[6], 6, &txBuf[0]);
                     (void)stdlser_xmt_data(&txBuf[0], 7);
                     break;
                  }
                  case RS232I_GET_PROD_ID:
                  case RS232I_GET_GEN2_CFG:
                  {
                     /* Product ID is uniquely identified by wing board cfg */
                     rs232proc_copy_dest(&gen2g_nv_cfg_p->wingCfg[0], &txBuf[RS232I_CMD_HDR], RS232I_NUM_PROC_WINGS);
                     rs232_glob.state = RS232_STRIP_CMD;
                     txBuf[6] = 0xff;
                     stdlser_calc_crc8(&txBuf[6], 6, &txBuf[0]);
                     (void)stdlser_xmt_data(&txBuf[0], 7);
                     break;
                  }
                  case RS232I_GET_VERS:
                  {
                     rs232proc_bswap_copy_dest((U32 *)&appStart.codeVers, &txBuf[RS232I_CMD_HDR]);
                     rs232_glob.state = RS232_STRIP_CMD;
                     txBuf[6] = 0xff;
                     stdlser_calc_crc8(&txBuf[6], 6, &txBuf[0]);
                     (void)stdlser_xmt_data(&txBuf[0], 7);
                     break;
                  }
                  case RS232I_SET_SER_NUM:
                  {
                     /* Check if serial number is blank, strip cmd */
                     if (gen2g_persist_p->serNum == 0xffffffff)
                     {
                        rs232_glob.state = RS232_RCV_DATA_CMD;
                     }
                     else
                     {
                        /* Already set, so respond with serial number */
                        rs232_glob.state = RS232_STRIP_CMD;
                        rs232proc_bswap_copy_dest((U32 *)(&gen2g_persist_p->serNum), &txBuf[RS232I_CMD_HDR]);
                        stdlser_calc_crc8(&txBuf[6], 6, &txBuf[0]);
                        (void)stdlser_xmt_data(&txBuf[0], 7);
                     }
                     break;
                  }
                  case RS232I_RESET:
                  case RS232I_GO_BOOT:
                  case RS232I_CONFIG_SOL:
                  case RS232I_KICK_SOL:
                  case RS232I_CONFIG_INP:
                  case RS232I_SAVE_CFG:
                  case RS232I_ERASE_CFG:
                  case RS232I_SET_GEN2_CFG:
                  case RS232I_INCAND_CMD:
                  case RS232I_CONFIG_IND_SOL:
                  case RS232I_CONFIG_IND_INP:
                  case RS232I_SET_IND_NEO:
                  case RS232I_SET_SOL_INPUT:
                  case RS232I_SOL_KICK_PWM:
                  {
                     /* Verify CRC to be sure */
                     rs232_glob.state = RS232_RCV_DATA_CMD;
                     break;
                  }
                  case RS232I_READ_GEN2_INP:
                  {
                     DisableInterrupts;
                     tmpU32 = gen2g_info.validSwitch;
                     gen2g_info.validSwitch = 0;
                     EnableInterrupts;
                     rs232proc_bswap_copy_dest(&tmpU32, &txBuf[RS232I_CMD_HDR]);
                     rs232_glob.state = RS232_STRIP_CMD;
                     txBuf[6] = 0xff;
                     stdlser_calc_crc8(&txBuf[6], 6, &txBuf[0]);
                     (void)stdlser_xmt_data(&txBuf[0], 7);
                     break;
                  }
                  case RS232I_SET_NEO_COLOR_TBL:
                  {
                     rs232_glob.state = RS232_NEO_COLOR_TBL;
                     break;
                  }
                  case RS232I_READ_MATRIX_INP:
                  {
                     /* Only state is supported for switch matrices */
                     rs232proc_copy_dest(&gen2g_info.matrixInp[0], &txBuf[RS232I_CMD_HDR], RS232I_MATRX_COL);
                     rs232_glob.state = RS232_STRIP_CMD;
                     txBuf[10] = 0xff;
                     stdlser_calc_crc8(&txBuf[10], 10, &txBuf[0]);
                     (void)stdlser_xmt_data(&txBuf[0], 11);

                     /* Clear bits/set bits currently not active.  This means that a bit
                      * that is active won't be cleared until it is read by the processor.
                      */
                     for (index = 0, dest_p = &gen2g_info.matrixInp[0],
                        src_p = &gen2g_info.matrixPrev[0];
                        index < RS232I_MATRX_COL;
                        index++, src_p++, dest_p++)
                     {
                        if (gen2g_info.switchMtrxActHigh)
                        {
                           *dest_p &= *src_p;
                        }
                        else
                        {
                           *dest_p |= *src_p;
                        }
                     }
                     break;
                  }
                  case RS232I_GET_INP_TIMESTAMP:
                  {
                     rs232proc_copy_dest((U8 *)&gen2g_info.inpTimestamp[0], &txBuf[RS232I_CMD_HDR], RS232I_TIMESTAMP_BYTES);
                     rs232_glob.state = RS232_STRIP_CMD;
                     txBuf[RS232I_CMD_HDR + RS232I_TIMESTAMP_BYTES] = 0xff;
                     stdlser_calc_crc8(&txBuf[RS232I_CMD_HDR + RS232I_TIMESTAMP_BYTES],
                        RS232I_CMD_HDR + RS232I_TIMESTAMP_BYTES, &txBuf[0]);
                     (void)stdlser_xmt_data(&txBuf[0], RS232I_CMD_HDR + RS232I_TIMESTAMP_BYTES + RS232I_CRC8_SZ);
                     break;
                  }
                  case RS232I_CHNG_NEO_CMD:
                  case RS232I_CHNG_NEO_COLOR:
                  case RS232I_CHNG_NEO_COLOR_TBL:
                  case RS232I_GEN2_UNUSED:
                  case RS232I_UPGRADE_OTHER_BRD:
                  default:
                  {
                     /* Bad command received, send EOM */
                     data = RS232I_EOM;
                     (void)stdlser_xmt_data(&data, 1);
                     rs232_glob.state = RS232_WAIT_FOR_CARD_ID;
                     break;
                  }
               }
            }
            else if (data == RS232I_NEO_FADE_CMD)
            {
               stdlser_calc_crc8(&rs232_glob.crc8, 1, &data);
               rs232_glob.state = RS232_RCV_NEO_HDR;
            }
            else
            {
               /* Bad command received, send EOM */
               data = RS232I_EOM;
               (void)stdlser_xmt_data(&data, 1);
               rs232_glob.state = RS232_WAIT_FOR_CARD_ID;
            }
            break;
         }
         case RS232_STRIP_CMD:
         {
            /* The following commands are stripped:  RS232I_GET_SER_NUM,
             * RS232I_GET_PROD_ID, RS232I_GET_GEN2_CFG, RS232I_GET_VERS,
             * RS232I_READ_GEN2_INP, and RS232I_READ_MATRIX_INP.
             * Responses are sent when first rcv'd.
             */
            rs232_glob.currIndex++;
            if (rs232_glob.currIndex < rs232_glob.cmdLen)
            {
               stdlser_calc_crc8(&rs232_glob.crc8, 1, &data);
            }
            else
            {
               if (data != rs232_glob.crc8)
               {
                  gen2g_info.crcErr++;
               }
               else
               {
                  /* Blink status LED */
                  gen2g_info.statusBlink ^= GEN2G_STAT_TOGGLE_LED;
                  *((R32 *)GEN2G_STAT_BSRR_PTR) = gen2g_info.statusBlink;
               }

               /* Whole command has been passed on, now wait for next cmd */
               rs232_glob.state = RS232_WAIT_FOR_CARD_ID;
            }
            break;
         }
         case RS232_RCV_DATA_CMD:
         {
            /* Copy the data into the rcv buffer, and calculate CRC */
            rs232_glob.rxTmpBuf[rs232_glob.currIndex++] = data;
            if (rs232_glob.currIndex < rs232_glob.cmdLen)
            {
               stdlser_calc_crc8(&rs232_glob.crc8, 1, &data);
            }
            else
            {
               /* Blink status LED */
               gen2g_info.statusBlink ^= GEN2G_STAT_TOGGLE_LED;
               *((R32 *)GEN2G_STAT_BSRR_PTR) = gen2g_info.statusBlink;

               /* Whole command has been received */
               if (data == rs232_glob.crc8)
               {
                  /* Do command special processing */
                  switch(rs232_glob.currCmd)
                  {
                     case RS232I_SET_SER_NUM:
                     {
                        U32 serNum = (U32)rs232_glob.rxTmpBuf[3] |
                           ((U32)rs232_glob.rxTmpBuf[2] << 8) |
                           ((U32)rs232_glob.rxTmpBuf[1] << 16) |
                           ((U32)rs232_glob.rxTmpBuf[0] << 24);
                        stdlflash_write((U16 *)&serNum, (U16 *)(&gen2g_persist_p->serNum), sizeof(U32));
                        break;
                     }
                     case RS232I_RESET:
                     {
                        scbBase_p->AIRCR = AIRCR_SYSRESETREQ;
                        break;
                     }
                     case RS232I_GO_BOOT:
                     {
                        /* This command resets the processor */
                    	*(U32 *)SRAM_FIRST_ADDR = 0xdeadbeef;
                        scbBase_p->AIRCR = AIRCR_SYSRESETREQ;
                        break;
                     }
                     case RS232I_CONFIG_SOL:
                     {
                        for (index = 0, src_p = &rs232_glob.rxTmpBuf[0];
                           index < RS232I_NUM_GEN2_SOL; index++)
                        {
                           digital_convert_v0_sol_cfg_to_v1_solcfg(
                              index, (RS232I_SOL_CFG_T *)src_p);
                           src_p += sizeof(RS232I_SOL_CFG_T);
                        }
                        /* Only update the original 16 solenoids */
                        digital_upd_sol_cfg(RS232I_NUM_GEN2_SOL);
                        break;
                     }
                     case RS232I_KICK_SOL:
                     {
                        DisableInterrupts;
                        gen2g_info.solDrvProcCtl = (gen2g_info.solDrvProcCtl &
                           ~(((U16)rs232_glob.rxTmpBuf[2] << 8) | (U16)rs232_glob.rxTmpBuf[3])) |
                           (((U16)rs232_glob.rxTmpBuf[0] << 8) | (U16)rs232_glob.rxTmpBuf[1]);
                        EnableInterrupts;
                        break;
                     }
                     case RS232I_CONFIG_INP:
                     {
                        for (index = 0, src_p = &rs232_glob.rxTmpBuf[0];
                           index < RS232I_NUM_GEN2_INP;
                           index++, src_p++)
                        {
                           digital_set_inp_cfg(index, (RS232I_CFG_INP_TYPE_E)*src_p);
                        }
                        digital_upd_inp_cfg(RS232I_NUM_GEN2_INP);
                        break;
                     }
                     case RS232I_SAVE_CFG:
                     case RS232I_ERASE_CFG:
                     case RS232I_SET_GEN2_CFG:
                     {
                        /* Unused command, config sector is updated through bootloader */
                        break;
                     }
                     case RS232I_INCAND_CMD:
                     {
                        mask = ((U32)rs232_glob.rxTmpBuf[INCAND_MASK_OFFSET] << 24) |
                           ((U32)rs232_glob.rxTmpBuf[INCAND_MASK_OFFSET + 1] << 16) |
                           ((U32)rs232_glob.rxTmpBuf[INCAND_MASK_OFFSET + 2] << 8) |
                           (U32)rs232_glob.rxTmpBuf[INCAND_MASK_OFFSET + 3];
                        incand_proc_cmd(rs232_glob.rxTmpBuf[INCAND_CMD_OFFSET], mask);
                        break;
                     }
                     case RS232I_CONFIG_IND_SOL:
                     {
                        /* First byte contains solenoid number */
                        if (rs232_glob.rxTmpBuf[CONFIG_NUM_OFFSET] < RS232I_NUM_GEN25_SOL)
                        {
                           src_p = &rs232_glob.rxTmpBuf[CONFIG_DATA_OFFSET];
                           digital_convert_v0_sol_cfg_to_v1_solcfg(
                              rs232_glob.rxTmpBuf[CONFIG_NUM_OFFSET],
							  (RS232I_SOL_CFG_T *)src_p);
                           digital_upd_ind_sol_cfg(rs232_glob.rxTmpBuf[CONFIG_NUM_OFFSET]);
                        }
                        break;
                     }
                     case RS232I_CONFIG_IND_INP:
                     {
                        if (rs232_glob.rxTmpBuf[CONFIG_NUM_OFFSET] < RS232I_NUM_GEN25_INP)
                        {
                           digital_set_inp_cfg(rs232_glob.rxTmpBuf[CONFIG_NUM_OFFSET],
                              rs232_glob.rxTmpBuf[CONFIG_DATA_OFFSET]);
                           digital_upd_ind_inp_cfg(rs232_glob.rxTmpBuf[CONFIG_NUM_OFFSET]);
                        }
                        break;
                     }
                     case RS232I_SET_SOL_INPUT:
                     {
                        digital_set_solenoid_input(rs232_glob.rxTmpBuf[CONFIG_NUM_OFFSET],
                           rs232_glob.rxTmpBuf[CONFIG_DATA_OFFSET]);
                        break;
                     }
                     case RS232I_SOL_KICK_PWM:
                     {
                        digital_set_kick_pwm(rs232_glob.rxTmpBuf[CONFIG_NUM_OFFSET],
                           rs232_glob.rxTmpBuf[CONFIG_DATA_OFFSET]);
                        break;
                     }
                     default:
                     {
                        /* Invalid cmd for RS232_RCV_DATA_CMD, send EOM */
                        data = RS232I_EOM;
                        (void)stdlser_xmt_data(&data, 1);
                        rs232_glob.state = RS232_WAIT_FOR_CARD_ID;
                        break;
                     }
                  }
               }
               else
               {
                  gen2g_info.crcErr++;
               }

               /* Whole command has been received */
               rs232_glob.state = RS232_WAIT_FOR_CARD_ID;
            }
            break;
         }
         case RS232_INVENTORY_CMD:
         {
            if (data == RS232I_EOM)
            {
               /* Rcv'd EOM, so my addr is next addr */
               txBuf[0] = CARD_ID_GEN2_CARD;
               txBuf[1] = RS232I_EOM;
               (void)stdlser_xmt_data(&txBuf[0], 2);
               rs232_glob.state = RS232_WAIT_FOR_CARD_ID;
            }
            else
            {
               /* Not my card type and make no assumptions, pass thru */
               (void)stdlser_xmt_data(&data, 1);
            }
            break;
         }
         case RS232_NEO_COLOR_TBL:
         {
            /* Command only passes bytesPerPixel, numPixels, and initial color */
            if (rs232_glob.currIndex < sizeof(GEN2G_NEO_CFG_T))
            {
               *((U8 *)&gen2g_info.neoCfg + rs232_glob.currIndex) = data;
            }
            rs232_glob.currIndex++;
            if (rs232_glob.currIndex < rs232_glob.cmdLen)
            {
               stdlser_calc_crc8(&rs232_glob.crc8, 1, &data);
            }
            else
            {
               /* Whole command has been received */
               if (data != rs232_glob.crc8)
               {
                  gen2g_info.crcErr++;
               }

               /* Whole command has been passed on, now wait for next cmd */
               rs232_glob.state = RS232_WAIT_FOR_CARD_ID;
            }
            break;
         }
         case RS232_RCV_NEO_HDR:
         {
       	    // Data = offset (2 bytes), num bytes (2 bytes), time of fade (2 bytes), data..., CRC8
            stdlser_calc_crc8(&rs232_glob.crc8, 1, &data);
            rs232_glob.rxTmpBuf[rs232_glob.currIndex++] = data;
            if (rs232_glob.currIndex == 6)
            {
               fade_update_rcv_cmd((rs232_glob.rxTmpBuf[0] << 8) | rs232_glob.rxTmpBuf[1],
                  (rs232_glob.rxTmpBuf[2] << 8) | rs232_glob.rxTmpBuf[3],
                  (rs232_glob.rxTmpBuf[4] << 8) | rs232_glob.rxTmpBuf[5]);
               rs232_glob.state = RS232_RCV_NEO_DATA;
            }
            break;
         }
         case RS232_RCV_NEO_DATA:
         {
            BOOL done = fade_update_rcv_data(data);
            if (!done)
            {
                stdlser_calc_crc8(&rs232_glob.crc8, 1, &data);
            }
            else
            {
               if (data != rs232_glob.crc8)
               {
                  gen2g_info.crcErr++;
               }
               else
               {
                  /* Blink status LED */
                  gen2g_info.statusBlink ^= GEN2G_STAT_TOGGLE_LED;
                  *((R32 *)GEN2G_STAT_BSRR_PTR) = gen2g_info.statusBlink;
               }

               /* Whole command has been rcvd, now wait for next cmd */
               rs232_glob.state = RS232_WAIT_FOR_CARD_ID;
            }
            break;
         }
         default:
         {
            /* Invalid state, send EOM */
            data = RS232I_EOM;
            (void)stdlser_xmt_data(&data, 1);
            rs232_glob.state = RS232_WAIT_FOR_CARD_ID;
            break;
         }
      }
   }
   stdlser_get_xmt_info(&xmtData_p, &xmtLength);
   if (xmtLength)
   {
      CDC_Transmit_FS(xmtData_p, xmtLength);
   }
} /* End rs232proc_task */

/*
 * ===============================================================================
 * 
 * Name: rs232proc_rx_ser_char
 * 
 * ===============================================================================
 */
/**
 * Receive a buffer
 * 
 * Receive a data buffer from USB.  Save the buffer, length, and mark the flag.
 * 
 * @param   rcv_p - received buffer pointer
 * @param   length - amount of data received
 * @return  TRUE if buffer overflow occurs
 * 
 * @pre     None 
 * @note    None
 * 
 * ===============================================================================
 */
void rs232proc_rx_buffer(
   U8                          *rcv_p,
   U32                         length)
{
   U8                          *nxt_p;

   nxt_p = rs232_glob.rcvHead_p;
   while (length != 0)
   {
      /* Verify write won't overflow the queue */
      nxt_p++;
      if (nxt_p >= &rs232_glob.rxBuf[RX_BUF_SIZE])
      {
         nxt_p = &rs232_glob.rxBuf[0];
      }
      if (nxt_p != rs232_glob.rcvTail_p)
      {
         *rs232_glob.rcvHead_p = *rcv_p++;
         rs232_glob.rcvHead_p = nxt_p;
         length--;
      }
      else
      {
         break;
      }
   }
} /* End rs232proc_rx_buffer */

/*
 * ===============================================================================
 * 
 * Name: rs232proc_bswap_copy_dest
 * 
 * ===============================================================================
 */
/**
 * Byte swap 32 bit data and copy to destination
 * 
 * This converts from little endian to big endian for destination byte stream.
 * 
 * @param   None 
 * @return  None
 * 
 * @pre     None 
 * @note    None
 * 
 * ===============================================================================
 */
void rs232proc_bswap_copy_dest(
   U32                        *data_p,
   U8                         *dst_p)
{
   U32                        data;
   U8                         *src_p;
   UINT                       index;
   
   data = *data_p;
   data = ((data >> 24) & 0xff) | ((data << 8) & 0xff0000) |
      ((data >> 8) & 0xff00) | ((data << 24) & 0xff000000);
   for (src_p = (U8 *)&data, index = 0; index < sizeof(U32); index++)
   {
      *dst_p++ = *src_p++;
   }
} /* End rs232proc_bswap_copy_dest */

/*
 * ===============================================================================
 * 
 * Name: rs232proc_copy_dest
 * 
 * ===============================================================================
 */
/**
 * Byte copy to destination
 * 
 * @param   None 
 * @return  None
 * 
 * @pre     None 
 * @note    None
 * 
 * ===============================================================================
 */
void rs232proc_copy_dest(
   U8                         *src_p,
   U8                         *dst_p,
   UINT                       length)
{
   UINT                       index;
   
   for (index = 0; index < length; index++)
   {
      *dst_p++ = *src_p++;
   }
} /* End rs232proc_copy_dest */


/*
 * ===============================================================================
 * 
 * Name: rs232proc_bswap_data_buf
 * 
 * ===============================================================================
 */
/**
 * Byte swap received data buffer to move from little endian to big endian
 * 
 * This converts from received bytes stream from little endian to big endian.
 * 
 * @param   None 
 * @return  None
 * 
 * @pre     None 
 * @note    None
 * 
 * ===============================================================================
 */
void rs232proc_bswap_data_buf(
   U32                        *data_p,
   UINT                       numBytes)
{
   U32                        data;
   UINT                       index;
   
   for (index = 0; index < numBytes; index += sizeof(U32))
   {
      data = *data_p;
      *data_p++ = ((data >> 24) & 0xff) | ((data << 8) & 0xff0000) |
         ((data >> 8) & 0xff00) | ((data << 24) & 0xff000000);
   }
} /* End rs232proc_bswap_data_buf */

/* [] END OF FILE */
