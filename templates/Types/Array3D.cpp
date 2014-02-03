#include "Array3D.h"
#include "../sm.h"
#include "primitives.h"
#include <iostream>
#include <cstring>
#include <cassert>
#include "../docs.h"
#include "../profiler.h"
extern Docs docs;
extern std::vector<transfer_prof_t> transfer_events;

extern int variable_counter;

Array3DDevicePartition::Array3DDevicePartition(int * dim)
{
  for(int i = 0 ; i < 3 ; i++)
  {
    this->dim[i] = dim[i];
    this->nd[i] = 1;
    this->d[i] = 0;
    this->b[i] = 0;
    this->t[i] = 0;
    this->e = 0;
  }
}

Array3DDevicePartition::Array3DDevicePartition(int * dim, int * nd)
{
  for(int i = 0 ; i < 3 ; i++)
  {
    this->dim[i] = dim[i];
    this->nd[i] = nd[i];
    this->d[i] = 0;
    this->b[i] = 0;
    this->t[i] = 0;
    this->e = 0;
  }
}
Array3DDevicePartition::Array3DDevicePartition(int * dim, int * nd, int * d)
{
  for(int i = 0 ; i < 3 ; i++)
  {
    this->dim[i] = dim[i];
    this->nd[i] = nd[i];
    this->d[i] = d[i];
    this->b[i] = 0;
    this->t[i] = 0;
    this->e = 0;
  }
}
Array3DDevicePartition::Array3DDevicePartition(int * dim, int * nd, int * d, int * b)
{
  for(int i = 0 ; i < 3 ; i++)
  {
    this->dim[i] = dim[i];
    this->nd[i] = nd[i];
    this->d[i] = d[i];
    this->b[i] = b[i];
    this->t[i] = 0;
    this->e = 0;
  }
}
Array3DDevicePartition::Array3DDevicePartition(int * dim, int * nd, int * d, int * b, int * t)
{
  for(int i = 0 ; i < 3 ; i++)
  {
    this->dim[i] = dim[i];
    this->nd[i] = nd[i];
    this->d[i] = d[i];
    this->b[i] = b[i];
    this->t[i] = t[i];
    this->e = 0;
  }
}
Array3DDevicePartition::Array3DDevicePartition(int * dim, int * nd, int * d, int * b, int * t, int e)
{
  for(int i = 0 ; i < 3 ; i++)
  {
    this->dim[i] = dim[i];
    this->nd[i] = nd[i];
    this->d[i] = d[i];
    this->b[i] = b[i];
    this->t[i] = t[i];
    this->e = e;
  }
}

void Array3DDevicePartition::print()
{
  std::cout << dim[0] << ", " << dim[1] << ", " << dim[2] << "\t" << nd[0] << ", " << nd[1] << ", " << nd[2] << "\t" << d[0] << ", " << d[1] << ", " << d[2] << "\t" << t[0] << ", " << t[1] << ", " << t[2] << "\t" << b[0] << ", " << b[1] << ", " << b[2] << "\t" << e << ", " << e << std::endl;
}

Array3DOperand::Array3DOperand(Variable * variable) : Operand(variable) {};
Array3DOperand::Array3DOperand(Variable * variable, Array3DDevicePartition partition) : Operand(variable), partition(partition) {};

Array3D * add_array3d(std::string prefix, int * dim, std::string dtype, std::map<std::string, Variable*> & symbolTable, cl_vars_t clv)
{
  // Come up with random name
  std::stringstream name_ss;
  name_ss << prefix << "_array3d_" << variable_counter++;
  while(symbolTable.find(name_ss.str()) != symbolTable.end())
  {
#ifdef VERBOSE_COMPILATION
    docs.compilation_ss << "Recalculating array name" << std::endl;
#endif
    name_ss.str(std::string());
    name_ss << prefix << "_array3d_" << variable_counter++;
  }

  Array3D * arr = new Array3D();
  arr->properties["name"] = name_ss.str();
  char dim0_str[40];
  char dim1_str[40];
  char dim2_str[40];
  sprintf(dim0_str, "%d", dim[0]);
  sprintf(dim1_str, "%d", dim[1]);
  sprintf(dim2_str, "%d", dim[2]);
  arr->properties["dim0"] = dim0_str;
  arr->properties["dim1"] = dim1_str;
  arr->properties["dim2"] = dim2_str;
  arr->properties["dtype"] = dtype;
  symbolTable[name_ss.str()] = (Variable*) arr;
  arr->allocate(clv);

  return arr;
}

void Array3D::allocate(cl_vars_t clv)
{
  size_t dim0 = MYMAX(get_property_int("dim0"), 1);
  size_t dim1 = MYMAX(get_property_int("dim1"), 1);
  size_t dim2 = MYMAX(get_property_int("dim2"), 1);
  bytes_per_element = get_num_bytes(properties["dtype"]);
  nbytes = dim0 * dim1 * dim2 * bytes_per_element;
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

std::string Array3D::parameterStr()
{
  return "__global " + get_ctype(properties["dtype"]) + " * " + properties["name"];
}

void Array3DOperand::callUp(cl_vars_t clv)
{
  Array3D * v = dynamic_cast<Array3D*>(variable);
  // Only splitting on last dimension
  for(int dev0 = 0 ; dev0 < partition.nd[2] ; dev0++)
  {
    int devId = dev0;
    ssize_t device_min = 0;
    ssize_t device_max = 0;
    int total_dim = v->getDim0() * v->getDim1() * v->getDim2();
    if(partition.d[0] * partition.d[1] * partition.d[2] == 0)
    {
      device_min = 0;
      device_max = total_dim;
    }
    else
    {
      device_min = partition.d[0] * partition.d[1] * partition.d[2] * devId;
      device_max = partition.d[0] * partition.d[1] * partition.d[2] * (devId+1);
      device_max = (device_max > total_dim) ? total_dim : device_max;
    }
    ssize_t device_range = device_max - device_min;
    ssize_t bytes_range = device_range * v->bytes_per_element;
    ssize_t bytes_offset = device_min * v->bytes_per_element;

#ifdef VERBOSE_EXECUTION
    docs.execution_ss << "Calling up variable: " << variable->properties["name"];
    docs.execution_ss << "\tdevId: " << devId;
    docs.execution_ss << "\tnbytes: " << bytes_range;
    docs.execution_ss << "\tbytes_offset: " << bytes_offset << std::endl;
#endif
    if(bytes_range > 0)
    {
      cl_int err = CL_TRUE;

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

void Array3DOperand::pushDown(cl_vars_t clv)
{
  Array3D * v = dynamic_cast<Array3D*>(variable);
  // Only splitting on last dimension
  for(int dev0 = 0 ; dev0 < partition.nd[2] ; dev0++)
  {
    int devId = dev0;
    ssize_t device_min = 0;
    ssize_t device_max = 0;
    int total_dim = v->getDim0() * v->getDim1() * v->getDim2();
    if(partition.d[0] * partition.d[1] * partition.d[2] == 0)
    {
      device_min = 0;
      device_max = total_dim;
    }
    else
    {
      device_min = partition.d[0] * partition.d[1] * partition.d[2] * devId;
      device_max = partition.d[0] * partition.d[1] * partition.d[2] * (devId+1);
      device_max = (device_max > total_dim) ? total_dim : device_max;
    }
    ssize_t device_range = device_max - device_min;
    ssize_t bytes_range = device_range * v->bytes_per_element;
    ssize_t bytes_offset = device_min * v->bytes_per_element;

#ifdef VERBOSE_EXECUTION
    docs.execution_ss << "Pushing down variable: " << variable->properties["name"];
    docs.execution_ss << "\tdevId: " << devId;
    docs.execution_ss << "\tnbytes: " << bytes_range;
    docs.execution_ss << "\tbytes_offset: " << bytes_offset << std::endl;
#endif
    if(bytes_range > 0)
    {
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

MemLevel Array3DOperand::matches(Operand * cmp)
{
  Array3DOperand * aCmp = dynamic_cast<Array3DOperand *>(cmp);
  if(!aCmp)
  {
    return ELEMENT;
  }
  if(aCmp->variable->properties["name"] != variable->properties["name"])
  {
    return ELEMENT;
  }
  Array3DDevicePartition cmp_partition = aCmp->partition;
  if((partition.nd[0] == 0) || (cmp_partition.nd[0] == 0) || (partition.nd[0] != cmp_partition.nd[0]) || (partition.nd[1] != cmp_partition.nd[1]) || (partition.nd[2] != cmp_partition.nd[2]) || (partition.d[0] != cmp_partition.d[0]) || (partition.d[1] != cmp_partition.d[1]) || (partition.d[2] != cmp_partition.d[2]))
  {
    return PLATFORM;
  }
  if((!partition.b[0]) || (!cmp_partition.b[0]) || (!partition.b[1]) || (!cmp_partition.b[1]) || (!partition.b[2]) || (!cmp_partition.b[2])  || (partition.b[0] != cmp_partition.b[0]) || (partition.b[1] != cmp_partition.b[1]) || (partition.b[2] != cmp_partition.b[2]))
  {
    return DEVICE;
  }
  if((!partition.t[0]) || (!cmp_partition.t[0]) || (!partition.t[1]) || (!cmp_partition.t[1]) || (!partition.t[2]) || (!cmp_partition.t[2]) || (partition.t[0] != cmp_partition.t[0]) || (partition.t[1] != cmp_partition.t[1]) || (partition.t[2] != cmp_partition.t[2]) )
  {
    return BLOCK;
  }
  if((!partition.e) || (!cmp_partition.e) || (partition.e != cmp_partition.e))
  {
    return THREAD;
  }
  return ELEMENT;
}

std::string Array3DOperand::get(std::string offset)
{
    return variable->properties["name"] + "[" + offset + "]";
}

std::string Array3DOperand::get(std::string offset0, std::string offset1, std::string offset2)
{
  std::stringstream ss;
  ss << variable->properties["name"] << "[" << offset_string(offset0, offset1, offset2) << "]";
  return ss.str();
}

std::string Array3DOperand::element_id_string()
{
  std::stringstream ss;
  //if(variable->properties["col_mjr"] == "True")
  {
    ss << "element_id0 + element_id1 * " << partition.dim[0] << " + element_id2 * " << partition.dim[0] * partition.dim[1];
  }
  //else
  //{
    //ss << "element_id0 + element_id1 * " << partition.d_end[1];
  //  ss << "element_id0 + element_id1 * " << partition.d_end[0];
  //}
  return ss.str();
}

std::string Array3DOperand::thread_id_string()
{
  std::stringstream ss;
  //if(variable->properties["col_mjr"] == "True")
  {
    int nthreads0 = (partition.t[0] > 0) ? (partition.dim[0] / partition.t[0]) : partition.dim[0];
    int nthreads1 = (partition.t[1] > 0) ? (partition.dim[1] / partition.t[1]) : partition.dim[1];
    ss << "thread_id0 + thread_id1 * " << nthreads0 << " + thread_id2 * " << nthreads0*nthreads1;
  }
  //else
  //{
    //ss << "element_id0 + element_id1 * " << partition.d_end[1];
  //  ss << "element_id0 + element_id1 * " << partition.d_end[0];
  //}
  return ss.str();
}

std::string Array3DOperand::offset_string(std::string offset0, std::string offset1, std::string offset2)
{
  std::stringstream ss;
  //if(variable->properties["col_mjr"] == "True")
  //{
    ss << "(" << offset0 << ")" << " + " << "(" << offset1 << ")" << " * " << partition.dim[0] << " + " << "(" << offset2 << ")" << " * " << partition.dim[0] * partition.dim[1];
  //}
  //else
 // {
 //   ss << offset1 << " + " << offset0 << " * " << partition.d_end[1];
 // }
  return ss.str();
}

std::string Array3DOperand::getElement()
{
  if(variable->level == ELEMENT)
  {
    return variable->properties["name"] + "_elt";
  }
  else
  {
    std::stringstream ss;
    ss << variable->properties["name"] << "[" << element_id_string() << "]";
    return ss.str();
  }
}

std::string Array3DOperand::getElement(std::string offset)
{
  std::stringstream ss;
  ss << variable->properties["name"] << "[" << element_id_string() << " + " << offset << "]";
  return ss.str();
}

std::string Array3DOperand::getElement(std::string offset0, std::string offset1, std::string offset2)
{
  std::stringstream ss;
  ss << variable->properties["name"] << "[" << element_id_string() << " + " << offset_string(offset0, offset1, offset2) << "]";
  return ss.str();
}

std::string Array3DOperand::getThread()
{
  if(variable->level == THREAD)
  {
    return variable->properties["name"] + "_thr";
  }
  else
  {
    std::stringstream ss;
    ss << variable->properties["name"] <<  "[" << thread_id_string() << "]";
    return ss.str();
  }
}

std::string Array3DOperand::getThread(std::string offset)
{
  std::stringstream ss;
  ss << variable->properties["name"] << "[" << thread_id_string() << " + " << offset << "]";
  return ss.str();
}


std::string Array3DOperand::getThread(std::string offset0, std::string offset1, std::string offset2)
{
  std::stringstream ss;
  ss << variable->properties["name"] << "[" << thread_id_string() << " + " << offset_string(offset0, offset1, offset2) << "]";
  return ss.str();
}

std::string Array3DOperand::getBlock()
{
  std::stringstream ss;
  int nblocks0 = (partition.dim[0] + partition.b[0]-1) / partition.b[0];
  int nblocks1 = (partition.dim[1] + partition.b[1]-1) / partition.b[1];
  ss << variable->properties["name"] << "[block_id0 + block_id1 * " << nblocks0 << " + block_id2 * " << nblocks0*nblocks1 << "]";
  return ss.str();
}

std::string Array3D::getDeclaration(MemLevel mlev)
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

std::string Array3DOperand::getLoad(MemLevel level)
{
  if(level == ELEMENT)
  {
    std::stringstream ss;
    ss << variable->properties["name"] << "_elt = " << variable->properties["name"] << "[" << element_id_string() << "];";
    return ss.str();
  }
  else if(level == THREAD)
  {
    std::stringstream ss;
    ss << variable->properties["name"] << "_thr = " << variable->properties["name"] << "[" << thread_id_string() << "];";
    return ss.str();
  }
  return "";
}

std::string Array3DOperand::getStore(MemLevel level)
{
  if(level == ELEMENT)
  {
    std::stringstream ss;
    ss << variable->properties["name"] << "[" << element_id_string() <<"] = " << variable->properties["name"] + "_elt;";
    return ss.str();
  }
  else if(level == THREAD)
  {
    std::stringstream ss;
    ss << variable->properties["name"] << "[" << thread_id_string() <<"] = " << variable->properties["name"] + "_thr;";
    return ss.str();
  }
  return "";
}

void Array3D::setParameter(cl_vars_t clv, cl_kernel kernel, int & arg_cnt, int device_id)
{
#ifdef VERBOSE_COMPILATION
  docs.compilation_ss << "Setting parameter: " << properties["name"];
  docs.compilation_ss << "\tDevice: " << device_id << std::endl;
#endif
  cl_int err = CL_SUCCESS;
  clSetKernelArg(kernel, arg_cnt++, sizeof(cl_mem), &(gpu_data[device_id])); CHK_ERR(err);
}

int Array3D::getTotalBytes()
{
  return nbytes;
}

MemLevel Array3DOperand::getRegisterLevel()
{
  if((partition.b[0] == 1) && (partition.b[1] == 1) && (partition.b[2] == 1) &&
     (partition.t[0] == 0) && (partition.t[1] == 0) && (partition.t[2] == 0) &&
     (partition.e == 0))
  {
    return BLOCK;
  }
  else if((partition.t[0] == 1) && (partition.t[1] == 1) && (partition.t[2] == 1) && 
          (partition.e == 0))
  {
    return THREAD;
  }
  else if(partition.e == 1)
  {
    return ELEMENT;
  }
  return NOLEVEL;
}

int Array3D::getDim0()
{
  int dim0 = get_property_int("dim0");
  return dim0;
}

int Array3D::getDim1()
{
  int dim1 = get_property_int("dim1");
  return dim1;
}

int Array3D::getDim2()
{
  int dim2 = get_property_int("dim2");
  return dim2;
}
