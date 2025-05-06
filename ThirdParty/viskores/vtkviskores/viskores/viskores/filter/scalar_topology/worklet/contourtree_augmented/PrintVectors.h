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

// include guard
#ifndef viskores_worklet_contourtree_augmented_print_vectors_h
#define viskores_worklet_contourtree_augmented_print_vectors_h

// global libraries
#include <iomanip>
#include <iostream>
#include <string>

// local includes
#include <viskores/cont/arg/Transport.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>


namespace viskores
{
namespace worklet
{
namespace contourtree_augmented
{

// local constants to allow changing the spacing as needed
constexpr int PRINT_WIDTH = 18;
constexpr int PREFIX_WIDTH = 36;

template <typename T, typename StorageType>
void PrintValues(std::string label,
                 const viskores::cont::ArrayHandle<T, StorageType>& dVec,
                 viskores::Id nValues = -1,
                 std::ostream& outStream = std::cout);
template <typename T>
void PrintIndices(std::string label,
                  const viskores::cont::ArrayHandle<T>& iVec,
                  viskores::Id nIndices = -1,
                  std::ostream& outStream = std::cout);
template <typename T>
void PrintArray(std::string label,
                const T& iVec,
                viskores::Id nIndices = -1,
                std::ostream& outStream = std::cout);
template <typename T, typename StorageType>
void PrintSortedValues(std::string label,
                       const viskores::cont::ArrayHandle<T, StorageType>& dVec,
                       IdArrayType& sortVec,
                       viskores::Id nValues = -1,
                       std::ostream& outStream = std::cout);

// base routines for printing label & prefix bars
inline void PrintLabel(std::string label, std::ostream& outStream = std::cout)
{ // PrintLabel()
  // print out the front end
  outStream << std::setw(PREFIX_WIDTH) << std::left << label;
  // print out the vertical line
  outStream << std::right << "|";
} // PrintLabel()


inline void PrintSeparatingBar(viskores::Id howMany, std::ostream& outStream = std::cout)
{ // PrintSeparatingBar()
  // print out the front end
  outStream << std::setw(PREFIX_WIDTH) << std::setfill('-') << "";
  // now the + at the vertical line
  outStream << "+";
  // now print out the tail end - fixed number of spaces per entry
  for (viskores::Id block = 0; block < howMany; block++)
  {
    outStream << std::setw(PRINT_WIDTH) << std::setfill('-') << "";
  }
  // now the std::endl, resetting the fill character
  outStream << std::setfill(' ') << std::endl;
} // PrintSeparatingBar()


// routine to print out a single index
inline void PrintIndexType(viskores::Id index, std::ostream& outStream = std::cout)
{ // PrintIndexType
  outStream << std::setw(PRINT_WIDTH - 6) << MaskedIndex(index) << " " << FlagString(index);
} // PrintIndexType


// routine to print out a single value
template <typename T>
inline void PrintDataType(T value, std::ostream& outStream = std::cout)
{ // PrintDataType
  outStream << std::setw(PRINT_WIDTH) << value;
} // PrintDataType


// Specialization of PrintDataType for viskores::Id to use PrintIndexType instead so we can properly
// print Id arrays using the PrintArrayHandle function, e.g,. to pint permutted Id arrays.
template <>
inline void PrintDataType<viskores::Id>(viskores::Id value, std::ostream& outStream)
{
  PrintIndexType(value, outStream);
}


// header line
inline void PrintHeader(viskores::Id howMany, std::ostream& outStream = std::cout)
{ // PrintHeader()
  // print out a separating bar
  PrintSeparatingBar(howMany, outStream);
  // print out a label
  PrintLabel("ID", outStream);
  // print out the ID numbers
  for (viskores::Id entry = 0; entry < howMany; entry++)
  {
    PrintIndexType(entry, outStream);
  }
  // and an std::endl
  outStream << std::endl;
  // print out another separating bar
  PrintSeparatingBar(howMany, outStream);
} // PrintHeader()


// base routines for reading & writing host vectors
template <typename ARRAYTYPE>
inline void PrintArrayHandle(std::string label,
                             const ARRAYTYPE& dVec,
                             viskores::Id nValues,
                             std::ostream& outStream)
{ // PrintArrayHandle()
  // -1 means full size
  if (nValues == -1)
  {
    nValues = dVec.GetNumberOfValues();
  }

  // print the label
  PrintLabel(label, outStream);

  // now print the data
  auto portal = dVec.ReadPortal();
  for (viskores::Id entry = 0; entry < nValues; entry++)
  {
    PrintDataType(portal.Get(entry), outStream);
  }
  // and an std::endl
  outStream << std::endl;
} // PrintArrayHandle()

// base routines for reading & writing host vectors
template <typename T, typename StorageType>
inline void PrintValues(std::string label,
                        const viskores::cont::ArrayHandle<T, StorageType>& dVec,
                        viskores::Id nValues,
                        std::ostream& outStream)
{ // PrintValues()
  // -1 means full size
  if (nValues == -1)
  {
    nValues = dVec.GetNumberOfValues();
  }

  // print the label
  PrintLabel(label, outStream);

  // now print the data
  auto portal = dVec.ReadPortal();
  for (viskores::Id entry = 0; entry < nValues; entry++)
  {
    PrintDataType(portal.Get(entry), outStream);
  }
  // and an std::endl
  outStream << std::endl;
} // PrintValues()


// base routines for reading & writing host vectors
template <typename T, typename StorageType>
inline void PrintSortedValues(std::string label,
                              const viskores::cont::ArrayHandle<T, StorageType>& dVec,
                              IdArrayType& sortVec,
                              viskores::Id nValues,
                              std::ostream& outStream)
{ // PrintSortedValues()
  // -1 means full size
  if (nValues == -1)
  {
    nValues = sortVec.GetNumberOfValues();
  }

  // print the label
  PrintLabel(label, outStream);

  // now print the data
  auto dportal = dVec.ReadPortal();
  auto sortPortal = sortVec.ReadPortal();
  for (viskores::Id entry = 0; entry < nValues; entry++)
  {
    PrintDataType(dportal.Get(sortPortal.Get(entry)), outStream);
  }
  // and an std::endl
  outStream << std::endl;
} // PrintSortedValues()


// routine for printing index arrays
template <typename T>
inline void PrintIndices(std::string label,
                         const viskores::cont::ArrayHandle<T>& iVec,
                         viskores::Id nIndices,
                         std::ostream& outStream)
{ // PrintIndices()
  // -1 means full size
  if (nIndices == -1)
  {
    nIndices = iVec.GetNumberOfValues();
  }

  // print the label
  PrintLabel(label, outStream);

  auto portal = iVec.ReadPortal();
  for (viskores::Id entry = 0; entry < nIndices; entry++)
    PrintIndexType(portal.Get(entry), outStream);

  // and the std::endl
  outStream << std::endl;
} // PrintIndices()

// routine for printing index arrays
template <typename T>
inline void PrintArray(std::string label,
                       const T& iVec,
                       viskores::Id nArray,
                       std::ostream& outStream)
{ // PrintArray()
  // -1 means full size
  if (nArray == -1)
  {
    nArray = iVec.GetNumberOfValues();
  }

  // print the label
  PrintLabel(label, outStream);

  auto portal = iVec.ReadPortal();
  for (viskores::Id entry = 0; entry < nArray; entry++)
    PrintIndexType(portal.Get(entry), outStream);

  // and the std::endl
  outStream << std::endl;
} // PrintArray()

template <typename T, typename StorageType>
inline void PrintLabelledDataBlock(std::string label,
                                   const viskores::cont::ArrayHandle<T, StorageType>& dVec,
                                   viskores::Id nColumns,
                                   std::ostream& outStream = std::cout)
{ // PrintLabelledDataBlock()
  // start with a header
  PrintHeader(nColumns, outStream);
  // loop control variable
  viskores::Id entry = 0;
  // per row
  auto portal = dVec.ReadPortal();
  for (viskores::Id row = 0; entry < portal.GetNumberOfValues(); row++)
  { // per row
    PrintLabel(label + "[" + std::to_string(row) + "]", outStream);
    // now print the data
    for (viskores::Id col = 0; col < nColumns; col++, entry++)
    {
      PrintDataType(portal.Get(entry), outStream);
    }
    outStream << std::endl;
  } // per row
  // and a final std::endl
  outStream << std::endl;
} // PrintLabelledDataBlock()


// routine for printing list of edge pairs in row format, i.e., First and Second of the Edge
// are printed separated. Used, e.g.,in standard debug.
inline void PrintEdgePairArray(std::string label,
                               const EdgePairArray& edgePairArray,
                               viskores::Id nIndices,
                               std::ostream& outStream = std::cout)
{ // PrintEdgePairArray()
  // -1 means full size
  if (nIndices == -1)
  {
    nIndices = edgePairArray.GetNumberOfValues();
  }
  // now print them out
  auto edgePairArrayConstPortal = edgePairArray.ReadPortal();

  // print the low end
  PrintLabel(label + " High", outStream);
  for (viskores::Id superarc = 0; superarc < nIndices; superarc++)
  { // per superarc
    PrintIndexType(edgePairArrayConstPortal.Get(superarc).second, outStream);
  } // per superarc
  outStream << std::endl;

  // print the high end
  PrintLabel(label + " Low", outStream);
  for (viskores::Id superarc = 0; superarc < nIndices; superarc++)
  { // per edge
    PrintIndexType(edgePairArrayConstPortal.Get(superarc).first, outStream);
  }
  outStream << std::endl;
} // PrintEdgePairArray()


// routine for printing list of edge pairs in column format (first, second).
// Used, e.g., to print the sorted list of saddle peaks from the ContourTree
inline void PrintEdgePairArrayColumnLayout(const EdgePairArray& edgePairArray,
                                           std::ostream& outStream = std::cout)
{ // PrintEdgePairArray()
  // now print them out
  auto edgePairArrayConstPortal = edgePairArray.ReadPortal();
  for (viskores::Id superarc = 0; superarc < edgePairArray.GetNumberOfValues(); superarc++)
  { // per superarc
    outStream << std::right << std::setw(PRINT_WIDTH)
              << edgePairArrayConstPortal.Get(superarc).first << " ";
    outStream << std::right << std::setw(PRINT_WIDTH)
              << edgePairArrayConstPortal.Get(superarc).second << std::endl;
  } // per superarc
} // PrintEdgePairArray()


} // namespace contourtree_augmented
} // worklet
} // viskores

// tail of include guard
#endif
