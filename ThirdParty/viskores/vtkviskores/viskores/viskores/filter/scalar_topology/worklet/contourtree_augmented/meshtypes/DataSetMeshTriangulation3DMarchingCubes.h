//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
// Copyright (c) 2018, The Regents of the University of California, through
// Lawrence Berkeley National Laboratory (subject to receipt of any required approvals
// from the U.S. Dept. of Energy).  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// (1) Redistributions of source code must retain the above copyright notice, this
//     list of conditions and the following disclaimer.
//
// (2) Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
// (3) Neither the name of the University of California, Lawrence Berkeley National
//     Laboratory, U.S. Dept. of Energy nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//
//=============================================================================
//
//  This code is an extension of the algorithm presented in the paper:
//  Parallel Peak Pruning for Scalable SMP Contour Tree Computation.
//  Hamish Carr, Gunther Weber, Christopher Sewell, and James Ahrens.
//  Proceedings of the IEEE Symposium on Large Data Analysis and Visualization
//  (LDAV), October 2016, Baltimore, Maryland.
//
//  The PPP2 algorithm and software were jointly developed by
//  Hamish Carr (University of Leeds), Gunther H. Weber (LBNL), and
//  Oliver Ruebel (LBNL)
//==============================================================================


#ifndef viskores_worklet_contourtree_augmented_data_set_mesh_triangulation_3d_marchingcubes_h
#define viskores_worklet_contourtree_augmented_data_set_mesh_triangulation_3d_marchingcubes_h

#include <cstdlib>

#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/DataSetMesh.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshtypes/MeshStructureMarchingCubes.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshtypes/marchingcubes_3D/Types.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshtypes/mesh_boundary/ComputeMeshBoundary3D.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshtypes/mesh_boundary/MeshBoundary3D.h>

namespace viskores
{
namespace worklet
{
namespace contourtree_augmented
{

class DataSetMeshTriangulation3DMarchingCubes
  : public DataSetMesh
  , public viskores::cont::ExecutionObjectBase
{ // class DataSetMeshTriangulation3DMarchingCubes
public:
  //Constants and case tables

  m3d_marchingcubes::EdgeBoundaryDetectionMasksType EdgeBoundaryDetectionMasks;
  m3d_marchingcubes::CubeVertexPermutationsType CubeVertexPermutations;
  m3d_marchingcubes::LinkVertexConnectionsType LinkVertexConnectionsSix;
  m3d_marchingcubes::LinkVertexConnectionsType LinkVertexConnectionsEighteen;
  m3d_marchingcubes::InCubeConnectionsType InCubeConnectionsSix;
  m3d_marchingcubes::InCubeConnectionsType InCubeConnectionsEighteen;
  static constexpr int MAX_OUTDEGREE = 6; // True for Freudenthal and Marching Cubes

  // mesh depended helper functions
  void SetPrepareForExecutionBehavior(bool getMax);

  MeshStructureMarchingCubes PrepareForExecution(viskores::cont::DeviceAdapterId device,
                                                 viskores::cont::Token& token) const;

  DataSetMeshTriangulation3DMarchingCubes(viskores::Id3 meshSize);

  MeshBoundary3DExec<true> GetMeshBoundaryExecutionObject() const;

  void GetBoundaryVertices(
    IdArrayType& boundaryVertexArray,    // output
    IdArrayType& boundarySortIndexArray, // output
    MeshBoundary3DExec<true>* meshBoundaryExecObj =
      nullptr // optional input, included for consistency with ContourTreeMesh
  ) const;

  /// Get of global indices of the vertices owned by this mesh. Implemented via
  /// DataSetMesh.GetOwnedVerticesByGlobalIdImpl.
  void GetOwnedVerticesByGlobalId(
    const viskores::worklet::contourtree_augmented::mesh_dem::IdRelabeler& localToGlobalIdRelabeler,
    IdArrayType& ownedVertices) const;

private:
  bool UseGetMax; // Define the behavior ofr the PrepareForExecution function
};                // class DataSetMesh_Triangulation

// creates input mesh
inline DataSetMeshTriangulation3DMarchingCubes::DataSetMeshTriangulation3DMarchingCubes(
  viskores::Id3 meshSize)
  : DataSetMesh(meshSize)

{
  // Initialize the case tables in viskores
  this->EdgeBoundaryDetectionMasks =
    viskores::cont::make_ArrayHandle(m3d_marchingcubes::EdgeBoundaryDetectionMasks,
                                     m3d_marchingcubes::N_ALL_NEIGHBOURS,
                                     viskores::CopyFlag::Off);
  this->CubeVertexPermutations = viskores::cont::make_ArrayHandleGroupVec<
    m3d_marchingcubes::
      CubeVertexPermutations_PermVecLength>( // create 2D array of vectors of lenghts ...PermVecLength
    viskores::cont::make_ArrayHandle(
      m3d_marchingcubes::CubeVertexPermutations, // the array to convert
      m3d_marchingcubes::CubeVertexPermutations_NumPermutations *
        m3d_marchingcubes::CubeVertexPermutations_PermVecLength, // total number of elements
      viskores::CopyFlag::Off));
  this->LinkVertexConnectionsSix = viskores::cont::make_ArrayHandleGroupVec<
    m3d_marchingcubes::
      VertexConnections_VecLength>( // create 2D array of vectors o lenght ...VecLength
    viskores::cont::make_ArrayHandle(
      m3d_marchingcubes::LinkVertexConnectionsSix, // the array to convert
      m3d_marchingcubes::LinkVertexConnectionsSix_NumPairs *
        m3d_marchingcubes::VertexConnections_VecLength, // total number of elements
      viskores::CopyFlag::Off));
  this->LinkVertexConnectionsEighteen = viskores::cont::make_ArrayHandleGroupVec<
    m3d_marchingcubes::
      VertexConnections_VecLength>( // create 2D array of vectors o lenght ...VecLength
    viskores::cont::make_ArrayHandle(
      m3d_marchingcubes::LinkVertexConnectionsEighteen, // the array to convert
      m3d_marchingcubes::LinkVertexConnectionsEighteen_NumPairs *
        m3d_marchingcubes::VertexConnections_VecLength, // total number of elements
      viskores::CopyFlag::Off));
  this->InCubeConnectionsSix =
    viskores::cont::make_ArrayHandle(m3d_marchingcubes::InCubeConnectionsSix,
                                     m3d_marchingcubes::InCubeConnectionsSix_NumElements,
                                     viskores::CopyFlag::Off);
  this->InCubeConnectionsEighteen =
    viskores::cont::make_ArrayHandle(m3d_marchingcubes::InCubeConnectionsEighteen,
                                     m3d_marchingcubes::InCubeConnectionsEighteen_NumElements,
                                     viskores::CopyFlag::Off);
}

inline void DataSetMeshTriangulation3DMarchingCubes::SetPrepareForExecutionBehavior(bool getMax)
{
  this->UseGetMax = getMax;
}

// Get VISKORES execution object that represents the structure of the mesh and provides the mesh helper functions on the device
inline MeshStructureMarchingCubes DataSetMeshTriangulation3DMarchingCubes::PrepareForExecution(
  viskores::cont::DeviceAdapterId device,
  viskores::cont::Token& token) const
{
  return MeshStructureMarchingCubes(this->MeshSize,
                                    this->UseGetMax,
                                    this->SortIndices,
                                    this->SortOrder,
                                    this->EdgeBoundaryDetectionMasks,
                                    this->CubeVertexPermutations,
                                    this->LinkVertexConnectionsSix,
                                    this->LinkVertexConnectionsEighteen,
                                    this->InCubeConnectionsSix,
                                    this->InCubeConnectionsEighteen,
                                    device,
                                    token);
}

inline MeshBoundary3DExec<true>
DataSetMeshTriangulation3DMarchingCubes::GetMeshBoundaryExecutionObject() const
{
  return MeshBoundary3DExec<true>(this->MeshSize, this->SortIndices);
}

inline void DataSetMeshTriangulation3DMarchingCubes::GetBoundaryVertices(
  IdArrayType& boundaryVertexArray,             // output
  IdArrayType& boundarySortIndexArray,          // output
  MeshBoundary3DExec<true>* meshBoundaryExecObj // input
) const
{
  viskores::Id numBoundary = 2 * this->MeshSize[1] * this->MeshSize[0] // xy faces
    + 2 * this->MeshSize[1] * (this->MeshSize[2] - 2)        // yz faces - excluding vertices on xy
    + 2 * (this->MeshSize[0] - 2) * (this->MeshSize[2] - 2); // xz face interiors
  auto boundaryId = viskores::cont::ArrayHandleIndex(numBoundary);
  ComputeMeshBoundary3D computeMeshBoundary3dWorklet;
  viskores::cont::Invoker invoke;
  invoke(computeMeshBoundary3dWorklet,
         boundaryId,        // input
         this->SortIndices, // input
         (meshBoundaryExecObj == NULL) ? this->GetMeshBoundaryExecutionObject()
                                       : *meshBoundaryExecObj, // input
         boundaryVertexArray,                                  // output
         boundarySortIndexArray                                // output
  );
}

// Overwrite the implemenation from the base DataSetMesh parent class
inline void DataSetMeshTriangulation3DMarchingCubes::GetOwnedVerticesByGlobalId(
  const viskores::worklet::contourtree_augmented::mesh_dem::IdRelabeler& localToGlobalIdRelabeler,
  IdArrayType& ownedVertices) const
{
  return this->GetOwnedVerticesByGlobalIdImpl(this, localToGlobalIdRelabeler, ownedVertices);
}

} // namespace contourtree_augmented
} // worklet
} // viskores

#endif
