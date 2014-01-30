#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <cassert>

#include "clhelp.h"
#include "docs.h"

extern Docs docs;

const char * memset_kernel_str = 
"__kernel void memset_kernel(__global char * arr, char val, int nbytes)" 
"{"
"  const char tid = get_global_id(0) + get_global_id(1) * get_global_size(0);"
"  if(tid < nbytes)"
"  {"
"    arr[tid] = val;"
"  }"
"}";

void mymemset(cl_vars_t cv, cl_mem ptr, char val, int nbytes)
{
  cl_int err;
  err = clSetKernelArg(cv.memset_kernel, 0, sizeof(cl_mem), &(ptr)); CHK_ERR(err);
  err = clSetKernelArg(cv.memset_kernel, 1, sizeof(char), &val); CHK_ERR(err);
  err = clSetKernelArg(cv.memset_kernel, 2, sizeof(int), &nbytes); CHK_ERR(err);
  size_t local = 16;
  size_t global = ((nbytes + local - 1) / local) * local;
  clEnqueueNDRangeKernel(cv.commands[0], cv.memset_kernel, 1, NULL, &global, &local, 0, NULL, NULL);
}

void initialize_ocl(cl_vars_t& cv)
{
  cl_uint num_platforms;
  cv.err = clGetPlatformIDs(1, &(cv.platform), &(num_platforms));
  if(cv.err != CL_SUCCESS)
  {
    std::cout << "Could not get platform ID" << std::endl;
    exit(1);
  }

  if(getenv("HM_CPU0"))
  {
    std::cout << "Running on CPU 0" << std::endl;
    cl_uint max_devices = 1;
    cv.err = clGetDeviceIDs(cv.platform, CL_DEVICE_TYPE_CPU, max_devices, cv.device_ids, &(cv.num_devices));
    cv.num_devices = 1;
  }
  else if(getenv("HM_CPU0_SUB1"))
  {
    std::cout << "Running on Subdivided1 CPU 0" << std::endl;
    cl_uint max_devices = 1;
    cl_device_id dev0;
    cv.err = clGetDeviceIDs(cv.platform, CL_DEVICE_TYPE_CPU, max_devices, &dev0, &(cv.num_devices));
    cl_uint num_subdevices;
    cl_device_partition_property props[3];
    props[0] = CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN;
    props[1] = CL_DEVICE_AFFINITY_DOMAIN_NEXT_PARTITIONABLE;
    props[2] = 0;
    cl_device_id id4[4];
    cv.err = clCreateSubDevices(dev0, props, 2, id4, &num_subdevices);
    std::cout << "num subdevices: " << num_subdevices << std::endl;
    cv.device_ids[0] = id4[1];
    cv.num_devices = 1;
  }
  else if(getenv("HM_GPU0"))
  {
    std::cout << "Running on GPU 0" << std::endl;
    cl_uint max_devices = 1;
    cv.err = clGetDeviceIDs(cv.platform, CL_DEVICE_TYPE_GPU, max_devices, cv.device_ids, &(cv.num_devices));
    cv.num_devices = 1;
  }
  else if(getenv("HM_GPU01"))
  {
    std::cout << "Running on GPU 0 and GPU 1" << std::endl;
    cl_uint max_devices = 2;
    cv.err = clGetDeviceIDs(cv.platform, CL_DEVICE_TYPE_GPU, max_devices, cv.device_ids, &(cv.num_devices));
    cv.num_devices = 2;
  }
  else if(getenv("HM_GPU1"))
  {
    std::cout << "Running on GPU 1" << std::endl;
    cl_uint max_devices = 2;
    cv.err = clGetDeviceIDs(cv.platform, CL_DEVICE_TYPE_GPU, max_devices, cv.device_ids, &(cv.num_devices));
    assert(cv.num_devices > 1);
    cv.device_ids[0] = cv.device_ids[1];
    cv.num_devices = 1;
  }
  else
  {
    std::cout << "Error: Specify target either HM_CPU0, HM_GPU0, HM_GPU01, or HM_GPU1" << std::endl;
  }
  if(cv.err != CL_SUCCESS)
  {
    std::cout << "Could not get GPU device ID" << std::endl;
    exit(1);
  }

  cv.context = clCreateContext(0, cv.num_devices, cv.device_ids, NULL, NULL, &(cv.err));
  if(!cv.context)
  {
    std::cout << "Could not create context" << std::endl;
    exit(1);
  }

  //cv.commands = clCreateCommandQueue(cv.context, cv.device_id, 0, &(cv.err));
  for(size_t devId = 0 ; devId < cv.num_devices ; devId++)
  {
    cv.commands[devId] = clCreateCommandQueue(cv.context, cv.device_ids[devId], CL_QUEUE_PROFILING_ENABLE, &(cv.err));
    if(!cv.commands[devId])
    {
      std::cout << "Could not create command queue" << std::endl;
      exit(1);
    }
  }
  compile_ocl_program(cv.memset_program, cv.memset_kernel, cv, memset_kernel_str, "memset_kernel");

#ifdef VERBOSE_COMPILATION
  docs.opencl_ss << "CL fill vars success" << std::endl;

  // Device info
  for(size_t devId = 0 ; devId < cv.num_devices ; devId++)
  {
    docs.opencl_ss << "Device ID: " << devId << std::endl;

    char device_name[255];
    cv.err = clGetDeviceInfo(cv.device_ids[devId], CL_DEVICE_NAME, 255, device_name, NULL);
    docs.opencl_ss << "Device Name: " << device_name << std::endl;

    cl_ulong mem_size;
    cv.err = clGetDeviceInfo(cv.device_ids[devId], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(cl_ulong), &mem_size, NULL);
    docs.opencl_ss << "Global mem size: " << mem_size << std::endl;

    size_t max_work_item[3];
    cv.err = clGetDeviceInfo(cv.device_ids[devId], CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(max_work_item), max_work_item, NULL);
    docs.opencl_ss << "Max work item sizes: " << max_work_item[0] << ", " << max_work_item[1] << ", " << max_work_item[2] << std::endl;
  }
#endif
}

void uninitialize_ocl(cl_vars_t & clv)
{
  clReleaseKernel(clv.memset_kernel);
  clReleaseProgram(clv.memset_program);
  for(size_t devId = 0 ; devId < clv.num_devices ; devId++)
  {
    clReleaseCommandQueue(clv.commands[devId]);
  }
  clReleaseContext(clv.context);
}

void write_ocl_program(const char * cl_src)
{
  std::ofstream logfile;
  logfile.open("kernels.cl", std::ios::app);
  logfile << cl_src << std::endl;
  logfile.close();
}

void compile_ocl_program(cl_program & program, cl_kernel & kernel, const cl_vars_t cv, const char * cl_src, const char * kname)
{
  cl_int err;
  program = clCreateProgramWithSource(cv.context, 1, (const char **) &cl_src, NULL, &err);
  if(!program)
  {
    std::cout << "Failed to compile OpenCL source" << std::endl;
    exit(1);
  }

  //err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
  err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
  if (err != CL_SUCCESS)
  {
    size_t len;
    char buffer[2048];
    std::cout << "Error: Failed to build program executable: " << kname <<  std::endl;
    clGetProgramBuildInfo(program, cv.device_ids[0], CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
    std::cout << buffer << std::endl;
    exit(1);
  }

  kernel = clCreateKernel(program, kname, &(err));
  if(!kernel || err != CL_SUCCESS)
  {
    std::cout << "Failed to create kernel: " << kname  << std::endl;
    exit(1);
  }
#ifdef VERBOSE_COMPILATION
  docs.compilation_ss << "Successfully compiled " << kname << std::endl;
#endif

}

void readFile(std::string& fileName, std::string &out)
{
  std::ifstream in(fileName.c_str(), std::ios::in | std::ios::binary);
  if(in)
    {
      in.seekg(0, std::ios::end);
      out.resize(in.tellg());
      in.seekg(0, std::ios::beg);
      in.read(&out[0], out.size());
      in.close();
    }
  else
    {
      std::cout << "Failed to open " << fileName << std::endl;
      exit(-1);
    }
}

std::string reportOCLError(cl_int err)
{
  std::stringstream stream;
  switch (err) 
    {
    case CL_DEVICE_NOT_FOUND:          
      stream << "Device not found.";
      break;
    case CL_DEVICE_NOT_AVAILABLE:           
      stream << "Device not available";
      break;
    case CL_COMPILER_NOT_AVAILABLE:     
      stream << "Compiler not available";
      break;
    case CL_MEM_OBJECT_ALLOCATION_FAILURE:   
      stream << "Memory object allocation failure";
      break;
    case CL_OUT_OF_RESOURCES:       
      stream << "Out of resources";
      break;
    case CL_OUT_OF_HOST_MEMORY:     
      stream << "Out of host memory";
      break;
    case CL_PROFILING_INFO_NOT_AVAILABLE:  
      stream << "Profiling information not available";
      break;
    case CL_MEM_COPY_OVERLAP:        
      stream << "Memory copy overlap";
      break;
    case CL_IMAGE_FORMAT_MISMATCH:   
      stream << "Image format mismatch";
      break;
    case CL_IMAGE_FORMAT_NOT_SUPPORTED:         
      stream << "Image format not supported";    break;
    case CL_BUILD_PROGRAM_FAILURE:     
      stream << "Program build failure";    break;
    case CL_MAP_FAILURE:         
      stream << "Map failure";    break;
    case CL_INVALID_VALUE:
      stream << "Invalid value";    break;
    case CL_INVALID_DEVICE_TYPE:
      stream << "Invalid device type";    break;
    case CL_INVALID_PLATFORM:        
      stream << "Invalid platform";    break;
    case CL_INVALID_DEVICE:     
      stream << "Invalid device";    break;
    case CL_INVALID_CONTEXT:        
      stream << "Invalid context";    break;
    case CL_INVALID_QUEUE_PROPERTIES: 
      stream << "Invalid queue properties";    break;
    case CL_INVALID_COMMAND_QUEUE:          
      stream << "Invalid command queue";    break;
    case CL_INVALID_HOST_PTR:            
      stream << "Invalid host pointer";    break;
    case CL_INVALID_MEM_OBJECT:              
      stream << "Invalid memory object";    break;
    case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:  
      stream << "Invalid image format descriptor";    break;
    case CL_INVALID_IMAGE_SIZE:           
      stream << "Invalid image size";    break;
    case CL_INVALID_SAMPLER:     
      stream << "Invalid sampler";    break;
    case CL_INVALID_BINARY:                    
      stream << "Invalid binary";    break;
    case CL_INVALID_BUILD_OPTIONS:           
      stream << "Invalid build options";    break;
    case CL_INVALID_PROGRAM:               
      stream << "Invalid program";
    case CL_INVALID_PROGRAM_EXECUTABLE:  
      stream << "Invalid program executable";    break;
    case CL_INVALID_KERNEL_NAME:         
      stream << "Invalid kernel name";    break;
    case CL_INVALID_KERNEL_DEFINITION:      
      stream << "Invalid kernel definition";    break;
    case CL_INVALID_KERNEL:               
      stream << "Invalid kernel";    break;
    case CL_INVALID_ARG_INDEX:           
      stream << "Invalid argument index";    break;
    case CL_INVALID_ARG_VALUE:               
      stream << "Invalid argument value";    break;
    case CL_INVALID_ARG_SIZE:              
      stream << "Invalid argument size";    break;
    case CL_INVALID_KERNEL_ARGS:           
      stream << "Invalid kernel arguments";    break;
    case CL_INVALID_WORK_DIMENSION:       
      stream << "Invalid work dimension";    break;
      break;
    case CL_INVALID_WORK_GROUP_SIZE:          
      stream << "Invalid work group size";    break;
      break;
    case CL_INVALID_WORK_ITEM_SIZE:      
      stream << "Invalid work item size";    break;
      break;
    case CL_INVALID_GLOBAL_OFFSET: 
      stream << "Invalid global offset";    break;
      break;
    case CL_INVALID_EVENT_WAIT_LIST: 
      stream << "Invalid event wait list";    break;
      break;
    case CL_INVALID_EVENT:                
      stream << "Invalid event";    break;
      break;
    case CL_INVALID_OPERATION:       
      stream << "Invalid operation";    break;
      break;
    case CL_INVALID_GL_OBJECT:              
      stream << "Invalid OpenGL object";    break;
      break;
    case CL_INVALID_BUFFER_SIZE:          
      stream << "Invalid buffer size";    break;
      break;
    case CL_INVALID_MIP_LEVEL:             
      stream << "Invalid mip-map level";   
      break;  
    default: 
      stream << "Unknown";
      break;
    }
  return stream.str();
 }
