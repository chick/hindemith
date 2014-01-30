#ifndef __EXECUTE_H
#define __EXECUTE_H

#include "em.h"
#include "lm.h"
#include "clhelp.h"
#include "profiler.h"

class Execute
{
  public:
    ENode * e;
    Execute(ENode * e);
    ~Execute();
    void executeRecursive(ENode * e, cl_vars_t clv, std::vector<EParam*> & retval);
    void doExecute(cl_vars_t clv, std::vector<EParam*> & retval);
};

#endif
