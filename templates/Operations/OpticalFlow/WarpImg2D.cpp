#include "../../sm.h"
#include "../../em.h"
#include "../../lm.h"
#include "../../Types/Array2D.h"

#include <cassert>
#include <iostream>

class WarpImg2DElementLevel : public ElementLevel
{
  public:
    Array2DOperand * dstOperand;
    Array2DOperand * srcOperand;
    Array2DOperand * offsetXOperand;
    Array2DOperand * offsetYOperand;
    int dim0;
    int dim1;
    WarpImg2DElementLevel(Array2DOperand * dstOperand, 
                        Array2DOperand * srcOperand,
			Array2DOperand * offsetXOperand,
			Array2DOperand * offsetYOperand,
			int dim0,
			int dim1) :
			dstOperand(dstOperand), 
			srcOperand(srcOperand), 
			offsetXOperand(offsetXOperand), 
			offsetYOperand(offsetYOperand), 
			dim0(dim0),
			dim1(dim1) {}
    void compile(std::stringstream & ss);
};

void WarpImg2DElementLevel::compile(std::stringstream & ss)
{
  CodeTemplate ct = CodeTemplate();

  ct.addLine("int my_x = (int) <OFFX>;");
  ct.addLine("int my_y = (int) <OFFY>;");
  ct.addLine("float xfrac = <OFFX> - my_x;");
  ct.addLine("float yfrac = <OFFY> - my_y;");
  ct.addLine("if(<OFFX> < 0.0)");
  ct.addLine("{");
  ct.addLine("  my_x--;");
  ct.addLine("  xfrac = 1.0 + xfrac;");
  ct.addLine("}");
  ct.addLine("if(<OFFY> < 0.0)");
  ct.addLine("{");
  ct.addLine("  my_y--;");
  ct.addLine("  yfrac = 1.0 + yfrac;");
  ct.addLine("}");
  ct.addLine("if((element_id0 + my_x >= 0) && (element_id0 + my_x + 1 < <DIM0>) && (element_id1 + my_y >= 0) && (element_id1 + my_y + 1 < <DIM1>) )");
  ct.addLine("{");
  ct.addLine("  <OUT> = 0.0;");
  ct.addLine("  <OUT> += <IN00> * (1.0 - xfrac) * (1.0 - yfrac);");
  ct.addLine("  <OUT> += <IN10> * (xfrac) * (1.0 - yfrac);");
  ct.addLine("  <OUT> += <IN01> * (1.0 - xfrac) * (yfrac);");
  ct.addLine("  <OUT> += <IN11> * (xfrac) * (yfrac);");
  ct.addLine("}");
  ct.addLine("else");
  ct.addLine("{");
  ct.addLine("  <OUT> = <IN>;");
  ct.addLine("}");

  ct.replace("IN00", srcOperand->getElement("my_x", "my_y"));
  ct.replace("IN10", srcOperand->getElement("my_x+1", "my_y"));
  ct.replace("IN01", srcOperand->getElement("my_x", "my_y+1"));
  ct.replace("IN11", srcOperand->getElement("my_x+1", "my_y+1"));
  ct.replace("OFFX", offsetXOperand->getElement());
  ct.replace("OFFY", offsetYOperand->getElement());
  ct.replace("DIM0", dim0);
  ct.replace("DIM1", dim1);
  ct.replace("OUT", dstOperand->getElement());
  ct.replace("IN", srcOperand->getElement());
  ss << ct.getText() << std::endl;
}

// Semantic model node generates execution nodes and strands while generating the execution model
void WarpImg2DSetupLaunch(std::vector<EStmt*> & estmts,  std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt)
{
  Array2D * a0 = dynamic_cast<Array2D*>(vars[0]);
  Array2D * a1 = dynamic_cast<Array2D*>(vars[1]);
  Array2D * a2 = dynamic_cast<Array2D*>(vars[2]);
  Array2D * a3 = dynamic_cast<Array2D*>(vars[3]);

  assert(a0);
  assert(a1);
  assert(a2);
  assert(a3);

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

  if(a1->properties["col_mjr"] == "True")
  {
    dim[0] = a1->get_property_int("dim0");
    dim[1] = a1->get_property_int("dim1");
  }
  else
  {
    dim[1] = a1->get_property_int("dim0");
    dim[0] = a1->get_property_int("dim1");
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

  int total_dim = dim[0] * dim[1];

  // Operands
  Array2DOperand * dstOperand = new Array2DOperand(a0, Array2DDevicePartition( dim, ndevices, devsize, blksize, nelements, 1));
  Array2DOperand * srcOperand = new Array2DOperand(a1, Array2DDevicePartition( dim, ndevices, devsize));
  Array2DOperand * offsetXOperand = new Array2DOperand(a2, Array2DDevicePartition( dim, ndevices, devsize, blksize, nelements, 1));
  Array2DOperand * offsetYOperand = new Array2DOperand(a3, Array2DDevicePartition( dim, ndevices, devsize, blksize, nelements, 1));

  // Create EStmt
  Level * level = new WarpImg2DElementLevel(dstOperand, srcOperand, offsetXOperand, offsetYOperand, dim[0], dim[1]);
  EStmt * estmt = new EStmt("WarpImg2D", level, LaunchPartition(2, dim, ndevices, nblocks, nthreads, nelements, ELEMENT));
  estmt->addSource(srcOperand);
  estmt->addSource(offsetXOperand);
  estmt->addSource(offsetYOperand);
  estmt->addSink(dstOperand);
  estmts.push_back(estmt);

}


