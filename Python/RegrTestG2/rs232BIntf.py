#!/usr/bin/env python
#
#===============================================================================
#
#                         OOOO
#                       OOOOOOOO
#      PPPPPPPPPPPPP   OOO    OOO   PPPPPPPPPPPPP
#    PPPPPPPPPPPPPP   OOO      OOO   PPPPPPPPPPPPPP
#   PPP         PPP   OOO      OOO   PPP         PPP
#  PPP          PPP   OOO      OOO   PPP          PPP
#  PPP          PPP   OOO      OOO   PPP          PPP
#  PPP          PPP   OOO      OOO   PPP          PPP
#   PPP         PPP   OOO      OOO   PPP         PPP
#    PPPPPPPPPPPPPP   OOO      OOO   PPPPPPPPPPPPPP
#     PPPPPPPPPPPPP   OOO      OOO   PPP
#               PPP   OOO      OOO   PPP
#               PPP   OOO      OOO   PPP
#               PPP   OOO      OOO   PPP
#               PPP    OOO    OOO    PPP
#               PPP     OOOOOOOO     PPP
#              PPPPP      OOOO      PPPPP
#
# @file:   rs232IntfByte.py
# @author: Hugh Spahr
# @date:   2/1/2024
#
# @note:   Open Pinball Project
#          Copyright 2012-2024, Hugh Spahr
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
#===============================================================================
#
# This is the serial port interface file that is included for serial port
# command definitions.  It uses bytes instead of chars.  (12 years ago, chars
# made sense, but now, bytes are the way to go.
#
#===============================================================================

testVers = '00.00.05'

#Public data
GET_SER_NUM_CMD     = 0x00
GET_PROD_ID_CMD     = 0x01
GET_VERS_CMD        = 0x02
SET_SER_NUM_CMD     = 0x03
RESET_CMD           = 0x04
GO_BOOT_CMD         = 0x05
CFG_SOL_CMD         = 0x06
KICK_SOL_CMD        = 0x07
READ_GEN2_INP_CMD   = 0x08
CFG_INP_CMD         = 0x09
SAVE_CFG_CMD        = 0x0b
ERASE_CFG_CMD       = 0x0c
GET_GEN2_CFG        = 0x0d
SET_GEN2_CFG        = 0x0e
CHNG_NEO_CMD        = 0x0f
CHNG_NEO_COLOR      = 0x10
CHNG_NEO_COLOR_TBL  = 0x11
SET_NEO_COLOR_TBL   = 0x12
INCAND_CMD          = 0x13
CFG_IND_SOL_CMD     = 0x14
CFG_IND_INP_CMD     = 0x15
SET_IND_NEO_CMD     = 0x16
SET_SOL_INPUT_CMD   = 0x17
PASS_THRU_CMD       = 0x18
READ_MATRIX_INP_CMD = 0x19
GET_INP_TIMESTAMP   = 0x1a
SOL_KICK_PWM        = 0x1b

RS232I_NEO_FADE_CMD = 0x40

INV_CMD             = 0xf0
EOM_CMD             = 0xff

CARD_ID_TYPE_MASK   = 0xf0
CARD_ID_SOL_CARD    = 0x00
CARD_ID_INP_CARD    = 0x10
CARD_ID_GEN2_CARD   = 0x20

MAX_NUM_GEN2_CARD   = 0x20

NUM_G2_WING_PER_BRD = 4
WING_SOL            = 0x01
WING_INP            = 0x02
WING_INCAND         = 0x03
WING_SW_MATRIX_OUT  = 0x04
WING_SW_MATRIX_IN   = 0x05
WING_NEO            = 0x06
WING_HI_SIDE_INCAND = 0x07
WING_NEO_SOL        = 0x08
WING_SPI            = 0x09
WING_SW_MATRIX_OUT_LOW = 0x0a
WING_LAMP_MATRIX_COL= 0x0b
WING_LAMP_MATRIX_ROW= 0x0c

NUM_G2_INP_PER_BRD  = 32
NUM_G2_MATRIX_INP   = 64
NUM_INP_PER_WING    = 8
CFG_BYTES_PER_INP   = 1
NUM_MATRIX_COL      = 8

CFG_INP_STATE       = 0x00
CFG_INP_FALL_EDGE   = 0x01
CFG_INP_RISE_EDGE   = 0x02

NUM_G2_SOL_PER_BRD  = 16
NUM_SOL_PER_WING    = 4
CFG_BYTES_PER_SOL   = 3
INIT_KICK_OFFSET    = 1
DUTY_CYCLE_OFFSET   = 2
CFG_SOL_DISABLE     = 0x00
CFG_SOL_USE_SWITCH  = 0x01
CFG_SOL_AUTO_CLR    = 0x02
CFG_SOL_ON_OFF      = 0x04
CFG_SOL_DLY_KICK    = 0x08
CFG_SOL_USE_MTRX_INP= 0x10
CFG_SOL_CAN_CANCEL  = 0x20

NUM_COLOR_TBL       = 32

NUM_INCAND_PER_WING = 8
INCAND_ROT_LEFT     = 0x00
INCAND_ROT_RIGHT    = 0x01
INCAND_LED_ON       = 0x02
INCAND_LED_OFF      = 0x03
INCAND_BLINK_SLOW   = 0x04
INCAND_BLINK_FAST   = 0x05
INCAND_BLINK_OFF    = 0x06
INCAND_SET_ON_OFF   = 0x07

INCAND_SET_CMD              = 0x80
INCAND_SET_ON               = 0x01
INCAND_SET_BLINK_SLOW       = 0x02
INCAND_SET_BLINK_FAST       = 0x04

SOL_INP_CLEAR_SOL   = 0x80

NUM_CHARS_CLEAR_PASSTHRU = 65
