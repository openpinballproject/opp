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
 * @file:   lampMtrx.c
 * @author: Hugh Spahr
 * @date:   2/2/2021
 *
 * @note:   Open Pinball Project
 *          Copyright© 2021, Hugh Spahr
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
 * This is the file for driving the Cobra lamp matrix plank.
 *
 *===============================================================================
 */
#include <stdlib.h>
#include "stdtypes.h"
#include "gen2glob.h"
#include "fadeintf.h"

#define NUM_COLUMNS          8
#define NUM_ROW_BITS         8
#define LAMPMTRX_MAX_LAMPS   NUM_COLUMNS * NUM_ROW_BITS
#define COL_WING_POS         0
#define ROW_WING_POS         1
#define LAMP_DATA_OFFSET     8
#define LAMPMTRX_ALL_ON_MASK 0x0000ff00
#define LAMPMTRX_CTL_MASK    0x0000ffff    /* Includes row and column mask */

typedef struct
{
   BOOL              blinkSlow;
   U8                currCol;
   INT               lastTime;
   U16               *intenData_p;
} LAMPMTRX_INFO;

LAMPMTRX_INFO lampInfo;

/* Prototypes */
void digital_upd_outputs(
   U32                        value,
   U32                        mask);
INT timer_get_ms_count();
U16 timer_get_us_count();
void lampmtrx_fade_proc(
   INT                  offset,
   U8                   newData);
void lampmtrx_end_fade_proc();

/*
 * ===============================================================================
 * 
 * Name: lampmtrx_init
 * 
 * ===============================================================================
 */
/**
 * Initialize the lamp matrix driver
 * 
 * @param   None
 * @return  None
 * 
 * @pre     None 
 * @note    None
 * 
 * ===============================================================================
 */
void lampmtrx_init()
{
   INT                        index;
   U8                         *currPxlVal_p;       /* Ptr to array of current pixel values */
   U8                         *newPxlVal_p;        /* Ptr to array of future pixel values */
   
   if ((gen2g_nv_cfg_p->wingCfg[COL_WING_POS] == WING_LAMP_MATRIX_COL) &&
      (gen2g_nv_cfg_p->wingCfg[ROW_WING_POS] == WING_LAMP_MATRIX_ROW))
   {
      gen2g_info.haveLampMtrx = TRUE;
      lampInfo.blinkSlow = TRUE;
      lampInfo.currCol = 0;
      lampInfo.intenData_p = (U16 *)malloc(sizeof(U16) * LAMPMTRX_MAX_LAMPS);
      if (lampInfo.intenData_p == NULL)
      {
         gen2g_info.error = ERR_MALLOC_FAIL;
      }
      if (gen2g_info.error == NO_ERRORS)
      {
         for (index = 0; index < LAMPMTRX_MAX_LAMPS; index++)
         {
            lampInfo.intenData_p[index] = 0;
         }

         /* Malloc memory for support of fade commands */
         fade_init_rec(RS232I_FADE_LMP_MATRIX_OFFSET, LAMPMTRX_MAX_LAMPS,
            &currPxlVal_p, &newPxlVal_p, lampmtrx_fade_proc, lampmtrx_end_fade_proc);
         if (gen2g_info.error == NO_ERRORS)
         {
            for (index = 0; index < LAMPMTRX_MAX_LAMPS; index++)
            {
               *currPxlVal_p++ = 0;
               *newPxlVal_p++ = 0;
            }
         }
      }
   }
} /* lampmtrx_init */

/*
 * ===============================================================================
 *
 * Name: lampmtrx_fade_proc
 *
 * ===============================================================================
 */
/**
 * Lamp matrix fade processing
 *
 * Special processing for lamp matrices.  Used to fill out fade mask.
 *
 * @param   None
 * @return  None
 *
 * @pre     None
 * @note    None
 *
 * ===============================================================================
 */
void lampmtrx_fade_proc(
   INT                  offset,
   U8                   newData)
{
   /* Disable initial test blinking after receiving first fade command */
   lampInfo.blinkSlow = FALSE;
   lampInfo.intenData_p[offset] = FADE_USEC_DUR[newData >> 3];
}

/*
 * ===============================================================================
 *
 * Name: lampmtrx_end_fade_proc
 *
 * ===============================================================================
 */
/**
 * Lamp matrix end fade processing
 *
 * Special processing occurs when fade processing is completed.
 *
 * @param   None
 * @return  None
 *
 * @pre     None
 * @note    None
 *
 * ===============================================================================
 */
void lampmtrx_end_fade_proc()
{
}

/*
 * ===============================================================================
 * 
 * Name: lampmtrx_task
 * 
 * ===============================================================================
 */
/**
 * Lamp matrix task
 * 
 * Check if a cycle needs to start.  Turn on LEDs as necessary.
 * 
 * @param   None 
 * @return  None
 * 
 * @pre     None 
 * @note    None
 * 
 * ===============================================================================
 */
void lampmtrx_task()
{
   U32                        ledOut;
   INT                        index;
   INT                        currMsTime;
   U16                        currUsTime;
   
   if (gen2g_info.haveLampMtrx)
   {
      /* Check if new cycle needs to be started */
      /* Switching column so no row/col bits to prevent ghosting */
      currMsTime = timer_get_ms_count();
      if (lampInfo.lastTime != currMsTime)
      {
         lampInfo.lastTime = currMsTime;
         lampInfo.currCol++;
         if (lampInfo.currCol >= NUM_COLUMNS)
         {
            lampInfo.currCol = 0;
         }
         ledOut = 0;
      }
      else
      {
         /* Set correct column drive bit */
         ledOut = 1 << lampInfo.currCol;

         /* Calculate bits to update */
         currUsTime = timer_get_us_count();
         if ((gen2g_info.ledStatus & GEN2G_STAT_BLINK_SLOW_ON) &&
            lampInfo.blinkSlow)
         {
            /* For white wood test blinking only light lamps 50% intensity */
            if (currUsTime < 500)
            {
               ledOut |= LAMPMTRX_ALL_ON_MASK;
            }
         }
         else
         {
            for (index = 0; index < NUM_ROW_BITS; index++)
            {
               if (lampInfo.intenData_p[index + (lampInfo.currCol * NUM_ROW_BITS)] > currUsTime)
               {
                  ledOut |= (1 << (index + LAMP_DATA_OFFSET));
               }
            }
         }
      }

      /* Write the new values */
      digital_upd_outputs(ledOut, LAMPMTRX_CTL_MASK);
   }
}

/* [] END OF FILE */
