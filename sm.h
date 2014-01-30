#ifndef __SMPARSE_H
#define __SMPARSE_H

#include <string>
#include <sstream>
#include <map>
#include <list>
#include <set>
#include <vector>
#include "pugixml.hpp"
#include "clhelp.h"
#include "lm.h"

#define MYMAX(a,b) ((a > b) ? a : b)

/* Generic Node */
class Node 
{
  public:
    std::string label;
    std::map<std::string, std::string> properties;
    std::vector<Node*> children;

    Node();
    Node(std::string label);
    virtual void parse(pugi::xml_node node);
    int get_property_int(std::string key);
};

/* Symbol Table */
class SymbolTable : public Node
{
  public:
    SymbolTable();
};

class ConstantTable : public Node
{
  public:
    ConstantTable();
};

class PlatformOperand;
class DeviceOperand;
class BlockOperand;
class ThreadOperand;
class ElementOpernad;

class Constant : public Node
{
  public:
    Constant();
};

class ScalarConstant : public Constant
{
  public:
    ScalarConstant();
    std::string value;
};

class Variable : public Node
{
  public:
    bool lives_in_python;
    bool use_host_ptr;
    MemLevel level;
    Operand * operand;

    Variable();
    virtual void allocate(cl_vars_t clv);
    virtual std::string getDeclaration(MemLevel mlev) {return "";};
    virtual int getTotalBytes() {return 0;};
    virtual std::string parameterStr() = 0;
    virtual void setParameter(cl_vars_t clv, cl_kernel kernel, int & arg_cnt, int device_id) = 0;
};

class Stencil : public Variable
{
  public:
    size_t bytes_per_element;
    size_t nbytes_data;
    size_t nbytes_offx;
    size_t nbytes_offy;
    char * cpu_data;
    char * cpu_offx;
    char * cpu_offy;
    int ndiags;
    int ndevices;
    cl_mem * gpu_data;
    cl_mem * gpu_offx;
    cl_mem * gpu_offy;

    Stencil();
    void allocate(cl_vars_t clv);
    /*virtual*/ int getTotalBytes();
    /*virtual*/ std::string parameterStr();
    /*virtual*/ void setParameter(cl_vars_t clv, cl_kernel kernel, int & arg_cnt, int device_id);
};


class SpMat : public Variable
{
  public:
    size_t bytes_per_element;
    size_t nbytes_data;
    size_t nbytes_offsets;
    size_t ndims;
    size_t ndiags;
    char * cpu_data;
    char * cpu_offsets;
    int ndevices;
    cl_mem * gpu_data;
    cl_mem * gpu_offsets;

    SpMat();
    void allocate(cl_vars_t clv);
    /*virtual*/ int getTotalBytes();
    /*virtual*/ std::string parameterStr();
    /*virtual*/ void setParameter(cl_vars_t clv, cl_kernel kernel, int & arg_cnt, int device_id);
    int getDim();
};

class SpMat2D : public Variable
{
  public:
    size_t bytes_per_element;
    size_t nbytes_data;
    size_t nbytes_offx;
    size_t nbytes_offy;
    size_t ndims;
    size_t ndiags;
    char * cpu_data;
    char * cpu_offx;
    char * cpu_offy;
    int ndevices;
    cl_mem * gpu_data;
    cl_mem * gpu_offx;
    cl_mem * gpu_offy;

    SpMat2D();
    void allocate(cl_vars_t clv);
    /*virtual*/ int getTotalBytes();
    /*virtual*/ std::string parameterStr();
    /*virtual*/ void setParameter(cl_vars_t clv, cl_kernel kernel, int & arg_cnt, int device_id);
    int getDim0();
    int getDim1();
};

class CSRMat : public Variable
{
  public:
    size_t bytes_per_element;
    size_t nbytes_data;
    size_t nbytes_indices;
    size_t nbytes_indptr;
    void * cpu_data;
    void * cpu_indices;
    void * cpu_indptr;
    cl_mem * gpu_data;
    cl_mem * gpu_indices;
    cl_mem * gpu_indptr;
    int ndevices;

    CSRMat();
    void allocate(cl_vars_t clv);
    /*virtual*/ int getTotalBytes();
    /*virtual*/ std::string parameterStr();
    /*virtual*/ void setParameter(cl_vars_t clv, cl_kernel kernel, int & arg_cnt, int device_id);
    int getDim0();
    int getDim1();
};

class Array : public Variable
{
  public:
    bool lives_in_python;
    size_t bytes_per_element;
    size_t nbytes;

    // Platform
    char * cpu_data;

    // Device
    int ndevices;
    cl_mem * gpu_data;

    Array();
    void allocate(cl_vars_t clv);
    /*virtual*/ std::string getDeclaration(MemLevel mlev);
    /*virtual*/ int getTotalBytes();
    /*virtual*/ std::string parameterStr();
    /*virtual*/ void setParameter(cl_vars_t clv, cl_kernel kernel, int & arg_cnt, int device_id);
    int getDim();
};

class Array3D : public Variable
{
  public:
    bool lives_in_python;
    size_t bytes_per_element;
    size_t nbytes;

    // Platform
    char * cpu_data;

    // Device
    int ndevices;
    cl_mem * gpu_data;

    Array3D();
    void allocate(cl_vars_t clv);
    /*virtual*/ std::string getDeclaration(MemLevel mlev);
    /*virtual*/ int getTotalBytes();
    /*virtual*/ std::string parameterStr();
    /*virtual*/ void setParameter(cl_vars_t clv, cl_kernel kernel, int & arg_cnt, int device_id);
    int getDim0();
    int getDim1();
    int getDim2();
};


class Array2D : public Variable
{
  public:
    bool lives_in_python;
    size_t bytes_per_element;
    size_t nbytes;

    // Platform
    char * cpu_data;

    // Device
    int ndevices;
    cl_mem * gpu_data;

    Array2D();
    void allocate(cl_vars_t clv);
    /*virtual*/ std::string getDeclaration(MemLevel mlev);
    /*virtual*/ int getTotalBytes();
    /*virtual*/ std::string parameterStr();
    /*virtual*/ void setParameter(cl_vars_t clv, cl_kernel kernel, int & arg_cnt, int device_id);
    int getDim0();
    int getDim1();
};

class Scalar : public Variable
{
  public:
    size_t bytes_per_element;
    size_t nbytes;
    size_t ndims;
    char * cpu_data;
    int ndevices;
    cl_mem * gpu_data;

    Scalar();
    /*virtual*/ std::string getDeclaration(MemLevel mlev);
    /*virtual*/ int getTotalBytes();
    /*virtual*/ std::string parameterStr();
    /*virtual*/ void setParameter(cl_vars_t clv, cl_kernel kernel, int & arg_cnt, int device_id);
    void allocate(cl_vars_t clv);
};

/* Hindemith AST */
class SemanticModel : public Node 
{
  public:
    SemanticModel();
};

class FunctionDef: public Node 
{
  public:
    FunctionDef();
};

class Iterator : public Node
{
  public:
    Iterator();
};

class PipeAndFilter: public Node 
{
  public:
    PipeAndFilter();
};

class Name: public Node
{
  public:
    Name();
};

class EStmt;
class Stmt : public Node
{
  public:
    Stmt();
    Stmt(std::string label);
    void (*setupLaunch)(std::vector<EStmt*> & e, std::vector<Variable*> vars, std::map<std::string, Variable*> & symbolTable, std::map<std::string, Constant*> & constantTable, cl_vars_t clv, Stmt * stmt) ;
};

void smparse(const char * xmlstring, Node ** p);
void build_symbol_table(SymbolTable * p, std::map<std::string, Variable*> & sym_tab);
void build_constant_table(ConstantTable * p, std::map<std::string, Constant*> & cst_tab);
void allocate_memory(std::map<std::string, Variable*> & sym_tab, cl_vars_t & clv);
void free_memory(SymbolTable * sm, cl_vars_t & clv);
void print_vars(SymbolTable * sm, const cl_vars_t clv);
void print_info(Node * node);
Variable * add_variable_from(std::string varname, std::map<std::string, Variable*> & sym_tab, const cl_vars_t clv);

#endif
