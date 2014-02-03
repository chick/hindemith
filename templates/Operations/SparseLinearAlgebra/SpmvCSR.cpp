#include "../../sm.h"
#include "../../em.h"
#include "../../lm.h"
#include "../../Types/Array.h"
#include "../../Types/CSRMat.h"
#include <cassert>

class SpmvCSRElementLevel : public ElementLevel
{
  public:
    ArrayOperand * dstOperand;
    CSRMatOperand * srcOperand1;
    ArrayOperand * srcOperand2;
    int dim;
    int ndiags;
    SpmvCSRElementLevel(ArrayOperand * dstOperand, 
                        CSRMatOperand * srcOperand1,
			ArrayOperand * srcOperand2,
			int dim,
			int ndiags) :
                        ElementLevel(), 
			dstOperand(dstOperand), 
			srcOperand1(srcOperand1), 
			srcOperand2(srcOperand2),
			dim(dim),
			ndiags(ndiags) {};
    void compile(std::stringstream & ss);
};

void SpmvCSRElementLevel::compile(std::stringstream & ss)
{
  CodeTemplate ct = CodeTemplate();
  ct.addLine("<y> = 0.0;");
  ct.addLine("for(int j = <INDPTR> ; j < <NEXT_INDPTR> ; j++)");
  ct.addLine("{");
  ct.addLine("  int index = <INDEX>;");
  ct.addLine("  <y> += <A> * <x>;");
  ct.addLine("}");

  ct.replace("A", srcOperand1->getData("j"));
  ct.replace("x", srcOperand2->get("index"));
  ct.replace("y", dstOperand->getElement());
  ct.replace("INDEX", srcOperand1->getIndex("j"));
  ct.replace("INDPTR", srcOperand1->getIndptrElement());
  ct.replace("NEXT_INDPTR", srcOperand1->getIndptrElement("1"));
  ss << ct.getText() << std::endl;
}

// Semantic model node generates execution nodes and strands while generating the execution model
void SpmvCSRSetupLaunch(std::vector<EStmt*> & estmts,  std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt)
{
  CSRMat * spmat = dynamic_cast<CSRMat*>(vars[1]);
  Array * a1 = dynamic_cast<Array*>(vars[2]);
  Array * a2 = dynamic_cast<Array*>(vars[0]);

  int dim_m = a2->getDim();
  int dim_n = a1->getDim();
  int nd = constantTable["nd1"]->get_property_int("value");
  int nt = constantTable["nt1"]->get_property_int("value");
  int ne = constantTable["ne1"]->get_property_int("value");
  int ndiags = spmat->get_property_int("ndiags");
  int nblocks = (((dim_m + ne-1)/ne + nt-1) / nt);

  // Operands
  ArrayOperand * dstOperand = new ArrayOperand(a2, ArrayDevicePartition( nd, dim_m, nt*ne, ne, 1));
  CSRMatOperand * srcOperand1 = new CSRMatOperand(spmat, CSRMatDevicePartition( dim_m, nt*ne, ne, 1));
  ArrayOperand * srcOperand2 = new ArrayOperand(a1, ArrayDevicePartition( nd, dim_n));

  // Strand
  Level * level = new SpmvCSRElementLevel(dstOperand, srcOperand1, srcOperand2, dim_m, ndiags);
  EStmt * estmt = new EStmt("SpmvCSR", level, LaunchPartition(1, dim_m, nblocks, nt, ne, ELEMENT));
  estmt->addSource(srcOperand1);
  estmt->addSource(srcOperand2);
  estmt->addSink(dstOperand);
  estmts.push_back(estmt);

}


