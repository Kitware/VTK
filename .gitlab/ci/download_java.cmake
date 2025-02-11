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

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "windows")
  set(jdk_sha256_sum 69f1c21a176f076bb01e985cf40a18b5772793004bbc4e92983e1ac55bd717e8)
  set(jdk_url https://download.java.net/openjdk/jdk11.0.0.2/ri/openjdk-11.0.0.2_windows-x64.zip)
  set(jdk_root_dir jdk-11.0.0.2)
  set(jdk_ext zip)
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos")
  if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "x86_64")
    set(jdk_sha256_sum f365750d4be6111be8a62feda24e265d97536712bc51783162982b8ad96a70ee)
    set(jdk_url https://download.java.net/java/GA/jdk11/9/GPL/openjdk-11.0.2_osx-x64_bin.tar.gz)
    set(jdk_root_dir jdk-11.0.2.jdk)
  elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "arm64")
    set(jdk_sha256_sum ac3ff5a57b9d00606e9b319bde7309a4ecb9f2c4ddc0f48d001f234e93b9da86)
    set(jdk_url https://download.oracle.com/java/17/archive/jdk-17.0.11_macos-aarch64_bin.tar.gz)
    set(jdk_root_dir jdk-17.0.11.jdk)
  endif()
  set(jdk_ext tar.gz)
endif()
if (NOT DEFINED jdk_url)
  message(FATAL_ERROR "Unknown platform")
endif()

message("Downloading ${jdk_url}")
set(outdir "${CMAKE_SOURCE_DIR}/.gitlab/")
set(archive_path "${outdir}/jdk.${jdk_ext}")

# Download and extract
file(DOWNLOAD ${jdk_url} "${archive_path}" EXPECTED_HASH SHA256=${jdk_sha256_sum})
file(ARCHIVE_EXTRACT INPUT "${archive_path}" DESTINATION "${outdir}" VERBOSE)

# Rename subdir to standard naming
file(RENAME "${outdir}/${jdk_root_dir}" "${outdir}/jdk/")
