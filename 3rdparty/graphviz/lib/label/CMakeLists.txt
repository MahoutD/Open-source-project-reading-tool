add_library(label STATIC
  index.c
  node.c
  rectangle.c
  split.q.c
  xlabels.c
)

target_include_directories(label PRIVATE
  ..
  ../cdt
  ../cgraph
  ../common
  ../gvc
  ../pathplan
)

target_link_libraries(label PRIVATE
  cdt
)
