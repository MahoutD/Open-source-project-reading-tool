# teach binaries how to find plugins and libraries, even if there is another
# copy of Graphviz installed in the system default paths
if(APPLE)
  list(APPEND CMAKE_INSTALL_RPATH "@executable_path/../${PLUGIN_INSTALL_DIR}")
  list(APPEND CMAKE_INSTALL_RPATH "@executable_path/../${LIBRARY_INSTALL_DIR}")
else()
  list(APPEND CMAKE_INSTALL_RPATH "\$ORIGIN/../${PLUGIN_INSTALL_DIR}")
  list(APPEND CMAKE_INSTALL_RPATH "\$ORIGIN/../${LIBRARY_INSTALL_DIR}")
endif()

add_subdirectory(dot)
add_subdirectory(edgepaint)
add_subdirectory(gvedit)
add_subdirectory(gvmap)
add_subdirectory(gvpr)
add_subdirectory(mingle)
add_subdirectory(smyrna)
add_subdirectory(tools)
