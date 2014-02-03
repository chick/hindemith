#ifndef __SPMAT_H
#define __SPMAT_H

#include "../lm.h"
#include "../clhelp.h"

class CSRMatDevicePartition 
{
  public:
    int devId;
    int d_start;
    int d_end;
    int b;
    int t;
    int e;
    CSRMatDevicePartition() {};
    CSRMatDevicePartition(int d_end, int b, int t, int e);
};

class CSRMatOperand : public Operand
{
  public:
    int ndevices;
    std::map<int, CSRMatDevicePartition> partitions;
    CSRMatOperand(Variable * variable);
    CSRMatOperand(Variable * variable, CSRMatDevicePartition partition);

    /*virtual*/ MemLevel matches(Operand * cmp);
    /*virtual*/ void callUp(cl_vars_t clv);
    /*virtual*/ void pushDown(cl_vars_t clv);
    /*virtual*/ std::string parameterStr();
    /*virtual*/ void setParameter(cl_vars_t clv, cl_kernel kernel, int & arg_cnt, int device_id);

    std::string getLoad(MemLevel);
    std::string getStore(MemLevel);

    std::string getData(std::string offset);
    std::string getIndex(std::string offset);
    std::string getIndptrElement();
    std::string getIndptrElement(std::string offset);

};

#endif
