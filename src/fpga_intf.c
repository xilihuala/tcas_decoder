#include "fpga_intf.h"

REPORT_T gFrameReport[FPGA_POOL_NUM];
REPORT_T gFrameEnd;

event_desc_t eventList[]={
  {4, GPIO0_EVENT},                   //S buffer1 ready interrupt
  {5, GPIO1_EVENT},                   //S buffer2 ready interrupt
  {6, EDMA3_CC_XFER_COMPLETION_INT},  //fpga dma complete interrupt
  {7, EDMA3_CC_ERROR_INT},            //fpga dma error interrupt
  {8, GPIO2_EVENT},                   //C buffer1 ready interrupt
  {9, GPIO3_EVENT}                    //C buffer2 ready interrupt
};
unsigned int EVENT_NUM = sizeof(eventList)/sizeof(event_desc_t);

gpio_desc_t  gpioList[]={
  { 0, GPIO_INPUT, INT_ENABLE , DROP_PULSE_INT  }, //S buffer1 ready int
  { 1, GPIO_INPUT, INT_ENABLE , DROP_PULSE_INT  }, //S buffer2 ready int
  { 2, GPIO_INPUT, INT_ENABLE , DROP_PULSE_INT  }, //C buffer1 ready int
  { 3, GPIO_INPUT, INT_ENABLE , DROP_PULSE_INT  }  //C buffer2 ready int
//{ 4, GPIO_INPUT, INT_DISABLE, 0               }, //report buffer1 ready int}
//{ 5, GPIO_INPUT, INT_DISABLE, 0               }  //report buffer2 ready int}
};
unsigned int GPIO_NUM = sizeof(gpioList)/sizeof(gpio_desc_t);

//init END frame which report to CPU at end of timeout
void report_init()
{
  gFrameEnd.head_delimit  = 0x55aa;
  gFrameEnd.data[0]       = 0xffff;
  gFrameEnd.data[1]       = 0x1fff;
  memset(&gFrameEnd.data[2],0xaaaa, 9*sizeof(short));
  gFrameEnd.tail_delimit  = 0xaa55;
}


/***********************************     */
/* send S/C report to FPGA               */
/* THEN, FPGA use DMA send report to CPU */
/***********************************     */
void report_to_CPU(unsigned char *report)
{
  static int report_pool_id = 0;
  
  //debug
  return;


  //wait extern DMA finish
  while(1)
  {
    char state;
  
  	state = (*(volatile unsigned char*)FPGA_REPORT_STATE)&0x1;
  	if((state & (1<< report_pool_id)) == 0)
  	  break;
  }
  
  //copy to dest report buffer
  memcpy(&gFrameReport[report_pool_id], report, sizeof(REPORT_T));
  
  //send report to FPGA use DMA CHANNEL
  SendReportToFPGA(report_pool_id);

  report_pool_id = (report_pool_id+1)%2;
}

/*send END frame to CPU*/
void send_END_to_CPU(char mode)
{
  gFrameEnd.data[1] |= (mode & 0x7)<<13;
  report_to_CPU((unsigned char*)&gFrameEnd);
}

void release_fpga_buffer(int mode, int pool_id)
{
  unsigned short temp; 
  if(mode == S_MODE)
  {  
    if(pool_id == 0)
      temp = *(volatile unsigned short*)(FPGA_S_DATA1_CLEAR);
    else
      temp = *(volatile unsigned short*)(FPGA_S_DATA2_CLEAR);
  }
  else if(mode == C_MODE)
  {
    if(pool_id == 0)
      temp = *(volatile unsigned short*)(FPGA_C_DATA1_CLEAR);
    else
      temp = *(volatile unsigned short*)(FPGA_C_DATA2_CLEAR);
  }
}

