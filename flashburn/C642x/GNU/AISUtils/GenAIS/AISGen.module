
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

/****************************************************************
 *  TI Abstract AISGen Class
 *  (C) 2007, Texas Instruments, Inc.                           
 *                                                              
 * Author:  Daniel Allred
 * History: 15-Mar-07 v1.00
 *          05-Apr-07 v1.01   
 *                                                              
 ****************************************************************/

using System;
using System.Text;
using System.Text.RegularExpressions;
using System.IO;
using System.IO.Ports;
using System.Reflection;
using System.Threading;
using System.Globalization;
using System.Collections;
using UtilLib;
using UtilLib.IO;
using UtilLib.CRC;

namespace AISGenLib
{
    #region Public enums and structs for AIS creation

    /// <summary>
    /// Generic return type enumeration
    /// </summary>
    public enum retType : uint
    {
        SUCCESS = 0,
        FAIL    = 1,
        TIMEOUT = 2
    }

    /// <summary>
    /// Enum of AIS values that can be used in AIS scripts
    /// </summary>
    public enum AIS : uint
    {
        MagicNumber     = 0x41504954,
        Section_Load    = 0x58535901,
        RequestCRC      = 0x58535902,
        EnableCRC       = 0x58535903,
        DisableCRC      = 0x58535904,
        Jump            = 0x58535905,
        Jump_Close      = 0x58535906,
        Set             = 0x58535907,
        Start_Over      = 0x58535908,
        Reserved        = 0x58535909,
        Section_Fill    = 0x5853590A,
        Ping            = 0x5853590B,
        Get             = 0x5853590C,
        FunctionExec    = 0x5853590D
    };   

    /// <summary>
    /// Enum of memory types
    /// </summary>
    public enum memType : uint
    {
        EightBit = 0x1,
        SixteenBit = 0x2,
        ThirtyTwoBit = 0x4,
        SixtyFourBit = 0x8
    };

    /// <summary>
    /// Enum of possible boot modes
    /// </summary>
    public enum BootModes : uint
    {
        NONE = 0x0,
        SPIMASTER,
        I2CMASTER,
        EMIFA,
        NAND,
        EMAC,
        UART,
        PCI,
        HPI,
        USB,
        MMC_SD,
        VLYNQ,
        RAW
    };

    /// <summary>
    /// CRC check type for the AIS stream
    /// </summary>
    public enum CRCCheckType : uint
    {
        NO_CRC = 0,
        SECTION_CRC = 1,
        SINGLE_CRC = 2
    };

    /// <summary>
    /// File Output types
    /// </summary>
    public enum OutputType : uint
    {
        ASCII = 0,
        ASM,
        BIN,
        TXT
    };

    /// <summary>
    /// AIS ROM Function struct
    /// </summary>
    public struct ROMFunction
    {
        public String funcName;
        public String iniSectionName;
        public UInt16 numParams;
        public String[] paramNames;
    }

    /// <summary>
    /// AIS Extra Function Struct
    /// </summary>
    public struct AISExtraFunction
    {
        public String funcName;
        public String iniSectionName;
        public UInt16 numParams;
        public String[] paramNames;
        public UInt32 paramAddr;
        public UInt32 funcAddr;
        public Boolean isInitFunc;
    }
   
    /// <summary>
    /// Struct for parsing the INI file sections
    /// </summary>
    public struct INISection
    {
        public String iniSectionName;
        public Hashtable sectionValues;
    }

    /// <summary>
    /// Cache Level Enumeration
    /// </summary>
    public enum CacheLevel:uint
    {
        L1 = 1,
        L2 = 2
    }

    /// <summary>
    /// Cache Type enumeration (Program or Data flags)
    /// </summary>
    [Flags]
    public enum CacheType:uint
    {
        Program = 1,
        Data = 2
    }

    /// <summary>
    /// Cache Information Struct
    /// </summary>
    public struct CacheInfo
    {
        public CacheLevel level;
        public CacheType type;
        public UInt32 startAddr;
        public UInt32 size;

        public Boolean InCache(UInt32 addr)
        {
            return ((addr < (startAddr+size)) && ( addr >= startAddr));
        }
    }

    /// <summary>
    /// GEM core internal DMA info class
    /// </summary>
    public class IDMARegisters
    {
        public readonly UInt32 channelNum;
        public readonly UInt32 IDMA_STAT_ADDR;
        public readonly UInt32 IDMA_MASK_ADDR;
        public readonly UInt32 IDMA_SRC_ADDR;
        public readonly UInt32 IDMA_DEST_ADDR;
        public readonly UInt32 IDMA_CNT_ADDR;

        public IDMARegisters(UInt32 ChannelNum, UInt32 baseAddr)
        {
            channelNum = ChannelNum;
            IDMA_STAT_ADDR = baseAddr;
            IDMA_MASK_ADDR = IDMA_STAT_ADDR + 4;
            IDMA_SRC_ADDR = IDMA_MASK_ADDR + 4;
            IDMA_DEST_ADDR = IDMA_SRC_ADDR + 4;
            IDMA_CNT_ADDR = IDMA_DEST_ADDR + 4;
        }
    }

    #endregion

    /// <summary>
    /// Public abstract class (with static parts) to handle generic activities
    /// for device specific AISGen objects.
    /// </summary>
    public abstract class AISGen
    {
        #region Public Properties
        
        /// <summary>
        /// Read or Set the type of CRC checking used in section loading.
        /// </summary>
        public CRCCheckType CRCType
        {
            get { return crcType; }
            set
            {
                crcType = value;
            }
        }

        /// <summary>
        /// Read or set the filename of the binary AIS file.
        /// </summary>
        public String AISFileName
        {
            set { devAISFileName = value; }
            get { return devAISFileName; }
        }

        /// <summary>
        /// Read or set the name of the initialization file, which includes values to setup DDR, PLLs, PINMUX, EMIFA, and PSCs
        /// </summary>
        public String INIFileName
        {
            get { return devINIFileName; }
            set { devINIFileName = value; }
        }

        /// <summary>
        /// Get or set the bootmode for the AIS generator.
        /// </summary>
        public BootModes BootMode
        {
            get { return bootMode; }
            set { bootMode = value; }
        }
        
        /// <summary>
        /// The AIS data.
        /// </summary>
        public Byte[] Data
        {
            get { return AISData; }
        }

        public String DeviceNameShort
        {
            get { return devNameShort; }
        }

        public String DeviceNameLong
        {
            get { return devNameLong; }
        }

        #endregion

        #region Protected methods and variables

        /// <summary>
        /// Short name of device 
        /// </summary>
        protected String devNameShort;

        /// <summary>
        /// Long name of device
        /// </summary>
        protected String devNameLong;

        /// <summary>
        /// The bootmode currently selected for the AIS generator.
        /// </summary>
        protected BootModes bootMode;

        /// <summary>
        /// An array of bytes for holding the generated AIS data.
        /// </summary>
        protected Byte[] AISData;

        /// <summary>
        /// An array of ROMFunction objects for holding info about the device's callable AIS ROM functions. 
        /// </summary>
        protected ROMFunction[] ROMFunc;

        /// <summary>
        /// An array of CacheInfo objects for holding info about the device's caches.
        /// </summary>
        protected CacheInfo[] Cache;

        /// <summary>
        /// An array of IDMARegister Objects for holding info about the device's internal DMA channels.
        /// </summary>
        protected IDMARegisters[] IDMA;

        /// <summary>
        /// The current type of CRC checking that the device generator is using for AIS section load commands.
        /// This can be modified via the public property.
        /// </summary>
        protected CRCCheckType crcType;

        /// <summary>
        /// The endianness of the device.
        /// </summary>
        protected Endian devEndian;

        /// <summary>
        /// The filename of the INI file.
        /// </summary>
        protected String devINIFileName;

        /// <summary>
        /// The FileStream object used to access the file holding the binary AIS data.
        /// </summary>
        protected FileStream devAISFileStream;

        /// <summary>
        /// The name of the binary file used to hold the generated AIS data.
        /// </summary>
        protected String devAISFileName;

        /// <summary>
        /// The name of the COFF file that holds special code to handle AIS Extra functions.
        /// </summary>
        protected String AISExtraFileName;

        /// <summary>
        /// An array of AISExtraFunction objects that correspond to the special functions offered in the AISExtraFileName file.
        /// </summary>
        protected AISExtraFunction[] AISExtraFunc;

        /// <summary>
        /// Address of the UARTSendDONE function that comes from the AISExtraFileName file. Used in no flow control UART boots.
        /// </summary>
        protected UInt32 UARTSendDONEAddr;

        /// <summary>
        /// The CRC32 object used in calculating the CRCs needed for the AIS Section Load commands.
        /// </summary>
        protected CRC32 devCRC;

        /// <summary>
        /// A boolean variable to indicate whether the UARTSendDONE function should be called for this AIS file.
        /// </summary>
        protected Boolean SendUARTSendDONE;

        #endregion

        #region Static methods and variables
        
        #region INI parsing commands
        
        /// <summary>
        /// Function to parse the INI configuration file and return an array of INISection structures
        /// </summary>
        /// <param name="iniFileName">The name of the INI file</param>
        /// <returns>return an array of INISection structures, which consists of a section
        /// name and a Hashtable of parameter names and values.</returns>
        public static INISection[] iniParse(String iniFileName)
        {
            StreamReader iniFileSR;
            Regex iniSecHdr = new Regex("\\[[A-Za-z]*\\]");
            String[] paramAndValue;
            String currLine;
            ArrayList tempRetVal = new ArrayList(4);
            UInt32 lineCnt = 0;
            INISection currSec = new INISection();
            Boolean inASection = false;

            Console.WriteLine("Parsing AIS INI file, {0}.", iniFileName);
            Debug.DebugMSG("Opening file {0}.", iniFileName);
            try
            {
                iniFileSR = new StreamReader(new FileStream(iniFileName, FileMode.Open, FileAccess.Read));
            }
            catch (FileNotFoundException fnfe)
            {
                Console.WriteLine(fnfe.Message);
                throw fnfe;
            }

            // Parse INI file
            
            // Count number of lines in file
            while (!iniFileSR.EndOfStream)
            {
                iniFileSR.ReadLine();
                lineCnt++;
            }
            iniFileSR.BaseStream.Seek(0, SeekOrigin.Begin);

            for (int i = 0; i< lineCnt; i++)
            {
                currLine = iniFileSR.ReadLine();

                // Ignore comment and empty lines
                if ( (currLine.Trim().StartsWith(";")) || (currLine.Trim().Equals("")) )
                {
                    continue;
                }                                  
                
                // If we find a section header, begin a new section
                Match m = iniSecHdr.Match(currLine);
                if (m.Success)
                {
                    if (inASection)
                    {
                        tempRetVal.Add(currSec);
                    }
                    inASection = true;
                    currSec = new INISection();
                    currSec.iniSectionName = m.Value.ToUpper().Trim('[', ']');
                    currSec.sectionValues = new Hashtable();
                    Debug.DebugMSG("INI Section: {0}", currSec.iniSectionName);
                    continue;
                }

                // If we find key/value paramter pairs, parse the value and key
                if (currLine.Contains("="))
                {
                    // Split the name at the '=' sign
                    paramAndValue = currLine.Split(new char[1] { '=' }, StringSplitOptions.RemoveEmptyEntries);

                    // Trim the param name and value
                    paramAndValue[0] = paramAndValue[0].Trim().ToUpper();
                    paramAndValue[1] = paramAndValue[1].Trim();

                    // Hex values must be prefixed by "0x"
                    if (paramAndValue[1].StartsWith("0x") || paramAndValue[1].StartsWith("0X"))
                        currSec.sectionValues[paramAndValue[0]]
                            = UInt32.Parse(paramAndValue[1].Replace("0x", ""), NumberStyles.AllowHexSpecifier);
                    else
                    {
                        UInt32 value;
                        if (UInt32.TryParse(paramAndValue[1], out value))
                            currSec.sectionValues[paramAndValue[0]] = value;
                        else
                            currSec.sectionValues[paramAndValue[0]] = paramAndValue[1];
                    }
                    Debug.DebugMSG("\t{0} = {1}", paramAndValue[0], currSec.sectionValues[paramAndValue[0]]);
                    continue;
                }
                
                // Any other lines throw an error
                throw new Exception("Bad INI file at line " + i.ToString() + ".");

            } // End of parsing INI
            
            // Add last section to return value
            if (inASection)
            {
                tempRetVal.Add(currSec);
            }

            // Return parsed ini sections
            INISection[] retVal = new INISection[tempRetVal.Count];
            tempRetVal.CopyTo(0, retVal, 0, tempRetVal.Count);

            return retVal;
        }

        #endregion

        #region AIS Generation methods

        /// <summary>
        /// genAIS command.  Always use section-by-section CRC checks
        /// </summary>
        /// <param name="coffFileName"></param>
        /// <param name="bootMode"></param>
        /// <returns>Bytes of the binary or text AIS command</returns>
        public static Byte[] GenAIS(String coffFileName,
                                    AISGen devAISGen)
        {
            UInt32 busWidth = 8;
            UInt32 numTargetSections;
            //UInt32 totalLoadableSize = 0;
            UInt32 numWords;
            Hashtable UARTSendDONE_DataSection=null, UARTSendDONE_TextSection=null;
                       
            // COFF file objects for the main application and the AIS extras executable
            COFFFile cf=null, AISExtrasCF=null;

            // ---------------------------------------------------------
            // ****************** BEGIN AIS GENERATION *****************
            // ---------------------------------------------------------
            Console.WriteLine("Begining the AIS file generation.");

            #region AIS Generation
            
            // Setup the binary writer to generate the temp AIS file
            devAISGen.devAISFileStream =  new FileStream( devAISGen.devAISFileName,
                                                          FileMode.Create,
                                                          FileAccess.Write);
            EndianBinaryWriter tempAIS_bw = new EndianBinaryWriter(
                                                          devAISGen.devAISFileStream,
                                                          devAISGen.devEndian);

            #region INI File parsing
            // Parse the INI file (describes details for initializing the device) 
            // using AIS ROM functions and possibly AISExtras functions)
            INISection[] iniSecs = AISGen.iniParse(devAISGen.devINIFileName);
            
                        
            // Get data from the GENERAL INI Section
            for (UInt32 i = 0; i < iniSecs.Length; i++)
            {
                INISection sec = iniSecs[i];
                if (sec.iniSectionName.Equals("GENERAL", StringComparison.OrdinalIgnoreCase))
                {
                    foreach (DictionaryEntry de in sec.sectionValues)
                    {
                        // Read AEMIF buswidth
                        if (((String)de.Key).Equals("BUSWIDTH"))
                            busWidth = (UInt32)sec.sectionValues["BUSWIDTH"];
                        // Read BootMode (unless already set)
                        if ((((String)de.Key).Equals("BOOTMODE")) && (devAISGen.bootMode == BootModes.NONE))
                             devAISGen.bootMode = (BootModes) Enum.Parse(typeof(BootModes), (String)sec.sectionValues["BOOTMODE"], true);
                    }
                }
            }

            // Check to make sure some bootmode has been set, if not make a default
            if (devAISGen.bootMode == BootModes.NONE)
            {
                devAISGen.bootMode = BootModes.EMIFA;
            }
            Console.WriteLine("AIS file being generated for the {0} bootmode.",Enum.GetName(typeof(BootModes),devAISGen.bootMode));
            #endregion

            #region AIS Extras and UARTSendDONE Setup
            //Create the COFF file object for the AISExtras file (if it exists/is defined)
            if (devAISGen.AISExtraFileName != null)
            {
                EmbeddedFileIO.ExtractFile(Assembly.GetExecutingAssembly(), devAISGen.AISExtraFileName, false);

                if (File.Exists(devAISGen.AISExtraFileName))
                {
                    // The file exists, so use it
                    AISExtrasCF = new COFFFile(devAISGen.AISExtraFileName);
                }
                else
                {
                    throw new FileNotFoundException("The AISExtra file, " + devAISGen.AISExtraFileName + ", was not found.");
                }
            }

            // If we have the AISExtras COFF file object, use it to setup the addresses
            // for the AISExtra functions.  These will utilize the L1P and L1D spaces that
            // are usually cache.
            if (AISExtrasCF != null)
            {
                // Use symbols to get address for AISExtra functions and parameters
                for (Int32 i = 0; i < devAISGen.AISExtraFunc.Length; i++)
                {
                    Hashtable sym = AISExtrasCF.symFind("_" + devAISGen.AISExtraFunc[i].funcName);
                    devAISGen.AISExtraFunc[i].funcAddr = (UInt32)sym["value"];
                    sym = AISExtrasCF.symFind(".params");
                    devAISGen.AISExtraFunc[i].paramAddr = (UInt32)sym["value"];
                }

                // If the bootMode is UART we need the UARTSendDONE function
                if (devAISGen.bootMode == BootModes.UART)
                {
                    // Set address for the UARTSendDone Function in the .text:uart section
                    UARTSendDONE_TextSection = AISExtrasCF.secFind(".text:uart");
                    UARTSendDONE_DataSection = AISExtrasCF.secFind(".data:uart");
                    
                    // Eliminate these as loadable since they will be loaded first, separate
                    // from the main COFF file
                    UARTSendDONE_TextSection["copyToTarget"] = false;
                    UARTSendDONE_DataSection["copyToTarget"] = false;
                    AISExtrasCF.Header["numTargetSections"] = ((UInt32)AISExtrasCF.Header["numTargetSections"] - (UInt32)2);
                    
                    // Set the actual run address for the UARTSendDONE function
                    if ((UARTSendDONE_TextSection != null) && (UARTSendDONE_DataSection != null))
                    {
                        Debug.DebugMSG("UART section found");
                        devAISGen.UARTSendDONEAddr = (UInt32)UARTSendDONE_TextSection["phyAddr"];
                        Hashtable sym = AISExtrasCF.symFind("_UARTSendDONE");
                        if (sym != null)
                        {
                            devAISGen.UARTSendDONEAddr = (UInt32)sym["value"];
                            Debug.DebugMSG("UARTSendDONE at 0x{0:X8}",devAISGen.UARTSendDONEAddr);
                        }
                    }
                    else
                    {
                        Console.WriteLine("UARTSendDONE function not found in AISExtra COFF file!!!");
                        return null;
                    }
                }
            }

            // Setup boolean indicating whether to include UARTSendDONE jump commands
            if ((devAISGen.bootMode == BootModes.UART) && (devAISGen.UARTSendDONEAddr != 0x0))
                devAISGen.SendUARTSendDONE = true;
            else
                devAISGen.SendUARTSendDONE = false;

            #endregion

            #region Write AIS MAGIC Number and initial data
            // Write the premilinary header and fields (everything before first AIS command)
            switch (devAISGen.bootMode)
            {
                case BootModes.EMIFA:
                    {
                        if (busWidth == 16)
                            tempAIS_bw.Write((UInt32)1);
                        else
                            tempAIS_bw.Write((UInt32)0);
                        tempAIS_bw.Write((UInt32)AIS.MagicNumber);
                        break;
                    }
                case BootModes.NAND:
                    {
                        tempAIS_bw.Write((UInt32)AIS.MagicNumber);
                        tempAIS_bw.Write((UInt32)0);
                        tempAIS_bw.Write((UInt32)0);
                        tempAIS_bw.Write((UInt32)0);
                        break;
                    }
                case BootModes.UART:
                    {
                        tempAIS_bw.Write((UInt32)AIS.MagicNumber);
                        break;
                    }
            };
            #endregion

            #region Send UARTSendDONE sections
            //Send UART code if it exists
            if (devAISGen.SendUARTSendDONE)
            {
                CRCCheckType tempCRCType = devAISGen.crcType;
                devAISGen.crcType = CRCCheckType.NO_CRC;
                devAISGen.SendUARTSendDONE = false;
                AISSectionLoad(AISExtrasCF, UARTSendDONE_TextSection, devAISGen);
                AISSectionLoad(AISExtrasCF, UARTSendDONE_DataSection, devAISGen);
                devAISGen.SendUARTSendDONE = true;
                devAISGen.crcType = tempCRCType;
            }
            #endregion
  
            #region ROM Function insertion

            // Insert words for ROM function execution
            for (UInt32 i = 0; i < devAISGen.ROMFunc.Length; i++)
            {
                for (UInt32 j = 0; j < iniSecs.Length; j++)
                {
                    if (iniSecs[j].iniSectionName.Equals(devAISGen.ROMFunc[i].iniSectionName))
                    {
                        UInt32 funcIndex = i;
                        tempAIS_bw.Write((UInt32)AIS.FunctionExec);
                        tempAIS_bw.Write((((UInt32)devAISGen.ROMFunc[i].numParams) << 16) + ((UInt32)funcIndex));
                        // Write Paramter values read from INI file
                        for (Int32 k = 0; k < devAISGen.ROMFunc[i].numParams; k++)
                        {
                            tempAIS_bw.Write((UInt32)iniSecs[j].sectionValues[devAISGen.ROMFunc[i].paramNames[k].ToString()]);
                            Debug.DebugMSG(((UInt32)iniSecs[j].sectionValues[devAISGen.ROMFunc[i].paramNames[k].ToString()]).ToString());
                        }
                        // Call UARTSendDONE from AIS Extras if this is for UART boot
                        if (devAISGen.SendUARTSendDONE)
                        {
                            tempAIS_bw.Write((UInt32)AIS.Jump);
                            tempAIS_bw.Write(devAISGen.UARTSendDONEAddr);
                        }
                    }
                }
            }

            #endregion

            #region AIS executable data download
            if (AISExtrasCF != null)
            {
                // Load the AISExtras COFF file
                AISCOFFLoad(AISExtrasCF, devAISGen);
            }
            #endregion

            #region AIS Extras init function execution
            //Insert calls for any AISExtra Init functions (like power domains)
            if (AISExtrasCF != null)
            {
                for (UInt32 i = 0; i < devAISGen.AISExtraFunc.Length; i++)
                {
                    if (devAISGen.AISExtraFunc[i].isInitFunc)
                    {
                        for (UInt32 j = 0; j < iniSecs.Length; j++)
                        {
                            if (iniSecs[j].iniSectionName.Equals(devAISGen.AISExtraFunc[i].iniSectionName))
                            {
                                for (UInt32 k = 0; k < devAISGen.AISExtraFunc[i].numParams; k++)
                                {
                                    // Write SET command
                                    tempAIS_bw.Write((UInt32)AIS.Set);
                                    //Write type field (32-bit only)
                                    tempAIS_bw.Write((UInt32)0x3);
                                    // Write appropriate parameter address
                                    tempAIS_bw.Write((UInt32) (devAISGen.AISExtraFunc[i].paramAddr + (k * 4)));
                                    //Write data to write
                                    tempAIS_bw.Write((UInt32)iniSecs[j].sectionValues[devAISGen.AISExtraFunc[i].paramNames[k].ToString()]);
                                    //Write Sleep value (should always be zero)
                                    //tempAIS_bw.Write((UInt32)0x1000);
                                    tempAIS_bw.Write((UInt32)0x0);
                                }

                                // Now that params are set, Jump to function
                                tempAIS_bw.Write((UInt32)AIS.Jump);
                                tempAIS_bw.Write(devAISGen.AISExtraFunc[i].funcAddr);

                                // Call UARTSendDONE from AIS Extras if this is for UART boot
                                if (devAISGen.SendUARTSendDONE)
                                {
                                    tempAIS_bw.Write((UInt32)AIS.Jump);
                                    tempAIS_bw.Write(devAISGen.UARTSendDONEAddr);
                                }
                            }
                        }
                    }
                }
            }
            #endregion

            #region Application COFF file creation and loading
            // Create the COFF file object for the main application being loaded
            if (File.Exists(coffFileName))
            {
                // The file exists, so use it
                cf = new COFFFile(coffFileName);
                Debug.DebugMSG("FileName {0} found.", coffFileName);
            }
            else
            {
                Debug.DebugMSG("FileName {0} NOT found.", coffFileName);
                throw new FileNotFoundException("AISGen: COFF file, " + coffFileName + ", not found.");
            }
            
            if (cf != null)
            {
                // Load the AISExtras COFF file
                AISCOFFLoad(cf, devAISGen);
            }

            #endregion

            #region Insert Final JUMP_CLOSE command
            tempAIS_bw.Write((UInt32)AIS.Jump_Close);
            tempAIS_bw.Write((UInt32)cf.Header["optEntryPoint"]);
            numTargetSections = (UInt32)cf.Header["numTargetSections"];
            if (AISExtrasCF != null)
            {
                numTargetSections += (UInt32)AISExtrasCF.Header["numTargetSections"];
            }
                
            tempAIS_bw.Write((UInt32)numTargetSections);
            //tempAIS_bw.Write((UInt32)0x0);
            
            //tempAIS_bw.Write((UInt32)totalLoadableSize);
            tempAIS_bw.Write((UInt32)0x0);

            // Flush the data to the file and then return to start
            devAISGen.devAISFileStream.Close();
            #endregion

            #endregion

            #region Prepare the return byte array
            // Now create return Byte array based on tempAIS file and the bootmode
            EndianBinaryReader tempAIS_br;
            tempAIS_br = new EndianBinaryReader(
                    new FileStream(devAISGen.devAISFileName, FileMode.Open, FileAccess.Read),
                    Endian.LittleEndian);
                        
            ASCIIEncoding enc = new ASCIIEncoding();

            // Setup the binary reader object
            if (devAISGen.bootMode == BootModes.UART)
            {
                numWords = ((UInt32)tempAIS_br.BaseStream.Length) >> 2;
                devAISGen.AISData = new Byte[numWords << 3];   //Each word converts to 8 ASCII bytes
            }
            else
            {
                numWords = ((UInt32)tempAIS_br.BaseStream.Length) >> 2;
                devAISGen.AISData = new Byte[numWords << 2];   //Each word converts to 4 binary bytes
            }

            Debug.DebugMSG("Number of words in the stream is {0}", numWords);

            // Copy the data to the output Byte array
            for (UInt32 i = 0; i < numWords; i++)
            {
                if (devAISGen.bootMode == BootModes.UART)
                    enc.GetBytes(tempAIS_br.ReadUInt32().ToString("X8")).CopyTo(devAISGen.AISData, i * 8);
                else
                    BitConverter.GetBytes(tempAIS_br.ReadUInt32()).CopyTo(devAISGen.AISData, i * 4);
            }

            // Close the binary reader
            tempAIS_br.Close();

            // Delete the temp AIS data file
            File.Delete(devAISGen.devAISFileName);

            // Close the COFF files since we are done with them
            AISExtrasCF.Close();
            cf.Close();
            
            // Clean up any embedded file resources that may have been extracted
            EmbeddedFileIO.CleanUpEmbeddedFiles();

            #endregion

            // Console output
            Console.WriteLine("AIS file generation was successful.");
            // ---------------------------------------------------------
            // ******************* END AIS GENERATION ******************
            // ---------------------------------------------------------

            // Return Byte Array
            return devAISGen.AISData;
        }

        /// <summary>
        /// Secondary genAIS thats calls the first
        /// </summary>
        /// <param name="coffFileName">File name of .out file</param>
        /// <param name="bootmode">String containing desired boot mode</param>
        /// <returns>an Array of bytes to write to create an AIS file</returns>
        public static Byte[] GenAIS(String coffFileName,
                                    String bootmode,
                                    AISGen devAISGen)
        {
            devAISGen.bootMode = (BootModes)Enum.Parse(typeof(BootModes), bootmode, true);
            Console.WriteLine("Chosen bootmode is {0}.", devAISGen.bootMode.ToString());
            return GenAIS(coffFileName, devAISGen);
        }
        
        /// <summary>
        /// Secondary genAIS thats calls the first
        /// </summary>
        /// <param name="coffFileName">File name of .out file</param>
        /// <param name="bootmode">AISGen.BootModes Enum value containing desired boot mode</param>
        /// <returns>an Array of bytes to write to create an AIS file</returns>
        public static Byte[] GenAIS(String coffFileName,
                                    BootModes bootmode,
                                    AISGen devAISGen)
        {
            devAISGen.bootMode = bootmode;
            Console.WriteLine("Chosen bootmode is {0}.", devAISGen.bootMode.ToString());
            return GenAIS(coffFileName, devAISGen);
        }

        /// <summary>
        /// AIS Section Load command generation
        /// </summary>
        /// <param name="cf">The COFFfile object that the section comes from.</param>
        /// <param name="secHeader">The Hashtable object of the section header to load.</param>
        /// <param name="devAISGen">The specific device AIS generator object.</param>
        /// <returns>retType enumerator indicating success or failure.</returns>
        private static retType AISSectionLoad(
                        COFFFile cf,
                        Hashtable secHeader,
                        AISGen devAISGen)
        {
            UInt32 secSize = (UInt32)secHeader["byteSize"];
            String secName = (String)secHeader["name"];
            UInt32 runAddr = (UInt32)secHeader["phyAddr"];
            UInt32 loadAddr = (UInt32)secHeader["virAddr"];
            UInt32 numWords = (UInt32)secHeader["wordSize"];
            UInt32[] secData = cf[secName];
            UInt32 crcVal;
            Byte[] srcCRCData = new Byte[secSize + 8];

            EndianBinaryWriter ebw = new EndianBinaryWriter(
                devAISGen.devAISFileStream,
                devAISGen.devEndian);
            
            // If we are doing section-by-section CRC, then zero out the CRC value
            if (devAISGen.crcType == CRCCheckType.SECTION_CRC)
                devAISGen.devCRC.ResetCRC();

            // Write Section_Load AIS command, load address, and size
            ebw.Write((UInt32)AIS.Section_Load);
            ebw.Write(loadAddr);
            ebw.Write(secSize);

            // Copy bytes to byte array for future CRC calculation
            if (devAISGen.crcType != CRCCheckType.NO_CRC)
            {
                Endian.swapEndian(BitConverter.GetBytes(loadAddr)).CopyTo(srcCRCData, 0);
                Endian.swapEndian(BitConverter.GetBytes(secSize)).CopyTo(srcCRCData, 4);
            }

            // Now write contents
            for (int k = 0; k < numWords; k++)
            {
                ebw.Write(secData[k]);
                // Copy bytes to array for future CRC calculation
                if (devAISGen.crcType != CRCCheckType.NO_CRC)
                {
                    Endian.swapEndian(BitConverter.GetBytes(secData[k])).CopyTo(srcCRCData, (8 + k * 4));
                }
            }

            // Check if we need to output DONE to the UART
            if (devAISGen.SendUARTSendDONE)
            {
                ebw.Write((UInt32)AIS.Jump);
                ebw.Write(devAISGen.UARTSendDONEAddr);
            }

            // Check for need to do IDMA relocation
            if (loadAddr != runAddr)
            {
                Boolean LoadInCache = false, RunInL1PCache = false;
                for (int j = 0; j < devAISGen.Cache.Length; j++)
                {
                    LoadInCache = (LoadInCache || devAISGen.Cache[j].InCache(loadAddr));
                    RunInL1PCache = (RunInL1PCache || 
                                       ((devAISGen.Cache[j].InCache(runAddr)) &&
                                       (devAISGen.Cache[j].level == CacheLevel.L1) &&
                                       (devAISGen.Cache[j].type == CacheType.Program))
                                       );
                }
                Debug.DebugMSG("LoadInCache : {0}, RunInL1PCache: {1}", LoadInCache, RunInL1PCache);
                if (RunInL1PCache && LoadInCache)
                {
                    //Write Set commands to make the IDMA do transfer
                    // Write SET command
                    ebw.Write((UInt32)AIS.Set);
                    //Write type field (32-bit only)
                    ebw.Write((UInt32)0x3);
                    // Write appropriate parameter address
                    ebw.Write((UInt32)devAISGen.IDMA[1].IDMA_SRC_ADDR);
                    //Write data to write (the src address)
                    ebw.Write((UInt32)loadAddr);
                    //Write Sleep value
                    ebw.Write((UInt32)0x0);                   

                    // Write SET command
                    ebw.Write((UInt32)AIS.Set);
                    //Write type field (32-bit only)
                    ebw.Write((UInt32)0x3);
                    // Write appropriate parameter address
                    ebw.Write((UInt32)devAISGen.IDMA[1].IDMA_DEST_ADDR);
                    //Write data to write (the dest address)
                    ebw.Write((UInt32)runAddr);
                    //Write Sleep value
                    ebw.Write((UInt32)0x0);

                    // Write SET command
                    ebw.Write((UInt32)AIS.Set);
                    //Write type field (32-bit only)
                    ebw.Write((UInt32)0x3);
                    // Write appropriate parameter address
                    ebw.Write((UInt32)devAISGen.IDMA[1].IDMA_CNT_ADDR);
                    //Write data to write (the byte count - must be multiple of four)
                    ebw.Write((UInt32)(numWords * 4));
                    //Write Sleep value
                    ebw.Write((UInt32)0x1000);

                    // Check if we need to output DONE to the UART
                    if (devAISGen.SendUARTSendDONE)
                    {
                        ebw.Write((UInt32)AIS.Jump);
                        ebw.Write((UInt32)devAISGen.UARTSendDONEAddr);
                    }
                }
            }

            // Perform CRC calculation of the sub-section's contents
            if (devAISGen.crcType != CRCCheckType.NO_CRC)
            {
                crcVal = devAISGen.devCRC.IncrementalSimpleCRC(srcCRCData);
                if (devAISGen.crcType == CRCCheckType.SECTION_CRC)
                {
                    // Write CRC request command, value, and jump value to temp AIS file
                    ebw.Write((UInt32)AIS.RequestCRC);
                    ebw.Write((UInt32)crcVal);
                    ebw.Write(((Int32)(-1) * (Int32)(secSize + 12 + 12)));
                }
            }

            Debug.DebugMSG("Section Load complete");

            return retType.SUCCESS;
        }

        /// <summary>
        /// AIS COFF file Load command generation (loads all sections)
        /// </summary>
        /// <param name="cf">The COFFfile object that the section comes from.</param>
        /// <param name="devAISGen">The specific device AIS generator object.</param>
        /// <returns>retType enumerator indicating success or failure.</returns>
        private static retType AISCOFFLoad(
                        COFFFile cf,
                        AISGen devAISGen)
        {
            UInt32 numSymbols, numSections, numTargetSections;
            Hashtable[] symbols, sections, targetSections;
            UInt32 totalLoadableSize = 0;
            UInt32 TIBootSetupAddr = 0x00000000;

            EndianBinaryWriter ebw = new EndianBinaryWriter(
                devAISGen.devAISFileStream,
                devAISGen.devEndian);
            
            if (!devAISGen.devEndian.ToString().Equals(cf.Endianness))
            {
                Console.WriteLine("Endianness mismatch. Device is {0} endian, COFF file is {1} endian",
                    devAISGen.devEndian.ToString(),
                    cf.Endianness);
                return retType.FAIL;
            }

            // Get COFF file section info
            numSections = (UInt32)cf.Header["numSectionHdrs"];
            sections = cf.Sections;

            // Get COFF file symbol info
            numSymbols = (UInt32)cf.Header["numEntriesInSymTable"];
            symbols = cf.Symbols;

            // Make sure the .TIBoot section is first (if it exists)
            Hashtable firstSection = sections[0];
            for (Int32 i = 1; i < numSections; i++)
            {
                if (((String)sections[i]["name"]).Equals(".TIBoot"))
                {
                    sections[0] = sections[i];
                    sections[i] = firstSection;
                    break;
                }
            }

            // Look for the _TIBootSetup symbol (if it exists)
            for (Int32 i = 0; i < numSymbols; i++)
            {
                if (((String)symbols[i]["name"]).Equals("_TIBootSetup"))
                {
                    TIBootSetupAddr = (UInt32)symbols[i]["value"];
                    Debug.DebugMSG("Symbol :{0}:", (String)symbols[i]["name"]);
                }
            }

            // Create array of Hashtable describing loadable sections
            numTargetSections = (UInt32)cf.Header["numTargetSections"];
            Debug.DebugMSG("Number of target loadable sections = {0}", numTargetSections);
            targetSections = new Hashtable[numTargetSections];
            Debug.DebugMSG("Loadable Sections:");
            for (Int32 i = 0, j = 0; i < numSections; i++)
            {
                if ((Boolean)sections[i]["copyToTarget"])
                {
                    targetSections[j++] = sections[i];
                    totalLoadableSize += (UInt32)sections[i]["byteSize"];
                    Debug.DebugMSG("\t{0}. Section Name: {1}, Section Size: {2}",j, (String)sections[i]["name"], (UInt32)sections[i]["byteSize"]);
                }
            }

            // If CRC type is either section or single CRC, then enable CRC
            if (devAISGen.crcType != CRCCheckType.NO_CRC)
            {
                ebw.Write((UInt32)AIS.EnableCRC);
            }   

            // Do all SECTION_LOAD commands
            for (Int32 i = 0; i < numTargetSections; i++)
            {
                if (AISSectionLoad(cf,targetSections[i],devAISGen) != retType.SUCCESS)
                    return retType.FAIL;
                totalLoadableSize += (UInt32)targetSections[i]["byteSize"];
                
                // Check for need to do TIBoot initialization
                if (i == 0)
                {
                    if (TIBootSetupAddr != 0)
                    {
                        ebw.Write((UInt32)AIS.Jump);
                        ebw.Write(TIBootSetupAddr);
                    }
                }
            } // End of SECTION_LOAD commands

            // If CRC type is for single CRC, send Request CRC now
            if (devAISGen.crcType == CRCCheckType.SINGLE_CRC)
            {
                // Write CRC request command, value, and jump value to temp AIS file
                ebw.Write((UInt32)AIS.RequestCRC);
                ebw.Write(devAISGen.devCRC.CurrentCRC);
                ebw.Write( (Int32)(-1) * (Int32)(totalLoadableSize + (12*numTargetSections) + 12) );
            }

            // If either CRC type is section or single CRC, now disable them
            if (devAISGen.crcType != CRCCheckType.NO_CRC)
            {
                ebw.Write((UInt32)AIS.DisableCRC);
            }

            Debug.DebugMSG("AISCOFFLoad Complete");

            return retType.SUCCESS;
        }     
        #endregion 

        #region AIS UART send method
        /// <summary>
        /// Public method to send AIS data via UART.
        /// </summary>
        /// <param name="AISdata">Array of bytes from text AIS file.</param>
        /// <param name="sp">Serial Port object to send data to.</param>
        public static retType SendToUART(Byte[] AISdata, SerialPort sp, AISGen devAISGen)
        {
            Int32 len = AISdata.Length;
            Int32 i = 0;
            Int32 numBytesToSend;
            AIS AIScmd;
            //SerialDataReceivedEventHandler DREH = new SerialDataReceivedEventHandler(SerialReadCallBack);
            
            while (i < len)
            {
                // Get the current AIS command
                AIScmd = (AIS) Enum.ToObject(typeof(AIS), UInt32.Parse(ASCIIEncoding.ASCII.GetString(AISdata, i, 8), NumberStyles.HexNumber));
                Debug.DebugMSG("Command: {0}", Enum.GetName(typeof(AIS), AIScmd));
                                
                // Determine how many bytes to send at once based on what command was sent
                // This lead us to the next viable AIS command.
                switch (AIScmd)
                {
                    case AIS.MagicNumber:
                        numBytesToSend = 8;
                        sp.Write(AISdata, i, numBytesToSend);
                        break;
                    case AIS.RequestCRC:
                        numBytesToSend = 8*3;
                        sp.Write(AISdata, i, numBytesToSend);
                        break;
                    case AIS.EnableCRC:
                        numBytesToSend = 8;
                        sp.Write(AISdata, i, numBytesToSend);
                        break;
                    case AIS.DisableCRC:
                        numBytesToSend = 8;
                        sp.Write(AISdata, i, numBytesToSend);
                        break;
                    case AIS.FunctionExec:
                        UInt16 numParams = UInt16.Parse(ASCIIEncoding.ASCII.GetString(AISdata, i + 8, 4), NumberStyles.HexNumber);
                        UInt16 funcIndex = UInt16.Parse(ASCIIEncoding.ASCII.GetString(AISdata, i + 12, 4), NumberStyles.HexNumber);
                        Debug.DebugMSG("numParams: {0}, funcIndex: {1}", numParams, funcIndex);
                        numBytesToSend = 8*(2 + numParams);
                        sp.Write(AISdata, i, numBytesToSend);
                        break;
                    case AIS.Section_Load:
                        Int32 secSize = Int32.Parse(ASCIIEncoding.ASCII.GetString(AISdata, i + 16, 8), NumberStyles.HexNumber);
                        Int32 secAddr = Int32.Parse(ASCIIEncoding.ASCII.GetString(AISdata, i + 8, 8), NumberStyles.HexNumber);
                        numBytesToSend = 8*3 + (secSize << 1);
                        Debug.DebugMSG("Loading {0} bytes to address 0x{1:X8}.", secSize, secAddr);
                        sp.Write(AISdata, i, numBytesToSend);
                        //Thread.Sleep(1);
                        break;
                    case AIS.Section_Fill:
                        numBytesToSend = 8*5;
                        sp.Write(AISdata, i, numBytesToSend);
                        break;
                    case AIS.Get:
                        numBytesToSend = 8 * 5;
                        sp.Write(AISdata, i, numBytesToSend);
                        Byte[] buf = new Byte[4];
                        sp.Read(buf, 0, 4);
                        Debug.DebugMSG("0x{3:X2}{2:X2}{1:X2}{0:X2}", buf[0], buf[1], buf[2], buf[3]);
                        break;
                    case AIS.Set:
                        numBytesToSend = 8*5;
                        sp.Write(AISdata, i, numBytesToSend);
                        //Thread.Sleep(1);
                        break;
                    case AIS.Jump:
                        numBytesToSend = 8 * 2;
                        String jumpAddrStr = (new ASCIIEncoding()).GetString(AISdata,i+8,8);
                        UInt32 jumpAddr = UInt32.Parse(jumpAddrStr,NumberStyles.HexNumber);
                        Debug.DebugMSG("Jump Address: {0:X8}", jumpAddr);
                        if (jumpAddr !=0)
                        {
                            sp.Write(AISdata, i, numBytesToSend);
                            if ((jumpAddr == devAISGen.UARTSendDONEAddr))
                            {
                                if (waitForSequence("   DONE\0", "BOOTME ", sp, false))
                                {
                                    Debug.DebugMSG("DONE found!!");
                                }
                                else
                                {
                                    Debug.DebugMSG("DONE NOT found!!");
                                    return retType.FAIL;
                                }
                            }
                        }
                        break;
                    case AIS.Jump_Close:
                        numBytesToSend = 8*4;
                        sp.Write(AISdata, i, numBytesToSend);
                        break;
                    default:
                        Debug.DebugMSG("Default");
                        numBytesToSend = 8;
                        sp.Write(AISdata, i, numBytesToSend);

                        break;
                }                
                // Increment to Byte array pointer
                i += numBytesToSend;
            }
            sp.Write(AISdata, i, len - i);
            Thread.Sleep(10);
            return retType.SUCCESS;
            //Console.Out.Flush();
        }

        /// <summary>
        /// Waitforsequence with option for verbosity
        /// </summary>
        /// <param name="str">String to look for</param>
        /// <param name="altStr">String to look for but don't want</param>
        /// <param name="sp">SerialPort object.</param>
        /// <param name="verbose">Boolean to indicate verbosity.</param>
        /// <returns>Boolean to indicate if str or altStr was found.</returns>
        private static Boolean waitForSequence(String str, String altStr, SerialPort sp, Boolean verbose)
        {
            Boolean strFound = false, altStrFound = false;
            Byte[] input = new Byte[256];
            String inputStr;
            Int32 i;

            while ((!strFound) && (!altStrFound))
            {

                i = 0;
                do
                {
                    input[i++] = (Byte)sp.ReadByte();
                    //Console.Write(input[i - 1] + " ");
                } while ((input[i - 1] != 0) &&
                          (i < (input.Length - 1)) &&
                          (input[i - 1] != 0x0A) &&
                          (input[i - 1] != 0x0D));

                // Convert to string for comparison
                if ((input[i - 1] == 0x0A) || (input[i - 1] == 0x0D))
                    inputStr = (new ASCIIEncoding()).GetString(input, 0, i - 1);
                else
                    inputStr = (new ASCIIEncoding()).GetString(input, 0, i);

                if (inputStr.Length == 0)
                {
                    continue;
                }

                // Compare Strings to see what came back
                if (verbose)
                    Console.WriteLine("\tTarget Device: {0}", inputStr);
                if (inputStr.Contains(altStr))
                {
                    altStrFound = true;
                    if (String.Equals(str, altStr))
                    {
                        strFound = altStrFound;
                    }
                }
                else if (inputStr.Contains(str))
                {
                    strFound = true;
                }
                else
                {
                    strFound = false;
                }
            }
            return strFound;
        }

        #endregion

        #endregion
    }
                        
} //end of AISGenLib namespace

