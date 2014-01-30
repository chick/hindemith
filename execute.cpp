#include <iostream>
#include "execute.h"
#include <cassert>
#include "profiler.h"
#include "docs.h"
extern Docs docs;
extern std::vector<kernel_prof_t> kernel_events;
extern std::vector<transfer_prof_t> transfer_events;

void Execute::executeRecursive(ENode * e, cl_vars_t clv, std::vector<EParam*> & retval)
{
  ESemanticModel * esemanticModel = dynamic_cast<ESemanticModel*>(e);
  EFunctionDef * efunctiondef = dynamic_cast<EFunctionDef*>(e);
  EIterator * efor = dynamic_cast<EIterator*>(e);
  EWhile * ewhile = dynamic_cast<EWhile*>(e);
  EPipeAndFilter * ebody = dynamic_cast<EPipeAndFilter*>(e);
  EBasicBlock * ebasicblock = dynamic_cast<EBasicBlock*>(e);

  if(esemanticModel)
  {
    executeRecursive(esemanticModel->body, clv, retval);
    if(retval.size() > 0)
    {
      return;
    }
  }
  if(efunctiondef)
  {
    executeRecursive(efunctiondef->body, clv, retval);
    if(retval.size() > 0)
    {
      return;
    }
  }
  if(efor)
  {
    for(int i = 0 ; i < efor->ub ; i++)
    {
      executeRecursive(efor->body, clv, retval);
      if(retval.size() > 0)
      {
        return;
      }
    }
  }
  if(ewhile)
  {
    executeRecursive(ewhile->body, clv, retval);
    if(retval.size() > 0)
    {
      return;
    }
  }
  if(ebody)
  {
    for(std::vector<ENode*>::iterator it = ebody->stmts.begin() ;
        it != ebody->stmts.end() ; it++)
    {
      executeRecursive(*it, clv, retval);
      if(retval.size() > 0)
      {
        return;
      }
    }
  }
  if(ebasicblock)
  {
    if(ebasicblock->p)
    {
#ifdef PROFILE
      kernel_events.clear();
#endif
      // Execute the strand
      ebasicblock->p->execute(clv);

      // Get profile information
#ifdef PROFILE
      doProfile();
#endif
    }

    // Find a return value
    for(std::vector<EStmt*>::iterator it = ebasicblock->stmts.begin() ;
        it != ebasicblock->stmts.end() ; it++)
    { 
      if((*it)->label == "Return")
      {
        for(std::vector<EParam*>::iterator it2 = (*it)->sources.begin() ; 
	    it2 != (*it)->sources.end() ; it2++)
	{
	  retval.push_back(*it2);
	}
      }
    }
  }
  return;
}


Execute::Execute(ENode * e) : e(e) 
{
}

Execute::~Execute()
{
}

void Execute::doExecute(cl_vars_t clv, std::vector<EParam*> & retval)
{
#ifdef VERBOSE_EXECUTION
  docs.execution_ss << "Executing" << std::endl;
#endif
  executeRecursive(e, clv, retval);
}
