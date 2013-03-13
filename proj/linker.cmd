/*
 *  Copyright 2007 by Spectrum Digital Incorporated.
 *  All rights reserved. Property of Spectrum Digital Incorporated.
 */

/*
 *  Linker command file
 *
 */

-l rts64plus.lib
-l biosDM420.a64p         /* link in bios library to use the BCACHE api's */

-stack          0x00000800      /* Stack Size */
-heap           0x00000800      /* Heap Size */

MEMORY
{
    L2RAM:      o = 0x10800000  l = 0x00020000
    DDR2:       o = 0x80000000  l = 0x08000000
}

SECTIONS
{
    .bss        >   L2RAM
    .cinit      >   L2RAM
    .cio        >   L2RAM
    .const      >   L2RAM
    .data       >   L2RAM
    .far        >   DDR2
    .stack      >   DDR2
    .switch     >   DDR2
    .sysmem     >   DDR2
    .text       >   DDR2
    .ddr2       >   DDR2
	 my_sect    >   DDR2
    .bios       >   L2RAM 
	.intvecs   >    L2RAM
	fpga_data   >   L2RAM
	report_data >   DDR2
}
