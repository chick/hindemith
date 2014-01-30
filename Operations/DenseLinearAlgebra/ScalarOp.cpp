#include "../../sm.h"
#include "../../em.h"
#include "../../lm.h"
#include "../../Types/Scalar.h"

#include <cassert>
#include <iostream>

class ScalarOpElementLevel : public ElementLevel
{
  public:
    ScalarOperand * dstOperand;
    ScalarOperand * srcOperand1;
    ScalarOperand * srcOperand2;
    std::string op;
    ScalarOpElementLevel(ScalarOperand * dstOperand, 
                        ScalarOperand * srcOperand1,
			ScalarOperand * srcOperand2,
			std::string op) :
                        ElementLevel(),
			dstOperand(dstOperand), 
			srcOperand1(srcOperand1), 
			srcOperand2(srcOperand2), 
			op(op) {};
    void compile(std::stringstream & ss);
};

void ScalarOpElementLevel::compile(std::stringstream & ss)
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
void ScalarOpSetupLaunch(std::vector<EStmt*> & estmts,  std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv)
{
}
void ScalarOpSetupLaunch(std::vector<EStmt*> & estmts,  std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt)
{
  Scalar * scalar1 = dynamic_cast<Scalar*>(vars[0]);
  Scalar * scalar2 = dynamic_cast<Scalar*>(vars[1]);
  Scalar * scalar3 = dynamic_cast<Scalar*>(vars[2]);

  ScalarOperand * dstOperand = new ScalarOperand(scalar1, "", 1);
  ScalarOperand * srcOperand1 = new ScalarOperand(scalar2, "", 1);
  ScalarOperand * srcOperand2 = new ScalarOperand(scalar3, "", 1);

  // Strand
  Level * level = new ScalarOpElementLevel(dstOperand, srcOperand1, srcOperand2, stmt->properties["op"]);
  EStmt * estmt = new EStmt("ScalarOp", level, LaunchPartition(1,  1, 1, 1, 1, ELEMENT));
  estmt->addSink(dstOperand);
  estmt->addSource(srcOperand1);
  estmt->addSource(srcOperand2);
  estmts.push_back(estmt);

}


