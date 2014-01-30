#include "../../sm.h"
#include "../../em.h"
#include "../../lm.h"
#include "../../Types/Array2D.h"
#include "../../Types/SpMat2D.h"
#include <cassert>

class SpmvDIA2DElementLevel : public ElementLevel
{
  public:
    Array2DOperand * dstOperand;
    SpMat2DOperand * srcOperand1;
    Array2DOperand * srcOperand2;
    int dim0;
    int dim1;
    int ndiags;
    SpmvDIA2DElementLevel(Array2DOperand * dstOperand, 
                        SpMat2DOperand * srcOperand1,
			Array2DOperand * srcOperand2,
			int dim0,
			int dim1,
			int ndiags) :
                        ElementLevel(), 
			dstOperand(dstOperand), 
			srcOperand1(srcOperand1), 
			srcOperand2(srcOperand2),
			dim0(dim0),
			dim1(dim1),
			ndiags(ndiags) {};
    void compile(std::stringstream & ss);
};

void SpmvDIA2DElementLevel::compile(std::stringstream & ss)
{
  CodeTemplate ct = CodeTemplate();
  ct.addLine("<y> = 0.0;");
  ct.addLine("for(int diag = 0 ; diag < <NDIAGS> ; diag++)");
  ct.addLine("{");
  ct.addLine("  int offx = <OFFX>;");
  ct.addLine("  int offy = <OFFY>;");
  ct.addLine("  if((element_id0 + offx >= 0) && (element_id0 + offx < <DIM0>) && (element_id1 + offy >= 0) && (element_id1 + offy < <DIM1>))");
  ct.addLine("  {");
  ct.addLine("    <y> += <A> * <x>;");
  ct.addLine("  }");
  ct.addLine("}");

  ct.replace("y", dstOperand->getElement());
  ct.replace("A", srcOperand1->getDataElement("diag", "offx", "offy"));
  ct.replace("OFFX", srcOperand1->getOffxElement("diag"));
  ct.replace("OFFY", srcOperand1->getOffyElement("diag"));
  ct.replace("x", srcOperand2->getElement("offx", "offy"));
  ct.replace("DIM0", dim0);
  ct.replace("DIM1", dim1);
  ct.replace("NDIAGS", ndiags);
  ss << ct.getText() << std::endl;
}

// Semantic model node generates execution nodes and strands while generating the execution model
void SpmvDIA2DSetupLaunch(std::vector<EStmt*> & estmts,  std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt)
{
  SpMat2D * spmat = dynamic_cast<SpMat2D*>(vars[1]);
  Array2D * a1 = dynamic_cast<Array2D*>(vars[2]);
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
  int ndiags = spmat->get_property_int("ndiags");

  // Operands
  Array2DOperand * dstOperand = new Array2DOperand(a2, Array2DDevicePartition( dim, ndevices, devsize, blksize, nelements, 1));
  SpMat2DOperand * srcOperand1 = new SpMat2DOperand(spmat, SpMat2DDevicePartition( dim, ndevices, devsize, blksize, nelements, 1 ));
  Array2DOperand * srcOperand2 = new Array2DOperand(a1, Array2DDevicePartition( dim, ndevices, devsize));

  // Strand
  Level * level = new SpmvDIA2DElementLevel(dstOperand, srcOperand1, srcOperand2, dim[0], dim[1], ndiags);
  EStmt * estmt = new EStmt("SpmvDIA2D", level, LaunchPartition(2, dim, ndevices, nblocks, nthreads, nelements, ELEMENT));
  estmt->addSource(srcOperand1);
  estmt->addSource(srcOperand2);
  estmt->addSink(dstOperand);
  estmts.push_back(estmt);

}


