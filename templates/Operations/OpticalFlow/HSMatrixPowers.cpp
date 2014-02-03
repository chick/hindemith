#include "../../sm.h"
#include "../../em.h"
#include "../../lm.h"
#include "../../Types/Array2D.h"
#include "../../Types/Array3D.h"
#include "../../Types/Scalar.h"
#include <cassert>

class Array2DArray3DCopyElementLevel : public ElementLevel
{
  public:
    Array3DOperand * V0Operand;
    Array3DOperand * V1Operand;
    Array2DOperand * v0Operand;
    Array2DOperand * v1Operand;
    Array2DArray3DCopyElementLevel(Array3DOperand * V0Operand,
    				   Array3DOperand * V1Operand,
				   Array2DOperand * v0Operand,
				   Array2DOperand * v1Operand) :
				   ElementLevel(),
				   V0Operand(V0Operand),
				   V1Operand(V1Operand),
				   v0Operand(v0Operand),
				   v1Operand(v1Operand) {};
    void compile(std::stringstream & ss);
};

void Array2DArray3DCopyElementLevel::compile(std::stringstream & ss)
{

  CodeTemplate ct = CodeTemplate();
  ct.addLine("<UDST> = <USRC>;\n");
  ct.addLine("<VDST> = <VSRC>;\n");
  ct.replace("UDST", V0Operand->get("element_id0", "element_id1", "0"));
  ct.replace("VDST", V1Operand->get("element_id0", "element_id1", "0"));
  ct.replace("USRC", v0Operand->get("element_id0", "element_id1"));
  ct.replace("VSRC", v1Operand->get("element_id0", "element_id1"));
  ss << ct.getText() << std::endl;
}

class HSMatrixPowersElementLevel: public ElementLevel
{
  public:
    Array3DOperand * V0Operand;
    Array3DOperand * V1Operand;
    ScalarOperand * lamOperand;
    Array2DOperand * IxOperand;
    Array2DOperand * IyOperand;
    int width;
    int height;
    int step;
    HSMatrixPowersElementLevel(Array3DOperand * V0Operand,
    			Array3DOperand * V1Operand,
    			ScalarOperand * lamOperand,
    			Array2DOperand * IxOperand,
    			Array2DOperand * IyOperand,
			int width,
			int height,
			int step) :
                        ElementLevel(), 
			V0Operand(V0Operand),
			V1Operand(V1Operand),
			lamOperand(lamOperand),
			IxOperand(IxOperand),
			IyOperand(IyOperand),
			width(width),
			height(height),
			step(step) {};
    void compile(std::stringstream & ss);
};

void HSMatrixPowersElementLevel::compile(std::stringstream & ss)
{
  CodeTemplate ct = CodeTemplate();
  ct.addLine("int step = <STEP>;");
  ct.addLine("<URES> = <IX> * (<IX> * <UMID> + <IY> * <VMID>);");
  ct.addLine("<VRES> = <IY> * (<IX> * <UMID> + <IY> * <VMID>);");
  ct.addLine("if(element_id0 > 0)");
  ct.addLine("{");
  ct.addLine("  <URES> += -<LAM> * <ULEFT>;");
  ct.addLine("  <VRES> += -<LAM> * <VLEFT>;");
  ct.addLine("  <URES> += <LAM> * <UMID>;");
  ct.addLine("  <VRES> += <LAM> * <VMID>;");
  ct.addLine("}");
  ct.addLine("if(element_id0 < <WIDTH>-1)");
  ct.addLine("{");
  ct.addLine("  <URES> += -<LAM> * <URIGHT>;");
  ct.addLine("  <VRES> += -<LAM> * <VRIGHT>;");
  ct.addLine("  <URES> += <LAM> * <UMID>;");
  ct.addLine("  <VRES> += <LAM> * <VMID>;");
  ct.addLine("}");
  ct.addLine("if(element_id1 > 0)");
  ct.addLine("{");
  ct.addLine("  <URES> += -<LAM> * <UUP>;");
  ct.addLine("  <VRES> += -<LAM> * <VUP>;");
  ct.addLine("  <URES> += <LAM> * <UMID>;");
  ct.addLine("  <VRES> += <LAM> * <VMID>;");
  ct.addLine("}");
  ct.addLine("if(element_id1 < <HEIGHT>-1)");
  ct.addLine("{");
  ct.addLine("  <URES> += -<LAM> * <UDOWN>;");
  ct.addLine("  <VRES> += -<LAM> * <VDOWN>;");
  ct.addLine("  <URES> += <LAM> * <UMID>;");
  ct.addLine("  <VRES> += <LAM> * <VMID>;");
  ct.addLine("}");

  ct.replace("LAM", lamOperand->getElement());
  ct.replace("STEP", step);
  ct.replace("URES", V0Operand->get("element_id0", "element_id1", "step"));
  ct.replace("VRES", V1Operand->get("element_id0", "element_id1", "step"));
  ct.replace("ULEFT", V0Operand->get("element_id0-1", "element_id1", "step-1"));
  ct.replace("VLEFT", V1Operand->get("element_id0-1", "element_id1", "step-1"));
  ct.replace("URIGHT", V0Operand->get("element_id0+1", "element_id1", "step-1"));
  ct.replace("VRIGHT", V1Operand->get("element_id0+1", "element_id1", "step-1"));
  ct.replace("UUP", V0Operand->get("element_id0", "element_id1-1", "step-1"));
  ct.replace("VUP", V1Operand->get("element_id0", "element_id1-1", "step-1"));
  ct.replace("UDOWN", V0Operand->get("element_id0", "element_id1+1", "step-1"));
  ct.replace("VDOWN", V1Operand->get("element_id0", "element_id1+1", "step-1"));
  ct.replace("UMID", V0Operand->get("element_id0", "element_id1", "step-1"));
  ct.replace("VMID", V1Operand->get("element_id0", "element_id1", "step-1"));
  ct.replace("IX", IxOperand->getElement());
  ct.replace("IY", IyOperand->getElement());
  ct.replace("WIDTH", width);
  ct.replace("HEIGHT", height);
  ss << ct.getText() << std::endl;
}

// Semantic model node generates execution nodes and strands while generating the execution model
void HSMatrixPowersSetupLaunch(std::vector<EStmt*> & estmts,  std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt)
{
  Array3D * V0 = dynamic_cast<Array3D*>(vars[0]);
  Array3D * V1 = dynamic_cast<Array3D*>(vars[1]);
  Scalar * lam = dynamic_cast<Scalar*>(vars[2]);
  Array2D * Ix = dynamic_cast<Array2D*>(vars[3]);
  Array2D * Iy = dynamic_cast<Array2D*>(vars[4]);
  Array2D * v0 = dynamic_cast<Array2D*>(vars[5]);
  Array2D * v1 = dynamic_cast<Array2D*>(vars[6]);

  assert(V0);
  assert(V1);
  assert(lam);
  assert(Ix);
  assert(Iy);
  assert(v0);
  assert(v1);

  int nd0 = constantTable["nd20"]->get_property_int("value");
  int nd1 = constantTable["nd21"]->get_property_int("value");
  int nt0 = constantTable["nt20"]->get_property_int("value");
  int nt1 = constantTable["nt21"]->get_property_int("value");
  int ne0 = constantTable["ne20"]->get_property_int("value");
  int ne1 = constantTable["ne21"]->get_property_int("value");
  int steps = constantTable["STEPS"]->get_property_int("value");

  int dim[3];
  int ndevices[3];
  int nblocks[2];
  int nthreads[2];
  int nelements[2];
  int devsize[3];
  int blksize[2];

  if(v0->properties["col_mjr"] == "True")
  {
    dim[0] = v0->get_property_int("dim0");
    dim[1] = v0->get_property_int("dim1");
  }
  else
  {
    dim[1] = v0->get_property_int("dim0");
    dim[0] = v0->get_property_int("dim1");
  }
  dim[2] = steps;

  ndevices[0] = nd0;
  ndevices[1] = nd1;
  ndevices[2] = 1;
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
  devsize[2] = steps;
  int nd = ndevices[0] * ndevices[1];

  // Copy sources into Array3D
  {
    Array2DOperand * v0Operand = new Array2DOperand(v0, Array2DDevicePartition( dim, ndevices, dim));
    Array2DOperand * v1Operand = new Array2DOperand(v1, Array2DDevicePartition( dim, ndevices, dim));
    Array3DOperand * V0Operand = new Array3DOperand(V0, Array3DDevicePartition( dim, ndevices, devsize));
    Array3DOperand * V1Operand = new Array3DOperand(V1, Array3DDevicePartition( dim, ndevices, devsize));
    Level * level = new Array2DArray3DCopyElementLevel(V0Operand, V1Operand, v0Operand, v1Operand);
    EStmt * estmt = new EStmt("Array2DArray3DCopy", level, LaunchPartition(2, dim, ndevices, nblocks, nthreads, nelements, ELEMENT));
    estmt->addSource(v0Operand);
    estmt->addSource(v1Operand);
    estmt->addSink(V0Operand);
    estmt->addSink(V1Operand);
    estmts.push_back(estmt);
  }

  // Run matrix powers on Array3D
  for(int s = 1 ; s < steps ; s++)
  {
    Array3DOperand * V0Operand = new Array3DOperand(V0, Array3DDevicePartition( dim, ndevices, devsize));
    Array3DOperand * V1Operand = new Array3DOperand(V1, Array3DDevicePartition( dim, ndevices, devsize));
    ScalarOperand * lamOperand = new ScalarOperand(lam, "", nd);
    Array2DOperand * IxOperand = new Array2DOperand(Ix, Array2DDevicePartition( dim, ndevices, devsize, blksize, nelements, 1));
    Array2DOperand * IyOperand = new Array2DOperand(Iy, Array2DDevicePartition( dim, ndevices, devsize, blksize, nelements, 1));

    // Strand
    Level * level = new HSMatrixPowersElementLevel(V0Operand, V1Operand, lamOperand, IxOperand, IyOperand, dim[0], dim[1], s);
    EStmt * estmt = new EStmt("HSMatrixPowers", level, LaunchPartition(2, dim, ndevices, nblocks, nthreads, nelements, ELEMENT));
    estmt->addSource(V0Operand);
    estmt->addSource(V1Operand);
    estmt->addSource(lamOperand);
    estmt->addSource(IxOperand);
    estmt->addSource(IyOperand);
    estmt->addSink(V0Operand);
    estmt->addSink(V1Operand);
    estmts.push_back(estmt);
  }

}


