#include <stdio.h>
#include "tcas.h"

//#include "tcas.h"
//extern void test_error();

extern volatile char g_s_sample_full[MAX_POOL_NUM];
extern volatile char g_c_sample_full[MAX_POOL_NUM];

unsigned long gSReleaseCnt[MAX_POOL_NUM]={0,0};
unsigned long gCReleaseCnt[MAX_POOL_NUM]={0,0};

int app_main();

int main()
{
  // test_error();
 
 // test_main();
//  init_isr();

  app_main();
  
  return 0;
}

int app_main()
{
  int rc;
  int pool_id;
  int i;
//  myprintf("app main start\n");
    
  //only for flash boot debug
  #ifdef __TIME_TEST
  while(0)
  {

    EVM6424_GPIO_setOutput(100, 0);
    for(i=0;i<5000;i++);
    EVM6424_GPIO_setOutput(100, 1);
    for(i=0;i<5000;i++);
  }
  #endif
    
  //do some init work
  rc = init_system();
  if(rc)
  {
    myprintf("system init failed, progame is exiting... \n");
    return rc;
  }
  
#ifdef __TEST__
  init_test_data();
  trigger_test(pool_id);
#endif

  //MAIN PROCESS
  while(1)
  {
    //poll S frame
    pool_id = S_select_recv();
    if((pool_id == 0) || (pool_id == 1))
    {
      s_decode(pool_id);
      if(g_s_sample_full[pool_id])
	  {
        release_fpga_buffer(S_MODE, pool_id);
        g_s_sample_full[pool_id]=0;
		gSReleaseCnt[pool_id]++;
      }
    }
     
    //poll C frame
    pool_id = C_select_recv();
	  if((pool_id == 0) || (pool_id == 1))
    {  
      c_decode(pool_id);
      if(g_c_sample_full[pool_id])
	  {
        release_fpga_buffer(C_MODE, pool_id);
    	g_c_sample_full[pool_id]=0;
		gCReleaseCnt[pool_id]++;
	  }
	} 
  }
}
