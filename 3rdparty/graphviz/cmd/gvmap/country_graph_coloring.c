/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * which accompanies this distribution, and is available at
 * https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include "config.h"

#define STANDALONE
#include "country_graph_coloring.h"
#include <math.h>
#include "power.h"
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <util/gv_math.h>
#include <util/prisize_t.h>

static size_t zabs(size_t a, size_t b) {
  if (a > b)
    return a - b;
  return b - a;
}

static size_t get_local_12_norm(size_t n, size_t i, const int *ia,
                                const int *ja, const size_t *p) {
  size_t norm = n;
  for (int j = ia[i]; j < ia[i+1]; j++){
    if (ja[j] >= 0 && (size_t)ja[j] == i) continue;
    norm = zmin(norm, zabs(p[i], p[ja[j]]));
  }
  return norm;
}

static void get_12_norm(size_t n, const int *ia, const int *ja, const size_t *p,
                        size_t *norm) {
  /* norm[0] := antibandwidth
     norm[1] := (\sum_{i\in V} (Min_{{j,i}\in E} |p[i] - p[j]|)/|V|
  */
  norm[0] = n; norm[1] = 0;
  for (size_t i = 0; i < n; i++){
    size_t tmp = n;
    for (int j = ia[i]; j < ia[i+1]; j++){
      if (ja[j] >= 0 && (size_t)ja[j] == i) continue;
      norm[0] = zmin(norm[0], zabs(p[i], p[ja[j]]));
      tmp = zmin(tmp, zabs(p[i], p[ja[j]]));
    }
    norm[1] += tmp;
  }
  norm[1] /= n;
}

void improve_antibandwidth_by_swapping(SparseMatrix A, size_t *p) {
  int cnt = 1, *ia = A->ia, *ja = A->ja;
  const size_t n = A->m;
  size_t norm1[2];
  clock_t start = clock();
  FILE *fp = NULL;
  
  if (Verbose){
    fprintf(stderr,"saving timing vs antiband data to timing_greedy\n");
    fp = fopen("timing_greedy","w");
  }
  assert(SparseMatrix_is_symmetric(A, true));
  for (bool improved = true; improved; ) {
    improved = false;
    for (size_t i = 0; i < n; i++) {
      norm1[0] = get_local_12_norm(n, i, ia, ja, p);
      for (size_t j = 0; j < n; j++) {
	if (j == i) continue;
	const size_t norm2 = get_local_12_norm(n, j, ia, ja, p);
	const size_t pi = p[i];
	const size_t pj = p[j];
	p[i] = pj;
	p[j] = pi;
	const size_t norm11 = get_local_12_norm(n, i, ia, ja, p);
	const size_t norm22 = get_local_12_norm(n, j, ia, ja, p);
	if (zmin(norm11, norm22) > zmin(norm1[0], norm2)){
	  improved = true;
	  norm1[0] = norm11;
	  continue;
	}
	p[i] = pi;
	p[j] = pj;
      }
      if (i%100 == 0 && Verbose) {
	get_12_norm(n, ia, ja, p, norm1);
	fprintf(fp, "%f %" PRISIZE_T " %" PRISIZE_T "\n", (double)(clock() - start) / CLOCKS_PER_SEC,
	        norm1[0], norm1[1]);
      }
    }
    if (Verbose) {
      get_12_norm(n, ia, ja, p, norm1);
      fprintf(stderr, "[%d] aband = %" PRISIZE_T ", aband_avg = %" PRISIZE_T "\n", cnt++, norm1[0], norm1[1]);
      fprintf(fp,"%f %" PRISIZE_T " %" PRISIZE_T "\n", (double)(clock() - start) / CLOCKS_PER_SEC,
              norm1[0], norm1[1]);
    }
  }
  if (fp != NULL) {
    fclose(fp);
  }
}
  
size_t *country_graph_coloring(int seed, SparseMatrix A) {
  const size_t n = A->m;

  clock_t start = clock();
  assert(A->m == (size_t)A->n);
  SparseMatrix A2 = SparseMatrix_symmetrize(A, true);
  const int *const ia = A2->ia;
  const int *const ja = A2->ja;

  /* Laplacian */
  SparseMatrix L = SparseMatrix_new(n, (int)n, 1, MATRIX_TYPE_REAL, FORMAT_COORD);
  for (size_t i = 0; i < n; i++){
    double nrow = 0.;
    for (int j = ia[i]; j < ia[i+1]; j++){
      const int jj = ja[j];
      if (jj != (int)i){
	nrow ++;
	L = SparseMatrix_coordinate_form_add_entry(L, (int)i, jj, &(double){-1});
      }
    }
    L = SparseMatrix_coordinate_form_add_entry(L, (int)i, (int)i, &nrow);
  }
  {
    SparseMatrix new = SparseMatrix_from_coordinate_format(L);
    SparseMatrix_delete(L);
    L = new;
  }

  /* largest eigen vector */
  double *v = power_method(L, L->n, seed);

  size_t *const p = vector_ordering(n, v);
  free(v);
  if (Verbose)
    fprintf(stderr, "cpu time for spectral ordering (before greedy) = %f\n",
            ((double)(clock() - start)) / CLOCKS_PER_SEC);

  clock_t start2 = clock();
  /* swapping */
  improve_antibandwidth_by_swapping(A2, p);
  if (Verbose) {
    fprintf(stderr, "cpu time for greedy refinement = %f\n",
            ((double)(clock() - start2)) / CLOCKS_PER_SEC);

    fprintf(stderr, "cpu time for spectral + greedy = %f\n",
            ((double)(clock() - start)) / CLOCKS_PER_SEC);

  }

  if (A2 != A) SparseMatrix_delete(A2);
  SparseMatrix_delete(L);
  return p;
}
