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

set(OSPRAY_INSTALL_DIR "" CACHE PATH "install location of ospray")
mark_as_advanced(OSPRAY_INSTALL_DIR)
set(OSPRAY_BUILD_DIR "" CACHE PATH "build location of ospray")
mark_as_advanced(OSPRAY_BUILD_DIR)
if (OSPRAY_INSTALL_DIR AND OSPRAY_BUILD_DIR)
   message("Ignoring the ospray build location in favor of the intall location.")
endif()

if (OSPRAY_INSTALL_DIR)

  find_package(ospray CONFIG REQUIRED HINTS ${OSPRAY_INSTALL_DIR})

else()

  if (OSPRAY_BUILD_DIR)
    #find corresponding source directory
    load_cache(${OSPRAY_BUILD_DIR} READ_WITH_PREFIX OSP_
      CMAKE_HOME_DIRECTORY
      )
    set(OSPRAY_SOURCE_DIR ${OSP_CMAKE_HOME_DIRECTORY})

    set(OSPRAY_INCLUDE_DIRS
      ${OSPRAY_BUILD_DIR}
      ${OSPRAY_BUILD_DIR}/include
      ${OSPRAY_SOURCE_DIR}
      ${OSPRAY_SOURCE_DIR}/ospray/include
      )

    set(LIB_OSPRAY_EMBREE LIB_OSPRAY_EMBREE-NOTFOUND)
    find_library(LIB_OSPRAY_EMBREE NAMES ospray_embree embree
      PATHS ${OSPRAY_BUILD_DIR} NO_DEFAULT_PATH)
    mark_as_advanced(LIB_OSPRAY_EMBREE)

    set(LIB_OSPRAY_COMMON LIB_OSPRAY_COMMON-NOTFOUND)
    find_library(LIB_OSPRAY_COMMON ospray_common
      PATHS ${OSPRAY_BUILD_DIR} NO_DEFAULT_PATH)
    mark_as_advanced(LIB_OSPRAY_COMMON)

    set(LIB_OSPRAY LIB_OSPRAY-NOTFOUND)
    find_library(LIB_OSPRAY ospray PATHS ${OSPRAY_BUILD_DIR} NO_DEFAULT_PATH)
    mark_as_advanced(LIB_OSPRAY)

    set(OSPRAY_LIBRARIES ${LIB_OSPRAY_EMBREE} ${LIB_OSPRAY_COMMON} ${LIB_OSPRAY})

  else()

    message("Supply OSPRAY_INSTALL_DIR or OSPRAY_BUILD_DIR to find OSPRay")

  endif()

endif()
