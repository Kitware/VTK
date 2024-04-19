// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// VTK_DEPRECATED_IN_9_3_0() warnings for this class.
#define VTK_DEPRECATION_LEVEL 0

#include "vtkAbstractCellLinks.h"

#include "vtkCellArray.h"
#include "vtkDataSet.h"
#include "vtkGarbageCollector.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkAbstractCellLinks, DataSet, vtkDataSet);

//------------------------------------------------------------------------------
vtkAbstractCellLinks::vtkAbstractCellLinks()
{
  this->DataSet = nullptr;
  this->SequentialProcessing = false;
  this->Type = vtkAbstractCellLinks::LINKS_NOT_DEFINED;
}

//------------------------------------------------------------------------------
vtkAbstractCellLinks::~vtkAbstractCellLinks()
{
  this->SetDataSet(nullptr);
}

//------------------------------------------------------------------------------
int vtkAbstractCellLinks::ComputeType(vtkIdType maxPtId, vtkIdType maxCellId, vtkCellArray* ca)
{
  return vtkAbstractCellLinks::ComputeType(maxPtId, maxCellId, ca->GetNumberOfConnectivityIds());
}

//------------------------------------------------------------------------------
int vtkAbstractCellLinks::ComputeType(
  vtkIdType maxPtId, vtkIdType maxCellId, vtkIdType connectivitySize)
{
  vtkIdType max = maxPtId;
  max = (maxCellId > max ? maxCellId : max);
  max = (connectivitySize > max ? connectivitySize : max);

  if (max < VTK_UNSIGNED_SHORT_MAX)
  {
    return vtkAbstractCellLinks::STATIC_CELL_LINKS_USHORT;
  }
  // for 64bit IDS we might be able to use a unsigned int instead
#if defined(VTK_USE_64BIT_IDS) && VTK_SIZEOF_INT == 4
  else if (max < static_cast<vtkIdType>(VTK_UNSIGNED_INT_MAX))
  {
    return vtkAbstractCellLinks::STATIC_CELL_LINKS_UINT;
  }
#endif
  return vtkAbstractCellLinks::STATIC_CELL_LINKS_IDTYPE;
}

//------------------------------------------------------------------------------
void vtkAbstractCellLinks::BuildLinks(vtkDataSet* dataset)
{
  this->SetDataSet(dataset);
  this->BuildLinks();
}

//------------------------------------------------------------------------------
void vtkAbstractCellLinks::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->DataSet)
  {
    os << indent << "DataSet: " << this->DataSet << "\n";
  }
  else
  {
    os << indent << "DataSet: (none)\n";
  }
  os << indent << "Sequential Processing: " << (this->SequentialProcessing ? "true\n" : "false\n");
  os << indent << "Type: " << this->Type << "\n";
}

//------------------------------------------------------------------------------
void vtkAbstractCellLinks::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->DataSet, "DataSet");
}
VTK_ABI_NAMESPACE_END
