#ifndef __LOGS_H
#define __LOGS_H

#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <vector>

class Docs
{
  public:
    // Streams for latex file
    std::stringstream opencl_ss;
    std::stringstream top_level_stmts_ss;
    std::stringstream top_level_dataflow_ss;
    std::vector<std::string> block_dumps;
    std::vector<std::string> within_block_live_ins;
    std::vector<std::string> within_block_live_outs;
    std::vector<std::string> within_block_stmts;
    std::vector<std::string> fusion_summaries;
    std::vector<std::string> sched_graphs;

    std::stringstream compilation_ss;
    std::stringstream execution_ss;
    std::stringstream runtime_ss;
    std::stringstream profile_ss;

    std::map<int, std::stringstream> bb_dots;
    Docs();
    void write_files();
    void write_stdout();

};

#endif
