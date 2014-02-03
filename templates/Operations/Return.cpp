#include "../sm.h"
#include "../em.h"
#include "../lm.h"
#include "../Types/Array.h"
#include "../Types/Array2D.h"
#include "../Types/Array3D.h"
#include "../Types/Scalar.h"
#include "../Types/CSRMat.h"

#include <cassert>
#include <iostream>

void ReturnSetupLaunch(std::vector<EStmt*> & estmts,  std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt)
{

  for(std::vector<Variable*>::iterator it = vars.begin() ; it != vars.end() ; it++)
  {
    Level * level = new PlatformLevel();
    EStmt * estmt = new EStmt("Return", level, LaunchPartition(1, 0, 0, 0, 0, PLATFORM));
    estmt->label = "Return";

    Scalar * scalar = dynamic_cast<Scalar*>(*it);
    Array * arr = dynamic_cast<Array*>(*it);
    Array2D * arr2d = dynamic_cast<Array2D*>(*it);
    Array3D * arr3d = dynamic_cast<Array3D*>(*it);
    CSRMat * csrmat = dynamic_cast<CSRMat*>(*it);

    if(arr)
    {
      int dim = arr->getDim();
      ArrayOperand * operand = new ArrayOperand(arr, ArrayDevicePartition(1,dim,0,0,0));
      estmt->addSource(operand);
      estmts.push_back(estmt);
    }
    if(arr2d)
    {
      int dim[2];
      dim[0] = arr2d->get_property_int("dim0");
      dim[1] = arr2d->get_property_int("dim1");
      Array2DOperand * operand = new Array2DOperand(arr2d, Array2DDevicePartition(dim));
      estmt->addSource(operand);
      estmts.push_back(estmt);
    }
    if(arr3d)
    {
      int dim[3];
      dim[0] = arr3d->get_property_int("dim0");
      dim[1] = arr3d->get_property_int("dim1");
      dim[2] = arr3d->get_property_int("dim2");
      Array3DOperand * operand = new Array3DOperand(arr3d, Array3DDevicePartition(dim));
      estmt->addSource(operand);
      estmts.push_back(estmt);
    }
    if(scalar)
    {
      ScalarOperand * operand = new ScalarOperand(scalar, "", 1);
      estmt->addSource(operand);
      estmts.push_back(estmt);
    }
    if(csrmat)
    {
      int dim = csrmat->getDim0();
      CSRMatOperand * operand = new CSRMatOperand(csrmat, CSRMatDevicePartition(dim, 256, 2, 1));
      estmt->addSource(operand);
      estmts.push_back(estmt);
    }
  }

}


