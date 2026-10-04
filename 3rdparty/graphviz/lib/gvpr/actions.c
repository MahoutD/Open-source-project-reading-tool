/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * which accompanies this distribution, and is available at
 * https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

/*
 *  Code for main functions in gpr
 */

#include "config.h"

#include <ast/ast.h>
#include <ast/error.h>
#include <cgraph/agstrcanon.h>
#include <gvpr/actions.h>
#include <gvpr/compile.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util/agxbuf.h>
#include <util/alloc.h>
#include <util/gv_ctype.h>
#include <util/list.h>
#include <util/strcasecmp.h>
#include <util/unreachable.h>
#include <util/unused.h>

static const char *kinds(Agobj_t *obj) {
  if (AGTYPE(obj) == AGRAPH)
    return "graph";
  if (AGTYPE(obj) == AGNODE)
    return "node";
  return "edge";
}

/* Return common root if objects belong to same root graph.
 * NULL otherwise
 */
Agraph_t *sameG(void *obj1, void *obj2, char *fn, char *msg) {
  Agraph_t *const root = agroot(agraphof(obj1));
  if (root != agroot(agraphof(obj2))) {
    if (msg)
      error(ERROR_WARNING, "%s in %s() belong to different graphs", msg, fn);
    else
      error(ERROR_WARNING, "%s and %s in %s() belong to different graphs",
            kinds(obj1), kinds(obj2), fn);
    return 0;
  }
  return root;
}

/// return index of leftmost string s2 in string s1, or -1
int indexOf(char *s1, char *s2) {
  char *s = strstr(s1, s2);
  return s == NULL ? -1 : (int)(s - s1);
}

/// return index of rightmost string s2 in string s1, or -1
long rindexOf(char *s1, char *s2) {
  char c1 = *s2;
  char *p;
  size_t len1 = strlen(s1);
  size_t len2 = strlen(s2);

  if (c1 == '\0') {
    assert(len1 <= LONG_MAX);
    return (long)len1;
  }
  if (len2 > len1)
    return -1;
  p = s1 + (len1 - len2);
  while (true) {
    if (strncmp(p, s2, len2) == 0)
      return p - s1;
    if (p == s1)
      break;
    p--;
  }
  return -1;
}

/// return index of pattern pat in string str, or SIZE_MAX
size_t match(char *str, char *pat) {
  size_t sub[2];

  if (strgrpmatch(str, pat, sub, 1, 0)) {
    return (sub[0]);
  } else
    return SIZE_MAX;
}

/* Copy attributes from src to tgt. Overrides currently
 * defined values.
 * NOTE: We do not assume src and tgt have the same kind.
 */
int copyAttr(Agobj_t *src, Agobj_t *tgt) {
  const int skind = AGTYPE(src);
  const int tkind = AGTYPE(tgt);

  Agraph_t *const srcg = agraphof(src);
  Agraph_t *const tgtg = agraphof(tgt);
  for (Agsym_t *sym = NULL; (sym = agnxtattr(srcg, skind, sym));) {
    Agsym_t *tsym = agattrsym(tgt, sym->name);
    if (!tsym)
      tsym = agattr_text(tgtg, tkind, sym->name, sym->defval);
    char *val = agxget(src, sym);
    if (aghtmlstr(val)) {
      val = agstrdup_html(tgtg, val);
      agxset(tgt, tsym, val);
      agstrfree(tgtg, val, true);
    } else
      agxset(tgt, tsym, val);
  }
  return 0;
}

/* Create new object of type AGTYPE(obj) with all of its
 * attributes.
 * If obj is an edge, only create end nodes if necessary.
 * If obj is a graph, if g is null, create a top-level
 * graph. Otherwise, create a subgraph of g.
 * Assume obj != NULL.
 */
Agobj_t *copy(Agraph_t *g, Agobj_t *obj) {
  Agobj_t *nobj = 0;
  Agedge_t *e;
  Agnode_t *h;
  Agnode_t *t;
  int kind = AGTYPE(obj);
  char *name;

  if (kind != AGRAPH && !g) {
    exerror("NULL graph with non-graph object in copy()");
    return 0;
  }

  switch (kind) {
  case AGNODE:
    name = agnameof(obj);
    nobj = (Agobj_t *)openNode(g, name);
    break;
  case AGRAPH:
    name = agnameof(obj);
    if (g)
      nobj = (Agobj_t *)openSubg(g, name);
    else
      nobj = (Agobj_t *)openG(name, ((Agraph_t *)obj)->desc);
    break;
  case AGINEDGE:
  case AGOUTEDGE:
    e = (Agedge_t *)obj;
    t = openNode(g, agnameof(agtail(e)));
    h = openNode(g, agnameof(aghead(e)));
    name = agnameof(AGMKOUT(e));
    nobj = (Agobj_t *)openEdge(g, t, h, name);
    break;
  default:
    UNREACHABLE();
  }
  if (nobj)
    copyAttr(obj, nobj);

  return nobj;
}

typedef struct {
  Agedge_t *key;
  Agedge_t *val;
} edgepair_t;

typedef LIST(edgepair_t) emap_t;

static int cmppair1(const void *key, const void *candidate) {
  const Agedge_t *const k = key;
  const edgepair_t *const c = candidate;
  if ((uintptr_t)k > (uintptr_t)c->key)
    return 1;
  if ((uintptr_t)k < (uintptr_t)c->key)
    return -1;
  return 0;
}

static Agedge_t *mapEdge(emap_t *emap, Agedge_t *e) {
  edgepair_t *ep =
      bsearch(e, emap->base, LIST_SIZE(emap), sizeof(emap->base[0]), cmppair1);
  if (ep)
    return ep->val;
  else
    return NULL;
}

/// clone subgraph sg in tgt
static Agraph_t *cloneSubg(Agraph_t *tgt, Agraph_t *g, emap_t *emap) {
  Agraph_t *ng;
  Agraph_t *sg;
  Agnode_t *t;
  Agnode_t *newt;
  Agedge_t *e;
  Agedge_t *newe;
  char *name;

  ng = (Agraph_t *)copy(tgt, OBJ(g));
  if (!ng)
    return NULL;
  for (t = agfstnode(g); t; t = agnxtnode(g, t)) {
    newt = agnode(tgt, agnameof(t), 0);
    if (!newt) {
      exerror("node %s not found in cloned graph %s", agnameof(t),
              agnameof(tgt));
      return NULL;
    } else
      agsubnode(ng, newt, 1);
  }
  for (t = agfstnode(g); t; t = agnxtnode(g, t)) {
    for (e = agfstout(g, t); e; e = agnxtout(g, e)) {
      newe = mapEdge(emap, e);
      if (!newe) {
        name = agnameof(AGMKOUT(e));
        if (name)
          exerror("edge (%s,%s)[%s] not found in cloned graph %s",
                  agnameof(agtail(e)), agnameof(aghead(e)), name,
                  agnameof(tgt));
        else
          exerror("edge (%s,%s) not found in cloned graph %s",
                  agnameof(agtail(e)), agnameof(aghead(e)), agnameof(tgt));
        return NULL;
      } else
        agsubedge(ng, newe, 1);
    }
  }
  for (sg = agfstsubg(g); sg; sg = agnxtsubg(sg)) {
    if (!cloneSubg(ng, sg, emap)) {
      exerror("error cloning subgraph %s from graph %s", agnameof(sg),
              agnameof(g));
      return NULL;
    }
  }
  return ng;
}

static int cmppair(const void *k1, const void *k2) {
  const edgepair_t *const key1 = k1;
  return cmppair1(key1->key, k2);
}

/// clone node, edge and subgraph structure from src to tgt
static void cloneGraph(Agraph_t *tgt, Agraph_t *src) {
  Agedge_t *e;
  Agedge_t *ne;
  Agnode_t *t;
  Agraph_t *sg;
  char *name;
  const size_t n_edges = (size_t)agnedges(src);
  emap_t emap = {0};
  LIST_RESERVE(&emap, n_edges);

  for (t = agfstnode(src); t; t = agnxtnode(src, t)) {
    if (!copy(tgt, &t->base)) {
      exerror("error cloning node %s from graph %s", agnameof(t),
              agnameof(src));
    }
  }
  for (t = agfstnode(src); t; t = agnxtnode(src, t)) {
    for (e = agfstout(src, t); e; e = agnxtout(src, e)) {
      if (!(ne = (Agedge_t *)copy(tgt, &e->base))) {
        name = agnameof(AGMKOUT(e));
        if (name)
          exerror("error cloning edge (%s,%s)[%s] from graph %s",
                  agnameof(agtail(e)), agnameof(aghead(e)), name,
                  agnameof(src));
        else
          exerror("error cloning edge (%s,%s) from graph %s",
                  agnameof(agtail(e)), agnameof(aghead(e)), agnameof(src));
        goto done;
      }
      LIST_APPEND(&emap, (edgepair_t){.key = e, .val = ne});
    }
  }
  LIST_SORT(&emap, cmppair);
  for (sg = agfstsubg(src); sg; sg = agnxtsubg(sg)) {
    if (!cloneSubg(tgt, sg, &emap)) {
      exerror("error cloning subgraph %s from graph %s", agnameof(sg),
              agnameof(src));
    }
  }

done:
  LIST_FREE(&emap);
}

Agraph_t *cloneG(Agraph_t *g, char *name) {
  if (!name || *name == '\0')
    name = agnameof(g);
  Agraph_t *const ng = openG(name, g->desc);
  if (ng) {
    copyAttr(&g->base, &ng->base);
    cloneGraph(ng, g);
  }
  return ng;
}

/* Create new object of type AGTYPE(obj) with all of its
 * attributes and substructure.
 * If obj is an edge, end nodes are cloned if necessary.
 * If obj is a graph, if g is null, create a clone top-level
 * graph. Otherwise, create a clone subgraph of g.
 * Assume obj != NULL.
 */
Agobj_t *cloneO(Agraph_t *g, Agobj_t *obj) {
  Agobj_t *nobj = 0;
  Agedge_t *e;
  Agnode_t *h;
  Agnode_t *t;
  int kind = AGTYPE(obj);
  char *name;

  if (kind != AGRAPH && !g) {
    exerror("NULL graph with non-graph object in clone()");
    return 0;
  }

  switch (kind) {
  case AGNODE: /* same as copy node */
    name = agnameof(obj);
    nobj = (Agobj_t *)openNode(g, name);
    if (nobj)
      copyAttr(obj, nobj);
    break;
  case AGRAPH:
    name = agnameof(obj);
    if (g)
      nobj = (Agobj_t *)openSubg(g, name);
    else
      nobj = (Agobj_t *)openG(name, ((Agraph_t *)obj)->desc);
    if (nobj) {
      copyAttr(obj, nobj);
      cloneGraph((Agraph_t *)nobj, (Agraph_t *)obj);
    }
    break;
  case AGINEDGE:
  case AGOUTEDGE:
    e = (Agedge_t *)obj;
    t = (Agnode_t *)cloneO(g, OBJ(agtail(e)));
    h = (Agnode_t *)cloneO(g, OBJ(aghead(e)));
    name = agnameof(AGMKOUT(e));
    nobj = (Agobj_t *)openEdge(g, t, h, name);
    if (nobj)
      copyAttr(obj, nobj);
    break;
  default:
    UNREACHABLE();
  }

  return nobj;
}

#define CCMARKED(n) (((nData(n))->iu.integer) & 2)
#define CCMARK(n) (((nData(n))->iu.integer) |= 2)
#define CCUNMARK(n) (((nData(n))->iu.integer) &= ~2)

static void cc_dfs(Agraph_t *g, Agraph_t *comp, Agnode_t *n) {
  Agedge_t *e;
  Agnode_t *other;

  CCMARK(n);
  agidnode(comp, AGID(n), 1);
  for (e = agfstedge(g, n); e; e = agnxtedge(g, e, n)) {
    if (agtail(e) == n)
      other = aghead(e);
    else
      other = agtail(e);
    if (!CCMARKED(other))
      cc_dfs(g, comp, other);
  }
}

/// return connected component of node
Agraph_t *compOf(Agraph_t *g, Agnode_t *n) {
  Agraph_t *cg;
  Agnode_t *np;
  static int id;
  char name[64];

  if (!(n = agidnode(g, AGID(n), 0)))
    return 0; /* n not in g */
  for (np = agfstnode(g); np; np = agnxtnode(g, np))
    CCUNMARK(np);

  snprintf(name, sizeof(name), "_cc_%d", id++);
  cg = openSubg(g, name);
  cc_dfs(g, cg, n);

  return cg;
}

/* Return edge, if any, between t and h with given key.
 * Edge is in g.
 */
Agedge_t *isEdge(Agraph_t *g, Agnode_t *t, Agnode_t *h, char *key) {
  Agraph_t *root;

  root = sameG(t, h, "isEdge", "tail and head node");
  if (!root)
    return 0;
  if (g) {
    if (root != agroot(g))
      return 0;
  } else
    g = root;

  return agedge(g, t, h, key, 0);
}

/* Insert node n into subgraph g.
 * Return image of n
 */
Agnode_t *addNode(Agraph_t *gp, Agnode_t *np, int doAdd) {
  if (!sameG(gp, np, "addNode", 0))
    return 0;
  return agsubnode(gp, np, doAdd);
}

/* Insert edge e into subgraph g.
 * Return image of e
 */
Agedge_t *addEdge(Agraph_t *gp, Agedge_t *ep, int doAdd) {
  if (!sameG(gp, ep, "addEdge", 0))
    return 0;
  return agsubedge(gp, ep, doAdd);
}

/* Set lock so that graph g will not be deleted.
 * g must be a root graph.
 * If v > 0, set lock
 * If v = 0, unset lock and delete graph is necessary.
 * If v < 0, no op
 * Always return previous lock state.
 * Return -1 on error.
 */
int lockGraph(Agraph_t *g, int v) {
  gdata *data;

  if (g != agroot(g)) {
    error(ERROR_WARNING, "Graph argument to lock() is not a root graph");
    return -1;
  }
  data = gData(g);
  const int oldv = data->lock.locked;
  if (v > 0)
    data->lock.locked = true;
  else if (v == 0 && oldv) {
    if (data->lock.zombie)
      agclose(g);
    else
      data->lock = (gvlock_t){0};
  }
  return oldv;
}

/* Remove obj from g.
 * obj may belong to a subgraph of g, so we first must map
 * obj to its version in g.
 * If g is null, remove object from root graph.
 * If obj is a (sub)graph, close it. The g parameter is unused.
 * Return 0 on success, non-zero on failure.
 */
int deleteObj(Agraph_t *g, Agobj_t *obj) {
  gdata *data;
  if (AGTYPE(obj) == AGRAPH) {
    g = (Agraph_t *)obj;
    if (g != agroot(g))
      return agclose(g);
    data = gData(g);
    if (data->lock.locked) {
      error(ERROR_WARNING, "Cannot delete locked graph %s", agnameof(g));
      data->lock.zombie = true;
      return -1;
    } else
      return agclose(g);
  }

  /* node or edge */
  if (!g)
    g = agroot(agraphof(obj));
  if (obj)
    return agdelete(g, obj);
  else
    return -1;
}

/* If the graph is passed in from a library, its output discipline
 * might not use stdio. In this case, we push a stdio discipline on
 * the graph, write it, and then pop it off.
 */
int sfioWrite(Agraph_t *g, FILE *fp) {
  int rv;

  Agiodisc_t *saveio = g->clos->disc.io;
  g->clos->disc.io = &AgIoDisc;
  rv = agwrite(g, fp);
  g->clos->disc.io = saveio;
  return rv;
}

/* Write graph into file f.
 * Return 0 on success
 */
int writeFile(Agraph_t *g, char *f) {
  int rv;

  if (!f) {
    exerror("NULL string passed to writeG");
    return 1;
  }
  FILE *fp = fopen(f, "w");
  if (!fp) {
    exwarn("Could not open %s for writing in writeG", f);
    return 1;
  }
  rv = sfioWrite(g, fp);
  fclose(fp);
  return rv;
}

/* Read graph from file f.
 * Return 0 on failure
 */
Agraph_t *readFile(char *f) {
  Agraph_t *gp;

  if (!f) {
    exerror("NULL string passed to readG");
    return 0;
  }
  FILE *fp = fopen(f, "r");
  if (!fp) {
    exwarn("Could not open %s for reading in readG", f);
    return 0;
  }
  gp = readG(fp);
  fclose(fp);

  return gp;
}

int fwriteFile(Expr_t *ex, Agraph_t *g, long long fd) {
  FILE *sp;

  if (fd < 0 || fd >= (long long)elementsof(ex->file) || !(sp = ex->file[fd])) {
    exerror("fwriteG: %lld: invalid descriptor", fd);
    return 0;
  }
  return sfioWrite(g, sp);
}

Agraph_t *freadFile(Expr_t *ex, long long fd) {
  FILE *sp;

  if (fd < 0 || fd >= (long long)elementsof(ex->file) || !(sp = ex->file[fd])) {
    exerror("freadG: %lld: invalid descriptor", fd);
    return 0;
  }
  return readG(sp);
}

int openFile(Expr_t *ex, const char *fname, const char *mode) {
  int idx;

  /* find open index */
  for (idx = 3; (size_t)idx < elementsof(ex->file); idx++)
    if (!ex->file[idx])
      break;
  if ((size_t)idx == elementsof(ex->file)) {
    exerror("openF: no available descriptors");
    return -1;
  }
  ex->file[idx] = fopen(fname, mode);
  if (ex->file[idx])
    return idx;
  else
    return -1;
}

int closeFile(Expr_t *ex, long long fd) {
  int rv;

  if (0 <= fd && fd <= 2) {
    exerror("closeF: cannot close standard stream %lld", fd);
    return -1;
  }
  if (fd < 0 || fd >= (long long)elementsof(ex->file)) {
    exerror("closeG: %lld: invalid descriptor", fd);
    return -1;
  }
  if (!ex->file[fd]) {
    exerror("closeF: stream %lld not open", fd);
    return -1;
  }
  rv = fclose(ex->file[fd]);
  if (!rv)
    ex->file[fd] = 0;
  return rv;
}

/*
 * Read single line from stream.
 * Return "" on EOF.
 */
char *readLine(Expr_t *ex, long long fd) {
  FILE *sp;
  int c;
  char *line;

  if (fd < 0 || fd >= (long long)elementsof(ex->file) || !(sp = ex->file[fd])) {
    exerror("readL: %lld: invalid descriptor", fd);
    return "";
  }

  agxbuf tmps = {0};
  while ((c = getc(sp)) > 0 && c != '\n')
    agxbputc(&tmps, (char)c);
  if (c == '\n')
    agxbputc(&tmps, (char)c);
  line = exstring(ex, agxbuse(&tmps));
  agxbfree(&tmps);
  return line;
}

/// lexicographic ordering of objects
int compare(Agobj_t *l, Agobj_t *r) {
  char lkind, rkind;
  if (l == NULL) {
    if (r == NULL)
      return 0;
    else
      return -1;
  } else if (r == NULL) {
    return 1;
  }
  if (AGID(l) < AGID(r))
    return -1;
  else if (AGID(l) > AGID(r))
    return 1;
  lkind = AGTYPE(l);
  rkind = AGTYPE(r);
  if (lkind == 3)
    lkind = 2;
  if (rkind == 3)
    rkind = 2;
  if (lkind == rkind)
    return 0;
  else if (lkind < rkind)
    return -1;
  else
    return 1;
}

/// convert characters to lowercase
char *toLower(Expr_t *pgm, char *src) {

  const size_t len = strlen(src);
  char *dst = exstralloc(pgm, len + 1);
  if (dst == NULL) {
    return NULL;
  }

  for (size_t i = 0; i < len; ++i) {
    dst[i] = gv_tolower(src[i]);
  }

  dst[len] = '\0';
  return dst;
}

/// convert characters to uppercase
char *toUpper(Expr_t *pgm, char *src) {

  const size_t len = strlen(src);
  char *dst = exstralloc(pgm, len + 1);
  if (dst == NULL) {
    return NULL;
  }

  for (size_t i = 0; i < len; ++i) {
    dst[i] = gv_toupper(src[i]);
  }

  dst[len] = '\0';
  return dst;
}

/// create a string marked as HTML
char *toHtml(Agraph_t *g, char *arg) { return agstrdup_html(g, arg); }

/// canonicalize a string for printing
char *canon(Expr_t *pgm, char *arg) {
  char *const buffer = exstralloc(pgm, agstrcanon_bytes(arg));
  return agstrcanon(arg, buffer);
}

#undef S

// force the upcoming ../common/colxlate.c functions to not be exported
#ifdef COLORPROCS_API
#undef COLORPROCS_API
#endif
#ifdef GVDLL
#undef GVDLL
#endif
#ifdef GVC_EXPORTS
#undef GVC_EXPORTS
#endif
#define COLORPROCS_API static UNUSED

#include "../common/colxlate.c"

/// RGB, RGBA, HSV, HSVA
char *colorx(Expr_t *ex, const char *incolor, char *fmt) {
  gvcolor_t color = {{{0}}, 0};
  color_type_t type;
  int rc;
  int alpha;

  if (*fmt == '\0' || *incolor == '\0')
    return "";
  if (*fmt == 'R') {
    type = RGBA_BYTE;
    if (!strcmp(fmt, "RGBA"))
      alpha = 1;
    else
      alpha = 0;
  } else if (*fmt == 'H') {
    type = HSVA_DOUBLE;
    if (!strcmp(fmt, "HSVA"))
      alpha = 1;
    else
      alpha = 0;
  } else
    return "";

  rc = colorxlate(incolor, &color, type);
  if (rc != COLOR_OK)
    return "";

  agxbuf fp = {0};

  switch (type) {
  case HSVA_DOUBLE:
    agxbprint(&fp, "%.03f %.03f %.03f", color.u.HSVA[0], color.u.HSVA[1],
              color.u.HSVA[2]);
    if (alpha)
      agxbprint(&fp, " %.03f", color.u.HSVA[3]);
    break;
  case RGBA_BYTE:
    agxbprint(&fp, "#%02x%02x%02x", color.u.rgba[0], color.u.rgba[1],
              color.u.rgba[2]);
    if (alpha)
      agxbprint(&fp, "%02x", color.u.rgba[3]);
    break;
  default:
    break;
  }

  char *result = exstring(ex, agxbuse(&fp));
  agxbfree(&fp);
  return result;
}
