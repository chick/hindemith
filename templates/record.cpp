
std::ofstream logfile;
std::ofstream treefile;
static int treeCount = 100;

void openLog(const char * fname)
{
  logfile.open(fname);
}

void closeLog()
{
  logfile.close();
}

void startDigraph(int blockId)
{
  std::stringstream fname_stream;
  fname_stream << "reports/" << "tree_" << blockId << "_" << treeCount << ".dot";
  treefile.open(fname_stream.str().c_str());
  treefile << "digraph G {" << std::endl;
  treeCount++;
}

void endDigraph()
{
  treefile << "}" << std::endl;
  treefile.close();
}

void emit_tree(PlatformLevel * p)
{
  if(p)
  {
    treefile << p->id << " [label=\"" << p->getStr() << "\", shape=\"record\"];" << std::endl;
    for(std::vector<DeviceLevel*>::iterator it = p->devices.begin() ; 
      it != p->devices.end() ; it++)
    {
      treefile << p->id << " -> " << (*it)->id << ";" << std::endl;
      treefile << (*it)->id << " [label=\"" << (*it)->getStr() << "\", shape=\"record\"];" << std::endl;
      emit_tree(*it);
    }
  }
}

void emit_tree(DeviceLevel * p)
{
  std::vector<BlockLevel*> blocks = p->blockList;
  for(std::vector<BlockLevel*>::iterator it = blocks.begin() ; 
      it != blocks.end() ; it++)
  {
    treefile << p->id << " -> " << (*it)->id << ";" << std::endl;
    treefile << (*it)->id << " [label=\"" << (*it)->getStr() << "\", shape=\"record\"];" << std::endl;
    emit_tree(*it);
  }
}

void emit_tree(BlockLevel * p)
{
  for(std::vector<ThreadLevel*>::iterator it = p->threads.begin() ; 
      it != p->threads.end() ; it++)
  {
    treefile << p->id << " -> " << (*it)->id << ";" << std::endl;
    treefile << (*it)->id << " [label=\"" << (*it)->getStr() << "\", shape=\"record\"];" << std::endl;
    emit_tree(*it);
  }
}

void emit_tree(ThreadLevel * p)
{
  for(std::vector<ElementLevel*>::iterator it = p->elements.begin() ; 
      it != p->elements.end() ; it++)
  {
    // Label with dumpStmts
    treefile << p->id << " -> " << (*it)->id << ";" << std::endl;
    treefile << (*it)->id << " [label=\"" << (*it)->getStr() << "\", shape=\"record\"];" << std::endl;
  }
}

void emit_tree(ElementLevel * e)
{
  {
  }
}

void emit_tree(EBasicBlock * bb)
{
  emit_tree(bb->p);
}
