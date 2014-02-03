#ifndef __CLHELP_H
#define __CLHELP_H

#ifdef __linux__
#include "CL/cl.h"
#elif __APPLE__
#include <OpenCL/opencl.h>
#else
#error Unsupported OS
#endif

#include <map>
#include <cstdio>
#include <string>
#include <sstream>

#define MAX_DEVICES 2

typedef struct CLVARS
{
  cl_int err;
  cl_device_id device_ids[MAX_DEVICES];
  cl_uint num_devices;
  cl_platform_id platform;
  cl_context context;
  cl_command_queue commands[MAX_DEVICES];
  cl_kernel memset_kernel;
  cl_program memset_program;

} cl_vars_t;

std::string reportOCLError(cl_int err);

#define CHK_ERR(err) {\
  if(err != CL_SUCCESS) {\
    printf("Error: %s, File: %s, Line: %d\n", reportOCLError(err).c_str(), __FILE__, __LINE__); \
    exit(-1);\
  }\
}

void initialize_ocl(cl_vars_t& cv);
void uninitialize_ocl(cl_vars_t & clv);
void compile_ocl_program(cl_program & program, cl_kernel & kernel, const cl_vars_t cv, const char * cl_src, const char * kname);
void write_ocl_program(const char * cl_src);
void mymemset(cl_vars_t clv, cl_mem ptr, unsigned char val, int nbytes);
void readFile(std::string& fileName, std::string &out); 
double timestamp();
#endif
