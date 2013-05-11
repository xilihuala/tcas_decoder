/* c_decode.c */
#include <stdio.h>
#include "tcas.h"

typedef struct _c_frame {
  char idx;
  unsigned char sum_ref_value;
  unsigned char dlt_ref_value;
  char ref_oba;
} t_c_frame;
static C_REPORT_T gCFrameReport;

unsigned char cdata_rptr[MAX_POOL_NUM]; //read pointer for data
volatile unsigned char cdata_wptr[MAX_POOL_NUM]; //write pointer for data

unsigned short cdataSt[MAX_POOL_NUM][MAX_BUF_IN_C_POOL][C_STATE_BUF_SIZE];
unsigned short c_sample_data[MAX_POOL_NUM][MAX_BUF_IN_C_POOL][MAX_RECV_SIZE];

t_c_frame cframe[MAX_CNT_IN_ONE_CSTEP];
unsigned char v_cnt;

char overlap[MAX_CNT_IN_ONE_CSTEP];
char spi_flag[MAX_CNT_IN_ONE_CSTEP];

short gBitValue;
short gConf;

char gSumRef;
char gDltRef;

void C_decode_init()
{
  int i;
  
  memset(spi_flag, 0, MAX_CNT_IN_ONE_CSTEP);
  memset(overlap, 0, MAX_CNT_IN_ONE_CSTEP);
  
  memset(c_sample_data, 0, sizeof(c_sample_data));
  memset(cdataSt, 0, sizeof(cdataSt));
  
  for(i=0;i<MAX_POOL_NUM;i++)
  {
    cdata_wptr[i] = 0;
  	cdata_rptr[i] = 0;
  }
  
  gCFrameReport.head_delimit = 0x55aa;
  gCFrameReport.tail_delimit = 0xaa55;
  
  gCFrameReport.resv[0] = 0xaaaa;
  gCFrameReport.resv[1] = 0xaaaa;
  gCFrameReport.resv[2] = 0xaaaa;
  gCFrameReport.resv[3] = 0xaaaa;
  gCFrameReport.resv[4] = 0xaaaa;
}

void construct_C_report(unsigned short *data)
{ 
  char spi;
  char omni;
  char t_b;
  char bs;

/*  
  if((data[19]>>8) > DATA_THRESHOLD)
    spi = 1;
  else
    spi = 0;
*/
  spi = 0;
    
  bs = (data[2]>>10) & 0x7;
  if((bs == 4) | (bs == 5))
    omni = 1; //FULL
  else
    omni = 0; //DIRECT
  
  if(bs == 5)
    t_b = 0;
  else
    t_b = 1;
    
  gCFrameReport.range       = data[0];
  gCFrameReport.param0      = data[2] | (t_b<<9) | (omni<<8) | (spi<<7) ;
  gCFrameReport.param1      = data[3];
  gCFrameReport.param2      = ((gSumRef&0xff)<<8) | (gDltRef&0xff);
  gCFrameReport.code        = gBitValue<<1;
  gCFrameReport.confidence  = gConf<<1;
}

int C_select_recv()
{ 
  unsigned char ridx0, ridx1;
  unsigned short *state_ptr0, *state_ptr1;
  static unsigned char cur_pool_id = 0;
  
  ridx0 = cdata_rptr[0];
  ridx1 = cdata_rptr[1];
  state_ptr0 = &cdataSt[0][ridx0][0];
  state_ptr1 = &cdataSt[1][ridx1][0];
  if( state_ptr0[C_DATA_LEN_POS] || state_ptr1[C_DATA_LEN_POS])
  {
    if(cur_pool_id == 0)
    {
      if(state_ptr0[C_DATA_LEN_POS])
      {
        cur_pool_id = 1;
        return 0;
      }  
      else if(state_ptr1[C_DATA_LEN_POS])
        return 1;
    }
    else if(cur_pool_id == 1)
    {
      if(state_ptr1[C_DATA_LEN_POS])
      {
        cur_pool_id = 0;
        return 1;
      }  
      else if(state_ptr0[C_DATA_LEN_POS])
        return 0;
    }
    else //bug
      return -1;
  }

  return -1; //no data in all sample data buffer
}

/*
use 20M sample frequence (50us), 
cycle=1.45us = 29 * clcok, 
one frame have 17 pulse (include SPI)  
overlap check only use 14 pulse (= 406 * clock) ,  +/-0.1us (+/-2 clock)
*/
#define AMP_RELATIVE(v1, v2, thr)  (_abs_diff(v1, v2) < thr)
#define OBA_RELATIVE(v1, v2, thr)  (_abs_diff(v1, v2) < thr)
#define GET_OBA(sum, dlt) ((sum)-(dlt))

void c_decode(int pool_id)  
{
  unsigned char ridx;
  unsigned short *state_ptr;
  unsigned short *data_ptr;
  unsigned int frame_cnt;
  unsigned short f1_range;
  unsigned short f2_range;
  unsigned short f1_next_range;
  unsigned short f2_next_range;
  unsigned short f2_value;
  unsigned short f2_next_value;
  unsigned long v;    
  int i;
  char bit;
  char idx;
  unsigned short sum_ref, dlt_ref;
  unsigned short *data;
  char f1_gt, f2_gt, f2_next_gt;
  unsigned char ref_cnt;
  unsigned char mode;
  
  ridx = cdata_rptr[pool_id];
  state_ptr = &cdataSt[pool_id][ridx][0];
  if((*state_ptr) == 0) //no frame in this buffer , goto next
    goto _c_do_next;
    
  frame_cnt = (*state_ptr)/ONE_C_FRAME_SIZE;
  if(frame_cnt == 0)
    return;
  myprintf("ridx=%u\n", ridx);
  data_ptr = &c_sample_data[pool_id][ridx][0];
  
  v_cnt = 0;
  memset(spi_flag, 0, MAX_CNT_IN_ONE_CSTEP);
  memset(overlap, 0, MAX_CNT_IN_ONE_CSTEP);
   
  /*****************************/
  /* PROCESS FRAMES IN ONE STEP*/
  /*****************************/
  
  /******************************************/
  /*      STEP 1: check phantom frame       */
  /******************************************/
  for(i=0;i<frame_cnt;i++)
  {
    int j;
	char ref;
	char f1_overlap;
    char spi;
    char check_ok;

    spi = spi_flag[i]; 
    if(spi) //this frame have been set as C2-SPI frame. maybe it is a valid frame, but it relative to the special overlap frame, so we drop it
      continue;
    
    ref = 0; //USE F1 as ref
    data = &data_ptr[i*ONE_C_FRAME_SIZE];
    f1_range = data[0];
    f2_range = f1_range + 406;
    f2_value = data[17];
    f2_gt = (data[1] >> 1) &0x1;
    check_ok = 1;
    f1_overlap = overlap[i];
    if(f1_overlap)
    {
      ref = 1; //use F2 as ref
      
      //check whether F2 is overlapped
      j = i+1;   
      if(j >= frame_cnt)
        break;
      
      for(;j<frame_cnt;j++)
      {
        data = &data_ptr[j*ONE_C_FRAME_SIZE];
        f1_next_range = data[0];
        f2_next_range = f1_next_range + 406; 
        f2_next_value = data[17];    
        f2_next_gt = (data[1] >> 1) &0x1;    
        
        if(f1_next_range <= f2_range)
        {
          //set F1 overlap flag
          v = (f2_range - f1_next_range)%29;
          if((v<=2) || (v>=27)) //overlap on next F1
          	overlap[i] = 1;
          
          //check f2 overlap
          if(f2_range < f2_next_range)
          {
            v = (f2_next_range - f2_range)%29;
            if((v<=2) || (v>=27)) //overlap on this F2
            {
              //check sum and dlt consistent(use F2), check BS consistent
              if( AMP_RELATIVE(f2_value&0xff, f2_next_value&0xff,DLT_DIFF_THRESHOLD) \
                && AMP_RELATIVE(f2_value>>8, f2_next_value>>8, SUM_DIFF_THRESHOLD)
                && (f2_gt == f2_next_gt))
              {  
                char pos;
                
                check_ok = 0;
                //check C2-SPI
                pos = (f1_next_range - f1_range)/29;
                v = (f1_next_range - f1_range)%29;
                if(!((pos == 3) && ((v<=2) || (v>=27)))) //overlap on next F1
                {
                  spi_flag[j] = 1;
                  break;
                }
              }
            }
          }
        }
        else
          break;
      } //for
    } //if
    
    if(check_ok == 0)
    	continue;
    	
    //calculate reference value
    data = &data_ptr[i * ONE_C_FRAME_SIZE];
    f1_gt = (data[1] >> 14) & 0x1;
    f2_gt = (data[1] >> 1) &0x1;
    sum_ref = (data[4] >>8) + (data[17]>>8);
    dlt_ref = (data[4] & 0xff) + (data[17] & 0xff);
    ref_cnt = 2;
    for(bit=2;bit<14;bit++)
    {
      char gt;
      char b_sum_val, b_dlt_val;

      gt = (data[1] >> bit) & 0x1;
      b_sum_val = data[18-bit]>>8;
      b_dlt_val = data[18-bit] & 0xff; 
      if( ((ref == 0) && (gt == f1_gt) && (b_sum_val >= DATA_THRESHOLD))
        ||((ref == 1) && (gt == f2_gt) && (b_sum_val >= DATA_THRESHOLD)))
      {  
        sum_ref += b_sum_val;
        dlt_ref += b_dlt_val;
        ref_cnt ++;
      }
    }  
    
    cframe[v_cnt].sum_ref_value = sum_ref /ref_cnt;
    cframe[v_cnt].dlt_ref_value = dlt_ref /ref_cnt;
    if(ref == 0) //USE F1
      cframe[v_cnt].ref_oba = GET_OBA(data[4] >>8,data[4] & 0xff);
    else //USE F2
      cframe[v_cnt].ref_oba = GET_OBA(data[17] >>8, data[17] & 0xff);
    cframe[v_cnt].idx = i;  
    
    v_cnt ++;  
  } //for
  
  
  /******************************************/
  /*  STEP 2:                               */
  /*    1) set bit value and confidence     */
  /*    2) send report to CPU board         */
  /******************************************/
  for(i=0;i<v_cnt;i++)
  {
    unsigned short bit_value;
    unsigned short conf;
    unsigned short bit_pos;
    unsigned short bit_amp; 
    unsigned short bit_sum_amp,bit_dlt_amp;
    unsigned char bit_gt;
    unsigned char bit_oba;
    int k;
    unsigned char other_amp_relative, other_oba_relative;
    unsigned char my_amp_relative, my_oba_relative;
    unsigned char ref_oba;
        
    //set bit value and confidence
    idx = cframe[i].idx;
    data = &data_ptr[idx*ONE_C_FRAME_SIZE];
    sum_ref = cframe[i].sum_ref_value;
    dlt_ref = cframe[i].dlt_ref_value;
    ref_oba = cframe[i].ref_oba;
    mode = data[2]>>13;
    f2_gt = (data[1] >> 1) &0x1;
    conf = 1<<15; //bit15 default set to 1, bit0 default set to 0
    bit_value =0;
       
    for(bit=0;bit<14;bit++) //set 14 bit value and confidence, bit0 - F1, bit14 - F2
    {
      f1_range = data[0];
      bit_pos =f1_range + 29*bit;
      bit_amp = data[4 + bit];
      bit_sum_amp = bit_amp>>8;
      bit_dlt_amp = bit_amp & 0xff;
      bit_gt = (data[1]>>(14-bit)) & 0x1;
      bit_oba = GET_OBA(bit_sum_amp,bit_dlt_amp);
      
      other_amp_relative = 0;
      other_oba_relative = 0;
           
      //check prev frame relation
      for(k=i-1;k>0;k--)
      {
        unsigned short prev_f2_range;
        unsigned char prev_f2_gt;
        
        data = &data_ptr[cframe[k].idx * ONE_C_FRAME_SIZE];
        prev_f2_range = data[0] + 406;
        prev_f2_gt = (data[1]>>1) & 0x1;
                
        //check frame range
        if(prev_f2_range < bit_pos)
					break;
        //check overlap  
        v = (prev_f2_range - bit_pos)%29;
        if((v>2) && (v<27)) //frame not overlap
          continue;
                 
        //check amp relative
        if(   (bit_sum_amp >= DATA_THRESHOLD)  \
           && AMP_RELATIVE(bit_sum_amp, cframe[k].sum_ref_value, SUM_DIFF_THRESHOLD)) //L1 & L0
        {  
          other_amp_relative = 1;
          if(other_oba_relative == 1)
            break;
        }
        
        //check oba relative
        if(   OBA_RELATIVE(bit_oba, cframe[k].ref_oba, OBA_THRESHOLD) \
           && (bit_gt == prev_f2_gt)) //Low confidence
        {
          other_oba_relative = 1;
          if(other_amp_relative == 1)
            break;
        }
      }//for
      
      //check next frame relation
      if((other_oba_relative == 0) || (other_amp_relative == 0))
      {  
        for(k=i+1;k<frame_cnt;k++)
        {
          unsigned short next_f1_range;
          unsigned char next_f2_gt;
          
          data = &data_ptr[cframe[k].idx * ONE_C_FRAME_SIZE];
          next_f1_range = data_ptr[0];
          next_f2_gt = (data[1]>>1) & 0x1;
          
          //check frame range
          if(next_f1_range > bit_pos)
            break;
          
          //check overlap
          v = (bit_pos - next_f1_range)%29;
          if((v>2) && (v<27)) //frame not overlap
            continue;
          
          //check amp relative
          if(  (bit_sum_amp >= DATA_THRESHOLD)  \
             && AMP_RELATIVE(bit_sum_amp, cframe[k].sum_ref_value, SUM_DIFF_THRESHOLD)) //L1 & L0
          {  
            other_amp_relative = 1; 
            if(other_oba_relative == 1)
              break;
          }  
          
          //check oba relative
          if(  OBA_RELATIVE(bit_oba, cframe[k].ref_oba, OBA_THRESHOLD) \
            && (bit_gt == next_f2_gt)) //Low confidence
          {
            other_oba_relative = 1;
            if(other_amp_relative == 1)
              break;
          }
        }//for
      }
      
      if(AMP_RELATIVE(bit_sum_amp, sum_ref, SUM_DIFF_THRESHOLD))
        my_amp_relative = 1;
      
      if(  OBA_RELATIVE(bit_oba, ref_oba, OBA_THRESHOLD) && (bit_gt == f2_gt))
        my_oba_relative = 1;
        
      if(bit_sum_amp < DATA_THRESHOLD) //H0 (exception monopulse H1 will result L0)
      {
        if(!((my_oba_relative == 1) && (other_oba_relative == 0))) //H0
          conf |= 1<<(13-bit);
      }
      else //H1, L1£¬L0
      {
        if(my_amp_relative == 1) //H1£¬L1
        {
          bit_value |= 1<<(13-bit);  
          if(other_amp_relative == 0)  //H1
            conf |= 1<<(13-bit);
          else //L1 (exception monopulse H1 will result H1)
          {
            if((my_oba_relative == 1) && (other_oba_relative == 0)) //H1
              conf |= 1<<(13-bit); 
          }
        }
        else //L0, L1
        {
          if(other_amp_relative == 0) //L1 (exception monopulse H1 will result H1)
          {
            bit_value |= 1<<(13-bit);
            if((my_oba_relative == 1) && (other_oba_relative == 0)) //H1
              conf |= 1<<(13-bit);
          }
        }
      }
    } //set bit value end
    
    //todo: maybe set spi value and confidence??? howto do it???
      
    //send C REPORT
    gConf = conf;
    gBitValue = bit_value;
    gSumRef = sum_ref;
    gDltRef = dlt_ref;
    construct_C_report(data);
    report_to_CPU((unsigned char *)&gCFrameReport);
  }
  
  //send END frame
  send_END_to_CPU(mode); //use last frame's mode
    
_c_do_next:
  //release this state buffer
  state_ptr[C_DATA_LEN_POS] = 0x0;
  
  //set read point to next position
  ridx++;
  if(ridx >= MAX_BUF_IN_C_POOL)
    ridx = 0;
  cdata_rptr[pool_id] = ridx;
}

