/*
 *  Copyright 2007-2015 The OpenMx Project
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */

/***********************************************************
 *
 *  omxMatrix.h
 *
 *  Created: Timothy R. Brick 	Date: 2008-11-13 12:33:06
 *
 *	Contains header information for the omxMatrix class
 *   omxDataMatrices hold necessary information to simplify
 * 	dealings between the OpenMX back end and BLAS.
 *
 **********************************************************/

#ifndef _OMXMATRIX_H_
#define _OMXMATRIX_H_

#include "omxDefines.h"
#include "Eigen/Core"
#include "omxBLAS.h"

#include "omxAlgebra.h"
#include "omxFitFunction.h"
#include "omxExpectation.h"
#include "omxState.h"

struct populateLocation {
	int from;
	int srcRow, srcCol;
	int destRow, destCol;

	void transpose() { std::swap(destRow, destCol); }
};

class omxMatrix {
/* For inclusion in(or of) other matrices */
	std::vector< populateLocation > populate;
 public:
	void transposePopulate();
	void omxProcessMatrixPopulationList(SEXP matStruct);
	void omxPopulateSubstitutions(int want, FitContext *fc);
										//TODO: Improve encapsulation
/* Actually Useful Members */
	int rows, cols;						// Matrix size  (specifically, its leading edge)
	double* data;						// Actual Data Pointer
	unsigned short colMajor;			// used for quick transpose
	unsigned short hasMatrixNumber;		// is this object in the matrix or algebra arrays?
	int matrixNumber;					// the offset into the matrices or algebras arrays

	SEXP owner;	// The R object owning data or NULL if we own it.

	// size of allocated memory of data pointer
	int originalRows;
	int originalCols;

/* For BLAS Multiplication Speedup */ 	// TODO: Replace some of these with inlines or macros.
	const char* majority;				// Filled by compute(), included for speed
	const char* minority;				// Filled by compute(), included for speed
	int leading;						// Leading edge; depends on original majority
	int lagging;						// Non-leading edge.

/* Curent State */
	omxState* currentState;				// Optimizer State
	unsigned cleanVersion;
	unsigned version;

/* For Algebra Functions */				// At most, one of these may be non-NULL.
	omxAlgebra* algebra;				// If it's not an algebra, this is NULL.
	omxFitFunction* fitFunction;		// If it's not a fit function, this is NULL.

	const char* name;

	// char pointers are from R and should not be freed
	std::vector<const char *> rownames;
	std::vector<const char *> colnames;

	friend void omxCopyMatrix(omxMatrix *dest, omxMatrix *src);  // turn into method later TODO
	void setNotConstant();
};

// If you call these functions directly then you need to free the memory with omxFreeMatrix.
// If you obtain a matrix from omxNewMatrixFromSlot then you must NOT free it.
omxMatrix* omxInitMatrix(int nrows, int ncols, unsigned short colMajor, omxState* os);

	void omxFreeMatrix(omxMatrix* om);						// Ditto, traversing argument trees

/* Matrix Creation Functions */
	omxMatrix* omxNewMatrixFromRPrimitive(SEXP rObject, omxState *state,
	unsigned short hasMatrixNumber, int matrixNumber); 							// Create an omxMatrix from an R object
	omxMatrix* omxNewIdentityMatrix(int nrows, omxState* state);				// Creates an Identity Matrix of a given size
	extern omxMatrix* omxMatrixLookupFromState1(SEXP matrix, omxState* os);	// Create a matrix/algebra from a matrix pointer

	omxMatrix* omxDuplicateMatrix(omxMatrix* src, omxState* newState);
	SEXP omxExportMatrix(omxMatrix *om);

/* Getters 'n Setters (static functions declared below) */
	// static OMXINLINE double omxMatrixElement(omxMatrix *om, int row, int col);
	// static OMXINLINE double omxVectorElement(omxMatrix *om, int index);
	// static OMXINLINE void omxSetMatrixElement(omxMatrix *om, int row, int col, double value);
	// static OMXINLINE void omxSetVectorElement(omxMatrix *om, int index, double value);

	double* omxLocationOfMatrixElement(omxMatrix *om, int row, int col);
	void omxMarkDirty(omxMatrix *om);
void omxMarkClean(omxMatrix *om);

/* Matrix Modification Functions */
	void omxZeroByZeroMatrix(omxMatrix *source);
void omxResizeMatrix(omxMatrix *source, int nrows, int ncols);
	omxMatrix* omxFillMatrixFromRPrimitive(omxMatrix* om, SEXP rObject, omxState *state,
		unsigned short hasMatrixNumber, int matrixNumber); 								// Populate an omxMatrix from an R object
	void omxTransposeMatrix(omxMatrix *mat);												// Transpose a matrix in place.
void omxEnsureColumnMajor(omxMatrix *mat);
	void omxToggleRowColumnMajor(omxMatrix *mat);										// Transform row-major into col-major and vice versa 

/* Function wrappers that switch based on inclusion of algebras */
	void omxPrint(omxMatrix *source, const char* d);

void omxRecompute(omxMatrix *matrix, FitContext *fc);


void omxRemoveElements(omxMatrix *om, int numRemoved, int removed[]);
void omxRemoveRowsAndColumns(omxMatrix* om, int numRowsRemoved, int numColsRemoved, int rowsRemoved[], int colsRemoved[]);

/* Matrix-Internal Helper functions */
	void omxMatrixLeadingLagging(omxMatrix *matrix);
void omxPrintMatrix(omxMatrix *source, const char* header);

/* OMXINLINE functions and helper functions */

void setMatrixError(omxMatrix *om, int row, int col, int numrow, int numcol);
void setVectorError(int index, int numrow, int numcol);
void matrixElementError(int row, int col, int numrow, int numcol);
void vectorElementError(int index, int numrow, int numcol);

OMXINLINE static bool omxMatrixIsDirty(omxMatrix *om) { return om->cleanVersion != om->version; }
OMXINLINE static bool omxMatrixIsClean(omxMatrix *om) { return om->cleanVersion == om->version; }
OMXINLINE static int omxGetMatrixVersion(omxMatrix *om) { return om->version; }

static OMXINLINE int omxIsMatrix(omxMatrix *mat) {
    return (mat->algebra == NULL && mat->fitFunction == NULL);
}

/* BLAS Wrappers */

static OMXINLINE void omxSetMatrixElement(omxMatrix *om, int row, int col, double value) {
	if((row < 0) || (col < 0) || (row >= om->rows) || (col >= om->cols)) {
		setMatrixError(om, row + 1, col + 1, om->rows, om->cols);
		return;
	}
	int index = 0;
	if(om->colMajor) {
		index = col * om->rows + row;
	} else {
		index = row * om->cols + col;
	}
	om->data[index] = value;
}

static OMXINLINE void omxAccumulateMatrixElement(omxMatrix *om, int row, int col, double value) {
        if((row < 0) || (col < 0) || (row >= om->rows) || (col >= om->cols)) {
                setMatrixError(om, row + 1, col + 1, om->rows, om->cols);
                return;
        }
        int index = 0;
        if(om->colMajor) {
                index = col * om->rows + row;
        } else {
                index = row * om->cols + col;
        }
        om->data[index] += value;
}

static OMXINLINE double omxMatrixElement(omxMatrix *om, int row, int col) {
	int index = 0;
	if((row < 0) || (col < 0) || (row >= om->rows) || (col >= om->cols)) {
		matrixElementError(row + 1, col + 1, om->rows, om->cols);
        return (NA_REAL);
	}
	if(om->colMajor) {
		index = col * om->rows + row;
	} else {
		index = row * om->cols + col;
	}
	return om->data[index];
}

static OMXINLINE double *omxMatrixColumn(omxMatrix *om, int col) {
  if (!om->colMajor) Rf_error("omxMatrixColumn requires colMajor order");
  if (col < 0 || col >= om->cols) Rf_error(0, col, om->rows, om->cols);
  return om->data + col * om->rows;
}

static OMXINLINE void omxAccumulateVectorElement(omxMatrix *om, int index, double value) {
	if (index < 0 || index >= (om->rows * om->cols)) {
		setVectorError(index + 1, om->rows, om->cols);
		return;
	} else {
		om->data[index] += value;
    }
}

static OMXINLINE void omxSetVectorElement(omxMatrix *om, int index, double value) {
	if (index < 0 || index >= (om->rows * om->cols)) {
		setVectorError(index + 1, om->rows, om->cols);
		return;
	} else {
		om->data[index] = value;
    }
}

static OMXINLINE double omxVectorElement(omxMatrix *om, int index) {
	if (index < 0 || index >= (om->rows * om->cols)) {
		vectorElementError(index + 1, om->rows, om->cols);
        return (NA_REAL);
	} else {
		return om->data[index];
    }
}

static OMXINLINE void omxUnsafeSetVectorElement(omxMatrix *om, int index, double value) {
	om->data[index] = value;
}

static OMXINLINE double omxUnsafeVectorElement(omxMatrix *om, int index) {
	return om->data[index];
}


static OMXINLINE void omxDGEMM(unsigned short int transposeA, unsigned short int transposeB,		// result <- alpha * A %*% B + beta * C
				double alpha, omxMatrix* a, omxMatrix *b, double beta, omxMatrix* result) {
	int nrow = (transposeA?a->cols:a->rows);
	int nmid = (transposeA?a->rows:a->cols);
	int ncol = (transposeB?b->rows:b->cols);

	F77_CALL(omxunsafedgemm)((transposeA?a->minority:a->majority), (transposeB?b->minority:b->majority), 
							&(nrow), &(ncol), &(nmid),
							&alpha, a->data, &(a->leading), 
							b->data, &(b->leading),
							&beta, result->data, &(result->leading));

	if(!result->colMajor) omxToggleRowColumnMajor(result);
}

static OMXINLINE void omxDGEMV(unsigned short int transposeMat, double alpha, omxMatrix* mat,	// result <- alpha * A %*% B + beta * C
				omxMatrix* vec, double beta, omxMatrix*result) {							// where B is treated as a vector
	int onei = 1;
	int nrows = mat->rows;
	int ncols = mat->cols;
	if(OMX_DEBUG_MATRIX) {
		int nVecEl = vec->rows * vec->cols;
		// mxLog("DGEMV: %c, %d, %d, %f, 0x%x, %d, 0x%x, %d, 0x%x, %d\n", *(transposeMat?mat->minority:mat->majority), (nrows), (ncols), 
        	// alpha, mat->data, (mat->leading), vec->data, onei, beta, result->data, onei); //:::DEBUG:::
		if((transposeMat && nrows != nVecEl) || (!transposeMat && ncols != nVecEl)) {
			Rf_error("Mismatch in vector/matrix multiply: %s (%d x %d) * (%d x 1).\n", (transposeMat?"transposed":""), mat->rows, mat->cols, nVecEl); // :::DEBUG:::
		}
	}
	F77_CALL(omxunsafedgemv)((transposeMat?mat->minority:mat->majority), &(nrows), &(ncols), 
	        &alpha, mat->data, &(mat->leading), vec->data, &onei, &beta, result->data, &onei);
	if(!result->colMajor) omxToggleRowColumnMajor(result);
}

static OMXINLINE void omxDSYMV(double alpha, omxMatrix* mat,            // result <- alpha * A %*% B + beta * C
				omxMatrix* vec, double beta, omxMatrix* result) {       // only A is symmetric, and B is a vector
	char u='U';
    int onei = 1;

	if(OMX_DEBUG_MATRIX) {
		int nVecEl = vec->rows * vec->cols;
		// mxLog("DSYMV: %c, %d, %f, 0x%x, %d, 0x%x, %d, %f, 0x%x, %d\n", u, (mat->cols),alpha, mat->data, (mat->leading), 
	                    // vec->data, onei, beta, result->data, onei); //:::DEBUG:::
		if(mat->cols != nVecEl) {
			Rf_error("Mismatch in symmetric vector/matrix multiply: %s (%d x %d) * (%d x 1).\n", "symmetric", mat->rows, mat->cols, nVecEl); // :::DEBUG:::
		}
	}

    F77_CALL(dsymv)(&u, &(mat->cols), &alpha, mat->data, &(mat->leading), 
                    vec->data, &onei, &beta, result->data, &onei);

    // if(!result->colMajor) omxToggleRowColumnMajor(result);
}

static OMXINLINE void omxDSYMM(unsigned short int symmOnLeft, double alpha, omxMatrix* symmetric, 		// result <- alpha * A %*% B + beta * C
				omxMatrix *other, double beta, omxMatrix* result) {	                            // One of A or B is symmetric

	char r='R', l = 'L';
	char u='U';
	F77_CALL(dsymm)((symmOnLeft?&l:&r), &u, &(result->rows), &(result->cols),
					&alpha, symmetric->data, &(symmetric->leading),
 					other->data, &(other->leading),
					&beta, result->data, &(result->leading));

	if(!result->colMajor) omxToggleRowColumnMajor(result);
}

static OMXINLINE void omxDAXPY(double alpha, omxMatrix* lhs, omxMatrix* rhs) {              // RHS += alpha*lhs  
    // N.B.  Not fully tested.                                                              // Assumes common majority or vectordom.
    if(lhs->colMajor != rhs->colMajor) { omxToggleRowColumnMajor(rhs);}
    int len = lhs->rows * lhs->cols;
    int onei = 1;
    F77_CALL(daxpy)(&len, &alpha, lhs->data, &onei, rhs->data, &onei);

}

static OMXINLINE double omxDDOT(omxMatrix* lhs, omxMatrix* rhs) {              // returns dot product, as if they were vectors
    // N.B.  Not fully tested.                                                  // Assumes common majority or vectordom.
    if(lhs->colMajor != rhs->colMajor) { omxToggleRowColumnMajor(rhs);}
    int len = lhs->rows * lhs->cols;
    int onei = 1;
    return(F77_CALL(ddot)(&len, lhs->data, &onei, rhs->data, &onei));
}

static OMXINLINE void omxDPOTRF(omxMatrix* mat, int* info) {										// Cholesky decomposition of mat
	// TODO: Add Rf_error checking, and/or adjustments for row vs. column majority.
	// N.B. Not fully tested.
	char u = 'U'; //l = 'L'; //U for storing upper triangle
	F77_CALL(dpotrf)(&u, &(mat->rows), mat->data, &(mat->cols), info);
}
static OMXINLINE void omxDPOTRI(omxMatrix* mat, int* info) {										// Invert mat from Cholesky
	// TODO: Add Rf_error checking, and/or adjustments for row vs. column majority.
	// N.B. Not fully tested.
	char u = 'U'; //l = 'L'; // U for storing upper triangle
	F77_CALL(dpotri)(&u, &(mat->rows), mat->data, &(mat->cols), info);
}

void omxShallowInverse(FitContext *fc, int numIters, omxMatrix* A, omxMatrix* Z, omxMatrix* Ax, omxMatrix* I );

double omxMaxAbsDiff(omxMatrix *m1, omxMatrix *m2);

void checkIncreasing(omxMatrix* om, int column, int count, FitContext *fc);

void omxStandardizeCovMatrix(omxMatrix* cov, double* corList, double* weights);

void omxMatrixHorizCat(omxMatrix** matList, int numArgs, omxMatrix* result);

void omxMatrixVertCat(omxMatrix** matList, int numArgs, omxMatrix* result);

void omxMatrixTrace(omxMatrix** matList, int numArgs, omxMatrix* result);

void expm_eigen(int n, double *rz, double *out);
void logm_eigen(int n, double *rz, double *out);

inline double *omxMatrixDataColumnMajor(omxMatrix *mat)
{
	omxEnsureColumnMajor(mat);
	return mat->data;
}

struct EigenMatrixAdaptor : Eigen::Map< Eigen::MatrixXd > {
	EigenMatrixAdaptor(omxMatrix *mat) :
	  Eigen::Map< Eigen::MatrixXd >(omxMatrixDataColumnMajor(mat), mat->rows, mat->cols) {}
};

struct EigenVectorAdaptor : Eigen::Map< Eigen::VectorXd > {
	EigenVectorAdaptor(omxMatrix *mat) :
	  Eigen::Map< Eigen::VectorXd >(mat->data, mat->rows * mat->cols) {}
};

#endif /* _OMXMATRIX_H_ */
