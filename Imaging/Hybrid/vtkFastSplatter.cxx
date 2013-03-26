/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFastSplatter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkFastSplatter.h"

#include "vtkExtentTranslator.h"
#include "vtkGraph.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkOnePieceExtentTranslator.h"
#include "vtkPoints.h"
#include "vtkPointSet.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedIntArray.h"

#include <algorithm>

vtkStandardNewMacro(vtkFastSplatter);

//-----------------------------------------------------------------------------

vtkFastSplatter::vtkFastSplatter()
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);

  this->ModelBounds[0] = this->ModelBounds[2] = this->ModelBounds[4] = 0;
  this->ModelBounds[1] = this->ModelBounds[3] = this->ModelBounds[5] = -1;

  this->OutputDimensions[0] = 100;
  this->OutputDimensions[1] = 100;
  this->OutputDimensions[2] = 1;

  this->LimitMode = NoneLimit;
  this->MinValue = 0.0;
  this->MaxValue = 1.0;

  this->Buckets = vtkImageData::New();

  this->NumberOfPointsSplatted = 0;

  this->LastDataMinValue = 0.0;
  this->LastDataMaxValue = 1.0;
}

vtkFastSplatter::~vtkFastSplatter()
{
  this->Buckets->Delete();
}

void vtkFastSplatter::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ModelBounds: "
     << this->ModelBounds[0] << ", " << this->ModelBounds[1] << ", "
     << this->ModelBounds[2] << ", " << this->ModelBounds[3] << ", "
     << this->ModelBounds[4] << ", " << this->ModelBounds[5] << endl;
  os << indent << "OutputDimensions: " << this->OutputDimensions[0] << ", "
     << this->OutputDimensions[1] << ", " << this->OutputDimensions[2] << endl;
  os << indent << "LimitMode: " << this->LimitMode << endl;
  os << indent << "MinValue: " << this->MinValue << endl;
  os << indent << "MaxValue: " << this->MaxValue << endl;
  os << indent << "NumberOfPointsSplatted: " << this->NumberOfPointsSplatted << endl;
}

//-----------------------------------------------------------------------------

int vtkFastSplatter::FillInputPortInformation(int port,
                                              vtkInformation* info)
{
  switch(port)
    {
    case 0:
      info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
      info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
      info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
      break;
    case 1:
      info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
      break;
    }
  return 1;
}

//-----------------------------------------------------------------------------

// For those familiar with the old pipeline, this is equivalent to the
// ExecuteInformation method.
int vtkFastSplatter::RequestInformation(
                                 vtkInformation *vtkNotUsed(request),
                                 vtkInformationVector **inputVector,
                                 vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // use model bounds if set
  this->Origin[0] = 0;
  this->Origin[1] = 0;
  this->Origin[2] = 0;
  if (   (   (this->ModelBounds[0] < this->ModelBounds[1])
          || (this->OutputDimensions[0] == 1) )
      && (   (this->ModelBounds[2] < this->ModelBounds[3])
          || (this->OutputDimensions[1] == 1) )
      && (   (this->ModelBounds[4] < this->ModelBounds[5])
          || (this->OutputDimensions[2] == 1) ) )
    {
    this->Origin[0] = this->ModelBounds[0];
    this->Origin[1] = this->ModelBounds[2];
    this->Origin[2] = this->ModelBounds[4];
    }

  outInfo->Set(vtkDataObject::ORIGIN(), this->Origin, 3);

  int i;
  for (i=0; i<3; i++)
    {
    if (this->OutputDimensions[i] > 1)
      {
      this->Spacing[i] = (  (this->ModelBounds[2*i+1] - this->ModelBounds[2*i])
                          / (this->OutputDimensions[i] - 1) );
      }
    else
      {
      this->Spacing[i] = 1.0;
      }
    if ( this->Spacing[i] <= 0.0 )
      {
      this->Spacing[i] = 1.0;
      }
    }
  outInfo->Set(vtkDataObject::SPACING(),this->Spacing,3);

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               0, this->OutputDimensions[0] - 1,
               0, this->OutputDimensions[1] - 1,
               0, this->OutputDimensions[2] - 1);
  vtkInformation *splatInfo = inputVector[1]->GetInformationObject(0);
  vtkImageData::SetScalarType(
    vtkImageData::GetScalarType(splatInfo),
    outInfo);
  // if (splatInfo->Has(vtkDataObject::SCALAR_NUMBER_OF_COMPONENTS()))
  //   {
  //   outInfo->Set(vtkDataObject::SCALAR_NUMBER_OF_COMPONENTS(),
  //                splatInfo->Get(vtkDataObject::SCALAR_NUMBER_OF_COMPONENTS()));
  //   }

  // Setup ExtentTranslator so that all downstream piece requests are
  // converted to whole extent update requests, as need by this filter.
  if (strcmp(
      vtkStreamingDemandDrivenPipeline::GetExtentTranslator(outInfo)
        ->GetClassName(), "vtkOnePieceExtentTranslator") != 0)
    {
    vtkExtentTranslator* et = vtkOnePieceExtentTranslator::New();
    vtkStreamingDemandDrivenPipeline::SetExtentTranslator(outInfo, et);
    et->Delete();
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkFastSplatter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
                                         vtkInformationVector** inputVector,
                                         vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* splatInfo = inputVector[1]->GetInformationObject(0);

  splatInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
                 splatInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()),
                 6);

  int numPieces = 1;
  int piece = 0;
  int ghostLevel = 0;
  // Use the output piece request to break up the input.
  // If not specified, use defaults.
  if (outInfo->Has(
        vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()))
    {
    numPieces = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
    }
  if (outInfo->Has(
        vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()))
    {
    piece = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    }
  if (outInfo->Has(
        vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()))
    {
    ghostLevel = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
    }
  vtkDataObject* data = inInfo->Get(vtkDataObject::DATA_OBJECT());
  // If input extent is piece based, just pass the update requests
  // from the output. Even though the output extent is structured,
  // piece-based request still gets propagated. This will not work
  // if there was no piece based request to start with. That is handled
  // above.
  if(data->GetExtentType() == VTK_PIECES_EXTENT)
    {
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
                numPieces);
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
                piece);
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
                ghostLevel);
    }
  else if(data->GetExtentType() == VTK_3D_EXTENT)
    {
    int* inWholeExtent =
      inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());

    vtkExtentTranslator* translator =
      vtkExtentTranslator::SafeDownCast(
        inInfo->Get(vtkStreamingDemandDrivenPipeline::EXTENT_TRANSLATOR()));
    if(translator)
      {
      translator->SetWholeExtent(inWholeExtent);
      translator->SetPiece(piece);
      translator->SetNumberOfPieces(numPieces);
      translator->SetGhostLevel(ghostLevel);
      translator->PieceToExtent();
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
                  translator->GetExtent(),
                  6);
      }
    else
      {
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
                  inWholeExtent,
                  6);
      }
    }


  return 1;
}

//-----------------------------------------------------------------------------

template<class T>
void vtkFastSplatterBucketPoints(const T *points, vtkIdType numPoints,
                                 unsigned int *buckets,
                                 const int dimensions[3],
                                 const double origin[3],
                                 const double spacing[3])
{
  // Clear out the buckets.
  std::fill_n(buckets, dimensions[0]*dimensions[1]*dimensions[2], 0);

  // Iterate over all the points.
  for (vtkIdType i = 0; i < numPoints; i++)
    {
    const T *p = points + 3*i;

    // Find the bucket.
    vtkIdType loc[3];
    loc[0] = static_cast<vtkIdType>(((p[0]-origin[0])/spacing[0]) + 0.5);
    loc[1] = static_cast<vtkIdType>(((p[1]-origin[1])/spacing[1]) + 0.5);
    loc[2] = static_cast<vtkIdType>(((p[2]-origin[2])/spacing[2]) + 0.5);
    if (   (loc[0] < 0) || (loc[0] >= dimensions[0])
        || (loc[1] < 0) || (loc[1] >= dimensions[1])
        || (loc[2] < 0) || (loc[2] >= dimensions[2]) )
      {
      // Point outside of splatting region.
      continue;
      }
    vtkIdType bucketId = (  loc[2]*dimensions[0]*dimensions[1]
                          + loc[1]*dimensions[0]
                          + loc[0] );

    // Increment the bucket.
    buckets[bucketId]++;
    }
}

//-----------------------------------------------------------------------------

template<class T>
void vtkFastSplatterConvolve(T *splat, const int splatDims[3],
                             unsigned int *buckets, T *output,
                             int *numPointsSplatted,
                             const int imageDims[3])
{
  int numPoints = 0;

  // First, clear out the output image.
  std::fill_n(output, imageDims[0]*imageDims[1]*imageDims[2],
                 static_cast<T>(0));

  int splatCenter[3];
  splatCenter[0] = splatDims[0]/2;
  splatCenter[1] = splatDims[1]/2;
  splatCenter[2] = splatDims[2]/2;

  // Iterate over all entries in buckets and splat anything that is nonzero.
  unsigned int *b = buckets;
  for (int k = 0; k < imageDims[2]; k++)
    {
    // Figure out how splat projects on image in this slab, taking into
    // account overlap.
    int splatProjMinZ = k - splatCenter[2];
    int splatProjMaxZ = splatProjMinZ + splatDims[2];
    if (splatProjMinZ < 0) splatProjMinZ = 0;
    if (splatProjMaxZ > imageDims[2]) splatProjMaxZ = imageDims[2];

    for (int j = 0; j < imageDims[1]; j++)
      {
      // Figure out how splat projects on image in this slab, taking into
      // account overlap.
      int splatProjMinY = j - splatCenter[1];
      int splatProjMaxY = splatProjMinY + splatDims[1];
      if (splatProjMinY < 0) splatProjMinY = 0;
      if (splatProjMaxY > imageDims[1]) splatProjMaxY = imageDims[1];

      for (int i = 0; i < imageDims[0]; i++)
        {
        // No need to splat 0.
        if (*b == 0)
          {
          b++;
          continue;
          }

        T value = static_cast<T>(*b);
        numPoints += static_cast<int>(*b);
        b++;

        // Figure out how splat projects on image in this pixel, taking into
        // account overlap.
        int splatProjMinX = i - splatCenter[0];
        int splatProjMaxX = splatProjMinX + splatDims[0];
        if (splatProjMinX < 0) splatProjMinX = 0;
        if (splatProjMaxX > imageDims[0]) splatProjMaxX = imageDims[0];

        // Do the splat.
        for (int imageZ = splatProjMinZ; imageZ < splatProjMaxZ; imageZ++)
          {
          int imageZOffset = imageZ*imageDims[0]*imageDims[1];
          int splatZ = imageZ - k + splatCenter[2];
          int splatZOffset = splatZ*splatDims[0]*splatDims[1];
          for (int imageY = splatProjMinY; imageY < splatProjMaxY; imageY++)
            {
            int imageYOffset = imageZOffset + imageY*imageDims[0];
            int splatY = imageY - j + splatCenter[1];
            int splatYOffset = splatZOffset + splatY*splatDims[0];
            for (int imageX = splatProjMinX; imageX < splatProjMaxX; imageX++)
              {
              int imageOffset = imageYOffset + imageX;
              int splatX = imageX - i + splatCenter[0];
              int splatOffset = splatYOffset + splatX;
              output[imageOffset] += value * splat[splatOffset];
              }
            }
          }
        }
      }
    }
  *numPointsSplatted = numPoints;
}


//-----------------------------------------------------------------------------

// For those of you familiar with the old pipeline, this is equivalent to the
// Execute method.
int vtkFastSplatter::RequestData(vtkInformation *vtkNotUsed(request),
                                 vtkInformationVector **inputVector,
                                 vtkInformationVector *outputVector)
{
  this->NumberOfPointsSplatted = 0;

  // Get the input and output objects.
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkPoints* points = 0;
  if(vtkPointSet* const input =
    vtkPointSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT())))
    {
    points = input->GetPoints();
    }
  else if(vtkGraph* const graph =
    vtkGraph::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT())))
    {
    points = graph->GetPoints();
    }

  vtkInformation *splatInfo = inputVector[1]->GetInformationObject(0);
  vtkImageData *splatImage
    = vtkImageData::SafeDownCast(splatInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkImageData *output
    = vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Figure out the real bounds to use.
  double *bounds;
  if (   (   (this->ModelBounds[0] < this->ModelBounds[1])
          || (this->OutputDimensions[0] == 1) )
      && (   (this->ModelBounds[2] < this->ModelBounds[3])
          || (this->OutputDimensions[1] == 1) )
      && (   (this->ModelBounds[4] < this->ModelBounds[5])
          || (this->OutputDimensions[2] == 1) ) )
    {
    bounds = this->ModelBounds;
    }
  else
    {
    bounds = points->GetBounds();
    }

  // Compute origin and spacing from bounds
  for (int i=0; i<3; i++)
    {
    this->Origin[i] = bounds[2*i];
    if (this->OutputDimensions[i] > 1)
      {
      this->Spacing[i] = (  (bounds[2*i+1] - bounds[2*i])
                          / (this->OutputDimensions[i] - 1) );
      }
    else
      {
      this->Spacing[i] = 2.0 * (bounds[2*i+1] - bounds[2*i]);
      }
    if ( this->Spacing[i] <= 0.0 )
      {
      this->Spacing[i] = 1.0;
      }
    }

  // Set up output.
  output->SetDimensions(this->OutputDimensions);
  outInfo->Set(vtkDataObject::ORIGIN(), this->Origin, 3);
  output->SetOrigin(this->Origin);
  outInfo->Set(vtkDataObject::SPACING(), this->Spacing, 3);
  output->SetSpacing(this->Spacing);
  output->SetExtent(0, this->OutputDimensions[0] - 1,
                    0, this->OutputDimensions[1] - 1,
                    0, this->OutputDimensions[2] - 1);
  output->AllocateScalars(splatImage->GetScalarType(),
                          splatImage->GetNumberOfScalarComponents());

  // Set up intermediate buckets image.
  this->Buckets->SetDimensions(this->OutputDimensions);
  this->Buckets->SetOrigin(this->Origin);
  this->Buckets->SetSpacing(this->Spacing);
  this->Buckets->SetExtent(0, this->OutputDimensions[0] - 1,
                           0, this->OutputDimensions[1] - 1,
                           0, this->OutputDimensions[2] - 1);
  this->Buckets->AllocateScalars(VTK_UNSIGNED_INT, 1);

  // Get array for buckets.
  unsigned int *buckets =
    static_cast<unsigned int *>(this->Buckets->GetScalarPointer());

  // Count how many points in the input lie in each pixel of the output image.
  void *p = points->GetVoidPointer(0);
  switch (points->GetDataType())
    {
    vtkTemplateMacro(vtkFastSplatterBucketPoints(static_cast<VTK_TT *>(p),
                                                 points->GetNumberOfPoints(),
                                                 buckets,
                                                 this->OutputDimensions,
                                                 this->Origin, this->Spacing));
    }

  // Now convolve the splat image with the bucket image.
  void *splat = splatImage->GetScalarPointer();
  void *o = output->GetScalarPointer();
  switch (output->GetScalarType())
    {
    vtkTemplateMacro(vtkFastSplatterConvolve(static_cast<VTK_TT *>(splat),
                                             splatImage->GetDimensions(),
                                             buckets,
                                             static_cast<VTK_TT *>(o),
                                             &(this->NumberOfPointsSplatted),
                                             this->OutputDimensions));
    }

  // Do any appropriate limiting.
  switch (this->LimitMode)
    {
    case NoneLimit:
      break;
    case ClampLimit:
      switch (output->GetScalarType())
        {
        vtkTemplateMacro(vtkFastSplatterClamp(
                           static_cast<VTK_TT *>(o),
                           output->GetNumberOfPoints()*
                           output->GetNumberOfScalarComponents(),
                           static_cast<VTK_TT>(this->MinValue),
                           static_cast<VTK_TT>(this->MaxValue)));
        }
      break;
    case FreezeScaleLimit:
      switch (output->GetScalarType())
        {
        vtkTemplateMacro(vtkFastSplatterFrozenScale(
                           static_cast<VTK_TT *>(o),
                           output->GetNumberOfScalarComponents(),
                           output->GetNumberOfPoints(),
                           static_cast<VTK_TT>(this->MinValue),
                           static_cast<VTK_TT>(this->MaxValue),
                           this->LastDataMinValue,
                           this->LastDataMaxValue));
        }
      break;

    case ScaleLimit:
      switch (output->GetScalarType())
        {
        vtkTemplateMacro(vtkFastSplatterScale(
                           static_cast<VTK_TT *>(o),
                           output->GetNumberOfScalarComponents(),
                           output->GetNumberOfPoints(),
                           static_cast<VTK_TT>(this->MinValue),
                           static_cast<VTK_TT>(this->MaxValue),
                           & this->LastDataMinValue,
                           & this->LastDataMaxValue));
        }
      break;
    }

  return 1;
}

void vtkFastSplatter::SetSplatConnection(vtkAlgorithmOutput* input)
{
  this->SetInputConnection(1, input);
}
