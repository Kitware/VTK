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

#ifndef viskores_worklet_contourtree_augmented_data_set_mesh_triangulation_2d_freudenthal_h
#define viskores_worklet_contourtree_augmented_data_set_mesh_triangulation_2d_freudenthal_h

#include <cstdlib>

#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/DataSetMesh.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshtypes/MeshStructureFreudenthal2D.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshtypes/mesh_boundary/ComputeMeshBoundary2D.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshtypes/mesh_boundary/MeshBoundary2D.h>

#include <viskores/cont/ExecutionObjectBase.h>

namespace viskores
{
namespace worklet
{
namespace contourtree_augmented
{

/// Class representing a 2D dataset mesh with freudenthal triangulation connectivity for contour tree computation
class DataSetMeshTriangulation2DFreudenthal
  : public DataSetMesh
  , public viskores::cont::ExecutionObjectBase
{ // class DataSetMeshTriangulation
public:
  /// Constants and case tables
  m2d_freudenthal::EdgeBoundaryDetectionMasksType EdgeBoundaryDetectionMasks;
  static constexpr int MAX_OUTDEGREE = 3;

  //Mesh dependent helper functions
  void SetPrepareForExecutionBehavior(bool getMax);

  /// Prepare mesh for use in Viskores worklets. This function creates a MeshStructureFreudenthal2D
  /// ExecutionObject that implements relevant mesh functions on the device.
  MeshStructureFreudenthal2D PrepareForExecution(viskores::cont::DeviceAdapterId device,
                                                 viskores::cont::Token& token) const;

  /// Constructor
  /// @param meshSize viskores::Id2 object describing the number of vertices in x and y
  DataSetMeshTriangulation2DFreudenthal(viskores::Id2 meshSize);

  /// Helper function to create a boundary excution object for the mesh. The MeshBoundary2DExec object
  /// implements functions for using in worklets in Viskores's execution environment related the boundary
  /// of the mesh.
  MeshBoundary2DExec GetMeshBoundaryExecutionObject() const;

  /// Get boundary vertices
  /// @param[out] boundaryVertexArray Array of boundary vertices
  /// @param[out] boundarySortIndexArray Array of sort index of boundary vertices
  /// @param[in] meshBoundaryExecObj Optional mesh boundary object inluced for consistency with ContourTreeMesh.
  ///                                if omitted, GetMeshBoundaryExecutionObject() will be used.
  void GetBoundaryVertices(IdArrayType& boundaryVertexArray,
                           IdArrayType& boundarySortIndexArray,
                           MeshBoundary2DExec* meshBoundaryExecObj = NULL) const;

  /// Get of global indices of the vertices owned by this mesh. Implemented via
  /// DataSetMesh.GetOwnedVerticesByGlobalIdImpl
  void GetOwnedVerticesByGlobalId(
    const viskores::worklet::contourtree_augmented::mesh_dem::IdRelabeler& localToGlobalIdRelabeler,
    IdArrayType& ownedVertices) const;

private:
  bool UseGetMax; // Define the behavior ofr the PrepareForExecution function
};                // class DataSetMeshTriangulation

// creates input mesh
inline DataSetMeshTriangulation2DFreudenthal::DataSetMeshTriangulation2DFreudenthal(
  viskores::Id2 meshSize)
  : DataSetMesh(viskores::Id3{ meshSize[0], meshSize[1], 1 })
  , EdgeBoundaryDetectionMasks{ viskores::cont::make_ArrayHandle(
      m2d_freudenthal::EdgeBoundaryDetectionMasks,
      m2d_freudenthal::N_INCIDENT_EDGES,
      viskores::CopyFlag::Off) }
{
}

inline void DataSetMeshTriangulation2DFreudenthal::SetPrepareForExecutionBehavior(bool getMax)
{
  this->UseGetMax = getMax;
}

// Get VISKORES execution object that represents the structure of the mesh and provides the mesh helper functions on the device
inline MeshStructureFreudenthal2D DataSetMeshTriangulation2DFreudenthal::PrepareForExecution(
  viskores::cont::DeviceAdapterId device,
  viskores::cont::Token& token) const
{
  return MeshStructureFreudenthal2D(viskores::Id2{ this->MeshSize[0], this->MeshSize[1] },
                                    m2d_freudenthal::N_INCIDENT_EDGES,
                                    this->UseGetMax,
                                    this->SortIndices,
                                    this->SortOrder,
                                    this->EdgeBoundaryDetectionMasks,
                                    device,
                                    token);
}

inline MeshBoundary2DExec DataSetMeshTriangulation2DFreudenthal::GetMeshBoundaryExecutionObject()
  const
{
  return MeshBoundary2DExec(viskores::Id2{ this->MeshSize[0], this->MeshSize[1] },
                            this->SortIndices);
}

inline void DataSetMeshTriangulation2DFreudenthal::GetBoundaryVertices(
  IdArrayType& boundaryVertexArray,    // output
  IdArrayType& boundarySortIndexArray, // output
  MeshBoundary2DExec*
    meshBoundaryExecObj // optional input, included for consistency with ContourTreeMesh
) const
{
  viskores::Id numBoundary = 2 * this->MeshSize[1] + 2 * this->MeshSize[0] - 4;
  auto boundaryId = viskores::cont::ArrayHandleIndex(numBoundary);
  ComputeMeshBoundary2D computeMeshBoundary2dWorklet;
  viskores::cont::Invoker invoke;
  invoke(computeMeshBoundary2dWorklet,
         boundaryId,        // input
         this->SortIndices, // input
         (meshBoundaryExecObj == NULL) ? this->GetMeshBoundaryExecutionObject()
                                       : *meshBoundaryExecObj, // input
         boundaryVertexArray,                                  // output
         boundarySortIndexArray                                // output
  );
}

// Overwrite the implemenation from the base DataSetMesh parent class
inline void DataSetMeshTriangulation2DFreudenthal::GetOwnedVerticesByGlobalId(
  const viskores::worklet::contourtree_augmented::mesh_dem::IdRelabeler& localToGlobalIdRelabeler,
  IdArrayType& ownedVertices) const
{
  return this->GetOwnedVerticesByGlobalIdImpl(this, localToGlobalIdRelabeler, ownedVertices);
}

} // namespace contourtree_augmented
} // worklet
} // viskores

#endif
