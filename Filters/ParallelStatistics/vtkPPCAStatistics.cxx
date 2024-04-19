// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2011 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkPPCAStatistics.h"

#include "vtkAbstractArray.h"
#include "vtkCommunicator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPMultiCorrelativeStatistics.h"
#include "vtkPOrderStatistics.h"
#include "vtkTable.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPPCAStatistics);
vtkCxxSetObjectMacro(vtkPPCAStatistics, Controller, vtkMultiProcessController);
//------------------------------------------------------------------------------
vtkPPCAStatistics::vtkPPCAStatistics()
{
  this->Controller = nullptr;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//------------------------------------------------------------------------------
vtkPPCAStatistics::~vtkPPCAStatistics()
{
  this->SetController(nullptr);
}

//------------------------------------------------------------------------------
void vtkPPCAStatistics::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}

//------------------------------------------------------------------------------
void vtkPPCAStatistics::Learn(
  vtkTable* inData, vtkTable* inParameters, vtkMultiBlockDataSet* outMeta)
{
  if (!outMeta)
  {
    return;
  }

  // First calculate correlative statistics on local data set
  this->Superclass::Learn(inData, inParameters, outMeta);

  // Get a hold of the (sparse) covariance matrix
  vtkTable* sparseCov = vtkTable::SafeDownCast(outMeta->GetBlock(0));
  if (!sparseCov)
  {
    return;
  }

  if (!this->MedianAbsoluteDeviation)
  {
    vtkPMultiCorrelativeStatistics::GatherStatistics(this->Controller, sparseCov);
  }
}

//------------------------------------------------------------------------------
void vtkPPCAStatistics::Test(vtkTable* inData, vtkMultiBlockDataSet* inMeta, vtkTable* outMeta)
{
  if (this->Controller->GetNumberOfProcesses() > 1)
  {
    vtkWarningMacro("Parallel PCA: Hypothesis testing not implemented for more than 1 process.");
    return;
  }

  this->Superclass::Test(inData, inMeta, outMeta);
}

//------------------------------------------------------------------------------
vtkOrderStatistics* vtkPPCAStatistics::CreateOrderStatisticsInstance()
{
  return vtkPOrderStatistics::New();
}
VTK_ABI_NAMESPACE_END
