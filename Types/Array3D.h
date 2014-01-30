#ifndef __ARRAY3D_H
#define __ARRAY3D_H
#include "../lm.h"
#include "../em.h"
#include "../clhelp.h"

Array3D * add_array3d(std::string prefix, int * dim, std::string dtype, std::map<std::string, Variable*> & symbolTable, cl_vars_t clv);

class Array3DDevicePartition
{
  public:
    int dim[3]; 
    int nd[3]; // Num devices
    int d[3]; // Items per device
    int b[3]; // Items per block
    int t[3]; // Items per thread
    int e; // Items per loop iteration
    Array3DDevicePartition() {};
    Array3DDevicePartition(int * dim);
    Array3DDevicePartition(int * dim, int * nd);
    Array3DDevicePartition(int * dim, int * nd, int * d);
    Array3DDevicePartition(int * dim, int * nd, int * d, int * b);
    Array3DDevicePartition(int * dim, int * nd, int * d, int * b, int * t);
    Array3DDevicePartition(int * dim, int * nd, int * d, int * b, int * t, int e);
    void print();
};

class Array3DOperand : public Operand
{
  public:
    int ndevices;
    Array3DDevicePartition partition;
    Array3DOperand(Variable * variable);
    Array3DOperand(Variable * variable, Array3DDevicePartition partition);

    /*virtual*/ MemLevel matches(Operand * cmp);
    /*virtual*/ void callUp(cl_vars_t clv);
    /*virtual*/ void pushDown(cl_vars_t clv);
    MemLevel getRegisterLevel();

    std::string get(std::string offset);
    std::string get(std::string offset0, std::string offset1, std::string offset2);
    std::string getElement();
    std::string getElement(std::string offset);
    std::string getElement(std::string offset0, std::string offset1, std::string offset2);
    std::string getThread();
    std::string getThread(std::string offset);
    std::string getThread(std::string offset0, std::string offset1, std::string offset2);
    std::string getBlock();
    std::string getLoad(MemLevel);
    std::string getStore(MemLevel);
  private:
    std::string element_id_string();
    std::string thread_id_string();
    std::string offset_string(std::string offset0, std::string offset1, std::string offset2);
};

#endif
