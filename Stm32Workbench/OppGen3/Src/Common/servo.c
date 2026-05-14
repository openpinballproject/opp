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
 * @file:   servo.c
 * @author: Hugh Spahr
 * @date:   9/17/2021
 *
 * @note:   Open Pinball Project
 *          Copyright© 2015 - 2021, Hugh Spahr
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
 * This is the file for servos using a pins on wing 1.  This uses a
 * TIM2, TIM3 and TIM4 to send PWM outputs to control servo motors.
 *
 * This file requires a 10ms tick to start the servo processing.
 *
 *===============================================================================
 */
#include <stdlib.h>
#include "stdtypes.h"
#include "procdefs.h"
#include "gen2glob.h"
#include "fadeintf.h"

#define MAX_SERVOS               8

#define SERVO_WAIT_FADE_DONE     1
#define SERVO_WAIT_FOR_TICK      2
#define SERVO_DISABLED           0xff

#define SERVO_TIM2          0x10
#define SERVO_TIM3          0x20
#define SERVO_TIM4          0x40
#define SERVO_COUNTER_MASK  0x0f
#define SERVO_TIM_MASK      0xf0

const U8                    SERVO_TIM_OFFS[MAX_SERVOS] =
{ SERVO_TIM2, SERVO_TIM2 | 1, SERVO_TIM3, SERVO_TIM3 | 1,
  SERVO_TIM4, SERVO_TIM4 | 1, SERVO_TIM4 | 2, SERVO_TIM4 | 3
};

/* Prototypes */
void servo_move_proc(
   INT                  offset,
   U8                   newData);
void servo_end_move_proc();
RS232I_CFG_INP_TYPE_E digital_get_inp_cfg(
   U32                        inpNum);

/*
 * ===============================================================================
 * 
 * Name: servo_init
 * 
 * ===============================================================================
 */
/**
 * Initialize servo processing
 * 
 * Initialize timers and set to default output, allocate memory, and reset state machine.
 * 
 * @param   None 
 * @return  None
 * 
 * @pre     None 
 * @note    None
 * 
 * ===============================================================================
 */
void servo_init()
{
   INT               index;
   U8                *currPwmVal_p;       /* Ptr to array of current PWM values */
   U8                *newPwmVal_p;        /* Ptr to array of future PWM values */
   U8                counter;
   BOOL              useTim2 = FALSE;
   BOOL              useTim3 = FALSE;
   BOOL              useTim4 = FALSE;
   BOOL              resetTim = FALSE;

   if (gen2g_info.servoMask)
   {
      /* Calculate maximum number of bytes to process at a time */
      fade_init_rec(RS232I_FADE_SERVO_OFFSET, MAX_SERVOS,
         &currPwmVal_p, &newPwmVal_p, servo_move_proc, servo_end_move_proc);

      if (gen2g_info.error == NO_ERRORS)
      {
         /* Set initial values to verify PWMs are working */
         TIMxRegs* volatile timBase_p;
         for (index = 0; index < MAX_SERVOS; index++)
         {
            /* Check if this is a servo */
            if (gen2g_info.servoMask & (1 << index))
            {
               /* Disable initial pulses if config value = 0xff */
               counter = digital_get_inp_cfg(index + GEN2G_SERVO_FIRST_INDX);
               if (counter == 0xff)
               {
            	   counter = 0;
               }
               *(currPwmVal_p + index) = counter;
               *(newPwmVal_p + index) = counter;
               if (SERVO_TIM_OFFS[index] & SERVO_TIM2)
               {
                  timBase_p = tim2Base_p;
                  if (!useTim2)
                  {
                     /* Enable altFunc clock, TIM2 clock, and TIM2 partial remap (01) */
                     rccBase_p->APB2ENR |= 0x00000001;
                     rccBase_p->APB1ENR |= 0x00000001;
                     afioBase_p->MAPR |= 0x00000100;
                     useTim2 = TRUE;
                     resetTim = TRUE;
                  }
               }
               else if (SERVO_TIM_OFFS[index] & SERVO_TIM3)
               {
                  timBase_p = tim3Base_p;
                  if (!useTim3)
                  {
                     /* Enable altFunc clock, TIM3 clock, and TIM3 partial remap (10) */
                     rccBase_p->APB2ENR |= 0x00000001;
                     rccBase_p->APB1ENR |= 0x00000002;
                     afioBase_p->MAPR |= 0x00000800;
                     useTim3 = TRUE;
                     resetTim = TRUE;
                  }
               }
               else
               {
                  timBase_p = tim4Base_p;
                  if (!useTim4)
                  {
                     /* Enable TIM4 clock */
                     rccBase_p->APB1ENR |= 0x00000004;
                     useTim4 = TRUE;
                     resetTim = TRUE;
                  }
               }
               if (resetTim)
               {
                  /* Reset timer */
                  timBase_p->CR1 = 0;
                  timBase_p->CCER = 0;
                  timBase_p->CCMR[0] = 0;
                  timBase_p->CCMR[1] = 0;
                  resetTim = FALSE;
               }
               /* Initialize TIM to 10 us tick, 20 ms period */
               timBase_p->PSC = 479;
               timBase_p->ARR = 1999;
               timBase_p->CCR[SERVO_TIM_OFFS[index] & SERVO_COUNTER_MASK] = (U32)counter;

               /* Set OCxM (PWM mode 1) and OCxPE (allow preloading) */
               timBase_p->CCMR[(SERVO_TIM_OFFS[index] & 0x02) >> 1] |=
                  (0x68 << ((SERVO_TIM_OFFS[index] & 0x01) << 3));

               /* Output enable active high CCR */
               timBase_p->CCER |= (0x1 << ((SERVO_TIM_OFFS[index] & SERVO_COUNTER_MASK) << 2));
            }
         }
         if (useTim2)
         {
            tim2Base_p->CR1 |= 1;
         }
         if (useTim3)
         {
            tim3Base_p->CR1 |= 1;
         }
         if (useTim4)
         {
            tim4Base_p->CR1 |= 1;
         }
      }
   }
}

/* Note:  No servo_10ms_tick and servo_task are needed because the updates are
 * made automatically by the fade processing.  Everything else is run by the PWM
 * within the processor.
 */

/*
 * ===============================================================================
 * 
 * Name: servo_move_proc
 *
 * ===============================================================================
 */
/**
 * Servo move processing
 *
 * Special processing for moving between two servo positions in a given time
 *
 * @param   None
 * @return  None
 *
 * @pre     None
 * @note    None
 *
 * ===============================================================================
 */
void servo_move_proc(
   INT                  offset,
   U8                   newData)
{
   TIMxRegs* volatile timBase_p;
   if (SERVO_TIM_OFFS[offset] & SERVO_TIM2)
   {
      timBase_p = tim2Base_p;
   }
   else if (SERVO_TIM_OFFS[offset] & SERVO_TIM3)
   {
      timBase_p = tim3Base_p;
   }
   else
   {
      timBase_p = tim4Base_p;
   }
   timBase_p->CCR[SERVO_TIM_OFFS[offset] & SERVO_COUNTER_MASK] = (U32)newData;
}

/*
 * ===============================================================================
 *
 * Name: servo_end_move_proc
 *
 * ===============================================================================
 */
/**
 * Servo end move processing
 *
 * Empty function since no special processing needs to be done for servos
 *
 * @param   None
 * @return  None
 *
 * @pre     None
 * @note    None
 *
 * ===============================================================================
 */
void servo_end_move_proc()
{
}

/* [] END OF FILE */
