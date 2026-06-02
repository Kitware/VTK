// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellGridSummaryInformationQuery.h"

#include "vtkCellAttribute.h"
#include "vtkObjectFactory.h"

#include <algorithm>
#include <limits>

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkCellGridSummaryInformationQuery);

vtkCellGridSummaryInformationQuery::~vtkCellGridSummaryInformationQuery()
{
  this->SetAttributeName(nullptr);
}

void vtkCellGridSummaryInformationQuery::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SummaryInformationMap: (" << this->SummaryInformationMap.size()
     << " attributes)\n";
  for (const auto& [att, summaryInformation] : this->SummaryInformationMap)
  {
    os << indent << "  " << att->GetName().Data() << ": OrderRange=["
       << summaryInformation.OrderRange[0] << ", " << summaryInformation.OrderRange[1]
       << "], DOFCount=" << summaryInformation.DOFCount << "\n";
  }
}

bool vtkCellGridSummaryInformationQuery::Initialize()
{
  const bool ok = this->Superclass::Initialize();
  this->SummaryInformationMap.clear();
  return ok;
}

const vtkCellGridSummaryInformationQuery::SummaryInformation&
vtkCellGridSummaryInformationQuery::GetSummaryInformation(vtkCellAttribute* att) const
{
  static constexpr SummaryInformation empty;
  auto it = this->SummaryInformationMap.find(att);
  return it != this->SummaryInformationMap.end() ? it->second : empty;
}

void vtkCellGridSummaryInformationQuery::AddSummaryInformation(
  vtkCellAttribute* att, const SummaryInformation& summaryInformation)
{
  auto it = this->SummaryInformationMap.find(att);
  if (it == this->SummaryInformationMap.end())
  {
    this->SummaryInformationMap.emplace(att, summaryInformation);
  }
  else
  {
    auto& summaryInfo = it->second;
    summaryInfo.OrderRange[0] =
      std::min(summaryInfo.OrderRange[0], summaryInformation.OrderRange[0]);
    summaryInfo.OrderRange[1] =
      std::max(summaryInfo.OrderRange[1], summaryInformation.OrderRange[1]);
    summaryInfo.DOFCount += summaryInformation.DOFCount;
  }
}

// --- Polynomial order range ------------------------------------------------

const std::array<int, 2>& vtkCellGridSummaryInformationQuery::GetOrderRange(
  vtkCellAttribute* att) const
{
  static constexpr SummaryInformation empty;
  auto it = this->SummaryInformationMap.find(att);
  return it != this->SummaryInformationMap.end() ? it->second.OrderRange : empty.OrderRange;
}

void vtkCellGridSummaryInformationQuery::GetOrderRange(vtkCellAttribute* att, int* range) const
{
  if (!range)
  {
    return;
  }
  static constexpr SummaryInformation empty;
  auto it = this->SummaryInformationMap.find(att);
  if (it == this->SummaryInformationMap.end())
  {
    range[0] = empty.OrderRange[0];
    range[1] = empty.OrderRange[1];
    return;
  }
  const auto& orderRange = it->second.OrderRange;
  range[0] = orderRange[0];
  range[1] = orderRange[1];
}

// --- Degrees of freedom ----------------------------------------------------

vtkIdType vtkCellGridSummaryInformationQuery::GetNumberOfDOF(vtkCellAttribute* att) const
{
  auto it = this->SummaryInformationMap.find(att);
  return it != this->SummaryInformationMap.end() ? it->second.DOFCount : 0;
}

VTK_ABI_NAMESPACE_END
