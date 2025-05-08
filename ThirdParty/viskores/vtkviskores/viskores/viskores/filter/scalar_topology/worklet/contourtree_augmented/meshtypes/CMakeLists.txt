##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
set(headers
  DataSetMeshTriangulation2DFreudenthal.h
  DataSetMeshTriangulation3DFreudenthal.h
  DataSetMeshTriangulation3DMarchingCubes.h
  ContourTreeMesh.h
  MeshStructureFreudenthal2D.h
  MeshStructureFreudenthal3D.h
  MeshStructureMarchingCubes.h
  MeshStructureContourTreeMesh.h
  )

#----------------------------------------------------------------------------
add_subdirectory(contourtreemesh)
add_subdirectory(mesh_boundary)

#-----------------------------------------------------------------------------
viskores_declare_headers(${headers})


#-----------------------------------------------------------------------------
set(extra_headers
  freudenthal_2D/Types.h
  freudenthal_3D/Types.h
  marchingcubes_3D/Types.h
  )

#we do the following to work around a couple of issues.
#1. viskores_declare_headers will do a flat install into the current respective
#   build directory
viskores_get_kit_name(name dir_prefix)
viskores_install_headers("${dir_prefix}/freudenthal_2D" freudenthal_2D/Types.h)
viskores_install_headers("${dir_prefix}/freudenthal_3D" freudenthal_3D/Types.h)
viskores_install_headers("${dir_prefix}/marchingcubes_3D" marchingcubes_3D/Types.h)
