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


#ifndef viskores_worklet_contourtree_augmented_mergetree_h
#define viskores_worklet_contourtree_augmented_mergetree_h

#include <iomanip>

// local includes
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/PrintVectors.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshtypes/ContourTreeMesh.h>


//VISKORES includes
#include <viskores/Types.h>
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandleConstant.h>

namespace viskores
{
namespace worklet
{
namespace contourtree_augmented
{

class MergeTree
{ // class MergeTree
public:
  // whether it is join or split tree
  bool IsJoinTree;

  // VECTORS INDEXED ON N = SIZE OF DATA

  // the list of nodes is implicit

  // vector of (regular) arcs in the merge tree
  IdArrayType Arcs;

  // vector storing which superarc owns each node
  IdArrayType Superparents;

  // VECTORS INDEXED ON T = SIZE OF TREE

  // vector storing the list of supernodes by ID
  // WARNING: THESE ARE NOT SORTED BY INDEX
  // Instead, they are sorted by hyperarc, secondarily on index
  IdArrayType Supernodes;

  // vector of superarcs in the merge tree
  // stored as supernode indices
  IdArrayType Superarcs;

  // vector of Hyperarcs to which each supernode/arc belongs
  IdArrayType Hyperparents;

  // VECTORS INDEXED ON H = SIZE OF HYPERTREE

  // vector of sort indices for the hypernodes
  IdArrayType Hypernodes;

  // vector of Hyperarcs in the merge tree
  // NOTE: These are supernode IDs, not hypernode IDs
  // because not all Hyperarcs lead to hypernodes
  IdArrayType Hyperarcs;

  // vector to find the first child superarc
  IdArrayType FirstSuperchild;

  // ROUTINES

  // creates merge tree (empty)
  MergeTree(viskores::Id meshSize, bool isJoinTree);

  // debug routine
  void DebugPrint(const char* message, const char* fileName, long lineNum);

  // debug routine for printing the tree for contourtree meshes
  template <typename FieldType>
  void DebugPrintTree(const char* message,
                      const char* fileName,
                      long lineNum,
                      const ContourTreeMesh<FieldType>& mesh);
  // debug routine for printing the tree for regular meshes
  template <typename MeshType>
  void DebugPrintTree(const char* message,
                      const char* fileName,
                      long lineNum,
                      const MeshType& mesh);


}; // class MergeTree


// creates merge tree (empty)
inline MergeTree::MergeTree(viskores::Id meshSize, bool isJoinTree)
  : IsJoinTree(isJoinTree)
  , Supernodes()
  , Superarcs()
  , Hyperparents()
  , Hypernodes()
  , Hyperarcs()
  , FirstSuperchild()
{ // MergeTree()
  // Allocate the arcs array
  // TODO it should be sufficient to just allocate arcs without initializing it with 0s
  viskores::cont::ArrayHandleConstant<viskores::Id> meshSizeNullArray(0, meshSize);
  viskores::cont::Algorithm::Copy(meshSizeNullArray, this->Arcs);

  // Initialize the superparents with NO_SUCH_ELEMENT
  viskores::cont::ArrayHandleConstant<viskores::Id> noSuchElementArray(
    (viskores::Id)NO_SUCH_ELEMENT, meshSize);
  viskores::cont::Algorithm::Copy(noSuchElementArray, this->Superparents);

} // MergeTree()


// debug routine
inline void MergeTree::DebugPrint(const char* message, const char* fileName, long lineNum)
{ // DebugPrint()
#ifdef DEBUG_PRINT
  std::cout << "---------------------------" << std::endl;
  std::cout << std::setw(30) << std::left << fileName << ":" << std::right << std::setw(4)
            << lineNum << std::endl;
  std::cout << std::left << std::string(message) << std::endl;
  std::cout << "Merge Tree Contains:       " << std::endl;
  std::cout << "---------------------------" << std::endl;
  std::cout << std::endl;

  PrintHeader(this->Arcs.GetNumberOfValues());
  PrintIndices("Arcs", this->Arcs);
  PrintIndices("Superparents", this->Superparents);
  std::cout << std::endl;
  PrintHeader(this->Supernodes.GetNumberOfValues());
  PrintIndices("Supernodes", this->Supernodes);
  PrintIndices("Superarcs", this->Superarcs);
  PrintIndices("Hyperparents", this->Hyperparents);
  std::cout << std::endl;
  PrintHeader(this->Hypernodes.GetNumberOfValues());
  PrintIndices("Hypernodes", this->Hypernodes);
  PrintIndices("Hyperarcs", this->Hyperarcs);
  PrintIndices("First Superchild", FirstSuperchild);
  std::cout << std::endl;
#else
  // Prevent unused parameter warning
  (void)message;
  (void)fileName;
  (void)lineNum;
#endif
} // DebugPrint()


template <typename FieldType>
inline void MergeTree::DebugPrintTree(const char* message,
                                      const char* fileName,
                                      long lineNum,
                                      const ContourTreeMesh<FieldType>& mesh)
{
  (void)mesh; // prevent unused parameter warning
  std::cout << std::setw(30) << std::left << fileName << ":" << std::right << std::setw(4)
            << lineNum << std::endl;
  std::cout << std::left << std::string(message) << std::endl;
  std::cout << "MergeTree::DebugPrintTree not implemented for ContourTreeMesh" << std::endl;
}



template <typename MeshType>
inline void MergeTree::DebugPrintTree(const char* message,
                                      const char* fileName,
                                      long lineNum,
                                      const MeshType& mesh)
{ //PrintMergeTree()
#ifdef DEBUG_PRINT
  std::cout << "---------------------------" << std::endl;
  std::cout << std::setw(30) << std::left << fileName << ":" << std::right << std::setw(4)
            << lineNum << std::endl;
  std::cout << std::left << std::string(message) << std::endl;
  if (this->IsJoinTree)
  {
    std::cout << "Join Tree:" << std::endl;
  }
  else
  {
    std::cout << "Split Tree:" << std::endl;
  }
  std::cout << "---------------------------" << std::endl;
  std::cout << std::endl;

  std::cout << "==========" << std::endl;

  for (viskores::Id entry = 0; entry < mesh.NumVertices; entry++)
  {
    viskores::Id sortIndex = viskores::cont::ArrayGetValue(entry, mesh.SortIndices);
    viskores::Id arc = viskores::cont::ArrayGetValue(sortIndex, this->Arcs);
    if (NoSuchElement(arc))
    {
      std::cout << "-1" << std::endl;
    }
    else
    {
      std::cout << viskores::cont::ArrayGetValue(arc, mesh.SortOrder) << std::endl;
    }
    if (mesh.MeshSize[2] == 1)
    { // 2D Mesh
      if ((entry % mesh.MeshSize[0]) == (mesh.MeshSize[0] - 1))
      {
        std::cout << std::endl;
      }
    }
    else
    { // 3D Mesh
      if ((entry % (mesh.MeshSize[0] * mesh.MeshSize[1])) ==
          (mesh.MeshSize[0] * mesh.MeshSize[1] - 1))
      {
        std::cout << std::endl;
      }
    }
  }
  std::cout << std::endl;
#else
  // Prevent unused parameter warning
  (void)message;
  (void)fileName;
  (void)lineNum;
  (void)mesh;
#endif
} // PrintMergeTree()

} // namespace contourtree_augmented
} // worklet
} // viskores

#endif
