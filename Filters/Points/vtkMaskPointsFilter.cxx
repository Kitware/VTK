/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMaskPointsFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMaskPointsFilter.h"

#include "vtkObjectFactory.h"
#include "vtkImageData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkSMPTools.h"

vtkStandardNewMacro(vtkMaskPointsFilter);

//----------------------------------------------------------------------------
// Helper classes to support efficient computing, and threaded execution.
namespace {

//----------------------------------------------------------------------------
// The threaded core of the algorithm
template <typename T>
struct ExtractPoints
{
  unsigned char *Mask;
  unsigned char EmptyValue;
  const T *Points;
  vtkIdType *PointMap;
  double hX, hY, hZ; //internal data members for performance
  double fX, fY, fZ, bX, bY, bZ;
  vtkIdType xD, yD, zD, xyD;

  ExtractPoints(unsigned char *mask, unsigned char ev, int dims[3],
                double origin[3], double spacing[3], T *points, vtkIdType *map) :
    Mask(mask), EmptyValue(ev), Points(points), PointMap(map)
  {
    this->hX = spacing[0];
    this->hY = spacing[1];
    this->hZ = spacing[2];
    this->fX = 1.0 / spacing[0];
    this->fY = 1.0 / spacing[1];
    this->fZ = 1.0 / spacing[2];
    this->bX = origin[0] - 0.5*this->hX;
    this->bY = origin[1] - 0.5*this->hY;
    this->bZ = origin[2] - 0.5*this->hZ;
    this->xD = dims[0];
    this->yD = dims[1];
    this->zD = dims[2];
    this->xyD = dims[0] * dims[1];
  }

  void operator() (vtkIdType ptId, vtkIdType endPtId)
  {
    const T *x = this->Points + 3*ptId;
    vtkIdType *map = this->PointMap + ptId;
    const unsigned char *mask = this->Mask;
    const unsigned char emptyValue = this->EmptyValue;
    int i, j, k;

    for ( ; ptId < endPtId; ++ptId, x+=3, ++map)
    {
      i = static_cast<int>(((x[0] - this->bX) * this->fX));
      j = static_cast<int>(((x[1] - this->bY) * this->fY));
      k = static_cast<int>(((x[2] - this->bZ) * this->fZ));

      // If not inside image then skip
      if ( i < 0 || i >= this->xD ||
           j < 0 || j >= this->yD ||
           k < 0 || k >= this->zD )
      {
        *map = -1;
      }
      else if ( mask[i + j*this->xD + k*this->xyD] != emptyValue )
      {
        *map = 1;
      }
      else
      {
        *map = -1;
      }
    }
  }

  static void Execute(unsigned char *mask, unsigned char ev, int dims[3],
                      double origin[3], double spacing[3], vtkIdType numPts,
                      T *points, vtkIdType *map)
  {
    ExtractPoints extract(mask, ev, dims, origin, spacing, points, map);
    vtkSMPTools::For(0, numPts, extract);
  }

}; //ExtractPoints

} //anonymous namespace

//================= Begin class proper =======================================
//----------------------------------------------------------------------------
vtkMaskPointsFilter::vtkMaskPointsFilter()
{
  this->SetNumberOfInputPorts(2);

  this->EmptyValue = 0;
  this->Mask = NULL;
}

//----------------------------------------------------------------------------
vtkMaskPointsFilter::~vtkMaskPointsFilter()
{
}

//----------------------------------------------------------------------------
int vtkMaskPointsFilter::
FillInputPortInformation(int port, vtkInformation *info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
    return 1;
  }
  else if (port == 1)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
void vtkMaskPointsFilter::SetMaskConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//----------------------------------------------------------------------------
void vtkMaskPointsFilter::SetMaskData(vtkDataObject *input)
{
  this->SetInputData(1, input);
}

//----------------------------------------------------------------------------
vtkDataObject *vtkMaskPointsFilter::GetMask()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return NULL;
  }

  return this->GetExecutive()->GetInputData(1, 0);
}

//----------------------------------------------------------------------------
// Traverse all the input points and extract points that are contained within
// the mask.
int vtkMaskPointsFilter::FilterPoints(vtkPointSet *input)
{
  // Grab the image data and metadata. The type and existence of image data
  // should have been checked in RequestData().
  double origin[3], spacing[3];
  int dims[3];
  this->Mask->GetDimensions(dims);
  this->Mask->GetOrigin(origin);
  this->Mask->GetSpacing(spacing);
  unsigned char ev = this->EmptyValue;

  unsigned char *m =
    static_cast<unsigned char*>(this->Mask->GetScalarPointer());

  // Determine which points, if any, should be removed. We create a map
  // to keep track. The bulk of the algorithmic work is done in this pass.
  vtkIdType numPts = input->GetNumberOfPoints();
  void *inPtr = input->GetPoints()->GetVoidPointer(0);
  switch (input->GetPoints()->GetDataType())
  {
    vtkTemplateMacro(ExtractPoints<VTK_TT>::Execute(m, ev, dims, origin, spacing,
                                      numPts, (VTK_TT *)inPtr, this->PointMap));
  }

  return 1;
}

//----------------------------------------------------------------------------
// Due to the second input, retrieve it and then invoke the superclass
// RequestData.
int vtkMaskPointsFilter::RequestData(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *maskInfo = inputVector[1]->GetInformationObject(0);

  // get the mask
  this->Mask = vtkImageData::SafeDownCast(
    maskInfo->Get(vtkDataObject::DATA_OBJECT()));

  if ( !this->Mask )
  {
    vtkWarningMacro(<<"No image mask available");
    return 1;
  }

  if ( this->Mask->GetScalarType() != VTK_UNSIGNED_CHAR )
  {
    vtkWarningMacro(<<"Image mask must be unsigned char type");
    return 1;
  }

  return this->Superclass::RequestData(request, inputVector, outputVector);
}


//----------------------------------------------------------------------------
int vtkMaskPointsFilter::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *maskInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  outInfo->CopyEntry(maskInfo,
                     vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  outInfo->CopyEntry(maskInfo,
                     vtkStreamingDemandDrivenPipeline::TIME_RANGE());

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()),
               6);

  // Make sure that the scalar type and number of components
  // are propagated from the mask not the input.
  if (vtkImageData::HasScalarType(maskInfo))
  {
    vtkImageData::SetScalarType(vtkImageData::GetScalarType(maskInfo),
                                outInfo);
  }
  if (vtkImageData::HasNumberOfScalarComponents(maskInfo))
  {
    vtkImageData::SetNumberOfScalarComponents(
      vtkImageData::GetNumberOfScalarComponents(maskInfo),
      outInfo);
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkMaskPointsFilter::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *maskInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
  maskInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  maskInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  maskInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
  maskInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
    maskInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()),
    6);

  return 1;
}

//----------------------------------------------------------------------------
void vtkMaskPointsFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Empty Value: " << this->EmptyValue << "\n";
}
