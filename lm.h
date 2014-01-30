#ifndef __LM_H
#define __LM_H

#include <vector>
#include <set>
#include <string>
#include "clhelp.h"

class PlatformLevel;
class DeviceLevel;
class BlockLevel;
class ThreadLevel;
class ElementLevel;

/* Operands */
enum MemLevel { NOLEVEL=-1, PLATFORM=0, DEVICE=1, BLOCK=2, THREAD=3, ELEMENT=4 };
class Variable;
class Operand
{
  public:
    Variable * variable;
    MemLevel level;
    Operand(Variable * variable);

    virtual MemLevel matches(Operand * o_compare) = 0;
    virtual void callUp(cl_vars_t clv) = 0;
    virtual void pushDown(cl_vars_t clv) = 0;
    std::string getStr();
    virtual std::string getDeclaration(MemLevel mlev);
    virtual std::string getLoad(MemLevel) = 0;
    virtual std::string getStore(MemLevel) = 0;
    virtual MemLevel getRegisterLevel();
};

struct OperandEqual
{
  bool operator()(const Operand * p1, const Operand * p2) const;
};

/* Levels */
class EStmt;
class Level
{
  public:
    int id;
    std::vector<Operand*> sources;
    std::vector<Operand*> sinks;
    std::vector<Variable*> parameters;
    std::vector<Operand*> parameterList;
    std::vector<EStmt*> stmts;
    std::set<Variable*> liveIns;
    std::set<Variable*> liveOuts;
    Level();
    void buildParameterSet(); 
    virtual void setLiveVariables(std::map<EStmt*, std::set<Variable*> > liveInMap, std::map<EStmt*, std::set<Variable*> > liveOutMap) = 0;
    virtual void setOperand(Operand * operand, bool is_source) = 0;
    virtual void dumpStmts(int tab, std::stringstream & ss) = 0;
    std::string getStr();
};

class EStmt;
class PlatformLevel : public Level
{
  public:
    std::vector<DeviceLevel*> children;
    PlatformLevel() {};

    // Recursive functions that can be overridden
    virtual bool matches(PlatformLevel* pl_compare);
    virtual void compile(cl_vars_t clv);
    virtual void execute(cl_vars_t clv);
    void setLiveVariables(std::map<EStmt*, std::set<Variable*> > liveInMap, std::map<EStmt*, std::set<Variable*> > liveOutMap);
    void setOperand(Operand * operand, bool is_source);
    /*virtual*/ void dumpStmts(int tab, std::stringstream & ss);
    /*virtual*/ std::string getStr();
};

class DeviceLevel : public Level
{
  public:
    int numDevices;
    std::vector<BlockLevel*> blockList;
    DeviceLevel(int numDevices) : numDevices(numDevices) {};

    // Recursive functions that can be overridden
    virtual bool matches(DeviceLevel * dl_compare);
    virtual void compile(cl_vars_t clv);
    virtual void execute(cl_vars_t clv);
    void setLiveVariables(std::map<EStmt*, std::set<Variable*> > liveInMap, std::map<EStmt*, std::set<Variable*> > liveOutMap);
    void setOperand(Operand * operand, bool is_source);
    /*virtual*/ void dumpStmts(int tab, std::stringstream & ss);
    /*virtual*/ std::string getStr();
};

#define MAX_LDIM 3
class BlockLevel : public Level
{
  public:
    size_t local[MAX_LDIM];
    size_t global[MAX_LDIM];
    size_t ndevices[MAX_LDIM];
    cl_kernel kernel[MAX_DEVICES];
    cl_program program[MAX_DEVICES];
    cl_event event[MAX_DEVICES];
    int ldim;
    std::vector<ThreadLevel*> children;
    BlockLevel(size_t * local_in, size_t * global_in, int ldim, size_t * ndevices_in) : ldim(ldim)
    {
      for(int l = 0 ; l < ldim ; l++)
      {
        local[l] = local_in[l];
	global[l] = global_in[l];
	ndevices[l] = ndevices_in[l];
      }
    };

    // Recursive functions that can be overridden
    virtual bool matches(BlockLevel * dl_compare);
    virtual void compile(cl_vars_t clv);
    virtual void execute(cl_vars_t clv);
    void setLiveVariables(std::map<EStmt*, std::set<Variable*> > liveInMap, std::map<EStmt*, std::set<Variable*> > liveOutMap);
    void setOperand(Operand * operand, bool is_source);
    /*virtual*/ void dumpStmts(int tab, std::stringstream & ss);

    // Helper functions
    std::string parameterStr(); 
    void setParameters(cl_vars_t clv, int devId); 
    /*virtual*/ std::string getStr();
};

class ThreadLevel : public Level
{
  public:
    int threadsPerBlock[MAX_LDIM];
    int elementsPerThread[MAX_LDIM];
    int dim[MAX_LDIM];
    int ldim;
    std::vector<ElementLevel*> children;
    std::string text;
    ThreadLevel(int * threadsPerBlock_in, int * elementsPerThread_in, int * dim_in, int ldim_in) 
    {
      ldim = ldim_in;
      for(int l = 0 ; l < ldim ; l++)
      {
        threadsPerBlock[l] = threadsPerBlock_in[l];
	elementsPerThread[l] = elementsPerThread_in[l];
	dim[l] = dim_in[l];
      }
    };

    // Recursive functions that can be overridden
    virtual bool matches(ThreadLevel * tl_compare);
    virtual void compile(std::stringstream & ss);
    void setLiveVariables(std::map<EStmt*, std::set<Variable*> > liveInMap, std::map<EStmt*, std::set<Variable*> > liveOutMap);
    void setOperand(Operand * operand, bool is_source);
    /*virtual*/ void dumpStmts(int tab, std::stringstream & ss);
    /*virtual*/ std::string getStr();
};

class ElementLevel : public Level
{
  public:
    ElementLevel() {};

    // Recursive functions that can be overridden
    virtual bool matches(ElementLevel * el_compare);
    virtual void compile(std::stringstream & ss) = 0;
    void setLiveVariables(std::map<EStmt*, std::set<Variable*> > liveInMap, std::map<EStmt*, std::set<Variable*> > liveOutMap);
    void setOperand(Operand * operand, bool is_source);
    /*virtual*/ void dumpStmts(int tab, std::stringstream & ss);
    /*virtual*/ std::string getStr();
};

class LaunchPartition
{
  public:
    int ldim;
    int d_offset;
    int dim[MAX_LDIM];
    int ndevices[MAX_LDIM];
    int nblocks[MAX_LDIM];
    int nthreads[MAX_LDIM];
    int nelements[MAX_LDIM];
    MemLevel level;
    LaunchPartition(int ldim, int dim, int ndevices, int nblocks, int nthreads, int nelements, MemLevel level); 
    LaunchPartition(int ldim, int * dim, int * ndevices, int * nblocks, int * nthreads, int * nelements, MemLevel level);
    LaunchPartition(int ldim, int dim, int nblocks, int nthreads, int nelements, MemLevel level); 
    LaunchPartition(int ldim, int * dim, int * nblocks, int * nthreads, int * nelements, MemLevel level);
};

class CodeTemplate
{
  public:
    std::string text;
    CodeTemplate();
    void addLine(std::string line);
    void clearText();
    void replace(std::string, std::string);
    void replace(std::string, int);
    void replace(std::map<std::string, std::string> vars);
    void replaceLine(std::string toReplace, std::string replaceWith);
    std::string getText();
};


EStmt * createStmt(LaunchPartition * launch, Level * levelInstance);
void createStrand(LaunchPartition launch, EStmt * estmt, Level * levelInstance, PlatformLevel * & p);
void createStrand(LaunchPartition launch, EStmt * estmt, Level * levelInstance, DeviceLevel * & d);
void createStrand(LaunchPartition launch, EStmt * estmt, Level * levelInstance, BlockLevel * & b);
void createStrand(LaunchPartition launch, EStmt * estmt, Level * levelInstance, ThreadLevel * & t);

#endif
