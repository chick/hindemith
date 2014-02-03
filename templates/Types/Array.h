#ifndef __ARRAY_H
#define __ARRAY_H
#include "../lm.h"
#include "../em.h"
#include "../clhelp.h"

Array * add_array(std::string prefix, int dim0, int dim1, std::string dtype, std::map<std::string, Variable*> & symbolTable, cl_vars_t clv);

class ArrayDevicePartition
{
  public:
    int nd;
    int d;
    int b; // Items per block
    int t; // Items per thread
    int e; // Items per loop iteration
    ArrayDevicePartition() {};
    ArrayDevicePartition(int nd);
    ArrayDevicePartition(int nd, int d);
    ArrayDevicePartition(int nd, int d, int b);
    ArrayDevicePartition(int nd, int d, int b, int t);
    ArrayDevicePartition(int nd, int d, int b, int t, int e);
};

class ArrayOperand : public Operand
{
  public:
    int ndevices;
    ArrayDevicePartition partition;
    ArrayOperand(Variable * variable);
    ArrayOperand(Variable * variable, ArrayDevicePartition partition);

    /*virtual*/ MemLevel matches(Operand * cmp);
    /*virtual*/ void callUp(cl_vars_t clv);
    /*virtual*/ void pushDown(cl_vars_t clv);
    MemLevel getRegisterLevel();

    std::string get(std::string offset);
    std::string getElement();
    std::string getElement(std::string offset);
    std::string getThread();
    std::string getThread(std::string offset);
    std::string getBlock();
    std::string getLoad(MemLevel);
    std::string getStore(MemLevel);
};

#endif
