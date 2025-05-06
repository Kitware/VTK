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


#ifndef _TREECOMPILER_H_
#define _TREECOMPILER_H_

#include <iomanip>
#include <iostream>
#include <viskores/Types.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/DataSet.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>

namespace viskores
{
namespace worklet
{
namespace contourtree_distributed
{
// Possibly change the following when comapring to PPP prototype
constexpr int PRINT_WIDTH = 12;
using dataType = viskores::Float64;
using indexType = viskores::Id;

// small class for storing the contour arcs
class Edge
{ // Edge
public:
  indexType low, high;

  // constructor - defaults to -1
  Edge(viskores::Id Low = -1, viskores::Id High = -1)
    : low(Low)
    , high(High)
  {
  }
}; // Edge

// comparison operator <
inline bool operator<(const Edge& LHS, const Edge& RHS)
{ // operator <
#if 0
  if (LHS.low < RHS.low) return true;
  if (LHS.low > RHS.low) return false;
  if (LHS.high < RHS.high) return true;
  if (LHS.high > RHS.high) return false;
#endif
  if (std::min(LHS.low, LHS.high) < std::min(RHS.low, RHS.high))
    return true;
  else if (std::min(LHS.low, LHS.high) > std::min(RHS.low, RHS.high))
    return false;
  if (std::max(LHS.low, LHS.high) < std::max(RHS.low, RHS.high))
    return true;
  else if (std::max(LHS.low, LHS.high) > std::max(RHS.low, RHS.high))
    return false;
  return false;
} // operator <

// comparison operator ==
inline bool operator==(const Edge& LHS, const Edge& RHS)
{ // operator ==
  return (LHS.low == RHS.low && LHS.high == RHS.high) ||
    (LHS.low == RHS.high && LHS.high == RHS.low);
} // operator ==

// a helper class which stores a single supernode inserted onto a superarc
class SupernodeOnSuperarc
{ // class SupernodeOnSuperarc
public:
  // the global ID of the supernode
  indexType globalID;
  // the data value stored at the supernode
  dataType dataValue;

  // the low and high ends of the superarc it is on (may be itself)
  indexType lowEnd, highEnd;

  // constructor
  SupernodeOnSuperarc(
    indexType GlobalID = viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT,
    dataType DataValue = viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT,
    indexType LowEnd = viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT,
    indexType HighEnd = viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT)
    : globalID(GlobalID)
    , dataValue(DataValue)
    , lowEnd(LowEnd)
    , highEnd(HighEnd)
  { // constructor
  } // constructor
};  // class SupernodeOnSuperarc

// overloaded comparison operator
// primary sort is by superarc (low, high),
// then secondary sort on datavalue
// tertiary on globalID to implement simulated simplicity
inline bool operator<(const SupernodeOnSuperarc& left, const SupernodeOnSuperarc& right)
{ // < operator
  // simple lexicographic sort
  if (left.lowEnd < right.lowEnd)
    return true;
  if (left.lowEnd > right.lowEnd)
    return false;
  if (left.highEnd < right.highEnd)
    return true;
  if (left.highEnd > right.highEnd)
    return false;
  if (left.dataValue < right.dataValue)
    return true;
  if (left.dataValue > right.dataValue)
    return false;
  if (left.globalID < right.globalID)
    return true;
  if (left.globalID > right.globalID)
    return false;

  // fall-through (shouldn't happen, but)
  // if they're the same, it's false
  return false;
} // < operator

// stream output
std::ostream& operator<<(std::ostream& outStream, SupernodeOnSuperarc& node);

// stream input
std::istream& operator>>(std::istream& inStream, SupernodeOnSuperarc& node);

// the class that compiles the contour tree
class TreeCompiler
{ // class TreeCompiler
public:
  // we want a vector of supernodes on superarcs
  std::vector<SupernodeOnSuperarc> supernodes;

  // and a vector of Edges (the output)
  std::vector<Edge> superarcs;

  // routine to add a known hierarchical tree to it
  // note that this DOES NOT finalise - we don't want too many sorts
  void AddHierarchicalTree(const viskores::cont::DataSet& addedTree);

  // routine to compute the actual superarcs
  void ComputeSuperarcs();

  // routine to print a superarcs array in our format
  static void PrintSuperarcArray(const std::vector<Edge>& superarc_array);

  // routine to print the superarcs
  void PrintSuperarcs(bool) const;

  // routine to write out binary file
  void WriteBinary(FILE* outFile) const;

  // routine to read in binary file & append to contents
  void ReadBinary(FILE* inFile);
}; // class TreeCompiler

// stream output
inline std::ostream& operator<<(std::ostream& outStream, SupernodeOnSuperarc& node)
{ // stream output
  outStream << node.lowEnd << " " << node.highEnd << " " << node.dataValue << " " << node.globalID
            << std::endl;
  return outStream;
} // stream output

// stream input
inline std::istream& operator>>(std::istream& inStream, SupernodeOnSuperarc& node)
{ // stream input
  inStream >> node.lowEnd >> node.highEnd >> node.dataValue >> node.globalID;
  return inStream;
} // stream input

// routine to add a known hierarchical tree to it
// note that this DOES NOT finalise - we don't want too many sorts
inline void TreeCompiler::AddHierarchicalTree(const viskores::cont::DataSet& addedTree)
{ // TreeCompiler::AddHierarchicalTree()
  // Copy relevant tree content to STL arrays
  viskores::cont::UnknownArrayHandle dataValues_array = addedTree.GetField("DataValues").GetData();
  std::vector<viskores::Float64> dataValues(dataValues_array.GetNumberOfValues());
  auto dataValues_handle = viskores::cont::make_ArrayHandle(dataValues, viskores::CopyFlag::Off);
  viskores::cont::ArrayCopy(dataValues_array, dataValues_handle);
  dataValues_handle.SyncControlArray();

  auto regularNodeGlobalIds_array = addedTree.GetField("RegularNodeGlobalIds").GetData();
  std::vector<viskores::Id> regularNodeGlobalIds(regularNodeGlobalIds_array.GetNumberOfValues());
  auto regularNodeGlobalIds_handle =
    viskores::cont::make_ArrayHandle(regularNodeGlobalIds, viskores::CopyFlag::Off);
  viskores::cont::ArrayCopy(regularNodeGlobalIds_array, regularNodeGlobalIds_handle);
  regularNodeGlobalIds_handle
    .SyncControlArray(); //Forces values to get updated if copy happened on GPU

  auto superarcs_array = addedTree.GetField("Superarcs").GetData();
  std::vector<viskores::Id> added_tree_superarcs(superarcs_array.GetNumberOfValues());
  auto superarcs_handle =
    viskores::cont::make_ArrayHandle(added_tree_superarcs, viskores::CopyFlag::Off);
  viskores::cont::ArrayCopy(superarcs_array, superarcs_handle);
  superarcs_handle.SyncControlArray(); //Forces values to get updated if copy happened on GPU

  auto supernodes_array = addedTree.GetField("Supernodes").GetData();
  std::vector<viskores::Id> added_tree_supernodes(supernodes_array.GetNumberOfValues());
  auto supernodes_handle =
    viskores::cont::make_ArrayHandle(added_tree_supernodes, viskores::CopyFlag::Off);
  viskores::cont::ArrayCopy(supernodes_array, supernodes_handle);
  supernodes_handle.SyncControlArray(); //Forces values to get updated if copy happened on GPU

  auto superparents_array = addedTree.GetField("Superparents").GetData();
  std::vector<viskores::Id> superparents(superparents_array.GetNumberOfValues());
  auto superparents_handle =
    viskores::cont::make_ArrayHandle(superparents, viskores::CopyFlag::Off);
  viskores::cont::ArrayCopy(superparents_array, superparents_handle);
  superparents_handle.SyncControlArray(); //Forces values to get updated if copy happened on GPU

  // loop through all of the supernodes in the hierarchical tree
  for (indexType supernode = 0; supernode < static_cast<indexType>(added_tree_supernodes.size());
       supernode++)
  { // per supernode
    // retrieve the regular ID for the supernode
    indexType regularId = added_tree_supernodes[supernode];
    indexType globalId = regularNodeGlobalIds[regularId];
    dataType dataVal = dataValues[regularId];

    // retrieve the supernode at the far end
    indexType superTo = added_tree_superarcs[supernode];

    // now test - if it is NO_SUCH_ELEMENT, there are two possibilities
    if (viskores::worklet::contourtree_augmented::NoSuchElement(superTo))
    { // no Superto

      // retrieve the superparent
      indexType superparent = superparents[regularId];


      // the root node will have itself as its superparent
      if (superparent == supernode)
        continue;
      else
      { // not own superparent - an attachment point
        // retrieve the superparent's from & to
        indexType regularFrom = added_tree_supernodes[superparent];
        indexType globalFrom = regularNodeGlobalIds[regularFrom];
        indexType superParentTo = added_tree_superarcs[superparent];
        indexType regularTo =
          added_tree_supernodes[viskores::worklet::contourtree_augmented::MaskedIndex(
            superParentTo)];
        indexType globalTo = regularNodeGlobalIds[regularTo];

        // test the superTo to see whether we ascend or descend
        // note that we will never have NO_SUCH_ELEMENT here
        if (viskores::worklet::contourtree_augmented::IsAscending(superParentTo))
        { // ascending
          this->supernodes.push_back(SupernodeOnSuperarc(globalId, dataVal, globalFrom, globalTo));
        } // ascending
        else
        { // descending
          this->supernodes.push_back(SupernodeOnSuperarc(globalId, dataVal, globalTo, globalFrom));
        } // descending
      }   // not own superparent - an attachment point
    }     // no Superto
    else
    { // there is a valid superarc
      // retrieve the "to" and convert to global
      indexType maskedTo = viskores::worklet::contourtree_augmented::MaskedIndex(superTo);
      indexType regularTo = added_tree_supernodes[maskedTo];
      indexType globalTo = regularNodeGlobalIds[regularTo];
      dataType dataTo = dataValues[regularTo];

      // test the superTo to see whether we ascend or descend
      // note that we will never have NO_SUCH_ELEMENT here
      // we add both ends
      if (viskores::worklet::contourtree_augmented::IsAscending(superTo))
      { // ascending
        this->supernodes.push_back(SupernodeOnSuperarc(globalId, dataVal, globalId, globalTo));
        this->supernodes.push_back(SupernodeOnSuperarc(globalTo, dataTo, globalId, globalTo));
      } // ascending
      else
      { // descending
        this->supernodes.push_back(SupernodeOnSuperarc(globalId, dataVal, globalTo, globalId));
        this->supernodes.push_back(SupernodeOnSuperarc(globalTo, dataTo, globalTo, globalId));
      } // descending
    }   // there is a valid superarc
  }     // per supernode

} // TreeCompiler::AddHierarchicalTree()

// routine to compute the actual superarcs
inline void TreeCompiler::ComputeSuperarcs()
{ // TreeCompiler::ComputeSuperarcs()
  // first we sort the vector
  std::sort(supernodes.begin(), supernodes.end());

  // we could do a unique test here, but it's easier just to suppress it inside the loop

  // now we loop through it: note the -1
  // this is because we know a priori that the last one is the last supernode on a superarc
  // and would fail the test inside the loop. By putting it in the loop test, we avoid having
  // to have an explicit if statement inside the loop
  for (indexType supernode = 0; supernode < static_cast<viskores::Id>(supernodes.size() - 1);
       supernode++)
  { // loop through supernodes
    // this is actually painfully simple: if the (lowEnd, highEnd) don't match the next one,
    // then we're at the end of the group and do nothing.  Otherwise, we link to the next one
    if ((supernodes[supernode].lowEnd != supernodes[supernode + 1].lowEnd) ||
        (supernodes[supernode].highEnd != supernodes[supernode + 1].highEnd))
      continue;

    // if the supernode matches, then we have a repeat, and can suppress
    if (supernodes[supernode].globalID == supernodes[supernode + 1].globalID)
      continue;

    // otherwise, add a superarc to the list
    superarcs.push_back(Edge(supernodes[supernode].globalID, supernodes[supernode + 1].globalID));
  } // loop through supernodes

  // now sort them
  std::sort(superarcs.begin(), superarcs.end());
} // TreeCompiler::ComputeSuperarcs()

// routine to print the superarcs
inline void TreeCompiler::PrintSuperarcArray(const std::vector<Edge>& superarc_array)
{ // TreeCompiler::PrintSuperarcArray()
  for (indexType superarc = 0; superarc < static_cast<indexType>(superarc_array.size()); superarc++)
  { // per superarc
    if (superarc_array[superarc].low < superarc_array[superarc].high)
    { // order by ID not value
      std::cout << std::setw(PRINT_WIDTH) << superarc_array[superarc].low << " ";
      std::cout << std::setw(PRINT_WIDTH) << superarc_array[superarc].high << std::endl;
    } // order by ID not value
    else
    { // order by ID not value
      std::cout << std::setw(PRINT_WIDTH) << superarc_array[superarc].high << " ";
      std::cout << std::setw(PRINT_WIDTH) << superarc_array[superarc].low << std::endl;
    } // order by ID not value

  } // per superarc

} // TreeCompiler::PrintSuperarcArray()

inline void TreeCompiler::PrintSuperarcs(bool printHeader = false) const
{
  if (printHeader)
  {
    std::cout << "============" << std::endl;
    std::cout << "Contour Tree" << std::endl;
  }

  PrintSuperarcArray(this->superarcs);
}

// routine to write out binary file
inline void TreeCompiler::WriteBinary(FILE* outFile) const
{ // WriteBinary()
  // do a bulk write of the entire contents
  // no error checking, no type checking, no nothing
  fwrite(&(supernodes[0]), sizeof(SupernodeOnSuperarc), supernodes.size(), outFile);
} // WriteBinary()

// routine to read in binary file and append
inline void TreeCompiler::ReadBinary(FILE* inFile)
{ // ReadBinary()
  // use fseek to jump to the end
  fseek(inFile, 0, SEEK_END);

  // use fTell to retrieve the size of the file
  std::size_t nBytes = ftell(inFile);
  // now rewind
  rewind(inFile);

  // compute how many elements are to be read
  std::size_t nSupernodes = nBytes / sizeof(SupernodeOnSuperarc);

  // retrieve the current size
  std::size_t currentSize = supernodes.size();

  // resize to add the right number
  supernodes.resize(currentSize + nSupernodes);

  // now read directly into the right chunk
  std::size_t nSupernodesRead =
    fread(&(supernodes[currentSize]), sizeof(SupernodeOnSuperarc), nSupernodes, inFile);

  if (nSupernodesRead != nSupernodes)
  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Error,
                   "Error: Expected to read " << nSupernodes << " supernodes but could read only "
                                              << nSupernodesRead << ". Output will be incorrect!"
                                              << std::endl);
  }
} // ReadBinary()

// stream output - just dumps the supernodeonsuperarcs
inline std::ostream& operator<<(std::ostream& outStream, TreeCompiler& tree)
{ // stream output
  for (indexType supernode = 0; supernode < static_cast<indexType>(tree.supernodes.size());
       supernode++)
    outStream << tree.supernodes[supernode];
  return outStream;
} // stream output

// stream input - reads in the supernodeonsuperarcs & appends them
inline std::istream& operator>>(std::istream& inStream, TreeCompiler& tree)
{ // stream input
  while (!inStream.eof())
  {
    SupernodeOnSuperarc tempNode;
    inStream >> tempNode;
    tree.supernodes.push_back(tempNode);
  }
  // we will overshoot, so subtract one
  tree.supernodes.resize(tree.supernodes.size() - 1);
  return inStream;
} // stream input

} // namespace contourtree_distributed
} // namespace worklet
} // namespace viskores

#endif
