__author__ = 'Michael J. Anderson'

from numpy import *
import scipy.sparse
from hindemith.hindemith import *

import cg_solver

cg_solver.setup_hindemith()

dim0 = 1023
dim1 = 1023

d0 = array([0.1] * dim1 * dim0, dtype=float32)
d1 = array([0.4] * dim1 * dim0, dtype=float32)
d2 = array([1.0] * dim1 * dim0, dtype=float32)
d3 = array([0.4] * dim1 * dim0, dtype=float32)
d4 = array([0.1] * dim1 * dim0, dtype=float32)

d0.shape = (dim1,dim0)
d1.shape = (dim1,dim0)
d2.shape = (dim1,dim0)
d3.shape = (dim1,dim0)
d4.shape = (dim1,dim0)

d0[0,:] = 0.0
d1[:,0] = 0.0
d3[:,dim0-1] = 0.0
d4[dim1-1,:] = 0.0

d0.shape = (dim1*dim0,)
d1.shape = (dim1*dim0,)
d2.shape = (dim1*dim0,)
d3.shape = (dim1*dim0,)
d4.shape = (dim1*dim0,)
data = [d4, d3, d2, d1, d0]

A = scipy.sparse.spdiags(data,[-dim0,-1,0,1,dim0],dim1*dim0,dim1*dim0)

b = 0.5*ones((dim1*dim0,), order='F', dtype=float32)
random.seed(5)
b[:] = random.rand(dim1*dim0)
x = 0.5 * ones((dim1*dim0,), order='F', dtype=float32)

residual_before = sqrt(dot(b - A * x, b - A * x))
z = cg_solver.cg_solver(A=A,b=b,x=x, sc=float32(2.0))
residual_after = sqrt(dot(b - A * z, b - A * z))

if (residual_after / residual_before) < 0.02:
  #print 'PASSED: ' + str(residual_before)  + ', ' + str(residual_after)
  print 'PASSED'
else:
  #print 'FAILED: ' + str(residual_before)  + ', ' + str(residual_after)
  print 'FAILED'
