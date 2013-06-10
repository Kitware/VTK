/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageToStructuredPoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageToStructuredPoints.h"

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredPoints.h"

#include <math.h>

vtkStandardNewMacro(vtkImageToStructuredPoints);

//----------------------------------------------------------------------------
vtkImageToStructuredPoints::vtkImageToStructuredPoints()
{
  this->SetNumberOfInputPorts(2);
  this->Translate[0] = this->Translate[1] = this->Translate[2] = 0;
}

//----------------------------------------------------------------------------
vtkImageToStructuredPoints::~vtkImageToStructuredPoints()
{
}


//----------------------------------------------------------------------------
void vtkImageToStructuredPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
vtkStructuredPoints* vtkImageToStructuredPoints::GetStructuredPointsOutput()
{
  return vtkStructuredPoints::SafeDownCast(this->GetOutputDataObject(0));
}

//----------------------------------------------------------------------------
void vtkImageToStructuredPoints::SetVectorInputData(vtkImageData *input)
{
  this->SetInputData(1, input);
}

//----------------------------------------------------------------------------
vtkImageData *vtkImageToStructuredPoints::GetVectorInput()
{
  if (this->GetNumberOfInputConnections(1) < 1)
    {
    return NULL;
    }

  return vtkImageData::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));
}

//----------------------------------------------------------------------------
int vtkImageToStructuredPoints::RequestData(
  vtkInformation *,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *vectorInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  int uExtent[6];
  int *wExtent;

  int idxX, idxY, idxZ;
  int maxX = 0;
  int maxY = 0;
  int maxZ = 0;;
  vtkIdType inIncX, inIncY, inIncZ;
  int rowLength;
  unsigned char *inPtr1, *inPtr, *outPtr;
  vtkStructuredPoints *output = vtkStructuredPoints::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkImageData *data = vtkImageData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkImageData *vData = 0;
  if (vectorInfo)
    {
    vData = vtkImageData::SafeDownCast(
      vectorInfo->Get(vtkDataObject::DATA_OBJECT()));
    }

  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
               uExtent);
  output->SetExtent(uExtent);

  uExtent[0] += this->Translate[0];
  uExtent[1] += this->Translate[0];
  uExtent[2] += this->Translate[1];
  uExtent[3] += this->Translate[1];
  uExtent[4] += this->Translate[2];
  uExtent[5] += this->Translate[2];

  // if the data extent matches the update extent then just pass the data
  // otherwise we must reformat and copy the data
  if (data)
    {
    wExtent = data->GetExtent();
    if (wExtent[0] == uExtent[0] && wExtent[1] == uExtent[1] &&
        wExtent[2] == uExtent[2] && wExtent[3] == uExtent[3] &&
        wExtent[4] == uExtent[4] && wExtent[5] == uExtent[5])
      {
      if (data->GetPointData())
        {
        output->GetPointData()->PassData(data->GetPointData());
        }
      if (data->GetCellData())
        {
        output->GetCellData()->PassData(data->GetCellData());
        }
      if (data->GetFieldData())
        {
        output->GetFieldData()->ShallowCopy(data->GetFieldData());
        }
      }
    else
      {
      inPtr =
        static_cast<unsigned char *>(data->GetScalarPointerForExtent(uExtent));
      outPtr = static_cast<unsigned char *>(output->GetScalarPointer());

      // Make sure there are data.
      if(!inPtr || !outPtr)
        {
        output->Initialize();
        return 1;
        }

      // Get increments to march through data
      data->GetIncrements(inIncX, inIncY, inIncZ);

      // find the region to loop over
      rowLength = (uExtent[1] - uExtent[0]+1)*inIncX*data->GetScalarSize();
      maxX = uExtent[1] - uExtent[0];
      maxY = uExtent[3] - uExtent[2];
      maxZ = uExtent[5] - uExtent[4];
      inIncY *= data->GetScalarSize();
      inIncZ *= data->GetScalarSize();

      // Loop through output pixels
      for (idxZ = 0; idxZ <= maxZ; idxZ++)
        {
        inPtr1 = inPtr + idxZ*inIncZ;
        for (idxY = 0; idxY <= maxY; idxY++)
          {
          memcpy(outPtr,inPtr1,rowLength);
          inPtr1 += inIncY;
          outPtr += rowLength;
          }
        }
      }
    }

  if (vData)
    {
    // if the data extent matches the update extent then just pass the data
    // otherwise we must reformat and copy the data
    wExtent = vData->GetExtent();
    if (wExtent[0] == uExtent[0] && wExtent[1] == uExtent[1] &&
        wExtent[2] == uExtent[2] && wExtent[3] == uExtent[3] &&
        wExtent[4] == uExtent[4] && wExtent[5] == uExtent[5])
      {
      output->GetPointData()->SetVectors(vData->GetPointData()->GetScalars());
      }
    else
      {
      vtkDataArray *fv = vtkDataArray::CreateDataArray(vData->GetScalarType());
      float *inPtr2 =
        static_cast<float *>(vData->GetScalarPointerForExtent(uExtent));

      // Make sure there are data.
      if(!inPtr2)
        {
        output->Initialize();
        return 1;
        }

      fv->SetNumberOfComponents(3);
      fv->SetNumberOfTuples((maxZ+1)*(maxY+1)*(maxX+1));
      vData->GetContinuousIncrements(uExtent, inIncX, inIncY, inIncZ);
      int numComp = vData->GetNumberOfScalarComponents();
      int idx = 0;

      // Loop through output pixels
      for (idxZ = 0; idxZ <= maxZ; idxZ++)
        {
        for (idxY = 0; idxY <= maxY; idxY++)
          {
          for (idxX = 0; idxX <= maxX; idxX++)
            {
            fv->SetTuple(idx,inPtr2);
            inPtr2 += numComp;
            idx++;
            }
          inPtr2 += inIncY;
          }
        inPtr2 += inIncZ;
        }
      output->GetPointData()->SetVectors(fv);
      fv->Delete();
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
// Copy WholeExtent, Spacing and Origin.
int vtkImageToStructuredPoints::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *vInfo = inputVector[1]->GetInformationObject(0);

  int whole[6], *tmp;
  double *spacing, origin[3];

  vtkInformation *inScalarInfo = vtkDataObject::GetActiveFieldInformation(inInfo,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
  if (!inScalarInfo)
    {
    vtkErrorMacro("Missing scalar field on input information!");
    return 0;
    }
  vtkDataObject::SetPointDataActiveScalarInfo(outInfo,
    inScalarInfo->Get( vtkDataObject::FIELD_ARRAY_TYPE() ),
    inScalarInfo->Get( vtkDataObject::FIELD_NUMBER_OF_COMPONENTS() ) );

  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),whole);
  spacing = inInfo->Get(vtkDataObject::SPACING());
  inInfo->Get(vtkDataObject::ORIGIN(), origin);

  // intersections for whole extent
  if (vInfo)
    {
    tmp = vInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
    if (tmp[0] > whole[0]) {whole[0] = tmp[0];}
    if (tmp[2] > whole[2]) {whole[2] = tmp[2];}
    if (tmp[4] > whole[4]) {whole[4] = tmp[4];}
    if (tmp[1] < whole[1]) {whole[1] = tmp[1];}
    if (tmp[3] < whole[1]) {whole[3] = tmp[3];}
    if (tmp[5] < whole[1]) {whole[5] = tmp[5];}
    }

  // slide min extent to 0,0,0 (I Hate this !!!!)
  this->Translate[0] = whole[0];
  this->Translate[1] = whole[2];
  this->Translate[2] = whole[4];

  origin[0] += spacing[0] * whole[0];
  origin[1] += spacing[1] * whole[2];
  origin[2] += spacing[2] * whole[4];
  whole[1] -= whole[0];
  whole[3] -= whole[2];
  whole[5] -= whole[4];
  whole[0] = whole[2] = whole[4] = 0;

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),whole,6);
  // Now should Origin and Spacing really be part of information?
  // How about xyx arrays in RectilinearGrid of Points in StructuredGrid?
  outInfo->Set(vtkDataObject::ORIGIN(),origin,3);
  outInfo->Set(vtkDataObject::SPACING(),spacing,3);

  return 1;
}

//----------------------------------------------------------------------------
int vtkImageToStructuredPoints::RequestUpdateExtent(
  vtkInformation *,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *vInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  int ext[6];

  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), ext);
  ext[0] += this->Translate[0];
  ext[1] += this->Translate[0];
  ext[2] += this->Translate[1];
  ext[3] += this->Translate[1];
  ext[4] += this->Translate[2];
  ext[5] += this->Translate[2];

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), ext, 6);

  if (vInfo)
    {
    vInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), ext, 6);
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkImageToStructuredPoints::FillOutputPortInformation(int port,
                                                          vtkInformation* info)
{
  if(!this->Superclass::FillOutputPortInformation(port, info))
    {
    return 0;
    }
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkStructuredPoints");
  return 1;
}

//----------------------------------------------------------------------------
int vtkImageToStructuredPoints::FillInputPortInformation(int port,
                                                         vtkInformation *info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }
  if (port == 1)
    {
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    }
  return 1;
}
