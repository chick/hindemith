#include "../../sm.h"
#include "../../em.h"
#include "../../lm.h"
#include "../../Types/Array2D.h"

#include <cassert>
#include <iostream>

class Array2DSqrtElementLevel : public ElementLevel
{
  public:
    Array2DOperand * dst;
    Array2DOperand * src;
    Array2DSqrtElementLevel(Array2DOperand * dst, 
			  Array2DOperand * src) :
			  dst(dst), 
			  src(src)
			  {}

    void compile(std::stringstream & ss);
};

void Array2DSqrtElementLevel::compile(std::stringstream & ss)
{
  CodeTemplate ct = CodeTemplate();
  ct.addLine("<a> = sqrt(<b>);");
  ct.replace("a", dst->getElement());
  ct.replace("b", src->getElement());
  ss << ct.getText() << std::endl;
}

// Semantic model node generates execution nodes and strands while generating the execution model
void Array2DSqrtSetupLaunch(std::vector<EStmt*> & estmts,  std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt)
{
  Array2D * a1 = dynamic_cast<Array2D*>(vars[1]);
  Array2D * a2 = dynamic_cast<Array2D*>(vars[0]);

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
  nblocks[0] = ((((dim[0] + ne0-1)/ne0 + nt0-1) / nt0) + nd0 - 1)/nd0;
  nblocks[1] = ((((dim[1] + ne1-1)/ne1 + nt1-1) / nt1) + nd1 - 1)/nd1;
  nthreads[0] = nt0;
  nthreads[1] = nt1;
  nelements[0] = ne0;
  nelements[1] = ne1;
  blksize[0] = nthreads[0] * nelements[0];
  blksize[1] = nthreads[1] * nelements[1];
  devsize[0] = (dim[0] + nd0 - 1) / nd0;
  devsize[1] = (dim[1] + nd1 - 1) / nd1;

  // Operands
  Array2DOperand * srcOperand = new Array2DOperand(a1, Array2DDevicePartition( dim, ndevices, devsize, blksize, nelements, 1) );
  Array2DOperand * dstOperand = new Array2DOperand(a2, Array2DDevicePartition( dim, ndevices, devsize, blksize, nelements, 1) );

  // Strand
  Level * level = new Array2DSqrtElementLevel(dstOperand, srcOperand);
  EStmt * estmt = new EStmt("Array2DSqrt", level, LaunchPartition(2, dim, ndevices, nblocks, nthreads, nelements, ELEMENT));

  // Add operands
  estmt->addSource(srcOperand);
  estmt->addSink(dstOperand);

  // Enqueue estmt
  estmts.push_back(estmt);

}


