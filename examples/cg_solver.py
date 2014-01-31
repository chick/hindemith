# CG solver using s-step specializer

from numpy import *
import scipy.sparse

from hindemith import *

def setup_hindemith():
  defines['nd1'] = 1
  defines['nt1'] = 128
  defines['ne1'] = 2

@hm
def cg_solver(A, b, x, sc):
  r = b - A*x
  p = b - A*x
  rsold = sum(r * r)
#  residual = b - A*x
#  rnorm = sum(residual*residual)
#  while(rnorm > 20.0):
  for i in range(400):
    Ap = A * p
    alpha = rsold / sum(p * Ap)
    x = x + alpha * p
    r = r - alpha * Ap
    rsnew = sum(r * r)
    beta = rsnew / rsold
    p = r + beta * p
    rsold = rsnew
  return x
