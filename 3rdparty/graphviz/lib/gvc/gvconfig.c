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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include <assert.h>
#include <gvc/gvconfig.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util/agxbuf.h>
#include <util/alloc.h>
#include <util/exit.h>
#include <util/gv_ctype.h>
#include <util/gv_fopen.h>
#include <util/list.h>
#include <util/path.h>
#include <util/startswith.h>

#ifdef ENABLE_LTDL
#ifdef HAVE_DL_ITERATE_PHDR
#include <link.h>
#endif
#include <sys/types.h>
#ifdef _WIN32
#include <windows.h>
#define GLOB_NOMATCH 3 /* No matches found.  */
#define GLOB_NOSORT 4
typedef struct {
  size_t gl_pathc; /* count of total paths so far */
  char **gl_pathv; /* list of paths matching pattern */
} glob_t;
static void globfree(glob_t *pglob);
static int glob(GVC_t *gvc, char *, int, int (*errfunc)(const char *, int),
                glob_t *);
#else
#include <glob.h>
#endif
#include <sys/stat.h>
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include <common/const.h>
#include <common/types.h>

#include <gvc/gvcint.h>
#include <gvc/gvcjob.h>
#include <gvc/gvcproc.h>
#include <gvc/gvplugin.h>

/* FIXME */
extern void textfont_dict_open(GVC_t *gvc);

/*
    A config for gvrender is a text file containing a
    list of plugin libraries and their capabilities using a tcl-like
    syntax

    Lines beginning with '#' are ignored as comments

    Blank lines are allowed and ignored.

    plugin_library_path packagename {
        plugin_api {
            plugin_type plugin_quality
            ...
        }
        ...
    ...

    e.g.

        /usr/lib/graphviz/libgvplugin_cairo.so cairo {renderer {x 0 png 10 ps
   -10}} /usr/lib/graphviz/libgvplugin_gd.so gd {renderer {png 0 gif 0 jpg 0}}

    Internally the config is maintained as lists of plugin_types for each
   plugin_api. If multiple plugins of the same type are found then the highest
   quality wins. If equal quality then the last-one-installed wins (thus giving
   preference to external plugins over internal builtins).

 */

static gvplugin_package_t *gvplugin_package_record(GVC_t *gvc,
                                                   const char *package_path,
                                                   const char *name) {
  gvplugin_package_t *package = gv_alloc(sizeof(gvplugin_package_t));
  package->path = package_path ? gv_strdup(package_path) : NULL;
  package->name = gv_strdup(name);
  package->next = gvc->packages;
  gvc->packages = package;
  return package;
}

#ifdef ENABLE_LTDL
/*
  separator - consume all non-token characters until next token.  This includes:
        comments:   '#' ... '\n'
        nesting:    '{'
        unnesting:  '}'
        whitespace: ' ','\t','\n'

        *nest is changed according to nesting/unnesting processed
 */
static void separator(int *nest, char **tokens) {
  char c, *s;

  s = *tokens;
  while ((c = *s)) {
    /* #->eol = comment */
    if (c == '#') {
      s++;
      while ((c = *s)) {
        s++;
        if (c == '\n')
          break;
      }
      continue;
    }
    if (c == '{') {
      (*nest)++;
      s++;
      continue;
    }
    if (c == '}') {
      (*nest)--;
      s++;
      continue;
    }
    if (c == ' ' || c == '\n' || c == '\t') {
      s++;
      continue;
    }
    break;
  }
  *tokens = s;
}

/*
  token - capture all characters until next separator, then consume separator,
        return captured token, leave **tokens pointing to next token.
 */
static char *token(int *nest, char **tokens) {
  char c, *s, *t;

  s = t = *tokens;
  while ((c = *s)) {
    if (c == '#' || c == ' ' || c == '\t' || c == '\n' || c == '{' || c == '}')
      break;
    s++;
  }
  *tokens = s;
  separator(nest, tokens);
  *s = '\0';
  return t;
}

static int gvconfig_plugin_install_from_config(GVC_t *gvc, char *s) {
  int nest = 0;

  separator(&nest, &s);
  while (*s) {
    char *const package_path = token(&nest, &s);
    char *const name = nest == 0 ? token(&nest, &s) : "x";
    gvplugin_package_t *const package =
        gvplugin_package_record(gvc, package_path, name);
    do {
      const char *api = token(&nest, &s);
      const api_t gv_api = gvplugin_api(api);
      if (gv_api == (api_t)-1) {
        agerrorf("config error: %s %s not found\n", package_path, api);
        return 0;
      }
      do {
        if (nest == 2) {
          const char *const type = token(&nest, &s);
          const int quality = nest == 2 ? atoi(token(&nest, &s)) : 0;
          bool rc = gvplugin_install(gvc, gv_api, type, quality, package, NULL);
          if (!rc) {
            agerrorf("config error: %s %s %s\n", package_path, api, type);
            return 0;
          }
        }
      } while (nest == 2);
    } while (nest == 1);
  }
  return 1;
}
#endif

void gvconfig_plugin_install_from_library(GVC_t *gvc, char *package_path,
                                          gvplugin_library_t *library) {
  gvplugin_api_t *apis;
  gvplugin_installed_t *types;
  gvplugin_package_t *package;
  int i;

  package = gvplugin_package_record(gvc, package_path, library->packagename);
  for (apis = library->apis; (types = apis->types); apis++) {
    for (i = 0; types[i].type; i++) {
      gvplugin_install(gvc, apis->api, types[i].type, types[i].quality, package,
                       &types[i]);
    }
  }
}

static void gvconfig_plugin_install_builtins(GVC_t *gvc) {
  const lt_symlist_t *s;
  const char *name;

  if (gvc->common.builtins == NULL)
    return;

  for (s = gvc->common.builtins; (name = s->name); s++)
    if (name[0] == 'g' && strstr(name, "_LTX_library"))
      gvconfig_plugin_install_from_library(gvc, NULL, s->address);
}

#ifdef ENABLE_LTDL
static void gvconfig_write_library_config(GVC_t *gvc, char *lib_path,
                                          gvplugin_library_t *library,
                                          FILE *f) {
  gvplugin_installed_t *types;

  fprintf(f, "%s %s {\n", lib_path, library->packagename);
  for (gvplugin_api_t *apis = library->apis; (types = apis->types); apis++) {
    fprintf(f, "\t%s {\n", gvplugin_api_name(apis->api));
    for (size_t i = 0; types[i].type; i++) {
      /* verify that dependencies are available */
      if (!gvplugin_load(gvc, apis->api, types[i].type,
                         gvc->common.verbose > 0 ? f : NULL))
        fprintf(f, "#FAILS");
      fprintf(f, "\t\t%s %d\n", types[i].type, types[i].quality);
    }
    fputs("\t}\n", f);
  }
  fputs("}\n", f);
}

#define BSZ 1024
#define DOTLIBS "/.libs"

#ifdef HAVE_DL_ITERATE_PHDR
static int line_callback(struct dl_phdr_info *info, size_t size, void *xb) {
  const char *p = info->dlpi_name;
  const char *const tmp = strstr(p, "/libgvc.");
  (void)size;
  if (tmp) {
    const char *slash;
    for (slash = tmp - 1; *slash != '/'; --slash)
      ;
    /* Check for real /lib dir. Don't accept pre-install /.libs */
    if (strncmp(slash, DOTLIBS, (size_t)(tmp - slash)) != 0) {
      agxbclear(xb);
      // plugins are in "graphviz" subdirectory
      agxbprint(xb, "%.*s/graphviz", (int)(tmp - p), p);
      return 1;
    }
  }
  return 0;
}
#endif

char *gvconfig_libdir(GVC_t *gvc) {
  agxbuf libdir = {0};
  static atomic_flag dirShown;

  const char *const gvbindir = getenv("GVBINDIR");
  if (gvbindir != NULL) {
    agxbput(&libdir, gvbindir);
  } else {
#ifdef _WIN32
    int r;
    char *s;

    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(&gvconfig_libdir, &mbi, sizeof(mbi)) == 0) {
      agerrorf("failed to get handle for executable.\n");
      return 0;
    }
    {
      char line[BSZ] = {0};
      r = GetModuleFileName((HMODULE)mbi.AllocationBase, line, BSZ);
      if (!r || (r == BSZ)) {
        agerrorf("failed to get path for executable.\n");
        return 0;
      }
      s = strrchr(line, '\\');
      if (!s) {
        agerrorf("no slash in path %s.\n", line);
        return 0;
      }
      agxbput_n(&libdir, line, (size_t)(s - line));
    }
#else
    agxbput(&libdir, GVLIBDIR);
#ifdef __APPLE__
    uint32_t i, c = _dyld_image_count();
    size_t ind;
    for (i = 0; i < c; ++i) {
      const char *p = _dyld_get_image_name(i);
      const char *tmp = strstr(p, "/libgvc.");
      if (tmp) {
        if (tmp > p) {
          /* Check for real /lib dir. Don't accept pre-install /.libs */
          const char *s = tmp - 1;
          /* back up to previous slash (or head of string) */
          while (*s != '/' && s > p)
            s--;
          if (startswith(s, DOTLIBS))
            continue;
        }

        ind = tmp - p; // byte offset
        /* plugins are in "graphviz" subdirectory */
        agxbclear(&libdir);
        agxbprint(&libdir, "%.*s/graphviz", (int)ind, p);
        break;
      }
    }
#elif defined(HAVE_DL_ITERATE_PHDR)
    dl_iterate_phdr(line_callback, &libdir);
#else
    FILE *f = gv_fopen("/proc/self/maps", "r");
    if (f) {
      while (!feof(f)) {
        char line[BSZ] = {0};
        if (!fgets(line, sizeof(line), f))
          continue;
        if (!strstr(line, " r-xp "))
          continue;
        char *p = strchr(line, '/');
        if (!p)
          continue;
        char *tmp = strstr(p, "/libgvc.");
        if (tmp) {
          *tmp = 0;
          /* Check for real /lib dir. Don't accept pre-install /.libs */
          if (strcmp(strrchr(p, '/'), DOTLIBS) == 0)
            continue;
          /* plugins are in "graphviz" subdirectory */
          agxbclear(&libdir);
          agxbprint(&libdir, "%s/graphviz", p);
          break;
        }
      }
      fclose(f);
    }
#endif
#endif
  }
  char *const dir = agxbdisown(&libdir);
  if (gvc->common.verbose && !atomic_flag_test_and_set(&dirShown)) {
    fprintf(stderr, "libdir = \"%s\"\n", dir);
  }
  return dir;
}
#endif

#ifdef ENABLE_LTDL
// does this path look like a Graphviz plugin of our version?
static bool is_plugin(const char *filepath) {

  if (filepath == NULL) {
    return false;
  }

  // shared library suffix to strip before looking for version number
#if defined(DARWIN_DYLIB)
  static const char SUFFIX[] = ".dylib";
#elif defined(__MINGW32__) || defined(__CYGWIN__) || defined(_WIN32)
  static const char SUFFIX[] = ".dll";
#else
  static const char SUFFIX[] = "";
#endif

  // does this filename end with the expected suffix?
  size_t len = strlen(filepath);
  if (len < strlen(SUFFIX) ||
      strcmp(filepath + len - strlen(SUFFIX), SUFFIX) != 0) {
    return false;
  }
  len -= strlen(SUFFIX);

#if defined(_WIN32) && !defined(__MINGW32__) && !defined(__CYGWIN__)
  // Windows libraries do not have a version in the filename

#elif defined(GVPLUGIN_VERSION)
// turn GVPLUGIN_VERSION into a string
#define STRINGIZE_(x) #x
#define STRINGIZE(x) STRINGIZE_(x)
  static const char VERSION[] = STRINGIZE(GVPLUGIN_VERSION);
#undef STRINGIZE
#undef STRINGIZE_

  // does this filename contain the expected version?
  if (len < strlen(VERSION) ||
      !startswith(filepath + len - strlen(VERSION), VERSION)) {
    return false;
  }
  len -= strlen(VERSION);

#else
  // does this filename have a version?
  if (len == 0 || !gv_isdigit(filepath[len - 1])) {
    return false;
  }
  while (len > 0 && gv_isdigit(filepath[len - 1])) {
    --len;
  }

#endif

  // ensure the remainder conforms to what we expect of a shared library
#if defined(DARWIN_DYLIB)
  if (len < 2 || gv_isdigit(filepath[len - 2]) || filepath[len - 1] != '.') {
    return false;
  }
#elif defined(__MINGW32__) || defined(__CYGWIN__)
  if (len < 2 || gv_isdigit(filepath[len - 2]) || filepath[len - 1] != '-') {
    return false;
  }
#elif defined(_WIN32)
  if (len < 1 || gv_isdigit(filepath[len - 1])) {
    return false;
  }
#else
  static const char SO[] = ".so.";
  if (len < strlen(SO) || !startswith(filepath + len - strlen(SO), SO)) {
    return false;
  }
#endif

  return true;
}

static void config_rescan(GVC_t *gvc, char *config_path) {
  FILE *f = NULL;
  glob_t globbuf;
  char *libdir;
  int rc;
  gvplugin_library_t *library;
#if defined(DARWIN_DYLIB)
  char *plugin_glob = "libgvplugin_*";
#elif defined(__MINGW32__)
  char *plugin_glob = "libgvplugin_*";
#elif defined(__CYGWIN__)
  char *plugin_glob = "cyggvplugin_*";
#elif defined(_WIN32)
  char *plugin_glob = "gvplugin_*";
#else
  char *plugin_glob = "libgvplugin_*";
#endif

  if (config_path) {
    f = gv_fopen(config_path, "w");
    if (!f) {
      agerrorf("failed to open %s for write.\n", config_path);
      graphviz_exit(1);
    }

    fprintf(f,
            "# This file was generated by \"dot -c\" at time of install.\n\n");
    fprintf(f, "# You may temporarily disable a plugin by removing or "
               "commenting out\n");
    fprintf(f, "# a line in this file, or you can modify its \"quality\" value "
               "to affect\n");
    fprintf(f, "# default plugin selection.\n\n");
    fprintf(f, "# Manual edits to this file **will be lost** on upgrade.\n\n");
  }

  libdir = gvconfig_libdir(gvc);

  agxbuf config_glob = {0};
  agxbprint(&config_glob, "%s%c%s", libdir, PATH_SEPARATOR, plugin_glob);
  free(libdir);

  /* load all libraries even if can't save config */

#if defined(_WIN32)
  rc = glob(gvc, agxbuse(&config_glob), GLOB_NOSORT, NULL, &globbuf);
#else
  rc = glob(agxbuse(&config_glob), 0, NULL, &globbuf);
#endif
  if (rc == 0) {
    for (size_t i = 0; i < globbuf.gl_pathc; i++) {
      if (is_plugin(globbuf.gl_pathv[i])) {
        library = gvplugin_library_load(gvc, globbuf.gl_pathv[i]);
        if (library) {
          gvconfig_plugin_install_from_library(gvc, globbuf.gl_pathv[i],
                                               library);
        }
      }
    }
    /* rescan with all libs loaded to check cross dependencies */
    for (size_t i = 0; i < globbuf.gl_pathc; i++) {
      if (is_plugin(globbuf.gl_pathv[i])) {
        library = gvplugin_library_load(gvc, globbuf.gl_pathv[i]);
        if (library) {
          char *p = strrchr(globbuf.gl_pathv[i], PATH_SEPARATOR);
          if (p)
            p++;
          if (f && p)
            gvconfig_write_library_config(gvc, p, library, f);
        }
      }
    }
  }
  globfree(&globbuf);
  agxbfree(&config_glob);
  if (f)
    fclose(f);
}
#endif

/*
  gvconfig - parse a config file and install the identified plugins
 */
void gvconfig(GVC_t *gvc, bool rescan) {
#ifdef ENABLE_LTDL
  int rc;
  struct stat config_st;
  FILE *f = NULL;
  char *config_text = NULL;
  char *libdir;
  char *config_file_name = GVPLUGIN_CONFIG_FILE;

#endif

  /* builtins don't require LTDL */
  gvconfig_plugin_install_builtins(gvc);

  gvc->config_found = false;
#ifdef ENABLE_LTDL
  if (gvc->common.demand_loading) {
    /* see if there are any new plugins */
    libdir = gvconfig_libdir(gvc);
    if (access(libdir, F_OK) < 0) {
      /* if we fail to stat it then it probably doesn't exist so just fail
       * silently */
      free(libdir);
      goto done;
    }

    if (!gvc->config_path) {
      agxbuf xb = {0};
      agxbprint(&xb, "%s%c%s", libdir, PATH_SEPARATOR, config_file_name);
      gvc->config_path = agxbdisown(&xb);
    }
    free(libdir);

    if (rescan) {
      config_rescan(gvc, gvc->config_path);
      gvc->config_found = true;
      gvtextlayout_select(
          gvc); /* choose best available textlayout plugin immediately */
      assert(gvc->textfont_dt != NULL &&
             "config rescan performed without any prior first scan");
      return;
    }

    /* load in the cached plugin library data */

    rc = stat(gvc->config_path, &config_st);
    if (rc == -1) {
      /* silently return without setting gvc->config_found = TRUE */
      goto done;
    } else {
      f = gv_fopen(gvc->config_path, "r");
      if (!f) {
        agerrorf("failed to open %s for read.\n", gvc->config_path);
        return;
      } else if (config_st.st_size == 0) {
        agerrorf("%s is zero sized.\n", gvc->config_path);
      } else {
        config_text = gv_alloc((size_t)config_st.st_size + 1);
        size_t sz = fread(config_text, 1, (size_t)config_st.st_size, f);
        if (sz == 0) {
          agerrorf("%s read error.\n", gvc->config_path);
        } else {
          gvc->config_found = true;
          config_text[sz] = '\0'; /* make input into a null terminated string */
          rc = gvconfig_plugin_install_from_config(gvc, config_text);
        }
        free(config_text);
      }
      if (f) {
        fclose(f);
      }
    }
  }
done:
#else
  (void)rescan;
#endif
  gvtextlayout_select(
      gvc); /* choose best available textlayout plugin immediately */
  textfont_dict_open(gvc); /* initialize font dict */
}

#ifdef ENABLE_LTDL
#ifdef _WIN32

/* Emulating windows glob */

/* glob:
 * Assumes only GLOB_NOSORT flag given. That is, there is no offset,
 * and no previous call to glob.
 */

static int glob(GVC_t *gvc, char *pattern, int flags,
                int (*errfunc)(const char *, int), glob_t *pglob) {
  (void)flags;
  (void)errfunc;

  WIN32_FIND_DATA wfd;
  LIST(char *) strs = {.dtor = LIST_DTOR_FREE};

  pglob->gl_pathc = 0;
  pglob->gl_pathv = NULL;

  // the Windows APIs use '\'-separated directory components, but MinGW uses
  // '/' as the canonical directory separator, so convert these
#ifdef __MINGW32__
  for (size_t i = 0; pattern[i] != '\0'; ++i) {
    if (pattern[i] == '/') {
      pattern[i] = '\\';
    }
  }
#endif

  HANDLE h = FindFirstFile(pattern, &wfd);
  if (h == INVALID_HANDLE_VALUE)
    return GLOB_NOMATCH;
  char *const libdir = gvconfig_libdir(gvc);
  do {
    agxbuf entry = {0};
    agxbprint(&entry, "%s%c%s", libdir, PATH_SEPARATOR, wfd.cFileName);
    LIST_APPEND(&strs, agxbdisown(&entry));
  } while (FindNextFile(h, &wfd));
  LIST_APPEND(&strs, NULL);

  LIST_DETACH(&strs, &pglob->gl_pathv, &pglob->gl_pathc);
  free(libdir);

  return 0;
}

static void globfree(glob_t *pglob) {
  for (int i = 0; i < pglob->gl_pathc; i++)
    free(pglob->gl_pathv[i]);
  free(pglob->gl_pathv);
}
#endif
#endif
