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

#ifndef viskores_worklet_contourtree_augmented_process_contourtree_inc_branch_h
#define viskores_worklet_contourtree_augmented_process_contourtree_inc_branch_h

#include <viskores/cont/ArrayHandle.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/ContourTree.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/processcontourtree/PiecewiseLinearFunction.h>

#include <cmath>

namespace viskores
{
namespace worklet
{
namespace contourtree_augmented
{
namespace process_contourtree_inc
{

// TODO The pointered list structure and use of std::vector don't seem to fit well with using Branch with VISKORES
template <typename T>
class Branch
{
public:
  viskores::Id OriginalId;          // Index of the extremum in the mesh
  viskores::Id Extremum;            // Index of the extremum in the mesh
  T ExtremumVal;                    // Value at the extremum:w
  viskores::Id Saddle;              // Index of the saddle in the mesh (or minimum for root branch)
  T SaddleVal;                      // Corresponding value
  viskores::Id Volume;              // Volume
  Branch<T>* Parent;                // Pointer to parent, or nullptr if no parent
  std::vector<Branch<T>*> Children; // List of pointers to children

  // Create branch decomposition from contour tree
  template <typename StorageType>
  static Branch<T>* ComputeBranchDecomposition(
    const IdArrayType& contourTreeSuperparents,
    const IdArrayType& contourTreeSupernodes,
    const IdArrayType& whichBranch,
    const IdArrayType& branchMinimum,
    const IdArrayType& branchMaximum,
    const IdArrayType& branchSaddle,
    const IdArrayType& branchParent,
    const IdArrayType& sortOrder,
    const viskores::cont::ArrayHandle<T, StorageType>& dataField,
    bool dataFieldIsSorted);

  // Simplify branch composition down to target size (i.e., consisting of targetSize branches)
  void SimplifyToSize(viskores::Id targetSize, bool usePersistenceSorter = true);

  // Print the branch decomposition
  void PrintBranchDecomposition(std::ostream& os, std::string::size_type indent = 0) const;

  // Persistence of branch
  T Persistence() { return std::fabs(ExtremumVal - SaddleVal); }

  // Destroy branch (deleting children and propagating Volume to parent)
  ~Branch();

  // Compute list of relevant/interesting isovalues
  void GetRelevantValues(int type, T eps, std::vector<T>& values) const;

  void AccumulateIntervals(int type, T eps, PiecewiseLinearFunction<T>& plf) const;

private:
  // Private default constructore to ensure that branch decomposition can only be created from a contour tree or loaded from storate (via static methods)
  Branch()
    : Extremum((viskores::Id)viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT)
    , ExtremumVal(0)
    , Saddle((viskores::Id)viskores::worklet::contourtree_augmented::NO_SUCH_ELEMENT)
    , SaddleVal(0)
    , Volume(0)
    , Parent(nullptr)
    , Children()
  {
  }

  // Remove symbolic perturbation, i.e., branches with zero persistence
  void removeSymbolicPerturbation();
}; // class Branch


template <typename T>
struct PersistenceSorter
{ // PersistenceSorter()
  inline bool operator()(Branch<T>* a, Branch<T>* b) { return a->Persistence() < b->Persistence(); }
}; // PersistenceSorter()


template <typename T>
struct VolumeSorter
{ // VolumeSorter()
  inline bool operator()(Branch<T>* a, Branch<T>* b) { return a->Volume < b->Volume; }
}; // VolumeSorter()


template <typename T>
template <typename StorageType>
Branch<T>* Branch<T>::ComputeBranchDecomposition(
  const IdArrayType& contourTreeSuperparents,
  const IdArrayType& contourTreeSupernodes,
  const IdArrayType& whichBranch,
  const IdArrayType& branchMinimum,
  const IdArrayType& branchMaximum,
  const IdArrayType& branchSaddle,
  const IdArrayType& branchParent,
  const IdArrayType& sortOrder,
  const viskores::cont::ArrayHandle<T, StorageType>& dataField,
  bool dataFieldIsSorted)
{ // C)omputeBranchDecomposition()
  auto branchMinimumPortal = branchMinimum.ReadPortal();
  auto branchMaximumPortal = branchMaximum.ReadPortal();
  auto branchSaddlePortal = branchSaddle.ReadPortal();
  auto branchParentPortal = branchParent.ReadPortal();
  auto sortOrderPortal = sortOrder.ReadPortal();
  auto supernodesPortal = contourTreeSupernodes.ReadPortal();
  auto dataFieldPortal = dataField.ReadPortal();
  viskores::Id nBranches = branchSaddle.GetNumberOfValues();
  std::vector<Branch<T>*> branches;
  Branch<T>* root = nullptr;
  branches.reserve(static_cast<std::size_t>(nBranches));

  for (int branchID = 0; branchID < nBranches; ++branchID)
    branches.push_back(new Branch<T>);

  // Reconstruct explicit branch decomposition from array representation
  for (std::size_t branchID = 0; branchID < static_cast<std::size_t>(nBranches); ++branchID)
  {
    branches[branchID]->OriginalId = static_cast<viskores::Id>(branchID);
    if (!NoSuchElement(branchSaddlePortal.Get(static_cast<viskores::Id>(branchID))))
    {
      branches[branchID]->Saddle = MaskedIndex(supernodesPortal.Get(
        MaskedIndex(branchSaddlePortal.Get(static_cast<viskores::Id>(branchID)))));
      viskores::Id branchMin = MaskedIndex(supernodesPortal.Get(
        MaskedIndex(branchMinimumPortal.Get(static_cast<viskores::Id>(branchID)))));
      viskores::Id branchMax = MaskedIndex(supernodesPortal.Get(
        MaskedIndex(branchMaximumPortal.Get(static_cast<viskores::Id>(branchID)))));
      if (branchMin < branches[branchID]->Saddle)
        branches[branchID]->Extremum = branchMin;
      else if (branchMax > branches[branchID]->Saddle)
        branches[branchID]->Extremum = branchMax;
      else
      {
        std::cerr << "Internal error";
        return 0;
      }
    }
    else
    {
      branches[branchID]->Saddle = supernodesPortal.Get(
        MaskedIndex(branchMinimumPortal.Get(static_cast<viskores::Id>(branchID))));
      branches[branchID]->Extremum = supernodesPortal.Get(
        MaskedIndex(branchMaximumPortal.Get(static_cast<viskores::Id>(branchID))));
    }

    if (dataFieldIsSorted)
    {
      branches[branchID]->SaddleVal = dataFieldPortal.Get(branches[branchID]->Saddle);
      branches[branchID]->ExtremumVal = dataFieldPortal.Get(branches[branchID]->Extremum);
    }
    else
    {
      branches[branchID]->SaddleVal =
        dataFieldPortal.Get(sortOrderPortal.Get(branches[branchID]->Saddle));
      branches[branchID]->ExtremumVal =
        dataFieldPortal.Get(sortOrderPortal.Get(branches[branchID]->Extremum));
    }

    branches[branchID]->Saddle = sortOrderPortal.Get(branches[branchID]->Saddle);
    branches[branchID]->Extremum = sortOrderPortal.Get(branches[branchID]->Extremum);

    if (NoSuchElement(branchParentPortal.Get(static_cast<viskores::Id>(branchID))))
    {
      root = branches[branchID]; // No parent -> this is the root branch
    }
    else
    {
      branches[branchID]->Parent = branches[static_cast<size_t>(
        MaskedIndex(branchParentPortal.Get(static_cast<viskores::Id>(branchID))))];
      branches[branchID]->Parent->Children.push_back(branches[branchID]);
    }
  }

  // FIXME: This is a somewhat hackish way to compute the Volume, but it works
  // It would probably be better to compute this from the already computed Volume information
  auto whichBranchPortal = whichBranch.ReadPortal();
  auto superparentsPortal = contourTreeSuperparents.ReadPortal();
  for (viskores::Id i = 0; i < contourTreeSuperparents.GetNumberOfValues(); i++)
  {
    branches[static_cast<size_t>(
               MaskedIndex(whichBranchPortal.Get(MaskedIndex(superparentsPortal.Get(i)))))]
      ->Volume++; // Increment Volume
  }
  if (root)
  {
    root->removeSymbolicPerturbation();
  }

  return root;
} // ComputeBranchDecomposition()


template <typename T>
void Branch<T>::SimplifyToSize(viskores::Id targetSize, bool usePersistenceSorter)
{ // SimplifyToSize()
  if (targetSize <= 1)
    return;

  // Top-down simplification, starting from one branch and adding in the rest on a biggest-first basis
  std::vector<Branch<T>*> q;
  q.push_back(this);

  std::vector<Branch<T>*> active;
  while (active.size() < static_cast<std::size_t>(targetSize) && !q.empty())
  {
    if (usePersistenceSorter)
    {
      std::pop_heap(
        q.begin(),
        q.end(),
        PersistenceSorter<
          T>()); // FIXME: This should be Volume, but we were doing this wrong for the demo, so let's start with doing this wrong here, too
    }
    else
    {
      std::pop_heap(
        q.begin(),
        q.end(),
        VolumeSorter<
          T>()); // FIXME: This should be Volume, but we were doing this wrong for the demo, so let's start with doing this wrong here, too
    }
    Branch<T>* b = q.back();
    q.pop_back();

    active.push_back(b);

    for (Branch<T>* c : b->Children)
    {
      q.push_back(c);
      if (usePersistenceSorter)
      {
        std::push_heap(q.begin(), q.end(), PersistenceSorter<T>());
      }
      else
      {
        std::push_heap(q.begin(), q.end(), VolumeSorter<T>());
      }
    }
  }

  // Rest are inactive
  for (Branch<T>* b : q)
  {
    // Hackish, remove c from its parents child list
    if (b->Parent)
      b->Parent->Children.erase(
        std::remove(b->Parent->Children.begin(), b->Parent->Children.end(), b));

    delete b;
  }
} // SimplifyToSize()


template <typename T>
void Branch<T>::PrintBranchDecomposition(std::ostream& os, std::string::size_type indent) const
{ // PrintBranchDecomposition()
  os << std::string(indent, ' ') << "{" << std::endl;
  os << std::string(indent, ' ') << "  Saddle = " << SaddleVal << " (" << Saddle << ")"
     << std::endl;
  os << std::string(indent, ' ') << "  Extremum = " << ExtremumVal << " (" << Extremum << ")"
     << std::endl;
  os << std::string(indent, ' ') << "  Volume = " << Volume << std::endl;
  if (!Children.empty())
  {
    os << std::string(indent, ' ') << "  Children = [" << std::endl;
    for (Branch<T>* c : Children)
      c->PrintBranchDecomposition(os, indent + 4);
    os << std::string(indent, ' ') << std::string(indent, ' ') << "  ]" << std::endl;
  }
  os << std::string(indent, ' ') << "}" << std::endl;
} // PrintBranchDecomposition()


template <typename T>
Branch<T>::~Branch()
{ // ~Branch()
  for (Branch<T>* c : Children)
    delete c;
  if (Parent)
    Parent->Volume += Volume;
} // ~Branch()


// TODO this recursive accumlation of values does not lend itself well to the use of VISKORES data structures
template <typename T>
void Branch<T>::GetRelevantValues(int type, T eps, std::vector<T>& values) const
{ // GetRelevantValues()
  T val;

  bool isMax = false;
  if (ExtremumVal > SaddleVal)
    isMax = true;

  switch (type)
  {
    default:
    case 0:
      val = SaddleVal + (isMax ? +eps : -eps);
      break;
    case 1:
      val = T(0.5f) * (ExtremumVal + SaddleVal);
      break;
    case 2:
      val = ExtremumVal + (isMax ? -eps : +eps);
      break;
  }
  if (Parent)
    values.push_back({ val });
  for (Branch* c : Children)
    c->GetRelevantValues(type, eps, values);
} // GetRelevantValues()


template <typename T>
void Branch<T>::AccumulateIntervals(int type, T eps, PiecewiseLinearFunction<T>& plf) const
{ //AccumulateIntervals()
  bool isMax = (ExtremumVal > SaddleVal);
  T val;

  switch (type)
  {
    default:
    case 0:
      val = SaddleVal + (isMax ? +eps : -eps);
      break;
    case 1:
      val = T(0.5f) * (ExtremumVal + SaddleVal);
      break;
    case 2:
      val = ExtremumVal + (isMax ? -eps : +eps);
      break;
  }

  if (Parent)
  {
    PiecewiseLinearFunction<T> addPLF;
    addPLF.addSample(SaddleVal, 0.0);
    addPLF.addSample(ExtremumVal, 0.0);
    addPLF.addSample(val, 1.0);
    plf += addPLF;
  }
  for (Branch<T>* c : Children)
    c->AccumulateIntervals(type, eps, plf);
} // AccumulateIntervals()


template <typename T>
void Branch<T>::removeSymbolicPerturbation()
{                                      // removeSymbolicPerturbation()
  std::vector<Branch<T>*> newChildren; // Temporary list of children that are not flat

  for (Branch<T>* c : Children)
  {
    // First recursively remove symbolic perturbation (zero persistence branches) for  all children below the current child
    // Necessary to be able to detect whether we can remove the current child
    c->removeSymbolicPerturbation();

    // Does child have zero persistence (flat region)
    if (c->ExtremumVal == c->SaddleVal && c->Children.empty())
    {
      // If yes, then we get its associated Volume and delete it
      delete c; // Will add Volume to parent, i.e., us
    }
    else
    {
      // Otherwise, keep child
      newChildren.push_back(c);
    }
  }
  // Swap out new list of children
  Children.swap(newChildren);
} // removeSymbolicPerturbation()

} // process_contourtree_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace viskores

#endif // viskores_worklet_contourtree_augmented_process_contourtree_inc_branch_h
