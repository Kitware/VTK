# =============================================================================
#
#  Program:   Visualization Toolkit
#  Module:    Find3DconnexionClient.cmake
#
#  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
#  All rights reserved.
#  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
#
#     This software is distributed WITHOUT ANY WARRANTY; without even
#     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#     PURPOSE.  See the above copyright notice for more information.
#
# =============================================================================

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
