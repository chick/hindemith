#ifndef __EM_H
#define __EM_H
#include "sm.h"
#include "lm.h"

/* Helper classes */

class EParam
{
  public:
    Variable * variable;
    std::string val;
    EParam() {};
    std::string getStr();
};

struct EEqual 
{
  bool operator()(const EParam * p1, const EParam * p2)  const 
  {
    if(!p1->variable || !p2->variable)
    {
      return false;
    }
    return (p1->variable->properties["name"] < p2->variable->properties["name"]);
  }
};

/* Execution Nodes */

class ENode 
{
  public:
    ENode() {};
    virtual void dumpStmts(int tab, std::stringstream & ss);
};

class EPipeAndFilter;
class ESemanticModel : public ENode
{
  public:
    EPipeAndFilter * body;
    /*virtual*/ void dumpStmts(int tab, std::stringstream & ss);
};

class EFunctionDef : public ENode
{
  public:
    EPipeAndFilter * body;
    /*virtual*/ void dumpStmts(int tab, std::stringstream & ss);
};

class EIterator : public ENode
{
  public:
    int ub;
    EPipeAndFilter * body;
    /*virtual*/ void dumpStmts(int tab, std::stringstream & ss);
};

class EPipeAndFilter : public ENode
{
  public:
    std::vector<ENode *> stmts;
    /*virtual*/ void dumpStmts(int tab, std::stringstream & ss);
};

class EWhile : public ENode
{
  public:
    EParam * test;
    EPipeAndFilter * body;
    /*virtual*/ void dumpStmts(int tab, std::stringstream & ss);
};

static int estmt_count = 0;
class EStmt : public ENode
{
  public:
    int id;
    std::string label;
    std::vector<EParam *> sources;
    std::vector<EParam *> sinks;
    std::string element_string;
    PlatformLevel * p;
    EStmt() {id = estmt_count++; p = NULL;};
    EStmt(std::string label) : label(label) {id = estmt_count++; p=NULL;};
    EStmt(std::string label, Level * level, LaunchPartition partition);
    /*virtual*/ void dumpStmts(int tab, std::stringstream & ss);
    void addSource(Operand* sourceOperand);
    void addSink(Operand * sinkOperand);
    std::string getStr();
};

class EReturn : public EStmt
{
  public:
    EParam * retparam;
    /*virtual*/ void dumpStmts(int tab, std::stringstream & ss);
};

class EBasicBlock;
class EBasicBlock : public ENode
{
  public:
    std::vector<EStmt*> stmts;
    std::set<EBasicBlock*> preds;
    std::set<EBasicBlock*> succs;
    int id;
    PlatformLevel * p;
    std::vector<cl_event> events;

    EBasicBlock(int id);
    void addStmt(EStmt * s);
    void addSuccessor(EBasicBlock *BB);
    size_t numStmts();
    void dumpGraph(std::string &s);
    /*virtual*/ void dumpStmts(int tab, std::stringstream & ss);
};

#endif
