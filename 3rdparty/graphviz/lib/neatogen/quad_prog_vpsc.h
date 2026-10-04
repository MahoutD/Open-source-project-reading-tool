/**
 * Authors:
 *   Tim Dwyer <tgdwyer@gmail.com>
 *
 * Copyright (C) 2005 Authors
 *
 * This version is released under the CPL (Common Public License) with
 * the Graphviz distribution.
 * A version is also available under the LGPL as part of the Adaptagrams
 * project: http://sourceforge.net/projects/adaptagrams.  
 * If you make improvements or bug fixes to this code it would be much
 * appreciated if you could also contribute those changes back to the
 * Adaptagrams repository.
 */

/**********************************************************
*      Written by Tim Dwyer for the graphviz package      *
*                  https://graphviz.org                   *
*                                                         *
**********************************************************/

#pragma once

#include <util/api.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DIGCOLA

#include <neatogen/defs.h>
#include <neatogen/digcola.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct CMajEnvVPSC {
	float **A;
	size_t nv; ///< number of actual vars
	size_t nldv; ///< number of dummy nodes included in lap matrix
	size_t ndv; ///< number of dummy nodes not included in lap matrix
	Variable **vs;
	size_t m; ///< total number of constraints for next iteration
	size_t gm; ///< number of global constraints
	Constraint **cs;
	/* global constraints are persistent throughout optimisation process */
	Constraint **gcs;
	VPSC *vpsc;
	float *fArray1; /* utility arrays - reusable memory */
	float *fArray2;
	float *fArray3;
} CMajEnvVPSC;

PRIVATE CMajEnvVPSC* initCMajVPSC(int n, float *packedMat, vtx_data* graph, ipsep_options *opt, int diredges);

PRIVATE int constrained_majorization_vpsc(CMajEnvVPSC*, float*, float*, int);

PRIVATE void deleteCMajEnvVPSC(CMajEnvVPSC *e);
PRIVATE void generateNonoverlapConstraints(
        CMajEnvVPSC* e,
        float nsizeScale,
        float** coords,
        int k,
	bool transitiveClosure,
	ipsep_options* opt
);

PRIVATE void removeoverlaps(int,float**,ipsep_options*);

typedef struct {
	int *nodes;
	int num_nodes;
} DigColaLevel;

PRIVATE int get_num_digcola_constraints(DigColaLevel *levels,
                                        size_t num_levels);

#endif 

#ifdef __cplusplus
}
#endif
