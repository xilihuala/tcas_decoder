#ifndef __FPGA_INTF_H_
#define __FPGA_INTF_H_


//value of db1 
#define VALUE_DB1    0x4  //0x3  by fy

#define CS3_BASE                0x44000000
#define CS4_BASE                0x46000000

/**********************/
/*    S MODE          */
/**********************/

//FRAME SIZE
#define PRE_FRAME_LEN    8
#define LONG_FRAME_LEN   112
#define SHORT_FRAME_LEN  56

#define S_SAMPLE_FREQ      10 /*10M*/

//FRAME BUFFER SIZE: MAX_FRAME_NUMBER * UNIT_SIZE
#define MAX_FRAME_NUM      3

//the max number of frame headers in one frame buffer
#define MAX_PREAMBLE_NUMBER  15

//state buffer size (short size)
#define END_FLAG_SIZE           1
#define DATALEN_SIZE            1
#define TIMESTAMP_SIZE          0
#define S_STATE_BUF_SIZE        /*32*/64

//data len position
#define DATA_LEN_POS            (S_STATE_BUF_SIZE-1)

//S MODE STATE AND DATA BUFFER ADDRESS (CS3)

#define FPGA_STATE1_ADDR        (CS3_BASE + 0xFC0*2)
#define FPGA_STATE2_ADDR        (CS3_BASE + 0x1FC0*2)
#define FPGA_DATA1_ADDR         (CS3_BASE)
#define FPGA_DATA2_ADDR         (CS3_BASE + 0x1000*2)
#define FPGA_AQUIRED_ADDR1      (CS3_BASE + (DATA_LEN_POS-2)*2)
#define FPGA_AQUIRED_ADDR2      (CS3_BASE + (DATA_LEN_POS-1)*2)
#define FPGA_DATA1_LEN_ADDR     (FPGA_STATE1_ADDR + DATA_LEN_POS*2)
#define FPGA_DATA2_LEN_ADDR     (FPGA_STATE2_ADDR + DATA_LEN_POS*2)

#define FPGA_S_DATA1_CLEAR      (CS3_BASE + 0xE10*2)
#define FPGA_S_DATA2_CLEAR      (CS3_BASE + 0x1E10*2)

/************************/
/*      C MODE          */
/***********************/
#define C_SAMPLE_FREQ      20  /*20M*/

#define FPGA_C_STATE1_ADDR      (CS3_BASE + 0x2FC0*2)
#define FPGA_C_STATE2_ADDR      (CS3_BASE + 0x3FC0*2)
#define FPGA_C_DATA1_ADDR       (CS3_BASE + 0x2000*2)
#define FPGA_C_DATA2_ADDR       (CS3_BASE + 0x3000*2)

#define FPGA_C_DATA1_CLEAR      (CS3_BASE + 0x2E10*2)
#define FPGA_C_DATA2_CLEAR      (CS3_BASE + 0x3E10*2)

#define FPGA_C_DATA1_LEN_ADDR   FPGA_C_STATE1_ADDR
#define FPGA_C_DATA2_LEN_ADDR   FPGA_C_STATE2_ADDR

#define C_DATA_LEN_POS          0  //unit of length is short

#define C_STATE_BUF_SIZE        1 /*short*/

#define ONE_C_FRAME_SIZE         18 /*short*/
#define MAX_CNT_IN_ONE_CSTEP     60 //todo

 
/*************************/
/*   INPUT GENERAL       */
/*************************/
#define DATA_THRESHOLD    0x10 //todo

#define FPGA_POOL_NUM     2
#define BUFFER_SIZE       (MAX_FRAME_NUM*(LONG_FRAME_LEN+PRE_FRAME_LEN)*S_SAMPLE_FREQ) //3600
#define RANGE_FEQ         20

/*DIFF threshold (used in C decode)*/
#define DLT_DIFF_THRESHOLD (3*VALUE_DB1)
#define SUM_DIFF_THRESHOLD (3*VALUE_DB1)
#define OBA_THRESHOLD      (1*VALUE_DB1)


/************************/
/*   OTHER              */
/************************/

//GPIO INTERRUPT

//event define 
/*GPIO0-7: 64-71*/
#define GPIO0_EVENT 64u
#define GPIO1_EVENT 65u
#define GPIO2_EVENT 66u
#define GPIO3_EVENT 67u
#define GPIO4_EVENT 68u
#define GPIO5_EVENT 69u
#define GPIO6_EVENT 70u
#define GPIO7_EVENT 71u

#define EDMA3_CC_XFER_COMPLETION_INT                    36u   
#define EDMA3_CC_ERROR_INT                              37u   


typedef struct event_desc {
	int vec_num;
	int event;
//	char desc[100];
} event_desc_t;


//GPIO DESC
#define GPIO_INPUT   1
#define GPIO_OUTPUT  0

#define INT_ENABLE   1
#define INT_DISABLE  0

#define LEVEL_INT         0
#define RISE_PULSE_INT    1
#define DROP_PULSE_INT    2

typedef struct gpio_desc {
  char gpioNum;
  char type; //input or output
  char int_enable; //int enable or disable
  char int_stype; //if int enable, set int trigger mode: level, raise pulse or drop pulse
//  char desc[100];
} gpio_desc_t;



/*************************************/
/*    REPORT(include C and S REPORT  */
/*************************************/
#define REPORT_POOL_NUM   2

//MODE REPORT BUFFER ADDRESS (CS4)
#define FPGA_REPORT_ADDR1       (CS4_BASE)
#define FPGA_REPORT_ADDR2       (CS4_BASE + 0x400*2)

//TO-CPU-DMA S REPORT BUFFER STATE(CS4)
#define FPGA_REPORT_STATE       (CS4_BASE + 0x800*2) //BIT0: STATE0, BIT1: STATE1, 0-ready, 1-busy

typedef struct _report {
  short head_delimit;         //0x55aa
  short data[11];
  short tail_delimit;         //0xaa55 
} REPORT_T;

#define REPORT_LEN       sizeof(REPORT_T)

//S report: only for little-endia
typedef struct s_report {
  short head_delimit;         //0x55aa
  short range;                // range
  short param0;               //mode[15:13],bs[12:10],t_b[9],omni[8],len[7](0:56,1:112),RSV[6:0]
  short param1;               //bs_sum[15:13], bs_dlt[12:10], pat[9:5], at[4:0]
  short param2;               //ref_sum[15:8], dlt_sum[7:0]
  unsigned char frame[14];    //frame data
  short tail_delimit;         //0xaa55 
  unsigned char conf[14];    //confidence: this section not report to CPU, only for inner use
} S_REPORT_T;

//C report
typedef struct c_report {
  short head_delimit; //0x55aa
  short range;        // range
  short param0;       //mode[15:13],bs[12:10],t_b[9],omni[8],spi[7],c_step[6:0]
  short param1;       //bs_sum[15:13], bs_dlt[12:10], pat[9:5], at[4:0]
  short param2;       //ref_sum[15:8], dlt_sum[7:0]
  short code;         //C CODE
  short confidence;   //CONFIDENCE
  short resv[5];      //reserved
  short tail_delimit; //0xaa55  
} C_REPORT_T;


//for inner use
#define S_MODE  0
#define C_MODE  1

#endif //__FPGA_INTF_H_
