add_library(tclstubs OBJECT
  tclStubLib.c
)

target_include_directories(tclstubs SYSTEM PRIVATE ${TCL_INCLUDE_PATH})
target_link_libraries(tclstubs PRIVATE ${TCL_LIBRARY})
