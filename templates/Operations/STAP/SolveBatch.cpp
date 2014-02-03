#include "../../sm.h"
#include "../../em.h"
#include "../../lm.h"
#include "../../Types/Array.h"
#include "../../Types/Scalar.h"
#include "../../Types/Array2D.h"
#include "../../Types/Array3D.h"

#include <cassert>
#include <iostream>

class SolveBatchThreadLevel : public ThreadLevel
{
  public:
    Array3DOperand * dstOperand;
    Array3DOperand * covarianceOperand;
    Array2DOperand * steeringOperand;
    int n_problems, problem_dim, nrhs;
    SolveBatchThreadLevel(Array3DOperand * dstOperand, 
                        Array3DOperand * covarianceOperand,
                        Array2DOperand * steeringOperand,
			int n_problems, int problem_dim, int nrhs
			) :
			ThreadLevel(NULL, NULL, NULL, 0),
			dstOperand(dstOperand), 
			covarianceOperand(covarianceOperand), 
			steeringOperand(steeringOperand), 
			n_problems(n_problems), problem_dim(problem_dim), nrhs(nrhs)
			{}
    void compile(std::stringstream & ss);
};

// 8x8 blocks. load snap, generate matrix, copy matrix, rhs into registers, solve, dot with snap, One kernel.

// matrix = snap' * snap
// x = solve(matrix, rhs)
// out = snap' * x

// Declare augmented matrix in register file ( 18 x 34 )
// Copy in covariances
// Copy in steering vectors
// Solve
// Copy rhs into weights

void SolveBatchThreadLevel::compile(std::stringstream & ss)
{
  CodeTemplate ct = CodeTemplate();
  ct.addLine("__local float2 mat[<DIM>][<DIM>+<NRHS>];");
  ct.addLine("__local float2 row[<DIM>+<NRHS>];");
  ct.addLine("__local float2 col[<DIM>];");
  ct.addLine("__local float sc_r, sc_i;");
  ct.addLine("int base = block_id1 * <DIM> * <DIM>;");
  ct.addLine("// Load matrix");
  ct.addLine("for(int i = local_id1 ; i < <DIM> ; i+=8)");
  ct.addLine("{");
  ct.addLine("  for(int j = local_id0 ; j < <DIM> ; j+=8)");
  ct.addLine("  {");
  ct.addLine("    mat[i][j] = <SRC>;");
  ct.addLine("  }");
  ct.addLine("}");
  ct.addLine("// Load rhs");
  ct.addLine("for(int i = local_id0 ; i < <DIM> ; i+=8)");
  ct.addLine("{");
  ct.addLine("  for(int j = local_id1 ; j < <NRHS> ; j+=8)");
  ct.addLine("  {");
  ct.addLine("    int offset = i + j * <DIM>;");
  ct.addLine("    mat[i][<DIM>+j] = <STEERING>;");
  ct.addLine("  }");
  ct.addLine("}");
  ct.addLine("barrier(CLK_LOCAL_MEM_FENCE);");
  ct.addLine("// Solve Ax=b");
  ct.addLine("for(int k = 0 ; k < <DIM> ; k++)");
  ct.addLine("{");
  ct.addLine("  for(int i = local_id1 ; i < <DIM> ; i+=8)");
  ct.addLine("  {");
  ct.addLine("    for(int j = local_id0 ; j < <DIM> ; j+=8)");
  ct.addLine("    {");
  ct.addLine("      // Extract inverse of diagonal element");
  ct.addLine("      if(k == i && k == j)");
  ct.addLine("      {");
  ct.addLine("        float mag = mat[i][j].x * mat[i][j].x + mat[i][j].y * mat[i][j].y;");
  ct.addLine("        sc_r = mat[i][j].x / mag;");
  ct.addLine("        sc_i = -mat[i][j].y / mag;");
  ct.addLine("      }");
  ct.addLine("    }");
  ct.addLine("  }");
  ct.addLine("  barrier(CLK_LOCAL_MEM_FENCE);");
  ct.addLine("  // Extract scaled row and un-scaled column");
  ct.addLine("  for(int i = local_id1 ; i < <DIM> ; i+=8)");
  ct.addLine("  {");
  ct.addLine("    for(int j = local_id0 ; j < <DIM> + <NRHS> ; j+=8)");
  ct.addLine("    {");
  ct.addLine("      if(k == i)");
  ct.addLine("      {");
  ct.addLine("        row[j].x = sc_r * mat[i][j].x - sc_i * mat[i][j].y;");
  ct.addLine("        row[j].y = sc_r * mat[i][j].y + sc_i * mat[i][j].x;");
  ct.addLine("      }");
  ct.addLine("      if(k == j)");
  ct.addLine("      {");
  ct.addLine("        col[i] = mat[i][j];");
  ct.addLine("      }");
  ct.addLine("    }");
  ct.addLine("  }");
  ct.addLine("  barrier(CLK_LOCAL_MEM_FENCE);");
  ct.addLine("  for(int i = local_id1 ; i < <DIM> ; i+=8)");
  ct.addLine("  {");
  ct.addLine("    for(int j = local_id0 ; j < <DIM> + <NRHS> ; j+=8)");
  ct.addLine("    {");
  ct.addLine("      // Update matrix");
  ct.addLine("      if(k != i)");
  ct.addLine("      {");
  ct.addLine("        mat[i][j].x -= row[j].x * col[i].x - row[j].y * col[i].y;");
  ct.addLine("        mat[i][j].y -= row[j].x * col[i].y + row[j].y * col[i].x;");
  ct.addLine("      }");
  ct.addLine("      else");
  ct.addLine("      {");
  ct.addLine("        mat[i][j] = row[j];");
  ct.addLine("      }");
  ct.addLine("    }");
  ct.addLine("  }");
  ct.addLine("  barrier(CLK_LOCAL_MEM_FENCE);");
  ct.addLine("}");

  ct.addLine("// Store x");
  ct.addLine("int base_x = block_id1 * <DIM> * <NRHS>;");
  ct.addLine("for(int i = local_id0 ; i < <DIM> ; i+=8)");
  ct.addLine("{");
  ct.addLine("  for(int j = local_id1 ; j < <NRHS> ; j+=8)");
  ct.addLine("  {");
  ct.addLine("    int offset = base_x + i + j * <DIM>;");
  ct.addLine("    <DST> = mat[i][<DIM>+j];");
  ct.addLine("  }");
  ct.addLine("}");

  ct.replace("SRC", covarianceOperand->get("j", "i", "thread_id2"));
  ct.replace("STEERING", steeringOperand->get("i", "j"));
  ct.replace("DST", dstOperand->get("i", "j", "thread_id2"));
  ct.replace("DIM", problem_dim);
  ct.replace("NRHS", nrhs);

  ss << ct.getText() << std::endl;
}

// Semantic model node generates execution nodes and strands while generating the execution model
void SolveBatchSetupLaunch(std::vector<EStmt*> & estmts,  std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt)
{
  Array3D * a0 = dynamic_cast<Array3D*>(vars[0]);
  Array3D * a1 = dynamic_cast<Array3D*>(vars[1]);
  Array2D * a2 = dynamic_cast<Array2D*>(vars[2]);

  assert(a0);
  assert(a1);
  assert(a2);

  int nd0 = constantTable["nd20"]->get_property_int("value");
  int nd1 = constantTable["nd21"]->get_property_int("value");
  int nd2 = 1;
  int nt0 = constantTable["nt20"]->get_property_int("value");
  int nt1 = constantTable["nt21"]->get_property_int("value");
  int nt2 = 1;
  int ne0 = constantTable["ne20"]->get_property_int("value");
  int ne1 = constantTable["ne21"]->get_property_int("value");
  int ne2 = 1;

  int ndevices[3];
  int nblocks[3];
  int nthreads[3];
  int nelements[3];
  int blksize[3];

  // Hardcode this for 18x18
  int n_problems = constantTable["N_BLOCKS"]->get_property_int("value") * constantTable["N_DOP"]->get_property_int("value");
  int problem_dim = constantTable["TDOF"]->get_property_int("value") * constantTable["N_CHAN"]->get_property_int("value");
  int nrhs = constantTable["N_STEERING"]->get_property_int("value");

  nthreads[0] = 8;
  nthreads[1] = 8;
  nthreads[2] = 1;
  nblocks[0] = 1;
  nblocks[1] = 1;
  nblocks[2] = n_problems; 
  nelements[0] = 1;
  nelements[1] = 1;
  nelements[2] = 1;
  blksize[0] = nthreads[0];
  blksize[1] = nthreads[1];
  blksize[2] = nthreads[2];
  ndevices[0] = nd0;
  ndevices[1] = nd1;
  ndevices[2] = nd2;

  Array3DOperand * dstOperand;
  Array3DOperand * covarianceOperand;
  Array2DOperand * steeringOperand;

  // Sink operands
  {
    int dim[3];
    int devsize[3];
    dim[0] = problem_dim;
    dim[1] = nrhs;
    dim[2] = n_problems;
    devsize[0] = problem_dim;
    devsize[1] = nrhs;
    devsize[2] = ((n_problems + nd2 - 1) / nd2);
    dstOperand = new Array3DOperand(a0, Array3DDevicePartition( dim, ndevices, devsize));
  }
 
  // Source operands
  {
    int dim[3];
    int devsize[3];
    dim[0] = problem_dim;
    dim[1] = problem_dim;
    dim[2] = n_problems;
    devsize[0] = problem_dim;
    devsize[1] = problem_dim;
    devsize[2] = ((n_problems + nd2 - 1) / nd2);
    covarianceOperand = new Array3DOperand(a1, Array3DDevicePartition( dim, ndevices, devsize));
  }
  {
    int dim[2];
    dim[0] = problem_dim;
    dim[1] = nrhs;
    steeringOperand = new Array2DOperand(a2, Array2DDevicePartition( dim, ndevices));
  }

  // Create EStmt
  {
    int dim[3];
    dim[0] = 8;
    dim[1] = 8;
    dim[2] = n_problems;
    Level * level = new SolveBatchThreadLevel(dstOperand, covarianceOperand, steeringOperand, n_problems, problem_dim, nrhs);
    EStmt * estmt = new EStmt("SolveBatch", level, LaunchPartition(3, dim, ndevices, nblocks, nthreads, nelements, THREAD));
    estmt->addSource(covarianceOperand);
    estmt->addSource(steeringOperand);
    estmt->addSink(dstOperand);
    estmts.push_back(estmt);
  }

}


