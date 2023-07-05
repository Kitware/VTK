// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2009 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#include "vtkPPairwiseExtractHistogram2D.h"

#include "vtkArrayData.h"
#include "vtkArrayIteratorIncludes.h"
#include "vtkCollection.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkImageMedian3D.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPExtractHistogram2D.h"
#include "vtkPointData.h"
#include "vtkStatisticsAlgorithmPrivate.h"
#include "vtkTable.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedIntArray.h"

#include <set>
#include <string>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPPairwiseExtractHistogram2D);
vtkCxxSetObjectMacro(vtkPPairwiseExtractHistogram2D, Controller, vtkMultiProcessController);

vtkPPairwiseExtractHistogram2D::vtkPPairwiseExtractHistogram2D()
{
  this->Controller = nullptr;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

vtkPPairwiseExtractHistogram2D::~vtkPPairwiseExtractHistogram2D()
{
  this->SetController(nullptr);
}

void vtkPPairwiseExtractHistogram2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}

vtkExtractHistogram2D* vtkPPairwiseExtractHistogram2D::NewHistogramFilter()
{
  vtkPExtractHistogram2D* ph = vtkPExtractHistogram2D::New();
  ph->SetController(this->Controller);
  return ph;
}
VTK_ABI_NAMESPACE_END
