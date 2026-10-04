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

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <sparse/general.h>
#include <errno.h>
#include <util/alloc.h>
#include <util/sort.h>

#ifdef DEBUG
double _statistics[10];
#endif

double drand(void){
  return rand()/(double) RAND_MAX;
}

double *vector_subtract_to(size_t n, double *x, double *y) {
  /* y = x-y */
  for (size_t i = 0; i < n; i++) y[i] = x[i] - y[i];
  return y;
}
double vector_product(int n, double *x, double *y){
  double res = 0;
  int i;
  for (i = 0; i < n; i++) res += x[i]*y[i];
  return res;
}

double *vector_saxpy(size_t n, double *x, double *y, double beta) {
  /* y = x+beta*y */
  for (size_t i = 0; i < n; i++) y[i] = x[i] + beta * y[i];
  return y;
}

double *vector_saxpy2(size_t n, double *x, double *y, double beta) {
  /* x = x+beta*y */
  for (size_t i = 0; i < n; i++) x[i] += beta * y[i];
  return x;
}

void vector_float_take(size_t n, float *v, size_t m, size_t *p, float **u) {
  /* take m elements v[p[i]]],i=1,...,m and oput in u */
  if (!*u) *u = gv_calloc(m, sizeof(float));

  for (size_t i = 0; i < m; i++) {
    assert(p[i] < n);
    (void)n;
    (*u)[i] = v[p[i]];
  }
  
}

/// compare two double vector values’ indices
///
/// @param s1 Index of the first value
/// @param s2 Index of the second value
/// @param values Values themselves
/// @return Comparison result
static int comp_ascend(const void *s1, const void *s2, void *values) {
  const size_t *const ss1 = s1;
  const size_t *const ss2 = s2;
  const double *const v = values;

  if (v[*ss1] > v[*ss2]) {
    return 1;
  } else if (v[*ss1] < v[*ss2]) {
    return -1;
  }
  return 0;
}

size_t *vector_ordering(size_t n, double *v) {
  size_t *const p = gv_calloc(n, sizeof(size_t));

  for (size_t i = 0; i < n; i++) {
    p[i] = i;
  }

  gv_sort(p, n, sizeof(p[0]), comp_ascend, v);
  return p;
}

double distance_cropped(double *x, int dim, int i, int j){
  double dist = distance(x, dim, i, j);
  return fmax(dist, MINDIST);
}

double distance(double *x, int dim, int i, int j){
  int k;
  double dist = 0.;
  for (k = 0; k < dim; k++) dist += (x[i*dim+k] - x[j*dim + k])*(x[i*dim+k] - x[j*dim + k]);
  dist = sqrt(dist);
  return dist;
}

double point_distance(double *p1, double *p2, int dim){
  int i;
  double dist;
  dist = 0;
  for (i = 0; i < dim; i++) dist += (p1[i] - p2[i])*(p1[i] - p2[i]);
  return sqrt(dist);
}

char *strip_dir(char *s){
  bool first = true;
  if (!s) return s;
  for (size_t i = strlen(s); ; i--) {
    if (first && s[i] == '.') {/* get rid of .mtx */
      s[i] = '\0';
      first = false;
    }
    if (s[i] == '/') return &s[i+1];
    if (i == 0) {
      break;
    }
  }
  return s;
}
