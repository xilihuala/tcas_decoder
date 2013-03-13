#include <stdio.h>
volatile int c1, c2, c3;
void clear_int(int intNo);
interrupt void PerformIsr1(void)
{
  clear_int(4);
  c1++;
}

interrupt void PerformIsr2(void)
{
  c2++;
}

interrupt void PerformIsr3(void)
{
  c3++;
}


void trigger_int(int gpio)
{
#if 0
  int i;
  EVM6424_GPIO_setOutput(gpio, 1);
  for(i=0;i<5000;i++);
  EVM6424_GPIO_setOutput(gpio, 0);
#else
  __asm(" MVK 10h, B16");
  __asm(" MVC B16, ISR");
#endif
}

void clear_int(int intNo)
{
  __asm(" MVK 10h, B16;"); //set bit 4
  __asm(" MVC B16,ICR"); //set ICR
}



void read_IFR()
{
  __asm(" NOP");
  __asm(" NOP");
  __asm(" NOP");
  __asm(" MVC IFR,B16");
  __asm(" MVC IER,B17");
  __asm(" MVC CSR,B18");
  __asm(" NOP");
//  __asm(" MVC ISTP,B2");

}

void test_isr()
{
  int i, j;
  
  c1 = c2 = c3 = 0;
  
  //init 
  init_isr();
   
  for(i=0;i<100;i++)
  {
    //trigger isr1
    trigger_int(0);

    for(j=0;j<10000;j++);   
    
    //get isr1 cnt
    printf("(%u)c1=%u\n", i, c1);
    
    read_IFR();
  }
}
