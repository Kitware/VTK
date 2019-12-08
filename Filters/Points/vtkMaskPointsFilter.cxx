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

#include "vtkArrayDispatch.h"
#include "vtkDataArrayRange.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkSMPTools.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkMaskPointsFilter);

//----------------------------------------------------------------------------
// Helper classes to support efficient computing, and threaded execution.
namespace
{

//----------------------------------------------------------------------------
// The threaded core of the algorithm
struct ExtractPoints
{
  template <typename PtArrayT>
  void operator()(PtArrayT* ptArray, const unsigned char* mask, unsigned char emptyValue,
    const int dims[3], const double origin[3], const double spacing[3], vtkIdType* pointMap) const
  {
    const vtkIdType numPts = ptArray->GetNumberOfTuples();

    const double fX = 1.0 / spacing[0];
    const double fY = 1.0 / spacing[1];
    const double fZ = 1.0 / spacing[2];

    const double bX = origin[0] - 0.5 * spacing[0];
    const double bY = origin[1] - 0.5 * spacing[1];
    const double bZ = origin[2] - 0.5 * spacing[2];

    const vtkIdType xD = dims[0];
    const vtkIdType yD = dims[1];
    const vtkIdType zD = dims[2];
    const vtkIdType xyD = dims[0] * dims[1];

    vtkSMPTools::For(0, numPts, [&](vtkIdType ptId, vtkIdType endPtId) {
      const auto pts = vtk::DataArrayTupleRange<3>(ptArray, ptId, endPtId);
      using PtCRefT = typename decltype(pts)::ConstTupleReferenceType;

      vtkIdType* map = pointMap + ptId;

      // MSVC 2015 x64 ICEs when this loop is written using std::transform,
      // so we'll work around that by using a for-range loop:
      for (PtCRefT pt : pts)
      {
        const int i = static_cast<int>(((pt[0] - bX) * fX));
        const int j = static_cast<int>(((pt[1] - bY) * fY));
        const int k = static_cast<int>(((pt[2] - bZ) * fZ));

        // If not inside image then skip
        if (i < 0 || i >= xD || j < 0 || j >= yD || k < 0 || k >= zD)
        {
          *map++ = -1;
        }
        else if (mask[i + j * xD + k * xyD] != emptyValue)
        {
          *map++ = 1;
        }
        else
        {
          *map++ = -1;
        }
      }
    });
  }
}; // ExtractPoints

} // anonymous namespace

//================= Begin class proper =======================================
//----------------------------------------------------------------------------
vtkMaskPointsFilter::vtkMaskPointsFilter()
{
  this->SetNumberOfInputPorts(2);

  this->EmptyValue = 0;
  this->Mask = nullptr;
}

//----------------------------------------------------------------------------
vtkMaskPointsFilter::~vtkMaskPointsFilter() = default;

//----------------------------------------------------------------------------
int vtkMaskPointsFilter::FillInputPortInformation(int port, vtkInformation* info)
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
void vtkMaskPointsFilter::SetMaskData(vtkDataObject* input)
{
  this->SetInputData(1, input);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkMaskPointsFilter::GetMask()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }

  return this->GetExecutive()->GetInputData(1, 0);
}

//----------------------------------------------------------------------------
// Traverse all the input points and extract points that are contained within
// the mask.
int vtkMaskPointsFilter::FilterPoints(vtkPointSet* input)
{
  // Grab the image data and metadata. The type and existence of image data
  // should have been checked in RequestData().
  double origin[3], spacing[3];
  int dims[3];
  this->Mask->GetDimensions(dims);
  this->Mask->GetOrigin(origin);
  this->Mask->GetSpacing(spacing);
  unsigned char ev = this->EmptyValue;

  unsigned char* m = static_cast<unsigned char*>(this->Mask->GetScalarPointer());

  // Determine which points, if any, should be removed. We create a map
  // to keep track. The bulk of the algorithmic work is done in this pass.
  vtkDataArray* ptArray = input->GetPoints()->GetData();

  // Use a fast path for double/float points:
  using vtkArrayDispatch::Reals;
  using Dispatcher = vtkArrayDispatch::DispatchByValueType<Reals>;
  ExtractPoints worker;
  if (!Dispatcher::Execute(ptArray, worker, m, ev, dims, origin, spacing, this->PointMap))
  { // fallback for weird types
    worker(ptArray, m, ev, dims, origin, spacing, this->PointMap);
  }

  return 1;
}

//----------------------------------------------------------------------------
// Due to the second input, retrieve it and then invoke the superclass
// RequestData.
int vtkMaskPointsFilter::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* maskInfo = inputVector[1]->GetInformationObject(0);

  // get the mask
  this->Mask = vtkImageData::SafeDownCast(maskInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!this->Mask)
  {
    vtkWarningMacro(<< "No image mask available");
    return 1;
  }

  if (this->Mask->GetScalarType() != VTK_UNSIGNED_CHAR)
  {
    vtkWarningMacro(<< "Image mask must be unsigned char type");
    return 1;
  }

  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkMaskPointsFilter::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* maskInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  outInfo->CopyEntry(maskInfo, vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  outInfo->CopyEntry(maskInfo, vtkStreamingDemandDrivenPipeline::TIME_RANGE());

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
    inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);

  // Make sure that the scalar type and number of components
  // are propagated from the mask not the input.
  if (vtkImageData::HasScalarType(maskInfo))
  {
    vtkImageData::SetScalarType(vtkImageData::GetScalarType(maskInfo), outInfo);
  }
  if (vtkImageData::HasNumberOfScalarComponents(maskInfo))
  {
    vtkImageData::SetNumberOfScalarComponents(
      vtkImageData::GetNumberOfScalarComponents(maskInfo), outInfo);
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkMaskPointsFilter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* maskInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
  maskInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  maskInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  maskInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
  maskInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
    maskInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);

  return 1;
}

//----------------------------------------------------------------------------
void vtkMaskPointsFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Empty Value: " << this->EmptyValue << "\n";
}
