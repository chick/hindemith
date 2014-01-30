#include "sm.h"

void ArrayCopySetupLaunch(std::vector<EStmt*> & e, std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt);
void ArrayOpSetupLaunch(std::vector<EStmt*> & e, std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt);
void SpmvDIASetupLaunch(std::vector<EStmt*> & e, std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt);
void ArraySumSetupLaunch(std::vector<EStmt*> & e, std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt);
void ScalarCopySetupLaunch(std::vector<EStmt*> & e, std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt);
void ScalarOpSetupLaunch(std::vector<EStmt*> & e, std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt);
void ScalarArrayOpSetupLaunch(std::vector<EStmt*> & e, std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt);
void ReturnSetupLaunch(std::vector<EStmt*> & e, std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt);
void SpmvDIA2DSetupLaunch(std::vector<EStmt*> & e, std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt);
void Array2DOpSetupLaunch(std::vector<EStmt*> & e, std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt);
void Array2DCopySetupLaunch(std::vector<EStmt*> & e, std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt);
void ScalarArray2DOpSetupLaunch(std::vector<EStmt*> & e, std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt);
void Stencil2DMVMSetupLaunch(std::vector<EStmt*> & e, std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt);
void Array2DSumSetupLaunch(std::vector<EStmt*> & e, std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt);
void ExtractSnapshotsSetupLaunch(std::vector<EStmt*> & e, std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt);
void Array3DCopySetupLaunch(std::vector<EStmt*> & e, std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt);
void CgemmBatchSetupLaunch(std::vector<EStmt*> & e, std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt);
void ScalarArray3DOpSetupLaunch(std::vector<EStmt*> & e, std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt);
void SolveBatchSetupLaunch(std::vector<EStmt*> & e, std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt);
void GammaWeightsSetupLaunch(std::vector<EStmt*> & e, std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt);
void InnerProductsSetupLaunch(std::vector<EStmt*> & e, std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt);
void WarpImg2DSetupLaunch(std::vector<EStmt*> & e, std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt);
void RedArray2DSetupLaunch(std::vector<EStmt*> & e, std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt);
void BlackArray2DSetupLaunch(std::vector<EStmt*> & e, std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt);
void ApplyHSPreconditionerSetupLaunch(std::vector<EStmt*> & e, std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt);
void Array2DScalarOpSetupLaunch(std::vector<EStmt*> & e, std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt);
void Array2DSqrtSetupLaunch(std::vector<EStmt*> & e, std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt);
void SetBroxMatrixSetupLaunch(std::vector<EStmt*> & e, std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt);
void ApplyBroxPreconditionerSetupLaunch(std::vector<EStmt*> & e, std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt);
void SetBroxRedblackMatrixSetupLaunch(std::vector<EStmt*> & e, std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt);
void LKLeastSquaresSetupLaunch(std::vector<EStmt*> & e, std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt);
void Array3DOpSetupLaunch(std::vector<EStmt*> & e, std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt);
void ArrayScalarOpSetupLaunch(std::vector<EStmt*> & e, std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt);
void SpmvCSRSetupLaunch(std::vector<EStmt*> & e, std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt);
void ScalarSqrtSetupLaunch(std::vector<EStmt*> & e, std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt);
void ArraySqrtSetupLaunch(std::vector<EStmt*> & e, std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt);
void HSMatrixPowersSetupLaunch(std::vector<EStmt*> & e, std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt);

void setFunctionPointer(Stmt * stmt)
{
  if(stmt->properties["name"] == "ArrayCopy")
  {
    stmt->setupLaunch = &ArrayCopySetupLaunch;
  }

  if(stmt->properties["name"] == "ArrayOp")
  {
    stmt->setupLaunch = &ArrayOpSetupLaunch;
  }

  if(stmt->properties["name"] == "SpmvDIA")
  {
    stmt->setupLaunch = &SpmvDIASetupLaunch;
  }

  if(stmt->properties["name"] == "ArraySum")
  {
    stmt->setupLaunch = &ArraySumSetupLaunch;
  }

  if(stmt->properties["name"] == "ScalarCopy")
  {
    stmt->setupLaunch = &ScalarCopySetupLaunch;
  }

  if(stmt->properties["name"] == "ScalarOp")
  {
    stmt->setupLaunch = &ScalarOpSetupLaunch;
  }

  if(stmt->properties["name"] == "ScalarArrayOp")
  {
    stmt->setupLaunch = &ScalarArrayOpSetupLaunch;
  }

  if(stmt->properties["name"] == "Return")
  {
    stmt->setupLaunch = &ReturnSetupLaunch;
  }

  if(stmt->properties["name"] == "SpmvDIA2D")
  {
    stmt->setupLaunch = &SpmvDIA2DSetupLaunch;
  }

  if(stmt->properties["name"] == "Array2DOp")
  {
    stmt->setupLaunch = &Array2DOpSetupLaunch;
  }

  if(stmt->properties["name"] == "Array2DCopy")
  {
    stmt->setupLaunch = &Array2DCopySetupLaunch;
  }

  if(stmt->properties["name"] == "ScalarArray2DOp")
  {
    stmt->setupLaunch = &ScalarArray2DOpSetupLaunch;
  }

  if(stmt->properties["name"] == "Stencil2DMVM")
  {
    stmt->setupLaunch = &Stencil2DMVMSetupLaunch;
  }

  if(stmt->properties["name"] == "Array2DSum")
  {
    stmt->setupLaunch = &Array2DSumSetupLaunch;
  }

  if(stmt->properties["name"] == "ExtractSnapshots")
  {
    stmt->setupLaunch = &ExtractSnapshotsSetupLaunch;
  }

  if(stmt->properties["name"] == "Array3DCopy")
  {
    stmt->setupLaunch = &Array3DCopySetupLaunch;
  }

  if(stmt->properties["name"] == "CgemmBatch")
  {
    stmt->setupLaunch = &CgemmBatchSetupLaunch;
  }

  if(stmt->properties["name"] == "ScalarArray3DOp")
  {
    stmt->setupLaunch = &ScalarArray3DOpSetupLaunch;
  }

  if(stmt->properties["name"] == "SolveBatch")
  {
    stmt->setupLaunch = &SolveBatchSetupLaunch;
  }

  if(stmt->properties["name"] == "GammaWeights")
  {
    stmt->setupLaunch = &GammaWeightsSetupLaunch;
  }

  if(stmt->properties["name"] == "InnerProducts")
  {
    stmt->setupLaunch = &InnerProductsSetupLaunch;
  }

  if(stmt->properties["name"] == "WarpImg2D")
  {
    stmt->setupLaunch = &WarpImg2DSetupLaunch;
  }

  if(stmt->properties["name"] == "RedArray2D")
  {
    stmt->setupLaunch = &RedArray2DSetupLaunch;
  }

  if(stmt->properties["name"] == "BlackArray2D")
  {
    stmt->setupLaunch = &BlackArray2DSetupLaunch;
  }

  if(stmt->properties["name"] == "ApplyHSPreconditioner")
  {
    stmt->setupLaunch = &ApplyHSPreconditionerSetupLaunch;
  }

  if(stmt->properties["name"] == "Array2DScalarOp")
  {
    stmt->setupLaunch = &Array2DScalarOpSetupLaunch;
  }

  if(stmt->properties["name"] == "Array2DSqrt")
  {
    stmt->setupLaunch = &Array2DSqrtSetupLaunch;
  }

  if(stmt->properties["name"] == "SetBroxMatrix")
  {
    stmt->setupLaunch = &SetBroxMatrixSetupLaunch;
  }

  if(stmt->properties["name"] == "ApplyBroxPreconditioner")
  {
    stmt->setupLaunch = &ApplyBroxPreconditionerSetupLaunch;
  }

  if(stmt->properties["name"] == "SetBroxRedblackMatrix")
  {
    stmt->setupLaunch = &SetBroxRedblackMatrixSetupLaunch;
  }

  if(stmt->properties["name"] == "LKLeastSquares")
  {
    stmt->setupLaunch = &LKLeastSquaresSetupLaunch;
  }

  if(stmt->properties["name"] == "Array3DOp")
  {
    stmt->setupLaunch = &Array3DOpSetupLaunch;
  }

  if(stmt->properties["name"] == "ArrayScalarOp")
  {
    stmt->setupLaunch = &ArrayScalarOpSetupLaunch;
  }

  if(stmt->properties["name"] == "SpmvCSR")
  {
    stmt->setupLaunch = &SpmvCSRSetupLaunch;
  }

  if(stmt->properties["name"] == "ScalarSqrt")
  {
    stmt->setupLaunch = &ScalarSqrtSetupLaunch;
  }

  if(stmt->properties["name"] == "ArraySqrt")
  {
    stmt->setupLaunch = &ArraySqrtSetupLaunch;
  }

  if(stmt->properties["name"] == "HSMatrixPowers")
  {
    stmt->setupLaunch = &HSMatrixPowersSetupLaunch;
  }
}
