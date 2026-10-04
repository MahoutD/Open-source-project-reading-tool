/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * which accompanies this distribution, and is available at
 * https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <neatogen/neato.h>

#include <neatogen/sparsegraph.h>
#include <stddef.h>

#ifdef DIGCOLA
#ifdef IPSEPCOLA
    typedef struct cluster_data {
	size_t nvars; ///< total count of vars in clusters
        size_t nclusters; ///< number of clusters
        size_t *clustersizes; ///< number of vars in each cluster
        size_t **clusters;   ///< list of var indices for constituents of each c
	size_t ntoplevel; ///< number of nodes not in any cluster
	size_t *toplevel; ///< array of nodes not in any cluster
	boxf *bb;	   /* bounding box of each cluster */
    } cluster_data;
#endif
#endif


#ifdef __cplusplus
}
#endif
