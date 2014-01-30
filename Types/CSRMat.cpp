#include "CSRMat.h"
#include "../sm.h"
#include "primitives.h"
#include <iostream>
#include <cstring>
#include <cassert>
#include "../docs.h"
extern Docs docs;

CSRMatDevicePartition::CSRMatDevicePartition(int d_end, int b, int t, int e) : devId(0), d_start(0), d_end(d_end), b(b), t(t), e(e)
{
}

CSRMatOperand::CSRMatOperand(Variable * variable) : Operand(variable) {};

CSRMatOperand::CSRMatOperand(Variable * variable, CSRMatDevicePartition partition) : Operand(variable)
{
  partitions[partition.devId] = partition;
};

void CSRMat::allocate(cl_vars_t clv)
{

  bytes_per_element = get_num_bytes(properties["dtype"]);
  nbytes_data = get_property_int("nnz") * bytes_per_element;
  nbytes_indices = get_property_int("nnz") * sizeof(int);
  nbytes_indptr = (get_property_int("dim") + 1) * sizeof(int);

  cpu_data = (char *) calloc(nbytes_data, sizeof(char));
  cpu_indices = (char *) calloc(nbytes_indices, sizeof(char));
  cpu_indptr = (char *) calloc(nbytes_indptr, sizeof(char));

  for(int i = 0 ; i < clv.num_devices ; i++)
  {
#ifdef VERBOSE_COMPILATION
    docs.compilation_ss << "Allocating: " << nbytes_data << "\tFor spmat: " << properties["name"] << "\tDevice: " << i <<std::endl; 
    docs.compilation_ss << "Allocating: " << nbytes_indices << "\tFor spmat: " << properties["name"] << "\tDevice: " << i << std::endl; 
    docs.compilation_ss << "Allocating: " << nbytes_indptr << "\tFor spmat: " << properties["name"] << "\tDevice: " << i << std::endl; 
#endif
    cl_int err;
    gpu_data[i] = clCreateBuffer(clv.context, CL_MEM_READ_WRITE, nbytes_data, NULL, &err);
    gpu_indices[i] = clCreateBuffer(clv.context, CL_MEM_READ_WRITE, nbytes_indices, NULL, &err);
    gpu_indptr[i] = clCreateBuffer(clv.context, CL_MEM_READ_WRITE, nbytes_indptr, NULL, &err);
    CHK_ERR(err);
  }
}

// Temporarally assume CSRMat never prevents fusion
MemLevel CSRMatOperand::matches(Operand * cmp)
{
  return ELEMENT;
}

void CSRMatOperand::callUp(cl_vars_t clv)
{
  CSRMat * v = dynamic_cast<CSRMat*>(variable);

  // For each device
  for(int devId = 0 ; devId < 1 ; devId++)
  {
    //CSRMatDevicePartition partition = (*it).second;
    // Copy it all
    if(v->nbytes_data > 0)
    {
#ifdef VERBOSE_EXECUTION
      docs.execution_ss << "Calling up CSRMat, Device: " << devId << "\tBytes data: " << v->nbytes_data;
#endif
      cl_int err = CL_TRUE;
      err = clEnqueueWriteBuffer(clv.commands[devId], v->gpu_data[devId], CL_FALSE, 
   		               0, v->nbytes_data, v->cpu_data, 0, NULL, NULL);
      CHK_ERR(err);
    }
    if(v->nbytes_indices > 0)
    {
#ifdef VERBOSE_EXECUTION
      docs.execution_ss << "\tBytes indices: " << v->nbytes_indices;
#endif
      cl_int err = CL_TRUE;
      err = clEnqueueWriteBuffer(clv.commands[devId], v->gpu_indices[devId], CL_FALSE, 
    		               0, v->nbytes_indices, v->cpu_indices, 0, NULL, NULL);
      CHK_ERR(err);
    }
    if(v->nbytes_indptr > 0)
    {
#ifdef VERBOSE_EXECUTION
      docs.execution_ss << "\tBytes indptr: " << v->nbytes_indptr;
#endif
      cl_int err = CL_TRUE;
      err = clEnqueueWriteBuffer(clv.commands[devId], v->gpu_indptr[devId], CL_FALSE, 
    		               0, v->nbytes_indptr, v->cpu_indptr, 0, NULL, NULL);
      CHK_ERR(err);
    }
  }

#ifdef VERBOSE_EXECUTION
    docs.execution_ss << std::endl;
#endif
}

void CSRMatOperand::pushDown(cl_vars_t clv)
{
  // Never happens, yet
}

std::string CSRMat::parameterStr()
{
  return "__global " + get_ctype(properties["dtype"]) + " * " + properties["name"] + ", __global int * " + properties["name"] + "_indices, __global int * " + properties["name"] + "_indptr";
}

void CSRMat::setParameter(cl_vars_t clv, cl_kernel kernel, int & arg_cnt, int device_id)
{
#ifdef VERBOSE_COMPILATION
  docs.compilation_ss << "Setting parameters for spmat" << std::endl;
#endif
  cl_int err = CL_SUCCESS;
  clSetKernelArg(kernel, arg_cnt++, sizeof(cl_mem), &(gpu_data[device_id])); CHK_ERR(err);
  clSetKernelArg(kernel, arg_cnt++, sizeof(cl_mem), &(gpu_indices[device_id])); CHK_ERR(err);
  clSetKernelArg(kernel, arg_cnt++, sizeof(cl_mem), &(gpu_indptr[device_id])); CHK_ERR(err);
}

int CSRMat::getTotalBytes()
{
  return nbytes_data + nbytes_indices + nbytes_indptr;
}

std::string CSRMatOperand::getLoad(MemLevel level){ return "";}
std::string CSRMatOperand::getStore(MemLevel level){ return "";}

std::string CSRMatOperand::getData(std::string offset)
{
  std::stringstream ss;
  ss << variable->properties["name"] << "[" << offset << "]";
  return ss.str();
}

std::string CSRMatOperand::getIndex(std::string offset)
{
  std::stringstream ss;
  ss << variable->properties["name"] << "_indices[" << offset << "]";
  return ss.str();
}

std::string CSRMatOperand::getIndptrElement()
{
  std::stringstream ss;
  ss << variable->properties["name"] << "_indptr[element_id0]";
  return ss.str();
}

std::string CSRMatOperand::getIndptrElement(std::string offset)
{
  std::stringstream ss;
  ss << variable->properties["name"] << "_indptr[element_id0 + " << offset << "]";
  return ss.str();
}

int CSRMat::getDim0()
{
  return get_property_int("dim0");
}
int CSRMat::getDim1()
{
  return get_property_int("dim1");
}
