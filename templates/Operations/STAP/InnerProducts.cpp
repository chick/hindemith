#include "../../sm.h"
#include "../../em.h"
#include "../../lm.h"
#include "../../Types/Scalar.h"
#include "../../Types/Array2D.h"
#include "../../Types/Array3D.h"

#include <cassert>
#include <iostream>

class InnerProductsElementLevel : public ElementLevel
{
  public:
    Array3DOperand * dstOperand;
    Array3DOperand * src1Operand;
    Array3DOperand * src2Operand;
    Array2DOperand * gammaOperand;
    int dim;
    int training_block_size;
    int nrhs;
    InnerProductsElementLevel(Array3DOperand * dstOperand, 
                        Array3DOperand * src1Operand,
                        Array3DOperand * src2Operand,
                        Array2DOperand * gammaOperand,
			int dim, int training_block_size, int nrhs
			) :
			dstOperand(dstOperand), 
			src1Operand(src1Operand), 
			src2Operand(src2Operand), 
			gammaOperand(gammaOperand), 
			dim(dim), training_block_size(training_block_size), nrhs(nrhs)
			{}
    void compile(std::stringstream & ss);
};

void InnerProductsElementLevel::compile(std::stringstream & ss)
{
  CodeTemplate ct = CodeTemplate();
  ct.addLine("float tmp_r = 0.0;");
  ct.addLine("float tmp_i = 0.0;");
  ct.addLine("for(int k = 0 ; k < <DIM> ; k++)");
  ct.addLine("{");
  ct.addLine("  tmp_r += <SRC1>.x * <SRC2>.x + <SRC1>.y * <SRC2>.y;");
  ct.addLine("  tmp_i += <SRC1>.x * <SRC2>.y - <SRC1>.y * <SRC2>.x;");
  ct.addLine("}");
  ct.addLine("<DST>.x = tmp_r * <GAMMA>.x - tmp_i * <GAMMA>.y;");
  ct.addLine("<DST>.y = tmp_r * <GAMMA>.y + tmp_i * <GAMMA>.x;");

  ct.replace("DST", dstOperand->getElement());
  ct.replace("SRC1", src1Operand->get("k", "element_id1", "element_id2"));
  ct.replace("SRC2", src2Operand->get("k", "element_id0", "element_id2"));
  ct.replace("GAMMA", gammaOperand->get("element_id0", "element_id2"));
  ct.replace("DIM", dim);
  ct.replace("NRHS", nrhs);
  ct.replace("TRAINING_BLOCK_SIZE", training_block_size);

  ss << ct.getText() << std::endl;
}

// Semantic model node generates execution nodes and strands while generating the execution model
void InnerProductsSetupLaunch(std::vector<EStmt*> & estmts,  std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt)
{
  Array3D * a0 = dynamic_cast<Array3D*>(vars[0]);
  Array3D * a1 = dynamic_cast<Array3D*>(vars[1]);
  Array3D * a2 = dynamic_cast<Array3D*>(vars[2]);
  Array2D * a3 = dynamic_cast<Array2D*>(vars[3]);

  assert(a0);
  assert(a1);
  assert(a2);
  assert(a3);

  int nd0 = constantTable["nd20"]->get_property_int("value");
  int nd1 = constantTable["nd21"]->get_property_int("value");
  int nd2 = 1;
  int nt0 = constantTable["nt20"]->get_property_int("value");
  int nt1 = constantTable["nt21"]->get_property_int("value");
  int nt2 = 1;
  int ne0 = constantTable["ne20"]->get_property_int("value");
  int ne1 = constantTable["ne21"]->get_property_int("value");
  int ne2 = 1;

  int n_problems = constantTable["N_BLOCKS"]->get_property_int("value") * constantTable["N_DOP"]->get_property_int("value");
  int problem_dim = constantTable["TDOF"]->get_property_int("value") * constantTable["N_CHAN"]->get_property_int("value");
  int nrhs = constantTable["N_STEERING"]->get_property_int("value");
  int training_block_size = constantTable["TRAINING_BLOCK_SIZE"]->get_property_int("value");

  int dim[3];
  int ndevices[3];
  int nblocks[3];
  int nthreads[3];
  int nelements[3];
  int devsize[3];
  int blksize[3];

  ndevices[0] = nd0;
  ndevices[1] = nd1;
  ndevices[2] = 1;

  int vdim1[2];
  int vsize1[2];
  vdim1[0] = a1->get_property_int("dim1");
  vdim1[1] = a1->get_property_int("dim0");
  vsize1[0] = vdim1[0];
  vsize1[1] = (vdim1[1] + nd1 - 1) / nd1;

  int vdim2[2];
  int vsize2[2];
  vdim2[0] = a2->get_property_int("dim1");
  vdim2[1] = a2->get_property_int("dim0");
  vsize2[0] = vdim2[0];
  vsize2[1] = (vdim2[1] + nd1 - 1) / nd1;

  int vdim3[2];
  int vsize3[2];
  vdim3[0] = a3->get_property_int("dim1");
  vdim3[1] = a3->get_property_int("dim0");
  vsize3[0] = vdim3[0];
  vsize3[1] = (vdim3[1] + nd1 - 1) / nd1;

  // Operands
  Array3DOperand * dstOperand;
  Array3DOperand * src1Operand;
  Array3DOperand * src2Operand;
  Array2DOperand * src3Operand;

  {
    int dim[3];
    int devsize[3];
    int blksize[3];
    dim[0] = nrhs;
    dim[1] = training_block_size;
    dim[2] = n_problems;
    devsize[0] = dim[0];
    devsize[1] = dim[1];
    devsize[2] = dim[2]; // Add multi device here!
    blksize[0] = dim[0];
    blksize[1] = dim[1];
    blksize[2] = 1;
    dstOperand = new Array3DOperand(a0, Array3DDevicePartition( dim, ndevices, devsize, blksize)); // Can extend this to element level
  }

  {
    int dim[3];
    int devsize[3];
    int blksize[3];
    dim[0] = problem_dim;
    dim[1] = training_block_size;
    dim[2] = n_problems;
    devsize[0] = dim[0];
    devsize[1] = dim[1];
    devsize[2] = dim[2]; // Add multi device here!
    blksize[0] = dim[0];
    blksize[1] = dim[1];
    blksize[2] = 1;
    src1Operand = new Array3DOperand(a1, Array3DDevicePartition( dim, ndevices, devsize, blksize ));
  }

  {
    int dim[3];
    int devsize[3];
    int blksize[3];
    dim[0] = problem_dim;
    dim[1] = nrhs;
    dim[2] = n_problems;
    devsize[0] = dim[0];
    devsize[1] = dim[1];
    devsize[2] = dim[2]; // Add multi device here!
    blksize[0] = dim[0];
    blksize[1] = dim[1];
    blksize[2] = 1;
    src2Operand = new Array3DOperand(a2, Array3DDevicePartition( dim, ndevices, devsize, blksize ));
  }

  {
    int dim[2];
    int devsize[2];
    int blksize[2];
    dim[0] = nrhs;
    dim[1] = n_problems;
    devsize[0] = dim[0];
    devsize[1] = dim[1];
    src3Operand = new Array2DOperand(a3, Array2DDevicePartition( dim, ndevices, devsize )); // Issue here. 2D partition, 3D launch?
  }

  // Create EStmt
  {
    int dim[3];
    dim[0] = nrhs;
    dim[1] = training_block_size;
    dim[2] = n_problems;
    nblocks[0] = ((((dim[0] + ne0-1)/ne0 + nt0-1) / nt0) + nd0 - 1)/nd0;
    nblocks[1] = ((((dim[1] + ne1-1)/ne1 + nt1-1) / nt1) + nd1 - 1)/nd1;
    nblocks[2] = ((((dim[2] + ne2-1)/ne2 + nt2-1) / nt2) + nd2 - 1)/nd2;
    nthreads[0] = nt0;
    nthreads[1] = nt1;
    nthreads[2] = nt2;
    nelements[0] = ne0;
    nelements[1] = ne1;
    nelements[2] = ne2;
    Level * level = new InnerProductsElementLevel(dstOperand, src1Operand, src2Operand, src3Operand, problem_dim, training_block_size, nrhs);
    EStmt * estmt = new EStmt("InnerProducts", level, LaunchPartition(3, dim, ndevices, nblocks, nthreads, nelements, ELEMENT));
    estmt->addSource(src1Operand);
    estmt->addSource(src2Operand);
    estmt->addSource(src3Operand);
    estmt->addSink(dstOperand);
    estmts.push_back(estmt);
  }
}


