// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPOutlineCornerFilter.h"

#include "vtkAMRInformation.h"
#include "vtkAppendPolyData.h"
#include "vtkBoundingBox.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObjectTree.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationDataObjectKey.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineCornerSource.h"
#include "vtkOutlineSource.h"
#include "vtkOverlappingAMR.h"
#include "vtkPOutlineFilterInternals.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUniformGrid.h"
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPOutlineCornerFilter);
vtkCxxSetObjectMacro(vtkPOutlineCornerFilter, Controller, vtkMultiProcessController);

vtkPOutlineCornerFilter::vtkPOutlineCornerFilter()
{
  this->Controller = nullptr;
  this->SetController(vtkMultiProcessController::GetGlobalController());
  this->CornerFactor = 0.2;
  this->Internals = new vtkPOutlineFilterInternals;
  this->Internals->SetController(this->Controller);
}

vtkPOutlineCornerFilter::~vtkPOutlineCornerFilter()
{
  this->SetController(nullptr);
  this->Internals->SetController(nullptr);
  delete this->Internals;
}

//------------------------------------------------------------------------------
void vtkPOutlineCornerFilter::SetCornerFactor(double cornerFactor)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting "
                << "CornerFactor to " << CornerFactor);
  double tempCornerFactor =
    (cornerFactor < 0.001 ? 0.001 : (cornerFactor > 0.5 ? 0.5 : cornerFactor));

  if (this->CornerFactor != tempCornerFactor)
  {
    std::cerr << "CornerFactor: " << tempCornerFactor << std::endl;
    this->CornerFactor = tempCornerFactor;
    this->Internals->SetCornerFactor(tempCornerFactor);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
int vtkPOutlineCornerFilter::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  this->Internals->SetIsCornerSource(true);
  return this->Internals->RequestData(request, inputVector, outputVector);
}

int vtkPOutlineCornerFilter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

void vtkPOutlineCornerFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CornerFactor: " << this->CornerFactor << "\n";
  os << indent << "Controller: " << this->Controller << endl;
}
VTK_ABI_NAMESPACE_END
