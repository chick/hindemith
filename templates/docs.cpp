#include "docs.h"
#include <fstream>

const char * latex_makefile_text = 
"all: compilation_report\n"
"DOTS = $(wildcard *.dot)\n"
"PDFS = $(patsubst %.dot,%.pdf,$(DOTS))\n"
"compilation_report: compilation_report.tex $(PDFS)\n"
"\tpdflatex $^\n"
"%.pdf: %.dot\n"
"\tdot -Tpdf $< -o $@\n"
"clean:\n"
"\trm -f compilation_report.pdf $(PDFS)\n";

Docs::Docs()
{
}

void Docs::write_files()
{
#ifdef VERBOSE_COMPILATION
  std::ofstream compilation_file;
  compilation_file.open("compilation.log");
  compilation_file << compilation_ss.str();
  compilation_file.close();
#endif

#ifdef VERBOSE_EXECUTION
  std::ofstream execution_file;
  execution_file.open("execution.log");
  execution_file << execution_ss.str();
  execution_file.close();
#endif

#ifdef PROFILE_TOP
  std::ofstream runtime_file;
  runtime_file.open("runtime.log");
  runtime_file << runtime_ss.str();
  runtime_file.close();
#endif

#ifdef PROFILE
  std::ofstream profile_file;
  profile_file.open("profile.log");
  profile_file << profile_ss.str();
  profile_file.close();

  std::ofstream latex_file;
  latex_file.open("compilation_report.tex");
  latex_file << "\\documentclass[12pt]{article}\n"
                "\\usepackage{pdfpages}\n"
                "\\usepackage[landscape]{geometry}\n"
                "\\begin{document}\n";
  latex_file << "\\small" << std::endl;
  latex_file << "\\section{OpenCL Setup}" << std::endl;
  latex_file << "\\begin{verbatim}\n" << opencl_ss.str() << "\\end{verbatim}\n";

  latex_file << "\\section{Overview}" << std::endl;
  latex_file << "\\subsection{Execution Model}" << std::endl;
  latex_file << "\\begin{verbatim}\n" << top_level_stmts_ss.str() << "\\end{verbatim}" << std::endl;
  latex_file << "\\subsection{Dataflow}" << std::endl;
  latex_file << "\\includepdf[pages={1}]{bb.pdf}" << std::endl;
  latex_file << "\\begin{verbatim}\n" << top_level_dataflow_ss.str() << "\\end{verbatim}\n";

  latex_file << "\\section{Basic Blocks}" << std::endl;
  for(size_t cnt = 0 ; cnt < block_dumps.size() ; cnt++)
  {
    latex_file << "\\subsection{Block " << cnt << "}" << std::endl;
    latex_file << "\\subsubsection{Statements}" << std::endl;
    latex_file << "\\begin{verbatim}\n" << within_block_stmts[cnt] << "\\end{verbatim}" << std::endl;
    latex_file << "\\subsubsection{Live Ins}" << std::endl;
    latex_file << "\\begin{verbatim}\n" << within_block_live_ins[cnt] << "\\end{verbatim}" << std::endl;
    latex_file << "\\subsubsection{Live Outs}" << std::endl;
    latex_file << "\\begin{verbatim}\n" << within_block_live_outs[cnt] << "\\end{verbatim}" << std::endl;
    latex_file << "\\subsubsection{Fusion}" << std::endl;
    latex_file << "\\begin{verbatim}\n" << fusion_summaries[cnt] << "\\end{verbatim}" << std::endl;
    latex_file << "\\includepdf[pages={1}]{"  << sched_graphs[cnt] << ".pdf}" << std::endl;
  }
  latex_file << "\\end{document}";
  latex_file.close();

  std::ofstream latex_makefile;
  latex_makefile.open("makefile.report");
  latex_makefile << latex_makefile_text << std::endl;
  latex_makefile.close();
#endif
}

void Docs::write_stdout()
{
}

