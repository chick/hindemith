#ifndef __STENCIL_H
#define __STENCIL_H

#include "../lm.h"
#include "../clhelp.h"

class StencilPartition
{
  public:
    int nd;
    StencilPartition();
    StencilPartition(int nd);
};

class StencilOperand : public Operand
{
  public:
    int ndevices;
    StencilPartition partition;
    StencilOperand(Variable * variable);
    StencilOperand(Variable * variable, StencilPartition partition);

    /*virtual*/ MemLevel matches(Operand * cmp);
    /*virtual*/ void callUp(cl_vars_t clv);
    /*virtual*/ void pushDown(cl_vars_t clv);
    /*virtual*/ std::string parameterStr();
    /*virtual*/ void setParameter(cl_vars_t clv, cl_kernel kernel, int & arg_cnt, int device_id);

    std::string getData(std::string diag);
    std::string getOffx(std::string diag);
    std::string getOffy(std::string diag);
    std::string getLoad(MemLevel);
    std::string getStore(MemLevel);
};

#endif
