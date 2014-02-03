#ifndef __SPMAT_H
#define __SPMAT_H

#include "../lm.h"
#include "../clhelp.h"

class SpMat2DDevicePartition 
{
  public:
    int dim[2]; 
    int nd[2]; // Num devices
    int d[2]; // Items per device
    int b[2]; // Items per block
    int t[2]; // Items per thread
    int e; // Items per loop iteration
    SpMat2DDevicePartition() {};
    SpMat2DDevicePartition(int * dim);
    SpMat2DDevicePartition(int * dim, int * nd);
    SpMat2DDevicePartition(int * dim, int * nd, int * d);
    SpMat2DDevicePartition(int * dim, int * nd, int * d, int * b);
    SpMat2DDevicePartition(int * dim, int * nd, int * d, int * b, int * t);
    SpMat2DDevicePartition(int * dim, int * nd, int * d, int * b, int * t, int e);
};

class SpMat2DOperand : public Operand
{
  public:
    int ndevices;
    SpMat2DDevicePartition partition;
    SpMat2DOperand(Variable * variable);
    SpMat2DOperand(Variable * variable, SpMat2DDevicePartition partition);

    /*virtual*/ MemLevel matches(Operand * cmp);
    /*virtual*/ void callUp(cl_vars_t clv);
    /*virtual*/ void pushDown(cl_vars_t clv);
    /*virtual*/ std::string parameterStr();
    /*virtual*/ void setParameter(cl_vars_t clv, cl_kernel kernel, int & arg_cnt, int device_id);

    std::string getDataElement(std::string diag, std::string offset0, std::string offset1);
    std::string getData(std::string offset);
    std::string getOffxElement(std::string diag);
    std::string getOffyElement(std::string diag);
    std::string getLoad(MemLevel);
    std::string getStore(MemLevel);
};

#endif
