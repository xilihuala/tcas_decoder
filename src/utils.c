#include <stdio.h>
#include "tcas.h"

/*#define PRINT_DEBUG*/

unsigned char _diff(unsigned char a, unsigned char b)
{
  return a-b;
}

unsigned char _abs_diff(unsigned char a, unsigned char b)
{
  if(a > b)
    return a-b;
  else
    return b-a;
}

void wait_debug(int cnt)
{
  int i;
  for(i=0;i<cnt;i++);
}

INLINE_DESC void myprintf(char *format, ...)
{
#ifdef PRINT_DEBUG
  va_list ap;
  va_start(ap, format);
  vprintf(format,ap);
  va_end(ap);
#endif
}

INLINE_DESC void dmaprintf(char *format, ...)
{
#ifdef PRINT_DEBUG
  va_list ap;
  va_start(ap, format);
  vprintf(format,ap);
  va_end(ap);
#endif
}

INLINE_DESC short getMax4(short s1, short s2, short s3, short s4)
{
  short maxval;
  
  maxval = s1;
  
  if(maxval < s2)
    maxval = s2;
  if(maxval < s3)
    maxval = s3;
  if(maxval < s4)
    maxval = s4;
  return maxval;
}


//todo
char get_operate_mode()
{
  return 0;
}

//todo
int get_dest_address()
{
  return 0;
}

//todo
short get_range()
{
  return 0;
}

char get_TB()
{
  return 0;
}

char get_pat()
{
  return 0;
}

char get_at()
{
  return 0;
}

char get_bs_sum()
{
  return 0;
}

char get_bs_dlt()
{
  return 0;
}

char get_omni()
{
  return 0;
}

