#include "SpMat2D.h"
#include "../sm.h"
#include "primitives.h"
#include <iostream>
#include <cstring>
#include <cassert>
#include "../docs.h"
extern Docs docs;

SpMat2DDevicePartition::SpMat2DDevicePartition(int * dim)
{
  for(int i = 0 ; i < 2 ; i++)
  {
    this->dim[i] = dim[i];
    this->nd[i] = 1;
    this->d[i] = 0;
    this->b[i] = 0;
    this->t[i] = 0;
    this->e = 0;
  }
}

SpMat2DDevicePartition::SpMat2DDevicePartition(int * dim, int * nd)
{
  for(int i = 0 ; i < 2 ; i++)
  {
    this->dim[i] = dim[i];
    this->nd[i] = nd[i];
    this->d[i] = 0;
    this->b[i] = 0;
    this->t[i] = 0;
    this->e = 0;
  }
}
SpMat2DDevicePartition::SpMat2DDevicePartition(int * dim, int * nd, int * d)
{
  for(int i = 0 ; i < 2 ; i++)
  {
    this->dim[i] = dim[i];
    this->nd[i] = nd[i];
    this->d[i] = d[i];
    this->b[i] = 0;
    this->t[i] = 0;
    this->e = 0;
  }
}
SpMat2DDevicePartition::SpMat2DDevicePartition(int * dim, int * nd, int * d, int * b)
{
  for(int i = 0 ; i < 2 ; i++)
  {
    this->dim[i] = dim[i];
    this->nd[i] = nd[i];
    this->d[i] = d[i];
    this->b[i] = b[i];
    this->t[i] = 0;
    this->e = 0;
  }
}
SpMat2DDevicePartition::SpMat2DDevicePartition(int * dim, int * nd, int * d, int * b, int * t)
{
  for(int i = 0 ; i < 2 ; i++)
  {
    this->dim[i] = dim[i];
    this->nd[i] = nd[i];
    this->d[i] = d[i];
    this->b[i] = b[i];
    this->t[i] = t[i];
    this->e = 0;
  }
}
SpMat2DDevicePartition::SpMat2DDevicePartition(int * dim, int * nd, int * d, int * b, int * t, int e)
{
  for(int i = 0 ; i < 2 ; i++)
  {
    this->dim[i] = dim[i];
    this->nd[i] = nd[i];
    this->d[i] = d[i];
    this->b[i] = b[i];
    this->t[i] = t[i];
    this->e = e;
  }
}

SpMat2DOperand::SpMat2DOperand(Variable * variable) : Operand(variable) {};
SpMat2DOperand::SpMat2DOperand(Variable * variable, SpMat2DDevicePartition partition) : Operand(variable), partition(partition) {};

void SpMat2D::allocate(cl_vars_t clv)
{
  bytes_per_element = get_num_bytes(properties["dtype"]);

  nbytes_data = get_property_int("ndiags") * get_property_int("dim0") * get_property_int("dim1") * bytes_per_element;
  nbytes_offx = get_property_int("ndiags") * sizeof(int);
  nbytes_offy = get_property_int("ndiags") * sizeof(int);
  ndiags = get_property_int("ndiags");

  cpu_data = (char *) calloc(nbytes_data, sizeof(char));
  cpu_offx = (char *) calloc(nbytes_offx, sizeof(char));
  cpu_offy = (char *) calloc(nbytes_offy, sizeof(char));
  for(int devId = 0 ; devId < clv.num_devices ; devId++)
  {
#ifdef VERBOSE_COMPILATION
    docs.compilation_ss << "Allocating: " << nbytes_data << "\tFor spmat: " << properties["name"] << "\tDevice: " << devId <<std::endl; 
    docs.compilation_ss << "Allocating: " << nbytes_offx << "\tFor spmat: " << properties["name"] << "\tDevice: " << devId << std::endl; 
    docs.compilation_ss << "Allocating: " << nbytes_offy << "\tFor spmat: " << properties["name"] << "\tDevice: " << devId << std::endl; 
#endif
    cl_int err;
    gpu_data[devId] = clCreateBuffer(clv.context, CL_MEM_READ_WRITE, nbytes_data, NULL, &err);
    gpu_offx[devId] = clCreateBuffer(clv.context, CL_MEM_READ_WRITE, nbytes_offx, NULL, &err);
    gpu_offy[devId] = clCreateBuffer(clv.context, CL_MEM_READ_WRITE, nbytes_offy, NULL, &err);
    CHK_ERR(err);
  }
}

// Temporarally assume SpMat2D never prevents fusion
MemLevel SpMat2DOperand::matches(Operand * cmp)
{
  return ELEMENT;
}

void SpMat2DOperand::callUp(cl_vars_t clv)
{
  SpMat2D * v = dynamic_cast<SpMat2D*>(variable);
  int nd = partition.nd[0] * partition.nd[1];

  // This won't cut it down the middle. Will need to do a memcpy2D
  for(int devId = 0 ; devId < nd; devId++)
  {
#ifdef VERBOSE_EXECUTION
    docs.execution_ss << "TRANSFER: Calling up SpMat2D, Device: " << devId << "\tBytes data: " << v->nbytes_data;
#endif
    cl_int err = CL_TRUE;
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
#ifdef VERBOSE_EXECUTION
    docs.execution_ss << std::endl;
#endif
  }
}

void SpMat2DOperand::pushDown(cl_vars_t clv)
{
  SpMat2D * v = dynamic_cast<SpMat2D*>(variable);
  int nd = partition.nd[0] * partition.nd[1];

  // This won't cut it down the middle. Will need to do a memcpy2D
  for(int devId = 0 ; devId < nd; devId++)
  {
#ifdef VERBOSE_EXECUTION
    docs.execution_ss << "TRANSFER: Pushing down SpMat2D, Device: " << devId << "\tBytes data: " << v->nbytes_data;
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
}


std::string SpMat2D::parameterStr()
{
  return "__global " + get_ctype(properties["dtype"]) + " * " + properties["name"] + ", __global int * " + properties["name"] + "_offx, __global int * " + properties["name"] + "_offy";
}

void SpMat2D::setParameter(cl_vars_t clv, cl_kernel kernel, int & arg_cnt, int device_id)
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
std::string SpMat2DOperand::getData(std::string offset)
{
  std::stringstream ss;
  ss << variable->properties["name"] << "[" << offset << "]";
  return ss.str();
}

std::string SpMat2DOperand::getDataElement(std::string diag, std::string off0, std::string off1)
{
  std::stringstream ss;
  ss << variable->properties["name"] << "[element_id0 + element_id1 * " << partition.dim[0] << " + " << diag << " * " << partition.dim[0] * partition.dim[1] << " + " << off0 << " + " << off1 << " * " << partition.dim[0]  << "]";
  return ss.str();
}

std::string SpMat2DOperand::getOffxElement(std::string diag)
{
  std::stringstream ss;
  ss << variable->properties["name"] << "_offx" << "[" << diag << "]";
  return ss.str();
}

std::string SpMat2DOperand::getOffyElement(std::string diag)
{
  std::stringstream ss;
  ss << variable->properties["name"] << "_offy" << "[" << diag << "]";
  return ss.str();
}

int SpMat2D::getTotalBytes()
{
  return nbytes_data + nbytes_offx + nbytes_offy;
}

int SpMat2D::getDim0()
{
  int dim = get_property_int("dim0");
  return dim;
}

int SpMat2D::getDim1()
{
  int dim = get_property_int("dim1");
  return dim;
}

std::string SpMat2DOperand::getLoad(MemLevel level){ assert(0); return "";}
std::string SpMat2DOperand::getStore(MemLevel level){ assert(0);  return "";}


