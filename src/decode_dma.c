#include "bios_edma3_drv_sample.h"
#include "tcas.h"

extern EDMA3_DRV_Handle hEdma;

extern short s_sample_data[MAX_POOL_NUM][MAX_BUF_IN_S_POOL][MAX_RECV_SIZE];
extern short c_sample_data[MAX_POOL_NUM][MAX_BUF_IN_C_POOL][MAX_RECV_SIZE];
extern short sdataSt[MAX_POOL_NUM][MAX_BUF_IN_S_POOL][S_STATE_BUF_SIZE];
extern short cdataSt[MAX_POOL_NUM][MAX_BUF_IN_C_POOL][C_STATE_BUF_SIZE];
//extern short state_pool[MAX_POOL_NUM][MAX_BUF_IN_POOL][STATE_BUF_SIZE];
//extern char gDataOut[FRAME_OUT_LEN]; 
//extern char gFrameOut[MAX_REPORT_FRAME][FRAME_OUT_LEN];
extern REPORT_T gFrameReport[REPORT_POOL_NUM];

/* state transfer Callback function */
static void data_callback (unsigned int tcc, EDMA3_RM_TccStatus status,
                       void *appData)
{
  (void)tcc;
  (void)appData;

  switch (status)
  {
  case EDMA3_RM_XFER_COMPLETE:
    /* Transfer completed successfully */
    if(tcc == EVENT_DATA_STATE1)
      sample_data1_complete_isr();
    else if(tcc == EVENT_DATA_STATE2)
      sample_data2_complete_isr();
    break;
  case EDMA3_RM_E_CC_DMA_EVT_MISS:
    /* Transfer resulted in DMA event miss error. */
    break;
  case EDMA3_RM_E_CC_QDMA_EVT_MISS:
    /* Transfer resulted in QDMA event miss error. */
    break;
  default:
    break;
  }
}

#if 0
static void report_callback(unsigned int tcc, EDMA3_RM_TccStatus status,
                       void *appData)
{
  (void)tcc;
  (void)appData;

  switch (status)
  {
  case EDMA3_RM_XFER_COMPLETE:
    /* Transfer completed successfully */
    if(tcc == EVENT_DATA_STATE1)
      report_complet1_isr();// ();
    else if(tcc == EVENT_DATA_STATE2)
      report_complet2_isr();//();
    break;
  case EDMA3_RM_E_CC_DMA_EVT_MISS:
    /* Transfer resulted in DMA event miss error. */
    break;
  case EDMA3_RM_E_CC_QDMA_EVT_MISS:
    /* Transfer resulted in QDMA event miss error. */
    break;
  default:
    break;
  }
}
#endif

/*paramSet have 127, event have 63(some resevered), link can use 64-127*/
int create_channel(unsigned int acnt,
                   unsigned int bcnt,
                   unsigned int ccnt,
                   unsigned char *srcBuff,
                   unsigned char *destBuff,
                   unsigned int event, //event number or EDMA3_DRV_TCC_ANY
                   unsigned char type,  //MAIN_CHANNEL, LINK_CHANNEL, CHAIN_CHANNEL
                   unsigned char event_queue,//event queue number
                   unsigned char sam, //1: fifo, 0: increment
                   unsigned char dam,//1: fifo,  0: increment
                   unsigned int  *chnlId, //return chnlId
                   unsigned char intEnable, //1: enable interrupt. 0:disable interrupt
                   unsigned int srcbidx,
                   unsigned int destbidx
                   )
{
  EDMA3_DRV_Result result = EDMA3_DRV_SOK;
  EDMA3_DRV_PaRAMRegs paramSet = {0,0,0,0,0,0,0,0,0,0,0,0};
  unsigned int chId = 0;
  unsigned int tcc = 0;
  unsigned int BRCnt = 0;
  EDMA3_RM_TccCallback callback;

  /* Set B count reload as B count. */
  BRCnt = bcnt;

  /* Setup for Channel*/
  tcc = event;
  if(type == MAIN_CHANNEL)
  {  
    chId = tcc;
  }
  else if(type == LINK_CHANNEL)
  {
    chId = EDMA3_DRV_LINK_CHANNEL; //link channe is from 64-127
  }
  else if(type == CHAIN_CHANNEL)  
  {  
    chId = EDMA3_DRV_DMA_CHANNEL_ANY; //chain channel 
  }
  else
  {  
    printf("create_channel: channel type invalid\n");
    return -1;
  }

  if((event == EVENT_DATA_STATE1) || (event == EVENT_DATA_STATE2))
    callback = data_callback;    
  else if((event == EVENT_REPORT1) ||(event == EVENT_REPORT2))
    callback = NULL/*report_callback*/;  
  else 
  {  
    //printf("create_channel: event invalid\n");
    //return -1;
    callback = NULL;
  }

  /* Request any DMA channel and any TCC */
  result = EDMA3_DRV_requestChannel (hEdma, &chId, &tcc,
    (EDMA3_RM_EventQueue)event_queue,
    callback, NULL);

  if (result != EDMA3_DRV_SOK)
  {  
    printf("create_channel: request channel failed, chId = %u\n", chId);
    return -1;
  }

  /* Fill the PaRAM Set with transfer specific information */
  paramSet.srcAddr    = (unsigned int)(srcBuff);
  paramSet.destAddr   = (unsigned int)(destBuff);

  /**
  * Be Careful !!!
  * Valid values for SRCBIDX/DSTBIDX are between ?2768 and 32767
  * Valid values for SRCCIDX/DSTCIDX are between ?2768 and 32767
  */
  paramSet.srcBIdx    = srcbidx;
  paramSet.destBIdx   = destbidx;
  paramSet.srcCIdx    = 1;
  paramSet.destCIdx   = 1;

  /**
  * Be Careful !!!
  * Valid values for ACNT/BCNT/CCNT are between 0 and 65535.
  * ACNT/BCNT/CCNT must be greater than or equal to 1.
  * Maximum number of bytes in an array (ACNT) is 65535 bytes
  * Maximum number of arrays in a frame (BCNT) is 65535
  * Maximum number of frames in a block (CCNT) is 65535
  */
  paramSet.aCnt       = acnt;
  paramSet.bCnt       = bcnt;
  paramSet.cCnt       = ccnt;

  /* For AB-synchronized transfers, BCNTRLD is not used. */
  paramSet.bCntReload = BRCnt;

  //set link to NULL
  paramSet.linkAddr   = 0xFFFFu;

  /* Src & Dest modes */
  paramSet.opt &= 0xFFFFFFFCu;
  if(sam == FIFO_MODE)
    paramSet.opt |= 0x1;
  if(dam == FIFO_MODE)
    paramSet.opt |= ~0x2;

    //set tcc mode to normal
    paramSet.opt &= ~(1<<11);

  /* Program the TCC */
  paramSet.opt |= ((tcc << OPT_TCC_SHIFT) & OPT_TCC_MASK);

  if(intEnable)
  {  
    /* Enable Intermediate & Final transfer completion interrupt */
    paramSet.opt |= (1 << OPT_ITCINTEN_SHIFT);
    paramSet.opt |= (1 << OPT_TCINTEN_SHIFT);
  }
  else
  {  
    /* Disable Intermediate & Final transfer completion interrupt */
    paramSet.opt &= ~(1 << OPT_ITCINTEN_SHIFT);
    paramSet.opt &= ~(1 << OPT_TCINTEN_SHIFT);
  }    

  //use AB-SYNC mode
  //paramSet.opt &= ~0xFFFFFFFBu;
  paramSet.opt |= 0x4u;
  /* Now, write the PaRAM Set. */
  result = EDMA3_DRV_setPaRAM (hEdma, chId, &paramSet);
  if (result != EDMA3_DRV_SOK)
  {  
    printf("create_channel: set PaRAM failed\n");
    return -1;
  }
  *chnlId = chId;
  return 0;
}

//chain ch2 to ch1
int chain_channel(unsigned int ch1Id , unsigned int ch2Id)
{
  EDMA3_DRV_ChainOptions chain = {EDMA3_DRV_TCCHEN_DIS,
        EDMA3_DRV_ITCCHEN_DIS,
        EDMA3_DRV_TCINTEN_DIS,
        EDMA3_DRV_ITCINTEN_DIS};
  EDMA3_DRV_Result result = EDMA3_DRV_SOK;

  /* Transfer complete chaining enable. */
  chain.tcchEn = EDMA3_DRV_TCCHEN_EN;
  /* Intermediate transfer complete chaining enable. */
  chain.itcchEn = EDMA3_DRV_ITCCHEN_EN;
  /* Transfer complete interrupt is enabled. */
  chain.tcintEn = EDMA3_DRV_TCINTEN_EN;
  /* Intermediate transfer complete interrupt is disabled. */
  chain.itcintEn = EDMA3_DRV_ITCINTEN_DIS;

  result = EDMA3_DRV_chainChannel(hEdma, ch1Id, ch2Id,
    (EDMA3_DRV_ChainOptions *)&chain);
  if (result != EDMA3_DRV_SOK)
  {  
    printf("chain_channel failed: %u to %u\n", ch2Id, ch1Id);
    return -1;
  }

  return 0;

}

int link_channel(unsigned int ch1Id , unsigned int ch2Id)
{
  EDMA3_DRV_Result result = EDMA3_DRV_SOK;

  result = EDMA3_DRV_linkChannel (hEdma, ch1Id, ch2Id);
  if (result != EDMA3_DRV_SOK)
  {  
    printf("link_channel failed: %u to %u\n", ch2Id, ch1Id);
    return -1;
  }

  return 0;
}

/*
mode is enum: 
  EDMA3_DRV_TRIG_MODE_MANUAL,
  EDMA3_DRV_TRIG_MODE_QDMA,
  EDMA3_DRV_TRIG_MODE_EVENT,
  EDMA3_DRV_TRIG_MODE_NONE
*/  
int enable_transfer(unsigned chId, EDMA3_DRV_TrigMode mode)
{
  EDMA3_DRV_Result result = EDMA3_DRV_SOK;

  result = EDMA3_DRV_enableTransfer (hEdma, chId, mode);
  if (result != EDMA3_DRV_SOK)
  {  
    printf("enable_transfer failed: %u\n", chId);
    return -1;
  }
  return 0;
}


//trigger data receive dma
//mode:  0: S_MODE , 1: A/C
int retrieve_sample_data(int mode, unsigned char pool_id, unsigned short datalen)
{
  //use chain mode, we construct chain at the init time , and paramSet is also set at init.
  //now we only change the length, and trigger it

  EDMA3_DRV_Result edmaResult;
  unsigned int chId;
  int rc;
  unsigned short dlen;

 
  if(mode == S_MODE)
  {
  	if(pool_id == 0)
      chId = EVENT_DATA1;
    else
      chId = EVENT_DATA2;
  }
  else if(mode == C_MODE)
  {
    if(pool_id == 0)
  	  chId = EVENT_C_DATA1;
  	else
  	  chId = EVENT_C_DATA2;
  }
  else //bug
    return -1;
  
  dlen = datalen*2;
    
  //set data len (not use BRCNT,so set it to 1)
  edmaResult = EDMA3_DRV_setTransferParams ( hEdma, chId, dlen, 1, 1, 1,EDMA3_DRV_SYNC_A);
  if(edmaResult != EDMA3_DRV_SOK)
  {
    printf("retrieve_sample_data: set data length failed\n");
    return -1;
  }

  //trigger report transfer
  rc = enable_transfer(chId, EDMA3_DRV_TRIG_MODE_MANUAL);
  if(rc)
  {
    printf("retrieve_sample_data: enable transfer failed\n");
    return -1;
  }
  
  return 0;
}

//trigger report send dma
int SendReportToFPGA(char pool_id)
{
 // EDMA3_DRV_Result edmaResult;
  unsigned int chId;
  int rc;

  if(pool_id == 0)
    chId = EVENT_REPORT1;
  else
    chId = EVENT_REPORT2;

#if 0   //only use default buffer in channel create
  //set src address
  edmaResult = EDMA3_DRV_setSrcParams ( hEdma, chId, (unsigned int)(srcBuff),
    EDMA3_DRV_ADDR_MODE_INCR,
    EDMA3_DRV_W8BIT);
  if(edmaResult != EDMA3_DRV_SOK)
  {
    printf("SendReportToFPGA: set src address failed\n");
    return -1;
  }
#endif

  //trigger report transfer
  rc = enable_transfer(chId, EDMA3_DRV_TRIG_MODE_MANUAL);
  if(rc)
  {
    printf("SendReportToFPGA: enable transfer failed\n");
    return -1;
  }

  return 0;
}

//check for sample data retrieve event(include data and state) miss
//0: no miss , 1: miss
//note : need clear relative bit
//mode: 0: S , 1: A/C
int check_retrieve_event_miss(int mode, char pool_id)
{

  unsigned char event[5];
  unsigned int eventNum;
  char tmp, miss;
  int i;
  
  eventNum = 0;
  
  if(pool_id == 0) 
  {  
    if(mode == S_MODE)
    {  
      event[0] = EVENT_DATA_STATE1;
      event[1] = EVENT_DATA1;
      eventNum = 2;
    }
    else if(mode == C_MODE)
    {
      event[0] = EVENT_C_DATA_STATE1;
      event[1] = EVENT_C_DATA1;
      eventNum = 2;
    }  
  }
  else
  {  
    if(mode == S_MODE)
    {  
      event[0] = EVENT_DATA_STATE2;
      event[1] = EVENT_DATA2;
      eventNum = 2;
    }
    else if(mode == C_MODE)
    {
      event[0] = EVENT_C_DATA_STATE2;
      event[1] = EVENT_C_DATA2;
      eventNum = 2;
    }
  }
  
  miss = 0;
  for(i=0;i<eventNum;i++)
  {
    if(event[i] < 32)
    {  
      tmp = ((_EMR>>event[i]) & 0x1);
      if(tmp)
        _EMCR = 1<<event[i];
    } 
    else
    {
      tmp = ((_EMRH>>(event[i]&0x1f)) & 0x1);  
      if(tmp)
        _EMCRH = 1<<(event[i]&0x1f);
    }
    miss |= tmp;
  }
  return miss;
}

int check_report_event_miss(char pool_id)
{
  unsigned char event;
  int miss = 0;
  
  if(pool_id == 0)
    event = EVENT_REPORT1;
  else
    event = EVENT_REPORT2;  
    
  if(event < 32)
  {  
    miss = (_EMR>>event) & 0x1;
    _EMCR = 1<<event;
  } 
  else
  {
    miss = (_EMRH>>(event&0x1f)) & 0x1;  
    _EMCRH = 1<<(event&0x1f);
  }
  return miss;
}

//wait channel DMA complete
void wait_complete(unsigned int tcc)
{
  EDMA3_DRV_Result result;

  result = EDMA3_DRV_waitAndClearTcc (hEdma, tcc);
  if (result != EDMA3_DRV_SOK)
    printf("wait complete failed: tcc=%u\n", tcc);
//  wait_debug(1000); //todo
}

unsigned short clear_dma_int(unsigned int tcc)
{
  EDMA3_DRV_Result result;
  unsigned short status;
  
  result = EDMA3_DRV_checkAndClearTcc (hEdma, tcc, &status);
  if (result != EDMA3_DRV_SOK)
    printf("wait complete failed: tcc=%u\n", tcc);
  return status;  
}

void clear_all_dma_int()
{
  clear_dma_int(EVENT_DATA1);
  clear_dma_int(EVENT_DATA2);
  clear_dma_int(EVENT_DATA_STATE1);
  clear_dma_int(EVENT_DATA_STATE2);
  
  clear_dma_int(EVENT_C_DATA1);
  clear_dma_int(EVENT_C_DATA2);
  
  
//  clear_dma_int(EVENT_FRAMEOUT);
  clear_dma_int(EVENT_REPORT1);
  clear_dma_int(EVENT_REPORT2);
}

/*******************************/
/*         S MODE CHANNEL      */
/*******************************/

int setup_s_data1_channel()
{
  unsigned int data1_mainChnl1;
  unsigned int data1_mainChnl2;
  unsigned int mainChnl1;
  unsigned int mainChnl2;
  unsigned int linkChnl1_0;
  unsigned int linkChnl1_1;
  unsigned int linkChnl2_0;
  unsigned int linkChnl2_1;
  int rc;
  int i;
  
	/**********************/
	/* CREATE MAIN CHANNEL*/
	/**********************/
  //create data1 main channel(NOTE: init length is not use, we change it in trigger time)
  rc = create_channel(1, 1, 1, /*size*/\
    (unsigned char*)FPGA_DATA1_ADDR, /*src address*/\
    (unsigned char*)&s_sample_data[0][0][0], /*dest address*/ \
    EVENT_DATA1, /*event*/ \
    MAIN_CHANNEL, /*type*/\
    0, /*EVENT QUEUE*/ \
    INCR_MODE, /*SRC*/ \
    INCR_MODE, /*DEST*/\
    &data1_mainChnl1, \
    INT_DISABLE, \
    1,  /*srcbidx*/ \
    1   /*destbidx*/ \
    );
  if(rc)
  {
    printf("create main channel for data1 failed\n");
    return -1;
  }
  
  //create state1 main channel
  rc = create_channel(S_STATE_BUF_SIZE*sizeof(short), 1, 1, /*size*/\
    (unsigned char*)FPGA_STATE1_ADDR, /*src address*/\
    (unsigned char*)&sdataSt[0][0][0], /*dest address*/ \
    EVENT_DATA_STATE1, /*event*/ \
    MAIN_CHANNEL, /*type*/\
    0, /*EVENT QUEUE*/ \
    INCR_MODE, /*SRC*/ \
    INCR_MODE, /*DEST*/\
    &data1_mainChnl2, \
    INT_ENABLE, \
		1,\
		1 \
    );                   
  if(rc)
  {
    printf("create main channel for data_state1 failed\n");
    return -1;
  }
  chain_channel(data1_mainChnl1, data1_mainChnl2);

  /****************************/
  /*  CREATE LINK CHANNEL     */
  /****************************/
  /******************************************************************************************/
  /*create channel for data1 receive(ring link and chain mode,event-queue0 and event-queue1)*/
  /*****************************************************************************************/
  //create array of chain channel and link channel
  linkChnl1_0 = data1_mainChnl1;
  linkChnl2_0 = data1_mainChnl2;
  for(i=0; i<MAX_BUF_IN_S_POOL; i++)
  {
    rc = create_channel(1, 1, 1, /*size*/\
      (unsigned char*)FPGA_DATA1_ADDR, /*src address*/\
      (unsigned char*)&s_sample_data[0][i][0], /*dest address*/ \
      EVENT_DATA1, /*event*/ \
      LINK_CHANNEL, /*type*/\
      0, /*EVENT QUEUE*/ \
      INCR_MODE, /*SRC*/ \
      INCR_MODE, /*DEST*/\
      &linkChnl1_1, \
      INT_DISABLE, \
      1,\
      1 \
      );
    if(rc)
    {
      printf("create link channel for data1 failed\n");
      return -1;
    }
    
    rc = create_channel(S_STATE_BUF_SIZE*sizeof(short), 1, 1, /*size*/\
      (unsigned char*)FPGA_STATE1_ADDR, /*src address*/\
      (unsigned char*)&sdataSt[0][i][0], /*dest address*/ \
      EVENT_DATA_STATE1, /*event*/ \
      LINK_CHANNEL, /*type*/\
      0, /*EVENT QUEUE*/ \
      INCR_MODE, /*SRC*/ \
      INCR_MODE, /*DEST*/\
      &linkChnl2_1, \
      INT_ENABLE, \
      1,\
    	1 \
      );
    if(rc)
    {
      printf("create link channel for data_state1 failed\n");
      return -1;
    }
    chain_channel(linkChnl1_1, linkChnl2_1);

    link_channel(linkChnl1_0, linkChnl1_1);
    link_channel(linkChnl2_0, linkChnl2_1);

    linkChnl1_0 = linkChnl1_1;    
    linkChnl2_0 = linkChnl2_1;

    if(i == 0)
    {
      mainChnl1 = linkChnl1_1;
      mainChnl2 = linkChnl2_1;
    }

  } //for 

  //loop last link node to first link node
  link_channel(linkChnl1_1, mainChnl1);
  link_channel(linkChnl2_1, mainChnl2);
  
  /**************************/
  /*  LOAD CHANNEL          */
  //trigger to load first paramSet to current paramSet
  rc = enable_transfer(data1_mainChnl1, EDMA3_DRV_TRIG_MODE_MANUAL);
  if(rc)
  {
    printf("data1: enable transfer failed\n");
    return -1;
  }
  wait_complete(EVENT_DATA_STATE1);
  EVM6424_GPIO_clear_INT(EVENT_DATA_STATE1);
  return 0;
}


int setup_s_data2_channel()
{
  unsigned int data2_mainChnl1;
  unsigned int data2_mainChnl2;
  unsigned int mainChnl1;
  unsigned int mainChnl2;
  unsigned int linkChnl1_0;
  unsigned int linkChnl1_1;
  unsigned int linkChnl2_0;
  unsigned int linkChnl2_1;
  int rc;
  int i;
  
	//create data2 main channel(NOTE: init length is not use, we change it in trigger time)
  rc = create_channel(1, 1, 1, /*size*/\
    (unsigned char*)FPGA_DATA2_ADDR, /*src address*/\
    (unsigned char*)&s_sample_data[1][0][0], /*dest address*/ \
    EVENT_DATA2, /*event*/ \
    MAIN_CHANNEL, /*type*/\
    1, /*EVENT QUEUE*/ \
    INCR_MODE, /*SRC*/ \
    INCR_MODE, /*DEST*/\
    &data2_mainChnl1, \
    INT_DISABLE, \
    1,\
    1 \
    );
  if(rc)
  {
    printf("create main channel for data2 failed\n");
    return -1;
  }
  
  //create data2 state channel
  rc = create_channel(S_STATE_BUF_SIZE*sizeof(short), 1, 1, /*size*/\
    (unsigned char*)FPGA_STATE2_ADDR, /*src address*/\
    (unsigned char*)&sdataSt[1][0][0], /*dest address*/ \
    EVENT_DATA_STATE2, /*event*/ \
    MAIN_CHANNEL, /*type*/\
    1, /*EVENT QUEUE*/ \
    INCR_MODE, /*SRC*/ \
    INCR_MODE, /*DEST*/\
    &data2_mainChnl2, \
    INT_ENABLE, \
    1,\
    1 \
    );
  if(rc)
  {
    printf("create main channel for data_state2 failed\n");
    return -1;
  }
  chain_channel(data2_mainChnl1, data2_mainChnl2);
	
	/*****************************************************************************************/
  /*create channel for data2 receive(ring link and chain mode,event-queue0 and event-queue1)*/
  /*****************************************************************************************/
  //create array of chain channel and link channel
  linkChnl1_0 = data2_mainChnl1;
  linkChnl2_0 = data2_mainChnl2;
  for(i=0; i<MAX_BUF_IN_S_POOL; i++)
  {
    rc = create_channel(1, 1, 1, /*size*/\
      (unsigned char*)FPGA_DATA2_ADDR, /*src address*/\
      (unsigned char*)&s_sample_data[1][i][0], /*dest address*/ \
      EVENT_DATA2, /*event*/ \
      LINK_CHANNEL, /*type*/\
      1, /*EVENT QUEUE*/ \
      INCR_MODE, /*SRC*/ \
      INCR_MODE, /*DEST*/\
      &linkChnl1_1, \
      INT_DISABLE, \
      1,\
    	1 \
      );
    if(rc)
    {
      printf("create link channel for data2 failed\n");
      return -1;
    }
  
    rc = create_channel(S_STATE_BUF_SIZE*sizeof(short), 1, 1, /*size*/\
      (unsigned char*)FPGA_STATE2_ADDR, /*src address*/\
      (unsigned char*)&sdataSt[1][i][0], /*dest address*/ \
      EVENT_DATA_STATE2, /*event*/ \
      LINK_CHANNEL, /*type*/\
      1, /*EVENT QUEUE*/ \
      INCR_MODE, /*SRC*/ \
      INCR_MODE, /*DEST*/\
      &linkChnl2_1, \
      INT_ENABLE, \
      1,\
    	1 \
    );
    if(rc)
    {
      printf("create link channel for data_state2 failed\n");
      return -1;
    }
    chain_channel(linkChnl1_1, linkChnl2_1);

    link_channel(linkChnl1_0, linkChnl1_1);
    link_channel(linkChnl2_0, linkChnl2_1);
    
    linkChnl1_0 = linkChnl1_1;    
    linkChnl2_0 = linkChnl2_1;

    if(i == 0)
    {
      mainChnl1 = linkChnl1_1;
      mainChnl2 = linkChnl2_1;
    }

  } //for 

  //loop last link node to first link node
  link_channel(linkChnl1_1, mainChnl1);
  link_channel(linkChnl2_1, mainChnl2);
  
  //trigger to load first paramSet to current paramSet
  rc = enable_transfer(data2_mainChnl1, EDMA3_DRV_TRIG_MODE_MANUAL);
  if(rc)
  {
    printf("data2: enable transfer failed\n");
    return -1;
  }
  wait_complete(EVENT_DATA_STATE2);
  EVM6424_GPIO_clear_INT(EVENT_DATA_STATE2);

  return 0;
}

/*********************************/
/*    C MODE CHANNEL             */
/*********************************/

int setup_c_data1_channel()
{
  unsigned int data1_mainChnl1;
  unsigned int data1_mainChnl2;
  unsigned int mainChnl1;
  unsigned int mainChnl2;
  unsigned int linkChnl1_0;
  unsigned int linkChnl1_1;
  unsigned int linkChnl2_0;
  unsigned int linkChnl2_1;
  
  int rc;
  int i;
  
	//create data2 main channel(NOTE: init length is not use, we change it in trigger time)
  rc = create_channel(1, 1, 1, /*size*/\
    (unsigned char*)FPGA_C_DATA1_ADDR, /*src address*/\
    (unsigned char*)&c_sample_data[0][0][0], /*dest address*/ \
    EVENT_C_DATA1, /*event*/ \
    MAIN_CHANNEL, /*type*/\
    1, /*EVENT QUEUE*/ \
    INCR_MODE, /*SRC*/ \
    INCR_MODE, /*DEST*/\
    &data1_mainChnl1, \
    INT_DISABLE, \
    1,\
    1 \
    );
  if(rc)
  {
    printf("create main channel for C data1 failed\n");
    return -1;
  }
  
  //create C state buffer dma channel
  rc = create_channel(C_STATE_BUF_SIZE*sizeof(short), 1, 1, /*size*/\
    (unsigned char*)FPGA_C_STATE1_ADDR, /*src address*/\
    (unsigned char*)&cdataSt[0][0][0], /*dest address*/ \
    EVENT_C_DATA_STATE1, /*event*/ \
    MAIN_CHANNEL, /*type*/\
    1, /*EVENT QUEUE*/ \
    INCR_MODE, /*SRC*/ \
    INCR_MODE, /*DEST*/\
    &data1_mainChnl2, \
    INT_ENABLE, \
    1,\
    1 \
    );
  if(rc)
  {
    printf("create main channel for c data state1 failed\n");
    return -1;
  }
  chain_channel(data1_mainChnl1, data1_mainChnl2);
	
  /*****************************************************************************************/
  /*create channel for data2 receive(ring link and chain mode,event-queue0 and event-queue1)*/
  /*****************************************************************************************/
  //create array of chain channel and link channel
  linkChnl1_0 = data1_mainChnl1;
  linkChnl2_0 = data1_mainChnl2;
  for(i=0; i<MAX_BUF_IN_C_POOL; i++)
  {
    rc = create_channel(1, 1, 1, /*size*/\
      (unsigned char*)FPGA_C_DATA1_ADDR, /*src address*/\
      (unsigned char*)&c_sample_data[0][i][0], /*dest address*/ \
      EVENT_C_DATA1, /*event*/ \
      LINK_CHANNEL, /*type*/\
      1, /*EVENT QUEUE*/ \
      INCR_MODE, /*SRC*/ \
      INCR_MODE, /*DEST*/\
      &linkChnl1_1, \
      INT_DISABLE, \
      1,\
    	1 \
      );
    if(rc)
    {
      printf("create link channel for c data1 failed\n");
      return -1;
    }
  
    //create C state buffer dma channel
    rc = create_channel(C_STATE_BUF_SIZE*sizeof(short), 1, 1, /*size*/\
    (unsigned char*)FPGA_C_STATE1_ADDR, /*src address*/\
    (unsigned char*)&cdataSt[0][i][0], /*dest address*/ \
    EVENT_C_DATA_STATE1, /*event*/ \
    LINK_CHANNEL, /*type*/\
    1, /*EVENT QUEUE*/ \
    INCR_MODE, /*SRC*/ \
    INCR_MODE, /*DEST*/\
    &linkChnl2_1, \
    INT_ENABLE, \
    1,\
    1 \
    );
    if(rc)
    {
      printf("create link channel for c data state1 failed\n");
      return -1;
    }
    chain_channel(linkChnl1_1, linkChnl2_1);
    
    link_channel(linkChnl1_0, linkChnl1_1);
    link_channel(linkChnl2_0, linkChnl2_1);
    
    linkChnl1_0 = linkChnl1_1; 
    linkChnl2_0 = linkChnl2_1; 
    if(i == 0)
    {
      mainChnl1 = linkChnl1_1;
      mainChnl2 = linkChnl2_1;
    }

  } //for 

  //loop last link node to first link node
  link_channel(linkChnl1_1, mainChnl1);
  link_channel(linkChnl2_1, mainChnl2);
  
  //trigger to load first paramSet to current paramSet
  rc = enable_transfer(data1_mainChnl1, EDMA3_DRV_TRIG_MODE_MANUAL);
  if(rc)
  {
    printf("data2: enable transfer failed\n");
    return -1;
  }
  wait_complete(EVENT_C_DATA_STATE1);
  EVM6424_GPIO_clear_INT(EVENT_C_DATA_STATE1);

  return 0;
}

int setup_c_data2_channel()
{
  unsigned int data2_mainChnl1;
  unsigned int data2_mainChnl2;
  unsigned int mainChnl1;
  unsigned int mainChnl2;
  unsigned int linkChnl1_0;
  unsigned int linkChnl1_1;
  unsigned int linkChnl2_0;
  unsigned int linkChnl2_1;
  int rc;
  int i;
  
	//create data2 main channel(NOTE: init length is not use, we change it in trigger time)
  rc = create_channel(1, 1, 1, /*size*/\
    (unsigned char*)FPGA_C_DATA2_ADDR, /*src address*/\
    (unsigned char*)&c_sample_data[1][0][0], /*dest address*/ \
    EVENT_C_DATA2, /*event*/ \
    MAIN_CHANNEL, /*type*/\
    1, /*EVENT QUEUE*/ \
    INCR_MODE, /*SRC*/ \
    INCR_MODE, /*DEST*/\
    &data2_mainChnl1, \
    INT_DISABLE, \
    1,\
    1 \
    );
  if(rc)
  {
    printf("create main channel for c data2 failed\n");
    return -1;
  }
 
  //create C state buffer dma channel
  rc = create_channel(C_STATE_BUF_SIZE*sizeof(short), 1, 1, /*size*/\
    (unsigned char*)FPGA_C_STATE2_ADDR, /*src address*/\
    (unsigned char*)&cdataSt[1][0][0], /*dest address*/ \
    EVENT_C_DATA_STATE2, /*event*/ \
    MAIN_CHANNEL, /*type*/\
    1, /*EVENT QUEUE*/ \
    INCR_MODE, /*SRC*/ \
    INCR_MODE, /*DEST*/\
    &data2_mainChnl2, \
    INT_ENABLE, \
    1,\
    1 \
    );
  if(rc)
  {
    printf("create main channe2 for c data state1 failed\n");
    return -1;
  }
  chain_channel(data2_mainChnl1, data2_mainChnl2);
  
  /*****************************************************************************************/
  /*create channel for data2 receive(ring link and chain mode,event-queue0 and event-queue1)*/
  /*****************************************************************************************/
  //create array of chain channel and link channel
  linkChnl1_0 = data2_mainChnl1;
  linkChnl2_0 = data2_mainChnl2;
  for(i=0; i<MAX_BUF_IN_C_POOL; i++)
  {
    rc = create_channel(1, 1, 1, /*size*/\
      (unsigned char*)FPGA_C_DATA2_ADDR, /*src address*/\
      (unsigned char*)&c_sample_data[1][i][0], /*dest address*/ \
      EVENT_C_DATA2, /*event*/ \
      LINK_CHANNEL, /*type*/\
      1, /*EVENT QUEUE*/ \
      INCR_MODE, /*SRC*/ \
      INCR_MODE, /*DEST*/\
      &linkChnl1_1, \
      INT_DISABLE, \
      1,\
    	1 \
      );
    if(rc)
    {
      printf("create link channel for c data2 failed\n");
      return -1;
    }
  	
    //create C state buffer dma channel
    rc = create_channel(C_STATE_BUF_SIZE*sizeof(short), 1, 1, /*size*/\
      (unsigned char*)FPGA_C_STATE2_ADDR, /*src address*/\
      (unsigned char*)&cdataSt[1][i][0], /*dest address*/ \
      EVENT_C_DATA_STATE2, /*event*/ \
      LINK_CHANNEL, /*type*/\
      1, /*EVENT QUEUE*/ \
      INCR_MODE, /*SRC*/ \
      INCR_MODE, /*DEST*/\
      &linkChnl2_1, \
      INT_ENABLE, \
      1,\
      1 \
      );
    if(rc)
    {
      printf("create link channel for c data state2 failed\n");
      return -1;
    }
    chain_channel(linkChnl1_1, linkChnl2_1);
    
    
    link_channel(linkChnl1_0, linkChnl1_1);
    link_channel(linkChnl2_0, linkChnl2_1);
    
    linkChnl1_0 = linkChnl1_1;    
    linkChnl2_0 = linkChnl2_1;    
    
    if(i == 0)
    {
      mainChnl1 = linkChnl1_1;
      mainChnl2 = linkChnl2_1;
    }

  } //for 

  //loop last link node to first link node
  link_channel(linkChnl1_1, mainChnl1);
  link_channel(linkChnl2_1, mainChnl2);
  
  //trigger to load first paramSet to current paramSet
  rc = enable_transfer(data2_mainChnl1, EDMA3_DRV_TRIG_MODE_MANUAL);
  if(rc)
  {
    printf("data2: enable transfer failed\n");
    return -1;
  }
  wait_complete(EVENT_C_DATA_STATE2);
  EVM6424_GPIO_clear_INT(EVENT_C_DATA_STATE2);

  return 0;
}



int setup_report_channel()
{
  unsigned int mainChnl1;
  int rc;

	/****************************************************************/
  /*create channel for S report1 send(link myself mode, event-queue2)*/
  /****************************************************************/
  //create main channel
  //NOTE: src address need change when trigger a dma process
  rc = create_channel(REPORT_LEN, 1, 1, /*size*/\
    (unsigned char*)&gFrameReport[0], /*src address*/\
    (unsigned char*)FPGA_REPORT_ADDR1, /*dest address*/ \
    EVENT_REPORT1, /*event*/ \
    MAIN_CHANNEL, /*type*/\
    2, /*EVENT QUEUE*/ \
    INCR_MODE, /*SRC*/ \
    INCR_MODE, /*DEST*/\
    &mainChnl1, \
    INT_DISABLE,/*INT_ENABLE*/ \
    1,\
    1 \
    );
  if(rc)
  {
    printf("create main channel for frame send failed\n");
    return -1;
  }

  //link to itself
  link_channel(mainChnl1, mainChnl1);
  
  /****************************************************************/
  /*create channel for S report2 send(link myself mode, event-queue2)*/
  /****************************************************************/
  rc = create_channel(REPORT_LEN, 1, 1, /*size*/\
    (unsigned char*)&gFrameReport[1], /*src address*/\
    (unsigned char*)FPGA_REPORT_ADDR2, /*dest address*/ \
    EVENT_REPORT2, /*event*/ \
    MAIN_CHANNEL, /*type*/\
    2, /*EVENT QUEUE*/ \
    INCR_MODE, /*SRC*/ \
    INCR_MODE, /*DEST*/\
    &mainChnl1, \
    INT_DISABLE,/*INT_ENABLE*/ \
    1,\
    1 \
    );
  if(rc)
  {
    printf("create main channel for frame send failed\n");
    return -1;
  }

  //link to itself
  link_channel(mainChnl1, mainChnl1);

  return 0;
}

/*priority in one event queue is determined by CHANNEL NUMBER*/
/* channel number low have higher priority*/
int init_adsb_dma()
{
  EDMA3_DRV_Result edmaResult = EDMA3_DRV_SOK;
  EDMA3_DRV_EvtQuePriority pri;

  //init edma3
  edmaResult = edma3init();
  if (edmaResult != EDMA3_DRV_SOK)
  {
    printf("echo: edma3init() FAILED, error code: %d\r\n", edmaResult);
  }

  //clear all event miss
  _EMCR  = 0xffffffff;
  _EMCRH = 0xffffffff;
  
  setup_s_data1_channel();
  setup_s_data2_channel();
  setup_c_data1_channel();
  setup_c_data2_channel();
  setup_report_channel();
  
  /************************************************/
  /*            SET EVENT QUEUE PRIORITY          */
  /************************************************/
  //default rule is low channel number is high priority
  //use EDMA3_DRV_setEvtQPriority change it, set event-queue2 to highest pri, event-queue0 to lowest pri
  pri.evtQPri[0] = 2;
  pri.evtQPri[1] = 1;
  pri.evtQPri[2] = 0;
  EDMA3_DRV_setEvtQPriority(hEdma, &pri);

  return 0;
}



