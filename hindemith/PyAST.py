# File: PyAST.py
# Author: Michael Anderson
# Description: These are the routines that process the Python AST, fill the semantic model,
# and insert a back-end call into the Python AST.

import ast
import Templates
from subprocess import call

class HindemithBuildError(Exception): pass
try:
    import backend
except ImportError as error:
    print error.message
    print "unable to open the Hindemith backend, have you run make successfully"
    raise HindemithBuildError


class PrototypeVisitor(ast.NodeVisitor):
    def __init__(self, symbolTable, kwargs):
        self.symbolTable = symbolTable
        self.kwargs = kwargs

    def visit_FunctionDef(self, node):
        for key in self.kwargs:
            name = key
            pyobj = self.kwargs[key]
            pytype_str = pyobj.__class__.__name__
            if pytype_str == 'SpMat2D':
                Templates.SpMat2D.Create(self.symbolTable, name, str(pyobj.dtype),
                                         (pyobj.dim0 * pyobj.dim1, pyobj.dim0 * pyobj.dim1), 'Header', pyobj.ndiags,
                                         pyobj.dim0, pyobj.dim1)
            if pytype_str == 'Stencil':
                Templates.Stencil.Create(self.symbolTable, name, str(pyobj.dtype), 'Header', len(pyobj.data))
            if pytype_str == 'dia_matrix':
                Templates.SpMat.Create(self.symbolTable, name, str(pyobj.dtype), pyobj.shape, 'Header',
                                       pyobj.data.shape[0])
            if pytype_str == 'csr_matrix':
                Templates.CSRMat.Create(self.symbolTable, name, str(pyobj.dtype), pyobj.shape, 'Header', pyobj.nnz)
            if pytype_str == 'ndarray':
                if len(pyobj.shape) == 1:
                    Templates.Array.Create(self.symbolTable, name, str(pyobj.dtype), pyobj.shape, 'Header')
                if len(pyobj.shape) == 2:
                    Templates.Array2D.Create(self.symbolTable, name, str(pyobj.dtype), pyobj.shape, 'Header',
                                             pyobj.flags['F_CONTIGUOUS'])
                if len(pyobj.shape) == 3:
                    Templates.Array3D.Create(self.symbolTable, name, str(pyobj.dtype), pyobj.shape, 'Header',
                                             pyobj.flags['F_CONTIGUOUS'])
            if pytype_str == 'float32':
                Templates.Scalar.Create(self.symbolTable, name, 'float32', 'Header')
            if pytype_str == 'float64':
                Templates.Scalar.Create(self.symbolTable, name, 'float64', 'Header')
            if pytype_str == 'int64':
                Templates.Scalar.Create(self.symbolTable, name, 'int64', 'Header')


class EmitVisitor(ast.NodeVisitor):
    def __init__(self, semanticModel, symbolTable, astVariables, astNodes, defines):
        self.semanticModel = semanticModel
        self.symbolTable = symbolTable
        self.astVariables = astVariables
        self.astNodes = astNodes
        self.defines = defines
        self.templates = [Templates.FunctionDef, Templates.Iterator, Templates.ArrayOp, Templates.ArrayScalarOp,
                          Templates.ScalarArrayOp, Templates.ScalarOp, Templates.ArraySum, Templates.Array2DSum,
                          Templates.ArrayCopy, Templates.ScalarCopy, Templates.ScalarSqrt, Templates.ArraySqrt,
                          Templates.SpmvDIA, Templates.SpmvCSR, Templates.Return, Templates.Num, Templates.Name,
                          Templates.Array2DCopy, Templates.Array2DCopy, Templates.WarpImg2D, Templates.LKLeastSquares,
                          Templates.Array2DOp, Templates.Array3DOp, Templates.Array3DCopy, Templates.Tuple,
                          Templates.ExtractSnapshots, Templates.CgemmBatch, Templates.ScalarArray2DOp,
                          Templates.Array2DScalarOp, Templates.SolveBatch, Templates.GammaWeights,
                          Templates.InnerProducts, Templates.Stencil2DMVM, Templates.SpmvDIA2D, Templates.SetBroxMatrix,
                          Templates.Array2DSqrt, Templates.ApplyHSPreconditioner, Templates.ApplyBroxPreconditioner,
                          Templates.RedArray2D, Templates.BlackArray2D, Templates.SetBroxRedblackMatrix,
                          Templates.ScalarArray3DOp, Templates.HSMatrixPowers]

    def visit(self, node):
        self.generic_visit(node)
        matches = False
        for template in self.templates:
            matches = template.match(node, self.semanticModel, self.symbolTable, self.astVariables, self.astNodes,
                                     self.defines)
            if matches:
                break

# XML Producer
class XMLVisitor(object):
    def visit(self, node, *args, **kwargs):
        meth = None
        for cls in node.__class__.__mro__:
            meth_name = 'visit_' + cls.__name__
            meth = getattr(self, meth_name, None)
            if meth:
                break

        if not meth:
            meth = self.generic_visit
        return meth(node, *args, **kwargs)

    def generic_visit(self, node, *args, **kwargs):
        xml = '<' + node.__class__.__name__
        for key in node.properties:
            xml += ' ' + key + '="' + str(node.properties[key]) + '"'
        if node.children:
            xml += '>'
            for child in node.children:
                xml += self.generic_visit(child)
            xml += '</' + node.__class__.__name__ + '>'
        else:
            xml += '/>'
        return xml


def defines_string(defines):
    vst = XMLVisitor()
    xmlStr = '<ConstantTable>'
    for key in defines:
        node = Templates.ScalarConstant()
        node.properties['name'] = key
        node.properties['value'] = defines[key]
        xmlStr += vst.visit(node)
    xmlStr += '</ConstantTable>'
    return xmlStr


def print_defines(defines):
    f = open('defines.xml', 'w')
    xmlStr = defines_string(defines)
    f.write(xmlStr)
    f.close()
    call_string = 'xmllint --format defines.xml --output defines.xml'
    call(call_string.split())
    return xmlStr


def symbol_table_string(symbolTable):
    vst = XMLVisitor()
    xmlStr = '<SymbolTable>'
    for key in symbolTable:
        xmlStr += vst.visit(symbolTable[key])
    xmlStr += '</SymbolTable>'
    return xmlStr


def print_symbol_table(symbolTable):
    f = open('symbolTable.xml', 'w')
    xmlStr = symbol_table_string(symbolTable)
    f.write(xmlStr)
    f.close()
    call_string = 'xmllint --format symbolTable.xml --output symbolTable.xml'
    call(call_string.split())
    return xmlStr


def print_semantic_model(semanticModel):
    vst = XMLVisitor()
    f = open('semanticModel.xml', 'w')
    xmlStr = vst.visit(semanticModel)
    f.write(xmlStr)
    f.close()
    call_string = 'xmllint --format semanticModel.xml --output semanticModel.xml'
    call(call_string.split())
    return xmlStr


records = []


def set_tuning_params(defines):
    #defines['nd1'] = 1
    #defines['nt1'] = 128
    #defines['ne1'] = 2
    #defines['nd20'] = 1
    #defines['nd21'] = 1
    #defines['nt20'] = 8
    #defines['nt21'] = 8
    #defines['ne20'] = 2
    #defines['ne21'] = 2
    pass


def mycompile(source, kwargs, defines):
    # Add stuff to defines
    set_tuning_params(defines)

    # Symbol table holds all variables names, shapes, and types
    symbolTable = {}

    # Mapping prototype variables into symbol table
    pytype_str = ''
    for key in kwargs:
        pytype_str += kwargs[key].__class__.__name__
        if kwargs[key].__class__.__name__ == 'ndarray':
            pytype_str += str(kwargs[key].shape)

    global records
    record_id = -1
    for (rid, record) in enumerate(records):
        if (record == pytype_str):
            #print 'Match rid: ' + str(rid) + ' ' + pytype_str
            record_id = rid

    if (record_id == -1):
        records.append(pytype_str)
        record_id = len(records) - 1

        # Parse ast
        astNode = ast.parse(source)
        semanticModel = Templates.SemanticModel()
        astVariables = {}
        astNodes = {}

        vst = PrototypeVisitor(symbolTable, kwargs)
        vst.visit(astNode)

        vst = EmitVisitor(semanticModel, symbolTable, astVariables, astNodes, defines)
        vst.visit(astNode)

        symbolTableXML = print_symbol_table(symbolTable)
        semanticModelXML = print_semantic_model(semanticModel)
        definesXML = print_defines(defines)

        backend.xmlcompile(record_id, symbolTableXML, semanticModelXML, definesXML)

        return (record_id, kwargs)

    else:
        return (record_id, kwargs)


def run_backend(record_id, kwargs):
    retval, state = backend.run(record_id, kwargs)
    return retval, state


def free_backend():
    backend.free_ocl()
