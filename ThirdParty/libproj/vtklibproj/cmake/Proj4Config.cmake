################################################################################
# SociConfig.cmake - CMake build configuration of SOCI library
################################################################################
# Copyright (C) 2010 Mateusz Loskot <mateusz@loskot.net>
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
################################################################################
include (CheckFunctionExists)

CHECK_FUNCTION_EXISTS(localeconv HAVE_LOCALECONV)

set(PACKAGE "proj")
set(PACKAGE_BUGREPORT "warmerdam@pobox.com")
set(PACKAGE_NAME "PROJ.4 Projections")
set(PACKAGE_STRING "PROJ.4 Projections ${${PROJECT_INTERN_NAME}_VERSION}")
set(PACKAGE_TARNAME "proj")
set(PACKAGE_VERSION "${${PROJECT_INTERN_NAME}_VERSION}")

configure_file(cmake/proj_config.cmake.in src/proj_config.h)


