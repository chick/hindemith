#ifndef DATAFLOW_H
#define DATAFLOW_H

#include "em.h"

class AcrossBlockDataFlowAnalysis
{
  public:
    std::map<EBasicBlock*, std::set<Variable*> > liveIns;
    std::map<EBasicBlock*, std::set<Variable*> > liveOuts;
    ENode * e;
    std::vector<EBasicBlock*> Blocks;

    AcrossBlockDataFlowAnalysis(ENode * e, 
                     std::vector<EBasicBlock*> Blocks) : 
		     e(e), 
		     Blocks(Blocks) {};
    void connectBlocks();
    void connectBlocksRecursive(std::list<EBasicBlock*> & blockStack,
				ENode * e, EBasicBlock * & currentBlock, 
				int &uuid);
    void doAnalysis();
    void dumpGraph();
};

class WithinBlockDataFlowAnalysis
{
  public:
    std::map<EStmt*, std::set<Variable*> > liveIns;
    std::map<EStmt*, std::set<Variable*> > liveOuts;
    EBasicBlock * BB;
    std::set<Variable*> blockLiveIns;
    std::set<Variable*> blockLiveOuts;
    WithinBlockDataFlowAnalysis(EBasicBlock * BB,
                                std::set<Variable*> blockLiveIns,
				std::set<Variable*> blockLiveOuts) : 
				BB(BB),
				blockLiveIns(blockLiveIns),
				blockLiveOuts(blockLiveOuts) {};
    void doAnalysis();
};
#endif
