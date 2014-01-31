import PyAST
import copy

nameCnt = 0
def uniqueName():
  global nameCnt
  name = '_f' + str(nameCnt)
  nameCnt = nameCnt + 1
  return name

# Generic Node
class Node(object): 
  def __init__(self, name=''):
    self.children = []
    self.properties = {}
    if name != '':
      self.properties['name'] = name

  def __repr__(self):
    return self.__str__()

  def __str__(self):
    r = str(self.__class__.__name__) + str(self.properties)
    for child in self.children:
      r += ', ' + str(child) 
    return r

class ScalarConstant(Node):
  pass

class SemanticModel(Node):
  pass

class PipeAndFilter(Node):
  pass

class Filter(Node):
  pass

class FunctionDef(Node):
  @staticmethod
  def match(astNode, semanticModel, symbolTable, astVariables, astNodes, defines):
    classname = astNode.__class__.__name__
    if(classname == 'FunctionDef'):
      f = FunctionDef()
      f.properties['name'] = astNode.name
      b = PipeAndFilter()
      for child in astNode.body:
	b.children += astNodes[id(child)]
      f.children.append(b)
      astNodes[id(astNode)] = [f]
      semanticModel.children.append(f)
      return True
    else:
      return False

class Iterator(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines):
    classname = astNode.__class__.__name__
    if(classname == 'For'):
      f = Iterator()
      f.properties['ub'] = astNode.iter.args[0].n
      b = PipeAndFilter()
      for child in astNode.body:
	b.children += astNodes[id(child)]
      f.children.append(b)
      astNodes[id(astNode)] = [f]
      return True
    else:
      return False

class Num(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines): 
    classname = astNode.__class__.__name__
    if(classname == 'Num'):
      astNodes[id(astNode)] = []
      astVariables[id(astNode)] = Scalar.CreateImmediate(astNode.n)
      return True
    else:
      return False

class ArrayOp(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines):
    classname = astNode.__class__.__name__
    if(classname == 'BinOp'):
      leftName = astVariables[id(astNode.left)]
      rightName = astVariables[id(astNode.right)]
      if(isArray(symbolTable, leftName) and isArray(symbolTable, rightName)):
	m = Filter('ArrayOp')
        m.properties['op'] = astNode.op.__class__.__name__
        newvar = Array.CreateCopy(symbolTable, uniqueName(), leftName)
        m.children.append(newvar)
	m.children.append(leftName)
	m.children.append(rightName)
        astVariables[id(astNode)] = newvar
        astNodes[id(astNode)] = astNodes[id(astNode.left)] + astNodes[id(astNode.right)] + [m]
        return True
    return False

class Array2DOp(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines):
    classname = astNode.__class__.__name__
    if(classname == 'BinOp'):
      leftName = astVariables[id(astNode.left)]
      rightName = astVariables[id(astNode.right)]
      if(isArray2D(symbolTable, leftName) and isArray2D(symbolTable, rightName)):
	m = Filter('Array2DOp')
        m.properties['op'] = astNode.op.__class__.__name__
        newvar = Array2D.CreateCopy(symbolTable, uniqueName(), leftName)
        m.children.append(newvar)
	m.children.append(leftName)
	m.children.append(rightName)
        astVariables[id(astNode)] = newvar
        astNodes[id(astNode)] = astNodes[id(astNode.left)] + astNodes[id(astNode.right)] + [m]
        return True
    return False

class Array3DOp(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines):
    classname = astNode.__class__.__name__
    if(classname == 'BinOp'):
      leftName = astVariables[id(astNode.left)]
      rightName = astVariables[id(astNode.right)]
      if(isArray3D(symbolTable, leftName) and isArray3D(symbolTable, rightName)):
	m = Filter('Array3DOp')
        m.properties['op'] = astNode.op.__class__.__name__
        newvar = Array3D.CreateCopy(symbolTable, uniqueName(), leftName)
        m.children.append(newvar)
	m.children.append(leftName)
	m.children.append(rightName)
        astVariables[id(astNode)] = newvar
        astNodes[id(astNode)] = astNodes[id(astNode.left)] + astNodes[id(astNode.right)] + [m]
        return True
    return False

class Array2DScalarOp(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines):
    classname = astNode.__class__.__name__
    if(classname == 'BinOp'):
      leftName = astVariables[id(astNode.left)]
      rightName = astVariables[id(astNode.right)]
      if isArray2D(symbolTable, leftName) and isScalar(symbolTable, rightName):
        m = Filter('Array2DScalarOp')
        m.properties['op'] = astNode.op.__class__.__name__
        newvar = Array2D.CreateCopy(symbolTable, uniqueName(), leftName)
        m.children.append(newvar)
        m.children.append(leftName)
        m.children.append(rightName)
        astVariables[id(astNode)] = newvar
        astNodes[id(astNode)] = astNodes[id(astNode.left)] + astNodes[id(astNode.right)] + [m]
        return True
    return False

class ArrayScalarOp(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines):
    classname = astNode.__class__.__name__
    if(classname == 'BinOp'):
      leftName = astVariables[id(astNode.left)]
      rightName = astVariables[id(astNode.right)]
      if isArray(symbolTable, leftName) and isScalar(symbolTable, rightName):
	m = Filter('ArrayScalarOp')
        m.properties['op'] = astNode.op.__class__.__name__
        newvar = Array.CreateCopy(symbolTable, uniqueName(), leftName)
        m.children.append(newvar)
        m.children.append(leftName)
        m.children.append(rightName)
        astVariables[id(astNode)] = newvar
        astNodes[id(astNode)] = astNodes[id(astNode.left)] + astNodes[id(astNode.right)] + [m]
        return True
    return False

class ScalarArray2DOp(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines):
    classname = astNode.__class__.__name__
    if(classname == 'BinOp'):
      leftName = astVariables[id(astNode.left)]
      rightName = astVariables[id(astNode.right)]
      if isScalar(symbolTable, leftName) and isArray2D(symbolTable, rightName):
	m = Filter('ScalarArray2DOp')
        m.properties['op'] = astNode.op.__class__.__name__
        newvar = Array2D.CreateCopy(symbolTable, uniqueName(), rightName)
        m.children.append(newvar)
        m.children.append(leftName)
        m.children.append(rightName)
        astVariables[id(astNode)] = newvar
        astNodes[id(astNode)] = astNodes[id(astNode.left)] + astNodes[id(astNode.right)] + [m]
        return True
    return False

class ScalarArray3DOp(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines):
    classname = astNode.__class__.__name__
    if(classname == 'BinOp'):
      leftName = astVariables[id(astNode.left)]
      rightName = astVariables[id(astNode.right)]
      if isScalar(symbolTable, leftName) and isArray3D(symbolTable, rightName):
	m = Filter('ScalarArray3DOp')
        m.properties['op'] = astNode.op.__class__.__name__
        newvar = Array3D.CreateCopy(symbolTable, uniqueName(), rightName)
        m.children.append(newvar)
        m.children.append(leftName)
        m.children.append(rightName)
        astVariables[id(astNode)] = newvar
        astNodes[id(astNode)] = astNodes[id(astNode.left)] + astNodes[id(astNode.right)] + [m]
        return True
    return False



class ScalarArrayOp(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines):
    classname = astNode.__class__.__name__
    if(classname == 'BinOp'):
      leftName = astVariables[id(astNode.left)]
      rightName = astVariables[id(astNode.right)]
      if isScalar(symbolTable, leftName) and isArray(symbolTable, rightName):
	m = Filter('ScalarArrayOp')
        m.properties['op'] = astNode.op.__class__.__name__
        newvar = Array.CreateCopy(symbolTable, uniqueName(), rightName)
        m.children.append(newvar)
        m.children.append(leftName)
        m.children.append(rightName)
        astVariables[id(astNode)] = newvar
        astNodes[id(astNode)] = astNodes[id(astNode.left)] + astNodes[id(astNode.right)] + [m]
        return True
    return False

class ScalarOp(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines):
    classname = astNode.__class__.__name__
    if(classname == 'BinOp'):
      leftName = astVariables[id(astNode.left)]
      rightName = astVariables[id(astNode.right)]
      if isScalar(symbolTable, leftName) and isScalar(symbolTable, rightName):
	m = Filter('ScalarOp')
        m.properties['op'] = astNode.op.__class__.__name__
        newvar = Scalar.CreateCopy(symbolTable, uniqueName(), leftName)
        m.children.append(newvar)
        m.children.append(leftName)
        m.children.append(rightName)
        astVariables[id(astNode)] = newvar
        astNodes[id(astNode)] = astNodes[id(astNode.left)] + astNodes[id(astNode.right)] + [m]
        return True
    return False

class WarpImg2D(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines):
    classname = astNode.__class__.__name__
    if(classname == 'Call'):
      if(astNode.func.id == 'warp_img2d'):
        srcName = astVariables[id(astNode.args[0])]
        offset0Name = astVariables[id(astNode.args[1])]
        offset1Name = astVariables[id(astNode.args[2])]
	m = Filter('WarpImg2D')
        newVar = Array2D.CreateCopy(symbolTable, uniqueName(), srcName)
        m.children.append(newVar)
        m.children.append(srcName)
        m.children.append(offset0Name)
        m.children.append(offset1Name)
        astVariables[id(astNode)] = newVar
        astNodes[id(astNode)] = astNodes[id(astNode.args[0])] + astNodes[id(astNode.args[1])] + astNodes[id(astNode.args[2])] + [m]
        return True
    return False

class HSMatrixPowers(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines):
    classname = astNode.__class__.__name__
    if(classname == 'Call'):
      if(astNode.func.id == 'hs_matrix_powers'):
        lamName = astVariables[id(astNode.args[0])]
        IxName = astVariables[id(astNode.args[1])]
        IyName = astVariables[id(astNode.args[2])]
        r0Name = astVariables[id(astNode.args[3])]
        r1Name = astVariables[id(astNode.args[4])]
	m = Filter('HSMatrixPowers')
	sym0 = symbolTable[r0Name.properties['name']]
	shape = (defines['STEPS'], sym0.properties['dim0'], sym0.properties['dim1'])
	newVar0 = Array3D.Create(symbolTable, uniqueName(), sym0.properties['dtype'], shape, 'Inferred', False)
	newVar1 = Array3D.Create(symbolTable, uniqueName(), sym0.properties['dtype'], shape, 'Inferred', False)
        m.children.append(newVar0)
        m.children.append(newVar1)
        m.children.append(lamName)
        m.children.append(IxName)
        m.children.append(IyName)
        m.children.append(r0Name)
        m.children.append(r1Name)
        astVariables[id(astNode)] = [newVar0, newVar1]
        astNodes[id(astNode)] = astNodes[id(astNode.args[0])] + \
	                        astNodes[id(astNode.args[1])] + \
				astNodes[id(astNode.args[2])] + \
				astNodes[id(astNode.args[3])] + \
				astNodes[id(astNode.args[4])] + [m]
        return True
    return False



class SetBroxMatrix(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines):
    classname = astNode.__class__.__name__
    if(classname == 'Expr'):
      astNode2 = astNode.value
      classname2 = astNode2.__class__.__name__
      if(classname2 == 'Call'):
        if(astNode2.func.id == 'set_brox_matrix'):
          matName = astVariables[id(astNode2.args[0])]
          srcName = astVariables[id(astNode2.args[1])]
          alphaName = astVariables[id(astNode2.args[2])]
	  m = Filter('SetBroxMatrix')
          m.children.append(matName)
          m.children.append(srcName)
          m.children.append(alphaName)
          astVariables[id(astNode)] = None
          astNodes[id(astNode)] = astNodes[id(astNode2.args[0])] + astNodes[id(astNode2.args[1])] + astNodes[id(astNode2.args[2])] + [m]
          return True
    return False

class SetBroxRedblackMatrix(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines):
    classname = astNode.__class__.__name__
    if(classname == 'Call'):
      if(astNode.func.id == 'set_brox_redblack_matrix'):
        matName = astVariables[id(astNode.args[0])]
        srcName = astVariables[id(astNode.args[1])]
        alphaName = astVariables[id(astNode.args[2])]
        newVar0 = Array2D.CreateCopy(symbolTable, uniqueName(), srcName )
	m = Filter('SetBroxRedblackMatrix')
        m.children.append(newVar0)
        m.children.append(matName)
        m.children.append(srcName)
        m.children.append(alphaName)
        astVariables[id(astNode)] = newVar0
        astNodes[id(astNode)] = astNodes[id(astNode.args[0])] + astNodes[id(astNode.args[1])] + astNodes[id(astNode.args[2])] + [m]
        return True
    return False

class LKLeastSquares(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines):
    classname = astNode.__class__.__name__
    if(classname == 'Call'):
      if(astNode.func.id == 'lk_least_squares'):
        v0 = astVariables[id(astNode.args[0])]
        v1 = astVariables[id(astNode.args[1])]
        v2 = astVariables[id(astNode.args[2])]
        newVar0 = Array2D.CreateCopy(symbolTable, uniqueName(), v1 )
        newVar1 = Array2D.CreateCopy(symbolTable, uniqueName(), v2 )
	m = Filter('LKLeastSquares')
        m.children.append(newVar0)
        m.children.append(newVar1)
        m.children.append(v0)
        m.children.append(v1)
        m.children.append(v2)
        astVariables[id(astNode)] = [newVar0, newVar1]
        astNodes[id(astNode)] = astNodes[id(astNode.args[0])] + \
                                astNodes[id(astNode.args[1])] + \
                                astNodes[id(astNode.args[2])] + \
                                [m]
        return True
    return False

class ApplyBroxPreconditioner(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines):
    classname = astNode.__class__.__name__
    if(classname == 'Call'):
      if(astNode.func.id == 'applyBroxPreconditioner'):
        v0 = astVariables[id(astNode.args[0])]
        v1 = astVariables[id(astNode.args[1])]
        v2 = astVariables[id(astNode.args[2])]
        v3 = astVariables[id(astNode.args[3])]
        v4 = astVariables[id(astNode.args[4])]
        v5 = astVariables[id(astNode.args[5])]
        newVar0 = Array2D.CreateCopy(symbolTable, uniqueName(), v4 )
        newVar1 = Array2D.CreateCopy(symbolTable, uniqueName(), v5 )
	m = Filter('ApplyBroxPreconditioner')
        m.children.append(newVar0)
        m.children.append(newVar1)
        m.children.append(v0)
        m.children.append(v1)
        m.children.append(v2)
        m.children.append(v3)
        m.children.append(v4)
        m.children.append(v5)
        astVariables[id(astNode)] = [newVar0, newVar1]
        astNodes[id(astNode)] = astNodes[id(astNode.args[0])] + \
                                astNodes[id(astNode.args[1])] + \
                                astNodes[id(astNode.args[2])] + \
                                astNodes[id(astNode.args[3])] + \
                                astNodes[id(astNode.args[4])] + \
                                astNodes[id(astNode.args[5])] + \
                                [m]
        return True
    return False


class ApplyHSPreconditioner(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines):
    classname = astNode.__class__.__name__
    if(classname == 'Call'):
      if(astNode.func.id == 'applyHSPreconditioner'):
        v0 = astVariables[id(astNode.args[0])]
        v1 = astVariables[id(astNode.args[1])]
        v2 = astVariables[id(astNode.args[2])]
        v3 = astVariables[id(astNode.args[3])]
        v4 = astVariables[id(astNode.args[4])]
        v5 = astVariables[id(astNode.args[5])]
        newVar0 = Array2D.CreateCopy(symbolTable, uniqueName(), v4 )
        newVar1 = Array2D.CreateCopy(symbolTable, uniqueName(), v5 )
        m = Filter('ApplyHSPreconditioner')
        m.children.append(newVar0)
        m.children.append(newVar1)
        m.children.append(v0)
        m.children.append(v1)
        m.children.append(v2)
        m.children.append(v3)
        m.children.append(v4)
        m.children.append(v5)
        astVariables[id(astNode)] = [newVar0, newVar1]
        astNodes[id(astNode)] = astNodes[id(astNode.args[0])] + \
                                astNodes[id(astNode.args[1])] + \
                                astNodes[id(astNode.args[2])] + \
                                astNodes[id(astNode.args[3])] + \
                                astNodes[id(astNode.args[4])] + \
                                astNodes[id(astNode.args[5])] + \
                                [m]
        return True
    return False

class ArraySum(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines):
    classname = astNode.__class__.__name__
    if(classname == 'Call'):
      if(astNode.func.id == 'sum'):
        srcVar = astVariables[id(astNode.args[0])]
	if isArray(symbolTable, srcVar):
	  m = Filter('ArraySum')
          newVar = Scalar.CreateCopy(symbolTable, uniqueName(), srcVar)
          m.children.append((newVar))
          m.children.append((srcVar))
          astVariables[id(astNode)] = newVar
          astNodes[id(astNode)] = astNodes[id(astNode.args[0])] + [m]
          return True
    return False

class Array2DSum(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines):
    classname = astNode.__class__.__name__
    if(classname == 'Call'):
      if(astNode.func.id == 'sum2d'):
        srcVar = astVariables[id(astNode.args[0])]
	if isArray2D(symbolTable, srcVar):
	  m = Filter('Array2DSum')
          newVar = Scalar.CreateCopy(symbolTable, uniqueName(), srcVar)
          m.children.append((newVar))
          m.children.append((srcVar))
          astVariables[id(astNode)] = newVar
          astNodes[id(astNode)] = astNodes[id(astNode.args[0])] + [m]
          return True
    return False

class ScalarSqrt(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines):
    classname = astNode.__class__.__name__
    if(classname == 'Call'):
      if(astNode.func.id == 'sqrt'):
        srcVar = astVariables[id(astNode.args[0])]
	if isScalar(symbolTable, srcVar):
	  m = Filter('ScalarSqrt')
          newVar = Scalar.CreateCopy(symbolTable, uniqueName(), srcVar)
          m.children.append((newVar))
          m.children.append((srcVar))
          astVariables[id(astNode)] = newVar
          astNodes[id(astNode)] = astNodes[id(astNode.args[0])] + [m]
          return True
    return False

class ArraySqrt(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines):
    classname = astNode.__class__.__name__
    if(classname == 'Call'):
      if(astNode.func.id == 'sqrt'):
        srcVar = astVariables[id(astNode.args[0])]
	if isArray(symbolTable, srcVar):
	  m = Filter('ArraySqrt')
          newVar = Array.CreateCopy(symbolTable, uniqueName(), srcVar)
          m.children.append((newVar))
          m.children.append((srcVar))
          astVariables[id(astNode)] = newVar
          astNodes[id(astNode)] = astNodes[id(astNode.args[0])] + [m]
          return True
    return False

class Array2DSqrt(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines):
    classname = astNode.__class__.__name__
    if(classname == 'Call'):
      if(astNode.func.id == 'sqrt'):
        srcVar = astVariables[id(astNode.args[0])]
	if isArray2D(symbolTable, srcVar):
	  m = Filter('Array2DSqrt')
          newVar = Array2D.CreateCopy(symbolTable, uniqueName(), srcVar)
          m.children.append((newVar))
          m.children.append((srcVar))
          astVariables[id(astNode)] = newVar
          astNodes[id(astNode)] = astNodes[id(astNode.args[0])] + [m]
          return True
    return False

class RedArray2D(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines):
    classname = astNode.__class__.__name__
    if(classname == 'Call'):
      if(astNode.func.id == 'red'):
        srcVar = astVariables[id(astNode.args[0])]
	if isArray2D(symbolTable, srcVar):
	  m = Filter('RedArray2D')
          newVar = Array2D.CreateCopy(symbolTable, uniqueName(), srcVar)
          m.children.append((newVar))
          m.children.append((srcVar))
          astVariables[id(astNode)] = newVar
          astNodes[id(astNode)] = astNodes[id(astNode.args[0])] + [m]
          return True
    return False

class BlackArray2D(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines):
    classname = astNode.__class__.__name__
    if(classname == 'Call'):
      if(astNode.func.id == 'black'):
        srcVar = astVariables[id(astNode.args[0])]
	if isArray2D(symbolTable, srcVar):
	  m = Filter('BlackArray2D')
          newVar = Array2D.CreateCopy(symbolTable, uniqueName(), srcVar)
          m.children.append((newVar))
          m.children.append((srcVar))
          astVariables[id(astNode)] = newVar
          astNodes[id(astNode)] = astNodes[id(astNode.args[0])] + [m]
          return True
    return False

class SpmvDIA(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines):
    classname = astNode.__class__.__name__
    if(classname == 'BinOp'):
      leftVariable = astVariables[id(astNode.left)]
      rightVariable = astVariables[id(astNode.right)]
      if isSpMat(symbolTable, leftVariable) and isArray(symbolTable, rightVariable):
	m = Filter('SpmvDIA')
        newvar = Array.CreateCopy(symbolTable, uniqueName(), rightVariable )
        m.children.append((newvar))
        m.children.append((astVariables[id(astNode.left)]))
        m.children.append((astVariables[id(astNode.right)]))
        astVariables[id(astNode)] = newvar
        astNodes[id(astNode)] = astNodes[id(astNode.left)] + astNodes[id(astNode.right)] + [m]
        return True
    return False

class SpmvDIA2D(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines):
    classname = astNode.__class__.__name__
    if(classname == 'BinOp'):
      leftVariable = astVariables[id(astNode.left)]
      rightVariable = astVariables[id(astNode.right)]
      if isSpMat2D(symbolTable, leftVariable) and isArray2D(symbolTable, rightVariable):
	m = Filter('SpmvDIA2D')
        newvar = Array2D.CreateCopy(symbolTable, uniqueName(), rightVariable )
        m.children.append((newvar))
        m.children.append((astVariables[id(astNode.left)]))
        m.children.append((astVariables[id(astNode.right)]))
        astVariables[id(astNode)] = newvar
        astNodes[id(astNode)] = astNodes[id(astNode.left)] + astNodes[id(astNode.right)] + [m]
        return True
    return False

class Stencil2DMVM(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines):
    classname = astNode.__class__.__name__
    if(classname == 'BinOp'):
      leftVariable = astVariables[id(astNode.left)]
      rightVariable = astVariables[id(astNode.right)]
      if isStencil(symbolTable, leftVariable) and isArray2D(symbolTable, rightVariable):
	m = Filter('Stencil2DMVM')
        newvar = Array2D.CreateCopy(symbolTable, uniqueName(), rightVariable )
        m.children.append((newvar))
        m.children.append((astVariables[id(astNode.left)]))
        m.children.append((astVariables[id(astNode.right)]))
        astVariables[id(astNode)] = newvar
        astNodes[id(astNode)] = astNodes[id(astNode.left)] + astNodes[id(astNode.right)] + [m]
        return True
    return False

class SpmvCSR(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines):
    classname = astNode.__class__.__name__
    if(classname == 'BinOp'):
      leftVariable = astVariables[id(astNode.left)]
      rightVariable = astVariables[id(astNode.right)]
      if isCSRMat(symbolTable, leftVariable) and isArray(symbolTable, rightVariable):
	m = Filter('SpmvCSR')
	shape = (leftVariable.properties['shape'][0], )
        newvar = Array.Create(symbolTable, uniqueName(), rightVariable.properties['dtype'], shape, 'Inferred')
        m.children.append((newvar))
        m.children.append((astVariables[id(astNode.left)]))
        m.children.append((astVariables[id(astNode.right)]))
        astVariables[id(astNode)] = newvar
        astNodes[id(astNode)] = astNodes[id(astNode.left)] + astNodes[id(astNode.right)] + [m]
        return True
    return False

class ArrayCopy(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines):
    classname = astNode.__class__.__name__
    if(classname == 'Assign'):
      value = astVariables[id(astNode.value)]

      # List of Arrays?
      if isList(value) is False:
        value_list = [value]
	target_list = astNode.targets
      else:
        value_list = value
	target_list = astNode.targets[0].elts
        
      astNodes[id(astNode)] = astNodes[id(astNode.value)]

      for (value_id, value) in enumerate(value_list):
        if isArray(symbolTable, value):
	  ac_node = Filter('ArrayCopy')
          target = Array.CreateCopy(symbolTable, target_list[value_id].id, value)
          ac_node.children.append((target))
          ac_node.children.append((value))
          astNodes[id(astNode)] += [ac_node]
          astVariables[id(astNode)] = target
	else:
	  return False
      return True
    return False

class Array2DCopy(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines):
    classname = astNode.__class__.__name__
    if(classname == 'Assign'):
      value = astVariables[id(astNode.value)]

      # List of Array2Ds?
      if isList(value) is False:
        value_list = [value]
	target_list = astNode.targets
      else:
        value_list = value
	target_list = astNode.targets[0].elts

      astNodes[id(astNode)] = astNodes[id(astNode.value)]

      for (value_id, value) in enumerate(value_list):
        if isArray2D(symbolTable, value):
	  ac_node = Filter('Array2DCopy')
          target = Array2D.CreateCopy(symbolTable, target_list[value_id].id, value)
          ac_node.children.append((target))
          ac_node.children.append((value))
          astNodes[id(astNode)] += [ac_node]
          astVariables[id(astNode)] = target
        else:
          return False
      return True
    return False

class Array3DCopy(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines):
    classname = astNode.__class__.__name__
    if(classname == 'Assign'):
      value = astVariables[id(astNode.value)]

      # List of Array3Ds?
      if isList(value) is False:
        value_list = [value]
	target_list = astNode.targets
      else:
        value_list = value
	target_list = astNode.targets[0].elts

      astNodes[id(astNode)] = astNodes[id(astNode.value)]

      for (value_id, value) in enumerate(value_list):
        if isArray3D(symbolTable, value):
          ac_node = Filter('Array3DCopy')
          target = Array3D.CreateCopy(symbolTable, target_list[value_id].id, value)
          ac_node.children.append((target))
          ac_node.children.append((value))
          astNodes[id(astNode)] += [ac_node]
          astVariables[id(astNode)] = target
        else:
          return False
      return True
    return False



class ScalarCopy(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines):
    classname = astNode.__class__.__name__
    if(classname == 'Assign'):
      value = astVariables[id(astNode.value)]

      # List of Scalars?
      if isList(value) is False:
        value_list = [value]
	target_list = astNode.targets
      else:
        value_list = value
	target_list = astNode.targets[0].elts
        
      astNodes[id(astNode)] = astNodes[id(astNode.value)]

      for (value_id, value) in enumerate(value_list):
        if isScalar(symbolTable, value):
	  ac_node = Filter('ScalarCopy')
          target = Scalar.CreateCopy(symbolTable, target_list[value_id].id, value)
          ac_node.children.append((target))
          ac_node.children.append((value))
          astNodes[id(astNode)] += [ac_node]
          astVariables[id(astNode)] = target
	else:
	  return False
      return True
    return False

class Return(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines):
    classname = astNode.__class__.__name__
    if(classname == 'Return'):
      m = Filter('Return')
      value = astVariables[id(astNode.value)]
      
      # List?
      if isList(value) is False:
        value_list = [value]
      else:
        value_list = value
      
      for value in value_list:
        m.children.append(value)

      astNodes[id(astNode)] = astNodes[id(astNode.value)] + [m]
      return True
    else:
      return False

class ExtractSnapshots(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines):
    classname = astNode.__class__.__name__
    if(classname == 'Call'):
      if(astNode.func.id == 'extract_snapshots'):
        v0 = astVariables[id(astNode.args[0])]
	sym0 = symbolTable[v0.properties['name']]
	shape = (defines['N_BLOCKS'] * defines['N_DOP'], defines['TRAINING_BLOCK_SIZE'], defines['N_CHAN'] * defines['TDOF'])
	newVar0 = Array3D.Create(symbolTable, uniqueName(), sym0.properties['dtype'], shape, 'Inferred', False)
	m = Filter('ExtractSnapshots')
        m.children.append(newVar0)
        m.children.append(v0)
        astVariables[id(astNode)] = newVar0
        astNodes[id(astNode)] = astNodes[id(astNode.args[0])] + \
                                [m]
        return True
    return False

class CgemmBatch(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines):
    classname = astNode.__class__.__name__
    if(classname == 'Call'):
      if(astNode.func.id == 'cgemm_batch'):
        v0 = astVariables[id(astNode.args[0])]
	sym0 = symbolTable[v0.properties['name']]
	shape = (defines['N_BLOCKS'] * defines['N_DOP'], defines['N_CHAN'] * defines['TDOF'], defines['N_CHAN'] * defines['TDOF'])
	newVar0 = Array3D.Create(symbolTable, uniqueName(), sym0.properties['dtype'], shape, 'Inferred', False)
	m = Filter('CgemmBatch')
        m.children.append(newVar0)
        m.children.append(v0)
        astVariables[id(astNode)] = newVar0
        astNodes[id(astNode)] = astNodes[id(astNode.args[0])] + \
                                [m]
        return True
    return False

class SolveBatch(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines):
    classname = astNode.__class__.__name__
    if(classname == 'Call'):
      if(astNode.func.id == 'solve_batch'):
        v0 = astVariables[id(astNode.args[0])]
        v1 = astVariables[id(astNode.args[1])]
	sym0 = symbolTable[v0.properties['name']]
	shape = (defines['N_BLOCKS'] * defines['N_DOP'], defines['N_STEERING'], defines['N_CHAN'] * defines['TDOF'])
	newVar0 = Array3D.Create(symbolTable, uniqueName(), sym0.properties['dtype'], shape, 'Inferred', False)
	m = Filter('SolveBatch')
        m.children.append(newVar0)
        m.children.append(v0)
        m.children.append(v1)
        astVariables[id(astNode)] = newVar0
        astNodes[id(astNode)] = astNodes[id(astNode.args[0])] + \
                                astNodes[id(astNode.args[1])] + \
                                [m]
        return True
    return False

class GammaWeights(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines):
    classname = astNode.__class__.__name__
    if(classname == 'Call'):
      if(astNode.func.id == 'gamma_weights'):
        v0 = astVariables[id(astNode.args[0])]
        v1 = astVariables[id(astNode.args[1])]
	sym0 = symbolTable[v0.properties['name']]
	shape = (defines['N_BLOCKS'] * defines['N_DOP'], defines['N_STEERING'])
	newVar0 = Array2D.Create(symbolTable, uniqueName(), sym0.properties['dtype'], shape, 'Inferred', False)
	m = Filter('GammaWeights')
        m.children.append(newVar0)
        m.children.append(v0)
        m.children.append(v1)
        astVariables[id(astNode)] = newVar0
        astNodes[id(astNode)] = astNodes[id(astNode.args[0])] + \
                                astNodes[id(astNode.args[1])] + \
                                [m]
        return True
    return False

class InnerProducts(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines):
    classname = astNode.__class__.__name__
    if(classname == 'Call'):
      if(astNode.func.id == 'inner_products'):
        v0 = astVariables[id(astNode.args[0])]
        v1 = astVariables[id(astNode.args[1])]
        v2 = astVariables[id(astNode.args[2])]
	sym0 = symbolTable[v0.properties['name']]
	shape = (defines['N_BLOCKS'] * defines['N_DOP'], defines['TRAINING_BLOCK_SIZE'], defines['N_STEERING'])
	newVar0 = Array3D.Create(symbolTable, uniqueName(), sym0.properties['dtype'], shape, 'Inferred', False)
	m = Filter('InnerProducts')
        m.children.append(newVar0)
        m.children.append(v0)
        m.children.append(v1)
        m.children.append(v2)
        astVariables[id(astNode)] = newVar0
        astNodes[id(astNode)] = astNodes[id(astNode.args[0])] + \
                                astNodes[id(astNode.args[1])] + \
                                astNodes[id(astNode.args[2])] + \
                                [m]
        return True
    return False



class Tuple(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines): 
    classname = astNode.__class__.__name__
    if(classname == 'Tuple'):
      astNodes[id(astNode)] = []
      astVariables[id(astNode)] = []
      for elt in astNode.elts:
        astVariables[id(astNode)] += [astVariables[id(elt)]]
      return True
    else:
      return False

class Name(Node):
  @staticmethod
  def match( astNode, semanticModel, symbolTable, astVariables, astNodes, defines): 
    classname = astNode.__class__.__name__
    if(classname == 'Name'):
      name = Name()
      name.properties['name'] = astNode.id
      astNodes[id(astNode)] = []
      try:
        astVariables[id(astNode)] = name
      except:
        pass
      return True
    else:
      return False

class Scalar(Node):
  @staticmethod
  def Create(symbolTable, name, dtype, source):
    currentSymbol = symbolTable.get(name, None)
    if currentSymbol is None:
      var = Scalar()
      var.properties['name'] = name
      var.properties['shape'] = ()
      var.properties['source'] = source
      var.properties['io'] = 'inout'
      var.properties['dtype'] = dtype
      symbolTable[name] = var
      ret = Name()
      ret.properties['name'] = name
      return ret
    else:
      ret = Name()
      ret.properties['name'] = name
      return ret

  @staticmethod
  def CreateImmediate(val):
    ret = Name()
    ret.properties['val'] = str(val)
    ret.properties['dtype'] = 'float64'
    return ret

  @staticmethod
  def CreateCopy(symbolTable, name, toCopy):
    currentSymbol = symbolTable.get(name, None)
    toCopy = symbolTable[toCopy.properties['name']]
    if currentSymbol is None:
      var = Scalar()
      var.properties['name'] = name
      var.properties['source'] = 'Inferred'
      var.properties['dtype'] = toCopy.properties['dtype']
      symbolTable[name] = var
      ret = Name()
      ret.properties['name'] = name
      return ret
    else:
      ret = Name()
      ret.properties['name'] = name
      return ret

class Stencil(Node):
  @staticmethod
  def Create(symbolTable, name, dtype, source, ndiags):
    currentSymbol = symbolTable.get(name, None)
    if currentSymbol is None:
      var = Stencil()
      var.properties['name'] = name
      var.properties['ndiags'] = ndiags
      var.properties['source'] = source
      var.properties['io'] = 'inout'
      var.properties['dtype'] = dtype
      symbolTable[name] = var
      ret = Name()
      ret.properties['name'] = name
      return ret
    else:
      ret = Name()
      ret.properties['name'] = name
      return ret

class SpMat(Node):
  @staticmethod
  def Create(symbolTable, name, dtype, shape, source, ndiags):
    currentSymbol = symbolTable.get(name, None)
    if currentSymbol is None:
      var = SpMat()
      var.properties['name'] = name
      var.properties['shape'] = shape
      var.properties['source'] = source
      var.properties['io'] = 'inout'
      var.properties['dtype'] = dtype
      var.properties['ndiags'] = ndiags
      var.properties['dim'] = shape[0]
      symbolTable[name] = var
      ret = Name()
      ret.properties['name'] = name
      return ret
    else:
      ret = Name()
      ret.properties['name'] = name
      return ret

class SpMat2D(Node):
  @staticmethod
  def Create(symbolTable, name, dtype, shape, source, ndiags, dim0, dim1):
    currentSymbol = symbolTable.get(name, None)
    if currentSymbol is None:
      var = SpMat2D()
      var.properties['name'] = name
      var.properties['shape'] = shape
      var.properties['source'] = source
      var.properties['io'] = 'inout'
      var.properties['dtype'] = dtype
      var.properties['ndiags'] = ndiags
      var.properties['dim0'] = dim0
      var.properties['dim1'] = dim1 
      symbolTable[name] = var
      ret = Name()
      ret.properties['name'] = name
      return ret
    else:
      ret = Name()
      ret.properties['name'] = name
      return ret

class CSRMat(Node):
  @staticmethod
  def Create(symbolTable, name, dtype, shape, source, nnz):
    currentSymbol = symbolTable.get(name, None)
    if currentSymbol is None:
      var = CSRMat()
      var.properties['name'] = name
      var.properties['shape'] = shape
      var.properties['source'] = source
      var.properties['io'] = 'inout'
      var.properties['dtype'] = dtype
      var.properties['nnz'] = nnz
      var.properties['dim'] = shape[0]
      var.properties['dim0'] = shape[0]
      var.properties['dim1'] = shape[1]
      symbolTable[name] = var
      ret = Name()
      ret.properties['name'] = name
      return ret
    else:
      ret = Name()
      ret.properties['name'] = name
      return ret

class Array(Node):
  @staticmethod
  def Create(symbolTable, name, dtype, shape, source):
    currentSymbol = symbolTable.get(name, None)
    if currentSymbol is None:
      var = Array()
      var.properties['name'] = name
      var.properties['source'] = source
      var.properties['io'] = 'inout'
      var.properties['dtype'] = dtype
      var.properties['dim0'] = shape[0] 
      var.properties['dim1'] = 0 
      var.properties['shape'] = shape
      symbolTable[name] = var
      ret = Name()
      ret.properties['name'] = name
      return ret
    else:
      ret = Name()
      ret.properties['name'] = name
      return ret

  @staticmethod
  def CreateCopy(symbolTable, name, toCopy):
    currentSymbol = symbolTable.get(name, None)
    toCopy = symbolTable[toCopy.properties['name']]
    if currentSymbol is None:
      var = Array()
      var.properties = copy.copy(toCopy.properties)
      var.properties['name'] = name
      var.properties['source'] = 'Inferred'
      symbolTable[name] = var
      ret = Name()
      ret.properties['name'] = name
      return ret
    else:
      ret = Name()
      ret.properties['name'] = name
      return ret

class Array2D(Node):
  @staticmethod
  def Create(symbolTable, name, dtype, shape, source, col_mjr):
    currentSymbol = symbolTable.get(name, None)
    if currentSymbol is None:
      var = Array2D()
      var.properties['name'] = name
      var.properties['source'] = source
      var.properties['io'] = 'inout'
      var.properties['dtype'] = dtype
      var.properties['dim0'] = shape[0] 
      var.properties['dim1'] = shape[1]
      var.properties['col_mjr'] = str(col_mjr)
      var.properties['shape'] = shape
      symbolTable[name] = var
      ret = Name()
      ret.properties['name'] = name
      return ret
    else:
      ret = Name()
      ret.properties['name'] = name
      return ret

  @staticmethod
  def CreateCopy(symbolTable, name, toCopy):
    currentSymbol = symbolTable.get(name, None)
    toCopy = symbolTable[toCopy.properties['name']]
    if currentSymbol is None:
      var = Array2D()
      var.properties = copy.copy(toCopy.properties)
      var.properties['name'] = name
      var.properties['source'] = 'Inferred'
      symbolTable[name] = var
      ret = Name()
      ret.properties['name'] = name
      return ret
    else:
      ret = Name()
      ret.properties['name'] = name
      return ret

class Array3D(Node):
  @staticmethod
  def Create(symbolTable, name, dtype, shape, source, col_mjr):
    currentSymbol = symbolTable.get(name, None)
    if currentSymbol is None:
      var = Array3D()
      var.properties['name'] = name
      var.properties['source'] = source
      var.properties['io'] = 'inout'
      var.properties['dtype'] = dtype
      var.properties['dim0'] = shape[0] 
      var.properties['dim1'] = shape[1]
      var.properties['dim2'] = shape[2]
      var.properties['col_mjr'] = str(col_mjr)
      var.properties['shape'] = shape
      symbolTable[name] = var
      ret = Name()
      ret.properties['name'] = name
      return ret
    else:
      ret = Name()
      ret.properties['name'] = name
      return ret

  @staticmethod
  def CreateCopy(symbolTable, name, toCopy):
    currentSymbol = symbolTable.get(name, None)
    toCopy = symbolTable[toCopy.properties['name']]
    if currentSymbol is None:
      var = Array3D()
      var.properties = copy.copy(toCopy.properties)
      var.properties['name'] = name
      var.properties['source'] = 'Inferred'
      symbolTable[name] = var
      ret = Name()
      ret.properties['name'] = name
      return ret
    else:
      ret = Name()
      ret.properties['name'] = name
      return ret



def isList(name):
  return name.__class__.__name__ == 'list'

def isArray(symbolTable, name):
  symbol = symbolTable.get(name.properties['name'], None)
  if symbol is None:
    import pdb; pdb.set_trace()
  else:
    return symbol.__class__.__name__ == 'Array'

def isArray2D(symbolTable, name):
  symbol = symbolTable.get(name.properties['name'], None)
  if symbol is None:
    import pdb; pdb.set_trace()
  else:
    return symbol.__class__.__name__ == 'Array2D'

def isArray3D(symbolTable, name):
  symbol = symbolTable.get(name.properties['name'], None)
  if symbol is None:
    import pdb; pdb.set_trace()
  else:
    return symbol.__class__.__name__ == 'Array3D'

def isScalar(symbolTable, name):
  symbol = symbolTable.get(name.properties['name'], None)
  if symbol is None:
    import pdb; pdb.set_trace()
  else:
    return symbol.__class__.__name__ == 'Scalar'

def isStencil(symbolTable, name):
  symbol = symbolTable.get(name.properties['name'], None)
  if symbol is None:
    import pdb; pdb.set_trace()
  else:
    return symbol.__class__.__name__ == 'Stencil'

def isSpMat(symbolTable, name):
  symbol = symbolTable.get(name.properties['name'], None)
  if symbol is None:
    import pdb; pdb.set_trace()
  else:
    return symbol.__class__.__name__ == 'SpMat'

def isSpMat2D(symbolTable, name):
  symbol = symbolTable.get(name.properties['name'], None)
  if symbol is None:
    import pdb; pdb.set_trace()
  else:
    return symbol.__class__.__name__ == 'SpMat2D'

def isCSRMat(symbolTable, name):
  symbol = symbolTable.get(name.properties['name'], None)
  if symbol is None:
    import pdb; pdb.set_trace()
  else:
    return symbol.__class__.__name__ == 'CSRMat'

