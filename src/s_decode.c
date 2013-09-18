/*********************
incoming sample data need set some state , the state include:
time stamp
frame head position(s)
data length

output frame like this:
|DB0|DB1|DB2|DB3|DB4|DB5|DB6|DB7|
|DB8|DB9|DB10|DB11|DB12|DB13|DB14|DB15|
.
.
.
|DB104|DB105|DB106|DB107|DB108|DB109|DB110|DB111|

so, BYTE0.B0=DB7, BYTE0.B7=DB0 and so on.
*/
#include <stdio.h>
#include <ecm.h>
#include <bcache.h>
#include "tcas.h"

//only for test
#define __TIME_TEST
//#undef __TIME_TEST
//#define __TEST_SIGNAL

#define __TEST_S_
#undef  __TEST_S_

extern INLINE_DESC void myprintf(char *format, ...);
extern INLINE_DESC void dmaprintf(char *format, ...);

#ifdef _DEBUG
char d_state_isr[MAX_POOL_NUM];
char d_frame_isr[MAX_POOL_NUM];
char d_frame_check[MAX_POOL_NUM];
#endif
  
#ifdef __TEST__

unsigned short fpga_state1[STATE_BUF_SIZE];
unsigned short fpga_state2[STATE_BUF_SIZE];
unsigned short fpga_data1[MAX_RECV_SIZE];
unsigned short fpga_data2[MAX_RECV_SIZE];
unsigned char  fpga_report[FRAME_OUT_LEN+HEAD_LENGTH];

void init_test_data();
void trigger_test(int gpio);
void init_fpga_data(char num, unsigned long long tm);

#endif

#define DB2     1.5849 //~= 7/12
#define DB3     2
#define DB6     4

static unsigned char ref_idx[12];
static unsigned int ref_cnt = 0;
static unsigned char cur_level_sum;
static unsigned char cur_level_dlt;
static unsigned char ref_level;     //sum ref level
static unsigned char ref_level_dlt; //dlt ref level
static unsigned char cur_rep_len;   
 

//fpga_data section from 0x10800000 to 0x10820000 (128k),
//#pragma DATA_ALIGN (sample_data, 128);
//#pragma DATA_ALIGN (sdataSt, 128);
//#pragma DATA_ALIGN (gConfidence, 128);
//#pragma DATA_ALIGN (gDataOut,    128);
//#pragma DATA_SECTION ( gConfidence ,  "fpga_data");

//wj : gDataOut changs to a pointer that point to gFrameout buffer
//#pragma DATA_SECTION ( gDataOut ,     "fpga_data");

/*
  describe the leading edge time for the first pulse
  there maybe four frame in the receive buffer
*/
//#pragma DATA_SECTION ( s_sample_data, "fpga_data");
#pragma DATA_SECTION ( sample_data , "fpga_data"); //fpga_data in L2SRAM
#pragma DATA_SECTION ( sample_data_dlt , "fpga_data"); //fpga_data in L2SRAM
#pragma DATA_SECTION ( sdataSt ,     "fpga_data");

unsigned short s_sample_data[MAX_POOL_NUM][MAX_BUF_IN_S_POOL][MAX_RECV_SIZE];
unsigned char sample_data[MAX_POOL_NUM][MAX_BUF_IN_S_POOL][MAX_RECV_SIZE];
unsigned char sample_data_dlt[MAX_POOL_NUM][MAX_BUF_IN_S_POOL][MAX_RECV_SIZE];
short sdataSt[MAX_POOL_NUM][MAX_BUF_IN_S_POOL][S_STATE_BUF_SIZE];

unsigned char sdata_rptr[MAX_POOL_NUM]; //read pointer for sample_data
volatile unsigned char sdata_wptr[MAX_POOL_NUM]; //write pointer for sample_data
volatile unsigned long gSCRCErrCnt=0;
volatile unsigned long gSRefLevelCnt=0;
volatile unsigned long gSReTrigerCnt=0;
volatile unsigned long gSDFCnt=0;
volatile unsigned long gSOverlapCnt=0;
volatile unsigned long gSConsistentCnt=0;
volatile unsigned long gSLenErrCnt=0;

/*
  output temp frame
*/
static S_REPORT_T gSFrameReport;

/*
  output frame buffer
*/
//#pragma DATA_ALIGN (gFrameOut,    128);
//#pragma DATA_SECTION ( gFrameOut ,  "report_data"); //report_data in DDR
//volatile char gFrameOut[MAX_REPORT_FRAME][HEAD_LENGTH + FRAME_OUT_LEN];

static char gGT;
static char gDltDirAvaliable;
static char gSumDirAvaliable;
static char gSumDir;
static char gJit;

void s_decode(int pool_id);
void cal_refer_level(unsigned char *sample_sum, unsigned char *sample_dlt);
char overlap_frame_check(unsigned char *sample);
int consistent_power_test(unsigned char *sample);
int df_valid(unsigned char *sample);
void multi_sample_baseline(unsigned char *sample, unsigned short level, unsigned int bit_len);
int check_dlt_ref_direction(unsigned char *sample_dlt);
int check_sum_ref_direction(unsigned char *sample_sum);
void get_ref_pulse(short *state);

long gSReportCnt=0;

extern volatile char g_s_sample_full[MAX_POOL_NUM];

unsigned char gDoCRCCheck;
unsigned char gCRCDone;
unsigned char gDoLowConfCheck;

unsigned long gLowConfCount;


#define RANGE_TO_DATA_OFFSET(range)  ((range+1)/2)

#define DO_CRC  0
#define TRY_CRC 1
#define NO_CRC  2

void S_decode_init()
{
  int i;

  memset(sample_data, 0, sizeof(sample_data));
  memset(sample_data_dlt, 0, sizeof(sample_data_dlt));
  memset(sdataSt, 0, sizeof(sdataSt));

  for(i=0;i<MAX_POOL_NUM;i++)
  {
    sdata_wptr[i] = 0;
  	sdata_rptr[i] = 0;
	g_s_sample_full[i] = 0;
  }

  gSFrameReport.head_delimit = 0x55aa;
  gSFrameReport.tail_delimit = 0xaa55;
}

void construct_S_report(short *state_ptr)
{
  long range;
  char mode;
  char t_b;
  char aq_bs;
  char bs_sum;
  char bs_dlt;
  char pat;
  char at;
  char omni;
  
  
  range = state_ptr[2]; //ST0[15:0]
  
  //param0 , param1
  mode  = state_ptr[0] & 0x7;    //ST2[2:0]
  if(mode == ACQUIRE_MODE) // aquiry mode
  	aq_bs = (state_ptr[0] >>8)&0x7;//ST2[10:8]
  else
    aq_bs = 0x7; // invalid direction

  bs_sum = gSumDir;
    
  if((bs_sum >=0) && (bs_sum <=3)) //use bs_sum and GT to get dlt direction   
  {
    char dlt_exist;
    dlt_exist = state_ptr[1]>>15;
    if((dlt_exist == 0)||(gDltDirAvaliable == 0)) //no dlt chnl or dlt direction unavaliable
      bs_dlt = 0x7; //111b means no dlt chnl
    else
    {  
      if((bs_sum == 0) || (bs_sum == 1)) //bs_sum is 0 or 180
      {  
        if(gGT == 0)
          bs_dlt = 3; //270
        else
          bs_dlt = 2; //90
      }
      else if((bs_sum == 2) || (bs_sum == 3)) //bs_sum is 90 or 270
      {  
        if(gGT == 0)
		  bs_dlt = 1; //180
        else
          bs_dlt = 0; //0
      }
      omni = 0; //direct
      t_b = UP_DIRECTION; //up
    } 
  }
  else if((bs_sum == 4) || (bs_sum == 5)) //up or down all direction
  {  
    bs_dlt = 0x7;  
    omni = 1; //all
    if(bs_sum == 4)
      t_b = UP_DIRECTION; //up
    else
      t_b = DOWN_DIRECTION; //down
  }
  else  //default value: only set bs_dlt and bs_sum to invalid value, app use these value to check direction valiable
  {
    bs_dlt = 0x7;
    bs_sum = 0x7;
    omni = 1; //all
    t_b = UP_DIRECTION;
  }

  pat = 0;     //todo
  at  = 0;     //todo
  
  gSFrameReport.param0 = 0;
  gSFrameReport.param1 = 0;
  gSFrameReport.param2 = 0;

  //set report
  gSFrameReport.range = range;
  gSFrameReport.param0 |=  ((mode&0x7)<<13) \
                         | ((aq_bs&0x7)<<10) \
                         | ((t_b&0x1)<<9) \
                         | ((omni&0x1)<<8) \
                         | ((cur_rep_len)<<7) \
                         | gCRCDone;
    
  gSFrameReport.param1 |= ((bs_sum&0x7)<<13) \
                        | ((bs_dlt&0x7)<<10) \
                        | ((pat&0x1f)<<5) \
                        | (at&0x1f);
                                              
  gSFrameReport.param2 = (cur_level_sum<<8) \
                        | ((cur_level_dlt<<1)&0xff);

}

//todo: use aa55 as tail flag
int check_tail(short st)
{
  if((st & 0xff00u) != 0xff00u)
    return 1; 
  else
    return 0;
}


//TODO: no more than 34 low confidence bit, and no more than 7 continue low confidence bits
int check_low_conf_consisten(int bit_len)
{
  int i,j,k;
  int cont = 0; //continue count

  if(gLowConfCount > 34) //no more than 34 low confidence bits
    return -1;
  else //no more than 7 continue low confidence bit
  {
    for(i=0;i<bit_len;i++)
	{
	   k = i/8;
       j = 7-i&7;
	   if(gSFrameReport.conf[k] & (1<<j))
	   {
	     cont ++;
		 if(cont>7)
		   return -1;
	   }
	   else 
	     cont = 0;
    }
  }
  return 0;
}

int do_frame_check(int pool_id, unsigned long sample_bit_len)
{
  int i;
  unsigned long stm;
  unsigned long first_stm;
  char rc;
  char jitter;
  long offset;
  long data_idx, state_idx;
  unsigned short datalen;
  char df_code;
  short *state_ptr;
  unsigned char *sum_data_ptr;
  unsigned char *dlt_data_ptr;
  unsigned char ridx;
  char mode;
  int cnt;
  unsigned long addr;
  short	patchcnt;
    
  //todo : need do some frame valid check????
  ridx = sdata_rptr[pool_id];
  state_ptr = &sdataSt[pool_id][ridx][0];
    
  datalen = state_ptr[DATA_LEN_POS];
  
  if(datalen == 0xffffu) //END FRAME
  {
    return MAX_PREAMBLE_NUMBER; //means end
  }  
  
  if(datalen > MAX_RECV_SIZE) //this is an overflow frame
  	return -2;
  else if(datalen < sample_bit_len) //this is an imcomplete frame
    return -2; 
  
  if (datalen < 1180)
	gSLenErrCnt++;
  if((state_ptr[0] & 0xff00u) == 0xff00u)  // AA section
  {
    myprintf("no preamble in buffer\n");
    return -2;
  }
  
  for(i=0;i<datalen;i++)
  {
    sample_data_dlt[pool_id][ridx][i] = s_sample_data[pool_id][ridx][i]>>8;
    sample_data[pool_id][ridx][i] = s_sample_data[pool_id][ridx][i] &0xff;
  }
  sum_data_ptr = &sample_data[pool_id][ridx][0];  
  dlt_data_ptr = &sample_data_dlt[pool_id][ridx][0];  
  
  //get first pulse range
  first_stm = ((state_ptr[1] & 0x7fff)<<16) + state_ptr[2];		//by fy
  
  cur_level_sum = 0;
  cur_level_dlt = 0;
  data_idx = -1;
  state_idx = -1;
  i=0; 
  cnt = 0;
  patchcnt = 0;
  
  //check for every possible preamble
  while(check_tail(state_ptr[i]))		//by fy
//  while (i<2)
  {
    cnt ++;
    if(cnt > MAX_PREAMBLE_NUMBER)
      break; //i can not do more check
    
    /*
      if two or more pulse have the leading edge time +/-1 to LE cal from first pulse,
      we need adjust the arrive time +/-1 sample
    */
    //adjust start time ST1[15:14]
    jitter = state_ptr[i]>>14;
    if(jitter == 0x1)
	  {
      gJit = -1;
	    if (((state_ptr[i]>>11)&0x7)==000)	//by fy
		    patchcnt++;
	  }
    else if(jitter == 0x2)
      gJit = 1;
  	else
      gJit = 0;
    
    stm = ((state_ptr[i+1] & 0x7fff)<<16) + state_ptr[i+2];		// by fy
    offset = RANGE_TO_DATA_OFFSET(stm-first_stm)+gJit ;
    
    if((offset + sample_bit_len + 8*S_SAMPLE_FREQ) > datalen /*MAX_RECV_SIZE*/)
    {
      myprintf("end of buffer at %u\n", ((unsigned int)i)/2);
      break; //todo: need frame's range is in-order
    }
        
    if((offset - data_idx) >= (sample_bit_len + 8*S_SAMPLE_FREQ))
    {
      myprintf("one frame is over, continue to next: %u\n",pool_id);
      if(state_idx == -1)
        goto do_next;
      break; //todo: now , in one buffer only process one frame. more frame is ignored
    }
    
    //get ref pulse
    get_ref_pulse(&state_ptr[i]);
    
    if(ref_cnt == 0) //no reference pulse
      goto do_next;
    
    //check ref pulse direction avaliable
    mode = state_ptr[i] & 0x7; //ST1[2:0]
    if(mode == ACQUIRE_MODE) //0x1
    {  
      gSumDirAvaliable = 1;
      gSumDir = (state_ptr[i]>>8)&0x7; //AQUIRY BS
      gDltDirAvaliable = check_dlt_ref_direction(&dlt_data_ptr[offset]);
    }
    else
    {  
      gSumDirAvaliable = check_sum_ref_direction(&sum_data_ptr[offset]);
      if(!gSumDirAvaliable) //no valide sum frame, skip this frame, check next
        goto do_next;
      gSumDir = (sum_data_ptr[offset]>>8)&0x7; //MONITOR BS
      gDltDirAvaliable = 0;
    }  
    
    //calculate reference power level
    //cal_refer_level(&sum_data_ptr[offset], &dlt_data_ptr[offset]);
    cal_refer_level(&sum_data_ptr[offset-gJit], &dlt_data_ptr[offset-gJit]);	//by fy
	  if ((cnt==0) && (ref_level<0x50))
	    gSRefLevelCnt++;
	  
	  //check preamble overlap
    //rc = overlap_frame_check(&sum_data_ptr[offset]);	  
    rc = overlap_frame_check(&sum_data_ptr[offset-gJit]);   //by fy
    if(rc == 1) //drop this preample
    {
	  gSOverlapCnt++;
      myprintf("preamble overlap failed\n");
      goto do_next;
    }
 
    //consisten power test
    //rc = consistent_power_test(&sum_data_ptr[offset]);
        rc = consistent_power_test(&sum_data_ptr[offset-gJit]);   //by fy
    if(rc == 1) //drop this preample
    {
	  gSConsistentCnt++;
      myprintf("power test failed\n");
 	  goto do_next;
    }
	
    //DF validation check
    //rc = df_valid(&sum_data_ptr[offset]);
    rc = df_valid(&sum_data_ptr[offset]);   //by fy
    if(rc == 1) //drop this preample
    {
	  gSDFCnt++;
      myprintf("DF validation failed\n");
      goto do_next;
    }

    //retrigger, determine which one is the current frame
    if( (state_idx == -1) 
    	||((ref_level - cur_level_sum) > 3*VALUE_DB1))
    {
	  if (cnt>1)
		gSReTrigerCnt++;
      cur_level_sum = ref_level;
      cur_level_dlt = ref_level_dlt;
      data_idx = offset;
      state_idx = i;
    }
  do_next:
    i += 3;
  
  } //while(1) for one possible frame

  if(state_idx == -1)
  {
  	myprintf("no valid frame in this buffer: %u\n",pool_id);
    return -1;
  }
  
  /*
    now , we have determin which frame is choosed to do follow check.
    note :at this time , we only check one frame, more frames need do more loop
  */
  
  //set report frame len
  if(sample_bit_len/S_SAMPLE_FREQ == SHORT_FRAME_LEN)
    gSFrameReport.param0 &= ~(1<<7);
  else
    gSFrameReport.param0 |= (1<<7);
    
  //Bit and Confidence Declaration
  multi_sample_baseline(&sum_data_ptr[data_idx], cur_level_sum, sample_bit_len/S_SAMPLE_FREQ);
   
  /*NOTE: in DF17 and DF11, error protect is PI , not AP, PI always zero in DF17, DF11 , so do nothing*/  
  mode = state_ptr[state_idx] & 0x7;
  df_code =  gSFrameReport.frame[0] >> 3; 
  
  gDoLowConfCheck = 0;
  gDoCRCCheck = 0;
  
  if(mode == LISTEN_MODE)
  {
    if((df_code == 11) || (df_code == 17))    /*0x11,0x17*/
	{
      gDoCRCCheck = DO_CRC;
	  addr = 0;
    }
    else if( (df_code == 0x0) || (df_code == 0x4))
	{
	  gDoCRCCheck = NO_CRC; 
      gDoLowConfCheck = 1;
    }
	else 
	  return -1; //drop
  }
  else if(mode == ACQUIRE_MODE)
  {
    if(df_code == 0x0)
	{
      gDoCRCCheck = TRY_CRC;
	  gDoLowConfCheck = 1;
//      addr = (state_ptr[i]&0xff)<<16 | state_ptr[i+1]; //TODO: USE 4061-4062
      addr = ((state_ptr[18]&0xff)<<16) | state_ptr[19];
	}
	else if(df_code == 0x4)
	{
      gDoCRCCheck = NO_CRC;
      gDoLowConfCheck = 1;
    }
	else if(df_code == 11)
	{
	  gDoCRCCheck = DO_CRC;
	  addr = 0;
	}
    else
	  return -1; //drop
  }

  else if(mode == CORDNATE_MODE)
  {
    if((df_code == 11) || (df_code == 17))
	{
      gDoCRCCheck = DO_CRC;
	  addr = 0;
    }
    else if((df_code == 0x0) || (df_code == 0x4))
	{
	  gDoCRCCheck = NO_CRC; 
      gDoLowConfCheck = 1;
    }
	else if(df_code == 16)
	{
	  gDoCRCCheck = TRY_CRC;
      addr = (state_ptr[i]&0xff)<<16 | state_ptr[i+1]; //TODO: USE 4061-4062
	}
	else 
	  return -1; //drop
    
  }
  else if(mode == BROADCAST_MODE)
  {
	if((df_code == 11) || (df_code == 17))
	{
      gDoCRCCheck = DO_CRC;
	  addr = 0;
    }
    else if((df_code == 0x0) || (df_code == 0x4))
	{
	  gDoCRCCheck = NO_CRC; 
      gDoLowConfCheck = 1;
    }
	else if(df_code == 16)
	{
	  gDoCRCCheck = NO_CRC;
	}
	else 
	  return -1; //drop
  }
  
  if(gDoLowConfCheck) //do continue low confidence check
  {
    if(check_low_conf_consisten(sample_bit_len/S_SAMPLE_FREQ)) 
      return -1;
  }


  rc = 0;
  if(gDoCRCCheck != NO_CRC)
  {
    rc = errCheck(&gSFrameReport, addr);
  }
  if(rc) //crc fail
  {
    if(gDoCRCCheck == DO_CRC)
	  {
	    gSCRCErrCnt++;
      myprintf("crc check failed! (%u)\n", pool_id);
      return -1;   //drop
	  }
	  else if(gDoCRCCheck == TRY_CRC)
	    gCRCDone = 0;
	  else
      gCRCDone = 0; //bug
  }
  else //crc success
  {
    if(gDoCRCCheck == NO_CRC)
      gCRCDone = 0;
    else
      gCRCDone = 1;
  }
  return state_idx;
}

//return the first buffer's pool id
int S_select_recv()
{
  unsigned char ridx0, ridx1;
  short *state_ptr0, *state_ptr1;
  static unsigned char cur_pool_id = 0;
  
  ridx0 = sdata_rptr[0];
  ridx1 = sdata_rptr[1];
  state_ptr0 = &sdataSt[0][ridx0][0];
  state_ptr1 = &sdataSt[1][ridx1][0];
  if( state_ptr0[DATA_LEN_POS] || state_ptr1[DATA_LEN_POS])
  {
    if(cur_pool_id == 0)
    {
      if(state_ptr0[DATA_LEN_POS])
      {
        cur_pool_id = 1;
        return 0;
      }  
      else if(state_ptr1[DATA_LEN_POS])
        return 1;
    }
    else if(cur_pool_id == 1)
    {
      if(state_ptr1[DATA_LEN_POS])
      {
        cur_pool_id = 0;
        return 1;
      }  
      else if(state_ptr0[DATA_LEN_POS])
        return 0;
    }
    else //bug
      return -1;
  }

  return -1; //no data in all sample data buffer
}

void s_decode(int pool_id)
{
  unsigned char ridx;
  short *state_ptr;
  int idx;
  char mode;
      
  ridx = sdata_rptr[pool_id];
  state_ptr = &sdataSt[pool_id][ridx][0];

  //only use frame0's state to determin current mode
  mode = state_ptr[0] &0x7;
  
  //check frame -----------by fy---------------
#ifndef __TEST_S_  
  if(mode == ACQUIRE_MODE) //acquire mode
  {
    idx = do_frame_check(pool_id, SHORT_FRAME_LEN*S_SAMPLE_FREQ);
    cur_rep_len = 0;
  }
  else if((mode == CORDNATE_MODE) || (mode == BROADCAST_MODE) || (mode == LISTEN_MODE))
  {
    idx = do_frame_check(pool_id, LONG_FRAME_LEN*S_SAMPLE_FREQ);
    cur_rep_len = 1;
  //else
  //{
    //idx = do_frame_check(pool_id, SHORT_FRAME_LEN*S_SAMPLE_FREQ);
	  if(idx < 0)
	  {
      idx = do_frame_check(pool_id, SHORT_FRAME_LEN*S_SAMPLE_FREQ);
      cur_rep_len = 0;
    }
  }
  //}
#else
//  idx = do_frame_check(pool_id, LONG_FRAME_LEN*S_SAMPLE_FREQ);
	idx = do_frame_check(pool_id, SHORT_FRAME_LEN*S_SAMPLE_FREQ);
#endif
    
  //send report
  if(idx >= 0)
  {
    if(idx == MAX_PREAMBLE_NUMBER)
    {
      send_END_to_CPU(mode);
      gSReportCnt ++;
    }  
    else
    {
      construct_S_report(&state_ptr[idx]);
      report_to_CPU((unsigned char *)&gSFrameReport);
      gSReportCnt ++;
    }  
  }
  
    
  //release this state buffer
  state_ptr[DATA_LEN_POS] = 0x0;
  
  //move sample data rptr to next
  ridx++;
  if(ridx >= MAX_BUF_IN_S_POOL)
    ridx = 0;
  sdata_rptr[pool_id] = ridx;

}

//check dlt bs consistent (only in aquiry stage)
int check_dlt_ref_direction(unsigned char *sample_dlt)
{
  char gt;
  int i;
  
  gt = sample_dlt[0]>>15; //bit15: gt
  for(i=1;i<ref_cnt;i++)
  {
    if(sample_dlt[i] >>15 != gt)
      break;
  }
  if(i == ref_cnt)
    return 1;
  else
    return 0;
}


//check sum bs consistent (only in monitor stage)
int check_sum_ref_direction(unsigned char *sample_sum)
{
  char sum_bs;
  int i;

  sum_bs = (sample_sum[0]>>8)&0x7; //bit8-10: bs
  for(i=1;i<ref_cnt;i++)
  {
    if(((sample_sum[i]>>8)&0x7) != sum_bs)
      break;
  }
  if(i == ref_cnt)
    return 1;
  else
    return 0;
}

void get_ref_pulse(short *state)
{
  int lead_miss;
 if (1)   //by fy
  {   
    ref_cnt = 0;
    lead_miss = (state[0]>>11) & 0x7; //st1(13:11)
    if((gJit != 0) && ((lead_miss & 0x7) == 0)) //0s miss
    {
      ref_idx[0] = 10+gJit+1;       //by fy
      ref_idx[1] = 11+gJit+1;
      ref_idx[2] = 12+gJit+1;

  	  ref_idx[3] = 35+gJit+1;
      ref_idx[4] = 36+gJit+1;
      ref_idx[5] = 37+gJit+1;

      ref_idx[6] = 45+gJit+1;
      ref_idx[7] = 46+gJit+1;
      ref_idx[8] = 47+gJit+1;

      ref_cnt = 9;
    }
  else
  {
 	//0us
    ref_idx[0] = 0+1;
  	ref_idx[1] = 1+1;
  	ref_idx[2] = 2+1;
  	ref_cnt = 3;

    if(0 == (lead_miss & 0x1)) //1us
    {
   	  ref_idx[ref_cnt++] = 10+gJit+1;
   	  ref_idx[ref_cnt++] = 11+gJit+1;
   	  ref_idx[ref_cnt++] = 12+gJit+1;
    }

 	  if(0 == (lead_miss & 0x2)) //3.5us
 	  {
      ref_idx[ref_cnt++] = 35+gJit+1;
   	  ref_idx[ref_cnt++] = 36+gJit+1;
   	  ref_idx[ref_cnt++] = 37+gJit+1;
 	  }
  
 	  if(0 == (lead_miss & 0x4)) //4.5us
 	  {
 	    ref_idx[ref_cnt++] = 45+gJit+1;
 	    ref_idx[ref_cnt++] = 46+gJit+1;
 	    ref_idx[ref_cnt++] = 47+gJit+1;
 	  }
  }   
}
  else
  {
  	ref_idx[0] = 1;   //by fy
  	ref_idx[1] = 2;
  	ref_idx[2] = 3;
  	
    ref_idx[3] = 11;
    ref_idx[4] = 12;
    ref_idx[5] = 13;

  	ref_idx[6] = 36;
    ref_idx[7] = 37;
    ref_idx[8] = 38;

    ref_idx[9] = 46;
    ref_idx[10] = 47;
    ref_idx[11] = 48;

    ref_cnt = 12;
  }
}


/*
1. only use pulse which have leading edge
2. each such pulse only use first three samples after leading edge
3. calculate all number for every sample
4. if max number is unique, use it. if not do 5
5. drop all samples whose max count less than max, in remain samples, cal min val, drop all samples that more than 2DB stronger than min
*/
//CAL SUM AND DLT REFER LEVEL
void cal_refer_level(unsigned char *sample_sum, unsigned char *sample_dlt)
{
  int i, j;
  unsigned char v_sum, v_dlt;
  unsigned long level_sum, level_dlt;
  unsigned char refnum[12];
  unsigned char maxnum;
  unsigned char refmax_sum[12], refmax_dlt[12];
  unsigned char refmin;
  unsigned char unique;
  unsigned char mincnt;
  
  maxnum = 0;
  unique = 100;
  refmin = 0xff;

  for(i=0; i<ref_cnt; i++)
  {
    v_sum = sample_sum[ref_idx[i]];
    v_dlt = sample_dlt[ref_idx[i]]&0x7f;
    refnum[i] = 0;
    
    for(j=0; j<ref_cnt; j++)
    {
      if(i == j)
        continue;
      if(_abs_diff(sample_sum[ref_idx[j]], v_sum) <= 2*VALUE_DB1)// within 2db
       refnum[i] ++;
    }
    
    if(refnum[i])
    {
      if(refnum[i] == maxnum)
      {
        unique++;
        refmax_sum[unique] = v_sum;
        refmax_dlt[unique] = v_dlt;
      }
      else if(refnum[i] > maxnum)
      {
        maxnum = refnum[i];
        unique = 0;
        refmax_sum[0] = v_sum;
        refmax_dlt[0] = v_dlt;
      }
    }
  }
  
  if(unique == 100)
    return;
    
  if(unique == 0) //max is unique
  {
    ref_level = refmax_sum[0];
    ref_level_dlt = refmax_dlt[0];
    return;
  }
  
  //check within 2db for min val
  mincnt = 0;
  level_sum = 0;
  level_dlt = 0;

  for(i=0;i<unique+1;i++)
  {
    if(refmax_sum[i] < refmin)
      refmin = refmax_sum[i];
  }

  for(i=0; i<unique+1; i++)
  {
    v_sum = refmax_sum[i];
    v_dlt = refmax_dlt[i];
  
    if(_diff(v_sum,refmin) <= 2*VALUE_DB1) //WITHIN 2db
    {
      mincnt++;
      level_sum += v_sum;
      level_dlt += v_dlt;
    }
  }
  ref_level = level_sum/mincnt;
  ref_level_dlt = level_dlt/mincnt;
}

/*for 1use, 3.5us, 4.5us check, and do frame-retrigger*/
/*T=0 means leading edge+1 sample*/
/*compare two group, if later is -3DB or weaker than first, reject later*/
/*return 0 means save this preamble, return 1 means drop this preamble*/
char overlap_frame_check(unsigned char *sample)
{
  short minval;
  short maxval;
  short v;
  
  /*1us check: min(T=1.0, 2.0, 4.5, 5.5) compare to max(T=0, 3.5)*/
  minval = sample[10+gJit];
  v = sample[20+gJit];
  if(v < minval)
    minval = v;

  v = sample[45+gJit];
  if(v < minval)
    minval = v;

  v = sample[55+gJit];
  if(v < minval)
    minval = v;
  
  maxval = sample[0];

  v = sample[35+gJit];
  if(maxval < v)
    maxval = v;
  
  if((minval - maxval) > 3*VALUE_DB1)
    return 1;
  
  /*3.5us check:  min(T=3.5, 4.5, 7.0) compare to max(T=0, 1.0)*/
  minval = sample[35+gJit];

  v = sample[45+gJit];
  if(v < minval)
    minval = v;

  v = sample[70+gJit];
  if(v < minval)
    minval = v;
  
  maxval = sample[0];
  v = sample[10+gJit];
  if(maxval < v)
    maxval = v;
  
  if((minval - maxval)> 3*VALUE_DB1)
    return 1;
  
  /*4.5us check: min(T= 4.5, 5.5, 8.0, 9.0) compare to max(T = 0, 1.0, and 3.5)*/
  //GET MIN
  minval = sample[45+gJit];

  v = sample[55+gJit];
  if(v < minval)
    minval = v;

  v = sample[80+gJit];
  if(v < minval)
    minval = v;

  v = sample[90+gJit];
  if(v < minval)
    minval = v;

  //GET MAX  
  maxval = sample[0];
  v = sample[10+gJit];
  if(maxval < v)
    maxval = v;

  v = sample[35+gJit];
  if(maxval < v)
    maxval = v;
  
  //COMPARE
  if((minval - maxval) > 3*VALUE_DB1)
    return 1;

  return 0;
}


/*check at least two of four pulse agree in power level with RL to within +/-3DB, if not drop this preamble*/
/*should check every sample of each pulse if need, in one pulse we only check fisrt 5 sample */
/*return 0 means consistent, 1 means not*/
int consistent_power_test(unsigned char *sample)
{
  char cnt=0;
  unsigned long v;
//todo : average in all samples when sample freq changes

  //check 0us
  //v = (sample[0]+sample[1]+sample[2]+sample[3]+sample[4])/5;
  v = (sample[1]+sample[2]+sample[3])/3;  
  if(_abs_diff(v, ref_level) < 3*VALUE_DB1)
    cnt ++;
    
  //check 1us
  //v = (sample[10+gJit]+sample[11+gJit]+sample[12+gJit]+sample[13+gJit]+sample[14+gJit])/5;
  v = (sample[11+gJit]+sample[12+gJit]+sample[13+gJit])/3;  
  if(_abs_diff(v, ref_level) < 3*VALUE_DB1)
    cnt ++;
  
  //check 3.5us
  //v = (sample[35+gJit]+sample[36+gJit]+sample[37+gJit]+sample[38+gJit]+sample[39+gJit])/5;
  v = (sample[36+gJit]+sample[37+gJit]+sample[38+gJit])/3;  
  if(_abs_diff(v, ref_level) < 3*VALUE_DB1)
    cnt ++;
  
  //check 4.5us
  //v = (sample[45+gJit]+sample[46+gJit]+sample[47+gJit]+sample[48+gJit]+sample[49+gJit])/5;
    v = (sample[46+gJit]+sample[47+gJit]+sample[48+gJit])/3;
  if(_abs_diff(v, ref_level) < 3*VALUE_DB1)
    cnt ++;
  
  if(cnt <= 2)
    return 1;
  
  return 0;
}

/*check first five data power level*/
/*
1. in each chip'sturn 1;
  
  return 0;
}

/*check first five data power level*/
/*
1. in each chip's leading edge time +/-1 sample, whether it is a valid pulse position
2. and , check each bit's two chip , whether it have pulse in these chip(first, second or both)
3. and , peak amplitude of pulse is equal or greater than 6db below RL
note: peak amplitude is the highest sample in M samples which comprise the pulse
*/
int df_valid(unsigned char *sample)
{
  int i;
  int j;
  unsigned char s1,s2,s3,s4,s5,s6;
  int idx;
  //char p[5]={0,0,0,0,0};
  unsigned char maxpluse;
  unsigned char nopulse;
  
  //check valide pulse position
  for(i=0;i<5;i++)
  {
  	nopulse = 0;
    for(j=0;j<2;j++)
    {
      idx = 80 + i*10 + j*5;
      s2 = sample[idx];
      s3 = sample[idx + 1];
      s4 = sample[idx + 2];
      s5 = sample[idx + 3];
      
      if((s2>DATA_THRESHOLD) && (s3>DATA_THRESHOLD) && (s4>DATA_THRESHOLD) && (s5>DATA_THRESHOLD)) //check 0
      {
        maxpluse = getMax4(s2,s3,s4,s5);
        if ((maxpluse< ref_level) && ((ref_level- maxpluse) > 6*VALUE_DB1))
          //return 1; //todo : maybe check +1 and -1???		--by fy
		{
			if (j==0)
				nopulse = 1;
			else if (nopulse==1)
				return 1;	
		}
        break;
      }
      else
      {
        if((s2<=DATA_THRESHOLD) && (s3<=DATA_THRESHOLD) && (s4<=DATA_THRESHOLD) && (s5<=DATA_THRESHOLD))
         //continue;		-- by fy
		{
			if (j==0)
				nopulse = 1;
			else if (nopulse==1)
				return 1;	
		}
        else
        {
          s1 = sample[idx - 1];
          if((s1>DATA_THRESHOLD) && (s2>DATA_THRESHOLD) && (s3>DATA_THRESHOLD) && (s4>DATA_THRESHOLD)) //check -1
          {
            maxpluse = getMax4(s1,s2,s3,s4);
            if ((maxpluse< ref_level) && ((ref_level- maxpluse) > 6*VALUE_DB1))
			{
				if (j==0)
					nopulse = 1;
				else if (nopulse==1)
					return 1;	
			}
            break;
          }
          else
          {
            if((s1<=DATA_THRESHOLD) && (s2<=DATA_THRESHOLD) && (s3<=DATA_THRESHOLD) && (s4<=DATA_THRESHOLD))
              continue;
            s6 = sample[idx + 4];
            if((s3>DATA_THRESHOLD) && (s4>DATA_THRESHOLD) && (s5>DATA_THRESHOLD) && (s6>DATA_THRESHOLD)) //check +1
            {
              maxpluse = getMax4(s3,s4,s5,s6);
              if ((maxpluse< ref_level) && ((ref_level- maxpluse) > 6*VALUE_DB1))
			  {
				if (j==0)
					nopulse = 1;
				else if (nopulse==1)
					return 1;	
			  }
              break;
            }
            /*else if((s3<=DATA_THRESHOLD) && (s4<=DATA_THRESHOLD) && (s5<=DATA_THRESHOLD) && (s6<=DATA_THRESHOLD))
              continue;
            */
          }
        }
       }
    }//for j
    if(j == 2)
      return 1; //no puls in both chip
  }//for i
  
  return 0;
}


/*Bit and Confidence Declaration */

/*central amplitude method is for Class A1*/
/*
  if only one chip center sample in the threshold of preamble level (+/-3DB), this one is value, this slot is high confidence
  if both in/out the threshold, high is the value, this slot is low confidence
  in our design , center is the third sample in one chip
*/
void central_amplitude()
{
  
}

/*for Class A2 & A3*/
/*
two categorized:
A: within the +/- 3 dB preamble window
B: below threshold (6 dB or more below the preamble)

calculate the sample number in each categorize (first and last weight 1, intermedia weight 2):
1ChipTypeA = #of weighted samples in the 1 chip of type A (Match Preamble)
1ChipTypeB = #of weighted samples in the 1 chip of type B (Lack energy)
0ChipTypeA = #of weighted samples in the 0 chip of type A (Match Preamble)
0ChipTypeB = #of weighted samples in the 0 chip of type B (Lack Energy)

produce two scores:
1Score = 1ChipTypeA 每 0ChipTypeA + 0ChipTypeB 每 1ChipTypeB
0Score = 0ChipTypeA 每 1ChipTypeA + 1ChipTypeB 每 0ChipTypeB

The highest score determines the bit value, if equal, default is zero
if difference >=3 , it is high confidence. otherwise  it is low confidence
*/
void multi_sample_baseline(unsigned char *sample, unsigned short level, unsigned int bit_len)
{
  unsigned long _1ChipTypeA=0;
  unsigned long _1ChipTypeB=0;
  unsigned long _0ChipTypeA=0;
  unsigned long _0ChipTypeB=0;
  unsigned long _1Score;
  unsigned long _0Score;
  long diff;
  unsigned char p;
  int j;
  int k;
  char _index;
  char _bit;
  
  gLowConfCount =0;

  //check from 8 to frame_len(56 or 112)
  for(j=8; j<bit_len+8; j++)
  {
	  _1ChipTypeB = 0;
	  _1ChipTypeA = 0;
	  _0ChipTypeB = 0;
	  _0ChipTypeA = 0;
    k = j*10;
    
	//CHIP 1
    p = sample[0+k];
    if(_abs_diff(p, level) <= 3*VALUE_DB1)
      _1ChipTypeA ++;
    else if(_diff(level,p) > 6*VALUE_DB1) //type B
      _1ChipTypeB ++;
       
    p = sample[1+k];
    if(_abs_diff(p, level) <= 3*VALUE_DB1)
      _1ChipTypeA ++;
    else if(_diff(level,p) > 6*VALUE_DB1) //type B
      _1ChipTypeB ++;
         
    p = sample[2+k];
	  if(_abs_diff(p, level) <= 3*VALUE_DB1)
      _1ChipTypeA ++;
    else if(_diff(level,p) > 6*VALUE_DB1) //type B
      _1ChipTypeB ++;
        
    p = sample[3+k];
	  if(_abs_diff(p, level) <= 3*VALUE_DB1)
      _1ChipTypeA ++;
    else if(_diff(level,p) > 6*VALUE_DB1) //type B
      _1ChipTypeB ++;
        
    p = sample[4+k];
	  if(_abs_diff(p, level) <= 3*VALUE_DB1)
      _1ChipTypeA ++;
    else if(_diff(level,p) > 6*VALUE_DB1) //type B
      _1ChipTypeB ++;
    
    //CHIP 0  
    p = sample[5+k];
    if(_abs_diff(p, level) <= 3*VALUE_DB1)
      _0ChipTypeA ++;
    else if(_diff(level,p) > 6*VALUE_DB1) //type B
      _0ChipTypeB ++;
      
    p = sample[6+k];
	  if(_abs_diff(p, level) <= 3*VALUE_DB1)
      _0ChipTypeA ++;
    else if(_diff(level,p) > 6*VALUE_DB1) //type B
      _0ChipTypeB ++;
      
    p = sample[7+k];
	  if(_abs_diff(p, level) <= 3*VALUE_DB1)
      _0ChipTypeA ++;
    else if(_diff(level,p) > 6*VALUE_DB1) //type B
      _0ChipTypeB ++;
      
    p = sample[8+k];
	  if(_abs_diff(p, level) <= 3*VALUE_DB1)
      _0ChipTypeA ++;
    else if(_diff(level,p) > 6*VALUE_DB1) //type B
      _0ChipTypeB ++;
      
    p = sample[9+k];
	  if(_abs_diff(p, level) <= 3*VALUE_DB1)
      _0ChipTypeA ++;
    else if(_diff(level,p) > 6*VALUE_DB1) //type B
      _0ChipTypeB ++;
      
    _1Score = _1ChipTypeA - _0ChipTypeA + _0ChipTypeB - _1ChipTypeB;
    _0Score = _0ChipTypeA - _1ChipTypeA + _1ChipTypeB - _0ChipTypeB;
    
    diff = _1Score - _0Score;
    _index = (j-8)/8;
    _bit = 7 - (j-8)&0x7;  //NOTE: frame's byte is like this : |DB0|DB1|...|DB6|DB7|  ,so , DB7 in B0,and, DB0 in B7
    
    //set confidence
    if((diff >= 3) || (diff <= -3))
      gSFrameReport.conf[_index] &= ~(1<<_bit); /*HIGH_CONFIDENCE*/
    else
	{
      gSFrameReport.conf[_index] |= 1<<_bit;    /*LOW_CONFIDENCE*/
	  gLowConfCount ++;
	}
    
    //set bit value
    if(diff <= 0)
      gSFrameReport.frame[_index] &= ~(1<<_bit);
    else
      gSFrameReport.frame[_index] |= 1<<_bit;
  }
}


#ifdef __TEST__

void init_fpga_data(char num, unsigned long long tm)
{
  if(num == 0)
  {
  fpga_state1[0]=  0x0;//ST2
  fpga_state1[1]=  (tm >>32);//ST1
  fpga_state1[2]=  tm &0xffff;//ST0
  fpga_state1[3]=  0xaa55;
  fpga_state1[DATA_LEN_POS] = 1200;

  }
  else
  {
  fpga_state2[0]=  0x0;//ST2
  fpga_state2[1]=  (tm >>32);//ST1
  fpga_state2[2]=  tm &0xffff;//ST0
  fpga_state2[3]=  0xaa55;
  fpga_state2[DATA_LEN_POS] = 1200;
  }

}
void init_test_data()
{
  int i;
  int v;
  int cnt;

  memset(fpga_report, 0, sizeof(fpga_report));
  
  //init data
  memset(fpga_data1, 0, sizeof(fpga_data1));
  for(i=0;i<5;i++)
    fpga_data1[i] = 50;
  for(i=10;i<15;i++)
    fpga_data1[i] = 45;
  for(i=35;i<40;i++)
    fpga_data1[i] = 55;
  for(i=45;i<50;i++)
    fpga_data1[i] = 52;
  
  v = 0;
  cnt = 0;
  for(i=80;i<800;i++)
  {
    if((cnt%5) == 0)
      v = 1 - v;
    fpga_data1[i] = 50*v;
    cnt ++;
  }
  v = 1;
  cnt = 0;
  for(i=800;i<1200;i++)
  {
    if(cnt%5 == 0)
      v = 1 - v;
    fpga_data1[i] = 50*v;it state
  init_fpga_data(0,0);
  init_fpga_data(0,1);
    
  
  
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
  
}

void trigger_test(int gpio)
{
  int i;

  EVM6424_GPIO_setOutput(gpio, 1);
  for(i=0;i<5000;i++);
  EVM6424_GPIO_setOutput(gpio, 0);
  for(i=0;i<5000;i++);
  
}
#endif






