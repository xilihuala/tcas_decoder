
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

using System;
using System.Collections.Generic;
using System.Text;

namespace UtilLib
{
    public class ECC
    {
        private enum colMasks: byte
        {
            evenHalf   = 0x0F,
            oddHalf    = 0xF0,
            evenFourth = 0x33,
            oddFourth  = 0xCC,
            evenEighth = 0x55,
            oddEighth  = 0xAA
        }

        // Single Bit ECC algorithm
        public static UInt32 Hamming(Byte[] data,UInt32 cnt)
        {
            UInt32 rowWidth = 8;
            UInt16 oddResult=0, evenResult=0;
            Byte bitParities = 0;
            Byte[] byteParities = new Byte[cnt];

            // Create column parities
            for (int i = 0; i < cnt; i++)
                bitParities ^= data[i];
            
            evenResult |= ( (CalcBitWiseParity(bitParities,evenHalf) << 2) |
                            (CalcBitWiseParity(bitParities,evenFourth) << 1) |
                            (CalcBitWiseParity(bitParities,evenEighth) << 0) );

            oddResult |= ( (CalcBitWiseParity(bitParities,~evenHalf) << 2) |
                           (CalcBitWiseParity(bitParities,~evenFourth) << 1) |
                           (CalcBitWiseParity(bitParities,~evenEighth) << 0) );

            // Create row Parities
            for (int i = 0; i < cnt; i++)
                byteParities[i] = CalcBitWiseParity(data[i],0xFF);

            // Place even row parity bits
            for (int i = 0; i < Math.Log(cnt,2); i++ )
            {
                Byte val = CalcRowParityBits(byteParities,true,Math.Pow(2,i));
                evenResult |= (val << (3+i));

                val = CalcRowParityBits(byteParities,false,Math.Pow(2,i));
                oddResult |= (val << (3+i));
            }
            
            return (((UInt32)oddResult << 16) & ( (UInt32) evenResult ));

        }

        private Byte CalcBitWiseParity(Byte val, Byte mask)
        {
            Byte result = 0;
            for (int i = 0; i < 8; i++)
            {
                if (mask & 0x1)
                {
                    result ^= (val & 0x1);
                }
                mask >>= 1;
                val >>= 1;
            }
            return (result & 0x1);
        }

        private Byte CalcRowParityBits(Byte[] byteParities, Boolean even, UInt32 chunkSize)
        {
            Byte result = 0;
            for (int i = (even ? 0 : chunkSize); i < byteParities.Length; i += (2*chunkSize))
                for (int j = 0; j < chunkSize; j++)
                    result ^= byteParities[i + j];
            return (result & 0x1);
        }
    }
}
