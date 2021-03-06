
/**************************************************************************
*                                                                         *
*   (C) Copyright 2008                                                    *
*   Texas Instruments, Inc.  <www.ti.com>                                 * 
*                                                                         *
*   See file CREDITS for list of people who contributed to this project.  *
*                                                                         *
**************************************************************************/


/**************************************************************************
*                                                                         *
*   This file is part of the DaVinci Flash and Boot Utilities.            *
*                                                                         *
*   This program is free software: you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation, either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful, but   *
*   WITHOUT ANY WARRANTY; without even the implied warranty of            *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
*   General Public License for more details.                              *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program.  if not, write to the Free Software          *
*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307   *
*   USA                                                                   *
*                                                                         *
**************************************************************************/

/* --------------------------------------------------------------------------
  FILE        : uartboot.c                                                   
  PROJECT     : TI Booting and Flashing Utilities
  AUTHOR      : Daniel Allred
  DESC        : Implementation of the UART boot mode for the SFT
 ----------------------------------------------------------------------------- */

// General type include
#include "tistdtypes.h"

// This module's header file
#include "uartboot.h"

// UART driver
#include "uart.h"

// Misc. utility function include
#include "util.h"

// Project specific debug functionality
#include "debug.h"

// Main SFT module
#include "sft.h"

// Device module
#include "device.h"

// Flash type includes
#if defined(UBL_NOR)
  #include "nor.h"
  #include "norboot.h"
#elif defined(UBL_NAND)
  #include "nand.h"
  #include "device_nand.h"
  #include "nandboot.h"
#endif

/************************************************************
* Explicit External Declarations                            *
************************************************************/

extern VUint32 gMagicFlag,gBootCmd;
extern Uint32 gEntryPoint;
extern __FAR__ Uint32 EMIFStart;


/************************************************************
* Local Macro Declarations                                  *
************************************************************/


/************************************************************
* Local Typedef Declarations                                *
************************************************************/


/************************************************************
* Local Function Declarations                               *
************************************************************/

static Uint32 LOCAL_sendSequence(String s);
static Uint32 LOCAL_recvCommand(Uint32* bootCmd);
static Uint32 LOCAL_recvHeaderAndData(UARTBOOT_HeaderHandle ackHeader);
#if defined(UBL_NAND)
  static Uint32 LOCAL_NANDWriteHeaderAndData(NAND_InfoHandle hNandInfo, NANDBOOT_HeaderHandle nandBoot, Uint8 *srcBuf);
#endif

/************************************************************
* Local Variable Definitions                                *
************************************************************/


/************************************************************
* Global Variable Definitions                               *
************************************************************/


/************************************************************
* Global Function Definitions                               *
************************************************************/

Uint32 UARTBOOT_copy(void)
{

#if defined(UBL_NAND)
  NANDBOOT_HeaderObj  nandBoot;
  NAND_InfoHandle     hNandInfo;
#elif defined(UBL_NOR)
  NOR_InfoHandle      hNorInfo;
#endif  
  UARTBOOT_HeaderObj  ackHeader;
  Uint32              bootCmd;

UART_tryAgain:
  DEBUG_printString("Starting UART Boot...\r\n");

  // UBL Sends 'BOOTUBL/0'
  if (LOCAL_sendSequence("BOOTUBL") != E_PASS)
    goto UART_tryAgain;

  // Receive the BOOT command
  if(LOCAL_recvCommand(&bootCmd) != E_PASS)
    goto UART_tryAgain;
    
  // Send ^^^DONE\0 to indicate command was accepted
  if ( LOCAL_sendSequence("   DONE") != E_PASS )
    goto UART_tryAgain;

  switch(bootCmd)
  {
#if defined(UBL_NOR)
    case UBL_MAGIC_NOR_BIN_BURN:
    {
      // Initialize the NOR Flash
      hNorInfo = NOR_open((Uint32)&EMIFStart, (Uint8)DEVICE_emifBusWidth()) ;
      if (hNorInfo == NULL)
      {
        DEBUG_printString("NOR_open() failed!");
        goto UART_tryAgain;    
      }
    
      // Get the APP (should be u-boot) into binary form
      if ( LOCAL_recvHeaderAndData(&ackHeader) != E_PASS )
        goto UART_tryAgain;
      
      // Erasing the Flash
      if ( NOR_erase(hNorInfo,hNorInfo->flashBase, ackHeader.byteCnt) != E_PASS )
        goto UART_tryAgain;

      // Write the actual application to the flash
      if ( NOR_writeBytes(hNorInfo,hNorInfo->flashBase, ackHeader.byteCnt, (Uint32)ackHeader.imageBuff) != E_PASS )
        goto UART_tryAgain;
          
      // Return DONE when UBL flash operation has been completed
      if ( LOCAL_sendSequence("   DONE") != E_PASS )
        return E_FAIL;

      // Set the entry point for code execution
      gEntryPoint = hNorInfo->flashBase;
      break;
    }
    case UBL_MAGIC_NOR_GLOBAL_ERASE:
    {
      // Initialize the NOR Flash
      hNorInfo = NOR_open((Uint32)&EMIFStart, (Uint8)DEVICE_emifBusWidth()) ;
      if (hNorInfo == NULL)
      {
        DEBUG_printString("NOR_open() failed!");
        goto UART_tryAgain;    
      }

      // Erasing the Flash
      if (NOR_globalErase(hNorInfo) != E_PASS)
      {
        DEBUG_printString("\r\nErase failed.\r\n");
      }
      else
      {
        DEBUG_printString("\r\nErase completed successfully.\r\n");
      }
      
      // Return DONE when erase operation has been completed
      if ( LOCAL_sendSequence("   DONE") != E_PASS )
        return E_FAIL;

      // Set the entry point for code execution
      // Go to reset in this case since no code was downloaded
      gEntryPoint = 0x0; 

      break;
    }
#elif defined(UBL_NAND)
    case UBL_MAGIC_NAND_BIN_BURN:
    {
      // Initialize the NAND Flash
      hNandInfo = NAND_open((Uint32)&EMIFStart);
      if ( hNandInfo ==  NULL )
      {
        DEBUG_printString("NAND_open() failed!");
        goto UART_tryAgain;
      }       
    
      // ------ Get UBL Data and Write it to Flash ------       
      // Get the UBL header and data
      if (LOCAL_recvHeaderAndData(&ackHeader) != E_PASS)
        goto UART_tryAgain;

      // Setup the NANDBOOT header that will be stored in flash
      nandBoot.magicNum = ackHeader.magicNum;
      nandBoot.entryPoint = ackHeader.startAddr;
      nandBoot.page = 1;                          // The page is always page 0 for the UBL header, so we use page 1 for data
      nandBoot.block = DEVICE_NAND_RBL_SEARCH_START_BLOCK;       // Set starting block to begin the write attempts
      nandBoot.ldAddress = ackHeader.loadAddr;    // This field doesn't matter for the UBL header

      // Calculate the number of NAND pages needed to store the UBL image
      nandBoot.numPage = 0;
      while ( (nandBoot.numPage * hNandInfo->dataBytesPerPage) < (ackHeader.byteCnt))
      {
        nandBoot.numPage++;
      }
  
      // Write header to page 0 of block 1(or up to block 5)
      // Write the UBL to the same block, starting at page 1
      DEBUG_printString("Writing UBL to NAND flash\r\n");
      if (LOCAL_NANDWriteHeaderAndData(hNandInfo, &nandBoot, ackHeader.imageBuff) != E_PASS)
        goto UART_tryAgain;
        
      // Return DONE when UBL flash operation has been completed
      if ( LOCAL_sendSequence("   DONE") != E_PASS )
        return E_FAIL;

      // ------ Get Application Data and Write it to Flash ------       
      // Get the application header and data
      if (LOCAL_recvHeaderAndData(&ackHeader) != E_PASS)
        goto UART_tryAgain;
      
      // Setup the NANDBOOT header that will be stored in flash
      nandBoot.magicNum = ackHeader.magicNum;         // Rely on the host applciation to send over the right magic number (safe or bin)
      nandBoot.entryPoint = ackHeader.startAddr;      // Use the entrypoint received in ACK header
      nandBoot.page = 1;                              // The page is always page 0 for the header, so we use page 1 for data
      nandBoot.block = DEVICE_NAND_UBL_SEARCH_START_BLOCK;           // Set the starting block for application
      nandBoot.ldAddress = ackHeader.loadAddr;        // The load address is only important if this is a binary image
      
      // Calculate the number of NAND pages needed to store the APP image
      nandBoot.numPage = 0;
      while ( (nandBoot.numPage * hNandInfo->dataBytesPerPage) < ackHeader.byteCnt )
      {
        nandBoot.numPage++;
      }

      // Write header to page 0 of block 1(or up to block 5)
      // Write the UBL to the same block, starting at page 1
      DEBUG_printString("Writing APP to NAND flash\r\n");
      if (LOCAL_NANDWriteHeaderAndData(hNandInfo,&nandBoot, ackHeader.imageBuff) != E_PASS)
        goto UART_tryAgain;
        
      // Return DONE when UBL flash operation has been completed
      if ( LOCAL_sendSequence("   DONE") != E_PASS )
        return E_FAIL;

      // Set the entry point to nowhere, since there isn't an appropriate binary image to run */
      gEntryPoint = 0x0;
      break;
    }  // end case UBL_MAGIC_NAND_BIN_BURN
    case UBL_MAGIC_NAND_GLOBAL_ERASE:
    {
      // Initialize the NAND Flash
      hNandInfo = NAND_open((Uint32)&EMIFStart);
      if ( hNandInfo ==  NULL )
      {
        DEBUG_printString("NAND_open() failed!");
        goto UART_tryAgain;
      }   

      // Unprotect the NAND Flash
      NAND_unProtectBlocks(hNandInfo, DEVICE_NAND_RBL_SEARCH_START_BLOCK, DEVICE_NAND_UBL_SEARCH_END_BLOCK-1);

      // Erase all the pages of the device
      if (NAND_eraseBlocks(hNandInfo, DEVICE_NAND_RBL_SEARCH_START_BLOCK, DEVICE_NAND_UBL_SEARCH_END_BLOCK-1) != E_PASS)
      {
        DEBUG_printString("Erase failed.\r\n");
        goto UART_tryAgain;
      }
      else
      {
        DEBUG_printString("Erase completed successfully.\r\n");
      }
                  
      // Protect the device
      NAND_protectBlocks(hNandInfo);
      
      // Return DONE when erase operation has been completed
      if ( LOCAL_sendSequence("   DONE") != E_PASS )
        return E_FAIL;

      // Set the entry point for code execution
      // Go to reset in this case since no code was downloaded 
      gEntryPoint = 0x0; 
      break;
    }
#endif
    default:
    {
      DEBUG_printString("Boot command not supported!");
      return E_FAIL;
    }
  }
  
  LOCAL_sendSequence("   DONE");

  return E_PASS;
}


/************************************************************
* Local Function Definitions                                *
************************************************************/

static Uint32 LOCAL_sendSequence(String s)
{
  return UART_sendString(s, TRUE);
}

static Uint32 LOCAL_recvCommand(Uint32* bootCmd)
{
  if(UART_checkSequence("    CMD", TRUE) != E_PASS)
  {
    return E_FAIL;
  }

  if(UART_recvHexData(4,bootCmd) != E_PASS)
  {
    return E_FAIL;
  }

  return E_PASS;
}

static Uint32 LOCAL_recvHeaderAndData(UARTBOOT_HeaderHandle ackHeader)
{
  Uint32  error = E_PASS, recvLen;
  Bool    imageIsUBL;
  Uint32  maxImageSize,minStartAddr,maxStartAddr;
  
  // Issue command to host to send image
  if ( LOCAL_sendSequence("SENDIMG") != E_PASS)
  {
    return E_FAIL;
  }

  // Recv ACK command
  if(UART_checkSequence("    ACK", TRUE) != E_PASS)
  {
    return E_FAIL;
  }

  // Get the ACK header elements
  error =  UART_recvHexData( 4, (Uint32 *) &(ackHeader->magicNum)  );
  error |= UART_recvHexData( 4, (Uint32 *) &(ackHeader->startAddr) );
  error |= UART_recvHexData( 4, (Uint32 *) &(ackHeader->byteCnt)   );
  error |= UART_recvHexData( 4, (Uint32 *) &(ackHeader->loadAddr)  );  
  error |= UART_checkSequence("0000", FALSE);
  if(error != E_PASS)
  {
    return E_FAIL;
  }
  
  // Check if this is a UBL or APP image
  if (ackHeader->loadAddr == 0x00000020)
  {
    imageIsUBL = TRUE;
    maxImageSize = UBL_IMAGE_SIZE;
    minStartAddr = 0x0100;
    maxStartAddr = UBL_IMAGE_SIZE;
  }
  else
  {
    imageIsUBL = FALSE;
    maxImageSize = APP_IMAGE_SIZE;
    minStartAddr = DEVICE_DDR2_START_ADDR;
    maxStartAddr = DEVICE_DDR2_END_ADDR;
  }
  
    
  // Verify that the data size is appropriate
  if((ackHeader->byteCnt == 0) || (ackHeader->byteCnt > maxImageSize))
  {
    LOCAL_sendSequence(" BADCNT");  // trailing /0 will come along
    return E_FAIL;
  }

  // Verify application start address is in RAM (lower 16bit of appStartAddr also used 
  // to hold UBL entry point if this header describes a UBL)
  if( (ackHeader->startAddr < minStartAddr) || (ackHeader->startAddr > maxStartAddr) )
  {
    LOCAL_sendSequence("BADADDR");  // trailing /0 will come along
    return E_FAIL;
  }
  
  // Allocate space in DDR to store image
  ackHeader->imageBuff = (Uint8 *) UTIL_allocMem(ackHeader->byteCnt);

  // Send BEGIN command
  if (LOCAL_sendSequence("  BEGIN") != E_PASS)
    return E_FAIL;

  // Receive the data over UART
  recvLen = ackHeader->byteCnt;
  error = UART_recvStringN((String)ackHeader->imageBuff, &recvLen, FALSE );
  if ( (error != E_PASS) || (recvLen != ackHeader->byteCnt) )
  {
    DEBUG_printString("\r\nUART Receive Error\r\n");
    return E_FAIL;
  }

  // Return DONE when all data arrives
  if ( LOCAL_sendSequence("   DONE") != E_PASS )
    return E_FAIL;

  return E_PASS;
}

// Generic function to write a UBL or Application header and the associated data
#if defined(UBL_NAND)
static Uint32 LOCAL_NANDWriteHeaderAndData(NAND_InfoHandle hNandInfo, NANDBOOT_HeaderHandle nandBoot, Uint8 *srcBuf)
{
  Uint32    endBlockNum;
  Uint32    *ptr;
  Uint32    blockNum,pageNum,pageCnt,i;
  Uint32    numBlks;

  Uint8     *hNandWriteBuf,*hNandReadBuf;
  
  // Allocate mem for write and read buffers
  hNandWriteBuf = UTIL_allocMem(hNandInfo->dataBytesPerPage);
  hNandReadBuf  = UTIL_allocMem(hNandInfo->dataBytesPerPage);
  
  // Clear buffers
  for (i=0; i < hNandInfo->dataBytesPerPage; i++)
  {
    hNandWriteBuf[i] = 0xFF;
    hNandReadBuf[i] = 0xFF;
  }

  // Get total number of blocks needed
  numBlks = 0;
  while ( (numBlks * hNandInfo->pagesPerBlock)  < (nandBoot->numPage + 1) )
  {
    numBlks++;
  }
  DEBUG_printString("Number of blocks needed for header and data: 0x");
  DEBUG_printHexInt(numBlks);
  DEBUG_printString("\r\n");

  // Check whether writing UBL or APP (based on destination block)
  blockNum = nandBoot->block;
  if (blockNum == DEVICE_NAND_RBL_SEARCH_START_BLOCK)
    endBlockNum = DEVICE_NAND_RBL_SEARCH_END_BLOCK;
  else if (blockNum == DEVICE_NAND_UBL_SEARCH_START_BLOCK)
    endBlockNum = DEVICE_NAND_UBL_SEARCH_END_BLOCK;
  else
    return E_FAIL; // Block number is out of range

NAND_WRITE_RETRY:
  if (blockNum > endBlockNum)
    return E_FAIL;
    
  // Go to first good block
  if (NAND_badBlockCheck(hNandInfo,blockNum) != E_PASS)
  {  
    blockNum++;
    goto NAND_WRITE_RETRY;
  }

  DEBUG_printString("Attempting to start in block number 0x");
  DEBUG_printHexInt(blockNum);
  DEBUG_printString(".\n");

  // Unprotect all needed blocks of the Flash 
  if (NAND_unProtectBlocks(hNandInfo,blockNum,numBlks) != E_PASS)
  {
    blockNum++;
    DEBUG_printString("Unprotect failed\r\n");
    goto NAND_WRITE_RETRY;
  }

  // Erase the block where the header goes and the data starts
  if (NAND_eraseBlocks(hNandInfo,blockNum,numBlks) != E_PASS)
  {
    blockNum++;
    DEBUG_printString("Erase failed\r\n");
    goto NAND_WRITE_RETRY;
  }
    
  // Setup header to be written
  ptr = (Uint32 *) hNandWriteBuf;
  ptr[0] = nandBoot->magicNum;
  ptr[1] = nandBoot->entryPoint;
  ptr[2] = nandBoot->numPage;
  ptr[3] = blockNum;  //always start data in current block
  ptr[4] = 1;      //always start data in page 1 (this header goes in page 0)
  ptr[5] = nandBoot->ldAddress;

  // Write the header to page 0 of the current blockNum
  DEBUG_printString("Writing header data to Block ");
  DEBUG_printHexInt(blockNum);
  DEBUG_printString(", Page ");
  DEBUG_printHexInt(0);
  DEBUG_printString("\r\n");

  if (NAND_writePage(hNandInfo, blockNum, 0, hNandWriteBuf) != E_PASS)
  {
    blockNum++;
    DEBUG_printString("Write failed!\r\n");
    goto NAND_WRITE_RETRY;
  }
    
  UTIL_waitLoop(200);

  // Verify the page just written
  if (NAND_verifyPage(hNandInfo, blockNum, 0, hNandWriteBuf, hNandReadBuf) != E_PASS)
  {
    blockNum++;
    DEBUG_printString("Write verify failed!\r\n");
    goto NAND_WRITE_RETRY;
  }

  pageCnt = 1;
  pageNum = 1;

  do
  {
    DEBUG_printString("Writing image data to Block ");
    DEBUG_printHexInt(blockNum);
    DEBUG_printString(", Page ");
    DEBUG_printHexInt(pageNum);
    DEBUG_printString("\r\n");

    // Write the UBL or APP data on a per page basis
    if (NAND_writePage(hNandInfo, blockNum, pageNum, srcBuf) != E_PASS)
    {
      blockNum++;
      DEBUG_printString("Write failed!\r\n");
      goto NAND_WRITE_RETRY;
    }

    UTIL_waitLoop(200);

    // Verify the page just written
    if (NAND_verifyPage(hNandInfo, blockNum, pageNum, srcBuf, hNandReadBuf) != E_PASS)
    {
      blockNum++;
      DEBUG_printString("Write verify failed!\r\n");
      goto NAND_WRITE_RETRY;
    }

    srcBuf += hNandInfo->dataBytesPerPage;
    pageCnt++;
    pageNum++;

    if (pageNum == hNandInfo->pagesPerBlock)
    {
      blockNum++;
      pageNum = 0;
    }

  }
  while (pageCnt < (nandBoot->numPage+1));

  NAND_protectBlocks(hNandInfo);

  return E_PASS;
}
#endif


/************************************************************
* End file                                                  *
************************************************************/


