#ifndef __SCALAR_H
#define __SCALAR_H
#include "../lm.h"
#include "../clhelp.h"
#include "../em.h"

class ScalarOperand : public Operand
{
  public:
    std::string val; // Immediate
    int ndevices;
    ScalarOperand(Variable * variable, std::string val, int ndevices);

    /*virtual*/ MemLevel matches(Operand * cmp);
    /*virtual*/ void callUp(cl_vars_t clv);
    /*virtual*/ void pushDown(cl_vars_t clv);
    /*virtual*/ std::string parameterStr();
    /*virtual*/ void setParameter(cl_vars_t clv, cl_kernel kernel, int & arg_cnt, int device_id);
    MemLevel getRegisterLevel();

    std::string getElement();
    std::string getThread();
    std::string getLoad(MemLevel);
    std::string getStore(MemLevel);
};

#endif
