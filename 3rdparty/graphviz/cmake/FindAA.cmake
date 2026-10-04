find_path(AA_INCLUDE_DIR aalib.h)
find_library(AA_LIBRARY NAMES aa libaa)
find_program(AA_RUNTIME_LIBRARY aa.dll)

include(FindPackageHandleStandardArgs)
if(WIN32)
  find_package_handle_standard_args(AA DEFAULT_MSG
                                    AA_LIBRARY AA_INCLUDE_DIR
                                    AA_RUNTIME_LIBRARY)
else()
  find_package_handle_standard_args(AA DEFAULT_MSG
                                    AA_LIBRARY AA_INCLUDE_DIR)
endif()

mark_as_advanced(AA_INCLUDE_DIR AA_LIBRARY AA_RUNTIME_LIBRARY)

set(AA_INCLUDE_DIRS ${AA_INCLUDE_DIR})
set(AA_LIBRARIES ${AA_LIBRARY})
set(AA_RUNTIME_LIBRARIES ${AA_RUNTIME_LIBRARY})
