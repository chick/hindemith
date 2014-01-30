#include "../../sm.h"
#include "../../em.h"
#include "../../lm.h"
#include "../../Types/Array.h"
#include "../../Types/Scalar.h"

#include <cassert>
#include <iostream>

class ArrayScalarOpElementLevel : public ElementLevel
{
  public:
    ArrayOperand * dstOperand;
    ArrayOperand * srcOperand1;
    ScalarOperand * srcOperand2;
    std::string op;
    ArrayScalarOpElementLevel(ArrayOperand * dstOperand, 
                        ArrayOperand * srcOperand1,
			ScalarOperand * srcOperand2,
			std::string op) :
                        ElementLevel(), 
			dstOperand(dstOperand), 
			srcOperand1(srcOperand1), 
			srcOperand2(srcOperand2), 
			op(op) {};
    void compile(std::stringstream & ss);
};

void ArrayScalarOpElementLevel::compile(std::stringstream & ss)
{
  CodeTemplate ct = CodeTemplate();
  ct.addLine("<a> = <b> <op> <c>;");

  ct.replace("a", dstOperand->getElement());
  ct.replace("b", srcOperand1->getElement());
  ct.replace("c", srcOperand2->getElement());

  if(op == "Add")
  {
    ct.replace("op", "+");
  }
  else if(op == "Sub")
  {
    ct.replace("op", "-");
  }
  else if(op == "Mult")
  {
    ct.replace("op", "*");
  }
  else if(op == "Div")
  {
    ct.replace("op", "/");
  }
  ss << ct.getText() << std::endl;
}

// Semantic model node generates execution nodes and strands while generating the execution model
void ArrayScalarOpSetupLaunch(std::vector<EStmt*> & estmts,  std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt)
{
  Scalar * scalar = dynamic_cast<Scalar*>(vars[2]);
  Array * a1 = dynamic_cast<Array*>(vars[1]);
  Array * a2 = dynamic_cast<Array*>(vars[0]);

  int nd = constantTable["nd1"]->get_property_int("value");
  int nt = constantTable["nt1"]->get_property_int("value");
  int ne = constantTable["ne1"]->get_property_int("value");
  int dim = a1->getDim();
  int nblocks = (((dim + ne-1)/ne + nt-1) / nt);

  // Operands
  ArrayOperand * dstOperand = new ArrayOperand(a2, ArrayDevicePartition( nd, dim, nt*ne, ne, 1));
  ArrayOperand * srcOperand1 = new ArrayOperand(a1, ArrayDevicePartition( nd, dim, nt*ne, ne, 1));
  ScalarOperand * srcOperand2 = new ScalarOperand(scalar, "", 1);

  // Strand
  Level * level = new ArrayScalarOpElementLevel(dstOperand, srcOperand1, srcOperand2, stmt->properties["op"]);
  EStmt * estmt = new EStmt("ArrayScalarOp", level, LaunchPartition(1, dim, nblocks, nt, ne, ELEMENT));
  estmt->addSource(srcOperand1);
  estmt->addSource(srcOperand2);
  estmt->addSink(dstOperand);
  estmts.push_back(estmt);

}


