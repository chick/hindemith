#include "../../sm.h"
#include "../../em.h"
#include "../../lm.h"
#include "../../Types/Array2D.h"
#include "../../Types/Scalar.h"

#include <cassert>
#include <iostream>

namespace Array2DSumNS
{
  class ZeroThreadLevel : public ThreadLevel
  {
    public:
      Array2DOperand * threadOperand;
      ZeroThreadLevel(int p, int b, 
                      Array2DOperand * threadOperand) :
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
  
  class Sum2DElementLevel : public ElementLevel
  {
    public:
      Array2DOperand * srcOperand;
      Array2DOperand * threadOperand;
      Sum2DElementLevel(Array2DOperand * srcOperand,
                      Array2DOperand * threadOperand) :
  		    ElementLevel(),
  		    srcOperand(srcOperand), 
  		    threadOperand(threadOperand) 
  		    {};
      void compile(std::stringstream & ss);
      bool matches(ElementLevel * tl_compare) { return false; } 
  };
  
  void Sum2DElementLevel::compile(std::stringstream & ss)
  {
    CodeTemplate ct = CodeTemplate();
    ct.addLine("<d1> += <s1>;");
    ct.replace("d1", threadOperand->getThread());
    ct.replace("s1", srcOperand->getElement());
    ss << ct.getText() << std::endl;
  }
  
  class Sum2DThreadLevel : public ThreadLevel
  {
    public:
      Array2DOperand * threadOperand;
      Operand * dstOperand;
      int nthreads[2];
      Sum2DThreadLevel(Array2DOperand * threadOperand,
                     Operand * dstOperand,
  		   int * nthreads) :
  		   ThreadLevel(NULL, NULL, NULL, 0),
		   threadOperand(threadOperand),
  		   dstOperand(dstOperand)
  		   {
		     this->nthreads[0] = nthreads[0];
		     this->nthreads[1] = nthreads[1];
		   };
      void compile(std::stringstream & ss);
      bool matches(ThreadLevel * tl_compare) { return false; } 
  };
  
  void Sum2DThreadLevel::compile(std::stringstream & ss)
  {
    CodeTemplate ct = CodeTemplate();
  
    ct.addLine("barrier(CLK_LOCAL_MEM_FENCE);");
    ct.addLine("for(int stride = <NTHREADS1> / 2 ; stride > 0 ; stride = stride >> 1)");
    ct.addLine("{");
    ct.addLine("if(local_id1 < stride)");
    ct.addLine("{");
    ct.addLine("  <VEC> += <VEC_STRIDE1>;");
    ct.addLine("}");
    ct.addLine("  barrier(CLK_LOCAL_MEM_FENCE);");
    ct.addLine("}");
    ct.addLine("for(int stride = <NTHREADS0> / 2 ; stride > 0 ; stride = stride >> 1)");
    ct.addLine("{");
    ct.addLine("if((local_id1 == 0) && (local_id0 < stride))");
    ct.addLine("{");
    ct.addLine("  <VEC> += <VEC_STRIDE0>;");
    ct.addLine("}");
    ct.addLine("  barrier(CLK_LOCAL_MEM_FENCE);");
    ct.addLine("}");
    ct.addLine("if((local_id0 == 0) && (local_id1 == 0))");
    ct.addLine("{");
    ct.addLine("  <DST> = <VEC>;");
    ct.addLine("}");
  
    ct.replace("NTHREADS0", nthreads[0]);
    ct.replace("NTHREADS1", nthreads[1]);
    ct.replace("VEC", threadOperand->getThread());
    ct.replace("VEC_STRIDE0", threadOperand->getThread("stride", "0"));
    ct.replace("VEC_STRIDE1", threadOperand->getThread("0", "stride"));
    Array2DOperand * a2d = dynamic_cast<Array2DOperand*>(dstOperand);
    ScalarOperand * scl = dynamic_cast<ScalarOperand*>(dstOperand);
    if(a2d)
    {
      ct.replace("DST", a2d->getBlock());
    }
    else if(scl)
    {
      ct.replace("DST", scl->getThread());
    }
    ss << ct.getText() << std::endl;
  }
  
}

void Array2DSumSetupLaunch(std::vector<EStmt*> & estmts,  std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt)
{
  Array2D * arr = dynamic_cast<Array2D*>(vars[1]);
  Scalar * scalar = dynamic_cast<Scalar*>(vars[0]);

  int nd0 = constantTable["nd20"]->get_property_int("value");
  int nd1 = constantTable["nd21"]->get_property_int("value");
  int nt0 = constantTable["nt20"]->get_property_int("value");
  int nt1 = constantTable["nt21"]->get_property_int("value");
  int ne0 = constantTable["ne20"]->get_property_int("value");
  int ne1 = constantTable["ne21"]->get_property_int("value");

  int dim[2];
  int ndevices[2];
  int nblocks[2];
  int nthreads[2];
  int nelements[2];
  int devsize[2];
  int blksize[2];

  if(arr->properties["col_mjr"] == "True")
  {
    dim[0] = arr->get_property_int("dim0");
    dim[1] = arr->get_property_int("dim1");
  }
  else
  {
    dim[1] = arr->get_property_int("dim0");
    dim[0] = arr->get_property_int("dim1");
  }

  ndevices[0] = nd0;
  ndevices[1] = nd1;
  nblocks[0] = (((dim[0] + ne0-1)/ne0 + nt0-1) / nt0);
  nblocks[1] = (((dim[1] + ne1-1)/ne1 + nt1-1) / nt1);
  nthreads[0] = nt0;
  nthreads[1] = nt1;
  nelements[0] = ne0;
  nelements[1] = ne1;
  blksize[0] = nthreads[0] * nelements[0];
  blksize[1] = nthreads[1] * nelements[1];
  devsize[0] = dim[0];
  devsize[1] = dim[1];
  int threadsize[2];
  threadsize[0] = nblocks[0] * nthreads[0];
  threadsize[1] = nblocks[1] * nthreads[1];
  int ones[2];
  ones[0] = ones[1] = 1;
  int zeros[2];
  zeros[0] = zeros[1] = 0;

  // Create intermediate variables
  Array2D * threadArray = add_array2d("tmp_thread1", threadsize, arr->properties["dtype"], symbolTable, clv);
  Array2D * blockArray = add_array2d("tmp_block", nblocks, arr->properties["dtype"], symbolTable, clv);
  Array2D * threadArray2 = add_array2d("tmp_thread2", nthreads, arr->properties["dtype"], symbolTable, clv);

  // Set thread_param to zero
  {
    Array2DOperand * threadOperand = new Array2DOperand(threadArray, Array2DDevicePartition( threadsize, ndevices, threadsize, nthreads, ones));
    Level * level = new Array2DSumNS::ZeroThreadLevel(0, 0, threadOperand);
    EStmt * estmt = new EStmt("Array2DSum", level, LaunchPartition(2, threadsize, ndevices, nblocks, nthreads, zeros, THREAD));
    estmt->addSink(threadOperand);
    estmts.push_back(estmt);
  }

  // Element reduction
  {
    Array2DOperand * srcOperand = new Array2DOperand(arr, Array2DDevicePartition( dim, ndevices, devsize, blksize, nelements, 1));
    Array2DOperand * threadOperand = new Array2DOperand(threadArray, Array2DDevicePartition( threadsize, ndevices, threadsize, nthreads, ones)); 
    Level * level = new Array2DSumNS::Sum2DElementLevel(srcOperand, threadOperand);
    EStmt * estmt = new EStmt("Array2DSum", level, LaunchPartition(2, dim, ndevices, nblocks, nthreads, nelements, ELEMENT));
    estmt->addSource(srcOperand);
    estmt->addSource(threadOperand);
    estmt->addSink(threadOperand);
    estmts.push_back(estmt);
  }

  // Thread reduction
  {
    Array2DOperand * blockOperand = new Array2DOperand(blockArray, Array2DDevicePartition( nblocks, ndevices, nblocks, ones));
    Array2DOperand * threadOperand = new Array2DOperand(threadArray, Array2DDevicePartition( threadsize, ndevices, threadsize, nthreads ));
    Level * level = new Array2DSumNS::Sum2DThreadLevel(threadOperand, (Operand*)blockOperand, nthreads);
    EStmt * estmt = new EStmt("Array2DSum", level, LaunchPartition(2, threadsize, ndevices, nblocks, nthreads, zeros, THREAD));
    estmt->addSource(threadOperand);
    estmt->addSink(threadOperand);
    estmt->addSink(blockOperand);
    estmts.push_back(estmt);
  }

  // SECOND LEVEL

  // Set thread_param to zero
  {
    Array2DOperand * threadOperand = new Array2DOperand(threadArray2, Array2DDevicePartition( nthreads, ndevices, nthreads, nthreads, ones));
    Level * level = new Array2DSumNS::ZeroThreadLevel(0, 0, threadOperand);
    EStmt * estmt = new EStmt("Array2DSum", level, LaunchPartition(2, nthreads, ndevices, ones, nthreads, zeros, THREAD));
    estmt->addSink(threadOperand);
    estmts.push_back(estmt);
  }

  // Element reduction
  {
    int elts_per_thread[2];
    elts_per_thread[0] = (nblocks[0] + nthreads[0] - 1) / nthreads[0];
    elts_per_thread[1] = (nblocks[1] + nthreads[1] - 1) / nthreads[1];
    Array2DOperand * blockOperand = new Array2DOperand(blockArray, Array2DDevicePartition( nblocks, ndevices, nblocks, nblocks, elts_per_thread, 1));
    Array2DOperand * threadOperand = new Array2DOperand(threadArray2, Array2DDevicePartition( nthreads, ndevices, nthreads, nthreads, ones));
    Level * level = new Array2DSumNS::Sum2DElementLevel(blockOperand, threadOperand);
    EStmt * estmt = new EStmt("Array2DSum", level, LaunchPartition(2,  nblocks, ndevices, ones, nthreads, elts_per_thread, ELEMENT));
    estmt->addSource(blockOperand);
    estmt->addSource(threadOperand);
    estmt->addSink(threadOperand);
    estmts.push_back(estmt);
  }

  // Thread reduction
  {
    Array2DOperand * threadOperand = new Array2DOperand(threadArray2, Array2DDevicePartition( nthreads, ndevices, nthreads, nthreads));
    ScalarOperand * deviceOperand = new ScalarOperand(scalar, "", 1);
    Level * level = new Array2DSumNS::Sum2DThreadLevel(threadOperand, (Operand*)deviceOperand, nthreads);
    EStmt * estmt = new EStmt("Array2DSum", level, LaunchPartition(2, nthreads, ndevices, ones, nthreads, zeros, THREAD));
    estmt->addSource(threadOperand);
    estmt->addSink(threadOperand);
    estmt->addSink(deviceOperand);
    estmts.push_back(estmt);
  }

}

