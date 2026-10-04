// get the value of PACKAGE_VERSION

#include <graphviz/graphviz_version.h>
#include <stdio.h>

#ifndef PACKAGE_VERSION
#error PACKAGE_VERSION missing from graphviz_version.h
#endif

int main(void) {
  printf("%s\n", PACKAGE_VERSION "");
  return 0;
}
