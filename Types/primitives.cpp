#include "primitives.h"
#include "../clhelp.h"

size_t get_num_bytes(std::string dtype)
{
  if(dtype == "float32")
  {
    return sizeof(float);
  }
  else if(dtype == "float64")
  {
    return sizeof(double);
  }
  else if(dtype == "int64")
  {
    return sizeof(long);
  }
  else if(dtype == "complex64")
  {
    return sizeof(cl_float2);
  }
  return 0;
}

std::string get_ctype(std::string dtype)
{
  if(dtype == "float32")
  {
    return "float";
  }
  else if(dtype == "float64")
  {
    return "double";
  }
  else if(dtype == "int64")
  {
    return "long";
  }
  else if(dtype == "complex64")
  {
    return "float2";
  }
  return "";
}
