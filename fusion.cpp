#include "fusion.h"
#include <iostream>
#include <cassert>
#include "docs.h"
extern Docs docs;

Fusion::Fusion(EBasicBlock * bb) : bb(bb) {}

bool Fusion::fuse(ThreadLevel * & fuse_t, ThreadLevel * stmt_t)
{
  // Check to see if it matches at this level
  if( fuse_t->matches(stmt_t) )
  {
#ifdef VERBOSE_COMPILATION
    docs.compilation_ss << "Fuses at thread level" << std::endl;
#endif
    fuse_t->sinks.insert(fuse_t->sinks.end(), stmt_t->sinks.begin(), stmt_t->sinks.end());
    fuse_t->sources.insert(fuse_t->sources.end(), stmt_t->sources.begin(), stmt_t->sources.end());
    fuse_t->stmts.insert(fuse_t->stmts.end(), stmt_t->stmts.begin(), stmt_t->stmts.end());
    fuse_t->children.insert(fuse_t->children.end(), stmt_t->children.begin(), stmt_t->children.end());
    return true;
  }
  return false;
}

bool Fusion::fuse(BlockLevel * & fuse_b, BlockLevel * stmt_b)
{
  // Check to see if it matches at this level
  if( fuse_b->matches(stmt_b) )
  {
#ifdef VERBOSE_COMPILATION
    docs.compilation_ss << "Fuses at block level" << std::endl;
#endif
    fuse_b->sinks.insert(fuse_b->sinks.end(), stmt_b->sinks.begin(), stmt_b->sinks.end());
    fuse_b->sources.insert(fuse_b->sources.end(), stmt_b->sources.begin(), stmt_b->sources.end());
    fuse_b->stmts.insert(fuse_b->stmts.end(), stmt_b->stmts.begin(), stmt_b->stmts.end());
    if( (fuse_b->children.size() > 0) && (stmt_b->children.size() > 0) )
    {
      ThreadLevel * fuse_t = fuse_b->children.back();
      ThreadLevel * stmt_t = stmt_b->children.back();
      bool fuses = fuse(fuse_t, stmt_t);
      if(!fuses)
      {
        fuse_b->children.insert(fuse_b->children.end(), stmt_b->children.begin(), stmt_b->children.end());
      }
    }
    return true;
  }
  return false;
}

bool Fusion::fuse(DeviceLevel * & fuse_d, DeviceLevel * stmt_d)
{
  // Check to see if it matches at this level
  if( fuse_d->matches(stmt_d) )
  {
#ifdef VERBOSE_COMPILATION
    docs.compilation_ss << "Fuses at device level" << std::endl;
#endif
    fuse_d->sinks.insert(fuse_d->sinks.end(), stmt_d->sinks.begin(), stmt_d->sinks.end());
    fuse_d->sources.insert(fuse_d->sources.end(), stmt_d->sources.begin(), stmt_d->sources.end());
    fuse_d->stmts.insert(fuse_d->stmts.end(), stmt_d->stmts.begin(), stmt_d->stmts.end());

    // Try to fuse the next level for each device (CPU/GPU)
    if((fuse_d->blockList.size() > 0) && (stmt_d->blockList.size() > 0))
    {
      BlockLevel * fuse_b = fuse_d->blockList.back();
      BlockLevel * stmt_b = stmt_d->blockList.back();
      bool fuses = fuse(fuse_b, stmt_b);
      if(!fuses)
      {
        fuse_d->blockList.insert(fuse_d->blockList.end(), stmt_d->blockList.begin(), stmt_d->blockList.end());
      }
    }
    return true;
  }
  return false;
}
bool Fusion::fuse(PlatformLevel * & fuse_p, PlatformLevel * stmt_p)
{
  if(!fuse_p)
  {
    fuse_p = stmt_p;
    return true;
  }
#ifdef VERBOSE_COMPILATION
  docs.compilation_ss << "Fuses at platform level" << std::endl;
#endif
  fuse_p->sinks.insert(fuse_p->sinks.end(), stmt_p->sinks.begin(), stmt_p->sinks.end());
  fuse_p->sources.insert(fuse_p->sources.end(), stmt_p->sources.begin(), stmt_p->sources.end());
  fuse_p->stmts.insert(fuse_p->stmts.end(), stmt_p->stmts.begin(), stmt_p->stmts.end());

  if(getenv("HM_DISABLE_FUSION"))
  {
    fuse_p->children.insert(fuse_p->children.end(), stmt_p->children.begin(), stmt_p->children.end());
    return true;
  }
 
  // Try to fuse the next level
  if((fuse_p->children.size() > 0) && (stmt_p->children.size() > 0))
  {
    DeviceLevel * fuse_d = fuse_p->children.back();
    DeviceLevel * stmt_d = stmt_p->children.back();
    bool fuses = fuse(fuse_d, stmt_d);
    if(!fuses)
    {
      fuse_p->children.insert(fuse_p->children.end(), stmt_p->children.begin(), stmt_p->children.end());
    }
  }
  return true;
}

void Fusion::doFusion()
{
  std::list<PlatformLevel*> strands;
  std::vector<EStmt*> stmts(bb->stmts);
  bb->stmts.clear();
#ifdef VERBOSE_COMPILATION
  docs.compilation_ss << "Scheduling: " << std::endl;
#endif

  // Populate list of strands
  for(std::vector<EStmt*>::iterator it = stmts.begin() ;
      it != stmts.end() ; it++)
  {
#ifdef VERBOSE_COMPILATION
    (*it)->dumpStmts(0, docs.compilation_ss);
#endif
    if((*it)->p)
    {
      strands.push_back((*it)->p);
    }
  }
#ifdef VERBOSE_COMPILATION
  docs.compilation_ss <<  std::endl;
#endif

  // Build use-def graph
  std::list<Edge*> edges;
  for(std::vector<EStmt*>::iterator it1 = stmts.begin() ;
      it1 != stmts.end() ; it1++)
  {
    EStmt * s1 = *it1;
    if(s1->sinks.size() == 0) continue;

    for(std::vector<EStmt*>::iterator it2 = it1 ;
        it2 != stmts.end() ; it2++)
    {
      EStmt * s2 = *it2;
      if(s1 == s2) continue;

      // RAW dependence
      for(std::vector<EParam*>::iterator it = s1->sinks.begin() ;
          it != s1->sinks.end() ; it++)
      {
        EParam * s1_sink = *it;
        for(std::vector<EParam*>::iterator it = s2->sources.begin() ;
            it != s2->sources.end() ; it++)
        {
          EParam * s2_src = *it;
  	  if(s1_sink->variable && s2_src->variable)
	  {
            if(s1_sink->variable->properties["name"] == s2_src->variable->properties["name"])
            {
              Edge * e = new Edge(s1, s1_sink, s2, s2_src);
              edges.push_back(e);
            }
  	  }
        }
      }

      // WAW dependence
      if(s2->sinks.size() == 0) continue;
      for(std::vector<EParam*>::iterator it = s1->sinks.begin() ;
          it != s1->sinks.end() ; it++)
      {
        EParam * s1_sink = *it;
        for(std::vector<EParam*>::iterator it2 = s2->sinks.begin() ;
            it2 != s2->sinks.end() ; it2++)
        {
          EParam * s2_sink = *it2;
          if(s1_sink->variable && s2_sink->variable)
          {
            if(s1_sink->variable->properties["name"] == s2_sink->variable->properties["name"])
            {
              Edge * e = new Edge(s1, s1_sink, s2, s2_sink);
              edges.push_back(e);
            }
          }
        } // s2.sinks
      } // s1.sinks

      // WAR dependence
      for(std::vector<EParam*>::iterator it = s1->sources.begin() ;
          it != s1->sources.end() ; it++)
      {
        EParam * s1_source = *it;
        for(std::vector<EParam*>::iterator it = s2->sinks.begin() ;
            it != s2->sinks.end() ; it++)
        {
          EParam * s2_sink = *it;
  	  if(s1_source->variable && s2_sink->variable)
	  {
            if(s1_source->variable->properties["name"] == s2_sink->variable->properties["name"])
            {
              Edge * e = new Edge(s1, s1_source, s2, s2_sink);
              edges.push_back(e);
            }
  	  }
        }
      }
    } // statement 2
  } // statement 1


  std::set<EStmt*> taken_stmts;
  //std::set<EStmt*> available_stmts;
  std::vector<EStmt*> available_stmts;
#ifdef VERBOSE_COMPILATION
  docs.compilation_ss << "Building available\n";
#endif
  for(std::vector<EStmt*>::iterator it = stmts.begin() ; it != stmts.end() ; it++)
  {
    //available_stmts.insert(*it);
    available_stmts.push_back(*it);
#ifdef VERBOSE_COMPILATION
    (*it)->dumpStmts(0, docs.compilation_ss);
#endif
  }

  while(available_stmts.size() > 0)
  {
    // Get set of ready stmts 
    //std::set<EStmt*> ready_stmts;
    std::vector<EStmt*> ready_stmts;
    for(std::vector<EStmt*>::iterator it = available_stmts.begin() ;
        it != available_stmts.end() ; it++)
    {
      // If there are no incoming edges
      bool ready = true;
      for(std::list<Edge*>::iterator it2 = edges.begin() ; it2 != edges.end() ; it2++)
      {
        Edge * edge = *it2;
        if((edge->dst == *it) && (taken_stmts.count(edge->src) == 0))
        {
          ready = false;
        }
      }
      if(ready)
      {
        //ready_stmts.insert(*it);
        ready_stmts.push_back(*it);
      }
    }

/*
    // Issue ready statements (discover which has the best fusion)
    std::pair<EStmt*, MemLevel> max_num_levels = std::make_pair((EStmt*)NULL, NOLEVEL);
    for(std::set<EStmt*>::iterator it = ready_stmts.begin() ; it != ready_stmts.end() ; it++)
    {
      MemLevel num_levels = get_fusion_depth((*it)->p, bb->p);
      if(num_levels > max_num_levels.second)
      {
        max_num_levels.first = *it;
	max_num_levels.second = num_levels;
      }
    }
    if(max_num_levels.second >= PLATFORM)
    {
      printf("Issuing: \n");
      printf("Max num levels: %d\n", max_num_levels.second);
      assert((max_num_levels.first));
      (max_num_levels.first)->dumpStmts(0);
      // Fuse the statement with the max num levels
      fuse((max_num_levels.first)->p, bb->p, max_num_levels.second);
      taken_stmts.insert(max_num_levels.first);
      available_stmts.erase(max_num_levels.first);
      bb->stmts.push_back(max_num_levels.first);
    }
    */
    
    // Just take the first ready statement
    EStmt * ready_stmt = *(ready_stmts.begin());
#ifdef VERBOSE_COMPILATION
    docs.compilation_ss << "Issuing: " << std::endl;
    ready_stmt->dumpStmts(0, docs.compilation_ss);
#endif
    fuse(bb->p, ready_stmt->p);
    taken_stmts.insert(ready_stmt);
    //available_stmts.erase(ready_stmt);
    // Erase
    std::vector<EStmt*>::iterator erase_it;
    for(std::vector<EStmt*>::iterator eit = available_stmts.begin() ; 
        eit != available_stmts.end() ; eit++)
    {
      if((EStmt*) *eit == ready_stmt)
      {
        erase_it = eit;
      }
    }
    available_stmts.erase(erase_it);
    bb->stmts.push_back(ready_stmt);
  }

#ifdef VERBOSE_COMPILATION
  // Print fused PlatformLevel
  if(bb->p)
  {
    std::stringstream ss;
    bb->p->dumpStmts(0, ss);
    docs.fusion_summaries.push_back(ss.str());
  }
  else
  {
    docs.fusion_summaries.push_back("");
  }

  // Print use-def graph with kernel boundaries
  static int uid = 0;
  char buf[255];
  char bufdot[255];
  sprintf(bufdot, "sched_graph_%d.dot", uid);
  sprintf(buf, "sched_graph_%d", uid++);
  docs.sched_graphs.push_back(buf);
  FILE * fp = fopen(bufdot, "w");
  fprintf(fp, "digraph G {\n");
  docs.compilation_ss << "Edges: " << std::endl;
  std::set<EStmt*> stmt_set;
  for(std::list<Edge*>::iterator it = edges.begin() ;
      it != edges.end() ; it++)
  {
    stmt_set.insert((*it)->src);
    stmt_set.insert((*it)->dst);
  }


  PlatformLevel * p = bb->p;
  if(p)
  {
  for(std::vector<DeviceLevel*>::iterator it = p->children.begin() ; it != p->children.end() ; it++)
  {
    DeviceLevel * d = (DeviceLevel*) *it;
    std::vector<BlockLevel*> children = d->blockList;
    for(std::vector<BlockLevel*>::iterator it2 = children.begin() ; it2 != children.end() ; it2++)
    {
      BlockLevel * b = (BlockLevel*) *it2;
      fprintf(fp, "subgraph cluster_%p {\n", b);
      for(std::vector<EStmt*>::iterator it3 = b->stmts.begin() ; it3 != b->stmts.end() ; it3++)
      {
        fprintf(fp, "node [label =\"%s\" ] p%p;\n", (*it3)->getStr().c_str(), (*it3) );
      }
      fprintf(fp, "}\n");
    }
  }
  }

/*
  for(std::set<EStmt*>::iterator it = stmt_set.begin() ; it != stmt_set.end() ; it++)
  {
    fprintf(fp, "p%p [label =\"%s\"];\n", (*it), (*it)->getStr().c_str());
  }
  */

  for(std::list<Edge*>::iterator it = edges.begin() ;
      it != edges.end() ; it++)
  {
    EStmt * src = (*it)->src;
    EStmt * dst = (*it)->dst;
    //fprintf(fp, "%s -> %s;\n", (*it)->src_param->variable->properties["name"].c_str(), (*it)->dst_param->variable->properties["name"].c_str());
    fprintf(fp, "p%p -> p%p;\n", src, dst);
    docs.compilation_ss << (*it)->src_param->variable->properties["name"] << "\t->\t" << (*it)->dst_param->variable->properties["name"] << std::endl;
  }
  fprintf(fp, "}\n");
  fclose(fp);


#endif

}
