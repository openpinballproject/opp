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
 * @file:   spiwing.c
 * @author: Hugh Spahr
 * @date:   6/7/2020
 *
 * @note:   Open Pinball Project
 *          Copyright© 2020, Hugh Spahr
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
 * This is the file for driving a SPI bus.  It can read/write or do both on a SPI
 * transfer.  It uses a DMA to send and receive data from the SPI.
 *
 *===============================================================================
 */
#include <stdlib.h>
#include "stdtypes.h"
#include "procdefs.h"
#include "gen2glob.h"
#include "spiintf.h"

#define STAT_DMA_DATA            0
#define STAT_WAIT_FOR_TICK       1

#define SPI_DATA_BUF_SZ          256

typedef struct
{
   U8                stat;                /* Status */
   U8                clkDivisor;          /* SPI clock divisor, defaults to 48MHz/4 = 12MHz */
   U8                numBytes;
   BOOL              tickOcc;             /* 10 ms tick occurred */
   U8                lastCmd;
   U8                *txBuf_p;
   U8                *rxBuf_p;
} SPI_INFO;

SPI_INFO spiInfo;

/* Prototypes */

/*
 * ===============================================================================
 * 
 * Name: spi_init
 * 
 * ===============================================================================
 */
/**
 * Initialize SPI processing
 * 
 * Allocate memory, and reset state machine.
 * 
 * @param   None 
 * @return  None
 * 
 * @pre     None 
 * @note    None
 * 
 * ===============================================================================
 */
void spi_init()
{
   /* Default to 48MHz/4 = 12MHz */
   spiInfo.clkDivisor = 4;

   /* Initialize the state machine to turn off all the LEDs, set indices to 0 */
   spiInfo.stat = STAT_DMA_DATA;
   spiInfo.tickOcc = FALSE;
   spiInfo.numBytes = 8;
   gen2g_info.haveSpi = TRUE;
    
   spiInfo.txBuf_p = (U8 *)&gen2g_info.spiOut[0];
   spiInfo.rxBuf_p = (U8 *)&gen2g_info.spiInp[0];

   /* Setup SPI2 GPIO port pins */
   gpioBBase_p->CRH &= ~0xf0000000;
   gpioBBase_p->CRH |= 0xb0000000;  // Alternate function push/pull output 50MHz
   gpioBBase_p->BSRR = 0x80000000;

   /* Enable clocks to SPI2 and DMA1 */
   rccBase_p->AHBENR |= 0x00000001;  // DMA1
   rccBase_p->APB1ENR |= 0x00004000;  // SPI2

   /* Set up SPI2 */
   spi2Base_p->CR1 = SPIx_CR1_SPE | SPIx_CR1_BR_8 | SPIx_CR1_MSTR | SPIx_CR1_SSM | SPIx_CR1_SSI;
   spi2Base_p->CR2 = SPIx_CR2_TXDMAEN;

   /* SPI tx DMA is dma1-5 */
   dma1Base_p->CPAR5 = (R32)&spi2Base_p->DR;
   dma1Base_p->CMAR5 = (R32)spiInfo.txBuf_p;
   dma1Base_p->CNDTR5 = spiInfo.numBytes;
   dma1Base_p->CCR5 = DMAx_CCR_MINC | DMAx_CCR_DIR | DMAx_CCR_EN;

   /* SPI rx DMA is dma1-4 */
   dma1Base_p->CPAR4 = (R32)&spi2Base_p->DR;
   dma1Base_p->CMAR4 = (R32)spiInfo.rxBuf_p;
   dma1Base_p->CNDTR4 = spiInfo.numBytes;
   dma1Base_p->CCR4 = DMAx_CCR_MINC | DMAx_CCR_DIR | DMAx_CCR_EN;
}

/*
 * ===============================================================================
 * 
 * Name: spi_task
 * 
 * ===============================================================================
 */
/**
 * SPI task
 * 
 * Check if SPI command has completed.  If so, received data back to host.
 * 
 * @param   None 
 * @return  None
 * 
 * @pre     None 
 * @note    None
 * 
 * ===============================================================================
 */
void spi_task()
{
   if (gen2g_info.validCfg && gen2g_info.haveSpi)
   {
      if ((spiInfo.stat == STAT_DMA_DATA) && (dma1Base_p->CNDTR5 == 0))
      {
         /* DMA complete, copy to data buffer */
    	 spiInfo.stat = STAT_WAIT_FOR_TICK;
      }
      else if ((spiInfo.stat == STAT_WAIT_FOR_TICK) && spiInfo.tickOcc)
      {
    	 spiInfo.tickOcc = FALSE;
    	 spiInfo.stat = STAT_DMA_DATA;

         dma1Base_p->CCR5 = DMAx_CCR_MINC | DMAx_CCR_DIR;
         dma1Base_p->CNDTR5 = spiInfo.numBytes;
         dma1Base_p->CCR5 = DMAx_CCR_MINC | DMAx_CCR_DIR | DMAx_CCR_EN;
      }
   }
}

/*
 * ===============================================================================
 *
 * Name: spi_update_rcv_cmd
 *
 * ===============================================================================
 */
/**
 * SPI update receive command
 *
 * Create a new fade command
 *
 * @param   cmd         [in]        Command such as xmt/rcv or xmt and rcv
 * @param   numBytes    [in]        Number of bytes to xmt or rcv
 * @param   xmtBuf_p    [in]        Data to xmt
 * @return  None
 *
 * @pre     None
 * @note    None
 *
 * ===============================================================================
 */
void spi_update_rcv_cmd(
   U8                   cmd,
   U16                  numBytes,
   U8                   *xmtData_p)
{
   if (spiInfo.stat == STAT_WAIT_FOR_TICK)
   {
      spiInfo.lastCmd = cmd;
      spiInfo.numBytes = numBytes;
   }
}

/* [] END OF FILE */
