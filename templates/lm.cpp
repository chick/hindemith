#include <iostream>
#include <cstdio>
#include <cassert>

#include "lm.h"
#include "sm.h"
#include "em.h"
#include "docs.h"
#include "profiler.h"
extern Docs docs;
extern std::vector<kernel_prof_t> kernel_events;

/* Utilities */
bool OperandEqual::operator()(const Operand * p1, const Operand * p2) const
{
  return (p1->variable->properties["name"] < p2->variable->properties["name"]);
}

static int level_id;
Level::Level()
{
  id = level_id++;
}

LaunchPartition::LaunchPartition(int ldim_in, int dim_in, int ndevices_in, int nblocks_in, int nthreads_in, int nelements_in, MemLevel level) : d_offset(0), level(level) 
{
  ldim = ldim_in;
  assert(ldim == 1);
  dim[0] = dim_in;
  ndevices[0] = ndevices_in;
  nblocks[0] = nblocks_in;
  nthreads[0] = nthreads_in;
  nelements[0] = nelements_in;
};

LaunchPartition::LaunchPartition(int ldim_in, int * dim_in, int * ndevices_in, int * nblocks_in, int * nthreads_in, int * nelements_in, MemLevel level_in)
{
  d_offset = 0;
  level = level_in;
  ldim = ldim_in;
  for(int l = 0 ; l < ldim ; l++)
  {
    dim[l] = dim_in[l];
    ndevices[l] = ndevices_in[l];
    nblocks[l] = nblocks_in[l];
    nthreads[l] = nthreads_in[l];
    nelements[l] = nelements_in[l];
  }
}

LaunchPartition::LaunchPartition(int ldim_in, int dim_in, int nblocks_in, int nthreads_in, int nelements_in, MemLevel level) : d_offset(0), level(level) 
{
  ldim = ldim_in;
  assert(ldim == 1);
  dim[0] = dim_in;
  ndevices[0] = 1;
  nblocks[0] = nblocks_in;
  nthreads[0] = nthreads_in;
  nelements[0] = nelements_in;
};

LaunchPartition::LaunchPartition(int ldim_in, int * dim_in, int * nblocks_in, int * nthreads_in, int * nelements_in, MemLevel level_in)
{
  d_offset = 0;
  level = level_in;
  ldim = ldim_in;
  for(int l = 0 ; l < ldim ; l++)
  {
    dim[l] = dim_in[l];
    ndevices[l] = 1;
    nblocks[l] = nblocks_in[l];
    nthreads[l] = nthreads_in[l];
    nelements[l] = nelements_in[l];
  }
}


void createStrand(LaunchPartition launch, EStmt * estmt, Level * levelInstance, PlatformLevel * & p)
{
  if(launch.level == PLATFORM)
  {
    p = (PlatformLevel*) levelInstance;
    p->stmts.push_back(estmt);
  }
  else
  {
    p = new PlatformLevel();
    p->stmts.push_back(estmt);

    DeviceLevel * d;
    createStrand(launch, estmt, levelInstance, d);
    p->children.push_back(d);
  }
}

void createStrand(LaunchPartition launch, EStmt * estmt, Level * levelInstance, DeviceLevel * &d)
{
  if(launch.level == DEVICE)
  {
    d = (DeviceLevel*) levelInstance;
    d->stmts.push_back(estmt);
  }
  else
  {
    d = new DeviceLevel(1);
    d->stmts.push_back(estmt);

    BlockLevel * b;
    createStrand(launch, estmt, levelInstance, b);
    d->blockList.push_back(b);
  }
}

void createStrand(LaunchPartition launch, EStmt * estmt, Level * levelInstance, BlockLevel * & b)
{
  if(launch.level == BLOCK)
  {
    b = (BlockLevel*) levelInstance;
    b->stmts.push_back(estmt);
  }
  else
  {
    size_t local[MAX_LDIM];
    size_t global[MAX_LDIM];
    size_t ndevices[MAX_LDIM];
    for(int l = 0 ; l < launch.ldim ; l++)
    {
      local[l] = (size_t) launch.nthreads[l];
      global[l] = (size_t) launch.nblocks[l]*launch.nthreads[l];
      ndevices[l] = (size_t) launch.ndevices[l];
    }

    b = new BlockLevel(local, global, launch.ldim, ndevices);
    b->stmts.push_back(estmt);

    ThreadLevel * t;
    createStrand(launch, estmt, levelInstance, t);
    b->children.push_back(t);
  }
}

void createStrand(LaunchPartition launch, EStmt * estmt, Level * levelInstance, ThreadLevel * & t)
{
  if(launch.level == THREAD)
  {
    t = (ThreadLevel*) levelInstance;
    t->stmts.push_back(estmt);
  }
  else
  {
    t = new ThreadLevel(launch.nthreads, launch.nelements, launch.dim, launch.ldim);
    t->stmts.push_back(estmt);

    ElementLevel * e = (ElementLevel *) levelInstance;
    t->children.push_back(e);
    e->stmts.push_back(estmt);
  }
}

EStmt * createStmt(LaunchPartition * launch, Level * levelInstance)
{
  EStmt * estmt = new EStmt();
  createStrand(*launch, estmt, levelInstance, estmt->p);
  return estmt;
}

Operand::Operand(Variable* variable) : variable(variable), level(DEVICE) {};

/* Matching */
bool passesDependence(std::vector<Operand*> s1_sources, 
                     std::vector<Operand*> s2_sources,
		     std::vector<Operand*> s1_sinks,
		     std::vector<Operand*> s2_sinks,
		     MemLevel level)
{
  // Check operand dependences: RAW, WAW, WAR
  for(std::vector<Operand*>::iterator it1 = s1_sinks.begin() ; it1 != s1_sinks.end() ; it1++)
  {
    for(std::vector<Operand*>::iterator it2 = s2_sources.begin() ; it2 != s2_sources.end() ; it2++)
    {
      if((*it1)->matches(*it2) < level)
      {
        return false;
      }
    }
    for(std::vector<Operand*>::iterator it2 = s2_sinks.begin() ; it2 != s2_sinks.end() ; it2++)
    {
      if((*it1)->matches(*it2) < level)
      {
        return false;
      }
    }
  }
  for(std::vector<Operand*>::iterator it1 = s1_sources.begin() ; it1 != s1_sources.end() ; it1++)
  {
    for(std::vector<Operand*>::iterator it2 = s2_sinks.begin() ; it2 != s2_sinks.end() ; it2++)
    {
      if((*it1)->matches(*it2) < level)
      {
        return false;
      }
    }
  }
  return true;
}

bool PlatformLevel::matches(PlatformLevel * pl_compare)
{
  return true;
};

bool DeviceLevel::matches(DeviceLevel * dl_compare)
{
  // Check operand dependences
  if(!passesDependence(sources, dl_compare->sources, sinks, dl_compare->sinks, DEVICE))
  {
#ifdef VERBOSE_EXECUTION
    docs.execution_ss << "Device level fusion prevented" << std::endl;
#endif
    return false;
  }
  return true;
};

bool BlockLevel::matches(BlockLevel * dl_compare)
{
  // Check partitioning/blocking
  if(ldim != dl_compare->ldim)
  {
    return false;
  }
  for(int l = 0 ; l < ldim ; l++)
  {
    if((global[l] != dl_compare->global[l]) || (local[l] != dl_compare->local[l]))
    {
      //printf("Failed launch parameters: %lu, %lu\t vs \t %lu %lu\n", local[l], global[l], dl_compare->local[l], dl_compare->global[l]);
      return false;
    }
  }
  // Check operand dependences
  if(!passesDependence(sources, dl_compare->sources, sinks, dl_compare->sinks, BLOCK))
  {
    //printf("Failed passes dependence\n");
    return false;
  }
  return true;
};

bool ThreadLevel::matches(ThreadLevel * dl_compare)
{
  // Check that both have children
  if(children.size() == 0 || dl_compare->children.size() == 0)
  {
    //printf("Failed. No children.\n");
    return false;
  }

  // Check partitioning/blocking
  if(ldim != dl_compare->ldim)
  {
    return false;
  }
  for(int l = 0 ; l < ldim ; l++)
  {
    if((threadsPerBlock[l] != dl_compare->threadsPerBlock[l]) || (elementsPerThread[l] != dl_compare->elementsPerThread[l]))
    {
      return false;
    }
  }
  // Check operand dependences
  if(!passesDependence(sources, dl_compare->sources, sinks, dl_compare->sinks, THREAD))
  {
    return false;
  }
  return true;
};

bool ElementLevel::matches(ElementLevel * dl_compare)
{
  // Check operand dependences
  if(!passesDependence(sources, dl_compare->sources, sinks, dl_compare->sinks, ELEMENT))
  {
    return false;
  }
  return true;
};

/* Dumping statements to standard out */
void PlatformLevel::dumpStmts(int tab, std::stringstream & ss)
{
  for(int i = 0 ; i < tab ; i++)
  {
    ss << " ";
  }
  ss << "PlatformLevel";
  if(stmts.size() == 1 && children.size() == 0)
  { 
    ss << ": ";
    stmts[0]->dumpStmts(0, ss);
  }
  else
  {
    ss << std::endl;
  }
  for(std::vector<DeviceLevel*>::iterator it = children.begin() ; it != children.end() ; it++)
  {
    (*it)->dumpStmts(tab+2, ss);
  }
}
void DeviceLevel::dumpStmts(int tab, std::stringstream & ss)
{
  for(int i = 0 ; i < tab ; i++)
  {
    ss << " ";
  }
  ss << "DeviceLevel";
  if(stmts.size() == 1 && blockList.size() == 0)
  { 
    ss << ": ";
    stmts[0]->dumpStmts(0, ss);
  }
  else
  {
    ss << std::endl;
  }
  std::vector<BlockLevel*> children = blockList;
  for(std::vector<BlockLevel*>::iterator it = children.begin() ; it != children.end() ; it++)
  {
    (*it)->dumpStmts(tab+2, ss);
  }
}
void BlockLevel::dumpStmts(int tab, std::stringstream & ss)
{
  for(int i = 0 ; i < tab ; i++)
  {
    ss << " ";
  }
  ss << "BlockLevel(" << id << ")";
  if(stmts.size() == 1 && children.size() == 0)
  { 
    ss << ": ";
    stmts[0]->dumpStmts(0, ss);
  }
  else
  {
    ss << std::endl;
  }
  for(std::vector<ThreadLevel*>::iterator it = children.begin() ; it != children.end() ; it++)
  {
    (*it)->dumpStmts(tab+2, ss);
  }
}
void ThreadLevel::dumpStmts(int tab, std::stringstream & ss)
{
  for(int i = 0 ; i < tab ; i++)
  {
    ss << " ";
  }
  ss << "ThreadLevel";
  if(stmts.size() == 1 && children.size() == 0)
  { 
    ss << ": ";
    stmts[0]->dumpStmts(0, ss);
  }
  else
  {
    ss << std::endl;
  }
  for(std::vector<ElementLevel*>::iterator it = children.begin() ; it != children.end() ; it++)
  {
    (*it)->dumpStmts(tab+2, ss);
  }
}
void ElementLevel::dumpStmts(int tab, std::stringstream & ss)
{
  for(int i = 0 ; i < tab ; i++)
  {
    ss << " ";
  }
  ss << "ElementLevel";
  if(stmts.size() == 1)
  { 
    ss << ": ";
    stmts[0]->dumpStmts(0, ss);
  }
  else
  {
    ss << std::endl;
  }
}

/* Compilation */
void PlatformLevel::compile(cl_vars_t clv) 
{ 
  for(std::vector<DeviceLevel*>::iterator it = children.begin() ; 
      it != children.end() ; it++)
  {
    (*it)->compile(clv);
  }
}

void DeviceLevel::compile(cl_vars_t clv) 
{
  for(std::vector<BlockLevel*>::iterator it2 = blockList.begin() ; 
      it2 != blockList.end() ; it2++)
  {
    (*it2)->compile(clv);
  }
}

std::string BlockLevel::parameterStr()
{
  std::stringstream ss;
  for(int i = 0 ; i < (int)parameters.size() - 1 ; i++)
  {
    ss << parameters[i]->parameterStr() << ", ";
  }
  if(parameters.size() > 0)
  {
    ss << parameters.back()->parameterStr();
  }
  return ss.str();
}

void Level::buildParameterSet()
{
  std::set<Variable*> parameters_set;
  for(std::vector<Operand*>::iterator it = sources.begin() ; it != sources.end() ; it++)
  {
    if((*it)->variable)
    {
      parameters_set.insert((*it)->variable);
    }
  }
  for(std::vector<Operand*>::iterator it = sinks.begin() ; it != sinks.end() ; it++)
  {
    if((*it)->variable)
    {
      parameters_set.insert((*it)->variable);
    }
  }
  parameters.insert(parameters.end(), parameters_set.begin(), parameters_set.end());
}

void BlockLevel::setParameters(cl_vars_t clv, int devId)
{
  int arg_cnt = 0;
  for(std::vector<Variable*>::iterator it = parameters.begin() ;
      it != parameters.end() ; it++)
  {
    (*it)->setParameter(clv, kernel[devId], arg_cnt, devId);
  }
}

const char * precision_check_str = 
"#ifdef cl_khr_fp64 \n"
"#pragma OPENCL EXTENSION cl_khr_fp64 : enable \n"
"#elif defined(cl_amd_fp64) \n"
"#pragma OPENCL EXTENSION cl_amd_fp64 : enable \n"
"#else \n"
"#error \"Double precision floating point not supported by OpenCL implementation.\" \n"
"#endif \n"
"#define MAX(a,b) (((a) > (b)) ? (a) : (b))\n"
"#define MIN(a,b) (((a) < (b)) ? (a) : (b))\n";


void BlockLevel::compile(cl_vars_t clv)
{
  std::stringstream ss;
  this->buildParameterSet();
  ss << "__kernel void k( " << parameterStr() << ")" << std::endl;
  ss << "{" << std::endl;
  for(int l = 0 ; l < ldim ; l++)
  {
    ss << "int block_id" << l << " = DEVICE_ID" << l << " * get_num_groups( " << l << ") + get_group_id(" << l << ");" << std::endl;
    ss << "int thread_id" << l << " = DEVICE_ID" << l << "* get_global_size( " << l << ") + get_global_id(" << l << ");" << std::endl;
    ss << "int local_id" << l << " = get_local_id(" << l << ");" << std::endl;
  }

  // Build set of register parameters (those with 1 value per thread)
  if(!getenv("HM_DISABLE_REGISTER"))
  {
    std::set<Variable*> taken;
    for(std::vector<Operand*>::iterator it = sources.begin() ; 
        it != sources.end() ; it++)
    {
      // Declare operand
      if( ( taken.count((*it)->variable) == 0 ) && ( (*it)->getRegisterLevel() == THREAD))
      {
        ss << (*it)->variable->getDeclaration(THREAD) << std::endl;
        taken.insert((*it)->variable);
      }
    }
    for(std::vector<Operand*>::iterator it = sinks.begin() ; 
        it != sinks.end() ; it++)
    {
      // Declare operand
      if( ( taken.count((*it)->variable) == 0 ) && ( (*it)->getRegisterLevel() == THREAD))
      {
        ss << (*it)->variable->getDeclaration(THREAD) << std::endl;
        taken.insert((*it)->variable);
      }
    }
  }

  // For each ThreadLevel
  for(std::vector<ThreadLevel*>::iterator it = children.begin() ; 
      it != children.end() ; it++)
  {
    if(!getenv("HM_DISABLE_REGISTER")) 
    {
      ThreadLevel * tl = *it;
      // Look through all liveIn sources to the ThreadLevel
      //ss << "// Storing and loading operands for ThreadLevel" << std::endl;
      for(std::vector<Operand*>::iterator it2 = tl->sources.begin() ; it2 != tl->sources.end() ; it2++)
      {
        Operand * op = *it2;
        Variable * variable = op->variable;
        // If the thing is sitting in a register 
        assert(tl);
        assert(variable);
        if((tl->liveIns.count(variable)) && (variable->level > DEVICE))
        {
           // If this needs to be pushed down
  	  if((variable->operand) && ((variable->operand)->matches(op) < THREAD))
	  {
            ss << variable->operand->getStore(THREAD) << std::endl;
	    variable->level = DEVICE;
 	  }
        }

        if((tl->liveIns.count(variable)) && (op->getRegisterLevel() == THREAD) && (variable->level == DEVICE))
        {
          // Promote register operands
	  ss << " // ? Should we load " << variable->properties["name"] << std::endl;
	  ss << " // It is at level: " << variable->operand->level << std::endl;
	  ss << " // Op is at level: " << op->level << std::endl;
	  ss << " // Matches up to level: " << ((variable->operand)->matches(op)) << std::endl;
          if((op->getRegisterLevel() == THREAD) && (variable->level == DEVICE))
  	  {
	    ss << " // Loading at block level " << variable->properties["name"] << std::endl;
            ss << variable->operand->getLoad(THREAD) << std::endl;
	    variable->level = THREAD;
	    variable->operand = op;
	  }
        }
      }

      for(std::vector<Operand*>::iterator it2 = tl->sinks.begin() ; it2 != tl->sinks.end() ; it2++)
      {
        Operand * op = *it2;
        Variable * variable = op->variable;
        if((tl->liveOuts.count(variable)) && (op->getRegisterLevel() == THREAD))
        {
          variable->level = THREAD;
  	  variable->operand = op;
        }
      }
    }

    (*it)->compile(ss);
  }

  if(!getenv("HM_DISABLE_REGISTER")) 
  {
    // Store any variables that are left before ending the kernel
    //ss << "// Storing operands after ThreadLevels" << std::endl;
    for(std::vector<Operand*>::iterator it = sinks.begin() ; it != sinks.end() ; it++)
    {
      Operand * op = *it;
      Variable * variable = op->variable;
      if(variable->level == THREAD)
      {
        if(liveOuts.count(variable))
        {
          ss << variable->operand->getStore(THREAD) << std::endl;
          variable->level = DEVICE;
        }
      }
    }
  }
  ss << "}" << std::endl;

  // Write kernel

  // Compile kernel
  // for each device
  // Compile

  int devices_in_launch = 1;
  for(int l = 0 ; l < ldim ; l++)
  {
    devices_in_launch *= ndevices[l];
  }

  for(int devId = 0 ; devId < devices_in_launch ; devId++)
  {
    std::stringstream full_program;
    full_program << precision_check_str << std::endl;
    for(int l = 0 ; l < ldim ; l++)
    {
      // Figure out device ID l
      int devIdl = 0;
      if(l == 0)
      {
        devIdl = devId % ndevices[0];
      }
      else
      {
        devIdl = devId / ndevices[0];
      }
      full_program << "#define DEVICE_ID" << l << " " << devIdl << std::endl;
    }
    full_program << ss.str() << std::endl;
    write_ocl_program(full_program.str().c_str());
    compile_ocl_program(program[devId], kernel[devId], clv, full_program.str().c_str(), "k");

    // Set arguments
    setParameters(clv, devId);
  }
}

void ThreadLevel::compile(std::stringstream & ss) 
{
  // strided
  for(int l = 0 ; l < ldim ; l++)
  {
     //Special case for elementsPerThread[l] == 1
    if(elementsPerThread[l] == 1)
    {
      ss << "if(thread_id" << l << " < " << dim[l] << ")" << std::endl;
      ss << "{" << std::endl;
      ss << "int element_id" << l << " = thread_id" << l << ";" << std::endl;
    }
    else
    {
      ss << "for(int element_id" << l << " = block_id" << l << " * " << threadsPerBlock[l] * elementsPerThread[l] << " + local_id" << l << " ; element_id" << l << " < MIN(" << dim[l] << ", (block_id" << l << "+1) * " << threadsPerBlock[l]*elementsPerThread[l] << ") ; element_id" << l << " += " << threadsPerBlock[l] << ")" << std::endl;
      ss << "{" << std::endl;
    }
  }

  this->buildParameterSet();

  if(!getenv("HM_DISABLE_REGISTER")) 
  {
    std::set<Variable*> taken;
    for(std::vector<Operand*>::iterator it = sources.begin() ; 
        it != sources.end() ; it++)
    {
      // Declare operand
      if( ( taken.count((*it)->variable) == 0 ) && ( (*it)->getRegisterLevel() == ELEMENT))
      {
        ss << (*it)->variable->getDeclaration(ELEMENT) << std::endl;
        taken.insert((*it)->variable);
      }
    }
    for(std::vector<Operand*>::iterator it = sinks.begin() ; 
        it != sinks.end() ; it++)
    {
      // Declare operand
      if( ( taken.count((*it)->variable) == 0 ) && ( (*it)->getRegisterLevel() == ELEMENT))
      {
        ss << (*it)->variable->getDeclaration(ELEMENT) << std::endl;
        taken.insert((*it)->variable);
      }
    }
  }

  for(std::vector<ElementLevel*>::iterator it = children.begin() ; 
      it != children.end() ; it++)
  {

    if(!getenv("HM_DISABLE_REGISTER"))
    {
      ElementLevel * tl = *it;
      // Look through all liveIn sources to the ElementLevel
      //ss << "// Storing and loading operands for ElementLevel" << std::endl;
      for(std::vector<Operand*>::iterator it2 = tl->sources.begin() ; it2 != tl->sources.end() ; it2++)
      {
        Operand * op = *it2;
        Variable * variable = op->variable;
        // If the thing is sitting in a register 
        if(variable->level > THREAD)
        {
          // If this needs to be pushed down
  	  if((variable->operand) && ((variable->operand)->matches(op) < ELEMENT))
	  {
	    ss << "// Needs to be pushed down. Match level: " << (variable->operand)->matches(op) << std::endl;
            ss << variable->operand->getStore(ELEMENT) << std::endl;
	    variable->level = DEVICE;
	  }
        }
        if((op->getRegisterLevel() == ELEMENT) && (variable->level == DEVICE))
        {
          // Promote register operands
	  ss << " // Loading at thread level " << variable->properties["name"] << std::endl;
          ss << op->getLoad(ELEMENT) << std::endl;
	  variable->level = ELEMENT;
	  variable->operand = op;
        }
      }

      for(std::vector<Operand*>::iterator it2 = tl->sinks.begin() ; it2 != tl->sinks.end() ; it2++)
      {
        Operand * op = *it2;
        Variable * variable = op->variable;
        if(op->getRegisterLevel() == ELEMENT)
        {
          variable->level = ELEMENT;
	  variable->operand = op;
        }
      }

    //ss << "// ElementLevel Operation" << std::endl;
    }

    ss << std::endl << "{" << std::endl;
    (*it)->compile(ss);
    ss << std::endl << "}" << std::endl;
  }

  if(!getenv("HM_DISABLE_REGISTER")) 
  {
    // Store any variables that are left before ending the loop
    //ss << "// Storing operands after ElementLevels" << std::endl;
    for(std::vector<Operand*>::iterator it = sinks.begin() ; it != sinks.end() ; it++)
    {
      Operand * op = *it;
      Variable * variable = op->variable;
      if(variable->level == ELEMENT)
      {
        if(liveOuts.count(variable))
        {
          ss << variable->operand->getStore(ELEMENT) << std::endl;
          variable->level = DEVICE;
        }
      }
    }

    for(std::vector<Operand*>::iterator it = sources.begin() ; it != sources.end() ; it++)
    {
      Operand * op = *it;
      Variable * variable = op->variable;
      if(variable->level == ELEMENT)
      {
        variable->level = DEVICE;
      }
    }
  }

  for(int l = 0 ; l < ldim ; l++)
  {
    ss << "}" << std::endl;
  }
}

/* Execution */

void PlatformLevel::execute(cl_vars_t clv)
{
#ifdef VERBOSE_EXECUTION
  docs.execution_ss << "Executing platform level" << std::endl;
#endif
  for(std::vector<DeviceLevel*>::iterator it = children.begin() ; 
      it != children.end() ; it++)
  {

    // For each live in source
    bool needs_sync = false;
    for(std::vector<Operand*>::iterator it2 = (*it)->sources.begin() ; it2 != (*it)->sources.end() ; it2++)
    {
      // If it is not in the correct format then push down
      if((*it2)->variable->level > PLATFORM)
      {
        if((*it2)->variable->operand)
	{
          MemLevel matchLevel = ((*it2)->variable->operand)->matches((*it2));
	  if(matchLevel == PLATFORM)
	  {
	    // Make sure the callUp event is complete?
	    ((*it2)->variable->operand)->pushDown(clv);
            (*it2)->variable->level = PLATFORM;
	    needs_sync = true;
	  }
	}
      }
    }


    // Synchronize multiple command queues if anything was transferred
    if(needs_sync)
    {
      // for each queue
      if(clv.num_devices > 1)
      {
        for(size_t devId = 0 ; devId < clv.num_devices ; devId++)
        {
          clFinish(clv.commands[devId]);
        }
      }
    }

    // Call up live in sources 
    needs_sync = false;
    for(std::vector<Operand*>::iterator it2 = (*it)->sources.begin() ; it2 != (*it)->sources.end() ; it2++)
    {
      if((*it)->liveIns.count((*it2)->variable))
      {
        if((*it2)->variable->level == PLATFORM)
        {
          (*it2)->callUp(clv); 
          (*it2)->variable->operand = *it2;
          (*it2)->variable->level = DEVICE;
	  needs_sync = true;
        }
      }
    }

    if(needs_sync)
    {
      // for each queue
      if(clv.num_devices > 1)
      {
        for(size_t devId = 0 ; devId < clv.num_devices ; devId++)
        {
          clFinish(clv.commands[devId]);
        }
      }
    }

    (*it)->execute(clv);

    // We've written some variables to the device level, set their status (live outs only)
    for(std::vector<Operand*>::iterator it2 = (*it)->sinks.begin() ; it2 != (*it)->sinks.end() ; it2++)
    {
      if((*it)->liveOuts.count((*it2)->variable))
      {
        (*it2)->variable->operand = *it2;
        (*it2)->variable->level = DEVICE;
      }
    }

  }
}

void DeviceLevel::execute(cl_vars_t clv)
{
#ifdef VERBOSE_EXECUTION
  docs.execution_ss << "Executing device level" << std::endl;
#endif

  for(std::vector<BlockLevel*>::iterator it2 = blockList.begin() ; 
      it2 != blockList.end() ; it2++)
  {
    (*it2)->execute(clv); 
  }
}

void BlockLevel::execute(cl_vars_t clv)
{
  cl_int err = CL_SUCCESS;
  int devices_in_launch = 1;
  for(int l = 0 ; l < ldim ; l++)
  {
    devices_in_launch *= ndevices[l];
  }
  for(int devId = 0 ; devId < devices_in_launch ; devId++)
  {
#ifdef VERBOSE_EXECUTION
    docs.execution_ss << "Executing block level Device: " << devId;
    for(int l = 0 ; l < ldim ; l++)
    {
      docs.execution_ss << "\tglobal[" << l << "]: " << global[l]  << "\tlocal[" << l << "]" << local[l];
    }
    docs.execution_ss << std::endl;
#endif

    // For each device
#ifdef PROFILE
    err = clEnqueueNDRangeKernel(clv.commands[devId], kernel[devId], ldim, NULL, global, local, 0, NULL, &event[devId]); CHK_ERR(err);
    kernel_prof_t kp;
    kp.event = event[devId];
    kp.devId = devId;
    kp.b = this;
    kernel_events.push_back(kp);
#else
    err = clEnqueueNDRangeKernel(clv.commands[devId], kernel[devId], ldim, NULL, global, local, 0, NULL, NULL); CHK_ERR(err);
#endif
  }
}


/* Set Operands */
void PlatformLevel::setOperand(Operand * operand, bool is_source)
{
  if(is_source)
  {
    sources.push_back(operand);
  }
  else
  {
    sinks.push_back(operand);
  }
  for(std::vector<DeviceLevel*>::iterator it = children.begin() ; 
      it != children.end() ; it++)
  {
    (*it)->setOperand(operand, is_source);
  }
}

void DeviceLevel::setOperand(Operand * operand, bool is_source)
{
  if(is_source)
  {
    sources.push_back(operand);
  }
  else
  {
    sinks.push_back(operand);
  }

  for(std::vector<BlockLevel*>::iterator it2 = blockList.begin() ; 
      it2 != blockList.end() ; it2++)
  {
    (*it2)->setOperand(operand, is_source);
  }
}

void BlockLevel::setOperand(Operand * operand, bool is_source)
{
  if(is_source)
  {
    sources.push_back(operand);
  }
  else
  {
    sinks.push_back(operand);
  }
  for(std::vector<ThreadLevel*>::iterator it = children.begin() ; 
      it != children.end() ; it++)
  {
    (*it)->setOperand(operand, is_source);
  }
}

void ThreadLevel::setOperand(Operand * operand, bool is_source)
{
  if(is_source)
  {
    sources.push_back(operand);
  }
  else
  {
    sinks.push_back(operand);
  }
  for(std::vector<ElementLevel*>::iterator it = children.begin() ; 
      it != children.end() ; it++)
  {
    (*it)->setOperand(operand, is_source);
  }
}

void ElementLevel::setOperand(Operand * operand, bool is_source)
{
  if(is_source)
  {
    sources.push_back(operand);
  }
  else
  {
    sinks.push_back(operand);
  }
}



/* Set live variables */
void PlatformLevel::setLiveVariables(std::map<EStmt*, std::set<Variable*> > liveInMap,
                                     std::map<EStmt*, std::set<Variable*> > liveOutMap)
{
  if(stmts.size() > 0)
  {
    liveIns.insert(liveInMap[stmts.front()].begin(), liveInMap[stmts.front()].end());
    liveOuts.insert(liveOutMap[stmts.back()].begin(), liveOutMap[stmts.back()].end());
  }
  for(std::vector<DeviceLevel*>::iterator it = children.begin() ; 
      it != children.end() ; it++)
  {
    (*it)->setLiveVariables(liveInMap, liveOutMap);
  }
}

void DeviceLevel::setLiveVariables(std::map<EStmt*, std::set<Variable*> > liveInMap,
                                   std::map<EStmt*, std::set<Variable*> > liveOutMap)
{
  if(stmts.size() > 0)
  {
    liveIns.insert(liveInMap[stmts.front()].begin(), liveInMap[stmts.front()].end());
    liveOuts.insert(liveOutMap[stmts.back()].begin(), liveOutMap[stmts.back()].end());
  }
  std::vector<BlockLevel*> children = blockList;
  for(std::vector<BlockLevel*>::iterator it = children.begin() ; 
      it != children.end() ; it++)
  {
    (*it)->setLiveVariables(liveInMap, liveOutMap);
  }
}

void BlockLevel::setLiveVariables(std::map<EStmt*, std::set<Variable*> > liveInMap,
                                     std::map<EStmt*, std::set<Variable*> > liveOutMap)
{
  if(stmts.size() > 0)
  {
    liveIns.insert(liveInMap[stmts.front()].begin(), liveInMap[stmts.front()].end());
    liveOuts.insert(liveOutMap[stmts.back()].begin(), liveOutMap[stmts.back()].end());
  }
  for(std::vector<ThreadLevel*>::iterator it = children.begin() ; 
      it != children.end() ; it++)
  {
    (*it)->setLiveVariables(liveInMap, liveOutMap);
  }
}

void ThreadLevel::setLiveVariables(std::map<EStmt*, std::set<Variable*> > liveInMap,
                                     std::map<EStmt*, std::set<Variable*> > liveOutMap)
{
  if(stmts.size() > 0)
  {
    liveIns.insert(liveInMap[stmts.front()].begin(), liveInMap[stmts.front()].end());
    liveOuts.insert(liveOutMap[stmts.back()].begin(), liveOutMap[stmts.back()].end());
  }
  for(std::vector<ElementLevel*>::iterator it = children.begin() ; 
      it != children.end() ; it++)
  {
    (*it)->setLiveVariables(liveInMap, liveOutMap);
  }
}

void ElementLevel::setLiveVariables(std::map<EStmt*, std::set<Variable*> > liveInMap,
                                     std::map<EStmt*, std::set<Variable*> > liveOutMap)
{
  if(stmts.size() > 0)
  {
    liveIns.insert(liveInMap[stmts.front()].begin(), liveInMap[stmts.front()].end());
    liveOuts.insert(liveOutMap[stmts.back()].begin(), liveOutMap[stmts.back()].end());
  }
}

std::string getOperandStr(std::vector<Operand*> sources,
			  std::vector<Operand*> sinks,
			  std::set<Variable*> liveIns,
			  std::set<Variable*> liveOuts)
{
  std::stringstream ss;
  std::set<Variable*> source_set;
  std::set<Variable*> sink_set;

  // Now print liveins and liveouts
  for(std::vector<Operand*>::iterator it = sources.begin() ; it != sources.end() ; it++)
  {
    if(source_set.count((*it)->variable) == 0)
    {
      source_set.insert((*it)->variable);
      ss << " | \\< " << (*it)->getStr() << " \\l ";
    }
  }
  for(std::vector<Operand*>::iterator it = sinks.begin() ; it != sinks.end() ; it++)
  {
    if(sink_set.count((*it)->variable) == 0)
    {
      sink_set.insert((*it)->variable);
      ss << " | \\> " << (*it)->getStr() << " \\l ";
    }
  }
  return ss.str();
}

std::string ElementLevel::getStr()
{
  std::stringstream ss;
  ss << " { ";
  if(stmts.size() == 1)
  {
    ss << stmts[0]->label;
    ss << getOperandStr(sources, sinks, liveIns, liveOuts);
  }
  else
  {
    ss << "Element: " << id;
  }
  ss << " } ";
  return ss.str();
}
std::string ThreadLevel::getStr()
{
  std::stringstream ss;
  ss << " { ";
  if((children.size() == 0) && (stmts.size() == 1))
  {
    ss << stmts[0]->label;
    ss << getOperandStr(sources, sinks, liveIns, liveOuts);
  }
  else
  {
    ss << "Thread: " << id << std::endl;
  }
  ss << " } ";
  return ss.str();
}
std::string BlockLevel::getStr()
{
  std::stringstream ss;
  ss << " { ";
  if((children.size() == 0) && (stmts.size() == 1))
  {
    ss << stmts[0]->label;
    ss << getOperandStr(sources, sinks, liveIns, liveOuts);
  }
  else
  {
    ss << "Block: " << id << std::endl;
  }
  ss << " } ";
  return ss.str();
}
std::string DeviceLevel::getStr()
{
  std::stringstream ss;
  ss << " { ";
  std::vector<BlockLevel*> children = blockList;
  if((children.size() == 0) && (stmts.size() == 1))
  {
    ss << stmts[0]->label;
    ss << getOperandStr(sources, sinks, liveIns, liveOuts);
  }
  else
  {
    ss << "Device: " << std::endl;
  }
  ss << " } ";
  return ss.str();
}
std::string PlatformLevel::getStr()
{
  std::stringstream ss;
  ss << " { ";
  if((children.size() == 0) && (stmts.size() == 1))
  {
    ss << stmts[0]->label;
    ss << getOperandStr(sources, sinks, liveIns, liveOuts);
  }
  else
  {
    ss << "Platform: " << id << std::endl;
  }
  ss << " } ";
  return ss.str();
}

std::string Operand::getStr()
{
  if(variable)
  {
    return variable->properties["name"];
  }
  else
  {
    return "";
  }
}

CodeTemplate::CodeTemplate() {};

void CodeTemplate::replaceLine(std::string toReplace, std::string replaceWith)
{
  size_t f = text.find(toReplace);
  while(f != std::string::npos)
  {
    text.replace(f, toReplace.length(), replaceWith);
    f = text.find(toReplace);
  }
}

void CodeTemplate::addLine(std::string line)
{
  text += line + "\n";
}

void CodeTemplate::clearText()
{
  text.clear();
}

void CodeTemplate::replace(std::map<std::string, std::string> vars)
{
  for(std::map<std::string, std::string>::iterator it = vars.begin() ; it != vars.end() ; it++)
  {
    replaceLine("<" + (*it).first + ">", (*it).second);
  }
}

void CodeTemplate::replace(std::string s1, std::string s2)
{
  replaceLine("<" + s1 + ">", s2);
}

void CodeTemplate::replace(std::string s1, int s2)
{
  std::stringstream ss;
  ss << s2;
  replaceLine("<" + s1 + ">", ss.str());
}

std::string CodeTemplate::getText()
{
  return text;
}

MemLevel Operand::getRegisterLevel()
{
  return NOLEVEL;
}

std::string Operand::getDeclaration(MemLevel level)
{
  return "";
}

