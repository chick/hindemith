#include "../../sm.h"
#include "../../em.h"
#include "../../lm.h"
#include "../../Types/Array.h"
#include "../../Types/Scalar.h"

#include <cassert>
#include <iostream>

class SumDeviceLevel : public DeviceLevel
{
  public:
    ArrayOperand * deviceOperand;
    ScalarOperand * scalarOperand;
    int ndevices;
    SumDeviceLevel(ArrayOperand * deviceOperand, 
                   ScalarOperand * scalarOperand, 
		   int ndevices) : 
		   DeviceLevel(0), 
		   deviceOperand(deviceOperand), 
		   scalarOperand(scalarOperand), 
		   ndevices(ndevices) {};
    bool matches(DeviceLevel * dl_compare) { printf("YAHHH\n");return false; }
    void execute(cl_vars_t clv);
};

void SumDeviceLevel::execute(cl_vars_t clv)
{
  float * vec = (float *) ((Array*)(deviceOperand->variable))->cpu_data;
  float * scalar = (float *) ((Scalar*)(scalarOperand->variable))->cpu_data;
  *scalar = 0.0;
  for(int i = 0 ; i < ndevices ; i++)
  {
//    printf("Vec: %f\n", vec[i]);
    *scalar += vec[i];
  }
  scalarOperand->callUp(clv);
}

class ZeroThreadLevel : public ThreadLevel
{
  public:
    ArrayOperand * threadOperand;
    ZeroThreadLevel(int p, int b, 
                    ArrayOperand * threadOperand) :
                     ThreadLevel(NULL, NULL, NULL, 0), 
  		     threadOperand(threadOperand)
  		     {};
    void compile(std::stringstream & ss);
    bool matches(ThreadLevel * tl_compare) { return false; } 
};

void ZeroThreadLevel::compile(std::stringstream & ss)
{
  CodeTemplate ct = CodeTemplate();
  ct.addLine("<v> = 0.0;");
  ct.replace("v", threadOperand->getThread());
  ss << ct.getText() << std::endl;
}

class SumElementLevel : public ElementLevel
{
  public:
    ArrayOperand * srcOperand;
    ArrayOperand * threadOperand;
    SumElementLevel(ArrayOperand * srcOperand,
                    ArrayOperand * threadOperand) :
		    ElementLevel(),
		    srcOperand(srcOperand), 
		    threadOperand(threadOperand) 
		    {};
    void compile(std::stringstream & ss);
    bool matches(ElementLevel * tl_compare) { return false; } 
};

void SumElementLevel::compile(std::stringstream & ss)
{
  CodeTemplate ct = CodeTemplate();
  ct.addLine("<d1> += <s1>;");
  ct.replace("d1", threadOperand->getThread());
  ct.replace("s1", srcOperand->getElement());
  ss << ct.getText() << std::endl;
}

class SumThreadLevel : public ThreadLevel
{
  public:
    ArrayOperand * threadOperand;
    ArrayOperand * blockOperand;
    int nthreads;
    SumThreadLevel(ArrayOperand * threadOperand,
                   ArrayOperand * blockOperand,
		   int nthreads) :
		   ThreadLevel(NULL, NULL, NULL, 0),
		   threadOperand(threadOperand),
		   blockOperand(blockOperand),
		   nthreads(nthreads)
		   {};
    void compile(std::stringstream & ss);
    bool matches(ThreadLevel * tl_compare) { return false; } 
};

void SumThreadLevel::compile(std::stringstream & ss)
{
  CodeTemplate ct = CodeTemplate();

  ct.addLine("barrier(CLK_LOCAL_MEM_FENCE);");
  ct.addLine("for(int stride = <NTHREADS> / 2 ; stride > 0 ; stride = stride >> 1)");
  ct.addLine("{");
  ct.addLine("if(local_id0 < stride)");
  ct.addLine("{");
  ct.addLine("  <VEC> += <VEC_STRIDE>;");
  ct.addLine("}");
  ct.addLine("  barrier(CLK_LOCAL_MEM_FENCE);");
  ct.addLine("}");
  ct.addLine("if(local_id0 == 0)");
  ct.addLine("{");
  ct.addLine("  <BLOCK> = <VEC>;");
  ct.addLine("}");

  ct.replace("NTHREADS", nthreads);
  ct.replace("VEC", threadOperand->getThread());
  ct.replace("VEC_STRIDE", threadOperand->getThread("stride"));
  ct.replace("BLOCK", blockOperand->getBlock());
  ss << ct.getText() << std::endl;
}

class SumThreadLevelArray : public ThreadLevel
{
  public:
    ArrayOperand * threadOperand;
    ArrayOperand * deviceOperand;
    int nthreads;
    SumThreadLevelArray(ArrayOperand * threadOperand,
                   ArrayOperand * deviceOperand,
		   int nthreads) :
		   ThreadLevel(NULL, NULL, NULL, 0),
		   threadOperand(threadOperand),
		   deviceOperand(deviceOperand),
		   nthreads(nthreads)
		   {};
    void compile(std::stringstream & ss);
    bool matches(ThreadLevel * tl_compare) { return false; } 
};

void SumThreadLevelArray::compile(std::stringstream & ss)
{
  CodeTemplate ct = CodeTemplate();
  ct.addLine("barrier(CLK_LOCAL_MEM_FENCE);");
  ct.addLine("for(int stride = <NTHREADS> / 2 ; stride > 0 ; stride = stride >> 1)");
  ct.addLine("{");
  ct.addLine("if(local_id0 < stride)");
  ct.addLine("{");
  ct.addLine("  <VEC> += <VEC_STRIDE>;");
  ct.addLine("}");
  ct.addLine("  barrier(CLK_LOCAL_MEM_FENCE);");
  ct.addLine("}");
  ct.addLine("if(local_id0 == 0)");
  ct.addLine("{");
  ct.addLine("  <SCALAR> = <VEC>;");
  ct.addLine("}");

  ct.replace("NTHREADS", nthreads);
  ct.replace("VEC", threadOperand->getThread());
  ct.replace("VEC_STRIDE", threadOperand->getThread("stride"));
  ct.replace("SCALAR", deviceOperand->getBlock());
  ss << ct.getText() << std::endl;
}

class SumThreadLevelScalar : public ThreadLevel
{
  public:
    ArrayOperand * threadOperand;
    ScalarOperand * scalarOperand;
    int nthreads;
    SumThreadLevelScalar(ArrayOperand * threadOperand,
                   ScalarOperand * scalarOperand,
		   int nthreads) :
		   ThreadLevel(NULL, NULL, NULL, 0),
		   threadOperand(threadOperand),
		   scalarOperand(scalarOperand),
		   nthreads(nthreads)
		   {};
    void compile(std::stringstream & ss);
    bool matches(ThreadLevel * tl_compare) { return false; } 
};

void SumThreadLevelScalar::compile(std::stringstream & ss)
{
  CodeTemplate ct = CodeTemplate();
  ct.addLine("barrier(CLK_LOCAL_MEM_FENCE);");
  ct.addLine("for(int stride = <NTHREADS> / 2 ; stride > 0 ; stride = stride >> 1)");
  ct.addLine("{");
  ct.addLine("if(local_id0 < stride)");
  ct.addLine("{");
  ct.addLine("  <VEC> += <VEC_STRIDE>;");
  ct.addLine("}");
  ct.addLine("  barrier(CLK_LOCAL_MEM_FENCE);");
  ct.addLine("}");
  ct.addLine("if(local_id0 == 0)");
  ct.addLine("{");
  ct.addLine("  <SCALAR> = <VEC>;");
  ct.addLine("}");

  ct.replace("NTHREADS", nthreads);
  ct.replace("VEC", threadOperand->getThread());
  ct.replace("VEC_STRIDE", threadOperand->getThread("stride"));
  ct.replace("SCALAR", scalarOperand->getThread());
  //ct.replace("SCALAR", scalarOperand->getElement());
  ss << ct.getText() << std::endl;
}

void ArraySumSetupLaunch(std::vector<EStmt*> & estmts,  std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt)
{
  Array * arr = dynamic_cast<Array*>(vars[1]);
  Scalar * scalar = dynamic_cast<Scalar*>(vars[0]);

  int nd = constantTable["nd1"]->get_property_int("value");
  int nt = constantTable["nt1"]->get_property_int("value");
  int ne = constantTable["ne1"]->get_property_int("value");
  int dim = arr->getDim();
  int nblocks = (((dim + ne-1)/ne + nt-1) / nt);
  int nthreads = nblocks * nt;

  // Create intermediate variables
  Array * threadArray = add_array("tmp_thread1", nthreads, 0, arr->properties["dtype"], symbolTable, clv);
  Array * blockArray = add_array("tmp_block", nblocks, 0, arr->properties["dtype"], symbolTable, clv);
  Array * threadArray2 = add_array("tmp_thread2", nt, 0, arr->properties["dtype"], symbolTable, clv);

  // Set thread_param to zero
  {
    ArrayOperand * threadOperand = new ArrayOperand(threadArray, ArrayDevicePartition( nd, nthreads, nt, 1, 0));

    Level * level = new ZeroThreadLevel(0, 0, threadOperand);
    EStmt * estmt = new EStmt("ArraySum", level, LaunchPartition(1, nthreads, nblocks, nt, 0, THREAD));
    estmt->addSink(threadOperand);
    estmts.push_back(estmt);
  }

  // Element reduction
  {
    ArrayOperand * srcOperand = new ArrayOperand(arr, ArrayDevicePartition( nd, dim, nt*ne, ne, 1));
    ArrayOperand * threadOperand = new ArrayOperand(threadArray, ArrayDevicePartition( nd, nthreads, nt, 1, 0));
    Level * level = new SumElementLevel(srcOperand, threadOperand);
    EStmt * estmt = new EStmt("ArraySum", level, LaunchPartition(1, dim, nblocks, nt, ne, ELEMENT));
    estmt->addSource(srcOperand);
    estmt->addSource(threadOperand);
    estmt->addSink(threadOperand);
    estmts.push_back(estmt);
  }

  // Thread reduction
  {
    ArrayOperand * blockOperand = new ArrayOperand(blockArray, ArrayDevicePartition( nd, nblocks, 1, 0, 0));
    ArrayOperand * threadOperand = new ArrayOperand(threadArray, ArrayDevicePartition( nd, nthreads, nt, 0, 0));
    Level * level = new SumThreadLevel(threadOperand, blockOperand, nt);
    EStmt * estmt = new EStmt("ArraySum", level, LaunchPartition(1, dim, nblocks, nt, ne, THREAD));
    estmt->addSource(threadOperand);
    estmt->addSink(threadOperand);
    estmt->addSink(blockOperand);
    estmts.push_back(estmt);
  }

  // Set thread_param to zero
  {
    ArrayOperand * threadOperand = new ArrayOperand(threadArray2, ArrayDevicePartition( nd, nt, nt, 1, 0));
    Level * level = new ZeroThreadLevel(0, 0, threadOperand);
    EStmt * estmt = new EStmt("ArraySum", level, LaunchPartition(1, nt, 1, nt, 0, THREAD));
    estmt->addSink(threadOperand);
    estmts.push_back(estmt);
  }

  // Element reduction
  {
    ArrayOperand * blockOperand = new ArrayOperand(blockArray, ArrayDevicePartition( nd, nblocks, 1, 0, 0));
    ArrayOperand * threadOperand2 = new ArrayOperand(threadArray2, ArrayDevicePartition( nd, nt, nt, 1, 0));
    Level * level = new SumElementLevel(blockOperand, threadOperand2);
    EStmt * estmt = new EStmt("ArraySum", level, LaunchPartition(1,  nblocks, 1, nt, (nblocks + nt-1) /nt, ELEMENT));
    estmt->addSource(blockOperand);
    estmt->addSource(threadOperand2);
    estmt->addSink(threadOperand2);
    estmts.push_back(estmt);
  }

  // Thread reduction(s)
  {
    ArrayOperand * threadOperand2 = new ArrayOperand(threadArray2, ArrayDevicePartition( nd, nt, nt, 0, 0));
    ScalarOperand * deviceOperand = new ScalarOperand(scalar, "", 1);
    Level * level = new SumThreadLevelScalar(threadOperand2, deviceOperand, nt);
    EStmt * estmt = new EStmt("ArraySum", level, LaunchPartition(1, nt, 1, nt, 0, THREAD));
    estmt->addSource(threadOperand2);
    estmt->addSink(threadOperand2);
    estmt->addSink(deviceOperand);
    estmts.push_back(estmt);
  }
}

