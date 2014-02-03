UNAME := $(shell uname)
HOSTNAME := $(shell hostname)

CC=g++
#DEFINES = -DVERBOSE_COMPILATION -DPROFILE_TOP -DVERBOSE_EXECUTION -DPROFILE
#DEFINES = -DPROFILE_TOP
DEFINES = 


TYPES_SOURCES=Types/SpMat.cpp Types/Array.cpp Types/Scalar.cpp Types/CSRMat.cpp Types/primitives.cpp Types/Array2D.cpp Types/Stencil.cpp Types/SpMat2D.cpp Types/Array3D.cpp
DENSE_LINEAR_ALGEBRA_SOURCES=Operations/DenseLinearAlgebra/ArrayOp.cpp Operations/DenseLinearAlgebra/ArrayCopy.cpp Operations/DenseLinearAlgebra/ArraySum.cpp Operations/DenseLinearAlgebra/ScalarCopy.cpp Operations/DenseLinearAlgebra/ScalarSqrt.cpp Operations/DenseLinearAlgebra/ArrayScalarOp.cpp Operations/DenseLinearAlgebra/ScalarArrayOp.cpp Operations/DenseLinearAlgebra/ScalarOp.cpp Operations/DenseLinearAlgebra/Array2DCopy.cpp Operations/DenseLinearAlgebra/Array2DOp.cpp Operations/DenseLinearAlgebra/Array2DScalarOp.cpp Operations/DenseLinearAlgebra/ScalarArray2DOp.cpp Operations/DenseLinearAlgebra/Array2DSum.cpp Operations/DenseLinearAlgebra/ArraySqrt.cpp Operations/DenseLinearAlgebra/Array2DSqrt.cpp Operations/DenseLinearAlgebra/RedArray2D.cpp Operations/DenseLinearAlgebra/BlackArray2D.cpp Operations/DenseLinearAlgebra/Array3DCopy.cpp Operations/DenseLinearAlgebra/Array3DOp.cpp Operations/DenseLinearAlgebra/ScalarArray3DOp.cpp
STAP_SOURCES=Operations/STAP/ExtractSnapshots.cpp Operations/STAP/CgemmBatch.cpp Operations/STAP/SolveBatch.cpp Operations/STAP/GammaWeights.cpp Operations/STAP/InnerProducts.cpp
SPARSE_LINEAR_ALGEBRA_SOURCES=Operations/SparseLinearAlgebra/SpmvDIA.cpp Operations/SparseLinearAlgebra/SpmvCSR.cpp Operations/SparseLinearAlgebra/SpmvDIA2D.cpp
OPTICAL_FLOW_SOURCES=Operations/OpticalFlow/WarpImg2D.cpp Operations/OpticalFlow/LKLeastSquares.cpp Operations/OpticalFlow/SetBroxRedblackMatrix.cpp Operations/OpticalFlow/SetBroxMatrix.cpp Operations/OpticalFlow/ApplyHSPreconditioner.cpp Operations/OpticalFlow/ApplyBroxPreconditioner.cpp Operations/OpticalFlow/HSMatrixPowers.cpp
STRUCTURED_GRID_SOURCES=Operations/StructuredGrid/Stencil2DMVM.cpp

OPERATIONS_SOURCES=Operations/Return.cpp $(DENSE_LINEAR_ALGEBRA_SOURCES) $(STAP_SOURCES) $(SPARSE_LINEAR_ALGEBRA_SOURCES) $(OPTICAL_FLOW_SOURCES) $(STRUCTURED_GRID_SOURCES)
OCL_SOURCES=backend.cpp sm.cpp operations.cpp clhelp.cpp pugixml.cpp timestamp.cpp dataflow.cpp em.cpp fusion.cpp lm.cpp execute.cpp oclgen.cpp docs.cpp $(TYPES_SOURCES) $(OPERATIONS_SOURCES)  profiler.cpp #record.cpp
OCL_OBJECTS=$(OCL_SOURCES:.cpp=.o)

ifeq ($(UNAME), Darwin)
OCL_LIB=-framework OpenCL
OCL_INCLUDE=
PYTHON_INCLUDE=-I/System/Library/Frameworks/Python.framework/Versions/2.7/Extras/lib/python/numpy/core/include -I/System/Library/Frameworks/Python.framework/Versions/2.7/include
OCL_EXECUTABLE=backend.so
endif

ifeq ($(UNAME), Linux)
ifeq ($(HOSTNAME), llano)
OCL_LIB=-L/opt/AMDAPP/lib/x86_64 -lOpenCL
OCL_INCLUDE=-I/opt/AMDAPP/include
PYTHON_INCLUDE=-I/usr/include/python2.7/
OCL_EXECUTABLE=backend.so
else
OCL_LIB=-L/usr/lib64/nvidia -lOpenCL
OCL_INCLUDE=-I/usr/local/cuda/include
PYTHON_INCLUDE=-I/usr/include/python2.7/
OCL_EXECUTABLE=backend.so
endif
endif

CFLAGS=-c -Wall $(OCL_INCLUDE) $(PYTHON_INCLUDE) -fpic -g
LDFLAGS=$(OCL_LIB)

all: $(SOURCES) $(OCL_EXECUTABLE)

ifeq ($(UNAME), Linux)
$(OCL_EXECUTABLE): $(OCL_OBJECTS)
	$(CC) $(OCL_OBJECTS) -shared -Wl,-soname,"$@" -fpic -o $@ $(LDFLAGS) 
endif

ifeq ($(UNAME), Darwin)
$(OCL_EXECUTABLE): $(OCL_OBJECTS)
	$(CC) -dynamiclib -Wl,-headerpad_max_install_names,-undefined,dynamic_lookup,-compatibility_version,1.0,-current_version,1.0 -o $@ $(OCL_OBJECTS) $(LDFLAGS) 
endif

.cpp.o:
	$(CC) $(CFLAGS) $(DEFINES) $< -o $@

clean:
	rm -f *.pyc *.xml $(OCL_EXECUTABLE) *.o *.so Operations/*.o Types/*.o Operations/STAP/*.o Operations/DenseLinearAlgebra/*.o Operations/SparseLinearAlgebra/*.o Operations/StructuredGrid/*.o Operations/OpticalFlow/*.o
