#ifndef __PROFILER_H
#define __PROFILER_H

#include "em.h"
#include "lm.h"
#include "clhelp.h"

typedef struct TRANSFER_PROF_T
{
  Variable * variable;
  cl_event event;
  int devId;
} transfer_prof_t;

typedef struct KERNEL_PROF_T
{
  BlockLevel * b;
  cl_event event;
  int devId;
} kernel_prof_t;

void doProfile();
void doProfileTransfers();

#endif
