##=============================================================================
##   This file is part of VTKEdge. See vtkedge.org for more information.
##
##   Copyright (c) 2008 Kitware, Inc.
##
##   VTKEdge may be used under the terms of the GNU General Public License 
##   version 3 as published by the Free Software Foundation and appearing in 
##   the file LICENSE.txt included in the top level directory of this source
##   code distribution. Alternatively you may (at your option) use any later 
##   version of the GNU General Public License if such license has been 
##   publicly approved by Kitware, Inc. (or its successors, if any).
##
##   VTKEdge is distributed "AS IS" with NO WARRANTY OF ANY KIND, INCLUDING
##   THE WARRANTIES OF DESIGN, MERCHANTABILITY, AND FITNESS FOR A PARTICULAR
##   PURPOSE. See LICENSE.txt for additional details.
##
##   VTKEdge is available under alternative license terms. Please visit
##   vtkedge.org or contact us at kitware@kitware.com for further information.
##
##=============================================================================

# -----------------------------------------------------------------------------
# Find 3DconnexionClient framework (Mac OS X).
#
# The 3DconnexionClient framework is the 3DxMacWare SDK, installed as part of
# the 3DxMacWare driver software. It is located in
# /Library/Frameworks/3DconnexionClient.framework
# This is the API to deal with 3D Connexion devices, like the SpaceNavigator.
#
# Define:
# 3DconnexionClient_FOUND
# 3DconnexionClient_INCLUDE_DIR
# 3DconnexionClient_LIBRARY

#message("CMAKE_SYSTEM_FRAMEWORK_PATH = ${CMAKE_SYSTEM_FRAMEWORK_PATH}")

set(3DconnexionClient_FOUND false)
set(3DconnexionClient_INCLUDE_DIR)
set(3DconnexionClient_LIBRARY)

if(APPLE) # The only platform it makes sense to check for 3DconnexionClient
 find_library(3DconnexionClient 3DconnexionClient)
 if(3DconnexionClient)
  set(3DconnexionClient_FOUND true)
  set(3DconnexionClient_INCLUDE_DIR ${3DconnexionClient})
  set(3DconnexionClient_LIBRARY ${3DconnexionClient})
 endif()
endif()
