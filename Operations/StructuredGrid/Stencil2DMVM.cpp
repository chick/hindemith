#include "../../sm.h"
#include "../../em.h"
#include "../../lm.h"
#include "../../Types/Array2D.h"
#include "../../Types/Scalar.h"
#include "../../Types/Stencil.h"
#include <cassert>

class Stencil2DMVMElementLevel : public ElementLevel
{
  public:
    Array2DOperand * dstOperand;
    StencilOperand * stencilOperand;
    Array2DOperand * srcOperand;
    int ndiags;
    int width;
    int height;
    Stencil2DMVMElementLevel(Array2DOperand * dstOperand,
    			StencilOperand * stencilOperand,
			Array2DOperand * srcOperand,
			int ndiags,
			int width,
			int height) :
                        ElementLevel(), 
			dstOperand(dstOperand), 
			stencilOperand(stencilOperand),
			srcOperand(srcOperand),
			ndiags(ndiags),
			width(width),
			height(height) {};
    void compile(std::stringstream & ss);
};

void Stencil2DMVMElementLevel::compile(std::stringstream & ss)
{
  CodeTemplate ct = CodeTemplate();
  ct.addLine("<y> = 0.0;");
  ct.addLine("for(int diag = 0 ; diag < <NDIAGS> ; diag++)");
  ct.addLine("{");
  ct.addLine("  int my_x = element_id0 + <OFFX>;");
  ct.addLine("  int my_y = element_id1 + <OFFY>;");
  ct.addLine("  my_x = MAX(my_x, 0);");
  ct.addLine("  my_x = MIN(my_x, <WIDTH>-1);");
  ct.addLine("  my_y = MAX(my_y, 0);");
  ct.addLine("  my_y = MIN(my_y, <HEIGHT>-1);");
  ct.addLine("  int off = my_x + my_y * <WIDTH>;");
  ct.addLine("  <y> += <D> * <x>;");
  ct.addLine("}");

  ct.replace("y", dstOperand->getElement());
  ct.replace("OFFX", stencilOperand->getOffx("diag"));
  ct.replace("OFFY", stencilOperand->getOffy("diag"));
  ct.replace("D", stencilOperand->getData("diag"));
  ct.replace("x", srcOperand->get("off"));
  ct.replace("WIDTH", width);
  ct.replace("HEIGHT", height);
  ct.replace("NDIAGS", ndiags);
  ss << ct.getText() << std::endl;
}

// Semantic model node generates execution nodes and strands while generating the execution model
void Stencil2DMVMSetupLaunch(std::vector<EStmt*> & estmts,  std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt)
{
  Array2D * vec_out = dynamic_cast<Array2D*>(vars[0]);
  Stencil * stencil = dynamic_cast<Stencil*>(vars[1]);
  Array2D * vec_in = dynamic_cast<Array2D*>(vars[2]);

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

  if(vec_in->properties["col_mjr"] == "True")
  {
    dim[0] = vec_in->get_property_int("dim0");
    dim[1] = vec_in->get_property_int("dim1");
  }
  else
  {
    dim[1] = vec_in->get_property_int("dim0");
    dim[0] = vec_in->get_property_int("dim1");
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
  int ndiags = stencil->get_property_int("ndiags");
  int nd = ndevices[0] * ndevices[1];

  // Operands
  Array2DOperand * dstOperand = new Array2DOperand(vec_out, Array2DDevicePartition( dim, ndevices, devsize, blksize, nelements, 1));
  StencilOperand * stencilOperand = new StencilOperand(stencil, StencilPartition( nd ));
  Array2DOperand * srcOperand = new Array2DOperand(vec_in, Array2DDevicePartition( dim, ndevices, dim));

  // Strand
  Level * level = new Stencil2DMVMElementLevel(dstOperand, stencilOperand, srcOperand, ndiags, dim[0], dim[1]);
  EStmt * estmt = new EStmt("Stencil2DMVM", level, LaunchPartition(2, dim, ndevices, nblocks, nthreads, nelements, ELEMENT));
  estmt->addSource(stencilOperand);
  estmt->addSource(srcOperand);
  estmt->addSink(dstOperand);
  estmts.push_back(estmt);

}


