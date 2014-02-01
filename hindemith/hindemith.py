"""
Hindemith specializer entry point
Uses the @hm function annotation to
parse, analyze and run python code
using openCL
"""

__author__ = "Michael J. Anderson"

from hlib import *
from asp.jit.asp_module import ASPModule
import inspect

# h compiler
import PyAST 

state = {}
defines = {}


class HindemithSpecializer:
    def __init__(self, fn):
        self.fn = fn
        self.source = inspect.getsource(fn)
        self.specializer = Hindemith()

    def __call__(self, **kwargs):
        global state
        global defines
        (record_id, new_kwargs) = PyAST.mycompile(self.source, kwargs, defines)
        (return_value, state) = PyAST.run_backend(record_id, new_kwargs)
        return return_value


def hm(fn):
    """
    creates the hm annotation which is hook by which this specializer
    is started, usage
    @hm
    def function_to_be_specialized()

    """
    return HindemithSpecializer(fn)


class Hindemith(object):
    """
    Singleton entry point to ASP
    """
    asp_module = ASPModule("hindemith")
    asp_module.validate()
