#include "../../sm.h"
#include "../../em.h"
#include "../../lm.h"
#include "../../Types/Scalar.h"
#include "../../Types/Array2D.h"
#include "../../Types/Array3D.h"

#include <cassert>
#include <iostream>

namespace GammaWeightsNS
{
  class GS8 : public ThreadLevel
  {
    public:
      Array2DOperand * dst;
      Array3DOperand * weights;
      Array2DOperand * src2;
      int dim, nrhs;
      
      GS8(Array2DOperand * dst,
	 Array3DOperand * weights,
	 Array2DOperand * src2,
	 int dim, int nrhs) : 
	 ThreadLevel(NULL,NULL,NULL,0), 
	 dst(dst),
	 weights(weights),
	 src2(src2),
	 dim(dim), nrhs(nrhs)
	 {};
      void compile(std::stringstream & ss);
  };

  void GS8::compile(std::stringstream & ss)
  {
    CodeTemplate ct = CodeTemplate();
    ct.addLine("__local float acc_r[8][16];");
    ct.addLine("__local float acc_i[8][16];");
    ct.addLine("for(int j = local_id1 ; j < <NRHS> ; j += 8)");
    ct.addLine("{");
    ct.addLine("  acc_r[local_id0][j] = 0.0;");
    ct.addLine("  acc_i[local_id0][j] = 0.0;");
    ct.addLine("  for(int i = local_id0 ; i < <DIM> ; i += 8)");
    ct.addLine("  {");
    ct.addLine("    acc_r[local_id0][j] += <SRC1>.x * <SRC2>.x + <SRC1>.y * <SRC2>.y;");
    ct.addLine("    acc_i[local_id0][j] += <SRC1>.x * <SRC2>.y - <SRC1>.y * <SRC2>.x;");
    ct.addLine("  }");
    ct.addLine("}");
    ct.addLine("barrier(CLK_LOCAL_MEM_FENCE);");
    ct.addLine("// Local sum");
    ct.addLine("for(int offset = 4 ; offset > 0 ; offset = offset >> 1)");
    ct.addLine("{");
    ct.addLine("  if(local_id0 < offset)");
    ct.addLine("  { ");
    ct.addLine("    for(int j = local_id1 ; j < <NRHS> ; j += 8)");
    ct.addLine("    {");
    ct.addLine("      acc_r[local_id0][j] += acc_r[local_id0+offset][j];");
    ct.addLine("      acc_i[local_id0][j] += acc_i[local_id0+offset][j];");
    ct.addLine("    }");
    ct.addLine("  }");
    ct.addLine("  barrier(CLK_LOCAL_MEM_FENCE);");
    ct.addLine("}");
    ct.addLine("if(local_id0 == 0)");
    ct.addLine("{");
    ct.addLine("    for(int j = local_id1 ; j < <NRHS> ; j += 8)");
    ct.addLine("    {");
    ct.addLine("      float r = acc_r[0][j];");
    ct.addLine("      float i = acc_i[0][j];");
    ct.addLine("      float mag = r * r + i * i;");
    ct.addLine("      <DST>.x = r / mag;");
    ct.addLine("      <DST>.y = i / mag;");
    ct.addLine("    }");
    ct.addLine("}");

    ct.replace("DST", dst->get("j", "thread_id2"));
    ct.replace("SRC1", weights->get("i", "j", "thread_id2"));
    ct.replace("SRC2", src2->get("i", "j"));
    ct.replace("DIM", dim);
    ct.replace("NRHS", nrhs);
    ss << ct.getText() << std::endl;
  }

} // Namespace

// Semantic model node generates execution nodes and strands while generating the execution model
void GammaWeightsSetupLaunch(std::vector<EStmt*> & estmts,  std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt)
{
  Array2D * a0 = dynamic_cast<Array2D*>(vars[0]);
  Array3D * a1 = dynamic_cast<Array3D*>(vars[1]);
  Array2D * a2 = dynamic_cast<Array2D*>(vars[2]);

  assert(a0);
  assert(a1);
  assert(a2);

  int nd0 = constantTable["nd20"]->get_property_int("value");
  int nd1 = constantTable["nd21"]->get_property_int("value");
  int nd2 = 1;
  int nt0 = constantTable["nt20"]->get_property_int("value");
  int nt1 = constantTable["nt21"]->get_property_int("value");
  int nt2 = 1;
  int ne0 = constantTable["ne20"]->get_property_int("value");
  int ne1 = constantTable["ne21"]->get_property_int("value");
  int ne2 = 1;

  int ndevices[3];
  int nblocks[3];
  int nthreads[3];
  int nelements[3];

  int n_problems = constantTable["N_BLOCKS"]->get_property_int("value") * constantTable["N_DOP"]->get_property_int("value");
  int problem_dim = constantTable["TDOF"]->get_property_int("value") * constantTable["N_CHAN"]->get_property_int("value");
  int nrhs = constantTable["N_STEERING"]->get_property_int("value");

  nblocks[0] = 1;
  nblocks[1] = 1;
  nblocks[2] = (n_problems + nd1 - 1) / nd1;
  nthreads[0] = 8;
  nthreads[1] = 8;
  nthreads[2] = 1;
  nelements[0] = 0;
  nelements[1] = 0;
  nelements[2] = 0;
  ndevices[0] = nd0;
  ndevices[1] = nd1;
  ndevices[2] = nd2;

  Array2DOperand * dstOp;
  Array3DOperand * weightsOp;
  Array2DOperand * src2Op;

  {
    int dim[2];
    int devsize[2];
    dim[0] = nrhs;
    dim[1] = n_problems;
    devsize[0] = dim[0];
    devsize[1] = (dim[1] + nd1 - 1) / nd1;
    dstOp = new Array2DOperand(a0, Array2DDevicePartition( dim, ndevices, devsize ));
  }

  {
    int dim[3];
    int devsize[3];
    dim[0] = problem_dim;
    dim[1] = nrhs;
    dim[2] = n_problems;
    devsize[0] = dim[0];
    devsize[1] = (dim[1] + nd1 - 1) / nd1;
    devsize[2] = (dim[2] + nd2 - 1) / nd2;
    weightsOp = new Array3DOperand(a1, Array3DDevicePartition( dim, ndevices, devsize ));
  }

  {
    int dim[2];
    int devsize[2];
    dim[0] = problem_dim;
    dim[1] = nrhs;
    devsize[0] = dim[0];
    devsize[1] = (dim[1] + nd1 - 1) / nd1;
    src2Op = new Array2DOperand(a2, Array2DDevicePartition( dim, ndevices ));
  }

  {
    int dim[3];
    dim[0] = 8;
    dim[1] = 8;
    dim[2] = n_problems;
    Level * level = new GammaWeightsNS::GS8(dstOp, weightsOp, src2Op, problem_dim, nrhs);
    EStmt * estmt = new EStmt("GammaWeights", level, LaunchPartition(3, dim, ndevices, nblocks, nthreads, nelements, THREAD));

    estmt->addSource(weightsOp);
    estmt->addSource(src2Op);
    estmt->addSink(dstOp);
    estmts.push_back(estmt);
  }

}



