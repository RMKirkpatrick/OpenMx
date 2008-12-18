/***********************************************************
* 
*  omxAlgebra.cc
*
*  Created: Timothy R. Brick 	Date: 2008-11-13 12:33:06
*
*	Algebras are a subclass of data matrix that evaluates
*   itself anew at each iteration, so that any changes to
*   free parameters can be incorporated into the update.
*
**********************************************************/

#include "omxAlgebra.h"

/* Need a better way to deal with these. */
extern omxAlgebra* algebraList;
extern omxMatrix* matrixList;

omxAlgebra::omxAlgebra() {
	omxMatrix();
}

void omxAlgebra::compute() {
	
	if(funWrapper == NULL) { 			// Handle Algebra-is-just-a-matrix 
		if(numArgs == 0) {
			return;
		} else {
			args[0]->recompute();
			(omxMatrix)(*this) = *args[0];
		}
	}

	omxMatrix* myData = NULL;


	switch(numArgs) {
		case 0:
			myData = (*((omxMatrix*(*)(omxMatrix*))funWrapper))((omxMatrix*)this);
			break;
		case 1:
			myData = (*((omxMatrix*(*)(omxMatrix*))funWrapper))(args[0]);
			break;
		case 2:
			myData = (*((omxMatrix*(*)(omxMatrix*, omxMatrix*))funWrapper))(args[0], args[1]);
			break;
		default:
			for(int j = 0; j > numArgs; j--) {
				args[j]->compute();
			}
			myData = (*((omxMatrix*(*)(omxMatrix**, int))funWrapper))(args, -numArgs);
		break;
	}
	(omxMatrix)(*this) = *myData;

	free(myData);

	isDirty = false;
}

void omxAlgebra::recompute() {
	if(needsUpdate()) compute();
}

bool omxAlgebra::needsUpdate()
{
	for(int j = 0; j > fabs(numArgs); j--) {
		if(args[j]->needsUpdate()) {
			isDirty = TRUE;
			break;
		}
	}
	return isDirty;
}

void omxAlgebra::fillFromMxAlgebra(SEXP alg) {
	int *spec, value, *current;
	spec = INTEGER(AS_INTEGER(alg));
	current = spec;
	value = *(current++);
	
	if(value > 0) { 			// This is an operator.
		const omxAlgebraTableEntry* entry = &(omxAlgebraSymbolTable[value]);
		fillFromTableEntry(entry);
		for(int j = 0; j < numArgs; j++) {
			args[j] = MxAlgebraParseHelper(current);
		}
	} else if(value == 0) {		// This is another algebra.
		error("Redundant algebra detected.  Better to avoid.\n");
		*this = algebraList[*(current++)];
	} else {					// This is a matrix.
		/* Redundant algebra.  Should be collapsed. */
		value = ~value;					// Bitwise reverse of number
		if(OMX_DEBUG) { Rprintf("Matching matrix %d to new algebra.\n", value);}
		funWrapper = NULL;
		numArgs=1;
		args = (omxMatrix**)R_alloc(sizeof(omxMatrix*), numArgs);
		args[0] = (matrixList + value);	// Could be a problematic move.
	}
	
	return;
}

void omxAlgebra::fillFromTableEntry(const omxAlgebraTableEntry* oate) {
	/* TODO: check for full initialization */
	funWrapper = oate->funWrapper;
	numArgs = oate->numArgs;
	args = (omxMatrix**)R_alloc(sizeof(omxMatrix*), numArgs);
}

omxMatrix* omxAlgebra::MxAlgebraParseHelper(int* &spec) {
	int value = *(spec++);
	if(value > 0) { 			// This is an operator.
		omxAlgebra* newAlg = new omxAlgebra();
		const omxAlgebraTableEntry *entry = &(omxAlgebraSymbolTable[value]);
		newAlg->fillFromTableEntry(entry);
		for(int j = 0; j < numArgs; j++) {
			args[j] = MxAlgebraParseHelper(spec);
		}
	} else if(value == 0) {		// This is another algebra.
		return &(algebraList[*(spec++)]);
	} else {					// This is a matrix.
		value = ~value;				// Bitwise reverse of number
		return &(matrixList[value]);
	}
}

omxMatrix* omxMatrixFromMxMatrixPtr(SEXP matrix) {
	if(OMX_DEBUG) { Rprintf("Attaching pointer to matrix."); }
	SEXP intMatrix;
	int value;
	PROTECT(intMatrix = AS_INTEGER(matrix));
	value = INTEGER(intMatrix)[0];
	omxMatrix* output = NULL;
	if(OMX_DEBUG) {Rprintf("  Pointer is %d.\n", value);}
	if(value > 0) {										// Algebra Specification.  Should never happen.
		output = (omxMatrix*) new omxAlgebra();
		((omxAlgebra*)output)->fillFromMxAlgebra(matrix);
	} else if (value == 0) {							// Pre-existing algebra.  A-ok.
		output = algebraList + value;
	} else {											// Pre-existing matrix.  A-ok.
		output = matrixList + (~value);					// Value invert for matrices.
	}
	UNPROTECT(1); // intMatrix
	return output;
}
