# Python matrix/vector data and routines
from numpy import *
from hlib import *
import scipy.sparse

# AST manipulation
import ast
import inspect
import copy

# h compiler
import PyAST 

state = {}
defines = {}

class hm_fn:
  def __init__(self, fn):
    self.fn = fn
    self.source = inspect.getsource(fn)

  def __call__(self, *args, **kwargs):
    global state
    global defines
    (record_id, new_kwargs) = PyAST.mycompile(self.source, kwargs, defines)
    (retval, state) = PyAST.run_backend(record_id, new_kwargs)
    return retval

def hm(fn):
  return hm_fn(fn)
