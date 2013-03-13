
###########################################################################
#                                                                         #
#   (C) Copyright 2008                                                    #
#   Texas Instruments, Inc.  <www.ti.com>                                 # 
#                                                                         #
#   See file CREDITS for list of people who contributed to this project.  #
#                                                                         #
###########################################################################


###########################################################################
#                                                                         #
#   This file is part of the DaVinci Flash and Boot Utilities.            #
#                                                                         #
#   This program is free software: you can redistribute it and/or modify  #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation, either version 2 of the License, or     #
#   (at your option) any later version.                                   #
#                                                                         #
#   This program is distributed in the hope that it will be useful, but   #
#   WITHOUT ANY WARRANTY; without even the implied warranty of            #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     #
#   General Public License for more details.                              #
#                                                                         #
#   You should have received a copy of the GNU General Public License     #
#   along with this program.  if not, write to the Free Software          #
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307   #
#   USA                                                                   #
#                                                                         #
###########################################################################

#############################################################
# Makefile for TI AISExtra project.                         #
#   Generates the COFF AISExtra file which is used to       #
#   supplement the AIS ROM bootloader to add the ability    #
#	  to boot successfully over UART and to change the power  #
#	  domain states of the device's Local Power Domains.      #
#############################################################
# Auth: Daniel Allred
#
PROGRAM:=AISExtra
DEVSTRING:=C642x

CROSSCOMPILE=
CC=$(CROSSCOMPILE)cl6x
SOURCES=$(PROGRAM).c

CFLAGS=-O=2 -c -I=../../../../../Common/include -I=../../../../Common/include -I=../../../Common/include -I=../include -ms3 -mv6400+ --symdebug:none
LNKFLAGS=-z -a -e_LPSCTransition -w -x

OBJECTS:=$(patsubst %.c,%.obj,$(SOURCES))
EXECUTABLE:=../$(PROGRAM)_$(DEVSTRING).out
LINKERSCRIPT=$(PROGRAM).cmd

all: $(EXECUTABLE) $(OBJECTS)

.PHONY : clean objclean
clean:
		-rm -f -v $(OBJECTS) $(EXECUTABLE)
		
objclean:
		-rm -f -v $(OBJECTS)

$(EXECUTABLE): $(OBJECTS)
		$(CC) $(LNKFLAGS) -o=$@ $(LINKERSCRIPT) $(OBJECTS)
		
%.obj : %.c %.h
		$(CC) $(CFLAGS) $< 

vpath %.h ../../../../Common/include:../../../Common/include:../../Common/include:../include        

