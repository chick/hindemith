#include "../../sm.h"
#include "../../em.h"
#include "../../lm.h"
#include "../../Types/Scalar.h"

#include <cassert>
#include <iostream>

class ScalarCopyElementLevel : public ElementLevel
{
  public:
    ScalarOperand * dst;
    ScalarOperand * src;
    ScalarCopyElementLevel(ScalarOperand * dst, 
			  ScalarOperand * src) :
			  ElementLevel(),
			  dst(dst), 
			  src(src) {};
    void compile(std::stringstream & ss);
};

void ScalarCopyElementLevel::compile(std::stringstream & ss)
{
  CodeTemplate ct = CodeTemplate();
  ct.addLine("<a> = <b>;");
  ct.replace("a", dst->getElement());
  ct.replace("b", src->getElement());
  ss << ct.getText() << std::endl;
}

void ScalarCopySetupLaunch(std::vector<EStmt*> & estmts,  std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt)
{
  Scalar * s1 = dynamic_cast<Scalar*>(vars[0]);
  Scalar * s2 = dynamic_cast<Scalar*>(vars[1]);

  ScalarOperand * dstOperand = new ScalarOperand(s1, "", 1);
  ScalarOperand * srcOperand = new ScalarOperand(s2, "", 1);

  // Strand
  Level * level = new ScalarCopyElementLevel(dstOperand, srcOperand);
  EStmt * estmt = new EStmt("ScalarCopy", level, LaunchPartition(1,  1, 1, 1, 1, ELEMENT));
  estmt->addSource(srcOperand);
  estmt->addSink(dstOperand);
  estmts.push_back(estmt);
}


