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
 * @file:   debug.c
 * @author: Hugh Spahr
 * @date:   11/29/2015
 *
 * @note:   Open Pinball Project
 *          Copyright© 2015, Hugh Spahr
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
 * This is a debug file.  It saves an initial configuration into flash
 * other utilities.
 *
 *===============================================================================
 */
#include "stdtypes.h"
#include "stdlintf.h"
#include "gen2glob.h"
#include "versglob.h"

/* Minimal configuration:  Only contains input configuration.
 */
#define NUM_DFLT_CFG_INP      32
const RS232I_CFG_INP_TYPE_E dfltInpCfg[NUM_DFLT_CFG_INP] = {
   /* All inputs set to state inputs */
   STATE_INPUT, STATE_INPUT, STATE_INPUT, STATE_INPUT, /* 0 - 3 */
   STATE_INPUT, STATE_INPUT, STATE_INPUT, STATE_INPUT, /* 4 - 7 */
   STATE_INPUT, STATE_INPUT, STATE_INPUT, STATE_INPUT, /* 8 - 11 */
   STATE_INPUT, STATE_INPUT, STATE_INPUT, STATE_INPUT, /* 12 - 15 */
   STATE_INPUT, STATE_INPUT, STATE_INPUT, STATE_INPUT, /* 16 - 19 */
   STATE_INPUT, STATE_INPUT, STATE_INPUT, STATE_INPUT, /* 20 - 23 */
   STATE_INPUT, STATE_INPUT, STATE_INPUT, STATE_INPUT, /* 24 - 27 */
   STATE_INPUT, STATE_INPUT, STATE_INPUT, STATE_INPUT, /* 28 - 31 */
};

#define CRC 0xf6
const GEN2G_NV_CFG_T dfltNvCfg = {
   CRC, NVCFG_VERS_1, { 0xff, 0xff },
   { WING_INP, WING_INP, WING_INP, WING_INP,
   WING_UNUSED2, WING_UNUSED2, WING_UNUSED2, WING_UNUSED2,
   WING_UNUSED2, WING_UNUSED2, WING_UNUSED2, WING_UNUSED2 }
};
   
/*
 * ===============================================================================
 * 
 * Name: debug_save_nv_cfg
 * 
 * ===============================================================================
 */
/**
 * Debug save non-volatile cfg
 * 
 * Save non-volatile configuration settings.
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
void debug_save_nv_cfg()
{
   /* Save persistent data */
   gen2g_info.persist.res1[0] = 0xffffffff;
   gen2g_info.persist.res1[1] = 0xffffffff;
   gen2g_info.persist.res2 = 0xffffffff;
   gen2g_info.persist.serNum = gen2g_persist_p->serNum;

   /* Erase sector */
   stdlflash_sector_erase((U16 *)GEN2G_CFG_TBL);
   
   /* Write persistent data back to flash */
   stdlflash_write((U16 *)&gen2g_info.persist,
      (U16 *)GEN2G_PERSIST_TBL, sizeof(GEN2G_PERSIST_T));

   /* Write 32 inputs to state inputs */
   stdlflash_write((U16 *)&dfltInpCfg[0],
      (U16 *)GEN2G_V1_INP_CFG_ADDR, sizeof(dfltInpCfg));

   /* Write wing cfg, version, and precomputed CRC */
   stdlflash_write((U16 *)&dfltNvCfg,
      (U16 *)GEN2G_CFG_TBL, sizeof(dfltNvCfg));

} /* End debug_save_nv_cfg */

/* [] END OF FILE */
