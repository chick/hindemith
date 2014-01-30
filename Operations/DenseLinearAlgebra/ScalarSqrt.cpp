#include "../../sm.h"
#include "../../em.h"
#include "../../lm.h"
#include "../../Types/Scalar.h"

#include <cassert>
#include <iostream>

class ScalarSqrtElementLevel : public ElementLevel
{
  public:
    ScalarOperand * dst;
    ScalarOperand * src;
    ScalarSqrtElementLevel(ScalarOperand * dst, 
			  ScalarOperand * src) :
			  ElementLevel(),
			  dst(dst), 
			  src(src) {};
    void compile(std::stringstream & ss);
};

void ScalarSqrtElementLevel::compile(std::stringstream & ss)
{
  CodeTemplate ct = CodeTemplate();
  ct.addLine("<a> = sqrt(<b>);");
  ct.replace("a", dst->getElement());
  ct.replace("b", src->getElement());
  ss << ct.getText() << std::endl;
}

// Semantic model node generates execution nodes and strands while generating the execution model
void ScalarSqrtSetupLaunch(std::vector<EStmt*> & estmts,  std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt)
{
  Scalar * s1 = dynamic_cast<Scalar*>(vars[1]);
  Scalar * s2 = dynamic_cast<Scalar*>(vars[0]);

  ScalarOperand * dstOperand = new ScalarOperand(s2, "", 1);
  ScalarOperand * srcOperand = new ScalarOperand(s1, "", 1);

  // Strand
  Level * level = new ScalarSqrtElementLevel(dstOperand, srcOperand);
  EStmt * estmt = new EStmt("ScalarSqrt", level, LaunchPartition(1,  1, 1, 1, 1, ELEMENT));
  estmt->addSource(srcOperand);
  estmt->addSink(dstOperand);
  estmts.push_back(estmt);
}


