#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <algorithm>

#include "sm.h"
#include "em.h"
#include "oclgen.h"
#include "clhelp.h"
#include "dataflow.h"
#include "fusion.h"
#include "execute.h"
#include "docs.h"

extern Docs docs;
extern bool enable_register_allocation;

ENode * BuildExecutionModel(Node * node, int & id, std::vector<EBasicBlock*> & Blocks, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv)
{
  SemanticModel * semanticModel = dynamic_cast<SemanticModel*>(node);
  FunctionDef * functionDef = dynamic_cast<FunctionDef*>(node);
  Iterator * fornode = dynamic_cast<Iterator*>(node);
  PipeAndFilter * body = dynamic_cast<PipeAndFilter*>(node);

  if(semanticModel)
  {
    ESemanticModel * e = new ESemanticModel();
    e->body = (EPipeAndFilter*) BuildExecutionModel(semanticModel->children[0], id, Blocks, symbolTable, constantTable, clv);
    return (ENode*) e;
  }
  else if(functionDef)
  {
    EFunctionDef * e = new EFunctionDef();
    e->body = (EPipeAndFilter*) BuildExecutionModel(functionDef->children[0], id, Blocks, symbolTable, constantTable, clv);
    return (ENode*) e;
  }
  else if(fornode)
  {
    EIterator * e = new EIterator();
    e->body = (EPipeAndFilter*) BuildExecutionModel(fornode->children[0], id, Blocks, symbolTable, constantTable, clv);
    e->ub = fornode->get_property_int("ub");
    return (ENode*) e;
  }
  else if(body)
  {
    EPipeAndFilter * e = new EPipeAndFilter();
    EBasicBlock * bb = new EBasicBlock(id++);
    Blocks.push_back(bb);
    for(std::vector<Node*>::iterator it = body->children.begin() ;
        it != body->children.end() ; it++)
    {
      Stmt * stmt = dynamic_cast<Stmt*>(*it);
      if(stmt)
      {
        // Get EStmts
	std::vector<EStmt*> estmts;
	std::vector<Variable*> vars;
	for(std::vector<Node*>::iterator it = stmt->children.begin() ;
	    it != stmt->children.end() ; it++)
	{
          Name * name_param = dynamic_cast<Name*>(*it);
	  assert(name_param);
	  Variable * v = symbolTable[name_param->properties["name"]];
	  assert(v);
          vars.push_back(v);
	}
        stmt->setupLaunch(estmts, vars, symbolTable, constantTable, clv, stmt);
	for(std::vector<EStmt*>::iterator it = estmts.begin() ; it != estmts.end() ; it++)
	{
	  bb->addStmt(*it);
	}
      }
      else
      {
        e->stmts.push_back((ENode*)bb);
	ENode * en = BuildExecutionModel(*it, id, Blocks, symbolTable, constantTable, clv);
	e->stmts.push_back(en);
	bb = new EBasicBlock(id++);
	Blocks.push_back(bb);
      }
    }
    if(bb->stmts.size() > 0)
    {
      e->stmts.push_back(bb);
    }
    return e;
  }
  return NULL;
}

void generate_ocl(SemanticModel * sm, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, const cl_vars_t clv, ENode ** e)
{

#ifdef VERBOSE_COMPILATION
  docs.compilation_ss << "Compiling kernels" << std::endl;
#endif
  std::vector<EBasicBlock*> Blocks;
  int id = 0;
  ENode * executionModel = BuildExecutionModel(sm, id, Blocks, symbolTable, constantTable, clv);

#ifdef VERBOSE_COMPILATION
  // Dump EStmt list
  executionModel->dumpStmts(0, docs.top_level_stmts_ss);
#endif

  // Do dataflow on block level
  AcrossBlockDataFlowAnalysis AB_DF = AcrossBlockDataFlowAnalysis(executionModel, Blocks);
  AB_DF.connectBlocks();
  AB_DF.doAnalysis();
#ifdef VERBOSE_COMPILATION
  AB_DF.dumpGraph();
#endif

  // Traverse basic blocks: Fuse, dataflow, and memory assignment
  for(std::vector<EBasicBlock*>::iterator it = Blocks.begin() ;
      it != Blocks.end() ; it++)
  {
    EBasicBlock * bb = *it;

    // Emit initial EStmt list
    std::stringstream dump_ss;
    bb->dumpStmts(0, dump_ss);
    docs.block_dumps.push_back(dump_ss.str());
    
    // Fuse strands into one tree per basic block.
    Fusion * FS = new Fusion(bb);
    FS->doFusion();
    //FS.dumpGraph(ss);

    // Emit dot file for fused block

    // Do dataflow on this basic block's execution tree
    WithinBlockDataFlowAnalysis WB_DD = WithinBlockDataFlowAnalysis(bb, AB_DF.liveIns[bb], AB_DF.liveOuts[bb]);
    WB_DD.doAnalysis();
  }

  // Traverse basic blocks: Compile fused strand
  for(std::vector<EBasicBlock*>::iterator it = Blocks.begin() ;
      it != Blocks.end() ; it++)
  {
    EBasicBlock * bb = *it;
    if(bb->p)
    {
      enable_register_allocation = 1;
      bb->p->compile(clv);
    }
  }

  *e = executionModel;
}

void enqueue_ocl_kernels(ENode * e, cl_vars_t & clv, std::map<std::string, Variable*> sym_tab, std::vector<EParam*> & retval)
{

#ifdef VERBOSE_EXECUTION
  docs.execution_ss << "Enqueuing kernels" << std::endl;
#endif

  // Initialize execution state
  for(std::map<std::string, Variable*>::iterator it = sym_tab.begin() ;
      it != sym_tab.end() ; it++)
  {
    Variable * variable = (*it).second;
    variable->operand = NULL;
    variable->level = PLATFORM;
  }

  Execute EX = Execute(e);
  EX.doExecute(clv, retval);

  // Push down all return values
  for(std::vector<EParam*>::iterator it = retval.begin() ; it != retval.end() ; it++)
  {
    Operand * return_op = (*it)->variable->operand;
    if((*it)->variable->level > PLATFORM)
    {
      return_op->pushDown(clv);
    }
  }
  return;
}

