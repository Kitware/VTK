/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPOutlineCornerFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPOutlineCornerFilter.h"

#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationDataObjectKey.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineCornerSource.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkPOutlineCornerFilter);
vtkCxxSetObjectMacro(vtkPOutlineCornerFilter, Controller, vtkMultiProcessController);

vtkPOutlineCornerFilter::vtkPOutlineCornerFilter ()
{
  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
  this->CornerFactor = 0.2;
  this->OutlineCornerSource = vtkOutlineCornerSource::New();
}

vtkPOutlineCornerFilter::~vtkPOutlineCornerFilter ()
{
  this->SetController(0);
  if (this->OutlineCornerSource != NULL)
    {
    this->OutlineCornerSource->Delete ();
    this->OutlineCornerSource = NULL;
    }
}

int vtkPOutlineCornerFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  double bds[6];

  if ( !this->Controller )
    {
    vtkErrorMacro("Controller not set");
    return 0;
    }

  int doCommunicate = 1;
  
  // If there is a composite dataset in the input, the request is
  // coming from a vtkCompositeDataPipeline and interprocess communication
  // is not necessary (simple datasets are not broken into pieces)
  vtkCompositeDataSet* cds = vtkCompositeDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (cds)
    {
    doCommunicate = 0;
    }

  input->GetBounds(bds);
  //cerr << "Bounds: " << bds[0] << ", " << bds[1] << ", " 
  //<< bds[2] << ", " << bds[3] << ", "
  //<< bds[4] << ", " << bds[5] << endl;

  int procid = this->Controller->GetLocalProcessId();
  
  if (doCommunicate)
    {
    if ( procid )
      {
      // Satellite node
      this->Controller->Send(bds, 6, 0, 792390);
      }
    else
      {
      int numProcs = this->Controller->GetNumberOfProcesses();
      int idx;
      double tmp[6];
      
      for (idx = 1; idx < numProcs; ++idx)
        {
        this->Controller->Receive(tmp, 6, idx, 792390);
        
        if (tmp[0] < bds[0])
          {
          bds[0] = tmp[0];
          }
        if (tmp[1] > bds[1])
          {
          bds[1] = tmp[1];
          }
        if (tmp[2] < bds[2])
          {
          bds[2] = tmp[2];
          }
        if (tmp[3] > bds[3])
          {
          bds[3] = tmp[3];
          }
        if (tmp[4] < bds[4])
          {
          bds[4] = tmp[4];
          }
        if (tmp[5] > bds[5])
          {
          bds[5] = tmp[5];
          }
        }
      }
    }

  if (!doCommunicate || procid == 0)
    {
    if (vtkMath::AreBoundsInitialized(bds))
      {
      // only output in process 0.
      this->OutlineCornerSource->SetBounds(bds);          
      this->OutlineCornerSource->SetCornerFactor(this->GetCornerFactor());
      this->OutlineCornerSource->Update();
      output->CopyStructure(this->OutlineCornerSource->GetOutput());
      }
    }

  return 1;
}

int vtkPOutlineCornerFilter::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
               -1);

  return 1;
}

int vtkPOutlineCornerFilter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

void vtkPOutlineCornerFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "CornerFactor: " << this->CornerFactor << "\n";
  os << indent << "Controller: " << this->Controller << endl;
}
