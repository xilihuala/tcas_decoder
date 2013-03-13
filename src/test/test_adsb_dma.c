#include <ecm.h>
#include <bcache.h>
#include "bios_edma3_drv_sample.h"
#include "tcas.h"

#pragma DATA_ALIGN(src, 128);
#pragma DATA_ALIGN(dest,128);
#pragma DATA_ALIGN(src2, 128);
#pragma DATA_ALIGN(dest2,128);
#pragma DATA_SECTION(src, "my_sect");
#pragma DATA_SECTION(dest, "my_sect");
#pragma DATA_SECTION(src2, "my_sect");
#pragma DATA_SECTION(dest2, "my_sect");

short src[128];
short dest[128];
short src2[128];
short dest2[128];

static void test_init()
{
  BCACHE_Size cacheSize;
  
  /*init GPIO for test*/
  EVM6424_GPIO_init();
  
  /*init GPIO0 as STATE1 EVENT*/
  EVM6424_GPIO_setDir(0, 0); //set GPIO0 as output
  EVM6424_GPIO_setOutput(0, 0); //set output to zero
  EVM6424_GPIO_setTrigEdge(0, 1); //rising edge trigger
  EVM6424_GPIO_enable_INT(0);
  
  /*init GPIO1 as STATE2 EVENT*/
  EVM6424_GPIO_setDir(1, 0); //set GPIO1 as output
  EVM6424_GPIO_setOutput(1, 0); //set output to zero
  EVM6424_GPIO_setTrigEdge(1, 1); //rising edge trigger
  EVM6424_GPIO_enable_INT(1);
  
  //init cache
  cacheSize.l1psize = BCACHE_L1_32K;
  cacheSize.l1dsize = BCACHE_L1_32K;
  cacheSize.l2size =  BCACHE_L2_32K;
  BCACHE_setSize(&cacheSize);
 
  //enable all cache
  BCACHE_setMode(BCACHE_L1P, BCACHE_NORMAL);
  BCACHE_setMode(BCACHE_L1D, BCACHE_NORMAL);
  BCACHE_setMode(BCACHE_L2,  BCACHE_NORMAL);
  
}


static void test_init_src(short *src, int val)
{
  int i;
  for(i=0;i<128;i++)
    src[i] = i+val;
    
}

int test_main()
{
  int rc;
  unsigned int chnl1, chnl2;
  int i;
  EDMA3_DRV_Result edmaResult;
  
  test_init();
  
  edmaResult = edma3init();
  if (edmaResult != EDMA3_DRV_SOK)
  {
    printf("echo: edma3init() FAILED, error code: %d\r\n",
      edmaResult);
  }
  else
  {
    printf("echo: edma3init() PASSED\r\n");
  }
   
  
  /*********************/
  /*  test REPORT      */
  /*********************/
  rc = create_channel(128*2, 1, 1, /*size*/ \
                      (unsigned char*)src, /*src address*/ \
                      (unsigned char*)dest, /*dest address*/ \
                      EVENT_REPORT1, /*event*/ \
                      MAIN_CHANNEL, /*type*/ \
                      2, /*EVENT QUEUE*/ \
                      INCR_MODE, /*SRC*/ \
                      INCR_MODE, /*DEST*/ \
                      &chnl1, \
                      INT_ENABLE \
                      );
  if(rc)
  {
    printf("create main channel for REPORT failed\n");
    return -1;
  }
  
  //link to itself
  link_channel(chnl1, chnl1);
  
  test_init_src(src, 0);
  
  //enable transfer
  SendReportToFPGA(src);
  
  //wait complete
  wait_complete(EVENT_REPORT1);
  
  //check result
  for(i=0;i<128;i++)
  {
    if(src[i] != dest[i])
      break;
  }
  if(i != 128)
    printf("TEST REPORT: FAILED\n");
  else
    printf("TEST REPORT: SUCCESS\n");   
  
  
  /*********************/
  /*  test FRAMEOUT    */
  /*********************/
  rc = create_channel(128*2, 1, 1, /*size*/ \
                      (unsigned char*)src, /*src address*/ \
                      (unsigned char*)dest, /*dest address*/ \
                      EVENT_FRAMEOUT, /*event*/ \
                      MAIN_CHANNEL, /*type*/ \
                      2, /*EVENT QUEUE*/ \
                      INCR_MODE, /*SRC*/ \
                      INCR_MODE, /*DEST*/ \
                      &chnl1, \
                      INT_ENABLE \
                      );
  if(rc)
  {
    printf("create main channel for FRAMEOUT failed\n");
    return -1;
  }
  
  //link to itself
  link_channel(chnl1, chnl1);
  
  test_init_src(src, 10);
  
  //enable transfer
  sendFrameData(dest);
  
  //wait complete
  wait_complete(EVENT_FRAMEOUT);

  Edma3_CacheInvalidate((unsigned int)dest, 128*2);//invalid dest address 's cache
  
  //check result
  for(i=0;i<128;i++)
  {
    if(src[i] != dest[i])
      break;
  }
  if(i != 128)
    printf("TEST FRAMEOUT: FAILED\n");
  else
    printf("TEST FRAMEOUT: SUCCESS\n");   
  
  
  
  
  /***********************/
  /*  test SAMPLE DATA(one chain , link to self*/
  /**********************/
  rc = create_channel(1, 1, 1, /*size*/ \
                      (unsigned char*)src, /*src address*/ \
                      (unsigned char*)dest, /*dest address*/ \
                      EVENT_DATA1, /*event*/ \
                      MAIN_CHANNEL, /*type*/ \
                      0, /*EVENT QUEUE*/ \
                      INCR_MODE, /*SRC*/ \
                      INCR_MODE, /*DEST*/ \
                      &chnl1, \
                      INT_DISABLE \
                      );
  if(rc)
  {
    printf("create main channel for data1 failed\n");
    return -1;
  }
  
  rc = create_channel(128*2, 1, 1, /*size*/ \
                      (unsigned char*)src2, /*src address*/ \
                      (unsigned char*)dest2, /*dest address*/ \
                      EVENT_DATA_STATE1, /*event*/ \
                      MAIN_CHANNEL, /*type*/ \
                      1, /*EVENT QUEUE*/ \
                      INCR_MODE, /*SRC*/ \
                      INCR_MODE, /*DEST*/ \
                      &chnl2, \
                      INT_ENABLE \
                      );
  if(rc)
  {
    printf("create main channel for data_state1 failed\n");
    return -1;
  }
  chain_channel(chnl1, chnl2);
  
  memset(dest,0,128*2);
  memset(dest2, 0, 128*2);
  test_init_src(src,  1);
  test_init_src(src2, 2);
  
  retrieve_sample_data(0, 128*2);
  
  //wait complete
  wait_complete(EVENT_DATA_STATE1);
  
  //check result
  for(i=0;i<128;i++)
  {
    if(src[i] != dest[i])
      break;
    if(src2[i] != dest2[i])
      break;
  }
  if(i != 128)
    printf("TEST SAMPLE DATA: FAILED\n");
  else
    printf("TEST SAMPLE DATA: SUCCESS\n");
   
#ifdef USE_STATE_EVENT  
  /*********************/
  /*  test STATE       */
  /*********************/
  rc = create_channel(128*sizeof(short), 1, 1, /*size*/ \
                      (unsigned char*)src, /*src address*/ \
                      (unsigned char*)dest, /*dest address*/ \
                      EVENT_STATE1, /*event*/ \
                      MAIN_CHANNEL, /*type*/ \
                      0, /*EVENT QUEUE*/ \
                      INCR_MODE, /*SRC*/ \
                      INCR_MODE, /*DEST*/ \
                      &chnl1, \
                      INT_ENABLE \
                      );
  if(rc)
  {
    printf("create main channel for state1 failed\n");
    return -1;
  }
  
  rc = create_channel(STATE_BUF_SIZE*sizeof(short), 1, 1, /*size*/ \
                      (unsigned char*)src, /*src address*/ \
                      (unsigned char*)dest, /*dest address*/ \
                      EVENT_STATE2, /*event*/ \
                      MAIN_CHANNEL, /*type*/ \
                      0, /*EVENT QUEUE*/ \
                      INCR_MODE, /*SRC*/ \
                      INCR_MODE, /*DEST*/ \
                      &chnl2, \
                      INT_ENABLE \
                      );
  if(rc)
  {
    printf("create main channel for state2 failed\n");
    return -1;
  }
  
  test_init_src(src,  3);
  test_init_src(src2, 4);
  
  //simulate GPIO0 event
  EVM6424_GPIO_setOutput(0, 1);
  for(i=0;i<5000;i++);
  EVM6424_GPIO_setOutput(0, 0);
  
  for(i=0;i<5000;i++);
  
  //simulate GPIO0 event
  EVM6424_GPIO_setOutput(0, 1);
  for(i=0;i<5000;i++);
  EVM6424_GPIO_setOutput(0, 0);
    
  wait_complete(EVENT_DATA_STATE1);
  wait_complete(EVENT_DATA_STATE2);
  
  //check result
  for(i=0;i<128;i++)
  {
    if(src[i] != dest[i])
      break;
    if(src2[i] != dest2[i])
      break;
  }
  if(i != 128)
    printf("TEST STATE: FAILED\n");
  else
    printf("TEST STATE: SUCCESS\n");
    
#endif
  
  return 0;
}
