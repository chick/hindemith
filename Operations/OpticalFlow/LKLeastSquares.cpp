#include "../../sm.h"
#include "../../em.h"
#include "../../lm.h"
#include "../../Types/Array2D.h"

#include <cassert>
#include <iostream>

class LKLeastSquaresElementLevel : public ElementLevel
{
  public:
    Array2DOperand * errOperand;
    Array2DOperand * IxOperand;
    Array2DOperand * IyOperand;
    Array2DOperand * duOperand;
    Array2DOperand * dvOperand;
    int dim0;
    int dim1;
    int rad;
    LKLeastSquaresElementLevel(Array2DOperand * errOperand, 
                        Array2DOperand * IxOperand,
			Array2DOperand * IyOperand,
			Array2DOperand * duOperand,
			Array2DOperand * dvOperand,
			int dim0,
			int dim1,
			int rad) :
			errOperand(errOperand), 
			IxOperand(IxOperand), 
			IyOperand(IyOperand), 
			duOperand(duOperand), 
			dvOperand(dvOperand), 
			dim0(dim0),
			dim1(dim1),
			rad(rad){}
    void compile(std::stringstream & ss);
};

void LKLeastSquaresElementLevel::compile(std::stringstream & ss)
{
  CodeTemplate ct = CodeTemplate();

  // if in bounds
  ct.addLine("float Ix2 = 0.0;");
  ct.addLine("float Iy2 = 0.0;");
  ct.addLine("float IxIy = 0.0;");
  ct.addLine("float IxIt = 0.0;");
  ct.addLine("float IyIt = 0.0;");
  ct.addLine("if((element_id0 >= <RAD>) && (element_id1 >= <RAD>) && (element_id0 <= <DIM0> - <RAD> - 1) && (element_id1 <= <DIM1> - <RAD> - 1))");
  ct.addLine("{");

  ct.addLine("for(int x = -<RAD> ; x < <RAD> ; x++)");
  ct.addLine("{");
  ct.addLine("for(int y = -<RAD> ; y < <RAD> ; y++)");
  ct.addLine("{");
  ct.addLine("Ix2 += <IX> * <IX>;");
  ct.addLine("Iy2 += <IY> * <IY>;");
  ct.addLine("IxIy += <IX> * <IY>;");
  ct.addLine("IxIt += <IX> * <IT>;");
  ct.addLine("IyIt += <IY> * <IT>;");
  ct.addLine("}");
  ct.addLine("}");
  ct.addLine("float det = Ix2 * Iy2 - IxIy * IxIy;");
  ct.addLine("if(det != 0)");
  ct.addLine("{");
  ct.addLine("<DU> = (IxIt * Iy2 - IxIy * IyIt) / det;");
  ct.addLine("<DV> = (Ix2 * IyIt - IxIy * IxIt) / det;");
  ct.addLine("}");
  ct.addLine("else");
  ct.addLine("{");
  ct.addLine("<DU> = 0.0;");
  ct.addLine("<DV> = 0.0;");
  ct.addLine("}");
  ct.addLine("}");
  ct.addLine("else");
  ct.addLine("{");
  ct.addLine("<DU> = 0.0;");
  ct.addLine("<DV> = 0.0;");
  ct.addLine("}");

  ct.replace("IX", IxOperand->getElement("x", "y"));
  ct.replace("IY", IyOperand->getElement("x", "y"));
  ct.replace("IT", errOperand->getElement("x", "y"));
  ct.replace("DU", duOperand->getElement());
  ct.replace("DV", dvOperand->getElement());
  ct.replace("DIM0", dim0);
  ct.replace("DIM1", dim1);
  ct.replace("RAD", rad);

  ss << ct.getText() << std::endl;
}

// Semantic model node generates execution nodes and strands while generating the execution model
void LKLeastSquaresSetupLaunch(std::vector<EStmt*> & estmts,  std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt)
{
  Array2D * a0 = dynamic_cast<Array2D*>(vars[0]);
  Array2D * a1 = dynamic_cast<Array2D*>(vars[1]);
  Array2D * a2 = dynamic_cast<Array2D*>(vars[2]);
  Array2D * a3 = dynamic_cast<Array2D*>(vars[3]);
  Array2D * a4 = dynamic_cast<Array2D*>(vars[4]);
  int rad = constantTable["RADIUS"]->get_property_int("value");

  int nd0 = constantTable["nd20"]->get_property_int("value");
  int nd1 = constantTable["nd21"]->get_property_int("value");
  int nt0 = constantTable["nt20"]->get_property_int("value");
  int nt1 = constantTable["nt21"]->get_property_int("value");
  int ne0 = constantTable["ne20"]->get_property_int("value");
  int ne1 = constantTable["ne21"]->get_property_int("value");

  assert(a0);
  assert(a1);
  assert(a2);
  assert(a3);
  assert(a4);

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
  nblocks[0] = (((dim[0] + 1)/2 + 7) / 8);
  nblocks[1] = (((dim[1] + 1)/2 + 7) / 8);
  nthreads[0] = nthreads[1] = 8;
  nelements[0] = nelements[1] = 2;
  blksize[0] = nthreads[0] * nelements[0];
  blksize[1] = nthreads[1] * nelements[1];
  ndevices[0] = nd0;
  ndevices[1] = nd1;
  devsize[0] = dim[0];
  devsize[1] = dim[1];

  int total_dim = dim[0] * dim[1];

  // Operands
  Array2DOperand * errOperand = new Array2DOperand(a2, Array2DDevicePartition( dim, ndevices, devsize));
  Array2DOperand * IxOperand = new Array2DOperand(a3, Array2DDevicePartition( dim, ndevices, devsize));
  Array2DOperand * IyOperand = new Array2DOperand(a4, Array2DDevicePartition( dim, ndevices, devsize));
  Array2DOperand * duOperand = new Array2DOperand(a0, Array2DDevicePartition( dim, ndevices, devsize, blksize, nelements, 1));
  Array2DOperand * dvOperand = new Array2DOperand(a1, Array2DDevicePartition( dim, ndevices, devsize, blksize, nelements, 1));

  // Create EStmt
  Level * level = new LKLeastSquaresElementLevel(errOperand, IxOperand, IyOperand, duOperand, dvOperand, dim[0], dim[1], rad);
  EStmt * estmt = new EStmt("LKLeastSquares", level, LaunchPartition(2, dim, ndevices, nblocks, nthreads, nelements, ELEMENT));
  estmt->addSource(errOperand);
  estmt->addSource(IxOperand);
  estmt->addSource(IyOperand);
  estmt->addSink(duOperand);
  estmt->addSink(dvOperand);
  estmts.push_back(estmt);

}


