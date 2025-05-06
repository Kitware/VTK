##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

#=============================================================================
# Copyright 2011 Kitware, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#=============================================================================

# Check for a hint left by the 'GitInfo' script.
if(NOT GIT_EXECUTABLE)
  get_filename_component(_Git_DIR ${CMAKE_CURRENT_LIST_FILE} PATH)
  include(${_Git_DIR}/GitInfo.cmake OPTIONAL)
  if(GitInfo_GIT_EXECUTABLE)
    if(EXISTS "${GitInfo_GIT_EXECUTABLE}")
      set(GIT_EXECUTABLE "${GitInfo_GIT_EXECUTABLE}")
    elseif(EXISTS "${GitInfo_GIT_EXECUTABLE}.exe")
      set(GIT_EXECUTABLE "${GitInfo_GIT_EXECUTABLE}.exe")
    endif()
  endif()
endif()

# Find Git.
find_package(Git)
