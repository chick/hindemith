#include "../../sm.h"
#include "../../em.h"
#include "../../lm.h"
#include "../../Types/Array2D.h"
#include "../../Types/Scalar.h"
#include "../../Types/Stencil.h"
#include <cassert>

class ApplyHSPreconditionerElementLevel : public ElementLevel
{
  public:
    Array2DOperand * dstOperand0;
    Array2DOperand * dstOperand1;
    StencilOperand * stencilOperand;
    Array2DOperand * diagOperand0;
    Array2DOperand * diagOperand1;
    Array2DOperand * offdiagOperand;
    Array2DOperand * srcOperand0;
    Array2DOperand * srcOperand1;
    int dim0;
    int dim1;
    ApplyHSPreconditionerElementLevel(Array2DOperand * dstOperand0,
    			Array2DOperand * dstOperand1,
    			StencilOperand * stencilOperand,
			Array2DOperand * diagOperand0,
			Array2DOperand * diagOperand1,
			Array2DOperand * offdiagOperand,
			Array2DOperand * srcOperand0,
			Array2DOperand * srcOperand1,
			int dim0,
			int dim1) :
                        ElementLevel(), 
			dstOperand0(dstOperand0), 
			dstOperand1(dstOperand1), 
			stencilOperand(stencilOperand),
			diagOperand0(diagOperand0),
			diagOperand1(diagOperand1),
			offdiagOperand(offdiagOperand),
			srcOperand0(srcOperand0),
			srcOperand1(srcOperand1),
			dim0(dim0),
			dim1(dim1) {};
    void compile(std::stringstream & ss);
};

void ApplyHSPreconditionerElementLevel::compile(std::stringstream & ss)
{
  CodeTemplate ct = CodeTemplate();
  ct.addLine("float coef_a = <A>;");
  ct.addLine("float coef_d = <D>;");
  ct.addLine("if(element_id0 < <DIM0> - 1)");
  ct.addLine("{");
  ct.addLine("  coef_a += <STENCIL_VAL>;");
  ct.addLine("  coef_d += <STENCIL_VAL>;");
  ct.addLine("}");
  ct.addLine("if(element_id1 < <DIM1> - 1)");
  ct.addLine("{");
  ct.addLine("  coef_a += <STENCIL_VAL>;");
  ct.addLine("  coef_d += <STENCIL_VAL>;");
  ct.addLine("}");
  ct.addLine("if(element_id0 > 0)");
  ct.addLine("{");
  ct.addLine("  coef_a += <STENCIL_VAL>;");
  ct.addLine("  coef_d += <STENCIL_VAL>;");
  ct.addLine("}");
  ct.addLine("if(element_id1 > 0)");
  ct.addLine("{");
  ct.addLine("  coef_a += <STENCIL_VAL>;");
  ct.addLine("  coef_d += <STENCIL_VAL>;");
  ct.addLine("}");

  ct.addLine("float det = coef_a*coef_d-<B>*<C>;");
  ct.addLine("if(fabs(det) > 1e-15)");
  ct.addLine("{");
  ct.addLine("  <DST0> = (<E>*coef_d - <B>*<F>)/det;");
  ct.addLine("  <DST1> = (-<C>*<E> + coef_a*<F>)/det;");
  ct.addLine("}");
  ct.addLine("else");
  ct.addLine("{");
  ct.addLine("  <DST0> = <E>;");
  ct.addLine("  <DST1> = <F>;");
  ct.addLine("}");
  //ct.addLine("  <DST0> = <E>;");
  //ct.addLine("  <DST1> = <F>;");
  ct.replace("A", diagOperand0->getElement());
  ct.replace("B", offdiagOperand->getElement());
  ct.replace("C", offdiagOperand->getElement());
  ct.replace("D", diagOperand1->getElement());
  ct.replace("E", srcOperand0->getElement());
  ct.replace("F", srcOperand1->getElement());
  ct.replace("STENCIL_VAL", stencilOperand->getData("2"));
  ct.replace("DST0", dstOperand0->getElement());
  ct.replace("DST1", dstOperand1->getElement());
  ct.replace("DIM0", dim0);
  ct.replace("DIM1", dim1);
  ss << ct.getText() << std::endl;
}

// Semantic model node generates execution nodes and strands while generating the execution model
void ApplyHSPreconditionerSetupLaunch(std::vector<EStmt*> & estmts,  std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt)
{
  Array2D * vec_out0 = dynamic_cast<Array2D*>(vars[0]);
  Array2D * vec_out1 = dynamic_cast<Array2D*>(vars[1]);
  Stencil * stencil = dynamic_cast<Stencil*>(vars[2]);
  Array2D * diag0 = dynamic_cast<Array2D*>(vars[3]);
  Array2D * diag1 = dynamic_cast<Array2D*>(vars[4]);
  Array2D * offdiag = dynamic_cast<Array2D*>(vars[5]);
  Array2D * vec_in0 = dynamic_cast<Array2D*>(vars[6]);
  Array2D * vec_in1 = dynamic_cast<Array2D*>(vars[7]);

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

  if(vec_in0->properties["col_mjr"] == "True")
  {
    dim[0] = vec_in0->get_property_int("dim0");
    dim[1] = vec_in0->get_property_int("dim1");
  }
  else
  {
    dim[1] = vec_in0->get_property_int("dim0");
    dim[0] = vec_in0->get_property_int("dim1");
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
  int nd = ndevices[0] * ndevices[1];

  Array2DOperand * dstOperand0 = new Array2DOperand(vec_out0, Array2DDevicePartition( dim, ndevices, devsize, blksize, nelements, 1));
  Array2DOperand * dstOperand1 = new Array2DOperand(vec_out1, Array2DDevicePartition( dim, ndevices, devsize, blksize, nelements, 1));
  StencilOperand * stencilOperand = new StencilOperand(stencil, StencilPartition( nd ));
  Array2DOperand * diagOperand0 = new Array2DOperand(diag0, Array2DDevicePartition( dim, ndevices, devsize, blksize, nelements, 1));
  Array2DOperand * diagOperand1 = new Array2DOperand(diag1, Array2DDevicePartition( dim, ndevices, devsize, blksize, nelements, 1));
  Array2DOperand * offdiagOperand = new Array2DOperand(offdiag, Array2DDevicePartition( dim, ndevices, devsize, blksize, nelements, 1));
  Array2DOperand * srcOperand0 = new Array2DOperand(vec_in0, Array2DDevicePartition( dim, ndevices, devsize, blksize, nelements, 1));
  Array2DOperand * srcOperand1 = new Array2DOperand(vec_in1, Array2DDevicePartition( dim, ndevices, devsize, blksize, nelements, 1));

  // Strand
  Level * level = new ApplyHSPreconditionerElementLevel(dstOperand0, dstOperand1, stencilOperand, diagOperand0, diagOperand1, offdiagOperand, srcOperand0, srcOperand1, dim[0], dim[1]);
  EStmt * estmt = new EStmt("ApplyHSPreconditioner", level, LaunchPartition(2, dim, ndevices, nblocks, nthreads, nelements, ELEMENT));
  estmt->addSource(stencilOperand);
  estmt->addSource(diagOperand0);
  estmt->addSource(diagOperand1);
  estmt->addSource(offdiagOperand);
  estmt->addSource(srcOperand0);
  estmt->addSource(srcOperand1);
  estmt->addSink(dstOperand0);
  estmt->addSink(dstOperand1);
  estmts.push_back(estmt);

}


