#include "profiler.h"
#include <iostream>
#include <cassert>
#include "sm.h"
#include "docs.h"
extern Docs docs;
extern std::vector<kernel_prof_t> kernel_events;
extern std::vector<transfer_prof_t> transfer_events;

int getBytesTransferred(BlockLevel * b)
{
  // Compute set of live-in sources and live-out sinks
  int bytes_transferred = 0.0;
  for(std::vector<Operand*>::iterator it = b->sources.begin() ; it != b->sources.end() ; it++)
  {
    if(b->liveIns.count((*it)->variable))
    {
      bytes_transferred += (*it)->variable->getTotalBytes();
    }
  }

  for(std::vector<Operand*>::iterator it = b->sinks.begin() ; it != b->sinks.end() ; it++)
  {
    if(b->liveOuts.count((*it)->variable))
    {
      bytes_transferred += (*it)->variable->getTotalBytes();
    }
  }
  return bytes_transferred;
}

void doProfileTransfers()
{
  for(unsigned int i = 0 ; i < transfer_events.size() ; i++)
  {
    Variable * v = transfer_events[i].variable;
    int devId = transfer_events[i].devId;
    cl_event event = transfer_events[i].event;
    cl_int err;
    cl_ulong profile_temp_end;
    cl_ulong profile_temp_start;
    cl_ulong profile_temp_queue;
    cl_ulong profile_temp_submit;
    err = clWaitForEvents(1, &(event)); CHK_ERR(err);
    err = clGetEventProfilingInfo((event), CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &profile_temp_end, 0); CHK_ERR(err);
    err = clGetEventProfilingInfo((event), CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &profile_temp_start, 0); CHK_ERR(err);
    err = clGetEventProfilingInfo((event), CL_PROFILING_COMMAND_QUEUED, sizeof(cl_ulong), &profile_temp_queue, 0); CHK_ERR(err);
    err = clGetEventProfilingInfo((event), CL_PROFILING_COMMAND_SUBMIT, sizeof(cl_ulong), &profile_temp_submit, 0); CHK_ERR(err);
    cl_ulong duration = profile_temp_end-profile_temp_start;

    int bytes_transferred = 2 * v->getTotalBytes();
    double gbytes_per_sec = ((double)bytes_transferred) / ((double)duration);

    docs.profile_ss << "\"" << "t_dev" << devId << "_" << v->properties["name"] << "\"," << ((double)duration) / (1000000.0) << ","<< profile_temp_queue << "," << profile_temp_submit << "," << profile_temp_start << "," << profile_temp_end << "," << bytes_transferred <<  "," << gbytes_per_sec << std::endl;
  }

}

void doProfile()
{
  for(unsigned int i = 0 ; i < kernel_events.size() ; i++)
  {
    BlockLevel * b = kernel_events[i].b;
    int devId = kernel_events[i].devId;
    int block_id = b->id;
    cl_event event = kernel_events[i].event;
    cl_int err;
    cl_ulong profile_temp_end;
    cl_ulong profile_temp_start;
    cl_ulong profile_temp_queue;
    cl_ulong profile_temp_submit;
    err = clWaitForEvents(1, &(event)); CHK_ERR(err);
    err = clGetEventProfilingInfo((event), CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &profile_temp_end, 0); CHK_ERR(err);
    err = clGetEventProfilingInfo((event), CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &profile_temp_start, 0); CHK_ERR(err);
    err = clGetEventProfilingInfo((event), CL_PROFILING_COMMAND_QUEUED, sizeof(cl_ulong), &profile_temp_queue, 0); CHK_ERR(err);
    err = clGetEventProfilingInfo((event), CL_PROFILING_COMMAND_SUBMIT, sizeof(cl_ulong), &profile_temp_submit, 0); CHK_ERR(err);
    cl_ulong duration = profile_temp_end-profile_temp_start;

    int bytes_transferred = getBytesTransferred(b);
    double gbytes_per_sec = ((double)bytes_transferred) / ((double)duration);

    docs.profile_ss << "\"" << "k_dev" << devId << "_" << block_id << "\"," << ((double)duration) / (1000000.0) << ","<< profile_temp_queue << "," << profile_temp_submit << "," << profile_temp_start << "," << profile_temp_end << "," << bytes_transferred <<  "," << gbytes_per_sec << std::endl;
  }
}

