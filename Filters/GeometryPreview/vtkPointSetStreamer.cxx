// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPointSetStreamer.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStaticPointLocator.h"

#include <numeric>

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkPointSetStreamer);

//------------------------------------------------------------------------------
vtkPointSetStreamer::vtkPointSetStreamer() = default;

//------------------------------------------------------------------------------
vtkPointSetStreamer::~vtkPointSetStreamer() = default;

//------------------------------------------------------------------------------
void vtkPointSetStreamer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfPointsPerBucket: " << this->NumberOfPointsPerBucket << endl;
  os << indent << "BucketId: " << this->BucketId << endl;
  os << indent << "NumberOfBuckets: " << this->NumberOfBuckets << endl;
  os << indent << "CreateVerticesCellArray: " << this->CreateVerticesCellArray << endl;
  os << indent << "PointLocator: " << this->PointLocator << endl;
}

//------------------------------------------------------------------------------
int vtkPointSetStreamer::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkPointSetStreamer::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the input
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkPointSet* input = vtkPointSet::GetData(inInfo);

  // get the output
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkPolyData* output = vtkPolyData::GetData(outInfo);

  if (!input || input->GetNumberOfCells() == 0)
  {
    vtkErrorMacro("No input or empty input.");
    return 0;
  }

  // build the locator if needed
  this->PointLocator->SetNumberOfPointsPerBucket(this->NumberOfPointsPerBucket);
  this->PointLocator->SetDataSet(input);
  this->PointLocator->BuildLocator();

  // get the number of buckets
  this->NumberOfBuckets = this->PointLocator->GetNumberOfBuckets();

  // get the points in the bucket
  vtkNew<vtkIdList> pointIds;
  this->PointLocator->GetBucketIds(this->BucketId, pointIds);

  vtkIdType numberOfPointsInBucket = pointIds->GetNumberOfIds();
  if (numberOfPointsInBucket == 0)
  {
    return 1;
  }

  // copy the points and point data
  vtkNew<vtkPoints> points;
  points->SetDataType(input->GetPoints()->GetDataType());
  points->SetNumberOfPoints(numberOfPointsInBucket);

  vtkPointData* inputPD = input->GetPointData();
  vtkPointData* outputPD = output->GetPointData();
  outputPD->CopyAllocate(inputPD, numberOfPointsInBucket);
  outputPD->SetNumberOfTuples(numberOfPointsInBucket);

  double point[3];
  vtkIdType id;
  for (vtkIdType i = 0; i < numberOfPointsInBucket; ++i)
  {
    id = pointIds->GetId(i);
    input->GetPoint(id, point);
    points->SetPoint(i, point);
    outputPD->CopyData(inputPD, id, i);
  }
  output->SetPoints(points);

  // create the original point ids array
  vtkNew<vtkIdTypeArray> originalIds;
  originalIds->SetName("vtkOriginalPointIds");
  originalIds->SetNumberOfValues(numberOfPointsInBucket);
  std::copy(pointIds->GetPointer(0), pointIds->GetPointer(numberOfPointsInBucket),
    originalIds->GetPointer(0));
  outputPD->AddArray(originalIds);

  // create output vertices cell array
  if (this->CreateVerticesCellArray)
  {
    vtkNew<vtkIdTypeArray> connectivity;
    connectivity->SetNumberOfValues(numberOfPointsInBucket);
    std::iota(connectivity->GetPointer(0), connectivity->GetPointer(numberOfPointsInBucket), 0);

    vtkNew<vtkIdTypeArray> offsets;
    offsets->SetNumberOfValues(numberOfPointsInBucket + 1);
    std::iota(offsets->GetPointer(0), offsets->GetPointer(numberOfPointsInBucket + 1), 0);

    vtkNew<vtkCellArray> vertices;
    vertices->SetData(offsets, connectivity);
    output->SetVerts(vertices);
  }
  return 1;
}
VTK_ABI_NAMESPACE_END
