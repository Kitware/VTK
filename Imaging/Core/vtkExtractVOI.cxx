/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractVOI.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractVOI.h"

#include "vtkCellData.h"
#include "vtkExtractStructuredGridHelper.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredData.h"

vtkStandardNewMacro(vtkExtractVOI);

// Construct object to extract all of the input data.
vtkExtractVOI::vtkExtractVOI()
{
  this->VOI[0] = this->VOI[2] = this->VOI[4] = 0;
  this->VOI[1] = this->VOI[3] = this->VOI[5] = VTK_INT_MAX;

  this->SampleRate[0] = this->SampleRate[1] = this->SampleRate[2] = 1;
  this->IncludeBoundary = 0;

  this->Internal = vtkExtractStructuredGridHelper::New();
}

//------------------------------------------------------------------------------
vtkExtractVOI::~vtkExtractVOI()
{
  if( this->Internal != NULL )
    {
    this->Internal->Delete();
    }
}

//------------------------------------------------------------------------------
int vtkExtractVOI::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  int i;

  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  bool emptyExtent = false;
  int uExt[6];
  for (i=0; i<3; i++)
    {
    if (this->Internal->GetSize(i) < 1)
      {
      uExt[0] = uExt[2] = uExt[4] = 0;
      uExt[1] = uExt[3] = uExt[5] = -1;
      emptyExtent = true;
      break;
      }
    }

  if (!emptyExtent)
    {
    // Find input update extent based on requested output
    // extent
    int oUExt[6];
    outputVector->GetInformationObject(0)->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), oUExt);

    if( (this->SampleRate[0]==1) &&
        (this->SampleRate[1]==1) &&
        (this->SampleRate[2]==1)  )
      {
      memcpy(uExt,oUExt,sizeof(int)*6);
      } // END if sub-sampling
    else
      {
      for (i=0; i<3; i++)
        {
        int idx = oUExt[2*i];
        if (idx < 0 || oUExt[2*i] >= (int)this->Internal->GetSize(i))
          {
          vtkWarningMacro("Requested extent outside whole extent.")
          idx = 0;
          }
        uExt[2*i] = this->Internal->GetMappedExtentValueFromIndex(i, idx);
        int jdx = oUExt[2*i+1];
        if (jdx < idx || jdx >= (int)this->Internal->GetSize(i))
          {
          vtkWarningMacro("Requested extent outside whole extent.")
          jdx = 0;
          }
        uExt[2*i + 1] = this->Internal->GetMappedExtentValueFromIndex(i, jdx);
        }
      } // END else if sub-sampling

    } // END if extent is not empty

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), uExt, 6);
  // We can handle anything.
  inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 0);

  return 1;
}

//------------------------------------------------------------------------------
int vtkExtractVOI::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  double in_origin[3];
  double in_spacing[3];
  double out_spacing[3];
  double out_origin[3];

  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  int wholeExtent[6], outWholeExt[6];

  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent);
  inInfo->Get(vtkDataObject::SPACING(),in_spacing);
  inInfo->Get(vtkDataObject::ORIGIN(),in_origin);

  this->Internal->Initialize(
      this->VOI,wholeExtent,this->SampleRate,(this->IncludeBoundary==1));
  this->Internal->GetOutputWholeExtent(outWholeExt);

  if( (this->SampleRate[0]==1) &&
      (this->SampleRate[1]==1) &&
      (this->SampleRate[2]==1) )
    {
    memcpy(out_spacing,in_spacing,sizeof(double)*3);
    memcpy(out_origin,in_origin,sizeof(double)*3);
    memcpy(outWholeExt,this->VOI,sizeof(int)*6);
    } // END if no sub-sampling
  else
    {
    // Calculate out_origin and out_spacing
    for(int dim=0; dim < 3; ++dim)
      {
      out_spacing[dim] = in_spacing[dim]*this->SampleRate[dim];
      out_origin[dim]  = in_origin[dim] + this->VOI[dim*2]*in_spacing[dim];
      }
    } // END else if sub-sampling

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               outWholeExt, 6);
  outInfo->Set(vtkDataObject::SPACING(),out_spacing,3);
  outInfo->Set(vtkDataObject::ORIGIN(),out_origin,3);

  return 1;
}

//------------------------------------------------------------------------------
int vtkExtractVOI::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  return this->RequestDataImpl(this->VOI, inputVector, outputVector) ? 1 : 0;
}

//------------------------------------------------------------------------------
bool vtkExtractVOI::RequestDataImpl(int voi[6],
                                    vtkInformationVector **inputVector,
                                    vtkInformationVector *outputVector)
{
  if( (this->SampleRate[0] < 1) ||
      (this->SampleRate[1] < 1) ||
      (this->SampleRate[2] < 1) )
    {
    vtkErrorMacro("SampleRate must be >= 1 in all 3 dimenstions!");
    return false;
    }

  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkImageData *input = vtkImageData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkImageData *output = vtkImageData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (input->GetNumberOfPoints() == 0)
    {
    return 1;
    }

  int *inExt = input->GetExtent();

  this->Internal->Initialize(voi, inExt, this->SampleRate,
                             (this->IncludeBoundary == 1));
  if (!this->Internal->IsValid())
    {
    vtkErrorMacro("Error initializing index map.");
    return 0;
    }

  // compute output spacing
  double outSpacing[3];
  input->GetSpacing(outSpacing);
  outSpacing[0] *= this->SampleRate[0];
  outSpacing[1] *= this->SampleRate[1];
  outSpacing[2] *= this->SampleRate[2];

  output->SetSpacing(outSpacing);

  vtkPointData *pd    = input->GetPointData();
  vtkCellData *cd     = input->GetCellData();
  vtkPointData *outPD = output->GetPointData();
  vtkCellData *outCD  = output->GetCellData();

  int *outExt;

  vtkDebugMacro(<< "Extracting Grid");

  // set output extent
  int begin[3];
  int end[3];
  this->Internal->ComputeBeginAndEnd(inExt, voi, begin, end);

  int inBegin[3];
  double outOrigin[3];
  if( (this->SampleRate[0]==1) &&
      (this->SampleRate[1]==1) &&
      (this->SampleRate[2]==1) )
    {
    // convert to a global extent w.r.t. the input extent
    for(int dim=0; dim < 3; ++dim)
      {
      int delta = end[ dim ]-begin[ dim ];
      begin[ dim ] = voi[ dim*2 ];
      end[ dim ]   = begin[ dim ]+delta;
      }
    memcpy(inBegin,begin,sizeof(int)*3);

    // set output origin to be the same as the input
    input->GetOrigin( outOrigin );
    } // END if no sub-sampling
  else
    {
    inBegin[0] = this->Internal->GetMappedExtentValue(0, begin[0]);
    inBegin[1] = this->Internal->GetMappedExtentValue(1, begin[1]);
    inBegin[2] = this->Internal->GetMappedExtentValue(2, begin[2]);

    // shift the origin accordingly
    // set output origin
    vtkIdType idx = vtkStructuredData::ComputePointIdForExtent(inExt,inBegin);
    input->GetPoint(idx,outOrigin);
    } // END else if sub-sampling

  output->SetExtent(begin[0], end[0], begin[1], end[1], begin[2], end[2]);
  outExt = output->GetExtent();
  output->SetOrigin( outOrigin );
  this->Internal->CopyPointsAndPointData(
      inExt,outExt,pd,NULL,outPD,NULL,this->SampleRate);
  this->Internal->CopyCellData(inExt,outExt,cd,outCD,this->SampleRate);

  return 1;
}

//------------------------------------------------------------------------------
void vtkExtractVOI::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "VOI: \n";
  os << indent << "  Imin,Imax: (" << this->VOI[0] << ", "
     << this->VOI[1] << ")\n";
  os << indent << "  Jmin,Jmax: (" << this->VOI[2] << ", "
     << this->VOI[3] << ")\n";
  os << indent << "  Kmin,Kmax: (" << this->VOI[4] << ", "
     << this->VOI[5] << ")\n";

  os << indent << "Sample Rate: (" << this->SampleRate[0] << ", "
               << this->SampleRate[1] << ", "
               << this->SampleRate[2] << ")\n";

  os << indent << "Include Boundary: "
      << (this->IncludeBoundary ? "On\n" : "Off\n");
}
