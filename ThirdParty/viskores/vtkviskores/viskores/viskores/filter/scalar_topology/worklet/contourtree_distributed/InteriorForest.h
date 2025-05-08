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
//================================================================================
//
//  Parallel Peak Pruning v. 2.0
//
//  Started June 15, 2017
//
// Copyright Hamish Carr, University of Leeds
//
// TreeResidue.h - A data structure storing the residue information for transfer
//                to the grafting phase
//
//================================================================================
//
// COMMENTS:
//
//
//================================================================================


#ifndef viskores_worklet_contourtree_distributed_interior_forest_h
#define viskores_worklet_contourtree_distributed_interior_forest_h

#include <viskores/Types.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/PrintVectors.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>

#include <sstream>
#include <string>
#include <utility>

namespace viskores
{
namespace worklet
{
namespace contourtree_distributed
{

/// \brief The contour tree of a data block restricted to the interior of a data block
///
/// The Boundary Restricted Augemented Contour Tree (BRACT) represents the contours
/// that cross the boundary of a data block. In contrast, this class represents all contours
/// that remain in the interior. Alternatively, the interior tree can be interpreted as the
/// residue (i.e., the part that are left over) from removing the BRACT from the full contour tree.
/// of a data block
class InteriorForest
{ // class InteriorForest
public:
  // array of vertices in the bract (by mesh index)
  viskores::worklet::contourtree_augmented::IdArrayType BoundaryTreeMeshIndices;

  // array of flags for whether necessary (i.e. needed in the BRACT)
  viskores::worklet::contourtree_augmented::IdArrayType IsNecessary;

  // arrays of nodes above and below supernodes for hierarchical search
  // stored as global IDs
  viskores::worklet::contourtree_augmented::IdArrayType Above;
  viskores::worklet::contourtree_augmented::IdArrayType Below;

  // constructor
  InteriorForest() {}

  // prints the contents of the object in a standard format
  void PrintContent(std::ostream& outStream) const;
  std::string DebugPrint(const char* message, const char* fileName, long lineNum) const;
  inline std::string PrintArraySizes() const;
};


// debug routine
inline void InteriorForest::PrintContent(std::ostream& outStream) const
{
  // Per Supernode Arrays
  viskores::worklet::contourtree_augmented::PrintHeader(this->IsNecessary.GetNumberOfValues(),
                                                        outStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "IsNecessary", this->IsNecessary, -1, outStream);
  viskores::worklet::contourtree_augmented::PrintIndices("Above", this->Above, -1, outStream);
  viskores::worklet::contourtree_augmented::PrintIndices("Below", this->Below, -1, outStream);

  // BRACT Sized Arrays
  viskores::worklet::contourtree_augmented::PrintHeader(
    this->BoundaryTreeMeshIndices.GetNumberOfValues(), outStream);
  viskores::worklet::contourtree_augmented::PrintIndices(
    "BRACT Mesh Indices", this->BoundaryTreeMeshIndices, -1, outStream);
}

inline std::string InteriorForest::DebugPrint(const char* message,
                                              const char* fileName,
                                              long lineNum) const
{ // DebugPrint
  std::stringstream resultStream;
  resultStream << "[CUTHERE]---------------------------------------------" << std::endl;
  resultStream << std::setw(30) << std::left << fileName << ":" << std::right << std::setw(4)
               << lineNum << " ";
  resultStream << std::left << std::string(message) << std::endl;

  resultStream << "------------------------------------------------------" << std::endl;
  resultStream << "Residue Contains:                                     " << std::endl;
  resultStream << "------------------------------------------------------" << std::endl;

  this->PrintContent(resultStream);

  resultStream << "------------------------------------------------------" << std::endl;
  resultStream << std::endl;
  return resultStream.str();
} // DebugPrint

inline std::string InteriorForest::PrintArraySizes() const
{ // PrintArraySizes
  std::stringstream arraySizeLog;
  arraySizeLog << std::setw(42) << std::left << "    #BoundaryTreeMeshIndices"
               << ": " << this->BoundaryTreeMeshIndices.GetNumberOfValues() << std::endl
               << std::setw(42) << std::left << "    #IsNecessary"
               << ": " << this->IsNecessary.GetNumberOfValues() << std::endl
               << std::setw(42) << std::left << "    #Above"
               << ": " << this->Above.GetNumberOfValues() << std::endl
               << std::setw(42) << std::left << "    #Below"
               << ": " << this->Below.GetNumberOfValues() << std::endl;
  return arraySizeLog.str();
} // PrintArraySizes

} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores

#endif
