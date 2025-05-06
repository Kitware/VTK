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

#ifndef viskores_worklet_contourtree_augmented_meshextrema_h
#define viskores_worklet_contourtree_augmented_meshextrema_h

#include <iomanip>

// local includes
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/Invoker.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/PrintVectors.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshextrema/PointerDoubling.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/meshextrema/SetStarts.h>


#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/data_set_mesh/SortIndices.h>

namespace mesh_extrema_inc_ns = viskores::worklet::contourtree_augmented::mesh_extrema_inc;

namespace viskores
{
namespace worklet
{
namespace contourtree_augmented
{

class MeshExtrema
{ // MeshExtrema
public:
  viskores::cont::Invoker Invoke;
  // arrays for peaks & pits
  IdArrayType Peaks;
  IdArrayType Pits;
  viskores::Id NumVertices;
  viskores::Id NumLogSteps;

  // constructor
  VISKORES_CONT
  MeshExtrema(viskores::Id meshSize);

  // routine to initialise the array before chaining
  template <class MeshType>
  void SetStarts(MeshType& mesh, bool isMaximal);

  // routine that computes regular chains in a merge tree
  VISKORES_CONT
  void BuildRegularChains(bool isMaximal);

  // debug routine
  VISKORES_CONT
  void DebugPrint(const char* message, const char* fileName, long lineNum);

}; // MeshExtrema


inline MeshExtrema::MeshExtrema(viskores::Id meshSize)
  : Peaks()
  , Pits()
  , NumVertices(meshSize)
  , NumLogSteps(0)
{ // MeshExrema
  // Compute the number of log steps required in this pass
  NumLogSteps = 1;
  for (viskores::Id shifter = NumVertices; shifter != 0; shifter >>= 1)
    this->NumLogSteps++;

  // Allocate memory for the peaks and pits
  Peaks.Allocate(NumVertices);
  Pits.Allocate(NumVertices);
  // TODO Check if we really need to set the Peaks and pits to zero or whether it is enough to allocate them
  viskores::cont::ArrayHandleConstant<viskores::Id> constZeroArray(0, NumVertices);
  viskores::cont::Algorithm::Copy(constZeroArray, Peaks);
  viskores::cont::Algorithm::Copy(constZeroArray, Pits);
} // MeshExtrema


inline void MeshExtrema::BuildRegularChains(bool isMaximal)
{ // BuildRegularChains()
  // Create vertex index array -- note, this is a fancy viskores array, i.e, the full array
  // is not actually allocated but the array only acts like a sequence of numbers
  viskores::cont::ArrayHandleIndex vertexIndexArray(NumVertices);
  IdArrayType& extrema = isMaximal ? Peaks : Pits;

  // Create the PointerDoubling worklet and corresponding dispatcher
  viskores::worklet::contourtree_augmented::mesh_extrema_inc::PointerDoubling pointerDoubler;

  // Iterate to perform pointer-doubling to build chains to extrema (i.e., maxima or minima)
  // depending on whether we are computing a JoinTree or a SplitTree
  for (viskores::Id logStep = 0; logStep < this->NumLogSteps; logStep++)
  {
    this->Invoke(pointerDoubler,
                 vertexIndexArray, // input
                 extrema);         // output. Update whole extrema array during pointer doubling
  }
  DebugPrint("Regular Chains Built", __FILE__, __LINE__);
} // BuildRegularChains()

template <class MeshType>
inline void MeshExtrema::SetStarts(MeshType& mesh, bool isMaximal)
{
  mesh.SetPrepareForExecutionBehavior(isMaximal);
  mesh_extrema_inc_ns::SetStarts setStartsWorklet;
  viskores::cont::ArrayHandleIndex sortIndexArray(mesh.NumVertices);
  if (isMaximal)
  {
    this->Invoke(setStartsWorklet,
                 sortIndexArray, // input
                 mesh,           // input
                 Peaks);         // output
  }
  else
  {
    this->Invoke(setStartsWorklet,
                 sortIndexArray, // input
                 mesh,           // input
                 Pits);          // output
  }
  DebugPrint("Regular Starts Set", __FILE__, __LINE__);
}


// debug routine
inline void MeshExtrema::DebugPrint(const char* message, const char* fileName, long lineNum)
{ // DebugPrint()
#ifdef DEBUG_PRINT
  std::cout << "---------------------------" << std::endl;
  std::cout << std::setw(30) << std::left << fileName << ":" << std::right << std::setw(4)
            << lineNum << std::endl;
  std::cout << std::left << std::string(message) << std::endl;
  std::cout << "Mesh Extrema Contain:      " << std::endl;
  std::cout << "---------------------------" << std::endl;
  std::cout << std::endl;

  PrintHeader(Peaks.GetNumberOfValues());
  PrintIndices("Peaks", Peaks);
  PrintIndices("Pits", Pits);
#else
  // Prevent unused parameter warning
  (void)message;
  (void)fileName;
  (void)lineNum;
#endif
} // DebugPrint()


} // namespace contourtree_augmented
} // worklet
} // viskores

#endif
