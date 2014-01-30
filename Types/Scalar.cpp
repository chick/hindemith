#include "Scalar.h"
#include "../sm.h"
#include "primitives.h"
#include <iostream>
#include "../docs.h"
#include "../profiler.h"
extern Docs docs;
extern std::vector<transfer_prof_t> transfer_events;

ScalarOperand::ScalarOperand(Variable * variable, std::string val, int ndevices) : 
               Operand(variable), val(val), ndevices(ndevices) {};

void Scalar::allocate(cl_vars_t clv)
{
  bytes_per_element = get_num_bytes(properties["dtype"]);
  nbytes = bytes_per_element;
#ifdef VERBOSE_COMPILATION
  docs.compilation_ss << "Allocating: " << nbytes << "\tFor scalar: " << properties["name"] << std::endl; 
#endif
  cpu_data = (char *) calloc(nbytes, sizeof(char));
  for(int i = 0 ; i < clv.num_devices; i++)
  {
    cl_int err;
    gpu_data[i] = clCreateBuffer(clv.context, CL_MEM_READ_WRITE, nbytes, NULL, &err);
    CHK_ERR(err);
  }
}

MemLevel ScalarOperand::matches(Operand * po_compare)
{
  ScalarOperand * aCmp = dynamic_cast<ScalarOperand *>(po_compare);
  if(!aCmp)
  {
    return ELEMENT;
  }
  if(aCmp->variable->properties["name"] != variable->properties["name"])
  {
    return ELEMENT;
  }
  if(ndevices != aCmp->ndevices)
  {
    return PLATFORM;
  }
  return ELEMENT; // Temporary, saying it never prevents fusion
  //return PLATFORM; // Temporary, saying it never prevents fusion
}

std::string Scalar::parameterStr()
{
  std::string dtype;
  return "__global " + get_ctype(properties["dtype"]) + " * " + properties["name"];
}

void ScalarOperand::callUp(cl_vars_t clv)
{
  Scalar * v = dynamic_cast<Scalar*>(variable);
  for(int devId = 0 ; devId < ndevices ; devId++)
  {
#ifdef VERBOSE_EXECUTION
    docs.execution_ss << "TRANSFER: Calling up variable: " << variable->properties["name"];
    docs.execution_ss << "\tnbytes: " << v->nbytes;
    docs.execution_ss << "\tdevice id: " << devId << std::endl;
#endif
    cl_int err = CL_TRUE;
#ifdef PROFILE
    transfer_prof_t tp;
    tp.devId = devId;
    tp.variable = variable;
    err = clEnqueueWriteBuffer(clv.commands[devId], v->gpu_data[devId], CL_FALSE, 
        		  0, v->nbytes, v->cpu_data, 0, NULL, &(tp.event));
    transfer_events.push_back(tp);
#else
    err = clEnqueueWriteBuffer(clv.commands[devId], v->gpu_data[devId], CL_FALSE, 
        		  0, v->nbytes, v->cpu_data, 0, NULL, NULL);
#endif
    CHK_ERR(err);
  }
}

void ScalarOperand::pushDown(cl_vars_t clv)
{
  Scalar * v = dynamic_cast<Scalar*>(variable);
#ifdef VERBOSE_EXECUTION
  docs.execution_ss << "TRANSFER: Pushing down variable: " << variable->properties["name"];
  docs.execution_ss << "\tnbytes: " << v->nbytes << std::endl;
#endif
  // Device 0 should have a valid copy
  cl_int err = CL_TRUE;
#ifdef PROFILE
  transfer_prof_t tp;
  tp.devId = 0;
  tp.variable = variable;
  err = clEnqueueReadBuffer(clv.commands[0], v->gpu_data[0], CL_FALSE, 
      		  0, v->nbytes, v->cpu_data, 0, NULL, &(tp.event));
  transfer_events.push_back(tp);
#else
  err = clEnqueueReadBuffer(clv.commands[0], v->gpu_data[0], CL_FALSE, 
      		  0, v->nbytes, v->cpu_data, 0, NULL, NULL);
#endif
  CHK_ERR(err);
}

void Scalar::setParameter(cl_vars_t clv, cl_kernel kernel, int & arg_cnt, int device_id)
{
  cl_int err = CL_SUCCESS;
  clSetKernelArg(kernel, arg_cnt++, sizeof(cl_mem), &(gpu_data[device_id])); CHK_ERR(err);
}

std::string ScalarOperand::getElement()
{
  if(variable)
  {
    if(variable->level == ELEMENT)
    {
      return variable->properties["name"] + "_elt";
    }
    else
    {
      return "*" + variable->properties["name"];
    }
  }
  else
  {
    return val;
  }
}

std::string ScalarOperand::getThread()
{
  return "*" + variable->properties["name"];
}

std::string Scalar::getDeclaration(MemLevel mlev)
{
  if(mlev == ELEMENT)
  {
    return get_ctype(properties["dtype"]) + " " + properties["name"] + "_elt;";
  }
  else if(mlev == THREAD)
  {
    return get_ctype(properties["dtype"]) + " " + properties["name"] + "_thr;";
  }
  else
  {
    return "";
  }
}

int Scalar::getTotalBytes()
{
  return nbytes;
}

std::string ScalarOperand::getLoad(MemLevel level)
{
  return variable->properties["name"] + "_elt = *" + variable->properties["name"] + ";";
}

std::string ScalarOperand::getStore(MemLevel level)
{
  return "*" + variable->properties["name"] + " = " + variable->properties["name"] + "_elt;";
}


MemLevel ScalarOperand::getRegisterLevel()
{
  return ELEMENT;
}
