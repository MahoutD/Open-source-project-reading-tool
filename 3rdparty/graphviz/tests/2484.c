/// \file
/// \brief test case driver for #2484
///
/// See test_regression.py:test_2484

#include <graphviz/cgraph.h>
#include <graphviz/gvc.h>
#include <stdio.h>

int dot(int argc, char *argv[]) {
  GVC_t *gvc = gvContext();
  gvParseArgs(gvc, argc, argv);
  graph_t *g, *prev = NULL;
  while ((g = gvNextInputGraph(gvc))) {
    if (prev) {
      gvFreeLayout(gvc, prev);
      agclose(prev);
    }
    gvLayoutJobs(gvc, g);
    gvRenderJobs(gvc, g);
    prev = g;
  }
  gvFinalize(gvc);
  int result = gvFreeContext(gvc);
  return result;
}

int main(int argc, char *argv[]) {
  printf("Point 1");
  dot(argc, argv);
  printf("Point 2");
  dot(argc, argv);
  printf("Point 3");
  dot(argc, argv);

  return 0;
}
