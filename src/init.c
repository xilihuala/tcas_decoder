#include <ecm.h>
#include <bcache.h>
#include "tcas.h"
#include "edma3_drv.h"

extern event_desc_t eventList[];
extern unsigned int EVENT_NUM;

extern gpio_desc_t  gpioList[];
extern unsigned int GPIO_NUM;

extern REPORT_T gFrameEnd;

/**
* Variable which will be used internally for referring transfer completion
* interrupt.
*/
unsigned int ccXferCompInt = EDMA3_CC_XFER_COMPLETION_INT;

/**
* Variable which will be used internally for referring channel controller's
* error interrupt.
*/
unsigned int ccErrorInt = EDMA3_CC_ERROR_INT;

//int bitoff;
void setup_int(int intNum, int event)
{
  int mux,evoff;
  if(intNum <4) 
	  return;
		
  mux = intNum/4;
  evoff = (intNum%4)*8;
  
  //bitoff = 1<<intNum;
  //__asm(" LDH *+B14(_bitoff), B16");

  if(mux == 1)	
  {
    //map event
    INTMUX1 &= ~(0x7f << evoff);
    INTMUX1 |= (event & 0x7f) << evoff;
  }
  else if(mux == 2)
  {
    //map event
    INTMUX2 &= ~(0x7f << evoff);
    INTMUX2 |= (event & 0x7f) << evoff;
  }
  else if(mux == 3)
  {
    //map event
    INTMUX3 &= ~(0x7f << evoff);
    INTMUX3 |= (event & 0x7f) << evoff;
  }
  else
    return;

  if(intNum == 4)
  {
    __asm(" MVK 10h, B16"); //set bit 5
    __asm(" MVK 10h,B17"); //set bit 5
  }
  else if(intNum == 5)
  {
    __asm(" MVK 20h, B16"); //set bit 5
    __asm(" MVK 20h,B17"); //set bit 5
  }
  else if(intNum == 6)
  {
    __asm(" MVK 40h, B16"); //set bit 5
    __asm(" MVK 40h,B17"); //set bit 5
  }
  else if(intNum == 7)
  {
    __asm(" MVK 80h, B16"); //set bit 5
    __asm(" MVK 80h,B17"); //set bit 5
  }
  else if(intNum == 8)
  {
    __asm(" MVK 100h, B16"); //set bit 5
    __asm(" MVK 100h,B17"); //set bit 5
  }
  else if(intNum == 9)
  {
    __asm(" MVK 200h, B16"); //set bit 5
    __asm(" MVK 200h,B17"); //set bit 5
  }
  else
    return;

  //clear INT
  __asm(" MVC B16,ICR"); //set ICR

  //enable INT
  __asm(" MVC IER,B16"); //get IER
  __asm(" OR B17,B16,B16"); //get ready to set IE
  __asm(" MVC B16,IER"); //set IER

}

void enable_gie()
{
  __asm(" MVC CSR,B16");
  __asm(" OR 1,B16,B16");
  __asm(" MVC B16,CSR"); 
}

void enable_nmie()
{
  __asm(" MVK 2h,B17"); //set bit 2
  __asm(" MVC IER,B16"); //get IER
  __asm(" OR B17,B16,B16"); //get ready to set IE
  __asm(" MVC B16,IER"); //set IER

}

void init_isr()
{
  int i;
  //SETUP INTERRUPT
  for(i=0; i< EVENT_NUM; i++)
  {
  	setup_int(eventList[i].vec_num, eventList[i].event);
  }
  
  /******************/
  /*ENABLE INTERRUPT*/
  /*****************/
  //enable GIE
  enable_gie();

  //enable NMIE
  enable_nmie();  
}

INLINE_DESC void init_gpio()
{
  int i;
  
 /*init GPIO for test*/
  EVM6424_GPIO_init();

  EVM6424_GPIO_ClearAllTrigEdge();
  
  for(i=0;i<GPIO_NUM;i++)
  {
    char gpioNum;
    gpioNum = gpioList[i].gpioNum;
    
    if(gpioList[i].type == GPIO_INPUT)
      EVM6424_GPIO_setDir(gpioNum, 1); //set GPIO0 as input
    else if(gpioList[i].type == GPIO_OUTPUT)
      EVM6424_GPIO_setDir(gpioNum, 0); //set GPIO0 as input
 
    if(gpioList[i].int_enable == INT_ENABLE)
    {
      if(gpioList[i].int_stype == LEVEL_INT)
        EVM6424_GPIO_setTrigEdge(gpioNum, 0); //low level trigger : todo
      else if(gpioList[i].int_stype == RISE_PULSE_INT)  
        EVM6424_GPIO_setTrigEdge(gpioNum, 1); //rising edge trigger
      else if(gpioList[i].int_stype == DROP_PULSE_INT)  
        EVM6424_GPIO_setTrigEdge(gpioNum, 2); //drop edge trigger
      
   	  //clear gpio interrupt
	  EVM6424_GPIO_clear_INT(gpioNum); 
      
	  //enable interrupt in this gpio port
      EVM6424_GPIO_enable_INT(gpioNum);
    }    
  }

#ifdef __TIME_TEST
  //only for test
  EVM6424_GPIO_setDir(100, 0); //set GPIO100 as output
  EVM6424_GPIO_setOutput(100, 0);
#endif

}

INLINE_DESC void init_pinmux()
{
  _setPinMux((1<<8),0); //select CS4
  _setPinMux((1<<10),0); //select CS3

#ifdef __TIME_TEST
  //GPIO100, only for test
  _clrPinMux(0, 3<<22);
#endif

}

INLINE_DESC void init_cache()
{
  BCACHE_Size cacheSize;

  cacheSize.l1psize = BCACHE_L1_32K;
  cacheSize.l1dsize = BCACHE_L1_0K;
  cacheSize.l2size =  BCACHE_L2_0K;
  BCACHE_setSize(&cacheSize);

  BCACHE_setMode(BCACHE_L1P, BCACHE_NORMAL);
  BCACHE_setMode(BCACHE_L1D, BCACHE_BYPASS);
  BCACHE_setMode(BCACHE_L2,  BCACHE_BYPASS);
  
  //disbale DMA buffer cache
  BCACHE_setMar((char *)FPGA_DATA1_ADDR, 0x1000, BCACHE_MAR_DISABLE); //disable FPGA S buffer1 cache
  BCACHE_setMar((char *)FPGA_DATA2_ADDR, 0x1000, BCACHE_MAR_DISABLE); //disable FPGA S buffer2 cache
  BCACHE_setMar((char *)FPGA_C_DATA1_ADDR, 0x1000, BCACHE_MAR_DISABLE); //disable FPGA C buffer1 cache
  BCACHE_setMar((char *)FPGA_C_DATA2_ADDR, 0x1000, BCACHE_MAR_DISABLE); //disable FPGA C buffer2 cache
  
  BCACHE_setMar((char *)FPGA_STATE1_ADDR, S_STATE_BUF_SIZE, BCACHE_MAR_DISABLE); //disable FPGA S state1 cache
  BCACHE_setMar((char *)FPGA_STATE2_ADDR, S_STATE_BUF_SIZE, BCACHE_MAR_DISABLE); //disable FPGA S state2 cache
  BCACHE_setMar((char *)FPGA_C_STATE1_ADDR, C_STATE_BUF_SIZE, BCACHE_MAR_DISABLE); //disable FPGA C state1 cache
  BCACHE_setMar((char *)FPGA_C_STATE2_ADDR, C_STATE_BUF_SIZE, BCACHE_MAR_DISABLE); //disable FPGA C state2 cache
}

INLINE_DESC int init_app()
{
  S_decode_init();
  C_decode_init();
  report_init();
  
  return 0;
}

void init_fpga()
{
  //clear buffer
  release_fpga_buffer(S_MODE, 0);
  release_fpga_buffer(S_MODE, 1);
  release_fpga_buffer(C_MODE, 0);
  release_fpga_buffer(C_MODE, 1);
}

INLINE_DESC int init_system()
{
  int rc;

  //init CPU, use GEL config
  initDSP();

  /* following is additional config for our board */
  init_cache();
  init_pinmux();
  init_gpio();
  
  //init EDMA
  rc = init_adsb_dma();
  if(rc)
  {
    myprintf("DMA init failed\n");
    return rc;
  }
  
  //clear dma int state
  clear_all_dma_int();
  
  //init for decoder application
  rc = init_app();
  if(rc)
  {
    myprintf("APP init failed\n");
    return rc;
  }
  
    //init crc
  init_crc();

  init_isr();

  //init fpga
  init_fpga();

  return 0;
}
