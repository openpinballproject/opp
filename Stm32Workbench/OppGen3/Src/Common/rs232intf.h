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
 * @file:   rs232intf.h
 * @author: Hugh Spahr
 * @date:   12/07/2012
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
 * RS232 Interface file for open pinball project
 *
 *===============================================================================
 */

#ifndef RS232INTF_H
#define RS232INTF_H

#include "stdtypes.h"   /* include peripheral declarations */
   
#define RS232I_NUM_PROC_WINGS 4
#define RS232I_NUM_GEN25_WING 12
#define RS232I_NUM_GEN2_SOL 16
#define RS232I_NUM_PROC_PINS 32
#define RS232I_NUM_GEN25_SOL  96
#define RS232I_NUM_GEN2_INP 32
#define RS232I_NUM_GEN25_INP  96
#define RS232I_SZ_COLOR_TBL 32
#define RS232I_NUM_SW_MATRX_SPI_INP 64
#define RS232I_MATRX_COL    8
#define RS232I_TIMESTAMP_BYTES 64
#define RS232I_CMD_HDR      2  /* CardAddr + Cmd */
#define RS232I_CRC8_SZ      1
#define RS232I_MAX_TX_BUF_SZ   (RS232I_CMD_HDR + RS232I_TIMESTAMP_BYTES + RS232I_CRC8_SZ)

#define RS232I_FADE_NEO_OFFSET        0x0000
#define RS232I_FADE_INCAND_OFFSET     0x1000
#define RS232I_FADE_LMP_MATRIX_OFFSET 0x2000
#define RS232I_FADE_SERVO_OFFSET      0x3000

/* Each command starts with the Card ID except for inventory and EOM.  Next comes the
 *  command, then any data.
 */
typedef enum
{
  RS232I_GET_SER_NUM        = 0x00,
  RS232I_GET_PROD_ID        = 0x01,
  RS232I_GET_VERS           = 0x02,
  RS232I_SET_SER_NUM        = 0x03,
  RS232I_RESET              = 0x04,
  RS232I_GO_BOOT            = 0x05,
  RS232I_CONFIG_SOL         = 0x06,     /* For each solenoid, CFG_SOL_TYPE,
                                         *  Initial kick, and Duty Cycle.
                                         */
  RS232I_KICK_SOL           = 0x07,     /* Value, mask */
  RS232I_READ_GEN2_INP      = 0x08,     /* Data */
  RS232I_CONFIG_INP         = 0x09,     /* For each input, CFG_INP_TYPE */
  RS232I_GEN2_UNUSED        = 0x0a,     /* Unused */
  RS232I_SAVE_CFG           = 0x0b,
  RS232I_ERASE_CFG          = 0x0c,
  RS232I_GET_GEN2_CFG       = 0x0d,     /* For each wing,  GEN2_WING_TYPE */
  RS232I_SET_GEN2_CFG       = 0x0e,     /* For each wing,  GEN2_WING_TYPE */
  RS232I_CHNG_NEO_CMD       = 0x0f,
  RS232I_CHNG_NEO_COLOR     = 0x10,
  RS232I_CHNG_NEO_COLOR_TBL = 0x11,
  RS232I_SET_NEO_COLOR_TBL  = 0x12,
  RS232I_INCAND_CMD         = 0x13,
  RS232I_CONFIG_IND_SOL     = 0x14,
  RS232I_CONFIG_IND_INP     = 0x15,
  RS232I_SET_IND_NEO        = 0x16,
  RS232I_SET_SOL_INPUT      = 0x17,
  RS232I_UPGRADE_OTHER_BRD  = 0x18,
  RS232I_READ_MATRIX_INP    = 0x19,
  RS232I_GET_INP_TIMESTAMP  = 0x1a,
  RS232I_SOL_KICK_PWM       = 0x1b,
  RS232I_NUM_CMDS,
  
  RS232I_NEO_FADE_CMD       = 0x40,
  RS232I_SPI_CMD            = 0x41,

  RS232I_INVENTORY          = 0xf0,     /* Each card adds byte for card type */
  RS232I_EOM                = 0xff,
} __attribute__((packed)) RS232I_CMD_E;

#ifndef RS232I_INSTANTIATE
 extern
#endif
const U8                    CMD_LEN[RS232I_NUM_CMDS]
#ifdef RS232I_INSTANTIATE
 ={ 4,  /* RS232I_GET_SER_NUM */       4,  /* RS232I_GET_PROD_ID */
    4,  /* RS232I_GET_VERS */          4,  /* RS232I_SET_SER_NUM */
    0,  /* RS232I_RESET */             0,  /* RS232I_GO_BOOT */
    48, /* RS232I_CONFIG_SOL */        4,  /* RS232I_KICK_SOL */
    4,  /* RS232I_READ_GEN2_INP */     32, /* RS232I_CONFIG_INP */
    2,  /* RS232I_GEN2_UNUSED */       0,  /* RS232I_SAVE_CFG */
    0,  /* RS232I_ERASE_CFG */         4,  /* RS232I_GET_GEN2_CFG */
    4,  /* RS232I_SET_GEN2_CFG */      6,  /* RS232I_CHNG_NEO_CMD */
    6,  /* RS232I_CHNG_NEO_COLOR */    4,  /* RS232I_CHNG_NEO_COLOR_TBL */
    97, /* RS232I_SET_NEO_COLOR_TBL */ 5,  /* RS232I_INCAND_CMD */
    4,  /* RS232I_CONFIG_IND_SOL */    2,  /* RS232I_CONFIG_IND_INP */
    2,  /* RS232I_SET_IND_NEO */       2,  /* RS232I_SET_SOL_INPUT */
    0,  /* RS232I_UPGRADE_OTHER_BRD */ 8,  /* RS232I_READ_MATRIX_INP */
    0,  /* RS232I_GET_INP_TIMESTAMP */ 2,  /* RS232I_SOL_KICK_PWM */
  }
#endif
;
/* Note:  This is the length of the cmd excluding card ID, cmd and CRC */

typedef enum
{
  CARD_ID_CARD_NUM_MASK     = 0x0f,

  CARD_ID_TYPE_MASK         = 0xf0,
  CARD_ID_SOL_CARD          = 0x00,
  CARD_ID_INP_CARD          = 0x10,
  CARD_ID_GEN2_CARD         = 0x20,
  
} __attribute__((packed)) RS232I_CARD_ID_E;

typedef enum
{
  USE_SWITCH                = 0x01,
  AUTO_CLR                  = 0x02,
  ON_OFF_SOL                = 0x04,
  DLY_KICK_SOL              = 0x08,
  USE_MATRIX_INP            = 0x10,
  CAN_CANCEL                = 0x20,
} __attribute__((packed)) RS232I_CFG_SOL_TYPE_E;

typedef enum
{
  DUTY_CYCLE_MASK           = 0x0f,   /* lsb 4 bits are duty cycle */
  MIN_OFF_MASK              = 0x70,
  DUTY_CYCLE_MSb            = 0x80,
} __attribute__((packed)) RS232I_DUTY_E;
/* Min off time is 0-7 times the initial kick time.  If initial kick
 * is 20 ms and min off is 5, the solenoid will be forced off for 100 ms
 */

typedef struct
{
   RS232I_CFG_SOL_TYPE_E      cfg;
   U8                         initKick;
   RS232I_DUTY_E              minOffDuty;
}  RS232I_SOL_CFG_T;

typedef struct
{
   RS232I_CFG_SOL_TYPE_E      cfg;
   U8                         initKick;
   RS232I_DUTY_E              minOffDuty;
   U8                         delayMs;
}  RS232I_SOL25_CFG_T;

typedef enum
{
  STATE_INPUT               = 0x00,
  FALL_EDGE                 = 0x01,
  RISE_EDGE                 = 0x02,

  SERVO_OUTPUT_THRESH       = 0x40  /* If wing 1, threshold for pin configured as servo */
} __attribute__((packed)) RS232I_CFG_INP_TYPE_E;

#define RS232I_MTRX_WAIT_THRESH_INDX        16
#define RS232I_MTRX_DEBOUNCE_THRESH_INDX    17

typedef enum
{
  WING_UNUSED               = 0x00,
  WING_SOL                  = 0x01,
  WING_INP                  = 0x02,
  WING_INCAND               = 0x03,
  WING_SW_MATRIX_OUT        = 0x04,
  WING_SW_MATRIX_IN         = 0x05,
  WING_NEO                  = 0x06,
  WING_HI_SIDE_INCAND       = 0x07,
  WING_NEO_SOL              = 0x08,
  WING_SPI                  = 0x09,
  WING_SW_MATRIX_OUT_LOW    = 0x0a,
  WING_LAMP_MATRIX_COL      = 0x0b,
  WING_LAMP_MATRIX_ROW      = 0x0c,
  WING_MAX_SOL              = 0x0d,
  MAX_WING_TYPES,

  WING_UNUSED2              = 0xff
} __attribute__((packed)) RS232I_GEN2_WING_TYPE_E;

typedef enum
{
  INCAND_ROT_LEFT           = 0x00,
  INCAND_ROT_RIGHT          = 0x01,
  INCAND_LED_ON             = 0x02,
  INCAND_LED_OFF            = 0x03,
  INCAND_LED_BLINK_SLOW     = 0x04,
  INCAND_LED_BLINK_FAST     = 0x05,
  INCAND_LED_BLINK_OFF      = 0x06,
  INCAND_LED_SET_ON_OFF     = 0x07,

  INCAND_SET                = 0x80,
  INCAND_SET_ON             = 0x01,
  INCAND_SET_BLINK_SLOW     = 0x02,
  INCAND_SET_BLINK_FAST     = 0x04,
} __attribute__((packed)) RS232I_GEN2_INCAND_CMD_E;

typedef enum
{
  SOL_INP_SOL_MASK          = 0x0f,

  SOL_INP_CLEAR_SOL         = 0x80,
} __attribute__((packed)) RS232I_SET_SOL_INP_E;

typedef enum
{
  SPI_READ                  = 0x01,
  SPI_WRITE                 = 0x02,
} __attribute__((packed)) RS232I_SPI_CMD_E;

#endif
