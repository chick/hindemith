#include <cstdio>
#include <cstdlib>
#include <string.h>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <cstring>

#ifdef __linux__
extern "C" {
#include <execinfo.h>
#include <python2.7/Python.h>
#include <python2.7/pyconfig.h>
#include <python2.7/numpy/arrayobject.h>
#include <python2.7/numpy/arrayscalars.h>
};
#elif __APPLE__
#include <execinfo.h>
#include <python2.7/Python.h>
#include <python2.7/pyconfig.h>
#include <numpy/arrayobject.h>
#include <numpy/arrayscalars.h>
#else
#error Unsupported OS
#endif

#include "sm.h"
#include "oclgen.h"
#include "clhelp.h"
#include "timestamp.h"
#include "docs.h"
#include "profiler.h"

#define EXIT_MSG(str) {\
	printf("Error: %s\n", str); \
	Py_INCREF(Py_None); \
	return Py_None; \
}

#define MAX_RETURN_DIMS 10

static char xmlcompile_doc[] = 
"This module is just a simple example.  It provides one xmlcompiletion: xmlcompile().";

static SymbolTable * symbolTable; // One per ID
static ConstantTable * constantTable; // One per ID
static SemanticModel * semanticModel; // One per ID
static ENode * executionModel; // One per ID
static std::map<std::string, Variable*> sym_tab; // One per ID
static std::map<std::string, Constant*> cst_tab; // One per ID

static std::map<int, SymbolTable*> symbolTables;
static std::map<int, SemanticModel*> semanticModels;
static std::map<int, ConstantTable*> constantTables;
static std::map<int, ENode*> executionModels;
static std::map<int, std::map<std::string, Variable*> > sym_tabs;
static std::map<int, std::map<std::string, Constant*> > cst_tabs;

static cl_vars_t clv; // Global (initialize in its own function)

int variable_counter;
bool enable_register_allocation;
Docs docs;
std::vector<kernel_prof_t> kernel_events;
std::vector<transfer_prof_t> transfer_events;

static PyObject*
xmlcompile(PyObject *self, PyObject *args, PyObject *kwargs)
{
  variable_counter = 0;
#ifdef VERBOSE_COMPILATION
  docs.compilation_ss << "Performing parsing, kernel generation, and memory allocation" << std::endl;
#endif

  /* get the number of arguments */
  Py_ssize_t argc = PyTuple_Size(args);
  if(argc != 4)
  {
    std::cout << "Not the right number of arguments" << std::endl;
    Py_INCREF(Py_None);
    return Py_None;
  }

  /* Get XML Strings and parse them */
  /* Symbol table */
  PyObject * idobj = PyTuple_GetItem(args, 0);
  int record_id;
  if(PyInt_Check(idobj))
  {
    record_id = (int) PyInt_AsLong(idobj);
#ifdef VERBOSE_COMPILATION
    docs.compilation_ss << "Record id: " << record_id << std::endl;
#endif
  }
  else
  {
	EXIT_MSG("Parsing ID failed");
  }


  PyObject *stobj = PyTuple_GetItem(args, 1); 
  if(PyString_Check(stobj))
  {
	smparse(PyString_AsString(stobj), (Node**)&symbolTable);
  }
  else
  {
	EXIT_MSG("Parsing symbol table failed");
  }

  /* Hindemith AST */
  PyObject *smobj = PyTuple_GetItem(args, 2); 
  if(PyString_Check(smobj))
  {
	smparse(PyString_AsString(smobj), (Node**)&semanticModel);
  }
  else
  {
	EXIT_MSG("Parsing semantic model failed");
  }

  /* Constants */
  PyObject *cstobj = PyTuple_GetItem(args, 3); 
  if(PyString_Check(smobj))
  {
	smparse(PyString_AsString(cstobj), (Node**)&constantTable);
  }
  else
  {
	EXIT_MSG("Parsing constant table failed");
  }


#ifdef VERBOSE_COMPILATION
  docs.compilation_ss << "Completed XML Parsing" << std::endl;
  print_info(semanticModel);
  print_info(symbolTable);
  print_info(constantTable);
#endif

  build_symbol_table(symbolTable, sym_tab);
  build_constant_table(constantTable, cst_tab);
  allocate_memory(sym_tab, clv);
  generate_ocl(semanticModel, sym_tab, cst_tab, clv, &executionModel);

  // Save this program record
  symbolTables[record_id] = symbolTable;
  constantTables[record_id] = constantTable;
  semanticModels[record_id] = semanticModel;
  executionModels[record_id] = executionModel;
  sym_tabs[record_id] = sym_tab;
  cst_tabs[record_id] = cst_tab;
  Py_INCREF(Py_None);
  return Py_None;

}

PyObject * createPyObject(Variable * variable)
{
  Array * arr = dynamic_cast<Array *>(variable);
  Array2D * arr2d = dynamic_cast<Array2D *>(variable);
  Array3D * arr3d = dynamic_cast<Array3D *>(variable);
  Scalar * scalar= dynamic_cast<Scalar *>(variable);

  if(arr)
  {
    npy_intp npyLen[MAX_RETURN_DIMS] = {0};
    int ndims = 0;
    if(arr->get_property_int("dim1") > 0)
    {
      ndims = 2;
      npyLen[0] = arr->get_property_int("dim0");
      npyLen[1] = arr->get_property_int("dim1");
    }
    else
    {
      ndims = 1;
      npyLen[0] = arr->get_property_int("dim0");
    }
    arr->lives_in_python = true;
    if(!strcmp(arr->properties["dtype"].c_str(), "float64"))
    {
      PyObject * retArray = PyArray_New(&PyArray_Type, ndims, npyLen, NPY_FLOAT64, NULL, arr->cpu_data, 0, NPY_C_CONTIGUOUS, NULL);
      return retArray;
    }
    if(!strcmp(arr->properties["dtype"].c_str(), "float32"))
    {
      PyObject * retArray = PyArray_New(&PyArray_Type, ndims, npyLen, NPY_FLOAT32, NULL, arr->cpu_data, 0, NPY_C_CONTIGUOUS, NULL);
      return retArray;
    }
    if(!strcmp(arr->properties["dtype"].c_str(), "int64"))
    {
      PyObject * retArray = PyArray_New(&PyArray_Type, ndims, npyLen, NPY_INT64, NULL, arr->cpu_data, 0, NPY_C_CONTIGUOUS, NULL);
      return retArray;
    }
    if(!strcmp(arr->properties["dtype"].c_str(), "bool"))
    {
      PyObject * retArray = PyArray_New(&PyArray_Type, ndims, npyLen, NPY_BOOL, NULL, arr->cpu_data, 0, NPY_C_CONTIGUOUS, NULL);
      return retArray;
    }
    if(!strcmp(arr->properties["dtype"].c_str(), "complex64"))
    {
      PyObject * retArray = PyArray_New(&PyArray_Type, ndims, npyLen, NPY_COMPLEX64, NULL, arr->cpu_data, 0, NPY_C_CONTIGUOUS, NULL);
      return retArray;
    }
  }
  if(arr2d)
  {
    npy_intp npyLen[MAX_RETURN_DIMS] = {0};
    int ndims = 2;
    npyLen[0] = arr2d->get_property_int("dim0");
    npyLen[1] = arr2d->get_property_int("dim1");
    arr2d->lives_in_python = true;
    int flags = (arr2d->properties["col_mjr"] == "True") ? NPY_F_CONTIGUOUS : NPY_C_CONTIGUOUS;
    if(!strcmp(arr2d->properties["dtype"].c_str(), "float64"))
    {
      PyObject * retArray = PyArray_New(&PyArray_Type, ndims, npyLen, NPY_FLOAT64, NULL, arr2d->cpu_data, 0, flags, NULL);
      return retArray;
    }
    if(!strcmp(arr2d->properties["dtype"].c_str(), "float32"))
    {
      PyObject * retArray = PyArray_New(&PyArray_Type, ndims, npyLen, NPY_FLOAT32, NULL, arr2d->cpu_data, 0, flags, NULL);
      return retArray;
    }
    if(!strcmp(arr2d->properties["dtype"].c_str(), "bool"))
    {
      PyObject * retArray = PyArray_New(&PyArray_Type, ndims, npyLen, NPY_BOOL, NULL, arr2d->cpu_data, 0, flags, NULL);
      return retArray;
    }
    if(!strcmp(arr2d->properties["dtype"].c_str(), "int64"))
    {
      PyObject * retArray = PyArray_New(&PyArray_Type, ndims, npyLen, NPY_INT64, NULL, arr2d->cpu_data, 0, flags, NULL);
      return retArray;
    }
    if(!strcmp(arr2d->properties["dtype"].c_str(), "complex64"))
    {
      PyObject * retArray = PyArray_New(&PyArray_Type, ndims, npyLen, NPY_COMPLEX64, NULL, arr2d->cpu_data, 0, flags, NULL);
      return retArray;
    }
  }
  if(arr3d)
  {
    npy_intp npyLen[MAX_RETURN_DIMS] = {0};
    int ndims = 3;
    npyLen[0] = arr3d->get_property_int("dim0");
    npyLen[1] = arr3d->get_property_int("dim1");
    npyLen[2] = arr3d->get_property_int("dim2");
    arr3d->lives_in_python = true;
    int flags = (arr3d->properties["col_mjr"] == "True") ? NPY_F_CONTIGUOUS : NPY_C_CONTIGUOUS;
    if(!strcmp(arr3d->properties["dtype"].c_str(), "float64"))
    {
      PyObject * retArray = PyArray_New(&PyArray_Type, ndims, npyLen, NPY_FLOAT64, NULL, arr3d->cpu_data, 0, flags, NULL);
      return retArray;
    }
    if(!strcmp(arr3d->properties["dtype"].c_str(), "float32"))
    {
      PyObject * retArray = PyArray_New(&PyArray_Type, ndims, npyLen, NPY_FLOAT32, NULL, arr3d->cpu_data, 0, flags, NULL);
      return retArray;
    }
    if(!strcmp(arr3d->properties["dtype"].c_str(), "bool"))
    {
      PyObject * retArray = PyArray_New(&PyArray_Type, ndims, npyLen, NPY_BOOL, NULL, arr3d->cpu_data, 0, flags, NULL);
      return retArray;
    }
    if(!strcmp(arr3d->properties["dtype"].c_str(), "int64"))
    {
      PyObject * retArray = PyArray_New(&PyArray_Type, ndims, npyLen, NPY_INT64, NULL, arr3d->cpu_data, 0, flags, NULL);
      return retArray;
    }
    if(!strcmp(arr3d->properties["dtype"].c_str(), "complex64"))
    {
      PyObject * retArray = PyArray_New(&PyArray_Type, ndims, npyLen, NPY_COMPLEX64, NULL, arr3d->cpu_data, 0, flags, NULL);
      return retArray;
    }
  }

  if(scalar)
  {
    if(!strcmp(scalar->properties["dtype"].c_str(), "float64"))
    {
      double * pd = (double*) scalar->cpu_data;
      PyObject * retNum = PyFloat_FromDouble(pd[0]);
      return retNum;
    }
    if(!strcmp(scalar->properties["dtype"].c_str(), "float32"))
    {
      float * pd = (float*) scalar->cpu_data;
      PyObject * retNum = PyFloat_FromDouble((double)pd[0]);
      return retNum;
    }
    if(!strcmp(scalar->properties["dtype"].c_str(), "int64"))
    {
      long * pd = (long*) scalar->cpu_data;
      PyObject * retNum = PyLong_FromLong(pd[0]);
      return retNum;
    }
    if(!strcmp(scalar->properties["dtype"].c_str(), "bool"))
    {
      char * pd = (char*) scalar->cpu_data;
      PyObject * retNum = PyBool_FromLong((long)pd[0]);
      return retNum;
    }
  }
  else
  { 
  //  std::cout << "Bad return type: " << std::endl;
  //  assert(0);
  }
  return Py_None;
}

static char run_doc[] = 
"run(a, b)\n\
\n\
Executes a DAG of Add, Subtract, and SPMV operations.";

static PyObject*
run(PyObject *self, PyObject *args, PyObject *kwargs)
{

#ifdef PROFILE_TOP
  double backend_ts = timestamp();
#endif

#ifdef VERBOSE_EXECUTION
  docs.execution_ss << "Running. Copy memory, run kernels, copy memory back." << std::endl;
#endif

  /* get the number of arguments */
  Py_ssize_t argc = PyTuple_Size(args);
  if(argc != 2)
  {
    std::cout << "Not the right number of arguments" << std::endl;
    Py_INCREF(Py_None);
    return Py_None;
  }

  PyObject * idobj = PyTuple_GetItem(args, 0);
  int record_id;
  if(PyInt_Check(idobj))
  {
    record_id = (int) PyInt_AsLong(idobj);
#ifdef VERBOSE_EXECUTION
    docs.execution_ss << "Record id: " << record_id << std::endl;
#endif
  }
  else
  {
	EXIT_MSG("Parsing ID failed");
  }

  assert(symbolTables.count(record_id) > 0);
  assert(constantTables.count(record_id) > 0);
  assert(semanticModels.count(record_id) > 0);
  assert(executionModels.count(record_id) > 0);
  assert(sym_tabs.count(record_id) > 0);

  symbolTable = symbolTables[record_id];
  constantTable = constantTables[record_id];
  semanticModel = semanticModels[record_id];
  executionModel = executionModels[record_id];
  sym_tab = sym_tabs[record_id];

#ifdef PROFILE
  transfer_events.clear();
#endif

  /* get the first argument (kwargs) */
  PyObject *dictobj = PyTuple_GetItem(args, 1); 
  if(PyDict_Check(dictobj))
  {
#ifdef VERBOSE_EXECUTION
  	docs.execution_ss << "Found the dict" << std::endl;
#endif
  }
  else
  {
	EXIT_MSG("Parsing failed");
  }

  // For each variable copy
  for(std::map<std::string, Variable*>::iterator it = sym_tab.begin() ; it != sym_tab.end() ; it++)
  {
    Variable * variable = it->second;
    if(strcmp(variable->properties["source"].c_str(), "Header"))
    {
      continue;
    }

#ifdef VERBOSE_EXECUTION
    int does_contain;
    does_contain = PyDict_Contains(dictobj, PyString_FromString((it->first).c_str()));
    docs.execution_ss << "Does contain: " << does_contain << std::endl;
    docs.execution_ss << "Getting " << variable->properties["name"] << std::endl;
#endif

    PyObject * obj = PyDict_GetItemString(dictobj, (variable->properties["name"]).c_str());

    // Found variable, now copy it into the C/OCL data structure
    if(obj)
    {
      Array * arr = dynamic_cast<Array *> (variable);
      Array2D * arr2d = dynamic_cast<Array2D *> (variable);
      Array3D * arr3d = dynamic_cast<Array3D *> (variable);
      Stencil * stencil = dynamic_cast<Stencil *> (variable);
      SpMat * spmat = dynamic_cast<SpMat *> (variable);
      SpMat2D * spmat2d = dynamic_cast<SpMat2D *> (variable);
      Scalar * scalar = dynamic_cast<Scalar *> (variable);
      CSRMat * csrmat = dynamic_cast<CSRMat *> (variable);
      if(arr)
      {
        PyArrayObject * pyArray = (PyArrayObject*)obj;
        void * pydata = (void *)pyArray->data;
        if(!pyArray)
        {
          std::cout << "Variable data not found: " << variable->properties["name"] << std::endl;
        }
	arr->cpu_data = (char*)pydata;
      }
      if(arr2d)
      {
        PyArrayObject * pyArray = (PyArrayObject*)obj;
        void * pydata = (void *)pyArray->data;
        if(!pyArray)
        {
          std::cout << "Variable data not found: " << variable->properties["name"] << std::endl;
        }
	arr2d->cpu_data = (char*)pydata;
      }
      if(arr3d)
      {
        PyArrayObject * pyArray = (PyArrayObject*)obj;
        void * pydata = (void *)pyArray->data;
        if(!pyArray)
        {
          std::cout << "Variable data not found: " << variable->properties["name"] << std::endl;
        }
	arr3d->cpu_data = (char*)pydata;
      }
      if(scalar)
      {
	if(scalar->properties["dtype"] == "int64")
	{
          PyLongObject * pyLong = (PyLongObject*)obj;
	  long val = (long)PyLong_AsLong((PyObject*) pyLong);
          memcpy(scalar->cpu_data, &val, sizeof(long));
	}
	else if(scalar->properties["dtype"] == "float32")
  	{
          PyFloatObject * pyFloat = (PyFloatObject*)obj;
	  float val = (float)PyFloat_AsDouble((PyObject*)pyFloat);
          memcpy(scalar->cpu_data, &val, sizeof(float));
	}
	else if(scalar->properties["dtype"] == "float64")
	{
          PyFloatObject * pyFloat = (PyFloatObject*)obj;
	  double val = (double)PyFloat_AsDouble((PyObject*) pyFloat);
          memcpy(scalar->cpu_data, &val, sizeof(double));
	}
	else if(scalar->properties["dtype"] == "complex64")
	{
	  assert(0); //Not supported yet
	}
      }
      if(stencil)
      {
        assert(PyObject_HasAttrString(obj, "data"));
        assert(PyObject_HasAttrString(obj, "offx"));
        assert(PyObject_HasAttrString(obj, "offy"));
        PyArrayObject * pyArrayData = (PyArrayObject*)PyObject_GetAttrString(obj, "data");
        PyArrayObject * pyArrayOffx = (PyArrayObject*)PyObject_GetAttrString(obj, "offx");
        PyArrayObject * pyArrayOffy = (PyArrayObject*)PyObject_GetAttrString(obj, "offy");
        if(!pyArrayData || !pyArrayOffx || !pyArrayOffy)
        {
          std::cout << "Variable data not found: " << variable->properties["name"] << std::endl;
        }
        void * pydata = (void *)pyArrayData->data;
        void * pyoffx = (void *)pyArrayOffx->data;
        void * pyoffy = (void *)pyArrayOffy->data;
	stencil->cpu_data = (char*)pydata;
	stencil->cpu_offx = (char*)pyoffx;
	stencil->cpu_offy = (char*)pyoffy;
      }
      if(spmat)
      {
        assert(PyObject_HasAttrString(obj, "data"));
        assert(PyObject_HasAttrString(obj, "offsets"));
        PyArrayObject * pyArrayData = (PyArrayObject*)PyObject_GetAttrString(obj, "data");
        PyArrayObject * pyArrayOffsets = (PyArrayObject*)PyObject_GetAttrString(obj, "offsets");
        if(!pyArrayData || !pyArrayOffsets)
        {
          std::cout << "Variable data not found: " << variable->properties["name"] << std::endl;
        }
        void * pydata = (void *)pyArrayData->data;
        void * pyoffsets = (void *)pyArrayOffsets->data;
	spmat->cpu_data = (char*)pydata;
	spmat->cpu_offsets = (char*)pyoffsets;
      }
      if(spmat2d)
      {
        assert(PyObject_HasAttrString(obj, "data"));
        assert(PyObject_HasAttrString(obj, "offx"));
        assert(PyObject_HasAttrString(obj, "offy"));
        PyArrayObject * pyArrayData = (PyArrayObject*)PyObject_GetAttrString(obj, "data");
        PyArrayObject * pyArrayOffx = (PyArrayObject*)PyObject_GetAttrString(obj, "offx");
        PyArrayObject * pyArrayOffy = (PyArrayObject*)PyObject_GetAttrString(obj, "offy");
        if(!pyArrayData || !pyArrayOffx || !pyArrayOffy)
        {
          std::cout << "Variable data not found: " << variable->properties["name"] << std::endl;
        }
        void * pydata = (void *)pyArrayData->data;
        void * pyoffx = (void *)pyArrayOffx->data;
        void * pyoffy = (void *)pyArrayOffy->data;
	spmat2d->cpu_data = (char*)pydata;
	spmat2d->cpu_offx = (char*)pyoffx;
	spmat2d->cpu_offy = (char*)pyoffy;
      }
      if(csrmat)
      {
        assert(PyObject_HasAttrString(obj, "data"));
        assert(PyObject_HasAttrString(obj, "indices"));
        assert(PyObject_HasAttrString(obj, "indptr"));
        PyArrayObject * pyArrayData = (PyArrayObject*)PyObject_GetAttrString(obj, "data");
        PyArrayObject * pyArrayIndices = (PyArrayObject*)PyObject_GetAttrString(obj, "indices");
        PyArrayObject * pyArrayIndptr= (PyArrayObject*)PyObject_GetAttrString(obj, "indptr");
        void * pydata = (void *)pyArrayData->data;
        void * pyindices = (void *)pyArrayIndices->data;
        void * pyindptr = (void *)pyArrayIndptr->data;
        if(!pyArrayData || !pyArrayIndices || !pyArrayIndptr)
        {
          std::cout << "Variable data not found: " << variable->properties["name"] << std::endl;
        }
	csrmat->cpu_data = pydata;
	csrmat->cpu_indices = pyindices;
	csrmat->cpu_indptr = pyindptr;
      }
    }
  }

#ifdef VERBOSE_EXECUTION
  //print_vars(symbolTable, clv);
#endif

  // Run kernels
  for(size_t devId = 0 ; devId < clv.num_devices ; devId++)
  {
    clFinish(clv.commands[devId]);
  }
  std::vector<EParam*> retval;
  enqueue_ocl_kernels(executionModel, clv, sym_tab, retval);
  for(size_t devId = 0 ; devId < clv.num_devices ; devId++)
  {
    clFinish(clv.commands[devId]);
  }
#ifdef VERBOSE_EXECUTION
  //print_vars(symbolTable, clv);
#endif


#ifdef PROFILE_TOP
  double backend_elapsed = timestamp() - backend_ts;
#endif

#ifdef PROFILE_TOP
  docs.runtime_ss << backend_elapsed << std::endl;
  //std::cout << backend_elapsed << std::endl;
#endif

#ifdef PROFILE
  doProfileTransfers();
  transfer_events.clear();
#endif

  docs.write_files();

  if(retval.size() == 0)
  {
    std::cout << "No return node" << std::endl;
    Py_INCREF(Py_None);
    return Py_None;
  }

  // Create tuple
  PyObject * returnTuple = (PyObject*) PyTuple_New(retval.size());

  // Return rn
  for(size_t i = 0 ; i < retval.size() ; i++)
  {
    PyObject * returnObj = createPyObject(retval[i]->variable);
    PyTuple_SetItem(returnTuple, i, returnObj);
  }

  // Create debug dictionary
  PyObject * returnObj = NULL;
  if(retval.size() == 1)
  {
    returnObj = PyTuple_GetItem(returnTuple, 0);
  }
  else if(retval.size() > 1)
  {
    returnObj = returnTuple;
  }
  else if(retval.size() == 0)
  {
    Py_INCREF(Py_None);
    returnObj = Py_None;
  }

  // Return retObj and debug dictionary
  PyObject * debugDict = (PyObject*) PyDict_New(); 
  /*
  for(std::map<std::string, Variable*>::iterator it = sym_tab.begin() ; it != sym_tab.end() ; it++)
  {
    Variable * variable = it->second;

    // Create object
    PyObject * po = createPyObject(variable);

    // Add object to dictionary
    PyDict_SetItemString(debugDict, (it->first).c_str(), po);
  }
  */

#ifdef PROFILE
  doProfileTransfers();
#endif

  PyObject * jointTuple = (PyObject*) PyTuple_New(2);
  PyTuple_SetItem(jointTuple, 0, returnObj);
  PyTuple_SetItem(jointTuple, 1, debugDict);

  Py_INCREF(jointTuple);
  return jointTuple;

}


static char free_ocl_doc[] = 
"free_ocl(a, b)\n\
\n\
Frees OpenCL memory buffers.";

static PyObject*
free_ocl(PyObject *self, PyObject *args, PyObject *kwargs)
{
  Py_INCREF(Py_None);
  return Py_None;
}


static char backend_doc[] = 
"xmlcompile(a, b)\n\
\n\
Executes a DAG of Add, Subtract, and SPMV operations.";

static PyMethodDef backend_methods[] = {
  {"xmlcompile", (PyCFunction)xmlcompile, METH_VARARGS | METH_KEYWORDS,
	 xmlcompile_doc},
  {"run", (PyCFunction)run, METH_VARARGS | METH_KEYWORDS,
	 run_doc},
  {"free_ocl", (PyCFunction)free_ocl, METH_VARARGS | METH_KEYWORDS,
	 free_ocl_doc},
	{NULL, NULL}
};

PyMODINIT_FUNC
initbackend(void)
{
  _import_array();
  Py_InitModule3("backend", backend_methods, backend_doc);
  initialize_ocl(clv);
}

