#ifndef __SPMAT_H
#define __SPMAT_H

#include "../lm.h"
#include "../clhelp.h"

class SpMatDevicePartition 
{
  public:
    int nd;
    int d;
    int b;
    int t;
    int e;
    SpMatDevicePartition() {};
    SpMatDevicePartition(int nd, int d, int b, int t, int e);
};

class SpMatOperand : public Operand
{
  public:
    SpMatDevicePartition partition;
    SpMatOperand(Variable * variable);
    SpMatOperand(Variable * variable, SpMatDevicePartition partition);

    /*virtual*/ MemLevel matches(Operand * cmp);
    /*virtual*/ void callUp(cl_vars_t clv);
    /*virtual*/ void pushDown(cl_vars_t clv);
    /*virtual*/ std::string parameterStr();
    /*virtual*/ void setParameter(cl_vars_t clv, cl_kernel kernel, int & arg_cnt, int device_id);

    std::string getDataElement(std::string diag, std::string offset, int dim);
    std::string getData(std::string);
    std::string getOffsetElement(std::string diag);
    std::string getLoad(MemLevel);
    std::string getStore(MemLevel);
};

#endif
