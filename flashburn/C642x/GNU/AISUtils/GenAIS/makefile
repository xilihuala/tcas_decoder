
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
# Makefile for TI hexAIS project.                           #
#############################################################
# Auth: Daniel Allred
#	    
PROGRAM:=GenAIS
DEVSTRING:=C642x

MONOCOMPILE=gmcs
DOTNETCOMPILE=csc
LOCALSOURCES=$(PROGRAM).cs
REMOTESOURCES=device_name.cs AISGen_$(DEVSTRING).cs CRC32.cs Endian.cs EmbeddedFileIO.cs Debug.cs COFF.cs AISGen.cs


OBJECTS:=$(patsubst %.cs,%.module,$(REMOTESOURCES))
EXECUTABLE:=../$(PROGRAM)_$(DEVSTRING).exe

COMMONPATH:=../../../../Common
ICON:=$(COMMONPATH)/ti.ico
UTILLIBPATH:=$(COMMONPATH)/UtilLib
AISGENLIBPATH:=$(COMMONPATH)/AISGenLib

DEVCOMMONPATH:=../../../Common

GNUCOMMONPATH:=../../Common
DEVAISGENLIBPATH:=$(GNUCOMMONPATH)/AISGenLib

AISEXTRAPATH:=../AISExtra
AISEXTRAIMAGE:=$(AISEXTRAPATH)/AISExtra_$(DEVSTRING).out

# Target definitions
all: $(EXECUTABLE) $(OBJECTS) $(LOCALSOURCES) $(AISEXTRAIMAGE) $(REMOTESOURCES) 

.PHONY : clean
clean:
		-rm -f -v $(EXECUTABLE) $(OBJECTS)

$(EXECUTABLE): $(LOCALSOURCES) $(OBJECTS) $(AISEXTRAIMAGE) $(REMOTESOURCES) 
ifeq ($(shell uname),Linux)
		@$(MONOCOMPILE) -t:exe -resource:$(AISEXTRAIMAGE) -define:$(DEVSTRING) -out:$(EXECUTABLE) $(LOCALSOURCES) $(OBJECTS)
else
		@$(DOTNETCOMPILE) /t:exe /resource:$(AISEXTRAIMAGE) /define:$(DEVSTRING) /out:$(EXECUTABLE) /win32icon:$(ICON) $(LOCALSOURCES) $(OBJECTS)
endif

%.module: %.cs
		@cp $< $@

vpath %.cs $(UTILLIBPATH)/:$(DEVAISGENLIBPATH)/:$(AISGENLIBPATH)/:$(DEVCOMMONPATH)/src/
