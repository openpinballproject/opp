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
 * @file:   digital.c
 * @author: Hugh Spahr
 * @date:   12/02/2012
 *
 * @note:   Open Pinball Project
 *          Copyright� 2012-2019, Hugh Spahr
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
 * This file controls the digital wing boards (inputs/solenoids).  It is
 * adapted from digital.c in the previous 1003-InpDrv\Sources\digital.c
 *
 *===============================================================================
 */
 
#include <stdlib.h>
#include "stdtypes.h"
#include "stdlintf.h"
#include "rs232intf.h"
#include "gen2glob.h"
#include "procdefs.h"         /* for EnableInterrupts macro */

#include "stm32f1xx_hal.h"

#define STDL_FILE_ID          3

typedef struct
{
   GPIO_TypeDef               *port_p;
   uint16_t                   GPIO_Pin;
} DIG_PIN_INFO_T;

const DIG_PIN_INFO_T dig_pinInfo[RS232I_NUM_GEN2_INP] =
   {  { GPIOA, GPIO_PIN_13 }, { GPIOB, GPIO_PIN_12 },
      { GPIOB, GPIO_PIN_13 }, { GPIOB, GPIO_PIN_14 },
      { GPIOB, GPIO_PIN_15 }, { GPIOA, GPIO_PIN_8 },
      { GPIOA, GPIO_PIN_9 }, { GPIOA, GPIO_PIN_10 },

      { GPIOA, GPIO_PIN_15 }, { GPIOB, GPIO_PIN_3 },
      { GPIOB, GPIO_PIN_4 }, { GPIOB, GPIO_PIN_5 },
      { GPIOB, GPIO_PIN_6 }, { GPIOB, GPIO_PIN_7 },
      { GPIOB, GPIO_PIN_8 }, { GPIOB, GPIO_PIN_9 },

      { GPIOA, GPIO_PIN_14 }, { GPIOC, GPIO_PIN_13 },
      { GPIOC, GPIO_PIN_14 }, { GPIOC, GPIO_PIN_15 },
      { GPIOA, GPIO_PIN_0 }, { GPIOA, GPIO_PIN_1 },
      { GPIOA, GPIO_PIN_2 }, { GPIOA, GPIO_PIN_3 },

      { GPIOA, GPIO_PIN_4 }, { GPIOA, GPIO_PIN_5 },
      { GPIOA, GPIO_PIN_6 }, { GPIOA, GPIO_PIN_7 },
      { GPIOB, GPIO_PIN_0 }, { GPIOB, GPIO_PIN_1 },
      { GPIOB, GPIO_PIN_10 }, { GPIOB, GPIO_PIN_11 } };

const U32                   PWM_MASK[32] =
{
   0x00000001, 0x00100001, 0x01004001, 0x41002001,
   0x08120201, 0x14240201, 0x41084481, 0x22A21081,
   0x608A2121, 0x32A28421, 0x62259421, 0x5125CA11,
   0x6296C451, 0x63B28A91, 0x6ADA3911, 0xABA92E51,
   0x7763A4A9, 0x76AEB239, 0x75DC96B9, 0x77AC5B75,
   0x7BD65BB9, 0x7BDEBAB3, 0x7B7BAEF9, 0x7EF7BABB,
   0x7EDDEF7D, 0x7FBFDBD7, 0x7FBFB7EF, 0x7FFBFF7D,
   0x7FFDFF7F, 0x7FFFF7FF, 0x7FFFFFFF, 0xFFFFFFFF,
};

typedef enum
{
   SOL_STATE_IDLE             = 0x00,
   SOL_INITIAL_KICK           = 0x01,
   SOL_SUSTAIN_PWM            = 0x02,
   SOL_MIN_TIME_OFF           = 0x03,
   SOL_WAIT_BEFORE_KICK       = 0x04,
   SOL_FULL_ON_SOLENOID       = 0x05,
} __attribute__((packed)) DIG_SOL_STATE_E;

typedef struct
{
   BOOL                       clearRcvd;
   DIG_SOL_STATE_E            solState;
   U8                         offCnt;
   U32                        kickIntenMask;
   U32                        holdIntenMask;
   U32                        drvBit;
   INT                        startMs;
   U32                        inpBits;
   U32                        spiInpBits[2];
} DIG_SOL_STATE_T;

typedef struct
{
   DIG_SOL_STATE_T            state;
   RS232I_SOL25_CFG_T     	  cfg;
} DIG_SOL_INFO_T;

typedef struct
{
   INT                        cnt;
} DIG_INP_STATE_T;

typedef struct
{
	DIG_INP_STATE_T           state;
	RS232I_CFG_INP_TYPE_E     cfg;
} DIG_INP_INFO_T;

#define MATRIX_FIRE_SOL       0x80
#define MATRIX_SOL_MASK       0x07
#define MATRIX_WAIT_CNT_THRESH   2
#define MATRIX_DEBOUNCE_THRESH   4
#define MATRIX_COL_OFFS       16
#define MATRIX_COL_MASK       0x00ff0000

typedef struct
{
   U8                         cnt;
   U8                         sol;
} DIG_MTRX_SPI_BIT_INFO_T;

typedef struct
{
   DIG_MTRX_SPI_BIT_INFO_T    info[RS232I_NUM_SW_MATRX_SPI_INP];
} DIG_MATRIX_SPI_INFO_T;

typedef struct
{
   U8                         column;
   U8                         waitCnt;
   U8                         mtrxWaitCntThresh;
   U8                         mtrxDebounceThresh;
   BOOL                       mtrxActHigh;
} DIG_MATRIX_CFG_T;

typedef struct
{
   U16                        port;
   U16                        msk;
} DIG_PORT_DATA_T;

typedef struct
{
   U32                        prevInputs;
   U32                        filtInputs;
   U32                        mtrxInpMask;
   U32                        stateMask;
   U32                        outputUpd;
   U32                        outputMask;
   U32                        solOutMask;
   DIG_MATRIX_CFG_T           mtrxCfg;
   DIG_MATRIX_SPI_INFO_T      mtrxSpi;
   DIG_INP_INFO_T             inpInfo[RS232I_NUM_GEN25_INP];
   DIG_SOL_INFO_T             solInfo[RS232I_NUM_GEN25_SOL];
} DIG_GLOB_T;

DIG_GLOB_T                    dig_info;

/* Prototypes */
INT timer_get_ms_count();
U16 timer_get_us_count();
void digital_set_solenoid_input(
   RS232I_SET_SOL_INP_E       inputIndex,
   U8                         solIndex);
void digital_set_kick_pwm(
   U8                         kickPwm,
   U8                         solIndex);
void digital_upd_sol_cfg(
   U32                        numSol);
void digital_upd_inp_cfg(
   U32                        updMask);
void digital_upd_outputs(
   U32                        value,
   U32                        mask);

/*
 * ===============================================================================
 *
 * Name: digital_init
 * 
 * ===============================================================================
 */
/**
 * Initialize the digital I/O port
 * 
 * Initialize digital I/O port, and the other digital control signals.
 * 
 * @param   None 
 * @return  None
 * 
 * @pre     None 
 * @note    None
 * 
 * ===============================================================================
 */
void digital_init(void) 
{
   U32                        outputMask = 0;
   GPIO_InitTypeDef           pinCfg;
   BOOL                       usedBit;

#define ALL_PINS_BIT_MASK     0xff
#define NEO_INP_BIT_MASK      0xef
#define NEO_OUT_BIT_MASK      0x10
#define NEO_SOL_INP_BIT_MASK  0x0e
#define NEO_SOL_OUT_BIT_MASK  0xf1
#define NEO_SPI_CLK_BIT_MASK  0x0f
#define SOL_MASK              0x0f
#define SOL_INP_BIT_MASK      0x0f  
#define SOL_OUT_BIT_MASK      0xf0

#define MTRX_INPUT_BIT_MASK   0xff000000
#define MTRX_OUTPUT_BIT_MASK  0x00ff0000
#define DBG_MTRX_OUT_BIT_MASK 0x00fe0000
#define DBG_PIN_BIT_MASK      0x00010001
#define MAX_SOL_MASK          0x000f000f
#define SPI_INP_BIT_MASK      0xf4000000
#define SPI_OUT_BIT_MASK      0x0b000000
   
   /* Init gen2g structure */
   U32 *u32_p = (U32 *)&dig_info;
   for (INT index = 0; index < sizeof(dig_info)/sizeof(U32); index++)
   {
      *u32_p++ = 0;
   }

   for (INT solNum = 0; solNum < RS232I_NUM_GEN25_SOL; solNum++)
   {
      DIG_SOL_STATE_T *solState_p = &dig_info.solInfo[solNum].state;
      if (solNum < RS232I_NUM_GEN2_SOL)
      {
         solState_p->drvBit = 1 << (((solNum >> 2) << 3) + (solNum & 0x03) + 4);
      }
      else if (solNum < RS232I_NUM_PROC_PINS)
      {
         solState_p->drvBit = 1 << ((((solNum - RS232I_NUM_GEN2_SOL) >> 2) << 3) +
            (solNum & 0x03));
      }
      solState_p->kickIntenMask = 0xffffffff;
   }

   /* Set up digital ports, walk through wing boards */
   for (INT wingNum = 0; wingNum < RS232I_NUM_PROC_WINGS; wingNum++)
   {
      switch (gen2g_nv_cfg_p->wingCfg[wingNum])
      {
         case WING_UNUSED:
         case WING_UNUSED2:
         {
            break;
         }
         case WING_INP:
         {
            /* Set up bit mask of valid inputs */
            gen2g_info.inpMask[0] |= (ALL_PINS_BIT_MASK << (wingNum << 3));

            /* Check if there are any servo outputs, since pins 8 to 15 are inputs,
             *  they can be specially configured as servo outputs
             */
            if (wingNum == 1)
            {
               for (INT input = GEN2G_SERVO_FIRST_INDX;
                  input < GEN2G_SERVO_FIRST_INDX + GEN2G_SERVO_NUM_INP_WING_SERVO; input++)
               {
                  if (dig_info.inpInfo[input].cfg >= SERVO_OUTPUT_THRESH)
                  {
                     gen2g_info.inpMask[0] &= ~(1 << input);
                     gen2g_info.servoMask |= (1 << (input - GEN2G_SERVO_FIRST_INDX));
                     outputMask |= (1 << input);
                  }
               }
            }
            break;
         }
         case WING_SOL:
         {
            outputMask |= (SOL_OUT_BIT_MASK << (wingNum << 3));
            gen2g_info.solMask[0] |= (SOL_MASK << (wingNum << 2));

            /* Set up bit mask of valid inputs */
            gen2g_info.inpMask[0] |= (SOL_INP_BIT_MASK << (wingNum << 3));

            /* Check if there are any servo outputs, since pins 8 to 16 are inputs,
             *  they can be specially configured as servo outputs
             */
            if (wingNum == 1)
            {
               for (INT input = GEN2G_SERVO_FIRST_INDX;
                  input < GEN2G_SERVO_FIRST_INDX + GEN2G_SERVO_NUM_SOL_WING_SERVO; input++)
               {
                  if (dig_info.inpInfo[input].cfg >= SERVO_OUTPUT_THRESH)
                  {
                     gen2g_info.inpMask[0] &= ~(1 << input);
                     gen2g_info.servoMask |= (1 << (input - GEN2G_SERVO_FIRST_INDX));
                     outputMask |= (1 << input);
                  }
               }
            }
            break;
         }
         case WING_SW_MATRIX_OUT:
         case WING_SW_MATRIX_OUT_LOW:
         {
            if ((wingNum == 2) && (gen2g_nv_cfg_p->wingCfg[3] == WING_SW_MATRIX_IN))
            {
               outputMask |= MTRX_OUTPUT_BIT_MASK;
               dig_info.mtrxInpMask |= MTRX_INPUT_BIT_MASK;
               if (gen2g_nv_cfg_p->wingCfg[wingNum] == WING_SW_MATRIX_OUT)
               {
                  gen2g_info.switchMtrxActHigh = TRUE;
               }
            }
            else
            {
               gen2g_info.error = ERR_SW_MATRIX_WING_BAD_LOC;
            }
            break;
         }
         case WING_SW_MATRIX_IN:
         {
            if (wingNum != 3)
            {
               gen2g_info.error = ERR_SW_MATRIX_WING_BAD_LOC;
            }
            break;
         }
         case WING_NEO:
         {
            if (wingNum == 0)
            {
               gen2g_info.inpMask[0] |= NEO_INP_BIT_MASK;
               outputMask |= NEO_OUT_BIT_MASK;

               // If Neo wing is configured for SPI LEDs clock is output
               if ((gen2g_info.neoCfg.bytesPerPixel == 3) &&
                  (gen2g_info.neoCfg.initColor[3] == 0xa5))
               {
                  gen2g_info.inpMask[0] &= ~NEO_SPI_CLK_BIT_MASK;
                  outputMask |= NEO_SPI_CLK_BIT_MASK;
               }
            }
            else
            {
               gen2g_info.error = ERR_NEO_WING_BAD_LOC;
            }
            break;
         }
         case WING_NEO_SOL:
         {
            if (wingNum == 0)
            {
               gen2g_info.inpMask[0] |= NEO_SOL_INP_BIT_MASK;
               gen2g_info.solMask[0] |= SOL_MASK;
               outputMask |= NEO_SOL_OUT_BIT_MASK;
               dig_info.solInfo[0].state.drvBit = 1;

               // If Neo wing is configured for SPI LEDs clock is output
               if ((gen2g_info.neoCfg.bytesPerPixel == 3) &&
                  (gen2g_info.neoCfg.initColor[3] == 0xa5))
               {
                  gen2g_info.inpMask[0] &= ~NEO_SPI_CLK_BIT_MASK;
                  outputMask |= NEO_SPI_CLK_BIT_MASK;
               }
            }
            else
            {
               gen2g_info.error = ERR_NEO_WING_BAD_LOC;
            }
            break;
         }
         case WING_SPI:
         {
            if (wingNum == 3)
            {
               gen2g_info.inpMask[0] |= SPI_INP_BIT_MASK;
               outputMask |= SPI_OUT_BIT_MASK;
            }
            else
            {
               gen2g_info.error = ERR_SPI_WING_BAD_LOC;
            }
            break;
         }
         case WING_INCAND:
         case WING_HI_SIDE_INCAND:
         case WING_LAMP_MATRIX_COL:
         case WING_LAMP_MATRIX_ROW:
         {
            outputMask |= (ALL_PINS_BIT_MASK << (wingNum << 3));
            break;
         }
         case WING_MAX_SOL:
         {
            outputMask |= (MAX_SOL_MASK << (wingNum << 2));
            gen2g_info.solMask[0] |= (MAX_SOL_MASK << (wingNum << 2));
            break;
         }
         default:
         {
            gen2g_info.error = ERR_BAD_WING_TYPE;
            break;
         }
      }
   }

#if GEN2G_DEBUG_PORT != 0
   /* Don't configure debug pins in debug mode */
   gen2g_info.inpMask &= ~DBG_PIN_BIT_MASK;
   outputMask &= ~DBG_PIN_BIT_MASK;
   if (gen2g_nv_cfg_p->wingCfg[0] == WING_MAX_SOL)
   {
	  gen2g_info.solMask &= ~DBG_PIN_BIT_MASK;
   }
#endif

   for (INT pinNum = 0; pinNum < RS232I_NUM_PROC_PINS; pinNum++)
   {
      usedBit = FALSE;
      pinCfg.Pin = dig_pinInfo[pinNum].GPIO_Pin;
      if ((gen2g_info.inpMask[0] & (1 << pinNum)) != 0)
      {
         pinCfg.Mode = GPIO_MODE_INPUT;
         pinCfg.Pull = GPIO_PULLUP;
         usedBit = TRUE;
      }
      else if ((dig_info.mtrxInpMask & (1 << pinNum)) != 0)
      {
         pinCfg.Mode = GPIO_MODE_INPUT;
         if (gen2g_info.switchMtrxActHigh)
         {
            pinCfg.Pull = GPIO_PULLDOWN;
         }
         else
         {
            pinCfg.Pull = GPIO_PULLUP;
         }
         usedBit = TRUE;
      }
      else if ((outputMask & (1 << pinNum)) != 0)
      {
         /* Check if servo output which uses alternate function */
         pinCfg.Speed = GPIO_SPEED_FREQ_LOW;
         if ((pinNum >= GEN2G_SERVO_FIRST_INDX) &&
            (pinNum < GEN2G_SERVO_FIRST_INDX + GEN2G_SERVO_NUM_INP_WING_SERVO) &&
            (gen2g_info.servoMask & 1 << (pinNum - GEN2G_SERVO_FIRST_INDX)))
         {
            pinCfg.Mode = GPIO_MODE_AF_PP;
         }
         else
         {
            pinCfg.Mode = GPIO_MODE_OUTPUT_PP;
         }
         usedBit = TRUE;
      }
      if (usedBit)
      {
         HAL_GPIO_Init(dig_pinInfo[pinNum].port_p, &pinCfg);
      }
   }
   dig_info.filtInputs = stdldigio_read_all_ports(gen2g_info.inpMask[0] | dig_info.mtrxInpMask);

   /* Setup GPB2 as output for status (available on STM32F103CB boards) */
   pinCfg.Pin = GPIO_PIN_2;
   pinCfg.Mode = GPIO_MODE_OUTPUT_PP;
   pinCfg.Speed = GPIO_SPEED_FREQ_LOW;
   HAL_GPIO_Init(GPIOB, &pinCfg);

   /* Previous code set all solenoid inputs to state inputs.  This should
    * happen automatically without needing to do it.
    */

   /* If a matrix wing exists set up thresholds */
   if ((gen2g_info.typeWingBrds & (1 << WING_SW_MATRIX_IN)) != 0)
   {
      /* Grab mtrxWaitCntThresh from config if set */
      if (dig_info.inpInfo[RS232I_MTRX_WAIT_THRESH_INDX].cfg == 0)
      {
         dig_info.mtrxCfg.mtrxWaitCntThresh = MATRIX_WAIT_CNT_THRESH;
      }
      else
      {
         dig_info.mtrxCfg.mtrxWaitCntThresh = dig_info.inpInfo[RS232I_MTRX_WAIT_THRESH_INDX].cfg;
      }

      /* Grab mtrxDebounceThresh from config if set */
      if (dig_info.inpInfo[RS232I_MTRX_DEBOUNCE_THRESH_INDX].cfg == 0)
      {
         dig_info.mtrxCfg.mtrxDebounceThresh = MATRIX_DEBOUNCE_THRESH;
      }
      else
      {
         dig_info.mtrxCfg.mtrxDebounceThresh = dig_info.inpInfo[RS232I_MTRX_DEBOUNCE_THRESH_INDX].cfg;
      }

      /* Initialize matrix input for rs232 reports */
      if (!gen2g_info.switchMtrxActHigh)
      {
         for (INT col = 0; col < RS232I_MATRX_COL; col++)
         {
            gen2g_info.matrixInp[col] = 0xff;
         }
      }
   }

   /* Create solOutMask */
   for (INT procSolNum = 0; procSolNum < RS232I_NUM_PROC_PINS; procSolNum++)
   {
      if ((gen2g_info.solMask[0] & (1 << procSolNum)) != 0)
      {
         dig_info.solOutMask |= dig_info.solInfo[procSolNum].state.drvBit;
      }
   }

   /* Set up the initial state */
   digital_upd_sol_cfg(RS232I_NUM_GEN25_SOL);
   digital_upd_inp_cfg(RS232I_NUM_GEN25_INP);

} /* End digital_init */

/*
 * ===============================================================================
 * 
 * Name: digital_task
 * 
 * ===============================================================================
 */
/**
 * Task for polling inputs
 * 
 * @param   None 
 * @return  None
 * 
 * @pre     None 
 * @note    None
 * 
 * ===============================================================================
 */
void digital_task(void)
{
   DIG_INP_INFO_T             *inpInfo_p;
   DIG_SOL_INFO_T             *solInfo_p;
   DIG_MTRX_SPI_BIT_INFO_T    *matrixBitInfo_p;
   U32                        inputs = 0;
   U32                        changedBits;
   U32                        updFilterHi;
   U32                        updFilterLow = 0;
   INT                        index;
   U8                         data;
   U8                         chngU8;
   U32                        currBit;
   INT                        elapsedTimeMs;
   U32                        currMsMask;

#define SWITCH_THRESH         16
#define PWM_PERIOD            16
#define MIN_OFF_INC           0x10
   
   if (gen2g_info.validCfg)
   {
      /* Grab the inputs */
      inputs = stdldigio_read_all_ports(gen2g_info.inpMask[0]);
      if ((gen2g_info.typeWingBrds & ((1 << WING_INP) | (1 << WING_SOL))) != 0)
      {
         /* See what bits have changed */
         changedBits = (dig_info.prevInputs ^ inputs) & gen2g_info.inpMask[0];
         updFilterHi = 0;
         updFilterLow = 0;
         
         /* Perform input processing for both input and solenoid boards */
         for (index = 0, currBit = 1, inpInfo_p = &dig_info.inpInfo[0];
            index < RS232I_NUM_GEN2_INP; index++, currBit <<= 1, inpInfo_p++)
         {
            if (currBit & gen2g_info.inpMask[0])
            {
               /* Check if this count has changed */
               if (changedBits & currBit)
               {
                  inpInfo_p->state.cnt = 0;
               }
               else
               {
                  if (inpInfo_p->state.cnt <= SWITCH_THRESH)
                  {
                     inpInfo_p->state.cnt++;
                  }
                  if (inpInfo_p->state.cnt == SWITCH_THRESH)
                  {
                     if (inputs & currBit)
                     {
                        updFilterHi |= currBit;
                        dig_info.filtInputs |= currBit;
                        if (inpInfo_p->cfg == RISE_EDGE)
                        {
                           DisableInterrupts;
                           gen2g_info.validSwitch |= currBit;
                           EnableInterrupts;
                           gen2g_info.inpTimestamp[index] = (U16)timer_get_ms_count();
                        }
                     }
                     else
                     {
                        updFilterLow |= currBit;
                        dig_info.filtInputs &= ~currBit;
                        if (inpInfo_p->cfg == FALL_EDGE)
                        {
                           DisableInterrupts;
                           gen2g_info.validSwitch |= currBit;
                           EnableInterrupts;
                           gen2g_info.inpTimestamp[index] = (U16)timer_get_ms_count();
                        }
                     }
                  }
               }
            }
         }
      }

      if ((gen2g_info.typeWingBrds & (1 << WING_SW_MATRIX_IN)) != 0)
      {
         if (dig_info.mtrxCfg.waitCnt >= dig_info.mtrxCfg.mtrxWaitCntThresh)
         {
            dig_info.mtrxCfg.waitCnt = 0;
    	    data = (U8)(stdldigio_read_all_ports(dig_info.mtrxInpMask) >> 24);
            chngU8 = data ^ gen2g_info.matrixPrev[dig_info.mtrxCfg.column];
            gen2g_info.matrixPrev[dig_info.mtrxCfg.column] = data;
            matrixBitInfo_p = &dig_info.mtrxSpi.info[dig_info.mtrxCfg.column * 8];
            for (index = 0, currBit = 1; index < 8; index++, currBit <<= 1, matrixBitInfo_p++)
            {
               /* If bit has changed, reset count */
               if ((currBit & chngU8) != 0)
               {
                  matrixBitInfo_p->cnt = 0;
               }
               else
               {
                  if (matrixBitInfo_p->cnt <= dig_info.mtrxCfg.mtrxDebounceThresh)
                  {
                     matrixBitInfo_p->cnt++;
                  }
                  /* Just passed the threshold so update data bit */
                  if (matrixBitInfo_p->cnt == dig_info.mtrxCfg.mtrxDebounceThresh)
                  {
                     if (((data & currBit) && gen2g_info.switchMtrxActHigh) ||
                       (((data & currBit) == 0) && !gen2g_info.switchMtrxActHigh))
                     {
                        /* Set or clear bit depending if active high or low */
                        if (gen2g_info.switchMtrxActHigh)
                        {
                           gen2g_info.matrixInp[dig_info.mtrxCfg.column] |= currBit;
                        }
                        else
                        {
                           gen2g_info.matrixInp[dig_info.mtrxCfg.column] &= ~currBit;
                        }
                        if (matrixBitInfo_p->sol != 0)
                        {
                           gen2g_info.solDrvProcCtl |=
                              (1 << (matrixBitInfo_p->sol & MATRIX_SOL_MASK));
                        }
                     }
                  }
               }
            }
         }

         if (dig_info.mtrxCfg.waitCnt == 0)
         {
            /* Move to next column */
            dig_info.mtrxCfg.column++;
            if (dig_info.mtrxCfg.column >= RS232I_MATRX_COL)
            {
               dig_info.mtrxCfg.column = 0;
            }
         
#if GEN2G_DEBUG_PORT == 0
            dig_info.outputMask |= MTRX_OUTPUT_BIT_MASK;
#else
            dig_info.outputMask |= DBG_MTRX_OUT_BIT_MASK;
#endif
            /* Reverse column numbering to match Bally documentation */
            if (gen2g_info.switchMtrxActHigh)
            {
               dig_info.outputUpd |= (1 << (MATRIX_COL_OFFS + RS232I_MATRX_COL - 1 - dig_info.mtrxCfg.column));
            }
            else
            {
               dig_info.outputUpd |= ((~(1 << (MATRIX_COL_OFFS + RS232I_MATRX_COL - 1 - dig_info.mtrxCfg.column))) &
                  MATRIX_COL_MASK);
            }
         }
         dig_info.mtrxCfg.waitCnt++;
      }
      
      if ((gen2g_info.typeWingBrds & (1 << WING_SOL)) != 0)
      {
         /* Perform solenoid processing */
         currMsMask = 1 << ((timer_get_ms_count() & 0xf) * 2);
         if (timer_get_us_count() >= 500)
         {
            currMsMask <<= 1;
         }

         /* Update sol output bits every time */
         dig_info.outputMask |= dig_info.solOutMask;
         for (index = 0, currBit = 1, solInfo_p = &dig_info.solInfo[0];
            index < RS232I_NUM_GEN2_SOL; index++, currBit <<= 1, solInfo_p++)
         {
            /* Check if processor is requesting a kick, or an input changed */
            if ((solInfo_p->state.solState == SOL_STATE_IDLE) &&
               ((gen2g_info.solDrvProcCtl & currBit) ||
               (updFilterLow & solInfo_p->state.inpBits)))
            {
               /* Check if processor is kicking normal solenoid */
               if ((solInfo_p->cfg.cfg & (ON_OFF_SOL | DLY_KICK_SOL)) == 0)
               {
                  /* Start the solenoid kick */
                  solInfo_p->state.solState = SOL_INITIAL_KICK;
                  solInfo_p->state.startMs = timer_get_ms_count();
                  dig_info.outputUpd |= solInfo_p->state.drvBit;
               }
               else if ((solInfo_p->cfg.cfg & ON_OFF_SOL) != 0)
               {
            	   solInfo_p->state.solState = SOL_FULL_ON_SOLENOID;
                  dig_info.outputUpd |= solInfo_p->state.drvBit;
               }
               else if ((solInfo_p->cfg.cfg & DLY_KICK_SOL) != 0)
               {
                  solInfo_p->state.solState = SOL_WAIT_BEFORE_KICK;
                  solInfo_p->state.startMs = timer_get_ms_count();
               }
               if ((solInfo_p->cfg.cfg & AUTO_CLR) &&
                  (gen2g_info.solDrvProcCtl & currBit))
               {
                  gen2g_info.solDrvProcCtl &= ~currBit;
                  solInfo_p->state.clearRcvd = TRUE;
               }
               else
               {
                  solInfo_p->state.clearRcvd = FALSE;
               }
            }
            else if (solInfo_p->state.solState == SOL_INITIAL_KICK)
            {
               if ((solInfo_p->cfg.cfg & CAN_CANCEL) != 0)
               {
                  if (((gen2g_info.solDrvProcCtl & currBit) == 0) &&
                     ((solInfo_p->state.inpBits == 0) ||
                     ((solInfo_p->state.inpBits & inputs) == solInfo_p->state.inpBits)))
                  {
                     /* Switch is inactive, move to idle */
                     solInfo_p->state.solState = SOL_STATE_IDLE;
                  }
               }
            
               if (solInfo_p->state.solState == SOL_INITIAL_KICK)
               {
                  /* Check if elapsed time is over initial kick time */
                  elapsedTimeMs = timer_get_ms_count() - solInfo_p->state.startMs;
                  if (elapsedTimeMs >= solInfo_p->cfg.initKick)
                  {
                     /* If this is a normal solenoid */
                     if ((solInfo_p->cfg.cfg & (ON_OFF_SOL | DLY_KICK_SOL | USE_MATRIX_INP)) == 0)
                     {
                        /* See if this has a sustaining PWM */
                        if (solInfo_p->cfg.minOffDuty & DUTY_CYCLE_MASK)
                        {
                           /* Make sure the input continues to be set */
                           if (solInfo_p->state.clearRcvd)
                           {
                              solInfo_p->state.solState = SOL_MIN_TIME_OFF;
                              solInfo_p->state.offCnt = 0;
                           }
                           else
                           {
                              /* Grab holdIntenMask */
                              solInfo_p->state.solState = SOL_SUSTAIN_PWM;
                              solInfo_p->state.holdIntenMask =
                                 PWM_MASK[(((solInfo_p->cfg.minOffDuty & DUTY_CYCLE_MASK) - 1) * 2) + 1 -
								   ((solInfo_p->cfg.minOffDuty & DUTY_CYCLE_MSb) >> 7)];
                              if (currMsMask & solInfo_p->state.holdIntenMask)
                              {
                                 dig_info.outputUpd |= solInfo_p->state.drvBit;
                              }
                           }
                        }
                        else
                        {
                           solInfo_p->state.solState = SOL_MIN_TIME_OFF;
                           solInfo_p->state.offCnt = 0;
                        }
                     }
                     else if ((solInfo_p->cfg.cfg & (DLY_KICK_SOL | USE_MATRIX_INP)) != 0)
                     {
                        solInfo_p->state.solState = SOL_MIN_TIME_OFF;
                        solInfo_p->state.offCnt = 0;
                     }
                     solInfo_p->state.startMs = timer_get_ms_count();
                  }
                  else if (currMsMask & solInfo_p->state.kickIntenMask)
                  {
                     dig_info.outputUpd |= solInfo_p->state.drvBit;
                  }
               }
            }
            else if (solInfo_p->state.solState == SOL_SUSTAIN_PWM)
            {
               if (((gen2g_info.solDrvProcCtl & currBit) == 0) &&
			     ((solInfo_p->state.inpBits == 0) ||
                 ((solInfo_p->state.inpBits & inputs) == solInfo_p->state.inpBits)))
               {
                  solInfo_p->state.clearRcvd = TRUE;
               }
               if (!solInfo_p->state.clearRcvd)
               {
                  /* Do faster PWM function by testing us timer */
                  if (currMsMask & solInfo_p->state.holdIntenMask)
                  {
                     dig_info.outputUpd |= solInfo_p->state.drvBit;
                  }
               }
               else
               {
                  /* Switch is inactive, move to idle */
                  solInfo_p->state.solState = SOL_STATE_IDLE;
               }
            }
            else if (solInfo_p->state.solState == SOL_MIN_TIME_OFF)
            {
               /* Check if an off time increment has happened */
               elapsedTimeMs = timer_get_ms_count() - solInfo_p->state.startMs;
               if (elapsedTimeMs >= solInfo_p->cfg.initKick)
               {
                  solInfo_p->state.offCnt += MIN_OFF_INC;
                  if (solInfo_p->state.offCnt >= (solInfo_p->cfg.minOffDuty & MIN_OFF_MASK))
                  {
                     solInfo_p->state.solState = SOL_STATE_IDLE;
                  }
                  else
                  {
                     solInfo_p->state.startMs = timer_get_ms_count();
                  }
               }
            }
            else if (solInfo_p->state.solState == SOL_WAIT_BEFORE_KICK)
            {
               /* Check if elapsed time is over the wait time
                * (stored in duty cycle nibble * 2)
                */
               elapsedTimeMs = timer_get_ms_count() - solInfo_p->state.startMs;
               if (elapsedTimeMs >= ((solInfo_p->cfg.minOffDuty & DUTY_CYCLE_MASK) << 1) +
                  ((solInfo_p->cfg.minOffDuty & DUTY_CYCLE_MSb) >> 2))
               {
                  /* Start the solenoid kick */
                  solInfo_p->state.solState = SOL_INITIAL_KICK;
                  solInfo_p->state.startMs = timer_get_ms_count();
                  dig_info.outputUpd |= solInfo_p->state.drvBit;
               }
            }
            else if (solInfo_p->state.solState == SOL_FULL_ON_SOLENOID)
            {
               if (((gen2g_info.solDrvProcCtl & currBit) == 0) &&
			     ((solInfo_p->state.inpBits == 0) ||
                 ((solInfo_p->state.inpBits & inputs) == solInfo_p->state.inpBits)))
               {
                  /* Switch is inactive, move to idle */
                  solInfo_p->state.solState = SOL_STATE_IDLE;
               }
               else if (currMsMask & solInfo_p->state.kickIntenMask)
               {
                  dig_info.outputUpd |= solInfo_p->state.drvBit;
               }
            }
         }
      }

      if ((gen2g_info.typeWingBrds & ((1 << WING_INP) | (1 << WING_SOL))) != 0)
      {
         DisableInterrupts;
         gen2g_info.validSwitch = (gen2g_info.validSwitch & ~dig_info.stateMask) |
            (dig_info.filtInputs & dig_info.stateMask);
         EnableInterrupts;
         dig_info.prevInputs = inputs;
      }
   }
} /* End digital_task */

/*
 * ===============================================================================
 * 
 * Name: digital_set_solenoid_input
 * 
 * ===============================================================================
 */
/**
 * Set a solenoid input
 *
 * Set a solenoid input.  To disable a solenoid, the input solenoid number can be
 * set to SOL_INP_CLEAR_SOL.
 * 
 * @param   None 
 * @return  None
 * 
 * @pre     None 
 * @note    If multiple inputs are used to fire a solenoid, the inputs are
 *    logically OR'd together.
 * 
 * ===============================================================================
 */
void digital_set_solenoid_input(
   U8                         inpIndex,
   RS232I_SET_SOL_INP_E       solIndex)
{
   DIG_SOL_STATE_T            *solState_p;
   
   if (inpIndex < RS232I_NUM_GEN2_INP)
   {
      solState_p = &dig_info.solInfo[solIndex & SOL_INP_SOL_MASK].state;
      if ((solIndex & SOL_INP_CLEAR_SOL) == 0)
      {
         solState_p->inpBits |= (1 << inpIndex);
      }
      else
      {
         solState_p->inpBits &= ~(1 << inpIndex);
      }
   }
   else if ((inpIndex - RS232I_NUM_GEN2_INP) < RS232I_NUM_SW_MATRX_SPI_INP)
   {
      /* Inputs 32 to 96 are from the switch matrix or SPI, first verify
       * there is a switch matrix or SPI.
       */
      if (((gen2g_info.typeWingBrds & (1 << WING_SW_MATRIX_IN)) != 0) ||
         ((gen2g_info.typeWingBrds & (1 << WING_SPI)) != 0))
      {
         if (solIndex & SOL_INP_CLEAR_SOL)
         {
            dig_info.mtrxSpi.info[inpIndex - RS232I_NUM_GEN2_INP].sol = 0;
         }
         else
         {
            dig_info.mtrxSpi.info[inpIndex - RS232I_NUM_GEN2_INP].sol =
               MATRIX_FIRE_SOL | (solIndex & SOL_INP_SOL_MASK);
         }
      }
   }
} /* End digital_set_solenoid_input */

/*
 * ===============================================================================
 * 
 * Name: digital_set_kick_pwm
 *
 * ===============================================================================
 */
/**
 * Set kick PWM
 *
 * Set a solenoid's power during the kick portion.  PWM is a number from 0-32
 * where 0 is off, and 32 is 100% power.
 *
 * @param   kickPwm value from 0-32
 * @param   solIndex index of solenoid (0-15)
 * @return  None
 *
 * @pre     None
 *
 * ===============================================================================
 */
void digital_set_kick_pwm(
   U8                         kickPwm,
   U8                         solIndex)
{
   if (solIndex < RS232I_NUM_GEN2_SOL)
   {
      dig_info.solInfo[solIndex].state.kickIntenMask = PWM_MASK[kickPwm & 0x1f];
   }
} /* End digital_set_kick_pwm */

/*
 * ===============================================================================
 *
 * Name: digital_upd_ind_sol_cfg
 *
 * ===============================================================================
 */
/**
 * Update individual solenoid configuration
 *
 * Update a solenoid configuration.
 *
 * @param   solNum - Number of solenoid to be updated
 * @return  None
 *
 * @pre     None
 * @note    None
 *
 * ===============================================================================
 */
void digital_upd_ind_sol_cfg(
   U32                        solNum)
{
   INT                        solGrp = solNum >> 5;
   INT                        solBit = 1 << (solNum & 0x1f);

   if (gen2g_info.solMask[solGrp] & solBit)
   {
      DIG_SOL_INFO_T *solInfo_p = &dig_info.solInfo[solNum];
      solInfo_p->state.solState = SOL_STATE_IDLE;
      if (solNum < RS232I_NUM_GEN2_SOL)
      {
         U32 inputIndex = ((solNum & 0x0c) << 1) + (solNum & 0x03);
         U32 inputBit = 1 << inputIndex;
         if (solInfo_p->cfg.cfg & USE_SWITCH)
         {
            if (gen2g_info.inpMask[solGrp] & inputBit)
            {
               solInfo_p->state.inpBits |= inputBit;
               dig_info.inpInfo[solNum].state.cnt = 0;
            }
         }
         else
         {
            solInfo_p->state.inpBits &= ~inputBit;
         }
      }
      else if (solGrp > 0)
      {
         if (solInfo_p->cfg.cfg & USE_SWITCH)
         {
            /* SPI solenoids use same input bit as output bit */
            dig_info.mtrxSpi.info[solNum].sol = MATRIX_FIRE_SOL | solNum;
         }
         else if (solInfo_p->cfg.cfg & USE_MATRIX_INP)
         {
            /* SPI can be configured for using other matrix bits */
            dig_info.mtrxSpi.info[solInfo_p->cfg.minOffDuty].sol = MATRIX_FIRE_SOL | solNum;
         }
      }
   }
} /* End digital_upd_ind_sol_cfg */

/*
 * ===============================================================================
 *
 * Name: digital_upd_sol_cfg
 * 
 * ===============================================================================
 */
/**
 * Update solenoid configurations
 *
 * Update solenoid configurations.
 * 
 * @param   numSol - Number of solenoids to be updated.
 * @return  None
 * 
 * @pre     None 
 * @note    None
 * 
 * ===============================================================================
 */
void digital_upd_sol_cfg(
   U32                        numSol)
{
   for (U32 solNum = 0; solNum < numSol; solNum++)
   {
      digital_upd_ind_sol_cfg(solNum);
   }
} /* End digital_upd_sol_cfg */

/*
 * ===============================================================================
 * 
 * Name: digital_upd_ind_inp_cfg
 *
 * ===============================================================================
 */
/**
 * Update individual input configuration
 *
 * Update an input configuration.
 *
 * @param   inpNum - Number of input to be updated
 * @return  None
 *
 * @pre     None
 * @note    None
 *
 * ===============================================================================
 */
void digital_upd_ind_inp_cfg(
   U32                        inpNum)
{
   INT                        inpGrp = inpNum >> 5;
   INT                        inpBit = 1 << (inpNum & 0x1f);

   if (gen2g_info.inpMask[inpGrp] & inpBit)
   {
      DIG_INP_INFO_T *inpInfo_p = &dig_info.inpInfo[inpNum];
      inpInfo_p->state.cnt = 0;
      if ((inpInfo_p->cfg < SERVO_OUTPUT_THRESH) && (inpGrp == 0))
      {
         if (inpInfo_p->cfg == STATE_INPUT)
         {
            dig_info.stateMask |= inpBit;
         }
         else
         {
            dig_info.stateMask &= ~inpBit;
         }
      }
   }
} /* End digital_upd_ind_inp_cfg */

/*
 * ===============================================================================
 *
 * Name: digital_set_inp_cfg
 *
 * ===============================================================================
 */
/**
 * Set an input configuration
 *
 * @param   inpNum - Number of input to be updated
 * @param   inpCfg - Input configuration
 * @return  None
 *
 * @pre     None
 * @note    None
 *
 * ===============================================================================
 */
void digital_set_inp_cfg(
   U32                        inpNum,
   RS232I_CFG_INP_TYPE_E      inpCfg)
{
   dig_info.inpInfo[inpNum].cfg = inpCfg;
} /* End digital_set_inp_cfg */

/*
 * ===============================================================================
 *
 * Name: digital_get_inp_cfg
 *
 * ===============================================================================
 */
/**
 * Get an input configuration
 *
 * @param   inpNum - Number of input to be updated
 * @return  None
 *
 * @pre     None
 * @note    None
 *
 * ===============================================================================
 */
RS232I_CFG_INP_TYPE_E digital_get_inp_cfg(
   U32                        inpNum)
{
   return(dig_info.inpInfo[inpNum].cfg);
} /* End digital_get_inp_cfg */

/*
 * ===============================================================================
 *
 * Name: digital_upd_inp_cfg
 * 
 * ===============================================================================
 */
/**
 * Update input configurations
 *
 * Update input configurations.
 * 
 * @param   numInp - Number of inputs to be updated.
 * @return  None
 * 
 * @pre     None 
 * @note    None
 * 
 * ===============================================================================
 */
void digital_upd_inp_cfg(
   U32                        numInp)
{
   for (U32 inpNum = 0; inpNum < numInp; inpNum++)
   {
      digital_upd_ind_inp_cfg(inpNum);
   }
} /* End digital_upd_inp_cfg */

/*
 * ===============================================================================
 *
 * Name: digital_upd_outputs
 *
 * ===============================================================================
 */
/**
 * Update outputs
 *
 * Update output bits in temporary registers.
 *
 * @param   value - Value of outputs to be updated.
 * @param   mask - Mask of outputs to be updated.
 * @return  None
 *
 * @pre     None
 * @note    None
 *
 * ===============================================================================
 */
void digital_upd_outputs(
   U32                        value,
   U32                        mask)
{
   dig_info.outputUpd |= value;
   dig_info.outputMask |= mask;
} /* End digital_upd_outputs */

/*
 * ===============================================================================
 *
 * Name: digital_write_outputs
 *
 * ===============================================================================
 */
/**
 * Write outputs to pins
 *
 * Write cached output updates to pins.
 *
 * @param   None
 * @return  None
 *
 * @pre     None
 * @note    None
 *
 * ===============================================================================
 */
void digital_write_outputs()
{
   stdldigio_write_all_ports(dig_info.outputUpd, dig_info.outputMask);

   dig_info.outputUpd = 0;
   dig_info.outputMask = 0;
} /* End digital_write_outputs */

/*
 * ===============================================================================
 *
 * Name: digital_convert_v0_sol_cfg_to_v1_solcfg
 *
 * ===============================================================================
 */
/**
 * Convert version 0 solenoid config to version 1 solenoid config
 *
 * Convert to the newer version of the solenoid config.  Copy to current RAM cfg.
 *
 * @param   None
 * @param   None
 * @return  None
 *
 * @pre     None
 * @note    None
 *
 * ===============================================================================
 */
void digital_convert_v0_sol_cfg_to_v1_solcfg(
   INT                        solIndex,
   RS232I_SOL_CFG_T           *cfg_p)
{
   dig_info.solInfo[solIndex].cfg.cfg = cfg_p->cfg;
   dig_info.solInfo[solIndex].cfg.initKick = cfg_p->initKick;
   dig_info.solInfo[solIndex].cfg.minOffDuty = cfg_p->minOffDuty;
   if (cfg_p->cfg & DLY_KICK_SOL)
   {
      dig_info.solInfo[solIndex].cfg.delayMs = ((cfg_p->minOffDuty & DUTY_CYCLE_MASK) << 1) +
         ((cfg_p->minOffDuty & DUTY_CYCLE_MSb) >> 2);
   }
} /* End digital_convert_v0_sol_cfg_to_v1_solcfg */

/*
 * ===============================================================================
 *
 * Name: digital_convert_v0_cfg_to_v1_cfg
 *
 * ===============================================================================
 */
/**
 * Convert version 0 config to version 1 config
 *
 * Convert to the newer version of the config.  Copy to current RAM cfg.
 *
 * @param   None
 * @param   None
 * @return  None
 *
 * @pre     None
 * @note    None
 *
 * ===============================================================================
 */
U8 *digital_convert_v0_cfg_to_v1_cfg()
{
   BOOL                       hasSol = FALSE;
   BOOL                       hasNeo = FALSE;
   U8                         *cfg_p = GEN2G_V0_CFG_DATA_ADDR;

   // Copy input configs
   for (INT inpIndex = 0; inpIndex < RS232I_NUM_GEN2_INP; inpIndex++)
   {
	   dig_info.inpInfo[inpIndex].cfg = *(RS232I_CFG_INP_TYPE_E *)cfg_p++;
   }
   for (INT wingIndex = 0; wingIndex < RS232I_NUM_PROC_WINGS; wingIndex++)
   {
      if ((gen2g_nv_cfg_p->wingCfg[wingIndex] == WING_SOL) ||
         (gen2g_nv_cfg_p->wingCfg[wingIndex] == WING_NEO_SOL))
      {
         hasSol = TRUE;
      }
      if ((gen2g_nv_cfg_p->wingCfg[wingIndex] == WING_NEO) ||
         (gen2g_nv_cfg_p->wingCfg[wingIndex] == WING_NEO_SOL))
      {
         hasNeo = TRUE;
      }
   }
   if (hasSol)
   {
      for (INT solIndex = 0; solIndex < RS232I_NUM_GEN2_SOL; solIndex++)
      {
         digital_convert_v0_sol_cfg_to_v1_solcfg(solIndex, (RS232I_SOL_CFG_T *)cfg_p);
         cfg_p += sizeof(RS232I_SOL_CFG_T);
      }
   }
   if (hasNeo)
   {
      for (INT index = 0; index < sizeof(GEN2G_NEO_CFG_T); index++)
      {
         ((U8 *)&gen2g_info.neoCfg)[index] = *cfg_p++;
      }
   }
   return (cfg_p);
} /* End digital_convert_v0_cfg_to_v1_cfg */

/*
 * ===============================================================================
 *
 * Name: digital_copy_v1_cfg
 *
 * ===============================================================================
 */
/**
 * Copy version 1 config RAM
 *
 * Copy from FLASH to the active configuration in RAM
 *
 * @param   None
 * @param   None
 * @return  None
 *
 * @pre     None
 * @note    None
 *
 * ===============================================================================
 */
void digital_copy_v1_cfg()
{
   // Copy input configs
   RS232I_CFG_INP_TYPE_E *inpCfg_p = GEN2G_V1_INP_CFG_ADDR;
   for (INT inpIndex = 0; inpIndex < RS232I_NUM_GEN25_INP; inpIndex++)
   {
	   dig_info.inpInfo[inpIndex].cfg = *inpCfg_p++;
   }

   // Copy sol configs
   RS232I_SOL25_CFG_T *solCfg_p = GEN2G_V1_SOL_CFG_ADDR;
   for (INT solIndex = 0; solIndex < RS232I_NUM_GEN25_SOL; solIndex++)
   {
      *(U32 *)&dig_info.solInfo[solIndex] = *(U32 *)solCfg_p++;
   }
} /* End digital_copy_v1_cfg */
