
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
  FILE        : slt.h
  PROJECT     : TI Booting and Flashing Utilities
  AUTHOR      : Daniel Allred
  DESC        : User boot loader header file for main components and intial
                entry point functions
 ----------------------------------------------------------------------------- */

#ifndef _SLT_H_
#define _SLT_H_

#include "tistdtypes.h"

// Prevent C++ name mangling
#ifdef __cplusplus
extern far "c" {
#endif

/***********************************************************
* Global Macro Declarations                                *
***********************************************************/

// UBL version number
#define SLT_VERSION_STRING  ("1.00")

// Define MagicNumber constants
#define MAGIC_NUMBER_VALID          (0xA1ACED00)

#define SLT_MAGIC_LOADIMAGE         (0xA1ACEDEE)		// Download via image UART

// Define max UBL image size
#define UBL_IMAGE_SIZE              (0x00007800u)

// Define max app image size
#define APP_IMAGE_SIZE              (0x02000000u)

/***********************************************************
* Global Typedef declarations                              *
***********************************************************/


/***********************************************************
* Global Function Declarations                             *
***********************************************************/

extern void main( void );


/***********************************************************
* End file                                                 *
***********************************************************/

#ifdef __cplusplus
}
#endif

#endif //_SLT_H_

/* --------------------------------------------------------------------------- 
  HISTORY
    v1.00  -  DJA  -  22-Jan-2007
			Initial Release 	   
    
 ----------------------------------------------------------------------------- */
