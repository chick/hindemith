#include "../../sm.h"
#include "../../em.h"
#include "../../lm.h"
#include "../../Types/Array.h"

#include <cassert>
#include <iostream>

class ArraySqrtElementLevel : public ElementLevel
{
  public:
    ArrayOperand * dst;
    ArrayOperand * src;
    ArraySqrtElementLevel(ArrayOperand * dst, 
			  ArrayOperand * src) :
			  dst(dst), 
			  src(src)
			  {}

    void compile(std::stringstream & ss);
};

void ArraySqrtElementLevel::compile(std::stringstream & ss)
{
  CodeTemplate ct = CodeTemplate();
  ct.addLine("<a> = sqrt(<b>);");
  ct.replace("a", dst->getElement());
  ct.replace("b", src->getElement());
  ss << ct.getText() << std::endl;
}

// Semantic model node generates execution nodes and strands while generating the execution model
void ArraySqrtSetupLaunch(std::vector<EStmt*> & estmts,  std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt)
{
  Array * a1 = dynamic_cast<Array*>(vars[1]);
  Array * a2 = dynamic_cast<Array*>(vars[0]);

  int nd = constantTable["nd1"]->get_property_int("value");
  int nt = constantTable["nt1"]->get_property_int("value");
  int ne = constantTable["ne1"]->get_property_int("value");
  int dim = a1->getDim();
  int nblocks = (((dim + ne-1)/ne + nt-1) / nt);

  // Operands
  ArrayOperand * srcOperand = new ArrayOperand(a1, ArrayDevicePartition( nd, dim, nt*ne, ne, 1) );
  ArrayOperand * dstOperand = new ArrayOperand(a2, ArrayDevicePartition( nd, dim, nt*ne, ne, 1) );

  // Strand
  Level * level = new ArraySqrtElementLevel(dstOperand, srcOperand);
  EStmt * estmt = new EStmt("ArraySqrt", level, LaunchPartition(1,  dim, nblocks, nt, ne, ELEMENT));

  // Add operands
  estmt->addSource(srcOperand);
  estmt->addSink(dstOperand);

  // Enqueue estmt
  estmts.push_back(estmt);

}


