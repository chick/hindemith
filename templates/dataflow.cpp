#include "dataflow.h"
#include <cassert>
#include <iostream>
#include <list>
#include "docs.h"
extern Docs docs;

void  AcrossBlockDataFlowAnalysis::connectBlocksRecursive(std::list<EBasicBlock*> & blockStack,
							  ENode * e, EBasicBlock * & currentBlock, 
							  int &uuid)
{
  ESemanticModel * eSemanticModel= dynamic_cast<ESemanticModel*>(e);
  EFunctionDef * efunctiondef = dynamic_cast<EFunctionDef*>(e);
  EIterator * efor = dynamic_cast<EIterator*>(e);
  EPipeAndFilter * ebody = dynamic_cast<EPipeAndFilter*>(e);
  EBasicBlock * ebasicblock = dynamic_cast<EBasicBlock*>(e);

  if(eSemanticModel)
  {
    connectBlocksRecursive(blockStack, (ENode*) eSemanticModel->body, currentBlock, uuid);
  }
  if(efunctiondef)
  {
    connectBlocksRecursive(blockStack, (ENode*) efunctiondef->body, currentBlock, uuid);
  }

  if(efor)
  {
    // EBasicBlock * ebb = endBlock; // Entry basic block
    //  printf("ebb-> = %d\n", ebb->id);
#ifdef VERBOSE_COMPILATION
    docs.compilation_ss << "stack depth = " << blockStack.size() << std::endl;
#endif
    EBasicBlock *eCondition = new EBasicBlock(uuid++); 
    if(currentBlock)
      {
	currentBlock->addSuccessor(eCondition);
      }
    currentBlock = eCondition;
    Blocks.push_back(eCondition);

    blockStack.push_back(eCondition);
    //blockStack.push_back(efor->body);


    connectBlocksRecursive(blockStack, (ENode*) efor->body, currentBlock, uuid);
    currentBlock->addSuccessor(eCondition);

    currentBlock = eCondition;
    if(blockStack.size()>1)
      {
	blockStack.pop_back();    
	EBasicBlock *eNext = blockStack.back();
	eCondition->addSuccessor(eNext);
      }
    //beginBlock->addSuccessor(endBlock); // Self loop
    //ebb->addSuccessor(beginBlock); // Connect entry basic block to beginning of body
    //eCondition->addSuccessor(beginBlock); // Connect entry basic block to beginning of body
    //endBlock->addSuccessor(eCondition);

  }
  if(ebody)
  {
    for(std::vector<ENode*>::iterator it = ebody->stmts.begin() ;
        it != ebody->stmts.end() ; it++)
    {
      connectBlocksRecursive(blockStack, (ENode*)*it, currentBlock, uuid);
    }
  }
  if(ebasicblock)
    {
      if(currentBlock)
	{
	  currentBlock->addSuccessor(ebasicblock);
	}      
      currentBlock = ebasicblock;
    }
}

void AcrossBlockDataFlowAnalysis::connectBlocks()
{
  EBasicBlock * currentBlock = NULL;
  std::list<EBasicBlock*> blockStack;
  int uuid = (1<<20);
  connectBlocksRecursive(blockStack, e, currentBlock, uuid);
}

void AcrossBlockDataFlowAnalysis::doAnalysis()
  {
    //bool changed = false;
    int cnt = 0;
    
    //walk over all stmts, compute gen and kill

    std::map<EBasicBlock*, std::set<Variable*> > killMap;
    std::map<EBasicBlock*, std::set<Variable*> > genMap;

    for(std::vector<EBasicBlock*>::iterator it = Blocks.begin();
	it != Blocks.end(); it++)
      {
	EBasicBlock *BB = *it;
	for(size_t i = 0; i < BB->numStmts(); i++)
	  {
	    EStmt *ss = dynamic_cast<EStmt*>(BB->stmts[i]);
	    assert(ss);
	    for(size_t j = 0; j < ss->sinks.size(); j++)
	      { 
		killMap[BB].insert(ss->sinks[j]->variable);
	      }
	    for(size_t j = 0; j < ss->sources.size(); j++)
	      {
		Variable *eP = ss->sources[j]->variable;
	        if(killMap[BB].count(eP) == 0)
		{
		  //printf("%s\n", eP->variable->properties["name"].c_str());
		  genMap[BB].insert(eP);
		}
	      }
	  }
#ifdef VERBOSE_COMPILATION
	  docs.compilation_ss << "Block " << BB->id << "\tgenMap.size() = " << genMap[BB].size() << std::endl;
	  docs.compilation_ss << "Block " << BB->id << "\tkillMap.size() = " << killMap[BB].size() << std::endl;
#endif
      }
   
    do
      {
	for(std::vector<EBasicBlock*>::iterator it = Blocks.begin();
	    it != Blocks.end(); it++)
	{
	  EBasicBlock *CBB = *it;

	  //remove kills from live out
	  liveIns[CBB].clear();
	  for(std::set<Variable*>::iterator lit = liveOuts[CBB].begin();
	     lit != liveOuts[CBB].end(); lit++)
	  {
	    if(killMap[CBB].count(*lit) == 0)
	      {
		liveIns[CBB].insert(*lit);
	      }
	  }
		
	  //union with gen
	  for(std::set<Variable*>::iterator lit = genMap[CBB].begin();
	      lit != genMap[CBB].end(); lit++)
	  {
	    liveIns[CBB].insert(*lit);
	  }
 
          // Liveouts gets the union of successors' live ins
	  liveOuts[CBB].clear();
	  for( std::set<EBasicBlock*>::iterator sit = CBB->succs.begin();
	       sit != CBB->succs.end(); sit++)
	  {
	    EBasicBlock *NBB = *sit;
  	    for(std::set<Variable*>::iterator ssit = liveIns[NBB].begin();
	        ssit != liveIns[NBB].end(); ssit++)
	    {
	      liveOuts[CBB].insert(*ssit);
	    }
	  }
  	  // If there is a return statement in this block then set liveouts to the return value
	  for(std::vector<EStmt*>::iterator sit = CBB->stmts.begin() ;
	      sit != CBB->stmts.end() ; sit++)
	  {
   	    EReturn * eret = dynamic_cast<EReturn*>(*sit);
	    if(eret)
	    {
	      liveOuts[CBB].insert(eret->retparam->variable);
	    }
	  }
	}
        cnt++;
      }
    while(cnt < 100);

#ifdef VERBOSE_COMPILATION
    for(std::vector<EBasicBlock*>::iterator it = Blocks.begin();
	    it != Blocks.end(); it++)
      {
	EBasicBlock *BB = *it;
	docs.top_level_dataflow_ss << "Block " << BB->id << std::endl;
	docs.top_level_dataflow_ss << "Live ins: ";
	for(std::set<Variable*>::iterator it = liveIns[BB].begin() ;
		it != liveIns[BB].end() ; it++)
	      {
		Variable * p = *it;
		docs.top_level_dataflow_ss <<  p->properties["name"] << ", ";
	      } 
	    docs.top_level_dataflow_ss << "\n";
	    
	    docs.top_level_dataflow_ss << "Live outs: ";
	    for(std::set<Variable*>::iterator it = liveOuts[BB].begin() ;
		it != liveOuts[BB].end() ; it++)
	      {
		Variable * p = *it;
		docs.top_level_dataflow_ss <<  p->properties["name"] << ", ";
	      } 
	    docs.top_level_dataflow_ss << std::endl;
	  }
#endif
  }


void AcrossBlockDataFlowAnalysis::dumpGraph()
{
  docs.compilation_ss << "Dumping " << (int)Blocks.size() << " blocks" << std::endl;
  std::string s;
  s += "digraph G {\n";

  for(size_t i = 0; i < Blocks.size(); i++)
    Blocks[i]->dumpGraph(s);

  s += "\n}\n";
  FILE *fp = fopen("bb.dot", "w");
  fprintf(fp, "%s", s.c_str());
  fclose(fp);
}

void WithinBlockDataFlowAnalysis::doAnalysis()
{
  if(BB->numStmts() == 0)
  {
#ifdef VERBOSE_COMPILATION
    docs.within_block_live_ins.push_back("");
    docs.within_block_live_outs.push_back("");
    docs.within_block_stmts.push_back("");
#endif
    return;
  }
    
  //walk over all stmts, compute gen and kill
  std::map<EStmt*, std::set<Variable*> > killMap;
  std::map<EStmt*, std::set<Variable*> > genMap;

  for(size_t i = 0; i < BB->numStmts(); i++)
  {
    EStmt *ss = dynamic_cast<EStmt*>(BB->stmts[i]);
    assert(ss);
    for(size_t j = 0; j < ss->sinks.size(); j++)
    { 
	killMap[ss].insert(ss->sinks[j]->variable);
    }
    for(size_t j = 0; j < ss->sources.size(); j++)
    {
      Variable *eP = ss->sources[j]->variable;
      //printf("%s\n", eP->variable->properties["name"].c_str());
      genMap[ss].insert(eP);
    }
    //printf("genMap.size() = %lu\n", genMap[ss].size());
    //exit(-1);
  }

  liveOuts[BB->stmts[BB->numStmts()-1]] = blockLiveOuts;
          
  // Dataflow equations
  for(ssize_t i = BB->numStmts() - 1 ; i >= 0 ; i--)
  { 
    // livein = gen U (liveout - kill)
    EStmt * stmt = BB->stmts[i];
    if(i < (ssize_t) BB->numStmts() - 1)
    {
      liveOuts[stmt].insert(liveIns[BB->stmts[i+1]].begin(), liveIns[BB->stmts[i+1]].end());
    }
    liveIns[stmt].insert(genMap[stmt].begin(), genMap[stmt].end());
    for(std::set<Variable*>::iterator it = liveOuts[stmt].begin() ;
        it != liveOuts[stmt].end() ; it++)
    {
      if(killMap[stmt].count(*it) == 0)
      {
        liveIns[stmt].insert(*it);
      }
    }
  }
   
#ifdef VERBOSE_COMPILATION
        std::stringstream stmt_ss;
	std::stringstream live_in_ss;
	std::stringstream live_out_ss;
	for(size_t i = 0; i < BB->numStmts(); i++)
	  {  
	    EStmt *n = BB->stmts[i];
	    n->dumpStmts(0, stmt_ss);

	    for(std::set<Variable*>::iterator it = liveIns[n].begin() ;
		it != liveIns[n].end() ; it++)
	      {
		Variable * p = *it;
		live_in_ss << p->properties["name"] << ", ";
	      } 
	    live_in_ss << "\n";
	    
	    for(std::set<Variable*>::iterator it = liveOuts[n].begin() ;
		it != liveOuts[n].end() ; it++)
	      {
		Variable * p = *it;
		live_out_ss << p->properties["name"] << ", ";
	      } 
	    live_out_ss << std::endl;
	  }
	  docs.within_block_live_ins.push_back(live_in_ss.str());
	  docs.within_block_live_outs.push_back(live_out_ss.str());
	  docs.within_block_stmts.push_back(stmt_ss.str());
#endif

  // Set liveins/liveouts in the strands
  if(BB->p)
  { 
#ifdef VERBOSE_COMPILATION
    docs.compilation_ss << "Setting live variables" << std::endl;
#endif
    BB->p->setLiveVariables(liveIns, liveOuts);
  }
}


