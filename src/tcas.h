#ifndef __ADSB_H_
#define __ADSB_H_

#include "fpga_intf.h"

#ifdef _DEBUG
#define INLINE_DESC 
#else
#define INLINE_DESC 
#endif

#define EDMA3_DRV_EVENT_GPINT0 EDMA3_DRV_HW_CHANNEL_EVENT_32
#define EDMA3_DRV_EVENT_GPINT1 EDMA3_DRV_HW_CHANNEL_EVENT_33
#define EDMA3_DRV_EVENT_GPINT2 EDMA3_DRV_HW_CHANNEL_EVENT_34
#define EDMA3_DRV_EVENT_GPINT3 EDMA3_DRV_HW_CHANNEL_EVENT_35
#define EDMA3_DRV_EVENT_GPINT4 EDMA3_DRV_HW_CHANNEL_EVENT_36
#define EDMA3_DRV_EVENT_GPINT5 EDMA3_DRV_HW_CHANNEL_EVENT_37
#define EDMA3_DRV_EVENT_GPINT6 EDMA3_DRV_HW_CHANNEL_EVENT_38
#define EDMA3_DRV_EVENT_GPINT7 EDMA3_DRV_HW_CHANNEL_EVENT_39


//set these event to reserved event(16-21,26-27,55-63)...(see P38),because we use MANUAL Trigger mode 
//NOTE: the low channel number have high priority, so we set REPORT as high priority,
//we can benefit from it when more fight need to  be monitored
//S BUFFER
#define EVENT_DATA_STATE1     27
#define EVENT_DATA_STATE2     26
//#define EVENT_DATA1_DLT       21
#define EVENT_DATA1           20
//#define EVENT_DATA2_DLT       19
#define EVENT_DATA2           18

//C BUFFER (only DMA data, state can be read directly
#define EVENT_C_DATA_STATE1   58
#define EVENT_C_DATA_STATE2   57
#define EVENT_C_DATA1         56
#define EVENT_C_DATA2         55

//REPORT
#define EVENT_REPORT1         17
#define EVENT_REPORT2         16


#define FIFO_MODE 1
#define INCR_MODE 0

#define INT_ENABLE  1
#define INT_DISABLE 0

#define MAIN_CHANNEL    0
#define LINK_CHANNEL    1
#define CHAIN_CHANNEL   2

#define ROUND_AREA   0xcccccccccc

#define MAX_POOL_NUM         FPGA_POOL_NUM

//define receive buffer in one pool
#define MAX_BUF_IN_S_POOL      2  
#define MAX_BUF_IN_C_POOL      3
#define MAX_RECV_SIZE        BUFFER_SIZE 


//14-bytes is confidence bit, 8-bytes is time-stamp, 2-bytes is end-word(0xaa55)
//#define FRAME_OUT_LEN    (FRAME_LENGTH + CONF_LENGTH + TIME_LENGTH + END_LENGTH)

//#define USE_POOL

//#define MY_ADSB_ADDRESS        0xffffff

//register define
#define EDMA3_BASE             0x01C00000
#define _ER   *(volatile unsigned long*)(EDMA3_BASE + 0x1000)
#define _ERH  *(volatile unsigned long*)(EDMA3_BASE + 0x1004)
#define _ESR   *(volatile unsigned long*)(EDMA3_BASE + 0x1010)
#define _ESRH  *(volatile unsigned long*)(EDMA3_BASE + 0x1014)
#define _EMR   *(volatile unsigned long*)(EDMA3_BASE + 0x300)
#define _EMRH  *(volatile unsigned long*)(EDMA3_BASE + 0x304)
#define _EMCR  *(volatile unsigned long*)(EDMA3_BASE + 0x308)
#define _EMCRH  *(volatile unsigned long*)(EDMA3_BASE + 0x30c)

#define INTMUX1 *(unsigned int*)0x01800104
#define INTMUX2 *(unsigned int*)0x01800108
#define INTMUX3 *(unsigned int*)0x0180010c
#define CPU_IER *(unsigned int*)(0x01800000 + 0x100)
#define CPU_ICR *(unsigned int*)(0x01800000+ 0x11)

#define LISTEN_MODE        0x0
#define ACQUIRE_MODE       0x1
#define CORDNATE_MODE      0x2
#define BROADCAST_MODE     0x3

#define UP_DIRECTION       0
#define DOWN_DIRECTION     1

#endif
