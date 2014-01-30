#ifndef __ARRAY2D_H
#define __ARRAY2D_H
#include "../lm.h"
#include "../em.h"
#include "../clhelp.h"

Array2D * add_array2d(std::string prefix, int * dim, std::string dtype, std::map<std::string, Variable*> & symbolTable, cl_vars_t clv);

class Array2DDevicePartition
{
  public:
    int dim[2]; 
    int nd[2]; // Num devices
    int d[2]; // Items per device
    int b[2]; // Items per block
    int t[2]; // Items per thread
    int e; // Items per loop iteration
    Array2DDevicePartition() {};
    Array2DDevicePartition(int * dim);
    Array2DDevicePartition(int * dim, int * nd);
    Array2DDevicePartition(int * dim, int * nd, int * d);
    Array2DDevicePartition(int * dim, int * nd, int * d, int * b);
    Array2DDevicePartition(int * dim, int * nd, int * d, int * b, int * t);
    Array2DDevicePartition(int * dim, int * nd, int * d, int * b, int * t, int e);
    void print();
};

class Array2DOperand : public Operand
{
  public:
    int ndevices;
    Array2DDevicePartition partition;
    Array2DOperand(Variable * variable);
    Array2DOperand(Variable * variable, Array2DDevicePartition partition);

    /*virtual*/ MemLevel matches(Operand * cmp);
    /*virtual*/ void callUp(cl_vars_t clv);
    /*virtual*/ void pushDown(cl_vars_t clv);
    MemLevel getRegisterLevel();

    std::string get(std::string offset);
    std::string get(std::string offset0, std::string offset1);
    std::string getElement();
    std::string getElement(std::string offset);
    std::string getElement(std::string offset0, std::string offset1);
    std::string getThread();
    std::string getThread(std::string offset);
    std::string getThread(std::string offset0, std::string offset1);
    std::string getBlock();
    std::string getLoad(MemLevel);
    std::string getStore(MemLevel);
  private:
    std::string element_id_string();
    std::string thread_id_string();
    std::string offset_string(std::string offset0, std::string offset1);
};

#endif
