/// \file
/// \brief test that `agxbuf` can use its entire memory as an inline string
///
/// This program accesses `agxbuf` internals in a way that is not expected from
/// users. This is testing the internal behavior of `agxbuf` itself.

#ifdef NDEBUG
#error "this program is not intended to be compiled with assertions disabled"
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <util/agxbuf.h>

int main(void) {

  agxbuf xb = {0};
  assert(agxbuf_is_inline(&xb));

  // construct a string that covers the entirety of an `agxbuf`
  char content[sizeof(xb)] = {0};
  for (size_t i = 0; i < sizeof(content) - 1; ++i) {
    content[i] = 'A' + (char)i;
  }

  // we should be able to print this to the buffer while remaining inline
  agxbprint(&xb, "%s", content);
  assert(agxbuf_is_inline(&xb));

  // we should be able to retrieve and use the content without incurring a heap
  // allocation
  printf("content is: %s\n", agxbuse(&xb));
  assert(agxbuf_is_inline(&xb));

  return EXIT_SUCCESS;
}
