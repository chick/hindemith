#include "SpMat.h"
#include "../sm.h"
#include "primitives.h"
#include <iostream>
#include <cstring>
#include <cassert>
#include "../docs.h"
extern Docs docs;

SpMatDevicePartition::SpMatDevicePartition(int nd, int d, int b, int t, int e) : nd(nd), d(d), b(b), t(t), e(e) {};
SpMatOperand::SpMatOperand(Variable * variable) : Operand(variable) {};
SpMatOperand::SpMatOperand(Variable * variable, SpMatDevicePartition partition) : Operand(variable), partition(partition) {};

void SpMat::allocate(cl_vars_t clv)
{
  bytes_per_element = get_num_bytes(properties["dtype"]);

  nbytes_data = get_property_int("ndiags") * get_property_int("dim") * bytes_per_element;
  nbytes_offsets = get_property_int("ndiags") * sizeof(int);
  ndiags = get_property_int("ndiags");

  cpu_data = (char *) calloc(nbytes_data, sizeof(char));
  cpu_offsets = (char *) calloc(nbytes_offsets, sizeof(char));
  for(int devId = 0 ; devId < clv.num_devices ; devId++)
  {
#ifdef VERBOSE_COMPILATION
  docs.compilation_ss << "Allocating: " << nbytes_data << "\tFor spmat: " << properties["name"] << "\tDevice: " << devId <<std::endl; 
  docs.compilation_ss << "Allocating: " << nbytes_offsets << "\tFor spmat: " << properties["name"] << "\tDevice: " << devId << std::endl; 
#endif
    cl_int err;
    gpu_data[devId] = clCreateBuffer(clv.context, CL_MEM_READ_WRITE, nbytes_data, NULL, &err);
    gpu_offsets[devId] = clCreateBuffer(clv.context, CL_MEM_READ_WRITE, nbytes_offsets, NULL, &err);
    CHK_ERR(err);
  }
}

// Temporarally assume SpMat never prevents fusion
MemLevel SpMatOperand::matches(Operand * cmp)
{
  return ELEMENT;
}

void SpMatOperand::callUp(cl_vars_t clv)
{
  SpMat * v = dynamic_cast<SpMat*>(variable);

  // This won't cut it down the middle. Will need to do a memcpy2D

  for(int devId = 0 ; devId < partition.nd ; devId++)
  {
    ssize_t device_min = partition.d * devId;
    ssize_t device_max = partition.d * (devId+1);
    device_max = (device_max > v->getDim()) ? v->getDim() : device_max;
    ssize_t device_range = device_max - device_min;
    size_t bytes_data = device_range * v->bytes_per_element * v->ndiags;
    size_t bytes_offsets = v->nbytes_offsets;
    if(bytes_data > 0)
    {
#ifdef VERBOSE_EXECUTION
      docs.execution_ss << "TRANSFER: Calling up SpMat, Device: " << devId << "\tBytes data: " << bytes_data;
#endif
      cl_int err = CL_TRUE;
      err = clEnqueueWriteBuffer(clv.commands[devId], v->gpu_data[devId], CL_FALSE, 
   		               0, bytes_data, v->cpu_data, 0, NULL, NULL);
      CHK_ERR(err);
    }
    if(bytes_offsets > 0)
    {
#ifdef VERBOSE_EXECUTION
      docs.execution_ss << "\tBytes offsets: " << bytes_offsets;
#endif
      cl_int err = CL_TRUE;
      err = clEnqueueWriteBuffer(clv.commands[devId], v->gpu_offsets[devId], CL_FALSE, 
    		               0, bytes_offsets, v->cpu_offsets, 0, NULL, NULL);
      CHK_ERR(err);
    }
#ifdef VERBOSE_EXECUTION
    docs.execution_ss << std::endl;
#endif
  }
}

void SpMatOperand::pushDown(cl_vars_t clv)
{
  SpMat * v = dynamic_cast<SpMat*>(variable);

  // For each device
  for(int devId = 0 ; devId < partition.nd ; devId++)
  {
    ssize_t device_min = partition.d * devId;
    ssize_t device_max = partition.d * (devId+1);
    device_max = (device_max > v->getDim()) ? v->getDim() : device_max;
    ssize_t device_range = device_max - device_min;
    size_t bytes_data = device_range * v->bytes_per_element * v->ndiags;
    size_t bytes_offsets = v->nbytes_offsets;
    if(bytes_data > 0)
    {
#ifdef VERBOSE_EXECUTION
      docs.execution_ss << "TRANSFER: Pushing down SpMat, Device: " << devId << "\tBytes data: " << bytes_data;
#endif
      cl_int err = CL_TRUE;
      err = clEnqueueReadBuffer(clv.commands[devId], v->gpu_data[devId], CL_FALSE, 
   		               0, bytes_data, v->cpu_data, 0, NULL, NULL);
      CHK_ERR(err);
    }
    if(bytes_offsets > 0)
    {
#ifdef VERBOSE_EXECUTION
      docs.execution_ss << "\tBytes offsets: " << bytes_offsets;
#endif
      cl_int err = CL_TRUE;
      err = clEnqueueReadBuffer(clv.commands[devId], v->gpu_offsets[devId], CL_FALSE, 
    		               0, bytes_offsets, v->cpu_offsets, 0, NULL, NULL);
      CHK_ERR(err);
    }
#ifdef VERBOSE_EXECUTION
    docs.execution_ss << std::endl;
#endif
  }

}

std::string SpMat::parameterStr()
{
  return "__global " + get_ctype(properties["dtype"]) + " * " + properties["name"] + ", __global int * " + properties["name"] + "_offset";
}

void SpMat::setParameter(cl_vars_t clv, cl_kernel kernel, int & arg_cnt, int device_id)
{
#ifdef VERBOSE_COMPILATION
  docs.compilation_ss << "Setting parameters for spmat" << std::endl;
#endif
  cl_int err = CL_SUCCESS;
  clSetKernelArg(kernel, arg_cnt++, sizeof(cl_mem), &(gpu_data[device_id])); CHK_ERR(err);
  clSetKernelArg(kernel, arg_cnt++, sizeof(cl_mem), &(gpu_offsets[device_id])); CHK_ERR(err);
}

/* Accessors */

std::string SpMatOperand::getData(std::string offset)
{
  std::stringstream ss;
  ss << variable->properties["name"] << "[" << offset << "]";
  return ss.str();
}

std::string SpMatOperand::getDataElement(std::string diag, std::string offset, int dim)
{
  std::stringstream ss;
  ss << variable->properties["name"] << "[element_id0 + " << diag << " * " << dim << " + " << offset << "]";
  return ss.str();
}

std::string SpMatOperand::getOffsetElement(std::string diag)
{
  std::stringstream ss;
  ss << variable->properties["name"] << "_offset" << "[" << diag << "]";
  return ss.str();
}

int SpMat::getTotalBytes()
{
  return nbytes_data + nbytes_offsets;
}

int SpMat::getDim()
{
  int dim = get_property_int("dim");
  return dim;
}

std::string SpMatOperand::getLoad(MemLevel level){ assert(0); return "";}
std::string SpMatOperand::getStore(MemLevel level){ assert(0);  return "";}


