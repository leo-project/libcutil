# - Try to find Subunit
# Once done this will define
#
#  SUBUNIT_FOUND - system has Subunit
#  SUBUNIT_INCLUDE_DIRS - the Subunit include directory
#  SUBUNIT_LIBRARIES - Link these to use Subunit
#  SUBUNIT_DEFINITIONS - Compiler switches required for using Subunit
#
#  Copyright (c) 2009 Andreas Schneider <mail@cynapses.org>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


if (SUBUNIT_LIBRARIES AND SUBUNIT_INCLUDE_DIRS)
  # in cache already
  set(SUBUNIT_FOUND TRUE PARENT_SCOPE)
else (SUBUNIT_LIBRARIES AND SUBUNIT_INCLUDE_DIRS)
  find_package(PkgConfig)
  if (PKG_CONFIG_FOUND)
      pkg_check_modules(_SUBUNIT subunit)
  endif (PKG_CONFIG_FOUND)

  find_path(SUBUNIT_INCLUDE_DIR
    NAMES
      child.h
    PATHS
      ${_SUBUNIT_INCLUDEDIR}
      /usr/include/subunit
      /usr/local/include/subunit
      /opt/local/include/subunit
      /sw/include/subunit
  )

  find_library(SUBUNIT_LIBRARY
    NAMES
      subunit
    PATHS
      ${_SUBUNIT_LIBDIR}
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /sw/lib
  )

  set(SUBUNIT_INCLUDE_DIRS
    ${SUBUNIT_INCLUDE_DIR}
  )

  set(SUBUNIT_LIBRARIES
    ${SUBUNIT_LIBRARY}
  )

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Subunit DEFAULT_MSG SUBUNIT_LIBRARIES SUBUNIT_INCLUDE_DIRS)

  # show the SUBUNIT_INCLUDE_DIRS and SUBUNIT_LIBRARIES variables only in the advanced view
  mark_as_advanced(SUBUNIT_INCLUDE_DIRS SUBUNIT_LIBRARIES)

endif (SUBUNIT_LIBRARIES AND SUBUNIT_INCLUDE_DIRS)

