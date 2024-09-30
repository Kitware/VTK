// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPolyDataNormals.h"

#include "vtkAlgorithmOutput.h"
#include "vtkAtomicMutex.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkEventForwarderCommand.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOrientPolyData.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkSplitSharpEdgesPolyData.h"
#include "vtkTriangleFilter.h"

//-----------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPolyDataNormals);

//-----------------------------------------------------------------------------
// Construct with feature angle=30, splitting and consistency turned on,
// flipNormals turned off, and non-manifold traversal turned on.
vtkPolyDataNormals::vtkPolyDataNormals()
{
  this->FeatureAngle = 30.0;
  this->Splitting = 1;
  this->Consistency = 1;
  this->FlipNormals = 0;
  this->ComputePointNormals = 1;
  this->ComputeCellNormals = 0;
  this->NonManifoldTraversal = 1;
  this->AutoOrientNormals = 0;
  this->OutputPointsPrecision = vtkAlgorithm::DEFAULT_PRECISION;
}

//-----------------------------------------------------------------------------
void vtkPolyDataNormals::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Feature Angle: " << this->GetFeatureAngle() << "\n";
  os << indent << "Splitting: " << (this->GetSplitting() ? "On\n" : "Off\n");
  os << indent << "Consistency: " << (this->GetConsistency() ? "On\n" : "Off\n");
  os << indent << "Flip Normals: " << (this->GetFlipNormals() ? "On\n" : "Off\n");
  os << indent << "Auto Orient Normals: " << (this->GetAutoOrientNormals() ? "On\n" : "Off\n");
  os << indent << "Compute Point Normals: " << (this->GetComputePointNormals() ? "On\n" : "Off\n");
  os << indent << "Compute Cell Normals: " << (this->GetComputeCellNormals() ? "On\n" : "Off\n");
  os << indent
     << "Non-manifold Traversal: " << (this->GetNonManifoldTraversal() ? "On\n" : "Off\n");
  os << indent << "Precision of the output points: " << this->GetOutputPointsPrecision() << "\n";
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkFloatArray> vtkPolyDataNormals::GetCellNormals(vtkPolyData* data)
{
  if (auto existingCellNormals = vtkFloatArray::FastDownCast(data->GetCellData()->GetNormals()))
  {
    return existingCellNormals;
  }
  vtkPoints* points = data->GetPoints();
  vtkCellArray* polys = data->GetPolys();
  const vtkIdType numVertices = data->GetNumberOfVerts();
  const vtkIdType numLines = data->GetNumberOfLines();
  const vtkIdType numPolys = data->GetNumberOfPolys();
  const vtkIdType numStrips = data->GetNumberOfStrips();
  // check if the cells are already built
  if (data->NeedToBuildCells())
  {
    data->BuildCells();
  }

  //  Initial pass to compute polygon normals without effects of neighbors
  auto cellNormals = vtkSmartPointer<vtkFloatArray>::New();
  cellNormals->SetName("Normals");
  cellNormals->SetNumberOfComponents(3);
  cellNormals->SetNumberOfTuples(numVertices + numLines + numPolys + numStrips);

  // Set default value for vertices and lines cell normals
  vtkIdType offsetCells = numVertices + numLines;
  vtkSMPTools::For(0, offsetCells,
    [&](vtkIdType begin, vtkIdType end)
    {
      static constexpr float n[3] = { 1.0, 0.0, 0.0 };
      for (vtkIdType cellId = begin; cellId < end; cellId++)
      {
        // add a default value for vertices and lines
        // normals do not have meaningful values, we set them to X
        cellNormals->SetTypedTuple(cellId, n);
      }
    });

  // Compute Cell Normals of polys
  vtkSMPThreadLocalObject<vtkIdList> tlTempCellPointIds;
  vtkSMPTools::For(0, numPolys,
    [&](vtkIdType begin, vtkIdType end)
    {
      auto tempCellPointIds = tlTempCellPointIds.Local();
      vtkIdType npts;
      const vtkIdType* pts = nullptr;
      double n[3];
      for (vtkIdType polyId = begin; polyId < end; polyId++)
      {
        polys->GetCellAtId(polyId, npts, pts, tempCellPointIds);
        vtkPolygon::ComputeNormal(points, npts, pts, n);
        cellNormals->SetTuple(offsetCells + polyId, n);
      }
    });

  // Set default value for strip cell normals
  offsetCells += numPolys;
  vtkSMPTools::For(0, numStrips,
    [&](vtkIdType begin, vtkIdType end)
    {
      static constexpr float n[3] = { 1.0, 0.0, 0.0 };
      for (vtkIdType cellId = begin; cellId < end; cellId++)
      {
        // add a default value for strips
        // normals do not have meaningful values, we set them to X
        cellNormals->SetTypedTuple(cellId, n);
      }
    });
  return cellNormals;
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkFloatArray> vtkPolyDataNormals::GetPointNormals(
  vtkPolyData* data, vtkFloatArray* cellNormals, double flipDirection)
{
  if (auto existingPointNormals = vtkFloatArray::FastDownCast(data->GetPointData()->GetNormals()))
  {
    return existingPointNormals;
  }
  const vtkIdType numPoints = data->GetNumberOfPoints();
  // check if the cells are already built
  if (data->NeedToBuildCells())
  {
    data->BuildCells();
  }
  data->BuildLinks();

  auto pointNormals = vtkSmartPointer<vtkFloatArray>::New();
  pointNormals->SetName("Normals");
  pointNormals->SetNumberOfComponents(3);
  pointNormals->SetNumberOfTuples(numPoints);
  float* pointNormalsPtr = pointNormals->GetPointer(0);
  float* cellNormalsPtr = cellNormals->GetPointer(0);

  vtkSMPTools::For(0, numPoints,
    [&](vtkIdType begin, vtkIdType end)
    {
      vtkIdType nCells;
      vtkIdType* cells = nullptr;
      for (vtkIdType pointId = begin; pointId < end; pointId++)
      {
        // Initialize point normals
        float* pointNormal = &pointNormalsPtr[3 * pointId];
        pointNormal[0] = pointNormal[1] = pointNormal[2] = 0.0;
        // Compute Point Normals
        data->GetPointCells(pointId, nCells, cells);
        for (vtkIdType i = 0; i < nCells; ++i)
        {
          vtkMath::Add(pointNormal, &cellNormalsPtr[3 * cells[i]], pointNormal);
        }
        // Normalize normals
        const double length = vtkMath::Norm(pointNormal) * flipDirection;
        if (length != 0.0)
        {
          vtkMath::MultiplyScalar(pointNormal, 1.0 / length);
        }
      }
    });

  return pointNormals;
}

//-----------------------------------------------------------------------------
// Generate normals for polygon meshes
int vtkPolyDataNormals::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the input and output
  vtkPolyData* input = vtkPolyData::GetData(inputVector[0]);
  vtkPolyData* output = vtkPolyData::GetData(outputVector);

  vtkDebugMacro(<< "Generating surface normals");

  const vtkIdType numInPoints = input->GetNumberOfPoints();
  const vtkIdType numInPolys = input->GetNumberOfPolys();
  const vtkIdType numInStrips = input->GetNumberOfStrips();
  if (numInPoints < 1)
  {
    vtkDebugMacro(<< "No data to generate normals for!");
    return 1;
  }

  // If there is nothing to do, pass the data through
  if ((this->ComputePointNormals == 0 && this->ComputeCellNormals == 0) ||
    (numInPolys < 1 && numInStrips < 1))
  {
    // don't do anything! pass data through
    output->CopyStructure(input);
    output->GetPointData()->PassData(input->GetPointData());
    output->GetCellData()->PassData(input->GetCellData());
    return 1;
  }
  // If the input has normals, and no orientation or splitting is asked pass the data through
  auto inputPointNormals = vtkFloatArray::FastDownCast(input->GetPointData()->GetNormals());
  const bool hasPointNormals = this->ComputePointNormals ? inputPointNormals != nullptr : true;
  auto inputCellNormals = vtkFloatArray::FastDownCast(input->GetCellData()->GetNormals());
  const bool hasCellNormals = this->ComputeCellNormals ? inputCellNormals != nullptr : true;
  if (hasPointNormals && hasCellNormals && (!this->Splitting || !this->ComputePointNormals) &&
    !this->Consistency && !this->AutoOrientNormals)
  {
    // don't do anything! pass data through
    output->CopyStructure(input);
    output->GetPointData()->PassData(input->GetPointData());
    output->GetCellData()->PassData(input->GetCellData());
    return 1;
  }

  // Forwards progress from the all internally used filters to this filter
  vtkNew<vtkEventForwarderCommand> progressForwarder;
  progressForwarder->SetTarget(this);

  vtkNew<vtkTriangleFilter> triangleFilter;
  triangleFilter->SetContainerAlgorithm(this);
  triangleFilter->AddObserver(vtkCommand::ProgressEvent, progressForwarder);
  triangleFilter->SetPassLines(true);
  triangleFilter->SetPassVerts(true);
  triangleFilter->SetPreservePolys(true);
  triangleFilter->SetInputData(input);
  vtkAlgorithmOutput* fixPolyDataPipeline = triangleFilter->GetOutputPort();
  vtkNew<vtkOrientPolyData> orientPolyData;
  if (this->Consistency || this->AutoOrientNormals)
  {
    orientPolyData->SetContainerAlgorithm(this);
    orientPolyData->AddObserver(vtkCommand::ProgressEvent, progressForwarder);
    orientPolyData->SetConsistency(this->Consistency);
    orientPolyData->SetFlipNormals(this->FlipNormals);
    orientPolyData->SetNonManifoldTraversal(this->NonManifoldTraversal);
    orientPolyData->SetAutoOrientNormals(this->AutoOrientNormals);
    orientPolyData->SetInputConnection(fixPolyDataPipeline);
    fixPolyDataPipeline = orientPolyData->GetOutputPort();
  }
  vtkNew<vtkSplitSharpEdgesPolyData> splitSharpEdgesPolyData;
  // splitting is only required if we are computing point normals
  if (this->Splitting && this->ComputePointNormals)
  {
    splitSharpEdgesPolyData->SetContainerAlgorithm(this);
    splitSharpEdgesPolyData->AddObserver(vtkCommand::ProgressEvent, progressForwarder);
    splitSharpEdgesPolyData->SetFeatureAngle(this->FeatureAngle);
    splitSharpEdgesPolyData->SetOutputPointsPrecision(this->OutputPointsPrecision);
    splitSharpEdgesPolyData->SetInputConnection(fixPolyDataPipeline);
    fixPolyDataPipeline = splitSharpEdgesPolyData->GetOutputPort();
  }
  auto fixPolyData = fixPolyDataPipeline->GetProducer();
  fixPolyData->Update();
  output->ShallowCopy(fixPolyData->GetOutputDataObject(0));

  vtkSmartPointer<vtkFloatArray> cellNormals = vtkPolyDataNormals::GetCellNormals(output);
  if (this->ComputeCellNormals)
  {
    output->GetCellData()->SetNormals(cellNormals);
  }
  this->UpdateProgress(0.5);
  if (this->CheckAbort())
  {
    return 1;
  }
  if (this->ComputePointNormals)
  {
    const double flipDirection = this->FlipNormals && !this->Consistency ? -1.0 : 1.0;
    vtkSmartPointer<vtkFloatArray> pointNormals =
      vtkPolyDataNormals::GetPointNormals(output, cellNormals, flipDirection);
    output->GetPointData()->SetNormals(pointNormals);
  }
  // if normals were not requested, they were not part of the input, but are part of the output.
  // remove them
  if (!this->ComputeCellNormals && !input->GetCellData()->GetNormals())
  {
    output->GetCellData()->SetNormals(nullptr);
  }
  if (!this->ComputePointNormals && !input->GetPointData()->GetNormals())
  {
    output->GetPointData()->SetNormals(nullptr);
  }
  // No longer need the links, so free them
  output->SetLinks(nullptr);
  this->UpdateProgress(1.0);

  return 1;
}
VTK_ABI_NAMESPACE_END
