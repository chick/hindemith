#include "em.h"
#include <iostream>
#include "docs.h"
#include <cassert>
extern Docs docs;

EBasicBlock::EBasicBlock(int id) : id(id), p(NULL) {};

void EBasicBlock::addStmt(EStmt * s)
{
  stmts.push_back(s);
}

void EBasicBlock::addSuccessor(EBasicBlock * BB)
{
  succs.insert(BB);
  BB->preds.insert(this);
}

size_t EBasicBlock::numStmts()
{
  return stmts.size();
}

void EBasicBlock::dumpGraph(std::string &s)
{
    char buf[80];
    for(std::set<EBasicBlock*>::iterator it=succs.begin();
	it != succs.end(); it++)
      {
	sprintf(buf, "%d -> %d;\n", id, (*it)->id);
	s += std::string(buf);
      }
}
void ENode::dumpStmts(int tab, std::stringstream & ss)
{
  for(int i = 0 ; i < tab ; i++)
  {
    ss << " ";
  }
  ss << "ENode" << std::endl;
}
void ESemanticModel::dumpStmts(int tab, std::stringstream & ss)
{
  for(int i = 0 ; i < tab ; i++)
  {
    ss << " ";
  }
  ss << "ESemanticModel" << std::endl;
  body->dumpStmts(tab+2, ss);
}
void EPipeAndFilter::dumpStmts(int tab, std::stringstream & ss)
{
  for(int i = 0 ; i < tab ; i++)
  {
    ss << " ";
  }
  ss << "EPipeAndFilter" << std::endl;
  for(std::vector<ENode*>::iterator it = stmts.begin() ;
      it != stmts.end() ; it++)
  {
    (*it)->dumpStmts(tab+2, ss);
  }
}
void EFunctionDef::dumpStmts(int tab, std::stringstream & ss)
{
  for(int i = 0 ; i < tab ; i++)
  {
    ss << " ";
  }
  ss << "EFunctionDef" << std::endl;
  body->dumpStmts(tab+2, ss);
}
void EWhile::dumpStmts( int tab, std::stringstream & ss)
{
  for(int i = 0 ; i < tab ; i++)
  {
    ss << " ";
  }
  ss << "EWhile" << std::endl;
  body->dumpStmts(tab+2, ss);
}
void EReturn::dumpStmts(int tab, std::stringstream & ss)
{
  for(int i = 0 ; i < tab ; i++)
  {
    ss << " ";
  }
  ss << "EReturn" << std::endl;
}
void EIterator::dumpStmts(int tab, std::stringstream & ss)
{
  for(int i = 0 ; i < tab ; i++)
  {
    ss << " ";
  }
  ss << "EIterator" << std::endl;
  body->dumpStmts(tab+2, ss );
}


std::string EStmt::getStr()
{
  std::stringstream ss;
  for(std::vector<EParam*>::iterator it = sinks.begin() ; 
      it != sinks.end() ; it++)
  {
    ss << (*it)->getStr();
  }
  ss << "=" << label << "(";
  for(std::vector<EParam*>::iterator it = sources.begin() ; 
      it != sources.end() ; it++)
  {
    ss << (*it)->getStr() << ",";
  }
  ss << ")";
  return ss.str();

}
void EStmt::dumpStmts(int tab, std::stringstream & ss)
{
  for(int i = 0 ; i < tab ; i++)
  {
    ss << " ";
  }
  for(std::vector<EParam*>::iterator it = sinks.begin() ; 
      it != sinks.end() ; it++)
  {
    ss << (*it)->getStr() << "\t";
  }
  ss << " = " << label << "(";
  for(std::vector<EParam*>::iterator it = sources.begin() ; 
      it != sources.end() ; it++)
  {
    ss << (*it)->getStr() << ",";
  }
  ss << ")" << std::endl;
}

void EBasicBlock::dumpStmts(int tab, std::stringstream & ss)
{
  for(int i = 0 ; i < tab ; i++)
  {
    ss << " ";
  }
  ss << "EBasicBlock" << std::endl;
  for(std::vector<EStmt*>::iterator it = stmts.begin() ;
      it != stmts.end() ; it++)
  {
    (*it)->dumpStmts(tab+2, ss);
  }
}

std::string EParam::getStr()
{
  if(variable)
  {
    return variable->properties["name"];
  }
  else
  {
    return val;
  }
}

EStmt::EStmt(std::string label, Level * levelInstance, LaunchPartition partition)
{
  this->label = label;
  id = estmt_count++;
  p = NULL;
  createStrand(partition, this, levelInstance, p);
//  addLaunch(levelInstance, partition);
}

void EStmt::addSource(Operand * sourceOperand)
{
  EParam * eparam = new EParam();
  eparam->variable = sourceOperand->variable;
  sources.push_back(eparam);

  if(p)
  {
    p->setOperand(sourceOperand, true);
  }
}

void EStmt::addSink(Operand* sinkOperand)
{
  EParam * eparam = new EParam();
  eparam->variable = sinkOperand->variable;
  sinks.push_back(eparam);

  if(p)
  {
    p->setOperand(sinkOperand, false);
  }
}
