#ifndef __OCLGEN_H
#define __OCLGEN_H

#include <string>
#include <sstream>
#include <map>
#include <set>
#include <vector>
#include "pugixml.hpp"
#include "sm.h"
#include "clhelp.h"
#include "em.h"


void generate_ocl(SemanticModel * sm, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, const cl_vars_t clv, ENode ** e);
void enqueue_ocl_kernels(ENode * e, cl_vars_t & clv, std::map<std::string, Variable*> sym_tab, std::vector<EParam*> & retval);

#endif
