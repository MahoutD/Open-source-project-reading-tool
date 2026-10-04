include(FindPackageHandleStandardArgs)
find_package(PkgConfig)

pkg_check_modules(JAVASCRIPTCORE javascriptcoregtk-4.1)
if(NOT JAVASCRIPTCORE_FOUND)
  pkg_check_modules(JAVASCRIPTCORE javascriptcoregtk-4.0)
endif()

find_package_handle_standard_args(JAVASCRIPTCORE DEFAULT_MSG
  JAVASCRIPTCORE_INCLUDE_DIRS
  JAVASCRIPTCORE_LIBRARIES
  JAVASCRIPTCORE_LINK_LIBRARIES
)
