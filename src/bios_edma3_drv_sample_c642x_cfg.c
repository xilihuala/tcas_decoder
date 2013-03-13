/*******************************************************************************
**+--------------------------------------------------------------------------+**
**|                            ****                                          |**
**|                            ****                                          |**
**|                            ******o***                                    |**
**|                      ********_///_****                                   |**
**|                      ***** /_//_/ ****                                   |**
**|                       ** ** (__/ ****                                    |**
**|                           *********                                      |**
**|                            ****                                          |**
**|                            ***                                           |**
**|                                                                          |**
**|         Copyright (c) 1998-2006 Texas Instruments Incorporated           |**
**|                        ALL RIGHTS RESERVED                               |**
**|                                                                          |**
**| Permission is hereby granted to licensees of Texas Instruments           |**
**| Incorporated (TI) products to use this computer program for the sole     |**
**| purpose of implementing a licensee product based on TI products.         |**
**| No other rights to reproduce, use, or disseminate this computer          |**
**| program, whether in part or in whole, are granted.                       |**
**|                                                                          |**
**| TI makes no representation or warranties with respect to the             |**
**| performance of this computer program, and specifically disclaims         |**
**| any responsibility for any damages, special or consequential,            |**
**| connected with the use of this program.                                  |**
**|                                                                          |**
**+--------------------------------------------------------------------------+**
*******************************************************************************/

/** \file   bios_edma3_drv_sample_c642x_cfg.c

  \brief  SoC specific EDMA3 hardware related information like number of
  transfer controllers, various interrupt ids etc. It is used while
  interrupts enabling / disabling. It needs to be ported for different
  SoCs.
  
    (C) Copyright 2006, Texas Instruments, Inc
    
      \version    1.0   Anuj Aggarwal         - Created
      1.1   Anuj Aggarwal         - Made the sample app generic
      - Removed redundant arguments
      from Cache-related APIs
      - Added new function for Poll mode
      testing
      
*/

#include "edma3_drv.h"


/* C642X Specific EDMA3 Information */

/** Number of Event Queues available */
#define EDMA3_NUM_EVTQUE                                3u

/** Number of Transfer Controllers available */
#define EDMA3_NUM_TC                                    3u

/** Interrupt no. for Transfer Completion */
#define EDMA3_CC_XFER_COMPLETION_INT                    36u

/** Interrupt no. for CC Error */
#define EDMA3_CC_ERROR_INT                              37u

/** Interrupt no. for TCs Error */
#define EDMA3_TC0_ERROR_INT                             38u
#define EDMA3_TC1_ERROR_INT                             39u
#define EDMA3_TC2_ERROR_INT                             40u
#define EDMA3_TC3_ERROR_INT                             0u
#define EDMA3_TC4_ERROR_INT                             0u
#define EDMA3_TC5_ERROR_INT                             0u
#define EDMA3_TC6_ERROR_INT                             0u
#define EDMA3_TC7_ERROR_INT                             0u

/**
* EDMA3 interrupts (transfer completion, CC error etc.) correspond to different
* ECM events (SoC specific). These ECM events come
* under ECM block XXX (handling those specific ECM events). Normally, block
* 0 handles events 4-31 (events 0-3 are reserved), block 1 handles events
* 32-63 and so on. This ECM block XXX (or interrupt selection number XXX)
* is mapped to a specific HWI_INT YYY in the tcf file.
* Define EDMA3_HWI_INT to that specific HWI_INT YYY.
*/
#define EDMA3_HWI_INT                                   8u


/**
* \brief Mapping of DMA channels 0-31 to Hardware Events from
* various peripherals, which use EDMA for data transfer.
* All channels need not be mapped, some can be free also.
* 1: Mapped
* 0: Not mapped
*
* This mapping will be used to allocate DMA channels when user passes
* EDMA3_DRV_DMA_CHANNEL_ANY as dma channel id (for eg to do memory-to-memory
* copy). The same mapping is used to allocate the TCC when user passes
* EDMA3_DRV_TCC_ANY as tcc id (for eg to do memory-to-memory copy).
*
* To allocate more DMA channels or TCCs, one has to modify the event mapping.
*/
#define EDMA3_DMA_CHANNEL_TO_EVENT_MAPPING_0          0x33FFFFFCu
//#define EDMA3_DMA_CHANNEL_TO_EVENT_MAPPING_0          0xFFFFFFFCu
/**
* \brief Mapping of DMA channels 32-63 to Hardware Events from
* various peripherals, which use EDMA for data transfer.
* All channels need not be mapped, some can be free also.
* 1: Mapped
* 0: Not mapped
*
* This mapping will be used to allocate DMA channels when user passes
* EDMA3_DRV_DMA_CHANNEL_ANY as dma channel id (for eg to do memory-to-memory
* copy). The same mapping is used to allocate the TCC when user passes
* EDMA3_DRV_TCC_ANY as tcc id (for eg to do memory-to-memory copy).
*
* To allocate more DMA channels or TCCs, one has to modify the event mapping.
*/
#define EDMA3_DMA_CHANNEL_TO_EVENT_MAPPING_1          0x007F7FFFu
//#define EDMA3_DMA_CHANNEL_TO_EVENT_MAPPING_1          0xFFFFFFFFu


/* Variable which will be used internally for referring number of Event Queues. */
unsigned int numEdma3EvtQue = EDMA3_NUM_EVTQUE;

/* Variable which will be used internally for referring number of TCs. */
unsigned int numEdma3Tc = EDMA3_NUM_TC;

/**
* Variable which will be used internally for referring transfer controllers'
* error interrupts.
*/
unsigned int tcErrorInt[8] =    {
  EDMA3_TC0_ERROR_INT, EDMA3_TC1_ERROR_INT,
    EDMA3_TC2_ERROR_INT, EDMA3_TC3_ERROR_INT,
    EDMA3_TC4_ERROR_INT, EDMA3_TC5_ERROR_INT,
    EDMA3_TC6_ERROR_INT, EDMA3_TC7_ERROR_INT
};



/**
* Variable which will be used internally for referring the hardware interrupt
* for various EDMA3 interrupts.
*/
unsigned int hwInt = EDMA3_HWI_INT;


/* Driver Object Initialization Configuration */
EDMA3_DRV_GblConfigParams sampleEdma3GblCfgParams =
{
  /** Total number of DMA Channels supported by the EDMA3 Controller */
  64u,
    /** Total number of QDMA Channels supported by the EDMA3 Controller */
    8u,
    /** Total number of TCCs supported by the EDMA3 Controller */
    64u,
    /** Total number of PaRAM Sets supported by the EDMA3 Controller */
    128u,
    /** Total number of Event Queues in the EDMA3 Controller */
    3u,
    /** Total number of Transfer Controllers (TCs) in the EDMA3 Controller */
    3u,
    /** Number of Regions on this EDMA3 controller */
    4u,
    
    /**
    * \brief Channel mapping existence
    * A value of 0 (No channel mapping) implies that there is fixed association
    * for a channel number to a parameter entry number or, in other words,
    * PaRAM entry n corresponds to channel n.
    */
    0u,
    
    /** Existence of memory protection feature */
    0u,
    
    /** Global Register Region of CC Registers */
    (void *)0x01C00000u,
    /** Transfer Controller (TC) Registers */
  {
    (void *)0x01C10000u,
      (void *)0x01C10400u,
      (void *)0x01C10800u,
      (void *)NULL,
      (void *)NULL,
      (void *)NULL,
      (void *)NULL,
      (void *)NULL
  },
  /** Interrupt no. for Transfer Completion */
  EDMA3_CC_XFER_COMPLETION_INT,
  /** Interrupt no. for CC Error */
  EDMA3_CC_ERROR_INT,
  /** Interrupt no. for TCs Error */
    {
      EDMA3_TC0_ERROR_INT,
        EDMA3_TC1_ERROR_INT,
        EDMA3_TC2_ERROR_INT,
        EDMA3_TC3_ERROR_INT,
        EDMA3_TC4_ERROR_INT,
        EDMA3_TC5_ERROR_INT,
        EDMA3_TC6_ERROR_INT,
        EDMA3_TC7_ERROR_INT
    },
    
    /**
    * \brief EDMA3 TC priority setting
    *
    * User can program the priority of the Event Queues
    * at a system-wide level.  This means that the user can set the
    * priority of an IO initiated by either of the TCs (Transfer Controllers)
    * relative to IO initiated by the other bus masters on the
    * device (ARM, DSP, USB, etc)
    */
    {
        0u,
          1u,
          2u,
          0u,
          0u,
          0u,
          0u,
          0u
      },
      /**
      * \brief To Configure the Threshold level of number of events
      * that can be queued up in the Event queues. EDMA3CC error register
      * (CCERR) will indicate whether or not at any instant of time the
      * number of events queued up in any of the event queues exceeds
      * or equals the threshold/watermark value that is set
      * in the queue watermark threshold register (QWMTHRA).
      */
      {
          16u,
            16u,
            16u,
            0u,
            0u,
            0u,
            0u,
            0u
        },
        
        /**
        * \brief To Configure the Default Burst Size (DBS) of TCs.
        * An optimally-sized command is defined by the transfer controller
        * default burst size (DBS). Different TCs can have different
        * DBS values. It is defined in Bytes.
        */
        {
            16u,
              32u,
              64u,
              0u,
              0u,
              0u,
              0u,
              0u
          },
          
          /**
          * \brief Mapping from each DMA channel to a Parameter RAM set,
          * if it exists, otherwise of no use.
          */
          {
              0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u,
                8u, 9u, 10u, 11u, 12u, 13u, 14u, 15u,
                16u, 17u, 18u, 19u, 20u, 21u, 22u, 23u,
                24u, 25u, 26u, 27u, 28u, 29u, 30u, 31u,
                32u, 33u, 34u, 35u, 36u, 37u, 38u, 39u,
                40u, 41u, 42u, 43u, 44u, 45u, 46u, 47u,
                48u, 49u, 50u, 51u, 52u, 53u, 54u, 55u,
                56u, 57u, 58u, 59u, 60u, 61u, 62u, 63u
            },
            
            /**
            * \brief Mapping from each DMA channel to a TCC. This specific
            * TCC code will be returned when the transfer is completed
            * on the mapped channel.
            */
            {
                EDMA3_DRV_CH_NO_TCC_MAP, EDMA3_DRV_CH_NO_TCC_MAP, 2u, 3u,
                  4u, 5u, 6u, 7u,
                  8u, 9u, 10u, 11u,
                  12u, 13u, 14u, 15u,
                  16u, 17u, 18u, 19u,
                  20u, 21u, 22u, 23u,
                  24u, 25u, EDMA3_DRV_CH_NO_TCC_MAP, EDMA3_DRV_CH_NO_TCC_MAP,
                  28u, 29u, EDMA3_DRV_CH_NO_TCC_MAP, EDMA3_DRV_CH_NO_TCC_MAP,
                  EDMA3_DRV_CH_NO_TCC_MAP, EDMA3_DRV_CH_NO_TCC_MAP,
                  EDMA3_DRV_CH_NO_TCC_MAP, EDMA3_DRV_CH_NO_TCC_MAP,
                  36u, 37u, 38u, 39u,
                  40u, 41u, 42u, 43u,
                  44u, 45u, 46u, EDMA3_DRV_CH_NO_TCC_MAP,
                  48u, 49u, 50u, 51u,
                  52u, 53u, 54u, EDMA3_DRV_CH_NO_TCC_MAP,
                  EDMA3_DRV_CH_NO_TCC_MAP, EDMA3_DRV_CH_NO_TCC_MAP,
                  EDMA3_DRV_CH_NO_TCC_MAP, EDMA3_DRV_CH_NO_TCC_MAP,
                  EDMA3_DRV_CH_NO_TCC_MAP, EDMA3_DRV_CH_NO_TCC_MAP,
                  EDMA3_DRV_CH_NO_TCC_MAP, EDMA3_DRV_CH_NO_TCC_MAP
              },
              
              /**
              * \brief Mapping of DMA channels to Hardware Events from
              * various peripherals, which use EDMA for data transfer.
              * All channels need not be mapped, some can be free also.
              */
              {
                  EDMA3_DMA_CHANNEL_TO_EVENT_MAPPING_0,
                    EDMA3_DMA_CHANNEL_TO_EVENT_MAPPING_1
                }
    };
    
    
    /* Driver Instance Initialization Configuration */
    EDMA3_DRV_InstanceInitConfig sampleInstInitConfig =
    {
      /* Resources owned by Region 1 */
      /* ownPaRAMSets */
    //  {0xFFFFFFFFu, 0xFFFFFFFFu, 0x00000FFFu, 0x0u,
        {0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0x0u,
        0x0u, 0x0u, 0x0u, 0x0u,
        0x0u, 0x0u, 0x0u, 0x0u,
        0x0u, 0x0u, 0x0u, 0x0u},
        
        /* ownDmaChannels */
        //{0xFFFFFFFFu, 0xFFFFFFF0u},
      {0xFFFFFFFFu, 0xFFFFFFFFu}, //wj modify
      /* ownQdmaChannels */
      {0x00000080u},
      
      /* ownTccs */
      //{0xFFFFFFFFu, 0xFFFFFFF0u},
      {0xFFFFFFFFu, 0xFFFFFFFFu}, //wj modify
      
      /* Resources reserved by Region 1 */
      /* resvdPaRAMSets */
      {EDMA3_DMA_CHANNEL_TO_EVENT_MAPPING_0,
      EDMA3_DMA_CHANNEL_TO_EVENT_MAPPING_1,
      0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u,
      0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u},
      
      /* resvdDmaChannels */
      {EDMA3_DMA_CHANNEL_TO_EVENT_MAPPING_0,
      EDMA3_DMA_CHANNEL_TO_EVENT_MAPPING_1},
      
      /* resvdQdmaChannels */
      {0x0u},
      
      /* resvdTccs */
      {EDMA3_DMA_CHANNEL_TO_EVENT_MAPPING_0,
      EDMA3_DMA_CHANNEL_TO_EVENT_MAPPING_1}
    };
    
    
    /* End of File */
    
