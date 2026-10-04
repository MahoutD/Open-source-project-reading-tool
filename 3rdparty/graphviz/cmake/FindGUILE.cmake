include(FindPackageHandleStandardArgs)
find_package(PkgConfig)

pkg_check_modules(GUILE guile-3.0)
if(NOT GUILE_FOUND)
  pkg_check_modules(GUILE guile-2.2)
endif()
if(NOT GUILE_FOUND)
  pkg_check_modules(GUILE guile-2.0)
endif()
if(NOT GUILE_FOUND)
  pkg_check_modules(GUILE guile-1.8)
endif()

find_package_handle_standard_args(GUILE DEFAULT_MSG
  GUILE_INCLUDE_DIRS
  GUILE_LIBRARIES
  GUILE_LINK_LIBRARIES
)
