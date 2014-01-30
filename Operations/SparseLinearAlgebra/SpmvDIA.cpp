#include "../../sm.h"
#include "../../em.h"
#include "../../lm.h"
#include "../../Types/Array.h"
#include "../../Types/SpMat.h"
#include <cassert>

class SpmvDIAElementLevel : public ElementLevel
{
  public:
    ArrayOperand * dstOperand;
    SpMatOperand * srcOperand1;
    ArrayOperand * srcOperand2;
    int dim;
    int ndiags;
    SpmvDIAElementLevel(ArrayOperand * dstOperand, 
                        SpMatOperand * srcOperand1,
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

void SpmvDIAElementLevel::compile(std::stringstream & ss)
{
  CodeTemplate ct = CodeTemplate();
  ct.addLine("<y> = 0.0;");
  ct.addLine("for(int diag = 0 ; diag < <NDIAGS> ; diag++)");
  ct.addLine("{");
  ct.addLine("  int offset = <A_OFFSETS>;");
  ct.addLine("  if((element_id0 + offset >= 0) && (element_id0 + offset < <DIM>))");
  ct.addLine("  {");
  ct.addLine("    <y> += <A> * <x>;");
  ct.addLine("  }");
  ct.addLine("}");

  ct.replace("y", dstOperand->getElement());
  ct.replace("A", srcOperand1->getDataElement("diag", "offset", dim));
  ct.replace("A_OFFSETS", srcOperand1->getOffsetElement("diag"));
  ct.replace("x", srcOperand2->getElement("offset"));
  ct.replace("DIM", dim);
  ct.replace("NDIAGS", ndiags);
  ss << ct.getText() << std::endl;
}

// Semantic model node generates execution nodes and strands while generating the execution model
void SpmvDIASetupLaunch(std::vector<EStmt*> & estmts,  std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt)
{
  SpMat * spmat = dynamic_cast<SpMat*>(vars[1]);
  Array * a1 = dynamic_cast<Array*>(vars[2]);
  Array * a2 = dynamic_cast<Array*>(vars[0]);

  int nd = constantTable["nd1"]->get_property_int("value");
  int nt = constantTable["nt1"]->get_property_int("value");
  int ne = constantTable["ne1"]->get_property_int("value");
  int dim = a1->getDim();
  int ndiags = spmat->get_property_int("ndiags");
  int nblocks = (((dim + ne-1)/ne + nt-1) / nt);

  // Operands
  ArrayOperand * dstOperand = new ArrayOperand(a2, ArrayDevicePartition( nd, dim, nt*ne, ne, 1));
  SpMatOperand * srcOperand1 = new SpMatOperand(spmat, SpMatDevicePartition( nd, dim, nt*ne, ne, 1));
  ArrayOperand * srcOperand2 = new ArrayOperand(a1, ArrayDevicePartition( nd, dim));

  // Strand
  Level * level = new SpmvDIAElementLevel(dstOperand, srcOperand1, srcOperand2, dim, ndiags);
  EStmt * estmt = new EStmt("SpmvDIA", level, LaunchPartition(1, dim, nblocks, nt, ne, ELEMENT));
  // LaunchPartition(1,  (1024, 1024), (128, 128), (32, 32), (2, 2), ELEMENT)
  estmt->addSource(srcOperand1);
  estmt->addSource(srcOperand2);
  estmt->addSink(dstOperand);
  estmts.push_back(estmt);

}


