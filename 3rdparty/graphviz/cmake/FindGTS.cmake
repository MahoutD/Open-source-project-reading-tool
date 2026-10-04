include(FindPackageHandleStandardArgs)

find_package(PkgConfig)
pkg_check_modules(GTS gts)

if(MINGW)
  find_package(GLIB)

  find_program(GTS_RUNTIME_LIBRARY NAMES libgts-0-7-5.dll)

  find_package_handle_standard_args(GTS DEFAULT_MSG
    GTS_INCLUDE_DIRS

    GTS_LINK_LIBRARIES

    GTS_RUNTIME_LIBRARY
    GLIB_RUNTIME_LIBRARIES
  )

  set(GTS_RUNTIME_LIBRARIES
    ${GTS_RUNTIME_LIBRARY}
    ${GLIB_RUNTIME_LIBRARIES}
  )
elseif(WIN32)
  find_package(GLIB)

  find_program(GTS_RUNTIME_LIBRARY NAMES gts.dll)

  find_package_handle_standard_args(GTS DEFAULT_MSG
    GTS_INCLUDE_DIRS

    GTS_LINK_LIBRARIES

    GTS_RUNTIME_LIBRARY
    GLIB_RUNTIME_LIBRARIES
  )

  set(GTS_RUNTIME_LIBRARIES
    ${GTS_RUNTIME_LIBRARY}
    ${GLIB_RUNTIME_LIBRARIES}
  )
else()
  find_package_handle_standard_args(GTS DEFAULT_MSG
    GTS_INCLUDE_DIRS
    GTS_LIBRARIES
    GTS_LINK_LIBRARIES
  )
endif()
