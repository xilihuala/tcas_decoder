// ----------------------------------------------------------------------------
// CRC tester v1.3 written on 4th of February 2003 by Sven Reifegerste (zorc/reflex)
// This is the complete compilable C program, consisting only of this .c file.
// No guarantee for any mistakes.
//
// changes to CRC tester v1.2: 
//
// - remove unneccessary (!(polynom&1)) test for invalid polynoms
//   (now also XMODEM parameters 0x8408 work in c-code as they should)
//
// changes to CRC tester v1.1:
//
// - include an crc&0crcmask after converting non-direct to direct initial
//   value to avoid overflow 
//
// changes to CRC tester v1.0: 
//
// - most int's were replaced by unsigned long's to allow longer input strings
//   and avoid overflows and unnecessary type-casting's
// ----------------------------------------------------------------------------

// includes:
 
#include <string.h>
#include <stdio.h>
#include "tcas.h"

// CRC parameters :
/*
in S mode, g(x)=x^24 + x^23 + x^22 + x^21 + x^20 + x^19 + x^18 + x^17 + x^16 + x^15 + x^14 + x^13 + x^12 +x^10 +x^3 + 1
this fuction do CRC24 check
*/
const int order = 24;
const unsigned long polynom = 0xfff409;
const int direct = 0;
const unsigned long crcinit = 0;
const unsigned long crcxor = 0;
const int refin = 0;
const int refout = 0;

// 'order' [1..32] is the CRC polynom order, counted without the leading '1' bit
// 'polynom' is the CRC polynom without leading '1' bit
// 'direct' [0,1] specifies the kind of algorithm: 1=direct, no augmented zero bits
// 'crcinit' is the initial CRC value belonging to that algorithm
// 'crcxor' is the final XOR value
// 'refin' [0,1] specifies if a data byte is reflected before processing (UART) or not
// 'refout' [0,1] specifies if the CRC will be reflected before XOR


// internal global values:

unsigned long crcmask;
unsigned long crchighbit;
unsigned long crcinit_direct;
unsigned long crcinit_nondirect;
unsigned long crctab[256];

#define CYCLE_SHIFT_RIGHT(a,n,N) (((a<<(N-n))|(a>>n)) &((1<<N)-1))
#define CYCLE_SHIFT_LEFT(a,n,N)  (((a>>(N-n))|(a<<n)) &((1<<N)-1))
#define SYNDROME_LEFT_TRANSFORM(S,G)  (S&0x800000)?(((S<<1)&0xffffff)^G):((S<<1)&0xffffff)
#define SYNDROME_RIGHT_TRANSFORM(S,G) (S&0x1)?((S^G)>>1):(S>>1)

#define SWAP24(b)  ((b &0xff00) | ((b&0xff)<<16) |((b&0xff0000)>>16))

//char frame_buf[112];
unsigned long bitErrSynm[112];

// subroutines
unsigned long reflect (unsigned long crc, int bitnum)
{

  // reflects the lower 'bitnum' bits of 'crc'

  unsigned long i, j=1, crcout=0;

  for (i=(unsigned long)1<<(bitnum-1); i; i>>=1)
  {
    if (crc & i) crcout|=j;
    j<<= 1;
  }
  return (crcout);
}

void generate_crc_table()
{

  // make CRC lookup table used by table algorithms

  int i, j;
  unsigned long bit, crc;

  for (i=0; i<256; i++)
  {

    crc=(unsigned long)i;
    if (refin) crc=reflect(crc, 8);
    crc<<= order-8;

    for (j=0; j<8; j++)
    {

      bit = crc & crchighbit;
      crc<<= 1;
      if (bit) crc^= polynom;
    }

    if (refin) crc = reflect(crc, order);
    crc&= crcmask;
    crctab[i]= crc;
  }
}


unsigned long crctablefast (unsigned char* p, unsigned long len)
{

  // fast lookup table algorithm without augmented zero bytes, e.g. used in pkzip.
  // only usable with polynom orders of 8, 16, 24 or 32.

  unsigned int crc = crcinit_direct;

  if (refin) crc = reflect(crc, order);

  if (!refin) while (len--) crc = (crc << 8) ^ crctab[ ((crc >> (order-8)) & 0xff) ^ *p++];
  else while (len--) crc = (crc >> 8) ^ crctab[ (crc & 0xff) ^ *p++];

  if (refout^refin) crc = reflect(crc, order);
  crc^= crcxor;
  crc&= crcmask;

  return(crc);
}


unsigned long crctable (unsigned char* p, unsigned long len)
{

  // normal lookup table algorithm with augmented zero bytes.
  // only usable with polynom orders of 8, 16, 24 or 32.

  unsigned long crc = crcinit_nondirect;

  if (refin) crc = reflect(crc, order);

  if (!refin) while (len--) crc = ((crc << 8) | *p++) ^ crctab[ (crc >> (order-8))  & 0xff];
  else while (len--) crc = ((crc >> 8) | (*p++ << (order-8))) ^ crctab[ crc & 0xff];

  if (!refin) while (++len < order/8) crc = (crc << 8) ^ crctab[ (crc >> (order-8))  & 0xff];
  else while (++len < order/8) crc = (crc >> 8) ^ crctab[crc & 0xff];

  if (refout^refin) crc = reflect(crc, order);
  crc^= crcxor;
  crc&= crcmask;

  return(crc);
}



unsigned long crcbitbybit(unsigned char* p, unsigned long len)
{

  // bit by bit algorithm with augmented zero bytes.
  // does not use lookup table, suited for polynom orders between 1...32.

  unsigned long i, j, c, bit;
  unsigned long crc = crcinit_nondirect;

  for (i=0; i<len; i++)
  {

    c = (unsigned long)*p++;
    if (refin) c = reflect(c, 8);

    for (j=0x80; j; j>>=1)
    {

      bit = crc & crchighbit;
      crc<<= 1;
      if (c & j) crc|= 1;
      if (bit) crc^= polynom;
    }
  }

  for (i=0; i<order; i++)
  {

    bit = crc & crchighbit;
    crc<<= 1;
    if (bit) crc^= polynom;
  }

  if (refout) crc=reflect(crc, order);
  crc^= crcxor;
  crc&= crcmask;

  return(crc);
}



unsigned long crcbitbybitfast(unsigned char* p, unsigned long len)
{

  // fast bit by bit algorithm without augmented zero bytes.
  // does not use lookup table, suited for polynom orders between 1...32.

  unsigned long i, j, c, bit;
  unsigned long crc = crcinit_direct;

  for (i=0; i<len; i++)
  {

    c = (unsigned long)*p++;
    if (refin) c = reflect(c, 8);

    for (j=0x80; j; j>>=1)
    {

      bit = crc & crchighbit;
      crc<<= 1;
      if (c & j) bit^= crchighbit;
      if (bit) crc^= polynom;
    }
  }

  if (refout) crc=reflect(crc, order);
  crc^= crcxor;
  crc&= crcmask;

  return(crc);
}

unsigned char temp_buf[11];
void generate_error_syndrome()
{
  int i;
  int j,k;
  unsigned long crc_ok, crc_err;

  memset(temp_buf, 0, sizeof(temp_buf));

  for (i=0;i<88;i++)
  {
    crc_ok = crctablefast(temp_buf,11);

    j= i/8;
    k= i%8;
    temp_buf[j] |= 1<<k;
    crc_err = crctablefast(temp_buf,11);

    bitErrSynm[i] = crc_ok ^ crc_err;

    temp_buf[j] = 0;
  }
  for(i=88;i<112;i++)
  {
    j= (i-88)/8;
    k= i%8;
    bitErrSynm[i] = 1<<(23- j*8 - 7 + k);
  }
}

int init_crc()
{

  // test program for checking four different CRC computing types that are:
  // crcbit(), crcbitfast(), crctable() and crctablefast(), see above.
  // parameters are at the top of this program.
  // Result will be printed on the console.

  int i;
  unsigned long bit, crc;


  // at first, compute constant bit masks for whole CRC and CRC high bit

  crcmask = ((((unsigned long)1<<(order-1))-1)<<1)|1;
  crchighbit = (unsigned long)1<<(order-1);

  // check parameters

  if (order < 1 || order > 32)
  {
    printf("ERROR, invalid order, it must be between 1..32.\n");
    return(0);
  }

  if (polynom != (polynom & crcmask))
  {
    printf("ERROR, invalid polynom.\n");
    return(0);
  }

  if (crcinit != (crcinit & crcmask))
  {
    printf("ERROR, invalid crcinit.\n");
    return(0);
  }

  if (crcxor != (crcxor & crcmask))
  {
    printf("ERROR, invalid crcxor.\n");
    return(0);
  }


  // generate lookup table
  generate_crc_table();


  // compute missing initial CRC value
  if (!direct)
  {

    crcinit_nondirect = crcinit;
    crc = crcinit;
    for (i=0; i<order; i++)
    {

      bit = crc & crchighbit;
      crc<<= 1;
      if (bit) crc^= polynom;
    }
    crc&= crcmask;
    crcinit_direct = crc;
  }

  else
  {

    crcinit_direct = crcinit;
    crc = crcinit;
    for (i=0; i<order; i++)
    {

      bit = crc & 1;
      if (bit) crc^= polynom;
      crc >>= 1;
      if (bit) crc|= crchighbit;
    }
    crcinit_nondirect = crc;
  }

  //create syndrome table (bitErrSynm[])
  generate_error_syndrome();

  return(0);
}

int get_lowc_bitmap(int framelen, \
                    unsigned char *conf, \
                    unsigned char *number, \
                    unsigned char *length, \
                    unsigned long *bitmap, \
                    unsigned char *lowbit)
{
  unsigned char num;
  unsigned char span;
  unsigned long bmap;
  int i,j;
  unsigned char c;
  unsigned char k;
  char first;


  first = -1;
  bmap = 0;
  num = 0;
  span = 0;

  for (i=0;i<framelen+3;i++)
  {
    c = conf[i];
    if (c)
    {
      for (j=7;j>=0;j--)
      {
        if (c &(1<<j))
        {
          k = i*8 + 7 - j;
          if (first == -1)
          {
            bmap = 1;
            first = k;
          }
          else
            span = k - first;

          lowbit[num] = k;
          num ++;
          if (num > 12)//conservative only allow 12 low confidence bits
            goto _ret;
        }
      }
    }
  }

_ret:
  *number = num;
  *length = span;
  *bitmap = bmap;
  if (span > 24)
  {  
    if(num <= 5)
      return -2; //do brute
    else
      return -1;
  } 
  else
  {     
    if(num > 12)
      return -1;
  }    
  return 0;
}

int errCheck(S_REPORT_T *report, unsigned long addr)
{
  unsigned int crc24;
  unsigned long err_syndrome;
  int rc;
  unsigned char lowbit[12];
  unsigned char number, length;
  unsigned long bitmap;
  int len;
  int bitlen;
  unsigned char *buf;

  if(report->param0 & (1<<7))
    bitlen = 88; //long frame
  else
    bitlen = 32;  //short frame
  len = bitlen/8;
  
  buf = report->frame;

  //xor address
  if(addr)
  {  
    buf[len]   ^= (addr>>16)&0xff;
    buf[len+1] ^= (addr>>8)&0xff;
    buf[len+2] ^=  addr & 0xff;
  }
  
  //cal crc24
  crc24 = crctablefast(buf,len);
  
  //get syndrome
  err_syndrome = crc24 ^ ((buf[len]<<16)|(buf[len+1]<<8)|buf[len+2]);
  
  if (err_syndrome != 0) //try to correct error
  {
    int i;
    unsigned long b, b1;
    unsigned long syndr;
    char bit_offset;
    char t1, t2;
    unsigned int move_offset;

    rc = get_lowc_bitmap(len, report->conf, &number, &length, &bitmap, &lowbit[0]);
    if (rc == -1) //length<24 && number > 12 || length >24 && num>5
      return -1;
    else if (rc == -2) //length>24 && num<=5
      goto do_brute;

    //no low confidence bit, we can't do anything
    if (number == 0)
      return -1;

    //try conservative first
    syndr = err_syndrome;
    b = 0;
    //move first low confidence bit(from left) to bit89
    if (lowbit[0] < bitlen) //error syndrome is the error
    {
      move_offset = 0;
      //printf("syndr=%x (%u)\n",syndr,move_offset);
      for (i=0; i<(bitlen-lowbit[0]); i++)
      {
        syndr = SYNDROME_RIGHT_TRANSFORM(syndr, polynom|0x1000000);
        move_offset++;
        //printf("syndr=%x (%x)\n",syndr,move_offset);
      }

      for (i=0;i<number;i++)
      {
        unsigned char of;
        of = lowbit[i]-lowbit[0];
        b |= 1<<(23-of);
      }

      b1 = b;
      bit_offset = lowbit[0];

      if ((b1 & syndr) == syndr)
      {
        t1 = bit_offset/8;
        t2 = bit_offset%8;

        //correct relative
        buf[t1]   ^= (syndr >>(16+t2)) & 0xff;
        buf[t1+1] ^= (syndr >>(8+t2))&0xff;
        buf[t1+2] ^= (syndr >>t2) & 0xff;
        buf[t1+3] ^= (syndr <<(8-t2)) & 0xff;

        //recheck crc
        crc24 = crctablefast(buf,len+3);
        if (crc24 == 0)
          return 0;
      }
    }
    else
    {
      unsigned long bmap;
      unsigned char *cp = (unsigned char *)&bmap;

      for (i=0;i<number;i++)
      {
        b |= 1<<(lowbit[i]-bitlen);
      }
      bmap = b;
      cp[0] = reflect(cp[0],8);
      cp[1] = reflect(cp[1],8);
      cp[2] = reflect(cp[2],8);

      syndr = SWAP24(syndr);

      if ((bmap& syndr) == syndr)
      {
        buf[len] ^= (syndr>>0);
        buf[len+1] ^= (syndr>>8) & 0xff;
        buf[len+2] ^= (syndr>>16) & 0xff;

        crc24 = crctablefast(buf,len+3);
        if(crc24 == 0)
          return 0;
      }
    }

do_brute:
    //use brute force
    {
      unsigned char c, m;
      unsigned long t;
      int of[5];
      int cnt;

      for (i=1;i<(1<<number);i++)
      {
        t = err_syndrome;
        cnt = 0;
        for (m=0;m<number;m++)
        {
          c = i>>m;
          if (c&1)
          {
            int offset;
            offset = lowbit[m] - 2*(lowbit[m]%8) + 7;
            t ^= bitErrSynm[offset];
            of[cnt] = offset;
            cnt ++;
            if (t == 0)
              break;
          }
        }
        if (t == 0)
          break;
      }

      if (t == 0) //correct frame
      {
        for (i=0;i<cnt;i++)
        {
          int j,k;

          j = of[i]/8;
          k = of[i]%8;
          buf[j] ^= 1<<k;
        }

        crc24 = crctablefast(buf,len+3);
        if (crc24 == 0)
          goto frame_ok;
      }
    }
    return -1;
  }
  
frame_ok:
  return 0;
}


#if 0

#define TEST_ONE_BIT_ERROR
#define TEST_TWO_CONT_ERROR
#define TEST_TWO_CONT_ERROR_MORE_LCB
#define TEST_TWO_CONT_ERROR_CROSS_CRC_MORE_LCB
#define TEST_TWO_ERROR_BRUTE
#define  TEST_MORE_CONT_ERROR

unsigned char testbuf[14+14];
void test_error()
{
  int i;
  unsigned long crc24/*, crc24_2*/;
  int rc;
  unsigned char *lowc;
  unsigned long addr;

  init_crc();

  memset(testbuf, 0, sizeof(testbuf));

  for (i=0;i<11;i++)
    testbuf[i] = 0xff;
  
  crc24 = crctablefast(testbuf, 11);
  addr = get_src_address(testbuf);
  crc24 ^= addr;

  printf("crc24 is %x\n",crc24);

//  return;



  for (i=0;i<11;i++)
    testbuf[i] = i;

  crc24 = crctablefast(testbuf, 11);
  addr = get_src_address(testbuf);
  crc24 ^= addr;

  testbuf[11] = (crc24>>16)&0xff;
  testbuf[12] = (crc24>>8)&0xff;
  testbuf[13] = (crc24>>0)&0xff;


  lowc = &testbuf[14];

#ifdef TEST_ONE_BIT_ERROR
  printf("\n*********TEST_ONE_BIT_ERROR**********\n");
  for (i=0;i<112;i++)
  {
    lowc[i/8] |= 1<<(i%8);
    testbuf[i/8] ^=1<<(i%8);

    rc = errCheck(testbuf);
    if (rc)
    {
      printf("[%u]drop this buffer\n", i);
      testbuf[i/8] ^=1<<(i%8);
    }
    else
      printf("[%u]check success!\n", i);

    lowc[i/8] &= ~(1<<(i%8));

    addr = get_src_address(testbuf);
    testbuf[11] ^= (addr>>16)&0xff;
    testbuf[12] ^= (addr>>8)&0xff;
    testbuf[13] ^=  addr&0xff;

  }

#endif

#ifdef TEST_TWO_CONT_ERROR
  printf("\n*********TEST_TWO_CONT_ERROR**********\n");
  for (i=0;i<24;i++)
  {
    lowc[i/8] |= 1<<(i%8);
    testbuf[i/8] ^=1<<(i%8);

    lowc[i/8+1] |= 1<<(i%8);
    testbuf[i/8+1] ^=1<<(i%8);

    rc = errCheck(testbuf);
    if (rc)
    {
      printf("[%u]drop this buffer\n", i);
      testbuf[i/8] ^=1<<(i%8);
      testbuf[i/8+1] ^=1<<(i%8);
    }
    else
      printf("[%u]check success!\n", i);

    lowc[i/8] &= ~(1<<(i%8));
    lowc[i/8+1] &= ~(1<<(i%8));

    addr = get_src_address(testbuf);
    testbuf[11] ^= (addr>>16)&0xff;
    testbuf[12] ^= (addr>>8)&0xff;
    testbuf[13] ^=  addr&0xff;

  }


#endif

#ifdef TEST_MORE_CONT_ERROR
  printf("\n*********TEST_MORE_CONT_ERROR**********\n");
  for (i=0;i<24;i++)
  {
    lowc[i/8] |= 0xff;
    lowc[i/8+1] |= 0x0f;

    testbuf[i/8] ^=0xff;
   	testbuf[i/8+1] ^=0x0f;

    rc = errCheck(testbuf);
    if (rc)
    {
      printf("[%u]drop this buffer\n", i);
      testbuf[i/8] ^=0xff;
   	  testbuf[i/8+1] ^=0x0f;
    }
    else
      printf("[%u]check success!\n", i);

    lowc[i/8]   &= ~0xff;
    lowc[i/8+1] &= ~0x0f;

    addr = get_src_address(testbuf);
    testbuf[11] ^= (addr>>16)&0xff;
    testbuf[12] ^= (addr>>8)&0xff;
    testbuf[13] ^=  addr&0xff;

  }


#endif

#ifdef TEST_TWO_CONT_ERROR_MORE_LCB
  printf("\n*********TEST_TWO_CONT_ERROR_MORE_LCB**********\n");
  for (i=0;i<24;i++)
  {
    lowc[i/8] |= 1<<(i%8);
    lowc[i/8+1] |= 1<<(i%8);
    lowc[i/8+2] |= 1<<(i%8);

    testbuf[i/8+1] ^=1<<(i%8);
    testbuf[i/8+2] ^=1<<(i%8);

    rc = errCheck(testbuf);
    if (rc)
    {
      printf("[%u]drop this buffer\n", i);
      testbuf[i/8+1] ^=1<<(i%8);
      testbuf[i/8+2] ^=1<<(i%8);
    }
    else
      printf("[%u]check success!\n", i);

    lowc[i/8] &= ~(1<<(i%8));
    lowc[i/8+1] &= ~(1<<(i%8));
    lowc[i/8+2] &= ~(1<<(i%8));

    addr = get_src_address(testbuf);
    testbuf[11] ^= (addr>>16)&0xff;
    testbuf[12] ^= (addr>>8)&0xff;
    testbuf[13] ^=  addr&0xff;
  }

#endif

#ifdef TEST_TWO_CONT_ERROR_CROSS_CRC_MORE_LCB
  printf("\n*********TEST_TWO_CONT_ERROR_CROSS_CRC_MORE_LCB**********\n");
  lowc[10] |= 0x80;
//testbuf[10] ^=0x80;
  for (i=88;i<96;i++)
  {
    lowc[i/8] |= 1<<(i%8);
    lowc[i/8+1] |= 1<<(i%8);

    testbuf[i/8] ^=1<<(i%8);
    testbuf[i/8+1] ^=1<<(i%8);

    rc = errCheck(testbuf);
    if (rc)
    {
      printf("[%u]drop this buffer\n", i);
      testbuf[i/8] ^=1<<(i%8);
      testbuf[i/8+1] ^=1<<(i%8);

    }
    else
      printf("[%u]check success!\n", i);

    lowc[10] &= ~0x80;

    lowc[i/8] &= ~(1<<(i%8));
    lowc[i/8+1] &= ~(1<<(i%8));


    addr = get_src_address(testbuf);
    testbuf[11] ^= (addr>>16)&0xff;
    testbuf[12] ^= (addr>>8)&0xff;
    testbuf[13] ^=  addr&0xff;
  }

#endif


#ifdef TEST_TWO_ERROR_BRUTE
#define LAST 12
  printf("\n*********TEST_TWO_ERROR_BRUTE**********\n");
  lowc[0] |= 0x22;
  lowc[LAST] |= 0x42;
  lowc[LAST+1] |= 0x40;

  testbuf[0] ^=0x22;
  testbuf[LAST] ^=0x42;
  testbuf[LAST+1] ^=0x40;

  rc = errCheck(testbuf);
  if (rc)
  {
    printf("[%u]drop this buffer\n", LAST);
    testbuf[LAST+1] ^=0x40;
    testbuf[LAST] ^=0x42;
    testbuf[0] ^=0x22;

  }
  else
    printf("[%u]check success!\n", LAST);


  lowc[0] &= ~0x22;
  lowc[LAST] &= ~0x42;
  lowc[LAST+1] &= ~0x40;

  addr = get_src_address(testbuf);
  testbuf[11] ^= (addr>>16)&0xff;
  testbuf[12] ^= (addr>>8)&0xff;
  testbuf[13] ^=  addr&0xff;
#endif

}

#endif
