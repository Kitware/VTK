## ======================================================================================= ##
## Copyright 2014-2015 Texas Advanced Computing Center, The University of Texas at Austin  ##
## All rights reserved.                                                                    ##
##                                                                                         ##
## Licensed under the BSD 3-Clause License, (the "License"); you may not use this file     ##
## except in compliance with the License.                                                  ##
## A copy of the License is included with this software in the file LICENSE.               ##
## If your copy does not contain the License, you may obtain a copy of the License at:     ##
##                                                                                         ##
##     http://opensource.org/licenses/BSD-3-Clause                                         ##
##                                                                                         ##
## Unless required by applicable law or agreed to in writing, software distributed under   ##
## the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY ##
## KIND, either express or implied.                                                        ##
## See the License for the specific language governing permissions and limitations under   ##
## limitations under the License.                                                          ##
## ======================================================================================= ##


###############################################################################
# Find OSPRay
# defines:
# OSPRAY_FOUND
# OSPRAY_INCLUDE_DIRS
# OSPRAY_LIBRARIES

# guess that OSPRay is installed in a peer directory (if in dev) or in a peer to the ParaView source
FIND_PATH(OSPRAY_DIR ospray
  HINTS ${PROJECT_SOURCE_DIR}/../OSPRay  ${PROJECT_SOURCE_DIR}/../../../OSPRay
  DOC "OSPRay base directory"
  )
IF(NOT OSPRAY_DIR)
  MESSAGE("Could not find OSPRay base directory. Please set OSPRAY_DIR to the root of your local OSPRay git repository.")
ENDIF(NOT OSPRAY_DIR)

FIND_PATH(OSPRAY_CMAKE_DIR ospray.cmake
  HINTS ${PROJECT_SOURCE_DIR}/../OSPRay/cmake ${PROJECT_SOURCE_DIR}/../../../OSPRay/cmake ${OSPRAY_DIR}/cmake
  DOC "OSPRay cmake directory"
  )
IF(NOT OSPRAY_CMAKE_DIR)
  MESSAGE("Could not find OSPRay cmake directory. Please set OSPRAY_CMAKE_DIR to the cmake directory of your local OSPRay git repository, usually <root>/cmake.")
ENDIF(NOT OSPRAY_CMAKE_DIR)

FIND_PATH(OSPRAY_BUILD_DIR ospModelViewer
  HINTS ${OSPRAY_DIR}/build ${PROJECT_SOURCE_DIR}/../OSPRay/build ${PROJECT_SOURCE_DIR}/../OSPRay ${PROJECT_SOURCE_DIR}/../../../OSPRay/build ${PROJECT_SOURCE_DIR}/../../../OSPRay
  DOC "OSPRay build directory"
  )
IF(NOT OSPRAY_BUILD_DIR)
  MESSAGE("Could not find OSPRay build directory. Please set OSPRAY_BUILD_DIR to the directory where OSPRay was built.")
ENDIF(NOT OSPRAY_BUILD_DIR)

if (OSPRAY_BUILD_DIR)
  LOAD_CACHE(${OSPRAY_BUILD_DIR} READ_WITH_PREFIX OSP_
    OSPRAY_BUILD_MIC_SUPPORT
    OSPRAY_BUILD_MPI_DEVICE
    OSPRAY_COMPILER
    OSPRAY_XEON_TARGET
    )

  SET(OSPRAY_INCLUDE_DIRS
    ${OSPRAY_BUILD_DIR}
    ${OSPRAY_DIR}
    ${OSPRAY_DIR}/ospray
    ${OSPRAY_DIR}/ospray/embree/common        #0.8
    ${OSPRAY_DIR}/ospray/embree               #0.8
    ${OSPRAY_DIR}/ospray/embree-v2.7.1/common #0.9
    ${OSPRAY_DIR}/ospray/embree-v2.7.1        #0.9
    ${OSPRAY_DIR}/ospray/include
    )

  SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${OSPRAY_CMAKE_DIR} ${OSPRAY_DIR})
  # which compiler was used to build OSPRay
  SET(OSPRAY_CC ${OSP_OSPRAY_COMPILER} CACHE STRING "OSPRay Compiler (ICC, GCC, CLANG)")
  # whehter to build in MIC/xeon phi support
  SET(OSPRAY_MIC ${OSP_OSPRAY_BUILD_MIC_SUPPORT} CACHE BOOL "Was OSPRay buit with Xeon Phi Support?")
  # whehter to build in MIC/xeon phi support
  SET(OSPRAY_MPI ${OSP_OSPRAY_BUILD_MPI_DEVICE} CACHE BOOL "Was OSPRay built with MPI Remote/Distributed rendering support?")
  # the arch we're targeting for the non-MIC/non-xeon phi part of ospray
  SET(OSPRAY_XEON_TARGET ${OSP_OSPRAY_XEON_TARGET} CACHE STRING "OSPRay target ISA on host (SSE,AVX,AVX2)")

  ADD_DEFINITIONS(${OSPRAY_EMBREE_CXX_FLAGS})
endif(OSPRAY_BUILD_DIR)

# MESSAGE("ospray_dir ${OSPRAY_DIR}")
# SET(OSPRAY_DIR2 ${OSPRAY_DIR})
# INCLUDE(${OSPRAY_DIR}/cmake/ospray.cmake)
# SET(OSPRAY_DIR ${OSPRAY_DIR2})
# MESSAGE("ospray_dir ${OSPRAY_DIR}")

if(OSPRAY_CMAKE_DIR)
  INCLUDE(${OSPRAY_CMAKE_DIR}/ospray.cmake)
  INCLUDE(${OSPRAY_CMAKE_DIR}/mpi.cmake)
endif(OSPRAY_CMAKE_DIR)

SET(LIB_OSPRAY_EMBREE LIB_OSPRAY_EMBREE-NOTFOUND)
SET(LIB_OSPRAY LIB_OSPRAY-NOTFOUND)
FIND_LIBRARY(LIB_OSPRAY_EMBREE ospray_embree ${OSPRAY_BUILD_DIR})
FIND_LIBRARY(LIB_OSPRAY ospray ${OSPRAY_BUILD_DIR})
IF (OSPRAY_MIC)
  # Xeon Phi specific build ops here
ENDIF(OSPRAY_MIC)

SET(OSPRAY_LIBRARIES
  ${LIB_OSPRAY_EMBREE}
  ${LIB_OSPRAY}
  )
