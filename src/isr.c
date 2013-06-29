#include "tcas.h"

extern volatile unsigned char sdata_wptr[MAX_POOL_NUM]; //write pointer for sample_data
extern short sdataSt[MAX_POOL_NUM][MAX_BUF_IN_S_POOL][S_STATE_BUF_SIZE];

extern volatile unsigned char cdata_wptr[MAX_POOL_NUM]; //write pointer for sample_data
extern short cdataSt[MAX_POOL_NUM][MAX_BUF_IN_C_POOL][C_STATE_BUF_SIZE];

volatile char g_s_sample_full[MAX_POOL_NUM];
volatile char g_c_sample_full[MAX_POOL_NUM];

#define _DEBUG_INT_
//#undef  _DEBUG_INT_

#ifdef _DEBUG_INT_
//fpga interrupt count
volatile unsigned long gCIntCnt1=0;
volatile unsigned long gCIntCnt2=0;
volatile unsigned long gSIntCnt1=0;
volatile unsigned long gSIntCnt2=0;

//data length error count
volatile unsigned long gCLenErrCnt1=0;
volatile unsigned long gCLenErrCnt2=0;
volatile unsigned long gSLenErrCnt1=0;
volatile unsigned long gSLenErrCnt2=0;

//dma error count
volatile unsigned long gCDmaErrCnt1=0;
volatile unsigned long gCDmaErrCnt2=0;
volatile unsigned long gSDmaErrCnt1=0;
volatile unsigned long gSDmaErrCnt2=0;

//dma interrupt count
volatile unsigned long gCDMAIntCnt1=0;
volatile unsigned long gCDMAIntCnt2=0;
volatile unsigned long gSDMAIntCnt1=0;
volatile unsigned long gSDMAIntCnt2=0;

//int lost because buffer full
volatile unsigned long gSIntFull1=0;
#endif


//extern char gFrameOut[MAX_REPORT_FRAME][FRAME_OUT_LEN];
//extern volatile unsigned long report_wptr[2];

/********************/
/* S MODE INTERRUPT */
/********************/

//this is S buffer1 ready interrupt process
interrupt void buffer1_ready_isr()
{
  volatile unsigned short datalen;
  unsigned char wptr;
  int rc;
  short *state_ptr;
  
/*
  #ifdef __TIME_TEST
    k = 1-k;
    EVM6424_GPIO_setOutput(100, k);
  #endif
*/

  //clear int flag
  EVM6424_GPIO_clear_INT(0);
#ifdef _DEBUG_INT_
  gSIntCnt1++;
#endif

  //check whether receive buffer is full(todo: when wptr have hole in it, it will product an un-order data)
  wptr = sdata_wptr[0];
  state_ptr = &sdataSt[0][wptr][0];
  if(state_ptr[DATA_LEN_POS]) //sample data buffer full
  {
    g_s_sample_full[0] = 1;
  #ifdef _DEBUG_INT_
	gSIntFull1++;
  #endif
    return;
  }
	
  //data length is number of words
  datalen = *(volatile unsigned short*)FPGA_DATA1_LEN_ADDR;
  if(datalen == 0xffffu)
  {
    datalen = 2;
  }
  else if((datalen < 640) || (datalen >3600))
  {
    myprintf("state1_complet_isr: datalen error (%u)\n",datalen);
#ifdef _DEBUG_INT_
    gSLenErrCnt1++;
  #endif
    goto clear_buffer1;
  }
  
  //get sum data
  rc = retrieve_sample_data(S_MODE, 0, datalen); // poolid, datalen
  if(rc)
    goto clear_buffer1;
    
  //check event miss
  rc = check_retrieve_event_miss(S_MODE,0);
  if(rc)
    goto clear_buffer1;
  
  //goto next write buffer
  wptr++;
  if(wptr >= MAX_BUF_IN_S_POOL)
    sdata_wptr[0] = 0;
  else
    sdata_wptr[0] = wptr;
    
  return;
  //todo :maybe check event-miss in EMCR/EMCRH, use this to indicate data-miss

clear_buffer1:
  #ifdef _DEGUG_INT_
    gSDmaErrCnt1++;
  #endif

	release_fpga_buffer(S_MODE, 0);

#if 0
	//only for debug
	EVM6424_GPIO_setOutput(2, 1);
	for(i=0;i<1000;i++);
	EVM6424_GPIO_setOutput(2, 0);
	for(i=0;i<1000;i++);
#endif

}

//this is buffer2 ready interrupt process
interrupt void buffer2_ready_isr()
{
  volatile unsigned short datalen;
  unsigned char wptr;
  int rc;
  short *state_ptr;
  
	//clear int flag
  EVM6424_GPIO_clear_INT(1);
#ifdef _DEBUG_INT_
  gSIntCnt2++;
#endif

  //check receive buffer full
  wptr = sdata_wptr[1];
  state_ptr = &sdataSt[1][wptr][0];
  if(state_ptr[DATA_LEN_POS]) //sample data buffer full
  {	
    g_s_sample_full[1] = 1;
    return;
  }
  
  //data length is number of words
	datalen = *(volatile unsigned short*)FPGA_DATA2_LEN_ADDR;

  if(datalen == 0xffffu)
    datalen = 2;
  else if((datalen < 640) || (datalen >3600))
  {
  #ifdef _DEBUG_INT_
    gSLenErrCnt2++;
  #endif
    myprintf("state2_complet_isr: datalen error (%u)\n",datalen);
    goto clear_buffer2;
  }
  
  //get sum data  & dlt data & state data (chains)
  rc = retrieve_sample_data(S_MODE, 1, datalen); // poolid, datalen
  if(rc)
    goto clear_buffer2;
       
      
  //check event-miss in EMCR/EMCRH, use this to indicate data-miss
  rc = check_retrieve_event_miss(S_MODE, 1);
  if(rc)
    goto clear_buffer2;
  
  //goto next write buffer  
  wptr++;
  if(wptr >= MAX_BUF_IN_S_POOL)
    sdata_wptr[1] = 0;
  else
    sdata_wptr[1] = wptr;
   	
	return;

clear_buffer2:
  #ifdef _DEGUG_INT_
    gSDmaErrCnt2++;
  #endif

  release_fpga_buffer(S_MODE, 1);
}


//this is sample data 1 transfer complete interrupt process
void sample_data1_complete_isr()
{
#ifdef _DEBUG_INT_
  gSDMAIntCnt1++;
#endif

  release_fpga_buffer(S_MODE, 0);
}


//this is sample data 2 transfer complete interrupt process
void sample_data2_complete_isr()
{
#ifdef _DEBUG_INT_
  gSDMAIntCnt2++;
#endif

  release_fpga_buffer(S_MODE, 1);
}

/******************************/
/*     C MODE INTERRUPT       */
/******************************/

//this is C buffer1 ready interrupt process
interrupt void c_buffer1_ready_isr()
{
  volatile unsigned short datalen;
  unsigned char wptr;
  int rc;
  short *state_ptr;
  int i;

  //clear int flag
  EVM6424_GPIO_clear_INT(2);
#ifdef _DEBUG_INT_
  gCIntCnt1++;      
#endif

  //check whether receive buffer is full
  wptr = cdata_wptr[0];
  state_ptr = &cdataSt[0][wptr][0];
  if(state_ptr[C_DATA_LEN_POS]) //sample data buffer full
  {
    g_c_sample_full[0] = 1;
    return;
  }
	
  //data length is number of words
  for(i=0;i<2;i++)
  datalen = *(volatile unsigned short*)FPGA_C_DATA1_LEN_ADDR;
  
  if((datalen < ONE_C_FRAME_SIZE) || (datalen >3600))
  {
   	myprintf("state2_complet_isr: datalen error (%u)\n",datalen);
#ifdef _DEBUG_INT_
	gCLenErrCnt1++;
#endif
    goto clear_buffer3;
  }
  
  //get data
  rc = retrieve_sample_data(C_MODE, 0, datalen); // poolid, datalen
  if(rc)
    goto clear_buffer3;
    
  //check event miss
  rc = check_retrieve_event_miss(C_MODE, 0);
  if(rc)
    goto clear_buffer3;
  
  //goto next write buffer
  wptr++;
  if(wptr >= MAX_BUF_IN_C_POOL)
    cdata_wptr[0] = 0;
  else
    cdata_wptr[0] = wptr;
      
  return;
  //todo :maybe check event-miss in EMCR/EMCRH, use this to indicate data-miss

clear_buffer3:
  #ifdef _DEGUG_INT_
    gCDmaErrCnt1++;
  #endif

  release_fpga_buffer(C_MODE, 0);
}


//this is C buffer2 ready interrupt process
interrupt void c_buffer2_ready_isr()
{
  volatile unsigned short datalen;
  unsigned char wptr;
  int rc;
  short *state_ptr;
  int i;
    
  //clear int flag
  EVM6424_GPIO_clear_INT(3);
#ifdef _DEBUG_INT_
  gCIntCnt2++;    
#endif
   
  //check whether receive buffer is full
  wptr = cdata_wptr[0];
  state_ptr = &cdataSt[0][wptr][0];
  if(state_ptr[C_DATA_LEN_POS]) //sample data buffer full
  {
    g_c_sample_full[1] = 1;
    return;
  }
	
  //data length is number of words
  for(i=0;i<2;i++)
  datalen = *(volatile unsigned short*)FPGA_C_DATA2_LEN_ADDR;
  
  if((datalen < ONE_C_FRAME_SIZE) || (datalen >3600))
  {
   	myprintf("state2_complet_isr: datalen error (%u)\n",datalen);
#ifdef _DEBUG_INT_
	gCLenErrCnt2++;
#endif
    goto clear_buffer4;
  }
    
  //get data
  rc = retrieve_sample_data(C_MODE, 1, datalen); // poolid, datalen
  if(rc)
    goto clear_buffer4;
    
  //check event miss
  rc = check_retrieve_event_miss(C_MODE, 1);
  if(rc)
    goto clear_buffer4;
  
  //goto next write buffer
  wptr++;
  if(wptr >= MAX_BUF_IN_C_POOL)
    cdata_wptr[1] = 0;
  else
    cdata_wptr[1] = wptr;
    
  return;
  //todo :maybe check event-miss in EMCR/EMCRH, use this to indicate data-miss

clear_buffer4:
  #ifdef _DEGUG_INT_
    gCDmaErrCnt2++;
  #endif
	release_fpga_buffer(C_MODE, 1);
}

//this is C sample data 1 transfer complete interrupt process
void c_sample_data1_complete_isr()
{
#ifdef _DEBUG_INT_
  gCDMAIntCnt1++;
#endif

  release_fpga_buffer(C_MODE, 0);
}

//this is C sample data 2 transfer complete interrupt process
void c_sample_data2_complete_isr()
{
#ifdef _DEBUG_INT_
  gCDMAIntCnt2++;
#endif

  release_fpga_buffer(C_MODE, 1);
}

//reserved
void report_complet1_isr()
{
  //gFrameOut[report_wptr[0]][HEAD_LENGTH + END_WORD_OFF] = 0;
  //gReportInSend[0] = 0;
}

void report_complet2_isr()
{
  //gFrameOut[report_wptr[1]][HEAD_LENGTH + END_WORD_OFF] = 0;
  //gReportInSend[1] = 0;
}
