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

#include <cgraph.h>
#include <sparse/SparseMatrix.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  COLOR_SCHEME_NONE,
  COLOR_SCHEME_PASTEL = 1,
  COLOR_SCHEME_BLUE_YELLOW,
  COLOR_SCHEME_WHITE_RED,
  COLOR_SCHEME_GREY_RED,
  COLOR_SCHEME_PRIMARY,
  COLOR_SCHEME_SEQUENTIAL_SINGLEHUE_RED,
  COLOR_SCHEME_ADAM,
  COLOR_SCHEME_ADAM_BLEND,
  COLOR_SCHEME_SEQUENTIAL_SINGLEHUE_RED_LIGHTER,
  COLOR_SCHEME_GREY
};
void initDotIO(Agraph_t *g);

void setDotNodeID(Agnode_t *n, int v);
int getDotNodeID(Agnode_t *n);

void attach_edge_colors(Agraph_t *g, size_t dim, double *colors);

SparseMatrix SparseMatrix_import_dot(Agraph_t *g, double **x, int format);
SparseMatrix Import_coord_clusters_from_dot(
    Agraph_t *g, int maxcluster, int *nn, double **label_sizes, double **x,
    int **clusters, float **rgb_r, float **rgb_g, float **rgb_b,
    int default_color_scheme, int clustering_scheme, int useClusters);

void Dot_SetClusterColor(Agraph_t *g, float *rgb_r, float *rgb_g, float *rgb_b,
                         int *clustering);
void attached_clustering(Agraph_t *g, int maxcluster, int clustering_scheme);

int Import_dot_splines(Agraph_t *g, int *ne, char ***xsplines);

/// sentinel values for cluster grouping
///
/// `gvmap` stores 1-based indices for “which cluster does this node belong to?”
/// in its grouping arrays. These values are stored for nodes that have a
/// different treatment. So their values are arbitrary but need to be ≤ 0.
enum {
  INVALID_GROUP = 0, ///< group was never assigned
  NO_GROUP = -1,     ///< inherited the default (invalid) group
  GRP_RANDOM = -2,   ///< randomize assignment of a node
  GRP_BBOX = -3,     ///< last 4 randomized points that form a bounding box
};

#ifdef __cplusplus
}
#endif
