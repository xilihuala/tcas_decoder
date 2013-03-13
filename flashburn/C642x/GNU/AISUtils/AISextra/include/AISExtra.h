
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
  FILE        : AISExtra.h
  PROJECT     : TI Booting and Flashing Utilities
  AUTHOR      : Daniel Allred
  DESC        : Header file for AIS Extra commands for use in AIS Scripts    
 ----------------------------------------------------------------------------- */

#ifndef _AISEXTRA_H_
#define _AISEXTRA_H_

#include "tistdtypes.h" 

// Prevent C++ name mangling
#ifdef __cplusplus
extern far "c" {
#endif

// Global Macro Declarations

// Function Prototypes

// Execute LPSC state transition
void LPSCTransition();

// UART send DONE function
void UARTSendDONE();

#endif // End _AISEXTRA_H_

