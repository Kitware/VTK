// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkLocator.h"

#include "vtkDataSet.h"
#include "vtkGarbageCollector.h"

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkLocator, DataSet, vtkDataSet);

//------------------------------------------------------------------------------
vtkLocator::vtkLocator()
{
  this->DataSet = nullptr;
  this->Tolerance = 0.001;
  this->Automatic = 1;
  this->MaxLevel = 8;
  this->Level = 8;
  this->UseExistingSearchStructure = 0;
}

//------------------------------------------------------------------------------
vtkLocator::~vtkLocator()
{
  // commented out because of compiler problems in g++
  //  this->FreeSearchStructure();
  this->SetDataSet(nullptr);
}

//------------------------------------------------------------------------------
void vtkLocator::Initialize()
{
  // free up hash table
  this->FreeSearchStructure();
}

//------------------------------------------------------------------------------
void vtkLocator::Update()
{
  if (!this->DataSet)
  {
    vtkErrorMacro(<< "Input not set!");
    return;
  }
  if ((this->MTime > this->BuildTime) || (this->DataSet->GetMTime() > this->BuildTime))
  {
    this->BuildLocator();
  }
}

//------------------------------------------------------------------------------
void vtkLocator::PrintSelf(ostream& os, vtkIndent indent)
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

  os << indent << "Automatic: " << (this->Automatic ? "On\n" : "Off\n");
  os << indent << "Tolerance: " << this->Tolerance << "\n";
  os << indent << "Build Time: " << this->BuildTime.GetMTime() << "\n";
  os << indent << "MaxLevel: " << this->MaxLevel << "\n";
  os << indent << "Level: " << this->Level << "\n";
  os << indent << "UseExistingSearchStructure: " << this->UseExistingSearchStructure << "\n";
}

//------------------------------------------------------------------------------
void vtkLocator::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->DataSet, "DataSet");
}
VTK_ABI_NAMESPACE_END
