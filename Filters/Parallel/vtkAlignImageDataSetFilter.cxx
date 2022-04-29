/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAlignImageDataSetFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAlignImageDataSetFilter.h"

#include "vtkBoundingBox.h"
#include "vtkCompositeDataSet.h"
#include "vtkDummyController.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

namespace
{

vtkVector3d AllGather(const vtkVector3d& spacing, vtkMultiProcessController* controller)
{
  vtkVector3d in(spacing);
  vtkVector3d out = in;
  controller->AllReduce(in.GetData(), out.GetData(), 3, vtkCommunicator::MAX_OP);
  for (int cc = 0; cc < 3; ++cc)
  {
    in[cc] = (in[cc] == 0 || in[cc] == out[cc]) ? out[cc] : VTK_DOUBLE_MAX;
  }

  controller->AllReduce(in.GetData(), out.GetData(), 3, vtkCommunicator::MAX_OP);
  return out;
}

bool IsSpacingValid(const vtkVector3d& spacing)
{
  return spacing[0] < VTK_DOUBLE_MAX && spacing[1] < VTK_DOUBLE_MAX && spacing[2] < VTK_DOUBLE_MAX;
}

bool ComputeGlobalOrigin(vtkVector3d& origin, const std::vector<vtkImageData*>& images,
  vtkMultiProcessController* controller)
{
  // First, confirm that spacing is compatible. All images must have the same
  // spacing otherwise we cannot pick a valid global origin/extent.
  vtkVector3d spacing =
    images.empty() ? vtkVector3d(0.0) : vtkVector3d(images.front()->GetSpacing());
  for (auto* image : images)
  {
    if (image->GetNumberOfPoints() > 0)
    {
      const vtkVector3d imgSpacing(image->GetSpacing());
      if (spacing != imgSpacing)
      {
        spacing = vtkVector3d(VTK_DOUBLE_MAX);
      }
    }
  }

  vtkVector3d globalSpacing = AllGather(spacing, controller);
  if (!IsSpacingValid(globalSpacing))
  {
    vtkLogF(ERROR, "Cannot determine acceptable global spacing.");
    return false;
  }

  // Compute global bounds to determine the global image origin.
  vtkBoundingBox bbox;
  for (auto* image : images)
  {
    bbox.AddBounds(image->GetBounds());
  }

  vtkBoundingBox globalBounds;
  controller->AllReduce(bbox, globalBounds);
  globalBounds.GetMinPoint(origin.GetData());
  return true;
}

}

vtkStandardNewMacro(vtkAlignImageDataSetFilter);
vtkCxxSetObjectMacro(vtkAlignImageDataSetFilter, Controller, vtkMultiProcessController);
//-----------------------------------------------------------------------------
vtkAlignImageDataSetFilter::vtkAlignImageDataSetFilter()
  : Controller(nullptr)
  , MinimumExtent{ 0, 0, 0 }
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//-----------------------------------------------------------------------------
vtkAlignImageDataSetFilter::~vtkAlignImageDataSetFilter()
{
  this->SetController(nullptr);
}

//-----------------------------------------------------------------------------
int vtkAlignImageDataSetFilter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSetCollection");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkAlignImageDataSetFilter::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto outputCD = vtkCompositeDataSet::GetData(outputVector, 0);
  outputCD->RecursiveShallowCopy(vtkDataObject::GetData(inputVector[0], 0));
  auto images = vtkCompositeDataSet::GetDataSets<vtkImageData>(outputCD);

  auto controller = this->GetController()
    ? vtk::MakeSmartPointer(this->GetController())
    : vtk::TakeSmartPointer<vtkMultiProcessController>(vtkDummyController::New());

  int countLocal = static_cast<int>(images.size());
  int countGlobal = countLocal;
  controller->AllReduce(&countLocal, &countGlobal, 1, vtkCommunicator::SUM_OP);
  if (countGlobal == 0)
  {
    // no images present. nothing to do.
    return 1;
  }

  vtkVector3d origin(VTK_DOUBLE_MAX);
  if (!::ComputeGlobalOrigin(origin, images, controller))
  {
    vtkErrorMacro("Failed to compute global origin.");
    return 0;
  }

  int illalignedOrigins = 0;
  // adjust image extents
  for (auto* image : images)
  {
    if (image->GetNumberOfPoints() == 0)
    {
      continue;
    }

    int dims[3];
    image->GetDimensions(dims);

    const vtkVector3d imgOrigin(image->GetOrigin());
    const vtkVector3d imgSpacing(image->GetSpacing());

    vtkVector3i startExt(0);
    for (int cc = 0; cc < 3; ++cc)
    {
      startExt[cc] = static_cast<int>((imgOrigin[cc] - origin[cc]) / imgSpacing[cc]);
    }

    int extents[6];
    extents[0] = startExt[0] + this->MinimumExtent[0];
    extents[1] = extents[0] + dims[0] - 1;

    extents[2] = startExt[1] + this->MinimumExtent[1];
    extents[3] = extents[2] + dims[1] - 1;

    extents[4] = startExt[2] + this->MinimumExtent[2];
    extents[5] = extents[4] + dims[2] - 1;

    vtkVector3d lorigin(origin);
    lorigin -= vtkVector3d(this->MinimumExtent[0], this->MinimumExtent[1], this->MinimumExtent[2]) *
      imgSpacing;

    const vtkVector3d pt0(image->GetPoint(0));
    image->SetOrigin(lorigin.GetData());
    image->SetExtent(extents);
    const vtkVector3d newPt0(image->GetPoint(0));
    if ((newPt0 - pt0).Norm() > 1e-10)
    {
      vtkLogF(ERROR,
        "Global spacing (%f, %f, %f)/origin (%f, %f, %f) incompatible for image with first point "
        "at (%f, %f, %f)",
        imgSpacing[0], imgSpacing[1], imgSpacing[2], lorigin[0], lorigin[1], lorigin[2], pt0[0],
        pt0[1], pt0[2]);
      illalignedOrigins = 1;
    }
  }

  int illalignedOriginsGlobal = illalignedOrigins;
  controller->AllReduce(&illalignedOrigins, &illalignedOriginsGlobal, 1, vtkCommunicator::MAX_OP);
  return (illalignedOriginsGlobal == 1) ? 0 : 1;
}

//-----------------------------------------------------------------------------
void vtkAlignImageDataSetFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
  os << indent << "MinimumExtent: " << this->MinimumExtent[0] << ", " << this->MinimumExtent[1]
     << ", " << this->MinimumExtent[2] << endl;
}
