#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cassert>

#include "sm.h"
#include "clhelp.h"
#include "docs.h"
#include "operations.h"

extern Docs docs;

/* Semantic model node constructors */
Node::Node(std::string label) 	: label(label) {};
Node::Node() 			: label("GenericNode") {};
Stmt::Stmt(std::string label) 	: Node(label) {};
Stmt::Stmt() 	: Node("Stmt") {};

/* Symbol Table */
SymbolTable::SymbolTable() 	: Node("SymbolTable") {};
ConstantTable::ConstantTable() 	: Node("ConstantTable") {};
Variable::Variable() 		: Node("Variable"), lives_in_python(false), level(PLATFORM), operand(NULL) 
{
  if(getenv("ZERO_COPY"))
  {
    use_host_ptr = true;
  }
  else
  {
    use_host_ptr = false;
  }
};
Constant::Constant()		: Node("Constant") {};
ScalarConstant::ScalarConstant() : Constant() {};
Array::Array() 			: Variable()
{
  cpu_data = NULL;

  // Get num devices
  gpu_data = new cl_mem[MAX_DEVICES];
};
Array2D::Array2D() 			: Variable()
{
  cpu_data = NULL;

  // Get num devices
  gpu_data = new cl_mem[MAX_DEVICES];
}
Array3D::Array3D() 			: Variable()
{
  cpu_data = NULL;

  // Get num devices
  gpu_data = new cl_mem[MAX_DEVICES];
}
Stencil::Stencil() 			: Variable()
{
  cpu_data = NULL; cpu_offx = NULL; cpu_offy = NULL;
  gpu_data = new cl_mem[MAX_DEVICES];
  gpu_offx = new cl_mem[MAX_DEVICES];
  gpu_offy = new cl_mem[MAX_DEVICES];
};

SpMat::SpMat() 			: Variable()
{
  cpu_data = NULL;
  gpu_data = new cl_mem[MAX_DEVICES];
  gpu_offsets = new cl_mem[MAX_DEVICES];
};

SpMat2D::SpMat2D() 			: Variable()
{
  cpu_data = NULL;
  gpu_data = new cl_mem[MAX_DEVICES];
  gpu_offx = new cl_mem[MAX_DEVICES];
  gpu_offy = new cl_mem[MAX_DEVICES];
};

CSRMat::CSRMat() 			: Variable()
{
  cpu_data = NULL;
  gpu_data = new cl_mem[MAX_DEVICES];
  gpu_indices = new cl_mem[MAX_DEVICES];
  gpu_indptr = new cl_mem[MAX_DEVICES];
};

Scalar::Scalar() 		: Variable()
{
  cpu_data = NULL;
  gpu_data = new cl_mem[MAX_DEVICES];
};

/* Hindemith AST */
SemanticModel::SemanticModel() 	: Node("SemanticModel") {};
FunctionDef::FunctionDef() 	: Node("FunctionDef") {};
Iterator::Iterator() 	: Node("Iterator") {};
PipeAndFilter::PipeAndFilter() 	: Node("PipeAndFilter") {};
Name::Name() 			: Node("Name") {};

void Node::parse(pugi::xml_node node)
{
  for(pugi::xml_attribute attribute = node.first_attribute() ;
      attribute; attribute = attribute.next_attribute())
  {
    properties[attribute.name()] = attribute.value();
  }
}

int Node::get_property_int(std::string key)
{
  return atoi(properties[key].c_str());
}

struct sm_visitor: pugi::xml_tree_walker
{
    std::vector<Node*> parents;
    virtual bool for_each(pugi::xml_node& node)
    {
        while((int)parents.size() > (int)depth())
	{
	  parents.pop_back();
	}

        Node * ast_node;

        /* Pick the right node */
        /* Symbol table */
	if(!strcmp(node.name(), "SymbolTable"))
	{
	  ast_node = new SymbolTable();
	  ast_node->parse(node);
	}
	else if(!strcmp(node.name(), "ConstantTable"))
	{
          ast_node = new ConstantTable();
	  ast_node->parse(node);
	}
	else if(!strcmp(node.name(), "ScalarConstant"))
	{
          ast_node = new ScalarConstant();
	  ast_node->parse(node);
	}
	else if(!strcmp(node.name(), "SpMat"))
	{
          ast_node = new SpMat();
	  ast_node->parse(node);
	}
	else if(!strcmp(node.name(), "SpMat2D"))
	{
          ast_node = new SpMat2D();
	  ast_node->parse(node);
	}
	else if(!strcmp(node.name(), "Stencil"))
	{
          ast_node = new Stencil();
	  ast_node->parse(node);
	}
	else if(!strcmp(node.name(), "CSRMat"))
	{
          ast_node = new CSRMat();
	  ast_node->parse(node);
	}
	else if(!strcmp(node.name(), "Array"))
	{
          ast_node = new Array();
	  ast_node->parse(node);
	}
	else if(!strcmp(node.name(), "Array2D"))
	{
          ast_node = new Array2D();
	  ast_node->parse(node);
	}
	else if(!strcmp(node.name(), "Array3D"))
	{
          ast_node = new Array3D();
	  ast_node->parse(node);
	}
	else if(!strcmp(node.name(), "Scalar"))
	{
          ast_node = new Scalar();
	  ast_node->parse(node);
	}

        /* Hindemith AST */
	else if(!strcmp(node.name(), "SemanticModel"))
	{
          ast_node = new SemanticModel();
	  ast_node->parse(node);
	  ast_node->label = ast_node->properties["name"];
	}
	else if(!strcmp(node.name(), "FunctionDef"))
	{
          ast_node = new FunctionDef();
	  ast_node->parse(node);
	  ast_node->label = ast_node->properties["name"];
	}
	else if(!strcmp(node.name(), "Iterator"))
	{
          ast_node = new Iterator();
	  ast_node->parse(node);
	  ast_node->label = ast_node->properties["name"];
	}
	else if(!strcmp(node.name(), "PipeAndFilter"))
	{
          ast_node = new PipeAndFilter();
	  ast_node->parse(node);
	  ast_node->label = ast_node->properties["name"];
	}
	else if(!strcmp(node.name(), "Filter"))
	{
	  ast_node = new Stmt();
	  ast_node->parse(node);
	  ast_node->label = ast_node->properties["name"];
	}
	else if(!strcmp(node.name(), "Name"))
	{
          ast_node = new Name();
	  ast_node->parse(node);
	  ast_node->label = ast_node->properties["name"];
	}
	else
	{
	  std::cout << "Unknown node: " << node.name() << std::endl;
	  exit(-1);
	}

	setFunctionPointer((Stmt*)ast_node);

	if(depth() > 0)
	{
	  Node * parent = parents[depth() - 1];
	  parent->children.push_back(ast_node);
	}
        if((int)parents.size() <= (int)depth())
	{
	  parents.push_back(ast_node); // Push self
	}
        return true; // continue traversal
    }
};

void smparse(const char * xmlstring, Node ** p)
{
  pugi::xml_document doc;
  pugi::xml_parse_result result = doc.load_buffer(xmlstring, strlen(xmlstring));
  if(!result)
  {
    std::cout << "XML parsing failed" << std::endl;
    return;
  }
  else
  {
#ifdef VERBOSE_COMPILATION
    docs.compilation_ss << "Parsing successful" << std::endl;
#endif
  }

  sm_visitor vst;
  doc.traverse(vst);

  // Return the root FunctionDef node
  *p = (Node*)vst.parents[0];
}

void print_info(Node * node)
{
}

void build_symbol_table(SymbolTable * sm, std::map<std::string, Variable*> & sym_tab)
{
  sym_tab.clear();
#ifdef VERBOSE_COMPILATION
  docs.compilation_ss << "Building symbol table" << std::endl;
#endif
  
  for(std::vector<Node*>::iterator it = sm->children.begin() ; 
      it != sm->children.end() ; it++)
  {
    sym_tab[(*it)->properties["name"]] = (Variable *) (*it);
  }
}

void build_constant_table(ConstantTable * sm, std::map<std::string, Constant*> & cst_tab)
{
  cst_tab.clear();
#ifdef VERBOSE_COMPILATION
  docs.compilation_ss << "Building constant table" << std::endl;
#endif
  
  for(std::vector<Node*>::iterator it = sm->children.begin() ; 
      it != sm->children.end() ; it++)
  {
    cst_tab[(*it)->properties["name"]] = (Constant*) (*it);
  }
}

void Variable::allocate(cl_vars_t clv) {}

void allocate_memory(std::map<std::string, Variable*> & sym_tab, cl_vars_t & clv)
{
#ifdef VERBOSE_COMPILATION
  docs.compilation_ss << "Allocating memory" << std::endl;
#endif

  for(std::map<std::string, Variable*>::iterator it = sym_tab.begin() ; it != sym_tab.end() ; it++)
  {
    Variable * v = it->second;
    v->allocate(clv);
  }
}

void print_vars(SymbolTable * sm, const cl_vars_t clv)
{
  std::cout << "Printing variable CPU data" << std::endl;
}

