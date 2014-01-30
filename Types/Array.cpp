#include "Array.h"
#include "../sm.h"
#include "primitives.h"
#include <iostream>
#include <cstring>
#include "../docs.h"
#include "../profiler.h"
extern Docs docs;
extern std::vector<transfer_prof_t> transfer_events;

extern int variable_counter;

ArrayDevicePartition::ArrayDevicePartition(int nd) : nd(nd), d(0), b(0), t(0), e(0) {};
ArrayDevicePartition::ArrayDevicePartition(int nd, int d) : nd(nd), d(d), b(0), t(0), e(0){};
ArrayDevicePartition::ArrayDevicePartition(int nd, int d, int b) : nd(nd), d(d), b(b), t(0), e(0) {};
ArrayDevicePartition::ArrayDevicePartition(int nd, int d, int b, int t) : nd(nd), d(d), b(b), t(t), e(0) {};
ArrayDevicePartition::ArrayDevicePartition(int nd, int d, int b, int t, int e) : nd(nd), d(d), b(b), t(t), e(e) {};

ArrayOperand::ArrayOperand(Variable * variable) : Operand(variable) {};
ArrayOperand::ArrayOperand(Variable * variable, ArrayDevicePartition partition) : Operand(variable), partition(partition) {};

Array * add_array(std::string prefix, int dim0, int dim1, std::string dtype, std::map<std::string, Variable*> & symbolTable, cl_vars_t clv)
{
  // Come up with random name
  std::stringstream name_ss;
  name_ss << prefix << "_array_" << variable_counter++;
  while(symbolTable.find(name_ss.str()) != symbolTable.end())
  {
#ifdef VERBOSE_COMPILATION
    docs.compilation_ss << "Recalculating array name" << std::endl;
#endif
    name_ss.str(std::string());
    name_ss << prefix << "_array_" << variable_counter++;
  }

  Array * arr = new Array();
  arr->properties["name"] = name_ss.str();
  char dim0_str[40];
  char dim1_str[40];
  sprintf(dim0_str, "%d", dim0);
  sprintf(dim1_str, "%d", dim1);
  arr->properties["dim0"] = dim0_str;
  arr->properties["dim1"] = dim1_str;
  arr->properties["dtype"] = dtype;
  symbolTable[name_ss.str()] = (Variable*) arr;
  arr->allocate(clv);

  return arr;
}

void Array::allocate(cl_vars_t clv)
{
  size_t dim0 = MYMAX(get_property_int("dim0"), 1);
  size_t dim1 = MYMAX(get_property_int("dim1"), 1);
  bytes_per_element = get_num_bytes(properties["dtype"]);
  nbytes = dim0 * dim1 * bytes_per_element;
#ifdef VERBOSE_COMPILATION
  docs.compilation_ss << "Allocating: " << nbytes << "\tFor array: " << properties["name"] << std::endl; 
#endif
  cpu_data = (char *) calloc(nbytes, sizeof(char));
  for(int i = 0 ; i < clv.num_devices ; i++)
  {
    cl_int err;
    gpu_data[i] = clCreateBuffer(clv.context, CL_MEM_READ_WRITE, nbytes, NULL, &err);
    CHK_ERR(err);
  }
}

std::string Array::parameterStr()
{
  return "__global " + get_ctype(properties["dtype"]) + " * " + properties["name"];
}

void ArrayOperand::callUp(cl_vars_t clv)
{
  Array * v = dynamic_cast<Array*>(variable);
  // For each device

  for(int devId = 0 ; devId < partition.nd ; devId++)
  {
    ssize_t device_min = 0;
    ssize_t device_max = 0;
    if(partition.d == 0)
    {
      device_min = 0;
      device_max = v->getDim();
    }
    else
    {
      device_min = partition.d * devId;
      device_max = partition.d * (devId+1);
      device_max = (device_max > v->getDim()) ? v->getDim() : device_max;
    }
    ssize_t device_range = device_max - device_min;
    ssize_t bytes_range = device_range * v->bytes_per_element;
    ssize_t bytes_offset = device_min * v->bytes_per_element;

    if(bytes_range > 0)
    {
#ifdef VERBOSE_EXECUTION
      docs.execution_ss << "TRANSFER: Calling up variable: " << variable->properties["name"];
      docs.execution_ss << "\tnbytes: " << bytes_range;
      docs.execution_ss << "\tbytes_offset: " << bytes_offset;
      docs.execution_ss << "\tdevice id: " << devId << std::endl;
#endif
      cl_int err = CL_SUCCESS;

#ifdef PROFILE
      transfer_prof_t tp;
      tp.devId = devId;
      tp.variable = variable;
      err = clEnqueueWriteBuffer(clv.commands[devId], v->gpu_data[devId], CL_FALSE, 
          		  bytes_offset, bytes_range, v->cpu_data + bytes_offset, 0, NULL, &(tp.event));
      transfer_events.push_back(tp);
#else
      err = clEnqueueWriteBuffer(clv.commands[devId], v->gpu_data[devId], CL_FALSE, 
          		  bytes_offset, bytes_range, v->cpu_data + bytes_offset, 0, NULL, NULL);
#endif
      CHK_ERR(err);
    }
  }
}

void ArrayOperand::pushDown(cl_vars_t clv)
{
  Array * v = dynamic_cast<Array*>(variable);
  // For each device
  for(int devId = 0 ; devId < partition.nd ; devId++)
  {
    ssize_t device_min = 0;
    ssize_t device_max = 0;
    if(partition.d == 0)
    {
      device_min = 0;
      device_max = v->getDim();
    }
    else
    {
      device_min = partition.d * devId;
      device_max = partition.d * (devId+1);
      device_max = (device_max > v->getDim()) ? v->getDim() : device_max;
    }
    ssize_t device_range = device_max - device_min;
    ssize_t bytes_range = device_range * v->bytes_per_element;
    ssize_t bytes_offset = device_min * v->bytes_per_element;
    if(bytes_range > 0)
    {
#ifdef VERBOSE_EXECUTION
      docs.execution_ss << "TRANSFER: Pushing down variable: " << variable->properties["name"];
      docs.execution_ss << "\tnbytes: " << bytes_range;
      docs.execution_ss << "\toffset: " << bytes_offset;
      docs.execution_ss << "\tdevice id: " << devId << std::endl;
#endif
      cl_int err = CL_TRUE;
#ifdef PROFILE
      transfer_prof_t tp;
      tp.devId = devId;
      tp.variable = variable;
      err = clEnqueueReadBuffer(clv.commands[devId], v->gpu_data[devId], CL_FALSE, 
        		  bytes_offset, bytes_range, v->cpu_data + bytes_offset, 0, NULL, &(tp.event));
      transfer_events.push_back(tp);
#else
      err = clEnqueueReadBuffer(clv.commands[devId], v->gpu_data[devId], CL_FALSE, 
        		  bytes_offset, bytes_range, v->cpu_data + bytes_offset, 0, NULL, NULL);
#endif
      CHK_ERR(err);
    }
  }
}

MemLevel ArrayOperand::matches(Operand * cmp)
{
  ArrayOperand * aCmp = dynamic_cast<ArrayOperand *>(cmp);
  if(!aCmp)
  {
    return ELEMENT;
  }
  if(aCmp->variable->properties["name"] != variable->properties["name"])
  {
    return ELEMENT;
  }
  ArrayDevicePartition cmp_partition = aCmp->partition;
  //if( (partition.d_start != cmp_partition.d_start) || (partition.d_end != cmp_partition.d_end) )
  
  // Check if these are partitioned the same way or if the prior partition envelops the latter
  // When does partition.b == 0??
  //if( (partition.d_start > cmp_partition.d_start) || (partition.d_end < cmp_partition.d_end) || (partition.b == 0) || (cmp_partition.b == 0)) // (1024)

      //printf("Comparing %s: %d, %d, %d, %d\t to \t %d %d %d %d\n", variable->properties["name"].c_str(),
      //      partition.d, partition.b, partition.t, partition.e, 
      //      cmp_partition.d, cmp_partition.b, cmp_partition.t, cmp_partition.e);

  // Consider adding partition.d == 0;
  if( (!partition.nd) || (!cmp_partition.nd) || (partition.nd != cmp_partition.nd) || (partition.d != cmp_partition.d)) // Or if the prior envlops the latter?
  {
    return PLATFORM;
  }
  else if((!partition.b) || (!cmp_partition.b) || (partition.b != cmp_partition.b)) // (256) Or if the prior block envelops the latter?
  {
    return DEVICE;
  }
  else if((!partition.t) || (!cmp_partition.t) || (partition.t != cmp_partition.t)) // (2)
  {
    return BLOCK;
  }
  else if((!partition.e) || (!cmp_partition.e) || (partition.e != cmp_partition.e)) // (1)
  {
    return THREAD;
  }
  return ELEMENT;
}

std::string ArrayOperand::get(std::string offset)
{
    return variable->properties["name"] + "[" + offset + "]";
}

std::string ArrayOperand::getElement()
{
  if(variable->level == ELEMENT)
  {
    return variable->properties["name"] + "_elt";
  }
  else
  {
    return variable->properties["name"] + "[element_id0]";
  }
}

std::string ArrayOperand::getElement(std::string offset)
{
  return variable->properties["name"] + "[element_id0 + (" + offset + ")]";
}

std::string ArrayOperand::getThread()
{
  if(variable->level == THREAD)
  {
    return variable->properties["name"] + "_thr";
  }
  else
  {
    return variable->properties["name"] + "[thread_id0]";
  }
}

std::string ArrayOperand::getThread(std::string offset)
{
  return variable->properties["name"] + "[thread_id0 + " + offset + "]";
}

std::string ArrayOperand::getBlock()
{
  return variable->properties["name"] + "[block_id0]";
}

std::string Array::getDeclaration(MemLevel mlev)
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

std::string ArrayOperand::getLoad(MemLevel level)
{
  if(level == ELEMENT)
  {
    return variable->properties["name"] + "_elt = " + variable->properties["name"] + "[element_id0];";
  }
  else if(level == THREAD)
  {
    return variable->properties["name"] + "_thr = " + variable->properties["name"] + "[thread_id0];";
  }
  return "";
}

std::string ArrayOperand::getStore(MemLevel level)
{
  if(level == ELEMENT)
  {
    return variable->properties["name"] + "[element_id0] = " + variable->properties["name"] + "_elt;";
  }
  else if(level == THREAD)
  {
    return variable->properties["name"] + "[thread_id0] = " + variable->properties["name"] + "_thr;";
  }
  return "";
}

void Array::setParameter(cl_vars_t clv, cl_kernel kernel, int & arg_cnt, int device_id)
{
#ifdef VERBOSE_COMPILATION
  docs.compilation_ss << "Setting parameter: " << properties["name"];
  docs.compilation_ss << "\tDevice: " << device_id << std::endl;
#endif
  cl_int err = CL_SUCCESS;
  clSetKernelArg(kernel, arg_cnt++, sizeof(cl_mem), &(gpu_data[device_id])); CHK_ERR(err);
}

int Array::getTotalBytes()
{
  return nbytes;
}

int Array::getDim()
{
  int dim0 = get_property_int("dim0");
  int dim1 = get_property_int("dim1");
  dim0 = (dim0 > 0) ? dim0 : 1;
  dim1 = (dim1 > 0) ? dim1 : 1;
  return dim0 * dim1;
}

MemLevel ArrayOperand::getRegisterLevel()
{
  if((partition.b == 1) && (partition.t == 0) && (partition.e == 0))
  {
    return BLOCK;
  }
  else if((partition.t == 1) && (partition.e == 0))
  {
    return THREAD;
  }
  else if(partition.e == 1)
  {
    return ELEMENT;
  }
  return NOLEVEL;
}
