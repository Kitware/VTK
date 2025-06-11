##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

set(maven_sha256_sum b917e5e8c9f86a063e621241392f43619f414e68de93c1a6753cd56594cdac40)
set(maven_url https://vtk.org/files/support/apache-maven-3.9.10-bin.zip)
set(maven_ext zip)

message("Downloading ${maven_url}")
set(outdir "${CMAKE_SOURCE_DIR}/.gitlab/")
set(archive_path "${outdir}/maven.${maven_ext}")

# Download and extract
file(DOWNLOAD ${maven_url} "${archive_path}" EXPECTED_HASH SHA256=${maven_sha256_sum})
file(ARCHIVE_EXTRACT INPUT "${archive_path}" DESTINATION "${outdir}" VERBOSE)

# Rename subdir to standard naming
file(GLOB maven_path "${outdir}/*maven*-*/")
file(RENAME ${maven_path} "${outdir}/maven/")
