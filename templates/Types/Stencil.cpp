#include "Stencil.h"
#include "../sm.h"
#include "primitives.h"
#include <iostream>
#include <cstring>
#include <cassert>
#include "../docs.h"
extern Docs docs;

StencilPartition::StencilPartition() : nd(0) {};
StencilPartition::StencilPartition(int nd) : nd(nd) {};

StencilOperand::StencilOperand(Variable * variable) : Operand(variable) {};
StencilOperand::StencilOperand(Variable * variable, StencilPartition partition) : Operand(variable), partition(partition) {};

void Stencil::allocate(cl_vars_t clv)
{
  bytes_per_element = get_num_bytes(properties["dtype"]);
  nbytes_data = get_property_int("ndiags") * bytes_per_element;
  nbytes_offx = get_property_int("ndiags") * sizeof(int);
  nbytes_offy = get_property_int("ndiags") * sizeof(int);
  ndiags = get_property_int("ndiags");
  cpu_data = (char *) calloc(nbytes_data, sizeof(char));
  cpu_offx = (char *) calloc(nbytes_offx, sizeof(char));
  cpu_offy = (char *) calloc(nbytes_offy, sizeof(char));
  for(int devId = 0 ; devId < clv.num_devices ; devId++)
  {
#ifdef VERBOSE_COMPILATION
  docs.compilation_ss << "Allocating: " << nbytes_data << "\tFor stencil: " << properties["name"] << "\tDevice: " << devId <<std::endl; 
  docs.compilation_ss << "Allocating: " << nbytes_offx << "\tFor stencil: " << properties["name"] << "\tDevice: " << devId << std::endl; 
  docs.compilation_ss << "Allocating: " << nbytes_offy << "\tFor stencil: " << properties["name"] << "\tDevice: " << devId << std::endl; 
#endif
    cl_int err;
    gpu_data[devId] = clCreateBuffer(clv.context, CL_MEM_READ_WRITE, nbytes_data, NULL, &err);
    gpu_offx[devId] = clCreateBuffer(clv.context, CL_MEM_READ_WRITE, nbytes_offx, NULL, &err);
    gpu_offy[devId] = clCreateBuffer(clv.context, CL_MEM_READ_WRITE, nbytes_offy, NULL, &err);
    CHK_ERR(err);
  }
}

// Temporarally assume Stencil never prevents fusion
MemLevel StencilOperand::matches(Operand * cmp)
{
  return ELEMENT;
}

void StencilOperand::callUp(cl_vars_t clv)
{
  Stencil * v = dynamic_cast<Stencil*>(variable);


  for(int devId = 0 ; devId < partition.nd; devId++)
  {
#ifdef VERBOSE_EXECUTION
    docs.execution_ss << "TRANSFER: Calling up Stencil, Device: " << devId << "\tBytes data: " << v->nbytes_data;
#endif
    cl_int err = CL_TRUE;
    float a[10];
    err = clEnqueueWriteBuffer(clv.commands[devId], v->gpu_data[devId], CL_FALSE, 
 		               0, v->nbytes_data, v->cpu_data, 0, NULL, NULL);
    CHK_ERR(err);
#ifdef VERBOSE_EXECUTION
    docs.execution_ss << "\tBytes offx: " << v->nbytes_offx;
#endif
    err = clEnqueueWriteBuffer(clv.commands[devId], v->gpu_offx[devId], CL_FALSE, 
  		               0, v->nbytes_offx, v->cpu_offx, 0, NULL, NULL);
    CHK_ERR(err);
#ifdef VERBOSE_EXECUTION
    docs.execution_ss << "\tBytes offy: " << v->nbytes_offy;
#endif
    err = clEnqueueWriteBuffer(clv.commands[devId], v->gpu_offy[devId], CL_FALSE, 
  		               0, v->nbytes_offy, v->cpu_offy, 0, NULL, NULL);
    CHK_ERR(err);
  }
#ifdef VERBOSE_EXECUTION
  docs.execution_ss << std::endl;
#endif
}

void StencilOperand::pushDown(cl_vars_t clv)
{
  Stencil * v = dynamic_cast<Stencil*>(variable);
  int devId = 0;
#ifdef VERBOSE_EXECUTION
  docs.execution_ss << "TRANSFER: Pushing down Stencil, Device: " << devId << "\tBytes data: " << v->nbytes_data;
#endif
  cl_int err = CL_TRUE;
  err = clEnqueueReadBuffer(clv.commands[devId], v->gpu_data[devId], CL_FALSE, 
 		               0, v->nbytes_data, v->cpu_data, 0, NULL, NULL);
  CHK_ERR(err);
#ifdef VERBOSE_EXECUTION
  docs.execution_ss << "\tBytes offx: " << v->nbytes_offx;
#endif
  err = clEnqueueReadBuffer(clv.commands[devId], v->gpu_offx[devId], CL_FALSE, 
  		               0, v->nbytes_offx, v->cpu_offx, 0, NULL, NULL);
  CHK_ERR(err);
#ifdef VERBOSE_EXECUTION
  docs.execution_ss << "\tBytes offy: " << v->nbytes_offy;
#endif
  err = clEnqueueReadBuffer(clv.commands[devId], v->gpu_offy[devId], CL_FALSE, 
		               0, v->nbytes_offy, v->cpu_offy, 0, NULL, NULL);
  CHK_ERR(err);
#ifdef VERBOSE_EXECUTION
  docs.execution_ss << std::endl;
#endif
}

std::string Stencil::parameterStr()
{
  return "__global " + get_ctype(properties["dtype"]) + " * " + properties["name"] + ", __global int * " + properties["name"] + "_offx, __global int * " + properties["name"] + "_offy" ;
}

void Stencil::setParameter(cl_vars_t clv, cl_kernel kernel, int & arg_cnt, int device_id)
{
#ifdef VERBOSE_COMPILATION
  docs.compilation_ss << "Setting parameters for spmat" << std::endl;
#endif
  cl_int err = CL_SUCCESS;
  clSetKernelArg(kernel, arg_cnt++, sizeof(cl_mem), &(gpu_data[device_id])); CHK_ERR(err);
  clSetKernelArg(kernel, arg_cnt++, sizeof(cl_mem), &(gpu_offx[device_id])); CHK_ERR(err);
  clSetKernelArg(kernel, arg_cnt++, sizeof(cl_mem), &(gpu_offy[device_id])); CHK_ERR(err);
}

/* Accessors */

std::string StencilOperand::getData(std::string diag)
{
  std::stringstream ss;
  ss << variable->properties["name"] << "[" << diag << "]";
  return ss.str();
}
std::string StencilOperand::getOffx(std::string diag)
{
  std::stringstream ss;
  ss << variable->properties["name"] << "_offx[" << diag << "]";
  return ss.str();
}
std::string StencilOperand::getOffy(std::string diag)
{
  std::stringstream ss;
  ss << variable->properties["name"] << "_offy[" << diag << "]";
  return ss.str();
}

int Stencil::getTotalBytes()
{
  return nbytes_data + nbytes_offx + nbytes_offy;
}

std::string StencilOperand::getLoad(MemLevel level){ assert(0); return "";}
std::string StencilOperand::getStore(MemLevel level){ assert(0);  return "";}


