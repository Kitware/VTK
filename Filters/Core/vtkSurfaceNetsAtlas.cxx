// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSurfaceNetsAtlas.h"

#include "vtkArrayDispatch.h"
#include "vtkArrayDispatchDataSetArrayList.h"
#include "vtkBatch.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkConstantArray.h"
#include "vtkDataArray.h"
#include "vtkDataAssembly.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkStringFormatter.h"
#include "vtkTypeInt8Array.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkSurfaceNetsAtlas);

namespace
{

// Name of the cell-data array that vtkSurfaceNets3D writes to identify the
// pair of labels on either side of each generated cell.
constexpr const char* BOUNDARY_LABELS_ARRAY = "BoundaryLabels";

// Name of the point-data array that vtkSurfaceNets3D writes (signed int8)
// describing the non-manifold treatment of each output point. Values:
//   -2: manifold (no duplication needed)
//   -1: detected non-manifold but no split pattern was applicable
//    0..: index into the non-manifold case table
constexpr const char* NM_INDICES_ARRAY = "NonManifoldTableIndices";

// Sentinel cell ID for "no patch" (shouldn't occur in well-formed input).
constexpr vtkIdType INVALID_PATCH_ID = -1;

// Ordered pair of labels (LID0, LID1) with LID0 < LID1 keying a Patch.
class Patch
{
public:
  vtkIdType LID0;
  vtkIdType LID1;

  // Constructor enforces (a < b) logic automatically
  Patch(vtkIdType a, vtkIdType b)
  {
    if (a < b)
    {
      this->LID0 = a;
      this->LID1 = b;
    }
    else
    {
      this->LID0 = b;
      this->LID1 = a;
    }
  }

  // Equality operator is required for std::unordered_map/set
  bool operator==(const Patch& other) const
  {
    return this->LID0 == other.LID0 && this->LID1 == other.LID1;
  }
};

// Custom Hash Structure
struct PatchHash
{
  std::size_t operator()(const Patch& p) const noexcept
  {
    auto h1 = std::hash<vtkIdType>{}(p.LID0);
    auto h2 = std::hash<vtkIdType>{}(p.LID1);

    // Using the golden ratio constant for better distribution
    return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
  }
};

} // anonymous namespace

// ============================================================================
// AtlasData: the internal database built from the input mesh.
// ============================================================================
struct vtkSurfaceNetsAtlas::AtlasData
{
  // LID -> Label. Dense [0, N).
  std::vector<vtkIdType> LIDToLabel;

  // Label -> LID.
  std::unordered_map<vtkIdType, int> LabelToLID;

  // Label -> name. User-set names persist across atlas rebuilds; BuildAtlas fills in
  // "Label_X" defaults for any label that does not have a user-set name.
  std::unordered_map<vtkIdType, std::string> LabelNames;

  // PatchIdx -> (LID0, LID1) with LID0 < LID1 by label value.
  // Note: ordered by label, not by LID, because the user-facing API speaks
  // in labels. So PatchLIDs[i] = (lid_of_min_label, lid_of_max_label).
  std::vector<Patch> Patches;

  // Patch -> PatchIdx, for reverse lookup.
  std::unordered_map<Patch, int, PatchHash> PatchToIdx;

  // For each cell in the source mesh: the PatchIdx it belongs to.
  // Length = source mesh numCells.
  std::vector<int> CellToPatch;

  // For each patch: the list of source cell IDs belonging to it.
  // PatchCells[patchIdx] = vector of source cell IDs.
  std::vector<std::vector<vtkIdType>> PatchCells;

  // For each LID: the list of patch indices touching it.
  // LIDToPatches[lid] = vector of patch indices.
  std::vector<std::vector<int>> LIDToPatches;

  // Reset atlas data. LabelNames is intentionally not cleared so that user-set names
  // survive atlas rebuilds triggered by input mesh changes.
  void Clear()
  {
    this->LIDToLabel.clear();
    this->LabelToLID.clear();
    this->Patches.clear();
    this->PatchToIdx.clear();
    this->CellToPatch.clear();
    this->PatchCells.clear();
    this->LIDToPatches.clear();
  }
};

//------------------------------------------------------------------------------
vtkSurfaceNetsAtlas::vtkSurfaceNetsAtlas()
  : Atlas(new AtlasData())
{
}

//------------------------------------------------------------------------------
vtkSurfaceNetsAtlas::~vtkSurfaceNetsAtlas() = default;

//------------------------------------------------------------------------------
void vtkSurfaceNetsAtlas::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ExtractionMode: " << this->ExtractionMode << "\n";
  os << indent << "OutputStyle: " << this->OutputStyle << "\n";
  os << indent << "BackgroundLabel: " << this->BackgroundLabel << "\n";
  os << indent << "GenerateRegions: " << (this->GenerateRegions ? "On" : "Off") << "\n";
  os << indent << "GeneratePatches: " << (this->GeneratePatches ? "On" : "Off") << "\n";
  os << indent << "ResolveNonManifoldPoints: " << (this->ResolveNonManifoldPoints ? "On" : "Off")
     << "\n";
  os << indent << "Number of selected labels: " << this->Labels.size() << "\n";
  os << indent << "Atlas: " << this->Atlas->LIDToLabel.size() << " labels, "
     << this->Atlas->Patches.size() << " patches\n";
}

//------------------------------------------------------------------------------
int vtkSurfaceNetsAtlas::FillInputPortInformation(int /*port*/, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  return 1;
}

//------------------------------------------------------------------------------
void vtkSurfaceNetsAtlas::AddSelectedLabel(vtkIdType label)
{
  for (vtkIdType v : this->Labels)
  {
    if (v == label)
    {
      return;
    }
  }
  this->Labels.push_back(label);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkSurfaceNetsAtlas::RemoveSelectedLabel(vtkIdType label)
{
  const auto it = std::find(this->Labels.begin(), this->Labels.end(), label);
  if (it != this->Labels.end())
  {
    this->Labels.erase(it);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkSurfaceNetsAtlas::ClearSelectedLabels()
{
  if (!this->Labels.empty())
  {
    this->Labels.clear();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkSurfaceNetsAtlas::SetNumberOfSelectedLabels(int n)
{
  if (n != static_cast<int>(this->Labels.size()))
  {
    this->Labels.resize(n);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
int vtkSurfaceNetsAtlas::GetNumberOfSelectedLabels() const
{
  return static_cast<int>(this->Labels.size());
}

//------------------------------------------------------------------------------
void vtkSurfaceNetsAtlas::SetSelectedLabel(int i, vtkIdType label)
{
  const int newSize = std::max(i + 1, static_cast<int>(this->Labels.size()));
  this->SetNumberOfSelectedLabels(newSize);
  if (this->Labels[i] != label)
  {
    this->Labels[i] = label;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkSurfaceNetsAtlas::GetSelectedLabel(int i) const
{
  if (i < 0 || i >= static_cast<int>(this->Labels.size()))
  {
    return -1;
  }
  return this->Labels[i];
}

//------------------------------------------------------------------------------
std::vector<vtkIdType> vtkSurfaceNetsAtlas::GetSelectedLabels() const
{
  return this->Labels;
}

//------------------------------------------------------------------------------
void vtkSurfaceNetsAtlas::SetSelectedLabels(const std::vector<vtkIdType>& labels)
{
  this->Labels = labels;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkSurfaceNetsAtlas::GenerateSelectedLabels(
  int numLabels, vtkIdType rangeStart, vtkIdType rangeEnd)
{
  this->Labels.clear();
  if (numLabels == 1)
  {
    this->Labels.push_back(rangeStart);
  }
  else if (numLabels > 1)
  {
    for (int i = 0; i < numLabels; ++i)
    {
      this->Labels.push_back(rangeStart + i * (rangeEnd - rangeStart) / (numLabels - 1));
    }
  }
  this->Modified();
}

//------------------------------------------------------------------------------
int vtkSurfaceNetsAtlas::GetNumberOfLabelNames() const
{
  return static_cast<int>(this->Atlas->LabelNames.size());
}

//------------------------------------------------------------------------------
void vtkSurfaceNetsAtlas::SetLabelName(vtkIdType label, const std::string& name)
{
  auto& entry = this->Atlas->LabelNames[label];
  if (entry != name)
  {
    entry = name;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
std::string vtkSurfaceNetsAtlas::GetLabelName(vtkIdType label) const
{
  const auto it = this->Atlas->LabelNames.find(label);
  return it != this->Atlas->LabelNames.end() ? it->second : std::string{};
}

//------------------------------------------------------------------------------
void vtkSurfaceNetsAtlas::AddLabelName(vtkIdType label, const std::string& name)
{
  this->SetLabelName(label, name);
}

//------------------------------------------------------------------------------
void vtkSurfaceNetsAtlas::RemoveLabelName(vtkIdType label)
{
  if (this->Atlas->LabelNames.erase(label) > 0)
  {
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkSurfaceNetsAtlas::ClearLabelNames()
{
  if (!this->Atlas->LabelNames.empty())
  {
    this->Atlas->LabelNames.clear();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkSurfaceNetsAtlas::GenerateSelectedLabels(int numLabels, vtkIdType range[2])
{
  this->GenerateSelectedLabels(numLabels, range[0], range[1]);
}

//------------------------------------------------------------------------------
void vtkSurfaceNetsAtlas::GenerateSelectedLabels(vtkIdType rangeStart, vtkIdType rangeEnd)
{
  const int numLabels = static_cast<int>(rangeEnd - rangeStart + 1);
  this->GenerateSelectedLabels(numLabels, rangeStart, rangeEnd);
}

//------------------------------------------------------------------------------
int vtkSurfaceNetsAtlas::GetNumberOfLabels() const
{
  return static_cast<int>(this->Atlas->LIDToLabel.size());
}

//------------------------------------------------------------------------------
bool vtkSurfaceNetsAtlas::HasLabel(vtkIdType label) const
{
  return this->Atlas->LabelToLID.find(label) != this->Atlas->LabelToLID.end();
}

//------------------------------------------------------------------------------
int vtkSurfaceNetsAtlas::GetLIDForLabel(vtkIdType label) const
{
  const auto it = this->Atlas->LabelToLID.find(label);
  return it == this->Atlas->LabelToLID.end() ? -1 : it->second;
}

//------------------------------------------------------------------------------
vtkIdType vtkSurfaceNetsAtlas::GetLabelForLID(int lid) const
{
  if (lid < 0 || lid >= static_cast<int>(this->Atlas->LIDToLabel.size()))
  {
    return -1;
  }
  return this->Atlas->LIDToLabel[lid];
}

//------------------------------------------------------------------------------
vtkIdType vtkSurfaceNetsAtlas::GetLabelForName(const std::string& name) const
{
  for (const auto& [label, n] : this->Atlas->LabelNames)
  {
    if (n == name)
    {
      return label;
    }
  }
  return -1;
}

//------------------------------------------------------------------------------
bool vtkSurfaceNetsAtlas::HasLabel(const std::string& name) const
{
  return this->HasLabel(this->GetLabelForName(name));
}

//------------------------------------------------------------------------------
int vtkSurfaceNetsAtlas::GetLIDForLabel(const std::string& name) const
{
  return this->GetLIDForLabel(this->GetLabelForName(name));
}

//------------------------------------------------------------------------------
bool vtkSurfaceNetsAtlas::AreAdjacent(vtkIdType label0, vtkIdType label1) const
{
  if (label0 == label1)
  {
    return false;
  }
  auto lid0 = this->GetLIDForLabel(label0);
  auto lid1 = this->GetLIDForLabel(label1);
  if (lid0 < 0 || lid1 < 0)
  {
    return false;
  }
  return this->Atlas->PatchToIdx.find({ lid0, lid1 }) != this->Atlas->PatchToIdx.end();
}

//------------------------------------------------------------------------------
bool vtkSurfaceNetsAtlas::AreAdjacent(const std::string& name0, const std::string& name1) const
{
  return this->AreAdjacent(this->GetLabelForName(name0), this->GetLabelForName(name1));
}

//------------------------------------------------------------------------------
std::vector<vtkIdType> vtkSurfaceNetsAtlas::GetAdjacentLabels(const std::string& name) const
{
  return this->GetAdjacentLabels(this->GetLabelForName(name));
}

//------------------------------------------------------------------------------
std::vector<vtkIdType> vtkSurfaceNetsAtlas::GetAdjacentLabels(vtkIdType label) const
{
  const vtkIdType lid = this->GetLIDForLabel(label);
  if (lid < 0)
  {
    return {};
  }
  std::vector<vtkIdType> adjacentLabels;
  adjacentLabels.reserve(this->Atlas->LIDToPatches[lid].size());
  for (int patchIdx : this->Atlas->LIDToPatches[lid])
  {
    const auto& patch = this->Atlas->Patches[patchIdx];
    const vtkIdType otherLID = (patch.LID0 == lid)
      ? patch.LID1
      : patch.LID0; // The other LID in the patch is the adjacent one.
    adjacentLabels.push_back(this->Atlas->LIDToLabel[otherLID]);
  }
  return adjacentLabels;
}

//------------------------------------------------------------------------------
vtkIdType vtkSurfaceNetsAtlas::GetNumberOfPatches() const
{
  return static_cast<vtkIdType>(this->Atlas->Patches.size());
}

//------------------------------------------------------------------------------
void vtkSurfaceNetsAtlas::GetPatchLabels(int patchID, vtkIdType labels[2]) const
{
  labels[0] = -1;
  labels[1] = -1;
  if (patchID < 0 || patchID >= static_cast<vtkIdType>(this->Atlas->Patches.size()))
  {
    return;
  }
  const auto& patch = this->Atlas->Patches[patchID];
  labels[0] = this->Atlas->LIDToLabel[patch.LID0];
  labels[1] = this->Atlas->LIDToLabel[patch.LID1];
}

//------------------------------------------------------------------------------
int vtkSurfaceNetsAtlas::GetPatchID(vtkIdType label0, vtkIdType label1) const
{
  if (label0 == label1)
  {
    return -1;
  }
  const int lid0 = this->GetLIDForLabel(label0);
  const int lid1 = this->GetLIDForLabel(label1);
  if (lid0 < 0 || lid1 < 0)
  {
    return -1;
  }
  const auto it = this->Atlas->PatchToIdx.find({ lid0, lid1 });
  return (it == this->Atlas->PatchToIdx.end()) ? -1 : it->second;
}

//------------------------------------------------------------------------------
int vtkSurfaceNetsAtlas::GetPatchID(const std::string& name0, const std::string& name1) const
{
  return this->GetPatchID(this->GetLabelForName(name0), this->GetLabelForName(name1));
}

//------------------------------------------------------------------------------
vtkIdType vtkSurfaceNetsAtlas::GetPatchCellCount(int patchID) const
{
  if (patchID < 0 || patchID >= static_cast<int>(this->Atlas->PatchCells.size()))
  {
    return 0;
  }
  return static_cast<vtkIdType>(this->Atlas->PatchCells[patchID].size());
}

//------------------------------------------------------------------------------
std::vector<int> vtkSurfaceNetsAtlas::GetPatchesForLabel(vtkIdType label) const
{
  int lid = this->GetLIDForLabel(label);
  if (lid < 0)
  {
    return {};
  }
  return this->Atlas->LIDToPatches[lid];
}

//------------------------------------------------------------------------------
std::vector<int> vtkSurfaceNetsAtlas::GetPatchesForLabel(const std::string& name) const
{
  return this->GetPatchesForLabel(this->GetLabelForName(name));
}

namespace
{
//------------------------------------------------------------------------------
template <typename TScalarArray>
struct ExtractUniqueLabelsMaps
{
  TScalarArray* BoundaryLabels;
  using T = vtk::GetAPIType<TScalarArray>;

  vtkSMPThreadLocal<std::unordered_set<T>> TLUniqueLabels;

  std::vector<vtkIdType>& LIDToLabel;
  std::unordered_map<vtkIdType, int>& LabelToLID;

  ExtractUniqueLabelsMaps(TScalarArray* boundaryLabels, std::vector<vtkIdType>& lIDToLabel,
    std::unordered_map<vtkIdType, int>& labelToLID)
    : BoundaryLabels(boundaryLabels)
    , LIDToLabel(lIDToLabel)
    , LabelToLID(labelToLID)
  {
  }

  void Initialize() {}

  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    std::unordered_set<T>& uniqueLabels = this->TLUniqueLabels.Local();

    auto boundaryLabels = vtk::DataArrayTupleRange<2>(this->BoundaryLabels, cellId, endCellId);
    for (const auto& tuple : boundaryLabels)
    {
      uniqueLabels.insert(tuple[0]);
      uniqueLabels.insert(tuple[1]);
    }
  }

  void Reduce()
  {
    // Compute global unique labels
    std::unordered_set<T> globalUniqueLabels;
    for (auto& localSet : this->TLUniqueLabels)
    {
      globalUniqueLabels.insert(localSet.begin(), localSet.end());
    }
    // Construct LID <-> Label mappings in sorted order for stable LIDs.
    std::vector<T> sortedLabels(globalUniqueLabels.begin(), globalUniqueLabels.end());
    std::sort(sortedLabels.begin(), sortedLabels.end());
    this->LIDToLabel.reserve(sortedLabels.size());
    for (const T label : sortedLabels)
    {
      const int lid = static_cast<int>(this->LIDToLabel.size());
      this->LIDToLabel.push_back(static_cast<vtkIdType>(label));
      this->LabelToLID[static_cast<vtkIdType>(label)] = lid;
    }
  }
};

//------------------------------------------------------------------------------
struct ExtractUniqueLabelsMapsWorker
{
  template <typename TScalarArray>
  void operator()(TScalarArray* boundaryLabels, std::vector<vtkIdType>& lIDToLabel,
    std::unordered_map<vtkIdType, int>& labelToLID)
  {
    ExtractUniqueLabelsMaps<TScalarArray> worker(boundaryLabels, lIDToLabel, labelToLID);
    vtkSMPTools::For(0, boundaryLabels->GetNumberOfTuples(), worker);
  }
};

//------------------------------------------------------------------------------
template <typename TScalarArray>
struct ExtractPatches
{
  TScalarArray* BoundaryLabels;
  const std::unordered_map<vtkIdType, int>& LabelToLID;

  vtkSMPThreadLocal<std::vector<Patch>> TLPatches;
  vtkSMPThreadLocal<std::unordered_map<Patch, int, PatchHash>> TLPatchToIdx;

  std::vector<Patch>& Patches;
  std::unordered_map<Patch, int, PatchHash>& PatchToIdx;

  ExtractPatches(TScalarArray* boundaryLabels, const std::unordered_map<vtkIdType, int>& labelToLID,
    std::vector<Patch>& patches, std::unordered_map<Patch, int, PatchHash>& patchToIdx)
    : BoundaryLabels(boundaryLabels)
    , LabelToLID(labelToLID)
    , Patches(patches)
    , PatchToIdx(patchToIdx)
  {
  }

  void Initialize() {}

  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    auto& patches = this->TLPatches.Local();
    auto& patchToIdx = this->TLPatchToIdx.Local();
    auto boundaryLabels = vtk::DataArrayTupleRange<2>(this->BoundaryLabels, cellId, endCellId);

    for (const auto& tuple : boundaryLabels)
    {
      const vtkIdType la = static_cast<vtkIdType>(tuple[0]);
      const vtkIdType lb = static_cast<vtkIdType>(tuple[1]);
      if (la == lb)
      {
        continue; // Degenerate case, skip.
      }
      const auto itA = this->LabelToLID.find(la);
      const auto itB = this->LabelToLID.find(lb);
      if (itA == this->LabelToLID.end() || itB == this->LabelToLID.end())
      {
        continue;
      }
      // Identify IDs and find the patch index
      Patch patch(itA->second, itB->second);
      auto it = patchToIdx.find(patch);
      int patchIdx;
      if (it == patchToIdx.end())
      {
        patchIdx = static_cast<int>(patches.size());
        patches.push_back(patch);
        patchToIdx[patch] = patchIdx;
      }
      else
      {
        patchIdx = it->second;
      }
    }
  }

  void Reduce()
  {
    // Collect all unique patches from thread-local maps.
    std::unordered_set<Patch, PatchHash> uniquePatches;
    for (const auto& localMap : this->TLPatchToIdx)
    {
      for (const auto& [patch, _] : localMap)
      {
        uniquePatches.insert(patch);
      }
    }
    // Sort for stable, deterministic patch indices (by LID0, then LID1).
    std::vector<Patch> sortedPatches(uniquePatches.begin(), uniquePatches.end());
    std::sort(sortedPatches.begin(), sortedPatches.end(), [](const Patch& a, const Patch& b)
      { return a.LID0 < b.LID0 || (a.LID0 == b.LID0 && a.LID1 < b.LID1); });
    // Assign stable indices in sorted order.
    this->Patches.reserve(sortedPatches.size());
    for (const auto& patch : sortedPatches)
    {
      this->PatchToIdx[patch] = static_cast<int>(this->Patches.size());
      this->Patches.push_back(patch);
    }
  }
};

//------------------------------------------------------------------------------
struct ExtractPatchesWorker
{
  template <typename TScalarArray>
  void operator()(TScalarArray* boundaryLabels, std::unordered_map<vtkIdType, int>& labelToLID,
    std::vector<Patch>& patches, std::unordered_map<Patch, int, PatchHash>& patchToIdx)
  {
    ExtractPatches<TScalarArray> worker(boundaryLabels, labelToLID, patches, patchToIdx);
    vtkSMPTools::For(0, boundaryLabels->GetNumberOfTuples(), worker);
  }
};

//------------------------------------------------------------------------------
template <typename TScalarArray>
struct ExtractCellPatches
{
  TScalarArray* BoundaryLabels;
  const std::unordered_map<vtkIdType, int>& LabelToLID;

  const std::unordered_map<Patch, int, PatchHash>& PatchToIdx;
  std::unique_ptr<std::atomic<int>[]> SizePerPatch;

  std::vector<int>& CellToPatch;
  std::vector<std::vector<vtkIdType>>& PatchCells;

  ExtractCellPatches(TScalarArray* boundaryLabels,
    const std::unordered_map<vtkIdType, int>& labelToLID,
    const std::unordered_map<Patch, int, PatchHash>& patchToIdx, int numberOfPatches,
    std::vector<int>& cellToPatch, std::vector<std::vector<vtkIdType>>& patchCells)
    : BoundaryLabels(boundaryLabels)
    , LabelToLID(labelToLID)
    , PatchToIdx(patchToIdx)
    , CellToPatch(cellToPatch)
    , PatchCells(patchCells)
  {
    // Initialize the cell-to-patch mapping
    this->CellToPatch.resize(boundaryLabels->GetNumberOfTuples(), INVALID_PATCH_ID);

    // Atomics must be explicitly initialized before use.
    this->SizePerPatch = std::make_unique<std::atomic<int>[]>(numberOfPatches);
    for (int i = 0; i < numberOfPatches; ++i)
    {
      this->SizePerPatch[i].store(0, std::memory_order_relaxed);
    }

    this->PatchCells.resize(numberOfPatches);
  }

  void Initialize() {}

  // First Pass: Count the number of cells per patch
  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    auto boundaryLabels = vtk::DataArrayTupleRange<2>(this->BoundaryLabels);

    for (; cellId < endCellId; ++cellId)
    {
      const auto tuple = boundaryLabels[cellId];
      const vtkIdType la = static_cast<vtkIdType>(tuple[0]);
      const vtkIdType lb = static_cast<vtkIdType>(tuple[1]);

      // Identify IDs and find the patch index
      const auto itA = this->LabelToLID.find(la);
      const auto itB = this->LabelToLID.find(lb);
      if (itA == this->LabelToLID.end() || itB == this->LabelToLID.end())
      {
        continue;
      }
      Patch patch(itA->second, itB->second);
      const auto itPatch = this->PatchToIdx.find(patch);
      if (itPatch == this->PatchToIdx.end())
      {
        continue;
      }
      const int patchIdx = itPatch->second;

      this->CellToPatch[cellId] = patchIdx;
      this->SizePerPatch[patchIdx].fetch_add(1, std::memory_order_relaxed);
    }
  }

  // Second Pass: Allocate memory and fill patch-cell lists
  void Reduce()
  {
    // 1. Resize each patch's cell list based on the atomic counts
    for (size_t patchIdx = 0; patchIdx < this->PatchCells.size(); ++patchIdx)
    {
      const int count = this->SizePerPatch[patchIdx].load(std::memory_order_relaxed);
      this->PatchCells[patchIdx].resize(count);
    }

    // 2. Use vtkSMPTools to fill the lists in parallel
    vtkSMPTools::For(0, static_cast<vtkIdType>(this->CellToPatch.size()),
      [this](vtkIdType cellId, vtkIdType endCellId)
      {
        for (; cellId < endCellId; ++cellId)
        {
          const int patchIdx = this->CellToPatch[cellId];
          if (patchIdx != INVALID_PATCH_ID)
          {
            // We use fetch_sub to work backwards from the end of the pre-allocated vector
            int idx = this->SizePerPatch[patchIdx].fetch_sub(1, std::memory_order_relaxed) - 1;
            this->PatchCells[patchIdx][idx] = cellId;
          }
        }
      });
  }
};

//------------------------------------------------------------------------------
struct ExtractCellPatchesWorker
{
  template <typename TScalarArray>
  void operator()(TScalarArray* boundaryLabels,
    const std::unordered_map<vtkIdType, int>& labelToLID,
    const std::unordered_map<Patch, int, PatchHash>& patchToIdx, std::vector<int>& cellToPatch,
    std::vector<std::vector<vtkIdType>>& patchCells)
  {
    const int numberOfPatches = static_cast<int>(patchToIdx.size());
    ExtractCellPatches<TScalarArray> worker(
      boundaryLabels, labelToLID, patchToIdx, numberOfPatches, cellToPatch, patchCells);
    vtkSMPTools::For(0, boundaryLabels->GetNumberOfTuples(), worker);
  }
};
}

//------------------------------------------------------------------------------
void vtkSurfaceNetsAtlas::BuildAtlas(vtkPolyData* input)
{
  if (!input)
  {
    return;
  }
  if (input->GetMTime() <= this->AtlasBuildTime.GetMTime() && !this->Atlas->Patches.empty())
  {
    return;
  }

  // Clear previous patch data.
  this->Atlas->Clear();

  vtkDataArray* boundaryLabels = input->GetCellData()->GetArray(BOUNDARY_LABELS_ARRAY);
  if (!boundaryLabels)
  {
    return;
  }

  // Pass 1: Extract unique labels and build LID <-> Label maps
  ExtractUniqueLabelsMapsWorker labelWorker;
  if (!vtkArrayDispatch::DispatchByArray<vtkArrayDispatch::AOSArrays>::Execute(
        boundaryLabels, labelWorker, this->Atlas->LIDToLabel, this->Atlas->LabelToLID))
  {
    labelWorker(boundaryLabels, this->Atlas->LIDToLabel, this->Atlas->LabelToLID);
  }

  if (this->Atlas->LIDToLabel.empty())
  {
    return; // No BoundaryLabels array; nothing to build.
  }

  // Pass 2: Extract Patches and PatchToIdx
  ExtractPatchesWorker patchWorker;
  if (!vtkArrayDispatch::DispatchByArray<vtkArrayDispatch::AOSArrays>::Execute(boundaryLabels,
        patchWorker, this->Atlas->LabelToLID, this->Atlas->Patches, this->Atlas->PatchToIdx))
  {
    patchWorker(
      boundaryLabels, this->Atlas->LabelToLID, this->Atlas->Patches, this->Atlas->PatchToIdx);
  }

  // Pass 3: Build CellToPatch and PatchCells (Detailed Seam Geometry)
  ExtractCellPatchesWorker cellPatchWorker;
  if (!vtkArrayDispatch::DispatchByArray<vtkArrayDispatch::AOSArrays>::Execute(boundaryLabels,
        cellPatchWorker, this->Atlas->LabelToLID, this->Atlas->PatchToIdx, this->Atlas->CellToPatch,
        this->Atlas->PatchCells))
  {
    cellPatchWorker(boundaryLabels, this->Atlas->LabelToLID, this->Atlas->PatchToIdx,
      this->Atlas->CellToPatch, this->Atlas->PatchCells);
  }

  // Pass 4: Build LID -> patches adjacency graph
  this->Atlas->LIDToPatches.assign(this->Atlas->LIDToLabel.size(), {});
  for (size_t p = 0; p < this->Atlas->Patches.size(); ++p)
  {
    const auto& lids = this->Atlas->Patches[p];
    this->Atlas->LIDToPatches[lids.LID0].push_back(static_cast<int>(p));
    if (lids.LID0 != lids.LID1)
    {
      this->Atlas->LIDToPatches[lids.LID1].push_back(static_cast<int>(p));
    }
  }

  // Pass 5: Resolve label names.
  // Warn before filling so the message reflects the state the user left things in.
  // emplace only inserts when the key is absent, so user-set names are preserved.
  if (!this->Atlas->LabelNames.empty() &&
    this->Atlas->LabelNames.size() != this->Atlas->LabelToLID.size())
  {
    vtkWarningMacro("Label name count ("
      << this->Atlas->LabelNames.size() << ") does not match atlas label count ("
      << this->Atlas->LabelToLID.size() << "); default names will be used for missing labels.");
  }
  for (const auto& [label, _] : this->Atlas->LabelToLID)
  {
    this->Atlas->LabelNames.emplace(label, "Label_" + vtk::to_string(label));
  }

  this->AtlasBuildTime.Modified();
}

namespace
{
//------------------------------------------------------------------------------
struct PointBatchData
{
  // Atomic so MarkPointUses threads can increment directly without a separate counter array.
  std::atomic<vtkIdType> PointsOffset;

  PointBatchData()
    : PointsOffset(0)
  {
  }
  // std::atomic is neither copy- nor move-constructible, so provide explicit overloads
  // that do a relaxed load. By the time batches are copied (prefix-sum phase), all
  // parallel marking is complete, so relaxed order is correct.
  PointBatchData(const PointBatchData& other)
    : PointsOffset(other.PointsOffset.load(std::memory_order_relaxed))
  {
  }
  PointBatchData& operator=(const PointBatchData& other)
  {
    this->PointsOffset.store(
      other.PointsOffset.load(std::memory_order_relaxed), std::memory_order_relaxed);
    return *this;
  }
  PointBatchData& operator+=(const PointBatchData& other)
  {
    this->PointsOffset.fetch_add(
      other.PointsOffset.load(std::memory_order_relaxed), std::memory_order_relaxed);
    return *this;
  }
  PointBatchData operator+(const PointBatchData& other) const
  {
    PointBatchData result;
    result.PointsOffset.store(this->PointsOffset.load(std::memory_order_relaxed) +
        other.PointsOffset.load(std::memory_order_relaxed),
      std::memory_order_relaxed);
    return result;
  }
};
using PointBatch = vtkBatch<PointBatchData>;
using PointBatches = vtkBatches<PointBatchData>;

//------------------------------------------------------------------------------
struct MarkPointUses : public vtkCellArray::DispatchUtilities
{
  std::atomic<unsigned char>* PtUses;
  const std::vector<vtkIdType>* SubsetCellIds = nullptr;
  PointBatches Batches;
  vtkIdType BatchSize;

  MarkPointUses(std::atomic<unsigned char>* ptUses, vtkIdType numInputPts)
    : PtUses(ptUses)
  {
    this->Batches.Initialize(numInputPts);
    this->BatchSize = static_cast<vtkIdType>(this->Batches.GetBatchSize());
  }

  void SetGroup(const std::vector<vtkIdType>& subsetCellIds)
  {
    this->SubsetCellIds = &subsetCellIds;
  }

  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* offsets, ConnectivityT* conn, vtkIdType begin, vtkIdType end)
  {
    const auto& subsetCellIds = *this->SubsetCellIds;
    auto offsetRange = GetRange(offsets);
    auto connRange = GetRange(conn);
    for (vtkIdType i = begin; i < end; ++i)
    {
      const vtkIdType cellId = subsetCellIds[i];
      const vtkIdType offsetBegin = offsetRange[cellId];
      const vtkIdType offsetEnd = offsetRange[cellId + 1];
      const vtkIdType numPts = offsetEnd - offsetBegin;
      auto pts = connRange.begin() + offsetBegin;
      for (vtkIdType ptIdx = 0; ptIdx < numPts; ++ptIdx)
      {
        const vtkIdType ptId = pts[ptIdx];
        unsigned char prev = 0;
        if (this->PtUses[ptId].compare_exchange_strong(prev, 1, std::memory_order_relaxed))
        {
          auto& pointBatchData = this->Batches[ptId / this->BatchSize].Data;
          pointBatchData.PointsOffset.fetch_add(1, std::memory_order_relaxed);
        }
      }
    }
  }
};

//------------------------------------------------------------------------------
// Trims empty batches, computes prefix-sum offsets, and assigns new point IDs.
struct BuildPointMap
{
  PointBatches& Batches;
  std::atomic<unsigned char>* PtUses;
  std::vector<vtkIdType>& PointMap;
  vtkIdType TotalKeptPoints;

  BuildPointMap(
    PointBatches& batches, std::atomic<unsigned char>* ptUses, std::vector<vtkIdType>& pointMap)
    : Batches(batches)
    , PtUses(ptUses)
    , PointMap(pointMap)
  {
    // Remove batches that have no points used by this label
    this->Batches.TrimBatches([](const PointBatch& b) { return b.Data.PointsOffset == 0; });
    // Compute global offsets for each remaining batch
    auto globalSum = this->Batches.BuildOffsetsAndGetGlobalSum();
    this->TotalKeptPoints = globalSum.PointsOffset;
  }

  void operator()(vtkIdType beginBatchId, vtkIdType endBatchId)
  {
    for (vtkIdType batchId = beginBatchId; batchId < endBatchId; ++batchId)
    {
      const PointBatch& batch = this->Batches[batchId];
      vtkIdType counter = batch.Data.PointsOffset.load(std::memory_order_relaxed);
      for (vtkIdType ptId = batch.BeginId; ptId < batch.EndId; ++ptId)
      {
        if (this->PtUses[ptId].load(std::memory_order_relaxed))
        {
          this->PointMap[ptId] = counter++;
        }
      }
    }
  }
};

//------------------------------------------------------------------------------
// Copies point coordinates (and optionally NM indices) for all used points, then resets
// ptUses/pointMap in the same pass. Must run after BuildOutputCells (which still reads pointMap).
template <typename TPointsArray, bool HasNMIndices>
struct BuildOutputPoints
{
  TPointsArray* InPointsArray;
  TPointsArray* OutPointsArray;
  PointBatches& Batches;
  std::atomic<unsigned char>* PtUses;
  std::vector<vtkIdType>& PointMap;
  vtkTypeInt8Array* InNMArray;
  vtkTypeInt8Array* OutNMArray;

  BuildOutputPoints(TPointsArray* inPointsArray, TPointsArray* outPointsArray,
    PointBatches& batches, std::atomic<unsigned char>* ptUses, std::vector<vtkIdType>& pointMap,
    vtkTypeInt8Array* inNMArray, vtkTypeInt8Array* outNMArray)
    : InPointsArray(inPointsArray)
    , OutPointsArray(outPointsArray)
    , Batches(batches)
    , PtUses(ptUses)
    , PointMap(pointMap)
    , InNMArray(inNMArray)
    , OutNMArray(outNMArray)
  {
  }

  void operator()(vtkIdType beginBatchId, vtkIdType endBatchId)
  {
    auto inPoints = vtk::DataArrayTupleRange<3>(this->InPointsArray);
    auto outPoints = vtk::DataArrayTupleRange<3>(this->OutPointsArray);

    for (vtkIdType batchId = beginBatchId; batchId < endBatchId; ++batchId)
    {
      const PointBatch& batch = this->Batches[batchId];
      for (vtkIdType pointId = batch.BeginId; pointId < batch.EndId; ++pointId)
      {
        const vtkIdType newId = this->PointMap[pointId];
        if (newId >= 0)
        {
          // Copy point coordinates
          auto inPoint = inPoints[pointId];
          auto outPoint = outPoints[newId];
          outPoint[0] = inPoint[0];
          outPoint[1] = inPoint[1];
          outPoint[2] = inPoint[2];
          if constexpr (HasNMIndices)
          {
            // copy point non-manifold indices
            this->OutNMArray->SetValue(newId, this->InNMArray->GetValue(pointId));
          }
          // reset point uses and point map
          this->PtUses[pointId].store(0, std::memory_order_relaxed);
          this->PointMap[pointId] = -1;
        }
      }
    }
  }
};

//------------------------------------------------------------------------------
template <bool HasNMIndices>
struct BuildOutputPointsWorker
{
  template <typename TPointsArray>
  void operator()(TPointsArray* inPointsArray, vtkDataArray* outPointsArrayDA,
    PointBatches& batches, std::atomic<unsigned char>* ptUses, std::vector<vtkIdType>& pointMap,
    vtkTypeInt8Array* inNMArray, vtkTypeInt8Array* outNMArray)
  {
    BuildOutputPoints<TPointsArray, HasNMIndices> worker(inPointsArray,
      TPointsArray::FastDownCast(outPointsArrayDA), batches, ptUses, pointMap, inNMArray,
      outNMArray);
    vtkSMPTools::For(0, batches.GetNumberOfBatches(), worker);
  }
};

//------------------------------------------------------------------------------
// Builds the output cell array for a subset of cells, and fills BoundaryLabels.
template <typename TScalarArray, bool IsPatch>
struct BuildOutputCells : public vtkCellArray::DispatchUtilities
{
  TScalarArray* InBoundaryLabels;
  TScalarArray* OutBoundaryLabels;
  using T = vtk::GetAPIType<TScalarArray>;
  T TargetLabel; // The label we are currently extracting
  const std::vector<vtkIdType>& PointMap;
  const std::vector<vtkIdType>& SubsetCellIds;
  vtkIdType Offset; // output cell offset for this group

  BuildOutputCells(TScalarArray* inBoundaryLabels, TScalarArray* outBoundaryLabels,
    vtkIdType targetLabel, const std::vector<vtkIdType>& pointMap,
    const std::vector<vtkIdType>& subsetCellIds, vtkIdType offset)
    : InBoundaryLabels(inBoundaryLabels)
    , OutBoundaryLabels(outBoundaryLabels)
    , TargetLabel(static_cast<T>(targetLabel))
    , PointMap(pointMap)
    , SubsetCellIds(subsetCellIds)
    , Offset(offset)
  {
  }

  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* inOffsets, ConnectivityT* inConn, vtkDataArray* outOffsetsDA,
    vtkDataArray* outConnDA, vtkIdType begin, vtkIdType end)
  {
    auto inOffsetsRange = GetRange(inOffsets);
    auto outOffsetsRange = GetRange(OffsetsT::FastDownCast(outOffsetsDA));
    auto inConnRange = GetRange(inConn);
    auto outConnRange = GetRange(ConnectivityT::FastDownCast(outConnDA));

    auto inLabelRange = vtk::DataArrayTupleRange<2>(this->InBoundaryLabels);
    auto outLabelRange = vtk::DataArrayTupleRange<2>(this->OutBoundaryLabels);
    for (vtkIdType i = begin; i < end; ++i)
    {
      const vtkIdType srcCellId = this->SubsetCellIds[i];
      const vtkIdType inOffsetBegin = inOffsetsRange[srcCellId];
      const vtkIdType inOffsetEnd = inOffsetsRange[srcCellId + 1];
      const vtkIdType outOffsetBegin = outOffsetsRange[this->Offset + i];
      const vtkIdType numPoints = inOffsetEnd - inOffsetBegin;

      auto inPts = inConnRange.begin() + inOffsetBegin;
      auto outPts = outConnRange.begin() + outOffsetBegin;

      const auto inLabel = inLabelRange[srcCellId];
      auto outLabel = outLabelRange[this->Offset + i];

      if constexpr (IsPatch)
      {
        // Keep original winding; labels are copied as-is.
        for (vtkIdType ptIdx = 0; ptIdx < numPoints; ++ptIdx)
        {
          outPts[ptIdx] = this->PointMap[inPts[ptIdx]];
        }
        outLabel[0] = inLabel[0];
        outLabel[1] = inLabel[1];
      }
      else
      {
        // Check if TargetLabel is L1. If so, normal points INWARD, must reverse both
        // the winding and the label pair so the convention (Normal: L0 -> L1) is preserved.
        if (inLabel[1] == this->TargetLabel)
        {
          // Reverse winding order (e.g., 0,1,2,3 becomes 3,2,1,0) and swap labels.
          for (vtkIdType ptIdx = 0; ptIdx < numPoints; ++ptIdx)
          {
            outPts[ptIdx] = this->PointMap[inPts[numPoints - 1 - ptIdx]];
          }
          outLabel[0] = inLabel[1];
          outLabel[1] = inLabel[0];
        }
        else
        {
          // Keep original winding
          for (vtkIdType ptIdx = 0; ptIdx < numPoints; ++ptIdx)
          {
            outPts[ptIdx] = this->PointMap[inPts[ptIdx]];
          }
          outLabel[0] = inLabel[0];
          outLabel[1] = inLabel[1];
        }
      }
    }
  }
};

//------------------------------------------------------------------------------
template <bool IsPatch>
struct BuildOutputCellsWorker
{
  template <typename TScalarArray>
  void operator()(TScalarArray* inBoundaryLabels, vtkDataArray* outBoundaryLabels,
    vtkIdType targetLabel, const std::vector<vtkIdType>& pointMap,
    const std::vector<vtkIdType>& subsetCellIds, vtkCellArray* inCells, vtkCellArray* outCells,
    vtkIdType offset)
  {
    BuildOutputCells<TScalarArray, IsPatch> worker(inBoundaryLabels,
      TScalarArray::FastDownCast(outBoundaryLabels), targetLabel, pointMap, subsetCellIds, offset);
    vtkSMPTools::For(0, static_cast<vtkIdType>(subsetCellIds.size()),
      [&](vtkIdType begin, vtkIdType end)
      {
        inCells->Dispatch(
          worker, outCells->GetOffsetsArray(), outCells->GetConnectivityArray(), begin, end);
      });
  }
};
}

//------------------------------------------------------------------------------
template <bool IsPatch>
vtkSmartPointer<vtkPolyData> vtkSurfaceNetsAtlas::ExtractLabel(vtkPolyData* source,
  const std::vector<const std::vector<vtkIdType>*>& groups, vtkIdType label,
  std::atomic<unsigned char>* ptUses, std::vector<vtkIdType>& pointMap) const
{
  auto output = vtkSmartPointer<vtkPolyData>::New();

  vtkIdType numSubsetCells = 0;
  for (const auto* group : groups)
  {
    numSubsetCells += static_cast<vtkIdType>(group->size());
  }

  if (numSubsetCells == 0)
  {
    return output;
  }

  vtkCellArray* inCells = source->GetNumberOfLines() > 0 ? source->GetLines() : source->GetPolys();
  const vtkIdType numInputPts = source->GetNumberOfPoints();

  // 1. Mark used points per group (parallel within each group).
  MarkPointUses markPoints(ptUses, numInputPts);
  for (const auto* group : groups)
  {
    if (group->empty())
    {
      continue;
    }
    markPoints.SetGroup(*group);
    vtkSMPTools::For(0, static_cast<vtkIdType>(group->size()),
      [&](vtkIdType begin, vtkIdType end) { inCells->Dispatch(markPoints, begin, end); });
  }

  // 2. Build Point Map
  BuildPointMap buildPointMap(markPoints.Batches, ptUses, pointMap);
  vtkSMPTools::For(0, markPoints.Batches.GetNumberOfBatches(), buildPointMap);
  const vtkIdType numNewPts = buildPointMap.TotalKeptPoints;

  // 3. Build output points (and NM indices if present)
  auto inPointsArray = source->GetPoints()->GetData();

  auto outPts = vtkSmartPointer<vtkPoints>::New();
  outPts->SetDataType(inPointsArray->GetDataType());
  outPts->SetNumberOfPoints(numNewPts);
  output->SetPoints(outPts);

  auto srcNmArray =
    vtkTypeInt8Array::FastDownCast(source->GetPointData()->GetArray(NM_INDICES_ARRAY));

  vtkSmartPointer<vtkTypeInt8Array> dstNmArray;
  if (srcNmArray)
  {
    dstNmArray = vtkSmartPointer<vtkTypeInt8Array>::New();
    dstNmArray->SetName(NM_INDICES_ARRAY);
    dstNmArray->SetNumberOfValues(numNewPts);
    output->GetPointData()->AddArray(dstNmArray);
  }

  // 4. Setup Output Cells and BoundaryLabels
  auto outCells = vtkSmartPointer<vtkCellArray>::New();
  if (inCells->IsStorageFixedSize())
  {
    const vtkIdType isHomogeneous = inCells->IsHomogeneous();
    outCells->UseFixedSizeDefaultStorage(isHomogeneous);
    outCells->ResizeExact(numSubsetCells, numSubsetCells * isHomogeneous);
  }
  else
  {
    // compute the offsets
    vtkNew<vtkAOSDataArrayTemplate<vtkIdType>> outOffsets;
    outOffsets->SetNumberOfValues(numSubsetCells + 1);
    vtkIdType offset = 0;
    for (const auto* groupPtr : groups)
    {
      const auto& group = *groupPtr;
      const vtkIdType groupSize = static_cast<vtkIdType>(group.size());
      const vtkIdType base = offset;

      vtkSMPTools::For(0, groupSize,
        [&](vtkIdType begin, vtkIdType end)
        {
          for (vtkIdType i = begin; i < end; ++i)
          {
            const vtkIdType& inCellId = group[i];
            outOffsets->SetValue(base + i, inCells->GetCellSize(inCellId));
          }
        });

      offset += groupSize;
    }
    const vtkIdType totalConnSize = vtkSMPTools::ExclusiveScan(
      outOffsets->GetPointer(0), outOffsets->GetPointer(numSubsetCells), vtkIdType{ 0 });
    outOffsets->SetValue(numSubsetCells, totalConnSize);

    vtkNew<vtkAOSDataArrayTemplate<vtkIdType>> outConn;
    outConn->SetNumberOfValues(totalConnSize);
    outCells->SetData(outOffsets, outConn);
  }

  auto inBoundaryLabels = source->GetCellData()->GetArray(BOUNDARY_LABELS_ARRAY);

  // Allocate output BoundaryLabels with the same underlying type as the input.
  auto outBoundaryLabels = vtk::TakeSmartPointer(inBoundaryLabels->NewInstance());
  outBoundaryLabels->SetName(BOUNDARY_LABELS_ARRAY);
  outBoundaryLabels->SetNumberOfComponents(2);
  outBoundaryLabels->SetNumberOfTuples(numSubsetCells);

  BuildOutputCellsWorker<IsPatch> buildCells;
  vtkIdType offset = 0;
  for (const auto* group : groups)
  {
    if (group->empty())
    {
      continue;
    }
    if (!vtkArrayDispatch::DispatchByArray<vtkArrayDispatch::AOSArrays>::Execute(inBoundaryLabels,
          buildCells, outBoundaryLabels, label, pointMap, *group, inCells, outCells, offset))
    {
      buildCells(
        inBoundaryLabels, outBoundaryLabels, label, pointMap, *group, inCells, outCells, offset);
    }
    offset += static_cast<vtkIdType>(group->size());
  }

  if (source->GetNumberOfLines() > 0)
  {
    output->SetLines(outCells);
  }
  else
  {
    output->SetPolys(outCells);
  }
  output->GetCellData()->SetScalars(outBoundaryLabels);

  // 5. Copy point coordinates and reset ptUses/pointMap in one pass over the non-empty batches.
  // Cells are already built above, so pointMap can be reset here safely.
  if (srcNmArray)
  {
    BuildOutputPointsWorker<true> worker;
    if (!vtkArrayDispatch::DispatchByArray<vtkArrayDispatch::PointArrays>::Execute(inPointsArray,
          worker, outPts->GetData(), markPoints.Batches, ptUses, pointMap, srcNmArray, dstNmArray))
    {
      worker(inPointsArray, outPts->GetData(), markPoints.Batches, ptUses, pointMap, srcNmArray,
        dstNmArray);
    }
  }
  else
  {
    BuildOutputPointsWorker<false> worker;
    if (!vtkArrayDispatch::DispatchByArray<vtkArrayDispatch::PointArrays>::Execute(inPointsArray,
          worker, outPts->GetData(), markPoints.Batches, ptUses, pointMap, nullptr, nullptr))
    {
      worker(
        inPointsArray, outPts->GetData(), markPoints.Batches, ptUses, pointMap, nullptr, nullptr);
    }
  }

  return output;
}

namespace
{
//------------------------------------------------------------------------------
struct ResolveNMPoints : public vtkCellArray::DispatchUtilities
{
  vtkDataArray* PointsArray;
  vtkPolyData* Mesh;
  const std::vector<vtkIdType>& TargetPoints;

  ResolveNMPoints(
    vtkDataArray* pointsArray, vtkPolyData* mesh, const std::vector<vtkIdType>& targetPoints)
    : PointsArray(pointsArray)
    , Mesh(mesh)
    , TargetPoints(targetPoints)
  {
  }

  // Work item produced by the parallel analysis phase: describes how to
  // split one non-manifold point. Component 0 keeps the original ptId;
  // each entry in CompCells holds the cell IDs for a subsequent component
  // that needs a freshly allocated duplicate point.
  struct SplitTask
  {
    vtkIdType PointId;
    std::vector<std::vector<vtkIdType>> CompCells;
  };

  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* offsets, ConnectivityT* conn)
  {
    auto offsetRange = GetRange(offsets);
    auto connRange = GetRange(conn);

    const vtkIdType numTargets = static_cast<vtkIdType>(this->TargetPoints.size());

    // -----------------------------------------------------------------------
    // Phase 1 (parallel, read-only): for each candidate point run the
    // union-find analysis and record the split plan in thread-local storage.
    // -----------------------------------------------------------------------
    vtkSMPThreadLocal<std::vector<SplitTask>> tlTasks;
    vtkSMPThreadLocalObject<vtkIdList> tlIncidentCells;

    vtkSMPTools::For(0, numTargets,
      [&](vtkIdType begin, vtkIdType end)
      {
        auto& localTasks = tlTasks.Local();
        vtkIdList* incidentCells = tlIncidentCells.Local();

        std::vector<std::vector<vtkIdType>> otherPts;
        std::vector<int> parent;
        std::vector<std::vector<int>> componentCells;

        for (vtkIdType t = begin; t < end; ++t)
        {
          const vtkIdType ptId = this->TargetPoints[t];
          this->Mesh->GetPointCells(ptId, incidentCells);
          const int numIncident = static_cast<int>(incidentCells->GetNumberOfIds());

          if (numIncident <= 1)
          {
            continue;
          }

          // Read "other" points for each incident cell.
          otherPts.resize(numIncident);
          for (int i = 0; i < numIncident; ++i)
          {
            const vtkIdType cellId = incidentCells->GetId(i);
            const vtkIdType offsetBegin = offsetRange[cellId];
            const vtkIdType offsetEnd = offsetRange[cellId + 1];
            auto pts = connRange.begin() + offsetBegin;
            const vtkIdType numPoints = offsetEnd - offsetBegin;
            otherPts[i].clear();
            for (vtkIdType k = 0; k < numPoints; ++k)
            {
              const vtkIdType p = pts[k];
              if (p != ptId)
              {
                otherPts[i].push_back(p);
              }
            }
            std::sort(otherPts[i].begin(), otherPts[i].end());
          }

          // Union-find (path halving) over incident cells.
          parent.resize(numIncident);
          for (int i = 0; i < numIncident; ++i)
          {
            parent[i] = i;
          }
          auto find = [&](int x)
          {
            while (parent[x] != x)
            {
              parent[x] = parent[parent[x]];
              x = parent[x];
            }
            return x;
          };

          // For each cell pair, intersect their sorted other-point arrays.
          for (int i = 0; i < numIncident; ++i)
          {
            for (int j = i + 1; j < numIncident; ++j)
            {
              const auto& a = otherPts[i];
              const auto& b = otherPts[j];
              const int sizeA = static_cast<int>(a.size());
              const int sizeB = static_cast<int>(b.size());
              for (int ai = 0, bi = 0; ai < sizeA && bi < sizeB;)
              {
                if (a[ai] == b[bi])
                {
                  const int ra = find(i), rb = find(j);
                  if (ra != rb)
                  {
                    parent[ra] = rb;
                  }
                  break;
                }
                a[ai] < b[bi] ? ++ai : ++bi;
              }
            }
          }

          // Group cells by union-find root into a flat vector
          componentCells.assign(numIncident, {});
          for (int i = 0; i < numIncident; ++i)
          {
            componentCells[find(i)].push_back(i);
          }

          int numComponents = 0;
          for (const auto& comp : componentCells)
          {
            numComponents += !comp.empty();
          }

          if (numComponents <= 1)
          {
            continue;
          }

          SplitTask task;
          task.PointId = ptId;

          // Record components 1..N; component 0 keeps the original ptId.
          bool firstComp = true;
          for (const auto& comp : componentCells)
          {
            if (comp.empty())
            {
              continue;
            }
            if (firstComp)
            {
              firstComp = false;
              continue;
            }
            task.CompCells.emplace_back();
            for (const int localIdx : comp)
            {
              task.CompCells.back().push_back(incidentCells->GetId(localIdx));
            }
          }
          localTasks.push_back(std::move(task));
        }
      });

    // -----------------------------------------------------------------------
    // Phase 2 (sequential): merge thread-local results into a single vector,
    // sort by PointId for deterministic output, then allocate new points and
    // rewrite connectivity. Non-manifold points are rare so this is cheap.
    // -----------------------------------------------------------------------
    std::vector<SplitTask> allTasks;
    for (auto& localTasks : tlTasks)
    {
      for (auto& task : localTasks)
      {
        allTasks.push_back(std::move(task));
      }
    }
    std::sort(allTasks.begin(), allTasks.end(),
      [](const SplitTask& a, const SplitTask& b) { return a.PointId < b.PointId; });

    for (const auto& task : allTasks)
    {
      for (const auto& cellIds : task.CompCells)
      {
        double x[3];
        this->PointsArray->GetTuple(task.PointId, x);
        const vtkIdType newPtId = this->PointsArray->InsertNextTuple(x);
        // Rewrite connectivity in-place
        for (const vtkIdType cellId : cellIds)
        {
          const vtkIdType offsetBegin = offsetRange[cellId];
          const vtkIdType offsetEnd = offsetRange[cellId + 1];
          auto pts = connRange.begin() + offsetBegin;
          const vtkIdType numPoints = offsetEnd - offsetBegin;
          for (vtkIdType k = 0; k < numPoints; ++k)
          {
            if (pts[k] == task.PointId)
            {
              pts[k] = newPtId;
              break;
            }
          }
        }
      }
    }
  }
};

//------------------------------------------------------------------------------
// In-place: for each candidate point, attempt to split its local cell fan by
// connected component. If the input carries a NonManifoldTableIndices array
// (produced by vtkSurfaceNets3D), only points marked -1 are candidates;
// otherwise every point is scanned. The array is always removed from the mesh
// so it never appears in the output.
void ResolveNonManifoldPointsInPlace(vtkPolyData* mesh)
{
  vtkPointData* pd = mesh->GetPointData();
  const vtkIdType numPts = mesh->GetNumberOfPoints();

  std::vector<vtkIdType> targetPoints;
  if (pd->HasArray(NM_INDICES_ARRAY))
  {
    auto* nmIdx = vtkTypeInt8Array::SafeDownCast(pd->GetArray(NM_INDICES_ARRAY));
    // Find all -1 points up front so we don't iterate while mutating.
    for (vtkIdType p = 0; p < numPts; ++p)
    {
      if (nmIdx->GetValue(p) == -1)
      {
        targetPoints.push_back(p);
      }
    }
    pd->RemoveArray(NM_INDICES_ARRAY);
  }
  else
  {
    targetPoints.resize(numPts);
    for (vtkIdType p = 0; p < numPts; ++p)
    {
      targetPoints[p] = p;
    }
  }
  if (targetPoints.empty())
  {
    return;
  }

  mesh->BuildLinks();
  vtkCellArray* cells = mesh->GetNumberOfLines() > 0 ? mesh->GetLines() : mesh->GetPolys();

  ResolveNMPoints worker(mesh->GetPoints()->GetData(), mesh, targetPoints);
  cells->Dispatch(worker);
}
} // anonymous namespace

//------------------------------------------------------------------------------
void vtkSurfaceNetsAtlas::BuildOutput(
  vtkPolyData* input, vtkPartitionedDataSetCollection* output) const
{
  const vtkIdType bgLabel = this->BackgroundLabel;
  const vtkIdType bgLID = this->GetLIDForLabel(bgLabel);
  const int numLabels = static_cast<int>(this->Atlas->LIDToLabel.size());

  // 1. Determine which LIDs to emit as Regions.
  std::vector<int> regionLIDs;
  regionLIDs.reserve(numLabels);
  if (this->ExtractionMode == EXTRACT_ALL)
  {
    for (int lid = 0; lid < numLabels; ++lid)
    {
      if (this->Atlas->LIDToLabel[lid] != bgLabel)
      {
        regionLIDs.push_back(lid);
      }
    }
  }
  else // extract subset
  {
    for (vtkIdType label : this->Labels)
    {
      const int lid = this->GetLIDForLabel(label);
      if (lid != -1)
      {
        regionLIDs.push_back(lid);
      }
      else
      {
        vtkWarningMacro("Requested label " << label << " is not present; skipping.");
      }
    }
  }
  regionLIDs.shrink_to_fit();

  // 2. Setup Data Assembly.
  auto assembly = vtkSmartPointer<vtkDataAssembly>::New();
  assembly->SetRootNodeName("Atlas");
  const int regionsNode = (this->GenerateRegions) ? assembly->AddNode("Regions") : -1;
  const int patchesNode = (this->GeneratePatches) ? assembly->AddNode("Patches") : -1;

  vtkIdType nextPDSIdx = 0;

  // Allocate scratch buffers once for the full mesh; reused across all ExtractLabel calls.
  // ptUses is zero-initialized here and reset in BuildOutputPoints
  // pointMap is -1-initialized here and reset the same way.
  const vtkIdType numInputPts = input->GetNumberOfPoints();
  auto ptUses = std::make_unique<std::atomic<unsigned char>[]>(numInputPts);
  std::vector<vtkIdType> pointMap(numInputPts, -1);

  // 3. Build Regions.
  if (this->GenerateRegions)
  {
    for (const int& lid : regionLIDs)
    {
      const vtkIdType label = this->Atlas->LIDToLabel[lid];
      vtkSmartPointer<vtkPolyData> regionMesh;

      std::vector<const std::vector<vtkIdType>*> groups;
      for (int patchID : this->Atlas->LIDToPatches[lid])
      {
        const auto& patch = this->Atlas->Patches[patchID];
        if (this->OutputStyle == OUTPUT_STYLE_BOUNDARY && patch.LID0 != bgLID &&
          patch.LID1 != bgLID)
        {
          continue;
        }
        groups.push_back(&this->Atlas->PatchCells[patchID]);
      }
      regionMesh =
        this->ExtractLabel</*isPatch=*/false>(input, groups, label, ptUses.get(), pointMap);
      if (this->ResolveNonManifoldPoints && regionMesh->GetNumberOfCells() > 0)
      {
        ResolveNonManifoldPointsInPlace(regionMesh);
      }

      // 5. Add label/lid array information
      vtkNew<vtkConstantArray<vtkIdType>> labelArray;
      labelArray->SetName("Label");
      labelArray->ConstructBackend(label);
      labelArray->SetNumberOfTuples(regionMesh->GetNumberOfCells());
      regionMesh->GetCellData()->AddArray(labelArray);

      vtkNew<vtkConstantArray<int>> lidArray;
      lidArray->SetName("LID");
      lidArray->ConstructBackend(lid);
      lidArray->SetNumberOfTuples(regionMesh->GetNumberOfCells());
      regionMesh->GetCellData()->AddArray(lidArray);

      // Adjacent labels (one per patch touching this region).
      auto adjacentLabels = this->GetAdjacentLabels(this->Atlas->LIDToLabel[lid]);
      vtkNew<vtkAOSDataArrayTemplate<vtkIdType>> adjacentLabelsArray;
      adjacentLabelsArray->SetName("AdjacentLabels");
      adjacentLabelsArray->SetNumberOfValues(static_cast<vtkIdType>(adjacentLabels.size()));
      for (vtkIdType k = 0; k < static_cast<vtkIdType>(adjacentLabels.size()); ++k)
      {
        adjacentLabelsArray->SetValue(k, adjacentLabels[k]);
      }
      regionMesh->GetFieldData()->AddArray(adjacentLabelsArray);

      const auto& patchIndices = this->Atlas->LIDToPatches[lid];
      vtkNew<vtkAOSDataArrayTemplate<int>> patchIDsArr;
      patchIDsArr->SetName("PatchIDs");
      patchIDsArr->SetNumberOfValues(static_cast<vtkIdType>(patchIndices.size()));
      for (vtkIdType k = 0; k < static_cast<vtkIdType>(patchIndices.size()); ++k)
      {
        patchIDsArr->SetValue(k, patchIndices[k]);
      }
      regionMesh->GetFieldData()->AddArray(patchIDsArr);

      auto pds = vtkSmartPointer<vtkPartitionedDataSet>::New();
      pds->SetNumberOfPartitions(1);
      pds->SetPartition(0, regionMesh);
      output->SetPartitionedDataSet(nextPDSIdx, pds);

      const std::string& labelName = this->Atlas->LabelNames.at(label);
      const int node = assembly->AddNode(labelName.c_str(), regionsNode);
      assembly->AddDataSetIndex(node, static_cast<unsigned int>(nextPDSIdx));
      assembly->SetAttribute(node, "Name", labelName.c_str());
      assembly->SetAttribute(node, "Label", label);
      assembly->SetAttribute(node, "LID", lid);

      ++nextPDSIdx;
    }
  } // GenerateRegions

  // 4. Build Patches (if requested).
  if (this->GeneratePatches)
  {
    // Build a quick lookup for selected regions.
    std::unordered_set<vtkIdType> selectedLIDs(regionLIDs.begin(), regionLIDs.end());

    // traverse all patches
    for (size_t patchID = 0; patchID < this->Atlas->Patches.size(); ++patchID)
    {
      const auto& p = this->Atlas->Patches[patchID];

      // In BOUNDARY mode skip patches that touch the background (those belong to regions).
      if (this->OutputStyle == OUTPUT_STYLE_BOUNDARY && (p.LID0 == bgLID || p.LID1 == bgLID))
      {
        continue;
      }
      // Background is never in selectedLIDs, so treat it as implicitly selected.
      if (!(p.LID0 == bgLID || selectedLIDs.count(p.LID0)) ||
        !(p.LID1 == bgLID || selectedLIDs.count(p.LID1)))
      {
        continue;
      }

      vtkSmartPointer<vtkPolyData> patchMesh;
      const std::vector<const std::vector<vtkIdType>*> groups = {
        &this->Atlas->PatchCells[patchID]
      };
      patchMesh = this->ExtractLabel</*isPatch=*/true>(input, groups, -1, ptUses.get(), pointMap);

      const vtkIdType label0 = this->Atlas->LIDToLabel[p.LID0];
      const vtkIdType label1 = this->Atlas->LIDToLabel[p.LID1];

      // PatchID constant cell-data
      vtkNew<vtkConstantArray<int>> patchIDArray;
      patchIDArray->SetName("PatchID");
      patchIDArray->ConstructBackend(static_cast<int>(patchID));
      patchIDArray->SetNumberOfValues(patchMesh->GetNumberOfCells());
      patchMesh->GetCellData()->AddArray(patchIDArray);

      auto pds = vtkSmartPointer<vtkPartitionedDataSet>::New();
      pds->SetNumberOfPartitions(1);
      pds->SetPartition(0, patchMesh);
      output->SetPartitionedDataSet(nextPDSIdx, pds);

      const std::string& pName0 = this->Atlas->LabelNames.at(label0);
      const std::string& pName1 = this->Atlas->LabelNames.at(label1);
      const std::string nodeName = pName0 + "_" + pName1;
      const int node = assembly->AddNode(nodeName.c_str(), patchesNode);
      assembly->AddDataSetIndex(node, static_cast<unsigned int>(nextPDSIdx));
      assembly->SetAttribute(node, "Name0", pName0.c_str());
      assembly->SetAttribute(node, "Name1", pName1.c_str());
      assembly->SetAttribute(node, "Label0", label0);
      assembly->SetAttribute(node, "Label1", label1);
      assembly->SetAttribute(node, "LID0", static_cast<int>(p.LID0));
      assembly->SetAttribute(node, "LID1", static_cast<int>(p.LID1));

      ++nextPDSIdx;
    }
  }

  output->SetDataAssembly(assembly);

  // Attach LIDToLabel Field Data.
  vtkNew<vtkAOSDataArrayTemplate<vtkIdType>> lidArray;
  lidArray->SetName("LIDToLabel");
  lidArray->SetNumberOfValues(numLabels);
  for (vtkIdType i = 0; i < numLabels; ++i)
  {
    lidArray->SetValue(i, this->Atlas->LIDToLabel[i]);
  }
  output->GetFieldData()->AddArray(lidArray);

  // Attach PatchLIDs: maps patchID -> [LID0, LID1]
  const vtkIdType numPatches = static_cast<vtkIdType>(this->Atlas->Patches.size());
  vtkNew<vtkAOSDataArrayTemplate<int>> patchLIDsArray;
  patchLIDsArray->SetName("PatchLIDs");
  patchLIDsArray->SetNumberOfComponents(2);
  patchLIDsArray->SetNumberOfTuples(numPatches);
  for (vtkIdType i = 0; i < numPatches; ++i)
  {
    patchLIDsArray->SetTypedComponent(i, 0, this->Atlas->Patches[i].LID0);
    patchLIDsArray->SetTypedComponent(i, 1, this->Atlas->Patches[i].LID1);
  }
  output->GetFieldData()->AddArray(patchLIDsArray);

  // Attach PatchLabels: maps patchID -> [Label0, Label1] (Label0 < Label1)
  vtkNew<vtkAOSDataArrayTemplate<vtkIdType>> patchLabelsArray;
  patchLabelsArray->SetName("PatchLabels");
  patchLabelsArray->SetNumberOfComponents(2);
  patchLabelsArray->SetNumberOfTuples(numPatches);
  for (vtkIdType i = 0; i < numPatches; ++i)
  {
    patchLabelsArray->SetTypedComponent(
      i, 0, this->Atlas->LIDToLabel[this->Atlas->Patches[i].LID0]);
    patchLabelsArray->SetTypedComponent(
      i, 1, this->Atlas->LIDToLabel[this->Atlas->Patches[i].LID1]);
  }
  output->GetFieldData()->AddArray(patchLabelsArray);
}

// ============================================================================
// RequestData entry point
// ============================================================================

int vtkSurfaceNetsAtlas::RequestData(vtkInformation* /*request*/,
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto input = vtkPolyData::GetData(inputVector[0]);
  auto output = vtkPartitionedDataSetCollection::GetData(outputVector);

  if (!input || !output)
  {
    return 0;
  }
  if (input->GetNumberOfCells() == 0)
  {
    // Nothing to do, but still a valid output (empty).
    return 1;
  }
  if (input->GetCellData() == nullptr)
  {
    vtkErrorMacro("Input has no cell data; expected output a SurfaceNets filter.");
    return 0;
  }
  auto boundaryLabels = input->GetCellData()->GetArray(BOUNDARY_LABELS_ARRAY);
  if (!boundaryLabels)
  {
    vtkErrorMacro("Input is missing required cell data array '"
      << BOUNDARY_LABELS_ARRAY << "'; expected output of a SurfaceNets filter.");
    return 0;
  }
  if (boundaryLabels->GetNumberOfComponents() != 2)
  {
    vtkErrorMacro("'" << BOUNDARY_LABELS_ARRAY << "' must have 2 components.");
    return 0;
  }

  this->BuildAtlas(input);

  if (this->Atlas->LIDToLabel.empty())
  {
    // Build failed (error already emitted) or input was empty.
    return this->Atlas->LIDToLabel.empty() ? 1 : 0;
  }

  this->BuildOutput(input, output);
  return 1;
}

VTK_ABI_NAMESPACE_END
