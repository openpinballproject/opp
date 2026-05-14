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
 * @file:   fade.c
 * @author: Hugh Spahr
 * @date:   1/24/2021
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
 * This is the file for fading NeoPixels, incandescent, and lamp matrices.
 *
 * This file requires a 10ms tick to start the fade processing.
 *
 *===============================================================================
 */
#include <stdlib.h>
#include "stdtypes.h"
#include "procdefs.h"
#include "gen2glob.h"

const U16                   FADE_USEC_DUR[32] =
{
   0,   63,  94,  125, 156, 188, 219, 250,
   281, 313, 344, 375, 406, 438, 469, 500,
   531, 563, 594, 625, 656, 688, 719, 750,
   781, 813, 844, 875, 906, 938, 969, 1000,
};

typedef enum
{
   NEO_REC                    = 0,
   INCAND_REC,
   LAMP_MATRIX_REC,
   SERVO_REC,
   MAX_FADE_RECS
} FADE_REC_T;

/* Illegal fade offset is used to indicate new fade record must be found */
#define ILLEGAL_FADE_OFFSET   0xffff

typedef struct
{
   U16              totTicks;
   U16              currTick;
} FADE_BYTE_INFO;

typedef struct
{
   BOOL              valid;
   INT               numDataBytes;        /* Number of dataBytes (pixels * bytesPerPixel) */
   U8                *currPxlVal_p;       /* Ptr to array of current pixel values */
   U8                *newPxlVal_p;        /* Ptr to array of future pixel values */
   FADE_BYTE_INFO    *fadeByte_p;         /* Current fade byte info being updated */
   void              (*fadeProc_fp)(INT currByteOffset, U8 newData);   /* Used for fade special processing */
   void              (*endFadeProc_fp)(); /* Used when fade processing is complete */
} FADE_INFO_REC;

typedef struct
{
   BOOL              tickOcc;
   BOOL              fadeRunning;
   BOOL              fadeRcvCmdErr;
   U16               rcvTotTicks;         /* Total ticks received */
   INT               totNumBytes;
   INT               maxProcBytes;        /* Max num data bytes to process at a time */
   INT               rcvNumBytes;         /* Number of rcv data bytes */
   INT               rcvOffset;           /* Current rcv offset */
   INT               currByteOffset;      /* Current fade byte offset */
   INT               totBytesProc;
   FADE_INFO_REC     *rcvCmdFadeRec_p;    /* Fade record being updated by rcvd cmd */
   FADE_INFO_REC     *currFadeRec_p;      /* Current fade record being processed */
   FADE_INFO_REC     fadeRec[MAX_FADE_RECS];
} FADE_INFO;

FADE_INFO fadeInfo;

/* Prototypes */
void fade_process_fade_bytes();

/*
 * ===============================================================================
 * 
 * Name: fade_init
 * 
 * ===============================================================================
 */
/**
 * Initialize fade info
 * 
 * Initialize fields in fade info.
 * 
 * @param   None 
 * @return  None
 * 
 * @pre     None 
 * @note    None
 * 
 * ===============================================================================
 */
void fade_init()
{
   INT               index;
   U32               *u32_p;

   /* Init fadeInfo structure */
   for (index = 0, u32_p = (U32 *)&fadeInfo; index < sizeof(fadeInfo)/sizeof(U32); index++)
   {
      *u32_p++ = 0;
   }
}

/*
 * ===============================================================================
 *
 * Name: fade_init_rec
 *
 * ===============================================================================
 */
/**
 * Initialize fade record
 *
 * Allocate memory for fade structures.  Mark record as valid.
 *
 * @param   None
 * @return  None
 *
 * @pre     None
 * @note    None
 *
 * ===============================================================================
 */
void fade_init_rec(
   INT               startOffset,
   INT               numFadeBytes,
   U8                **currPxlVal_pp,
   U8                **newPxlVal_pp,
   void              (*fadeProc_fp)(INT offset, U8 newData),
   void              (*endFadeProc_fp)())
{
   INT               index;
   FADE_BYTE_INFO    *tmpFadeByte_p;

   FADE_INFO_REC*    fadeRec_p;

   fadeRec_p = &fadeInfo.fadeRec[startOffset >> 12];
   fadeRec_p->numDataBytes = numFadeBytes;
   fadeInfo.totNumBytes += numFadeBytes;

   /* Test for null on commands */
   if (gen2g_info.error == NO_ERRORS)
   {
	  fadeRec_p->currPxlVal_p = malloc(numFadeBytes);
      if (fadeRec_p->currPxlVal_p == NULL)
      {
         gen2g_info.error = ERR_MALLOC_FAIL;
      }
   }

   if (gen2g_info.error == NO_ERRORS)
   {
      *currPxlVal_pp = fadeRec_p->currPxlVal_p;
	  fadeRec_p->newPxlVal_p = malloc(numFadeBytes);
      if (fadeRec_p->newPxlVal_p == NULL)
      {
         gen2g_info.error = ERR_MALLOC_FAIL;
      }
   }

   if (gen2g_info.error == NO_ERRORS)
   {
      *newPxlVal_pp = fadeRec_p->newPxlVal_p;
      fadeRec_p->fadeByte_p = malloc(numFadeBytes * sizeof(FADE_BYTE_INFO));
      if (fadeRec_p->fadeByte_p == NULL)
      {
         gen2g_info.error = ERR_MALLOC_FAIL;
      }
      else
      {
         gen2g_info.haveFade = TRUE;
         fadeRec_p->valid = TRUE;
         fadeRec_p->fadeProc_fp = fadeProc_fp;
         fadeRec_p->endFadeProc_fp = endFadeProc_fp;
         for (index = 0, tmpFadeByte_p = fadeRec_p->fadeByte_p;
            index < numFadeBytes; index++, tmpFadeByte_p++)
         {
            tmpFadeByte_p->totTicks = 0;
         }
      }
   }
}

void fade_start()
{
   /* Calculate maximum number of bytes to process at a time */
   fadeInfo.maxProcBytes = fadeInfo.totNumBytes/16;
   if (fadeInfo.maxProcBytes < 16)
   {
      fadeInfo.maxProcBytes = 16;
   }
}

/*
 * ===============================================================================
 * 
 * Name: fade_10ms_tick
 *
 * ===============================================================================
 */
/**
 * Fade 10 ms tick
 *
 * Set the flag to start fade processing.
 *
 * @param   None
 * @return  None
 *
 * @pre     None
 * @note    None
 *
 * ===============================================================================
 */
void fade_10ms_tick()
{
   fadeInfo.tickOcc = TRUE;
}

/*
 * ===============================================================================
 * 
 * Name: fade_task
 * 
 * ===============================================================================
 */
/**
 * Fade task
 * 
 * Check if a new fade processing cycle needs to start.
 * 
 * @param   None 
 * @return  None
 * 
 * @pre     None 
 * @note    None
 * 
 * ===============================================================================
 */
void fade_task()
{
   if (gen2g_info.validCfg && gen2g_info.haveFade)
   {
      if (fadeInfo.tickOcc && !fadeInfo.fadeRunning)
      {
         fadeInfo.tickOcc = FALSE;
         fadeInfo.currByteOffset = ILLEGAL_FADE_OFFSET;
         fadeInfo.currFadeRec_p = &fadeInfo.fadeRec[0];
         fadeInfo.fadeRunning = TRUE;
      }
      if (fadeInfo.fadeRunning)
      {
          /* Process this fade bytes */
       	 fade_process_fade_bytes();
      }
   }
}

/*
 * ===============================================================================
 * 
 * Name: fade_find_next_fade_rec
 *
 * ===============================================================================
 */
/**
 * Find next fade record
 *
 * Find next record or if done mark fadeRunning to false
 *
 * @param   None
 * @return  None
 *
 * @pre     None
 * @note    None
 *
 * ===============================================================================
 */
void fade_find_next_fade_rec()
{
   BOOL              foundRec = FALSE;

   /* Find next valid fade record */
   while (fadeInfo.currFadeRec_p < &fadeInfo.fadeRec[MAX_FADE_RECS])
   {
      if (fadeInfo.currFadeRec_p->valid)
      {
         foundRec = TRUE;
         break;
      }
      fadeInfo.currFadeRec_p++;
   }
   if (foundRec)
   {
      fadeInfo.currByteOffset = 0;
   }
   else
   {
      fadeInfo.fadeRunning = FALSE;
      fadeInfo.totBytesProc = 0;
   }
}

/*
 * ===============================================================================
 *
 * Name: fade_process_fade_bytes
 * 
 * ===============================================================================
 */
/**
 * Fade process fade bytes
 * 
 * Process a fade bytes (up to 64 are processed at a time)
 * 
 * @param   None
 * @return  None
 * 
 * @pre     None 
 * @note    None
 * 
 * ===============================================================================
 */
void fade_process_fade_bytes()
{
   U8                *src_p;
   U8                *dst_p;
   U8                newData;
   U8                currData;
   U8                diff;
   BOOL              yield = FALSE;
   FADE_BYTE_INFO    *tmpFadeByte_p;

   if (fadeInfo.currByteOffset == ILLEGAL_FADE_OFFSET)
   {
      /* Find next valid fade record */
      fade_find_next_fade_rec();
   }
   if (fadeInfo.fadeRunning)
   {
      src_p = fadeInfo.currFadeRec_p->newPxlVal_p + fadeInfo.currByteOffset;
      dst_p = fadeInfo.currFadeRec_p->currPxlVal_p + fadeInfo.currByteOffset;
      tmpFadeByte_p = fadeInfo.currFadeRec_p->fadeByte_p + fadeInfo.currByteOffset;
      while ((fadeInfo.currByteOffset < fadeInfo.currFadeRec_p->numDataBytes) && !yield)
      {
         if (tmpFadeByte_p->totTicks != 0)
         {
            if (tmpFadeByte_p->currTick >= tmpFadeByte_p->totTicks)
            {
               newData = *src_p++;
               *dst_p++ = newData;
               fadeInfo.currFadeRec_p->fadeProc_fp(fadeInfo.currByteOffset, newData);
               tmpFadeByte_p->totTicks = 0;
            }
            else
            {
               newData = *src_p++;
               currData = *dst_p++;
               if (newData > currData)
               {
                  diff = newData - currData;
               }
               else
               {
                  diff = currData - newData;
               }
               diff = (U8)(((UINT)diff * (UINT)tmpFadeByte_p->currTick)/(UINT)tmpFadeByte_p->totTicks);
               if (newData > currData)
               {
                  currData += diff;
               }
               else
               {
                  currData -= diff;
               }
               fadeInfo.currFadeRec_p->fadeProc_fp(fadeInfo.currByteOffset, currData);
               tmpFadeByte_p->currTick++;
            }
         }
         else
         {
            src_p++;
            dst_p++;
         }
         tmpFadeByte_p++;
         fadeInfo.currByteOffset++;
         fadeInfo.totBytesProc++;

         /* Check if should yield to reduce main loop latency */
         if ((fadeInfo.totBytesProc % fadeInfo.maxProcBytes) == 0)
         {
            yield = TRUE;
         }
      }
      if (fadeInfo.currByteOffset >= fadeInfo.currFadeRec_p->numDataBytes)
      {
         fadeInfo.currFadeRec_p->endFadeProc_fp();

         /* Move to next record if exists */
         fadeInfo.currByteOffset = ILLEGAL_FADE_OFFSET;
         fadeInfo.currFadeRec_p++;
         fade_find_next_fade_rec();
      }
   }
}

/*
 * ===============================================================================
 *
 * Name: fade_update_rcv_cmd
 *
 * ===============================================================================
 */
/**
 * Fade update receive command
 *
 * Create a new fade command
 *
 * @param   offset      [in]        Byte offset for fade update
 * @param   numBytes    [in]        Number of bytes to update
 * @param   fadeTime    [in]        Fade time in ms
 * @return  None
 *
 * @pre     None
 * @note    None
 *
 * ===============================================================================
 */
void fade_update_rcv_cmd(
   U16                  offset,
   U16                  numBytes,
   U16                  fadeTime)
{
   fadeInfo.fadeRcvCmdErr = TRUE;
   fadeInfo.rcvTotTicks = 0;
   fadeInfo.rcvNumBytes = numBytes;
   if ((offset >> 12) < MAX_FADE_RECS)
   {
      fadeInfo.rcvCmdFadeRec_p = &fadeInfo.fadeRec[offset >> 12];
      fadeInfo.rcvOffset = offset & 0xfff;
      if ((fadeInfo.rcvOffset + numBytes) <= fadeInfo.rcvCmdFadeRec_p->numDataBytes)
      {
         fadeInfo.rcvTotTicks = fadeTime/10;
         fadeInfo.fadeRcvCmdErr = FALSE;
      }
   }
}

/*
 * ===============================================================================
 *
 * Name: fade_update_rcv_data
 *
 * ===============================================================================
 */
/**
 * Fade update receive data
 *
 * Update fade command final data
 *
 * @param   data        [in]        Byte of data
 * @return  TRUE if update is finished
 *
 * @pre     None
 * @note    Last byte is CRC8, so not saved as data
 *
 * ===============================================================================
 */
BOOL fade_update_rcv_data(
   U8                   data)
{
   FADE_BYTE_INFO    *tmpFadeByte_p;
   U8                prevNewByte;

   if (fadeInfo.rcvNumBytes != 0)
   {
      fadeInfo.rcvNumBytes--;
      if (!fadeInfo.fadeRcvCmdErr)
      {
         tmpFadeByte_p = fadeInfo.rcvCmdFadeRec_p->fadeByte_p + fadeInfo.rcvOffset;
         if (fadeInfo.rcvTotTicks == 0)
         {
            *(fadeInfo.rcvCmdFadeRec_p->currPxlVal_p + fadeInfo.rcvOffset) = data;
            tmpFadeByte_p->totTicks = 0;
            fadeInfo.rcvCmdFadeRec_p->fadeProc_fp(fadeInfo.rcvOffset, data);
         }
         else
         {
            /* If currently fading, set byte to end value of prev fade */
            if (tmpFadeByte_p->totTicks != 0)
            {
               prevNewByte = *(fadeInfo.rcvCmdFadeRec_p->newPxlVal_p + fadeInfo.rcvOffset);
               *(fadeInfo.rcvCmdFadeRec_p->currPxlVal_p + fadeInfo.rcvOffset) = prevNewByte;
               fadeInfo.rcvCmdFadeRec_p->fadeProc_fp(fadeInfo.rcvOffset, prevNewByte);
            }
            else
            {
               prevNewByte = *(fadeInfo.rcvCmdFadeRec_p->currPxlVal_p + fadeInfo.rcvOffset);
            }

            /* Only fade if prevByte does not match new value */
            if (prevNewByte == data)
            {
               tmpFadeByte_p->totTicks = 0;
            }
            else
            {
               tmpFadeByte_p->totTicks = fadeInfo.rcvTotTicks;
    	       tmpFadeByte_p->currTick = 0;
               *(fadeInfo.rcvCmdFadeRec_p->newPxlVal_p + fadeInfo.rcvOffset) = data;
            }
         }
         fadeInfo.rcvOffset++;
      }
      return (FALSE);
   }
   return (TRUE);
}

/* [] END OF FILE */
