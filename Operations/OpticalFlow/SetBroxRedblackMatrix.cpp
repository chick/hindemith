#include "../../sm.h"
#include "../../em.h"
#include "../../lm.h"
#include "../../Types/Array2D.h"
#include "../../Types/Scalar.h"
#include "../../Types/SpMat2D.h"
#include <cassert>
#include <iostream>

class SetBroxRedblackMatrixElementLevel : public ElementLevel
{
  public:
    Array2DOperand * AdiagOperand;
    SpMat2DOperand * spmatOperand;
    Array2DOperand * psiOperand;
    ScalarOperand * alphaOperand;
    int dim0;
    int dim1;
    SetBroxRedblackMatrixElementLevel(Array2DOperand * AdiagOperand,
    			SpMat2DOperand * spmatOperand, 
    			Array2DOperand * psiOperand,
			ScalarOperand * alphaOperand,
			int dim0,
			int dim1 
			) :
                        ElementLevel(), 
			AdiagOperand(AdiagOperand), 
			spmatOperand(spmatOperand), 
			psiOperand(psiOperand),
			alphaOperand(alphaOperand), 
			dim0(dim0),
			dim1(dim1)
			{};
    void compile(std::stringstream & ss);
};

void SetBroxRedblackMatrixElementLevel::compile(std::stringstream & ss)
{
  CodeTemplate ct = CodeTemplate();

  ct.addLine("// SetBroxRedblackMatrix");
  ct.addLine("<ADIAG> = 0.0;");

  // Set top
  ct.addLine("if(element_id1 > 0)");
  ct.addLine("{");
  ct.addLine("<A_TOP> = -<ALPHA> * <PSI_UP>;");
  ct.addLine("<ADIAG> += <ALPHA> * <PSI_UP>;");
  ct.addLine("}");

  ct.addLine("if(element_id0 > 0)");
  ct.addLine("{");
  ct.addLine("<A_LEFT> = -<ALPHA> * <PSI_LEFT>;");
  ct.addLine("<ADIAG> += <ALPHA> * <PSI_LEFT>;");
  ct.addLine("}");

  ct.addLine("if(element_id0 < <DIM0> - 1)");
  ct.addLine("{");
  ct.addLine("<A_RIGHT> = -<ALPHA> * <PSI>;");
  ct.addLine("<ADIAG> += <ALPHA> * <PSI>;");
  ct.addLine("}");

  ct.addLine("if(element_id1 < <DIM1> - 1)");
  ct.addLine("{");
  ct.addLine("<A_BOTTOM> = -<ALPHA> * <PSI>;");
  ct.addLine("<ADIAG> += <ALPHA> * <PSI>;");
  ct.addLine("}");

  ct.replace("PSI_LEFT", psiOperand->getElement("-1", "0"));
  ct.replace("PSI_UP", psiOperand->getElement("0", "-1"));
  ct.replace("PSI", psiOperand->getElement());
  ct.replace("A_TOP", spmatOperand->getDataElement("0", "0", "-1"));
  ct.replace("A_LEFT", spmatOperand->getDataElement("1", "-1", "0"));
  ct.replace("ADIAG", AdiagOperand->getElement());
  ct.replace("A_RIGHT", spmatOperand->getDataElement("2", "1", "0"));
  ct.replace("A_BOTTOM", spmatOperand->getDataElement("3", "0", "1"));
  ct.replace("ALPHA", alphaOperand->getElement());
  ct.replace("DIM0", dim0);
  ct.replace("DIM1", dim1); 
  ss << ct.getText() << std::endl;
}

// Semantic model node generates execution nodes and strands while generating the execution model
void SetBroxRedblackMatrixSetupLaunch(std::vector<EStmt*> & estmts,  std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt)
{
  Array2D * Adiag = dynamic_cast<Array2D*>(vars[0]);
  SpMat2D * spmat = dynamic_cast<SpMat2D*>(vars[1]);
  Array2D * psi = dynamic_cast<Array2D*>(vars[2]);
  Scalar * alpha = dynamic_cast<Scalar*>(vars[3]);

  int nd0 = constantTable["nd20"]->get_property_int("value");
  int nd1 = constantTable["nd21"]->get_property_int("value");
  int nt0 = constantTable["nt20"]->get_property_int("value");
  int nt1 = constantTable["nt21"]->get_property_int("value");
  int ne0 = constantTable["ne20"]->get_property_int("value");
  int ne1 = constantTable["ne21"]->get_property_int("value");
  int nd = nd0 * nd1;

  int dim[2];
  int ndevices[2];
  int nblocks[2];
  int nthreads[2];
  int nelements[2];
  int devsize[2];
  int blksize[2];

  if(psi->properties["col_mjr"] == "True")
  {
    dim[0] = psi->get_property_int("dim0");
    dim[1] = psi->get_property_int("dim1");
  }
  else
  {
    dim[1] = psi->get_property_int("dim0");
    dim[0] = psi->get_property_int("dim1");
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

  // Operands
  Array2DOperand * AdiagOperand = new Array2DOperand(Adiag, Array2DDevicePartition( dim, ndevices, devsize, blksize, nelements, 1));
  SpMat2DOperand * spmatOperand = new SpMat2DOperand(spmat, SpMat2DDevicePartition( dim, ndevices, devsize, blksize, nelements, 1));
  Array2DOperand * psiOperand = new Array2DOperand(psi, Array2DDevicePartition( dim, ndevices, devsize));
  ScalarOperand * alphaOperand = new ScalarOperand(alpha, "", nd);

  // Strand
  Level * level = new SetBroxRedblackMatrixElementLevel(AdiagOperand, spmatOperand, psiOperand, alphaOperand, dim[0], dim[1]);
  EStmt * estmt = new EStmt("SetBroxRedblackMatrix", level, LaunchPartition(2, dim, ndevices, nblocks, nthreads, nelements, ELEMENT));

  estmt->addSource(spmatOperand);
  estmt->addSource(psiOperand);
  estmt->addSource(alphaOperand);
  estmt->addSink(spmatOperand);
  estmt->addSink(AdiagOperand);
  estmts.push_back(estmt);

}


