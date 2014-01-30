#include "../../sm.h"
#include "../../em.h"
#include "../../lm.h"
#include "../../Types/Array3D.h"
#include "../../Types/Scalar.h"

#include <cassert>
#include <iostream>

class ScalarArray3DOpElementLevel : public ElementLevel
{
  public:
    Array3DOperand * dstOperand;
    ScalarOperand * srcOperand1;
    Array3DOperand * srcOperand2;
    std::string op;
    ScalarArray3DOpElementLevel(Array3DOperand * dstOperand, 
                        ScalarOperand * srcOperand1,
			Array3DOperand * srcOperand2,
			std::string op) :
                        ElementLevel(), 
			dstOperand(dstOperand), 
			srcOperand1(srcOperand1), 
			srcOperand2(srcOperand2), 
			op(op) {};
    void compile(std::stringstream & ss);
};

void ScalarArray3DOpElementLevel::compile(std::stringstream & ss)
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
void ScalarArray3DOpSetupLaunch(std::vector<EStmt*> & estmts,  std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt)
{
  Scalar * scalar = dynamic_cast<Scalar*>(vars[1]);
  Array3D * a1 = dynamic_cast<Array3D*>(vars[2]);
  Array3D * a2 = dynamic_cast<Array3D*>(vars[0]);

  int nd0 = constantTable["nd20"]->get_property_int("value");
  int nd1 = constantTable["nd21"]->get_property_int("value");
  int nd2 = 1;
  int nt0 = constantTable["nt20"]->get_property_int("value");
  int nt1 = constantTable["nt21"]->get_property_int("value");
  int nt2 = 1;
  int ne0 = constantTable["ne20"]->get_property_int("value");
  int ne1 = constantTable["ne21"]->get_property_int("value");
  int ne2 = 1;

  int dim[3];
  int ndevices[3];
  int nblocks[3];
  int nthreads[3];
  int nelements[3];
  int devsize[3];
  int blksize[3];

  if(a1->properties["col_mjr"] == "True")
  {
    dim[0] = a1->get_property_int("dim0");
    dim[1] = a1->get_property_int("dim1");
    dim[2] = a1->get_property_int("dim2");
  }
  else
  {
    dim[2] = a1->get_property_int("dim0");
    dim[1] = a1->get_property_int("dim1");
    dim[0] = a1->get_property_int("dim2");
  }

  ndevices[0] = nd0;
  ndevices[1] = nd1;
  ndevices[2] = nd2;
  nblocks[0] = ((((dim[0] + ne0-1)/ne0 + nt0-1) / nt0) + nd0 - 1)/nd0;
  nblocks[1] = ((((dim[1] + ne1-1)/ne1 + nt1-1) / nt1) + nd1 - 1)/nd1;
  nblocks[2] = ((((dim[2] + ne2-1)/ne2 + nt2-1) / nt2) + nd2 - 1)/nd2;
  nthreads[0] = nt0;
  nthreads[1] = nt1;
  nthreads[2] = nt2;
  nelements[0] = ne0;
  nelements[1] = ne1;
  nelements[2] = ne2;
  blksize[0] = nthreads[0] * nelements[0];
  blksize[1] = nthreads[1] * nelements[1];
  blksize[2] = nthreads[2] * nelements[2];
  devsize[0] = (dim[0] + nd0 - 1) / nd0;
  devsize[1] = (dim[1] + nd1 - 1) / nd1;
  devsize[2] = (dim[2] + nd2 - 1) / nd2;
  int nd = nd0 * nd1 * nd2;

  // Operands
  Array3DOperand * dstOperand = new Array3DOperand(a2, Array3DDevicePartition( dim, ndevices, devsize, blksize, nelements, 1));
  Array3DOperand * srcOperand2 = new Array3DOperand(a1, Array3DDevicePartition( dim, ndevices, devsize, blksize, nelements, 1));
  ScalarOperand * srcOperand1 = new ScalarOperand(scalar, "", nd);

  // Strand
  Level * level = new ScalarArray3DOpElementLevel(dstOperand, srcOperand1, srcOperand2, stmt->properties["op"]);
  EStmt * estmt = new EStmt("ScalarArray3DOp", level, LaunchPartition(3, dim, ndevices, nblocks, nthreads, nelements, ELEMENT));
  estmt->addSource(srcOperand1);
  estmt->addSource(srcOperand2);
  estmt->addSink(dstOperand);
  estmts.push_back(estmt);

}


