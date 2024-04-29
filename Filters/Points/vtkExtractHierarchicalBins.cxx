// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkExtractHierarchicalBins.h"

#include "vtkDataArray.h"
#include "vtkGarbageCollector.h"
#include "vtkHierarchicalBinningFilter.h"
#include "vtkObjectFactory.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkExtractHierarchicalBins);
vtkCxxSetObjectMacro(vtkExtractHierarchicalBins, BinningFilter, vtkHierarchicalBinningFilter);

//------------------------------------------------------------------------------
// Helper classes to support efficient computing, and threaded execution.
namespace
{

//------------------------------------------------------------------------------
// Mark points to be extracted
void MaskPoints(vtkIdType numPts, vtkIdType* map, vtkIdType offset, vtkIdType numFill)
{
  std::fill_n(map, offset, static_cast<vtkIdType>(-1));
  std::fill_n(map + offset, numFill, static_cast<vtkIdType>(1));
  std::fill_n(map + offset + numFill, numPts - (offset + numFill), static_cast<vtkIdType>(-1));
}

} // anonymous namespace

//================= Begin class proper =======================================
//------------------------------------------------------------------------------
vtkExtractHierarchicalBins::vtkExtractHierarchicalBins()
{
  this->Level = 0;
  this->Bin = -1;
  this->BinningFilter = nullptr;
}

//------------------------------------------------------------------------------
vtkExtractHierarchicalBins::~vtkExtractHierarchicalBins()
{
  this->SetBinningFilter(nullptr);
}

void vtkExtractHierarchicalBins::ReportReferences(vtkGarbageCollector* collector)
{
  // Report references held by this object that may be in a loop.
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->BinningFilter, "Binning Filter");
}

//------------------------------------------------------------------------------
// Traverse all the input points and extract points that are contained within
// and implicit function.
int vtkExtractHierarchicalBins::FilterPoints(vtkPointSet* input)
{
  // Check the input.
  if (!this->BinningFilter)
  {
    vtkErrorMacro(<< "vtkHierarchicalBinningFilter required\n");
    return 0;
  }

  // Access the correct bin and determine how many points to extract.
  vtkIdType offset;
  vtkIdType numFill;

  if (this->Level >= 0)
  {
    int level = (this->Level < this->BinningFilter->GetNumberOfLevels()
        ? this->Level
        : (this->BinningFilter->GetNumberOfLevels() - 1));
    offset = this->BinningFilter->GetLevelOffset(level, numFill);
  }
  else if (this->Bin >= 0)
  {
    int bin = (this->Level < this->BinningFilter->GetNumberOfGlobalBins()
        ? this->Bin
        : (this->BinningFilter->GetNumberOfGlobalBins() - 1));
    offset = this->BinningFilter->GetBinOffset(bin, numFill);
  }
  else // pass everything through
  {
    return 1;
  }

  vtkIdType numPts = input->GetNumberOfPoints();
  MaskPoints(numPts, this->PointMap, offset, numFill);

  return 1;
}

//------------------------------------------------------------------------------
void vtkExtractHierarchicalBins::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Level: " << this->Level << "\n";
  os << indent << "Bin: " << this->Bin << "\n";
  os << indent << "Binning Filter: " << static_cast<void*>(this->BinningFilter) << "\n";
}
VTK_ABI_NAMESPACE_END
