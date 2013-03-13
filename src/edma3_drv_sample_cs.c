/*******************************************************************************
**+--------------------------------------------------------------------------+**
**|                            ****                                          |**
**|                            ****                                          |**
**|                            ******o***                                    |**
**|                      ********_///_****                                   |**
**|                      ***** /_//_/ ****                                   |**
**|                       ** ** (__/ ****                                    |**
**|                           *********                                      |**
**|                            ****                                          |**
**|                            ***                                           |**
**|                                                                          |**
**|         Copyright (c) 1998-2006 Texas Instruments Incorporated           |**
**|                        ALL RIGHTS RESERVED                               |**
**|                                                                          |**
**| Permission is hereby granted to licensees of Texas Instruments           |**
**| Incorporated (TI) products to use this computer program for the sole     |**
**| purpose of implementing a licensee product based on TI products.         |**
**| No other rights to reproduce, use, or disseminate this computer          |**
**| program, whether in part or in whole, are granted.                       |**
**|                                                                          |**
**| TI makes no representation or warranties with respect to the             |**
**| performance of this computer program, and specifically disclaims         |**
**| any responsibility for any damages, special or consequential,            |**
**| connected with the use of this program.                                  |**
**|                                                                          |**
**+--------------------------------------------------------------------------+**
*******************************************************************************/

/** \file   bios_edma3_drv_sample_cs.c

  \brief  Sample functions showing the implementation of Critical section
  entry/exit routines and various semaphore related routines (all OS
  depenedent). These implementations MUST be provided by the user /
  application, using the EDMA3 driver, for its
  correct functioning.
  
    (C) Copyright 2006, Texas Instruments, Inc
    
      \version    1.0   Anuj Aggarwal         - Created
      1.1   Anuj Aggarwal         - Made the sample app generic
      - Removed redundant arguments
      from Cache-related APIs
      - Added new function for Poll mode
      testing
      
*/
#if 0 //wj

#include <hwi.h>
#include <tsk.h>
#include <clk.h>
#include <sem.h>
#endif

#include <ecm.h>
#include <bcache.h>

#include "bios_edma3_drv_sample.h"

#define EDMA3_CACHE_WAIT        (1u)


/**
* \brief   EDMA3 OS Protect Entry
*
*      This function saves the current state of protection in 'intState'
*      variable passed by caller, if the protection level is
*      EDMA3_OS_PROTECT_INTERRUPT. It then applies the requested level of
*      protection.
*      For EDMA3_OS_PROTECT_INTERRUPT_XFER_COMPLETION and
*      EDMA3_OS_PROTECT_INTERRUPT_CC_ERROR, variable 'intState' is ignored,
*      and the requested interrupt is disabled.
*      For EDMA3_OS_PROTECT_INTERRUPT_TC_ERROR, '*intState' specifies the
*      Transfer Controller number whose interrupt needs to be disabled.
*
* \param   level is numeric identifier of the desired degree of protection.
* \param   intState is memory location where current state of protection is
*      saved for future use while restoring it via edma3OsProtectExit() (Only
*      for EDMA3_OS_PROTECT_INTERRUPT protection level).
* \return  None
*/
void edma3OsProtectEntry (int level, unsigned int *intState)
{
#if 0  //wj
  if (((level == EDMA3_OS_PROTECT_INTERRUPT)
    || (level == EDMA3_OS_PROTECT_INTERRUPT_TC_ERROR))
    && (intState == NULL))
  {
    return;
  }
  else
  {
    switch (level)
    {
      /* Disable all (global) interrupts */
    case EDMA3_OS_PROTECT_INTERRUPT :
      *intState = HWI_disable();
      break;
      
      /* Disable scheduler */
    case EDMA3_OS_PROTECT_SCHEDULER :
      TSK_disable();
      break;
      
      /* Disable EDMA3 transfer completion interrupt only */
    case EDMA3_OS_PROTECT_INTERRUPT_XFER_COMPLETION :
      ECM_disableEvent(ccXferCompInt);
      break;
      
      /* Disable EDMA3 CC error interrupt only */
    case EDMA3_OS_PROTECT_INTERRUPT_CC_ERROR :
      ECM_disableEvent(ccErrorInt);
      break;
      
      /* Disable EDMA3 TC error interrupt only */
    case EDMA3_OS_PROTECT_INTERRUPT_TC_ERROR :
      switch (*intState)
      {
      case 0:
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
      case 6:
      case 7:
        /* Fall through... */
        /* Disable the corresponding interrupt */
        ECM_disableEvent(tcErrorInt[*intState]);
        break;
        
      default:
        break;
      }
      
      break;
      
      default:
        break;
    }
  }
#endif
}


/**
* \brief   EDMA3 OS Protect Exit
*
*      This function undoes the protection enforced to original state
*      as is specified by the variable 'intState' passed, if the protection
*      level is EDMA3_OS_PROTECT_INTERRUPT.
*      For EDMA3_OS_PROTECT_INTERRUPT_XFER_COMPLETION and
*      EDMA3_OS_PROTECT_INTERRUPT_CC_ERROR, variable 'intState' is ignored,
*      and the requested interrupt is enabled.
*      For EDMA3_OS_PROTECT_INTERRUPT_TC_ERROR, 'intState' specifies the
*      Transfer Controller number whose interrupt needs to be enabled.
* \param   level is numeric identifier of the desired degree of protection.
* \param   intState is original state of protection at time when the
*      corresponding edma3OsProtectEntry() was called (Only
*      for EDMA3_OS_PROTECT_INTERRUPT protection level).
* \return  None
*/
void edma3OsProtectExit (int level, unsigned int intState)
{
#if 0 //wj
  switch (level)
  {
    /* Enable all (global) interrupts */
  case EDMA3_OS_PROTECT_INTERRUPT :
    HWI_restore(intState);
    break;
    
    /* Enable scheduler */
  case EDMA3_OS_PROTECT_SCHEDULER :
    TSK_enable();
    break;
    
    /* Enable EDMA3 transfer completion interrupt only */
  case EDMA3_OS_PROTECT_INTERRUPT_XFER_COMPLETION :
    ECM_enableEvent(ccXferCompInt);
    break;
    
    /* Enable EDMA3 CC error interrupt only */
  case EDMA3_OS_PROTECT_INTERRUPT_CC_ERROR :
    ECM_enableEvent(ccErrorInt);
    break;
    
    /* Enable EDMA3 TC error interrupt only */
  case EDMA3_OS_PROTECT_INTERRUPT_TC_ERROR :
    switch (intState)
    {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
      /* Fall through... */
      /* Enable the corresponding interrupt */
      ECM_enableEvent(tcErrorInt[intState]);
      break;
      
    default:
      break;
    }
    
    break;
    
    default:
      break;
  }
#endif
}


/**
*  \brief   EDMA3 Cache Invalidate
*
*  This function invalidates the D cache.
*
*  \param  mem_start_ptr [IN]      Starting adress of memory.
*                                  Please note that this should be
*                                  32 bytes alinged.
*  \param  num_bytes [IN]          length of buffer
*  \return  nil return value
*/
void Edma3_CacheInvalidate(unsigned int mem_start_ptr,
                           unsigned int    num_bytes)
{
  /* Verify whether the start address is 128-bytes aligned or not */
  if((mem_start_ptr & (0x7FU))    !=    0)
  {
#ifdef EDMA3_DRV_DEBUG
    EDMA3_DRV_PRINTF("\r\n Cache : Memory is not 128 bytes alinged\r\n");
#endif
    return;
  }
  
  BCACHE_inv((void *)mem_start_ptr,
    num_bytes,
    EDMA3_CACHE_WAIT);
}



/**
* \brief   EDMA3 Cache Flush
*
*  This function flushes (cleans) the Cache
*
*  \param  mem_start_ptr [IN]      Starting adress of memory. Please note that
*                                  this should be 32 bytes alinged.
*  \param  num_bytes [IN]          length of buffer
* \return  nil return value
*/
void Edma3_CacheFlush(unsigned int mem_start_ptr,
                      unsigned int num_bytes)
{
  /* Verify whether the start address is 128-bytes aligned or not */
  if((mem_start_ptr & (0x7FU))    !=    0)
  {
#ifdef EDMA3_DRV_DEBUG
    EDMA3_DRV_PRINTF("\r\n Cache : Memory is not 128 bytes alinged\r\n");
#endif
  }
  
  BCACHE_wb ((void *)mem_start_ptr,
    num_bytes,
    EDMA3_CACHE_WAIT);
}


/**
* Counting Semaphore related functions (OS dependent) should be
* called/implemented by the application. A handle to the semaphore
* is required while opening the driver/resource manager instance.
*/

/**
* \brief   EDMA3 OS Semaphore Create
*
*      This function creates a counting semaphore with specified
*      attributes and initial value. It should be used to create a semaphore
*      with initial value as '1'. The semaphore is then passed by the user
*      to the EDMA3 driver/RM for proper sharing of resources.
* \param   initVal [IN] is initial value for semaphore
* \param   attrs [IN] is the semaphore attributes ex: Fifo type
* \param   hSem [OUT] is location to recieve the handle to just created
*      semaphore
* \return  EDMA3_DRV_SOK if succesful, else a suitable error code.
*/
EDMA3_DRV_Result edma3OsSemCreate(int initVal,
                                  const EDMA3_OS_SemAttrs *attrs,
                                  EDMA3_OS_Sem_Handle *hSem)
{
#if 0 //wj  
  EDMA3_DRV_Result semCreateResult = EDMA3_DRV_SOK;
  
  if(NULL == hSem)
  {
    semCreateResult = EDMA3_DRV_E_INVALID_PARAM;
  }
  else
  {
    *hSem = (EDMA3_OS_Sem_Handle)SEM_create(initVal, (SEM_Attrs*)attrs);
    if ( (*hSem) == NULL )
    {
      semCreateResult = EDMA3_DRV_E_SEMAPHORE;
    }
  }
  
  return semCreateResult;
#else
  *hSem = (EDMA3_OS_Sem_Handle)1;
  return 0;
#endif
  
}


/**
* \brief   EDMA3 OS Semaphore Delete
*
*      This function deletes or removes the specified semaphore
*      from the system. Associated dynamically allocated memory
*      if any is also freed up.
* \warning OsSEM services run in client context and not in a thread
*      of their own. If there exist threads pended on a semaphore
*      that is being deleted, results are undefined.
* \param   hSem [IN] handle to the semaphore to be deleted
* \return  EDMA3_DRV_SOK if succesful else a suitable error code
*/
EDMA3_DRV_Result edma3OsSemDelete(EDMA3_OS_Sem_Handle hSem)
{
#if 0
  EDMA3_DRV_Result semDeleteResult = EDMA3_DRV_SOK;
  
  if(NULL == hSem)
  {
    semDeleteResult = EDMA3_DRV_E_INVALID_PARAM;
  }
  else
  {
    SEM_delete(hSem);
  }
  
  return semDeleteResult;
#else
  return 0;
#endif
}


/**
* \brief   EDMA3 OS Semaphore Take
*
*      This function takes a semaphore token if available.
*      If a semaphore is unavailable, it blocks currently
*      running thread in wait (for specified duration) for
*      a free semaphore.
* \param   hSem [IN] is the handle of the specified semaphore
* \param   mSecTimeout [IN] is wait time in milliseconds
* \return  EDMA3_DRV_Result if successful else a suitable error code
*/
EDMA3_DRV_Result edma3OsSemTake(EDMA3_OS_Sem_Handle hSem, int mSecTimeout)
{
#if 0  //wj
  EDMA3_DRV_Result semTakeResult = EDMA3_DRV_SOK;
  unsigned short semPendResult;
  
  if(NULL == hSem)
  {
    semTakeResult = EDMA3_DRV_E_INVALID_PARAM;
  }
  else
  {
    if (TSK_self() != (TSK_Handle)&KNL_dummy)
    {
      semPendResult = SEM_pend(hSem, mSecTimeout);
      if (semPendResult == FALSE)
      {
        semTakeResult = EDMA3_DRV_E_SEMAPHORE;
      }
    }
  }
  
  return semTakeResult;
#else
  return 0;
#endif
}


/**
* \brief   EDMA3 OS Semaphore Give
*
*      This function gives or relinquishes an already
*      acquired semaphore token
* \param   hSem [IN] is the handle of the specified semaphore
* \return  EDMA3_DRV_Result if successful else a suitable error code
*/
EDMA3_DRV_Result edma3OsSemGive(EDMA3_OS_Sem_Handle hSem)
{
#if 0  
  EDMA3_DRV_Result semGiveResult = EDMA3_DRV_SOK;
  
  if(NULL == hSem)
  {
    semGiveResult = EDMA3_DRV_E_INVALID_PARAM;
  }
  else
  {
    if (TSK_self() != (TSK_Handle)&KNL_dummy)
    {
      SEM_post(hSem);
    }
  }
  
  return semGiveResult;
#else
  return 0;
#endif
}






