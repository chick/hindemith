#include "../../sm.h"
#include "../../em.h"
#include "../../lm.h"
#include "../../Types/Scalar.h"
#include "../../Types/Array3D.h"

#include <cassert>
#include <iostream>

class ExtractSnapshotsElementLevel : public ElementLevel
{
  public:
    Array3DOperand * snapshotsOperand;
    Array3DOperand * datacubeOperand;
    int tdof, ndop, training_block_size;
    ExtractSnapshotsElementLevel(Array3DOperand * snapshotsOperand, 
                        Array3DOperand * datacubeOperand,
			int tdof, int ndop, int training_block_size
			) :
			snapshotsOperand(snapshotsOperand), 
			datacubeOperand(datacubeOperand), 
			tdof(tdof), ndop(ndop), training_block_size(training_block_size)
			{}
    void compile(std::stringstream & ss);
};

void ExtractSnapshotsElementLevel::compile(std::stringstream & ss)
{
  CodeTemplate ct = CodeTemplate();

  ct.addLine("int chan = element_id0 / <TDOF>;");
  ct.addLine("int dof = element_id0 % <TDOF>;");
  ct.addLine("int problem_id = element_id2;");
  ct.addLine("int block = problem_id / <N_DOP>;");
  ct.addLine("int cell = element_id1 + block * <TRAINING_BLOCK_SIZE>;");
  ct.addLine("int dop_index = problem_id % <N_DOP>;");
  ct.addLine("int dop = dop_index - (<TDOF>-1)/2 + dof;");
  ct.addLine("if(dop < 0)");
  ct.addLine("{");
  ct.addLine("  dop += <N_DOP>;");
  ct.addLine("}");
  ct.addLine("else if(dop >= <N_DOP>)");
  ct.addLine("{");
  ct.addLine("  dop -= <N_DOP>;");
  ct.addLine("}");

  ct.addLine("<DST> = <SRC>;");

  ct.replace("DST", snapshotsOperand->getElement());
  ct.replace("SRC", datacubeOperand->get("chan", "dop", "cell"));
  ct.replace("TRAINING_BLOCK_SIZE", training_block_size);
  ct.replace("N_DOP", ndop);
  ct.replace("TDOF", tdof);

  ss << ct.getText() << std::endl;
}

// Semantic model node generates execution nodes and strands while generating the execution model
void ExtractSnapshotsSetupLaunch(std::vector<EStmt*> & estmts,  std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt)
{
  Array3D * a0 = dynamic_cast<Array3D*>(vars[0]);
  Array3D * a1 = dynamic_cast<Array3D*>(vars[1]);

  assert(a0);
  assert(a1);

  int nd0 = constantTable["nd20"]->get_property_int("value");
  int nd1 = constantTable["nd21"]->get_property_int("value");
  int nt0 = constantTable["nt20"]->get_property_int("value");
  int nt1 = constantTable["nt21"]->get_property_int("value");
  int ne0 = constantTable["ne20"]->get_property_int("value");
  int ne1 = constantTable["ne21"]->get_property_int("value");

  int ndevices[3];
  int nblocks[3];
  int nthreads[3];
  int nelements[3];

  ndevices[0] = nd0;
  ndevices[1] = nd1;
  ndevices[2] = 1;
  nthreads[0] = nt0;
  nthreads[1] = nt1;
  nthreads[2] = 1;
  nelements[0] = ne0;
  nelements[1] = ne1;
  nelements[2] = 1;

  int nrange = constantTable["N_RANGE"]->get_property_int("value");
  int training_block_size = constantTable["TRAINING_BLOCK_SIZE"]->get_property_int("value");
  int ndop = constantTable["N_DOP"]->get_property_int("value");
  int tdof = constantTable["TDOF"]->get_property_int("value");
  int n_radar_blocks = constantTable["N_BLOCKS"]->get_property_int("value");
  int nchan = constantTable["N_CHAN"]->get_property_int("value");

  nblocks[0] = (((((nchan * tdof) + ne0-1)/ne0 + nt0-1) / nt0) + nd0 - 1)/nd0;
  nblocks[1] = ((((training_block_size + ne1-1)/ne1 + nt1-1) / nt1) + nd1 - 1)/nd1;
  nblocks[2] = n_radar_blocks * ndop;

  Array3DOperand * snapshotsOperand;
  Array3DOperand * datacubeOperand;

  // Output operands
  {
    int dim[3];
    int devsize[3];
    int blksize[3];
    dim[0] = nchan * tdof; // 18
    dim[1] = training_block_size; // 64;
    dim[2] = n_radar_blocks * ndop; // 4096
    devsize[0] = (dim[0] + nd0 - 1) / nd0;
    devsize[1] = (dim[1] + nd1 - 1) / nd1;
    devsize[2] = dim[2];
    blksize[0] = nthreads[0] * nelements[0];
    blksize[1] = nthreads[1] * nelements[1];
    blksize[1] = 1;
    snapshotsOperand = new Array3DOperand(a0, Array3DDevicePartition( dim, ndevices, devsize, blksize, nelements, 1));
  }

  // Input operands
  {
    int dim[3];
    int devsize[3];
    dim[0] = nchan;
    dim[1] = ndop;
    dim[2] = nrange;
    devsize[0] = (dim[0] + nd0 - 1) / nd0;
    devsize[1] = (dim[1] + nd1 - 1) / nd1;
    devsize[2] = dim[2];
    datacubeOperand = new Array3DOperand(a1, Array3DDevicePartition( dim, ndevices, devsize));
  }

  // Create EStmt
  {
    int dim[3];
    dim[0] = nchan * tdof; // 18
    dim[1] = training_block_size; // 64;
    dim[2] = n_radar_blocks * ndop; // 4096
    Level * level = new ExtractSnapshotsElementLevel(snapshotsOperand, datacubeOperand, tdof, ndop, training_block_size);
    EStmt * estmt = new EStmt("ExtractSnapshots", level, LaunchPartition(3, dim, ndevices, nblocks, nthreads, nelements, ELEMENT));
    estmt->addSource(datacubeOperand);
    estmt->addSink(snapshotsOperand);
    estmts.push_back(estmt);
  }

}


