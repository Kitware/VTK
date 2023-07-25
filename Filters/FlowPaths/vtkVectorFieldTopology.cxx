// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include <vtkVectorFieldTopology.h>

// VTK includes
#include <vtkAppendPolyData.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkCellDataToPointData.h>
#include <vtkCellLocator.h>
#include <vtkCellTypes.h>
#include <vtkContourFilter.h>
#include <vtkDataSet.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkDataSetTriangleFilter.h>
#include <vtkDoubleArray.h>
#include <vtkFeatureEdges.h>
#include <vtkGeometryFilter.h>
#include <vtkIdFilter.h>
#include <vtkImageData.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkIntersectionPolyDataFilter.h>
#include <vtkLine.h>
#include <vtkMath.h>
#include <vtkMatrix3x3.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPointLocator.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyLine.h>
#include <vtkProbeFilter.h>
#include <vtkQuad.h>
#include <vtkRegularPolygonSource.h>
#include <vtkRuledSurfaceFilter.h>
#include <vtkSelectEnclosedPoints.h>
#include <vtkSmartPointer.h>
#include <vtkStreamSurface.h>
#include <vtkStreamTracer.h>
#include <vtkTetra.h>
#include <vtkTriangle.h>
#include <vtkUnstructuredGrid.h>
#include <vtkVector.h>
#include <vtkVertex.h>

// Eigen3
#include <vtk_eigen.h>
#include VTK_EIGEN(Eigenvalues)

#include <cmath>
#include <map>

#define epsilon (1e-10)

//----------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkVectorFieldTopology);

//----------------------------------------------------------------------------
vtkVectorFieldTopology::vtkVectorFieldTopology()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(5);

  // by default process active point vectors
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::VECTORS);

  this->StreamSurface->SetContainerAlgorithm(this);
}

//----------------------------------------------------------------------------
vtkVectorFieldTopology::~vtkVectorFieldTopology() = default;

//----------------------------------------------------------------------------
void vtkVectorFieldTopology::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MaxNumSteps =  " << this->MaxNumSteps << "\n";
  os << indent << "IntegrationStepSize =  " << this->IntegrationStepSize << "\n";
  os << indent << "SeparatrixDistance =  " << this->SeparatrixDistance << "\n";
  os << indent << "UseIterativeSeeding =  " << this->UseIterativeSeeding << "\n";
  os << indent << "InterpolatorType = " << this->InterpolatorType << "\n";
  os << indent << "ComputeSurfaces =  " << this->ComputeSurfaces << "\n";
  os << indent << "EpsilonCriticalPoint = " << this->EpsilonCriticalPoint << "\n";
  os << indent << "vtkStreamSurface: \n";
  this->StreamSurface->PrintSelf(os, indent.GetNextIndent());
}

//----------------------------------------------------------------------------
int vtkVectorFieldTopology::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkVectorFieldTopology::FillOutputPortInformation(int port, vtkInformation* info)
{
  if (port < 5)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkVectorFieldTopology::Validate()
{
  if (UseBoundarySwitchPoints && ExcludeBoundary)
  {
    vtkErrorMacro(
      "vtkVectorFieldTopology: both UseBoundarySwitchPoints and ExcludeBoundary are true.");
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkVectorFieldTopology::SetInterpolatorType(int interpType)
{
  this->InterpolatorType = interpType;
  if (interpType != vtkStreamTracer::INTERPOLATOR_WITH_DATASET_POINT_LOCATOR &&
    interpType != vtkStreamTracer::INTERPOLATOR_WITH_CELL_LOCATOR)
    vtkErrorMacro(
      "The interpolator type is neither vtkStreamTracer::INTERPOLATOR_WITH_CELL_LOCATOR nor "
      "vtkStreamTracer::INTERPOLATOR_WITH_DATASET_POINT_LOCATOR.");
}

//----------------------------------------------------------------------------
void vtkVectorFieldTopology::SetInterpolatorTypeToCellLocator()
{
  SetInterpolatorType(static_cast<int>(vtkStreamTracer::INTERPOLATOR_WITH_CELL_LOCATOR));
}

//----------------------------------------------------------------------------
void vtkVectorFieldTopology::SetInterpolatorTypeToDataSetPointLocator()
{
  SetInterpolatorType(static_cast<int>(vtkStreamTracer::INTERPOLATOR_WITH_DATASET_POINT_LOCATOR));
}

//----------------------------------------------------------------------------
int vtkVectorFieldTopology::Classify2D(int countComplex, int countPos, int countNeg)
{
  // make simple type that corresponds to the number of positive eigenvalues
  // SOURCE_2D 2, SADDLE_2D 1, SINK_2D 0, (CENTER_2D 3)
  // in analogy to ttk, where the type corresponds to the down directions
  int critType = vtkVectorFieldTopology::DEGENERATE_2D;
  if (countPos + countNeg == 2)
  {
    switch (countPos)
    {
      case 0:
        critType = vtkVectorFieldTopology::SINK_2D;
        break;
      case 1:
        critType = vtkVectorFieldTopology::SADDLE_2D;
        break;
      case 2:
        critType = vtkVectorFieldTopology::SOURCE_2D;
        break;
      default:
        break;
    }
  }
  else if (countComplex == 2)
  {
    critType = vtkVectorFieldTopology::CENTER_2D;
  }
  return critType;
}

//----------------------------------------------------------------------------
int vtkVectorFieldTopology::ClassifyDetailed2D(int countComplex, int countPos, int countNeg)
{
  // determine which type of critical point we have including distinction between node and spiral
  // ATTRACTING_NODE_2D 0, ATTRACTING_FOCUS_2D 1, NODE_SADDLE_2D 2, REPELLING_NODE_2D 3,
  // REPELLING_FOCUS_2D 4, CENTER_DETAILED_2D 5
  int critType = vtkVectorFieldTopology::DEGENERATE_2D;
  if (countPos + countNeg == 2)
  {
    switch (countPos)
    {
      case 0:
        if (countComplex == 0)
        {
          critType = vtkVectorFieldTopology::ATTRACTING_NODE_2D;
        }
        else
        {
          critType = vtkVectorFieldTopology::ATTRACTING_FOCUS_2D;
        }
        break;
      case 1:
        critType = vtkVectorFieldTopology::NODE_SADDLE_2D;
        break;
      case 2:
        if (countComplex == 0)
        {
          critType = vtkVectorFieldTopology::REPELLING_NODE_2D;
        }
        else
        {
          critType = vtkVectorFieldTopology::REPELLING_FOCUS_2D;
        }
        break;
      default:
        break;
    }
  }
  else if (countComplex == 2)
  {
    critType = vtkVectorFieldTopology::CENTER_DETAILED_2D;
  }
  return critType;
}

//----------------------------------------------------------------------------
int vtkVectorFieldTopology::Classify3D(int countComplex, int countPos, int countNeg)
{
  // make simple type that corresponds to the number of positive eigenvalues
  // SOURCE_3D 3, SADDLE_2_3D 2, SADDLE_1_3D 1, SINK_3D 0, (CENTER_3D 4)
  // in analogy to ttk, where the type corresponds to the down directions
  int critType = vtkVectorFieldTopology::DEGENERATE_3D;
  if (countPos + countNeg == 3)
  {
    switch (countPos)
    {
      case 0:
        critType = vtkVectorFieldTopology::SINK_3D;
        break;
      case 1:
        critType = vtkVectorFieldTopology::SADDLE_1_3D;
        break;
      case 2:
        critType = vtkVectorFieldTopology::SADDLE_2_3D;
        break;
      case 3:
        critType = vtkVectorFieldTopology::SOURCE_3D;
        break;
      default:
        break;
    }
  }
  else if (countComplex > 0)
  {
    critType = vtkVectorFieldTopology::CENTER_3D;
  }
  return critType;
}

//----------------------------------------------------------------------------
int vtkVectorFieldTopology::ClassifyDetailed3D(int countComplex, int countPos, int countNeg)
{
  // determine which type of critical point we have including distinction between node and spiral
  // ATTRACTING_NODE_3D 0, ATTRACTING_FOCUS_3D 1, NODE_SADDLE_1_3D 2, FOCUS_SADDLE_1_3D 3,
  // NODE_SADDLE_2_3D 4, FOCUS_SADDLE_2_3D 5, REPELLING_NODE_3D 6, REPELLING_FOCUS_3D 7,
  // CENTER_DETAILED_3D 8
  int critType = vtkVectorFieldTopology::DEGENERATE_3D;
  if (countPos + countNeg == 3)
  {
    switch (countPos)
    {
      case 0:
        if (countComplex == 0)
        {
          critType = vtkVectorFieldTopology::ATTRACTING_NODE_3D;
        }
        else
        {
          critType = vtkVectorFieldTopology::ATTRACTING_FOCUS_3D;
        }
        break;
      case 1:
        if (countComplex == 0)
        {
          critType = vtkVectorFieldTopology::NODE_SADDLE_1_3D;
        }
        else
        {
          critType = vtkVectorFieldTopology::FOCUS_SADDLE_1_3D;
        }
        break;
      case 2:
        if (countComplex == 0)
        {
          critType = vtkVectorFieldTopology::NODE_SADDLE_2_3D;
        }
        else
        {
          critType = vtkVectorFieldTopology::FOCUS_SADDLE_2_3D;
        }
        break;
      case 3:
        if (countComplex == 0)
        {
          critType = vtkVectorFieldTopology::REPELLING_NODE_3D;
        }
        else
        {
          critType = vtkVectorFieldTopology::REPELLING_FOCUS_3D;
        }
        break;
      default:
        break;
    }
  }
  else if (countComplex > 0)
  {
    critType = vtkVectorFieldTopology::CENTER_DETAILED_3D;
  }
  return critType;
}

//----------------------------------------------------------------------------
int vtkVectorFieldTopology::ComputeCriticalPoints2D(
  vtkSmartPointer<vtkPolyData> criticalPoints, vtkSmartPointer<vtkUnstructuredGrid> tridataset)
{
  for (int cellId = 0; cellId < tridataset->GetNumberOfCells(); cellId++)
  {
    if (this->CheckAbort())
    {
      break;
    }
    auto cell = tridataset->GetCell(cellId);
    if (cell->GetCellType() != VTK_TRIANGLE)
    {
      continue;
    }
    vtkIdType indices[3] = { cell->GetPointId(0), cell->GetPointId(1), cell->GetPointId(2) };

    // array with the coordinates of the three triagle points: coords[point][component]
    vtkVector3d coords[3] = { vtkVector3d(tridataset->GetPoint(indices[0])),
      vtkVector3d(tridataset->GetPoint(indices[1])),
      vtkVector3d(tridataset->GetPoint(indices[2])) };

    // array with the vector values at the three triagle points: values[point][component]
    vtkVector3d values[3] = {
      vtkVector3d(
        tridataset->GetPointData()->GetArray(this->NameOfVectorArray)->GetTuple(indices[0])),
      vtkVector3d(
        tridataset->GetPointData()->GetArray(this->NameOfVectorArray)->GetTuple(indices[1])),
      vtkVector3d(
        tridataset->GetPointData()->GetArray(this->NameOfVectorArray)->GetTuple(indices[2]))
    };

    // matrix f(T) to convert to barycentric coordinates
    vtkNew<vtkMatrix3x3> valueMatrix;
    vtkNew<vtkMatrix3x3> coordsMatrix;
    for (int i = 0; i < 2; i++)
    {
      for (int j = 0; j < 2; j++)
      {
        valueMatrix->SetElement(j, i, values[i + 1][j] - values[0][j]);
        coordsMatrix->SetElement(j, i, coords[i + 1][j] - coords[0][j]);
      }
    }

    if (valueMatrix->Determinant() != 0)
    {
      valueMatrix->Invert();

      // barycentric coordinates of the zero: lambda = f(T)^-1 (-values[0])
      double lambda[3] = { -values[0][0], -values[0][1], -values[0][2] };
      valueMatrix->MultiplyPoint(lambda, lambda);

      // barycentric interpolation f(r) = f(T) * lambda + values[0] set to zero and solved for r
      // with lambda = T^-1 (r-r_0) results in r = T * f(T)^-1 (-values[0]) + coords[0]
      double zeroPos[3] = { coords[0][0] + lambda[0] * (coords[1][0] - coords[0][0]) +
          lambda[1] * (coords[2][0] - coords[0][0]),
        coords[0][1] + lambda[0] * (coords[1][1] - coords[0][1]) +
          lambda[1] * (coords[2][1] - coords[0][1]),
        coords[0][2] + lambda[0] * (coords[1][2] - coords[0][2]) +
          lambda[1] * (coords[2][2] - coords[0][2]) };

      // Check if zeroPos is inside the cell, i.e. if 0 <= lambda <= 1
      if (valueMatrix->Determinant() != 0 && lambda[0] >= -epsilon && lambda[1] >= -epsilon &&
        lambda[0] + lambda[1] <= 1.0 + epsilon)
      {
        bool isNewPoint = true;
        for (int i = 0; i < criticalPoints->GetNumberOfPoints(); ++i)
        {
          if (vtkMath::Distance2BetweenPoints(zeroPos, criticalPoints->GetPoint(i)) < epsilon)
          {
            isNewPoint = false;
          }
        }
        if (isNewPoint)
        {
          // gradient = f(T)T^-1
          vtkNew<vtkMatrix3x3> gradientMatrix;
          vtkMatrix3x3::Multiply3x3(coordsMatrix, valueMatrix, gradientMatrix);
          gradientMatrix->Invert();
          criticalPoints->GetPoints()->InsertNextPoint(zeroPos);
          criticalPoints->GetPointData()
            ->GetArray("gradient")
            ->InsertNextTuple9(gradientMatrix->GetElement(0, 0), gradientMatrix->GetElement(0, 1),
              gradientMatrix->GetElement(0, 2), gradientMatrix->GetElement(1, 0),
              gradientMatrix->GetElement(1, 1), gradientMatrix->GetElement(1, 2),
              gradientMatrix->GetElement(2, 0), gradientMatrix->GetElement(2, 1),
              gradientMatrix->GetElement(2, 2));
          vtkNew<vtkVertex> vertex;
          vertex->GetPointIds()->SetId(0, criticalPoints->GetNumberOfPoints() - 1);
          criticalPoints->GetVerts()->InsertNextCell(vertex);
        }
      }
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkVectorFieldTopology::ComputeCriticalPoints3D(
  vtkSmartPointer<vtkPolyData> criticalPoints, vtkSmartPointer<vtkUnstructuredGrid> tridataset)
{
  for (int cellId = 0; cellId < tridataset->GetNumberOfCells(); cellId++)
  {
    if (this->CheckAbort())
    {
      break;
    }
    auto cell = tridataset->GetCell(cellId);
    if (cell->GetCellType() != VTK_TETRA)
    {
      continue;
    }
    vtkIdType indices[4] = { cell->GetPointId(0), cell->GetPointId(1), cell->GetPointId(2),
      cell->GetPointId(3) };

    vtkVector3d coords[4] = { vtkVector3d(tridataset->GetPoint(indices[0])),
      vtkVector3d(tridataset->GetPoint(indices[1])), vtkVector3d(tridataset->GetPoint(indices[2])),
      vtkVector3d(tridataset->GetPoint(indices[3])) };

    vtkVector3d values[4] = {
      vtkVector3d(
        tridataset->GetPointData()->GetArray(this->NameOfVectorArray)->GetTuple(indices[0])),
      vtkVector3d(
        tridataset->GetPointData()->GetArray(this->NameOfVectorArray)->GetTuple(indices[1])),
      vtkVector3d(
        tridataset->GetPointData()->GetArray(this->NameOfVectorArray)->GetTuple(indices[2])),
      vtkVector3d(
        tridataset->GetPointData()->GetArray(this->NameOfVectorArray)->GetTuple(indices[3]))
    };

    vtkNew<vtkMatrix3x3> valueMatrix;
    vtkNew<vtkMatrix3x3> coordsMatrix;
    for (int i = 0; i < 3; i++)
    {
      for (int j = 0; j < 3; j++)
      {
        valueMatrix->SetElement(j, i, values[3][j] - values[i][j]);
        coordsMatrix->SetElement(j, i, coords[3][j] - coords[i][j]);
      }
    }

    if (valueMatrix->Determinant() != 0)
    {
      valueMatrix->Invert();
      double lambda[3] = { values[3][0], values[3][1], values[3][2] };
      valueMatrix->MultiplyPoint(lambda, lambda);

      double zeroPos[3] = { coords[0][0] * lambda[0] + coords[1][0] * lambda[1] +
          coords[2][0] * lambda[2] + coords[3][0] * (1.0 - lambda[0] - lambda[1] - lambda[2]),
        coords[0][1] * lambda[0] + coords[1][1] * lambda[1] + coords[2][1] * lambda[2] +
          coords[3][1] * (1.0 - lambda[0] - lambda[1] - lambda[2]),
        coords[0][2] * lambda[0] + coords[1][2] * lambda[1] + coords[2][2] * lambda[2] +
          coords[3][2] * (1.0 - lambda[0] - lambda[1] - lambda[2]) };

      // Check if zeroPos is inside the cell
      if (valueMatrix->Determinant() != 0 && lambda[0] >= -epsilon && lambda[1] >= -epsilon &&
        lambda[2] >= -epsilon && lambda[0] + lambda[1] + lambda[2] <= 1.0 + epsilon)
      {
        bool isNewPoint = true;
        for (int i = 0; i < criticalPoints->GetNumberOfPoints(); ++i)
        {
          if (vtkMath::Distance2BetweenPoints(zeroPos, criticalPoints->GetPoint(i)) < epsilon)
          {
            isNewPoint = false;
          }
        }
        if (isNewPoint)
        {
          // gradient = f(T)T^-1
          vtkNew<vtkMatrix3x3> gradientMatrix;
          vtkMatrix3x3::Multiply3x3(coordsMatrix, valueMatrix, gradientMatrix);
          gradientMatrix->Invert();
          criticalPoints->GetPoints()->InsertNextPoint(zeroPos);
          criticalPoints->GetPointData()
            ->GetArray("gradient")
            ->InsertNextTuple9(gradientMatrix->GetElement(0, 0), gradientMatrix->GetElement(0, 1),
              gradientMatrix->GetElement(0, 2), gradientMatrix->GetElement(1, 0),
              gradientMatrix->GetElement(1, 1), gradientMatrix->GetElement(1, 2),
              gradientMatrix->GetElement(2, 0), gradientMatrix->GetElement(2, 1),
              gradientMatrix->GetElement(2, 2));
          vtkNew<vtkVertex> vertex;
          vertex->GetPointIds()->SetId(0, criticalPoints->GetNumberOfPoints() - 1);
          criticalPoints->GetVerts()->InsertNextCell(vertex);
        }
      }
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkVectorFieldTopology::InterpolateVector(
  double x0, double x1, double x, const double v0[3], const double v1[3], double v[3])
{
  double y0[3], y1[3];
  vtkMath::Assign(v0, y0);
  vtkMath::MultiplyScalar(y0, x1 - x);
  vtkMath::Assign(v1, y1);
  vtkMath::MultiplyScalar(y1, x - x0);
  vtkMath::Add(y0, y1, v);
  vtkMath::MultiplyScalar(v, 1 / (x1 - x0));
  // vtkMath::Assign(v0, v);
}

//----------------------------------------------------------------------------
int vtkVectorFieldTopology::ComputeBoundarySwitchPoints(
  vtkPolyData* boundarySwitchPoints, vtkUnstructuredGrid* tridataset)
{
  vtkDataArray* vectors = tridataset->GetPointData()->GetArray(this->NameOfVectorArray);

  vtkNew<vtkDoubleArray> BoundarySwitchTypeArray;
  BoundarySwitchTypeArray->SetNumberOfComponents(1);
  BoundarySwitchTypeArray->SetName("BoundarySwitchType");
  BoundarySwitchTypeArray->SetNumberOfTuples(tridataset->GetNumberOfPoints());
  boundarySwitchPoints->GetPointData()->AddArray(BoundarySwitchTypeArray);

  vtkNew<vtkDoubleArray> BoundarySwitchPointVector;
  BoundarySwitchPointVector->SetNumberOfComponents(3);
  BoundarySwitchPointVector->SetName("Vector");
  BoundarySwitchPointVector->SetNumberOfTuples(tridataset->GetNumberOfPoints());
  boundarySwitchPoints->GetPointData()->AddArray(BoundarySwitchPointVector);

  vtkNew<vtkDoubleArray> BoundarySwitchPointNormal;
  BoundarySwitchPointNormal->SetNumberOfComponents(3);
  BoundarySwitchPointNormal->SetName("Normal");
  BoundarySwitchPointNormal->SetNumberOfTuples(tridataset->GetNumberOfPoints());
  boundarySwitchPoints->GetPointData()->AddArray(BoundarySwitchPointNormal);

  vtkNew<vtkGeometryFilter> geometry;
  geometry->SetInputData(tridataset);
  geometry->SetContainerAlgorithm(this);
  geometry->Update();

  vtkNew<vtkFeatureEdges> surface;
  surface->SetInputData(geometry->GetOutput());
  surface->SetContainerAlgorithm(this);
  surface->Update();

  vtkNew<vtkCellLocator> cellLocator;
  cellLocator->SetDataSet(tridataset);
  cellLocator->BuildLocator();
  cellLocator->Update();

  int numPoint = 0;

  // main loop
  for (int i = 0; i < surface->GetOutput()->GetNumberOfCells(); i++)
  {
    if (this->CheckAbort())
    {
      break;
    }
    // compute tangent and line normal of the line in the ith cell
    vtkCell* cell = surface->GetOutput()->GetCell(i);

    double p0[3], p1[3];
    surface->GetOutput()->GetPoint(cell->GetPointId(0), p0);
    surface->GetOutput()->GetPoint(cell->GetPointId(1), p1);

    double normal[3], tangent[3];
    normal[0] = p1[1] - p0[1];
    normal[1] = -(p1[0] - p0[0]);
    normal[2] = 0;

    double norm = vtkMath::Norm(normal);
    if (norm == 0)
      continue;

    tangent[0] = p1[0] - p0[0];
    tangent[1] = p1[1] - p0[1];
    tangent[2] = 0;

    // makes sure that the line normal points inward
    double offset[3], shiftedPoint[3];
    vtkMath::Assign(normal, offset);
    vtkMath::MultiplyScalar(offset, 0.1);

    vtkMath::Subtract(p0, offset, shiftedPoint);

    if (cellLocator->FindCell(shiftedPoint) == -1)
    {
      vtkMath::Subtract(p1, offset, shiftedPoint);
      if (cellLocator->FindCell(shiftedPoint) == -1)
      {
        normal[0] = -normal[0];
        normal[1] = -normal[1];
      }
    }

    // get vectors at the two end points of the line
    double vector0[3], vector0Normalized[3];
    surface->GetOutput()
      ->GetPointData()
      ->GetArray(vectors->GetName())
      ->GetTuple(cell->GetPointId(0), vector0);
    double vector0Norm = vtkMath::Norm(vector0);
    vtkMath::Assign(vector0, vector0Normalized);
    vtkMath::MultiplyScalar(vector0Normalized, 1 / vector0Norm);
    double vector1[3], vector1Normalized[3];
    surface->GetOutput()
      ->GetPointData()
      ->GetArray(vectors->GetName())
      ->GetTuple(cell->GetPointId(1), vector1);
    double vector1Norm = vtkMath::Norm(vector1);
    vtkMath::Assign(vector1, vector1Normalized);
    vtkMath::MultiplyScalar(vector1Normalized, 1 / vector1Norm);

    // find the location of boundary switch point using the inverse of linear interpolate function.
    double x, y, vn0, vn1, point[3], vector[3];
    vn0 = vtkMath::Dot(vector0, normal);
    vn1 = vtkMath::Dot(vector1, normal);
    y = vn0 - vn1;

    // when vn0 == 0 or vn1 == 0, the vectors are parallel to the boundary
    // when vn0 * vn1 > 0, both of the vectors are neither inflow nor outflow.
    if (vn0 * vn1 >= 0)
      continue;

    x = vn0 / y;

    // if the location is in between the two end points of the line
    if (x > 0 && x < 1)
    {
      InterpolateVector(0, 1, x, vector0, vector1, vector);

      double vectorNorm = vtkMath::Norm(vector);

      // if the vector at the boundary switch point is not a zero vector
      if (vectorNorm > 1e-16)
      {
        vtkMath::MultiplyScalar(vector, 1 / vectorNorm);

        double cosTheta = vtkMath::Dot(vector0Normalized, vector1Normalized);
        if (fabs(cosTheta) <= this->VectorAngleThreshold)
        {
          // inflow boundary switch point
          if ((vtkMath::Dot(vector, tangent) > 0 && vn1 < 0) ||
            (vtkMath::Dot(vector, tangent) < 0 && vn0 < 0))
          {
            InterpolateVector(0, 1, x, p0, p1, point);
            boundarySwitchPoints->GetPoints()->InsertNextPoint(point);
            BoundarySwitchTypeArray->SetTuple1(numPoint, 0);

            vtkNew<vtkVertex> vertex;
            vertex->GetPointIds()->SetId(0, boundarySwitchPoints->GetNumberOfPoints() - 1);
            boundarySwitchPoints->GetVerts()->InsertNextCell(vertex);
          }
          // outflow boundary switch point
          else if ((vtkMath::Dot(vector, tangent) > 0 && vn1 > 0) ||
            (vtkMath::Dot(vector, tangent) < 0 && vn0 > 0))
          {
            InterpolateVector(0, 1, x, p0, p1, point);
            boundarySwitchPoints->GetPoints()->InsertNextPoint(point);
            BoundarySwitchTypeArray->SetTuple1(numPoint, 1);

            vtkNew<vtkVertex> vertex;
            vertex->GetPointIds()->SetId(0, boundarySwitchPoints->GetNumberOfPoints() - 1);
            boundarySwitchPoints->GetVerts()->InsertNextCell(vertex);
          }
          else
            continue;
          BoundarySwitchPointVector->SetTuple3(numPoint, vector[0], vector[1], vector[2]);
          BoundarySwitchPointNormal->SetTuple3(numPoint, normal[0], normal[1], normal[2]);

          numPoint++;
        }
      }
    }
  }

  BoundarySwitchTypeArray->SetNumberOfTuples(numPoint);
  BoundarySwitchPointVector->SetNumberOfTuples(numPoint);
  BoundarySwitchPointNormal->SetNumberOfTuples(numPoint);

  return 1;
}

//----------------------------------------------------------------------------
int vtkVectorFieldTopology::ComputeSurface(int numberOfSeparatingSurfaces, bool isBackward,
  double normal[3], double zeroPos[3], vtkPolyData* streamSurfaces, vtkDataSet* dataset,
  int vtkNotUsed(integrationStepUnit), double dist, double vtkNotUsed(stepSize), int maxNumSteps,
  bool useIterativeSeeding)
{
  // generate circle and add first point again in the back to avoid gap
  vtkNew<vtkRegularPolygonSource> circle;
  circle->GeneratePolygonOff();
  circle->SetNumberOfSides(8);
  circle->SetRadius(dist);
  circle->SetCenter(zeroPos);
  circle->SetNormal(normal);
  circle->SetContainerAlgorithm(this);
  circle->Update();

  // close circle exactly with a point instead of an edge to correctly treat points exiting the
  // boundary
  circle->GetOutput()->GetPoints()->InsertNextPoint(circle->GetOutput()->GetPoint(0));
  vtkNew<vtkPolyData> currentCircle;
  currentCircle->SetPoints(circle->GetOutput()->GetPoints());
  vtkNew<vtkDoubleArray> integrationTimeArray;
  integrationTimeArray->SetName("IntegrationTime");
  currentCircle->GetPointData()->AddArray(integrationTimeArray);
  integrationTimeArray->Resize(currentCircle->GetNumberOfPoints());
  for (int i = 0; i < currentCircle->GetNumberOfPoints(); ++i)
  {
    integrationTimeArray->SetTuple1(i, (double)0);
  }

  this->StreamSurface->SetInputData(0, dataset);
  this->StreamSurface->SetInputData(1, currentCircle);
  this->StreamSurface->SetUseIterativeSeeding(useIterativeSeeding);
  this->StreamSurface->SetIntegratorTypeToRungeKutta4();
  this->StreamSurface->SetIntegrationStepUnit(this->IntegrationStepUnit);
  this->StreamSurface->SetInitialIntegrationStep(this->IntegrationStepSize);
  this->StreamSurface->SetIntegrationDirection(isBackward);
  this->StreamSurface->SetComputeVorticity(false);
  this->StreamSurface->SetMaximumNumberOfSteps(maxNumSteps);
  this->StreamSurface->SetSourceData(currentCircle);
  this->StreamSurface->SetMaximumPropagation(dist * maxNumSteps);
  this->StreamSurface->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, this->NameOfVectorArray);
  this->StreamSurface->Update();

  vtkNew<vtkDoubleArray> indexArray;
  indexArray->SetName("index");
  indexArray->SetNumberOfTuples(StreamSurface->GetOutput()->GetNumberOfPoints());
  indexArray->Fill(numberOfSeparatingSurfaces);
  StreamSurface->GetOutput()->GetPointData()->AddArray(indexArray);

  // add current surface to existing surfaces
  vtkNew<vtkAppendPolyData> appendSurfaces;
  appendSurfaces->AddInputData(this->StreamSurface->GetOutput());
  appendSurfaces->AddInputData(streamSurfaces);
  appendSurfaces->SetContainerAlgorithm(this);
  appendSurfaces->Update();
  streamSurfaces->DeepCopy(appendSurfaces->GetOutput());
  this->StreamSurface->SetInputData(0, nullptr);
  this->StreamSurface->SetInputData(1, nullptr);

  return (streamSurfaces != nullptr);
}

//----------------------------------------------------------------------------
int vtkVectorFieldTopology::ComputeSeparatricesBoundarySwitchPoints(
  vtkPolyData* boundarySwitchPoints, vtkPolyData* separatrices, vtkDataSet* dataset,
  vtkPoints* interestPoints, int integrationStepUnit, double dist, int maxNumSteps)
{
  double offsetAwayFromBoundary = this->OffsetAwayFromBoundary;
  if (integrationStepUnit == vtkStreamTracer::CELL_LENGTH_UNIT)
  {
    dist *= sqrt(static_cast<double>(dataset->GetCell(0)->GetLength2()));
    offsetAwayFromBoundary *= sqrt(static_cast<double>(dataset->GetCell(0)->GetLength2()));
  }

  vtkNew<vtkStreamTracer> streamTracer;
  streamTracer->SetInputData(dataset);
  streamTracer->SetIntegratorTypeToRungeKutta4();
  streamTracer->SetIntegrationStepUnit(this->IntegrationStepUnit);
  streamTracer->SetInitialIntegrationStep(this->IntegrationStepSize);
  streamTracer->SetComputeVorticity(false);
  streamTracer->SetMaximumNumberOfSteps(maxNumSteps);
  streamTracer->SetMaximumPropagation(dist * maxNumSteps);
  streamTracer->SetTerminalSpeed(epsilon);
  streamTracer->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, this->NameOfVectorArray);
  streamTracer->SetInterpolatorType(this->InterpolatorType);
  streamTracer->SetContainerAlgorithm(this);

  int numberOfSeparatingLines = 0;

  for (int i = 0; i < boundarySwitchPoints->GetNumberOfPoints(); i++)
  {
    if (this->CheckAbort())
    {
      break;
    }
    if (boundarySwitchPoints->GetPointData()->GetArray("BoundarySwitchType")->GetTuple1(i) == 1)
      continue;

    double point[3], vector[3], normal[3];
    boundarySwitchPoints->GetPoint(i, point);
    boundarySwitchPoints->GetPointData()->GetArray("Vector")->GetTuple(i, vector);
    boundarySwitchPoints->GetPointData()->GetArray("Normal")->GetTuple(i, normal);

    double normalNorm = vtkMath::Norm(normal);
    vtkMath::MultiplyScalar(normal, offsetAwayFromBoundary / normalNorm);
    double vectorNorm = vtkMath::Norm(vector);
    vtkMath::MultiplyScalar(vector, 1 / vectorNorm);

    for (int k = 0; k < 2; k++)
    {
      // insert seed with small offset
      vtkNew<vtkPolyData> seeds;
      vtkNew<vtkPoints> seedPoints;
      vtkNew<vtkCellArray> seedCells;
      seeds->SetPoints(seedPoints);
      seeds->SetVerts(seedCells);

      int sign = pow(-1, k);

      double offset[3], shiftedPoint[3], shiftedPoint1[3];
      vtkMath::Assign(vector, offset);
      vtkMath::MultiplyScalar(offset, sign * dist);
      vtkMath::Add(point, offset, shiftedPoint);
      vtkMath::Subtract(shiftedPoint, normal, shiftedPoint1);

      seedPoints->InsertNextPoint(shiftedPoint1);

      vtkNew<vtkVertex> vertex;
      vertex->GetPointIds()->SetId(0, seedPoints->GetNumberOfPoints() - 1);
      seedCells->InsertNextCell(vertex);

      // integrate
      if (k == 0)
        streamTracer->SetIntegrationDirection(false);
      else
        streamTracer->SetIntegrationDirection(true);

      streamTracer->SetSourceData(seeds);
      streamTracer->Update();

      if (streamTracer->GetOutput()->GetNumberOfPoints() > 0)
      {
        // close gap to the boundary switch point at the beginning
        streamTracer->GetOutput()->GetPoints()->InsertNextPoint(boundarySwitchPoints->GetPoint(i));
        for (int j = 0; j < streamTracer->GetOutput()->GetPointData()->GetNumberOfArrays(); j++)
        {
          streamTracer->GetOutput()->GetPointData()->GetArray(j)->InsertNextTuple(
            streamTracer->GetOutput()->GetPointData()->GetArray(j)->GetTuple(0));
        }

        // this polyline with the 2 new points will replace the old polyline
        vtkSmartPointer<vtkPolyLine> polyLine = vtkSmartPointer<vtkPolyLine>::New();
        polyLine->GetPointIds()->SetNumberOfIds(streamTracer->GetOutput()->GetNumberOfPoints());
        polyLine->GetPointIds()->SetId(0, streamTracer->GetOutput()->GetNumberOfPoints() - 1);
        for (int l = 1; l < streamTracer->GetOutput()->GetNumberOfPoints(); l++)
        {
          polyLine->GetPointIds()->SetId(l, l - 1);
        }

        // close gap to the boundary switch point at the end
        int closestBoundarySwitchPointToEnd = 0;
        double closestDistance[3];
        vtkMath::Subtract(
          streamTracer->GetOutput()->GetPoint(streamTracer->GetOutput()->GetNumberOfPoints() - 2),
          interestPoints->GetPoint(closestBoundarySwitchPointToEnd), closestDistance);
        double currentDistance[3];

        // find closest BoundarySwitch point to endpoint
        for (int j = 0; j < interestPoints->GetNumberOfPoints(); j++)
        {
          vtkMath::Subtract(
            streamTracer->GetOutput()->GetPoint(streamTracer->GetOutput()->GetNumberOfPoints() - 2),
            interestPoints->GetPoint(j), currentDistance);
          if (vtkMath::Norm(currentDistance) < vtkMath::Norm(closestDistance))
          {
            closestBoundarySwitchPointToEnd = j;
            for (int d = 0; d < 3; d++)
            {
              closestDistance[d] = currentDistance[d];
            }
          }
        }

        if (vtkMath::Norm(closestDistance) < dist)
        {
          // find closest point on streamline to that Boundary Switch point to avoid self
          // intersection
          int firstClosePointToBoundarySwitchPoint = 0;
          for (int j = 0; j < streamTracer->GetOutput()->GetNumberOfPoints(); j++)
          {
            vtkMath::Subtract(streamTracer->GetOutput()->GetPoint(j),
              interestPoints->GetPoint(closestBoundarySwitchPointToEnd), currentDistance);
            if (vtkMath::Norm(currentDistance) < dist)
            {
              firstClosePointToBoundarySwitchPoint = j;
              for (int d = 0; d < 3; d++)
              {
                closestDistance[d] = currentDistance[d];
              }
              break;
            }
          }

          // insert new point
          streamTracer->GetOutput()->GetPoints()->InsertNextPoint(
            interestPoints->GetPoint(closestBoundarySwitchPointToEnd));
          for (int j = 0; j < streamTracer->GetOutput()->GetPointData()->GetNumberOfArrays(); j++)
          {
            streamTracer->GetOutput()->GetPointData()->GetArray(j)->InsertNextTuple(
              streamTracer->GetOutput()->GetPointData()->GetArray(j)->GetTuple(0));
          }

          // remove superfluous lines in the tail and connect to Boundary Switch point instead
          polyLine->GetPointIds()->SetNumberOfIds(firstClosePointToBoundarySwitchPoint + 2);
          polyLine->GetPointIds()->SetId(firstClosePointToBoundarySwitchPoint + 1,
            streamTracer->GetOutput()->GetNumberOfPoints() - 1);
        }
        vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
        cells->InsertNextCell(polyLine);
        streamTracer->GetOutput()->SetLines(cells);

        // fill arrays
        vtkNew<vtkDoubleArray> iterationArray;
        iterationArray->SetName("iteration");
        iterationArray->SetNumberOfTuples(streamTracer->GetOutput()->GetNumberOfPoints());
        streamTracer->GetOutput()->GetPointData()->AddArray(iterationArray);
        vtkNew<vtkDoubleArray> indexArray;
        indexArray->SetName("index");
        indexArray->SetNumberOfTuples(streamTracer->GetOutput()->GetNumberOfPoints());
        streamTracer->GetOutput()->GetPointData()->AddArray(indexArray);
        for (int j = 0; j < streamTracer->GetOutput()->GetNumberOfPoints(); j++)
        {
          iterationArray->SetTuple1(j, j + 1);
          indexArray->SetTuple1(j, numberOfSeparatingLines);
        }
        // iterationArray->SetTuple1(streamTracer->GetOutput()->GetNumberOfPoints() - 1, 0);
        // the inserted points will get iteration 0
        iterationArray->SetTuple1(streamTracer->GetOutput()->GetNumberOfPoints() - 1, 0);
        if (vtkMath::Norm(closestDistance) < dist)
        {
          iterationArray->SetTuple1(streamTracer->GetOutput()->GetNumberOfPoints() - 2, 0);
        }

        // combine lines of this separatrix with existing ones
        vtkNew<vtkAppendPolyData> appendFilter;
        appendFilter->SetContainerAlgorithm(this);
        appendFilter->AddInputData(separatrices);
        appendFilter->AddInputData(streamTracer->GetOutput());
        appendFilter->Update();
        separatrices->DeepCopy(appendFilter->GetOutput());
        numberOfSeparatingLines++;
      }
    }
  }

  return 1;
}

int vtkVectorFieldTopology::ComputeSeparatricesBoundarySwitchLines(vtkPolyData* boundarySwitchLines,
  vtkPolyData* separatrices, vtkDataSet* dataset, int integrationStepUnit, double dist,
  int maxNumSteps, bool computeSurfaces, bool useIterativeSeeding)
{
  double offsetAwayFromBoundary = this->OffsetAwayFromBoundary;
  if (integrationStepUnit == vtkStreamTracer::CELL_LENGTH_UNIT)
  {
    dist *= sqrt(static_cast<double>(dataset->GetCell(0)->GetLength2()));
    offsetAwayFromBoundary *= sqrt(static_cast<double>(dataset->GetCell(0)->GetLength2()));
  }

  vtkDataArray* vectors = dataset->GetPointData()->GetArray(this->NameOfVectorArray);

  vtkNew<vtkGeometryFilter> geometry;
  geometry->SetInputData(dataset);
  geometry->SetContainerAlgorithm(this);
  geometry->Update();

  vtkNew<vtkDataSetSurfaceFilter> surface;
  surface->SetInputData(geometry->GetOutput());
  surface->SetContainerAlgorithm(this);
  surface->Update();

  vtkNew<vtkDoubleArray> normals;
  normals->SetNumberOfComponents(3);
  normals->SetName("Normals");
  normals->SetNumberOfTuples(dataset->GetNumberOfCells());
  surface->GetOutput()->GetCellData()->AddArray(normals);

  vtkNew<vtkDoubleArray> scalarProduct;
  scalarProduct->SetNumberOfComponents(1);
  scalarProduct->SetName("ScalarProduct");
  scalarProduct->SetNumberOfTuples(dataset->GetNumberOfPoints());
  surface->GetOutput()->GetPointData()->AddArray(scalarProduct);

  // compute surface normals and store them as cell data
  for (int i = 0; i < surface->GetOutput()->GetNumberOfCells(); i++)
  {
    vtkCell* cell = surface->GetOutput()->GetCell(i);

    if (cell->GetNumberOfPoints() < 3)
      vtkErrorMacro(
        "vtkVectorFieldTopology::ComputeBoundarySwitchLines: the number of points in a cell is "
        "less then 3. This cell should represent a 2D surface and should have at least 3 points");
    double p0[3], p1[3], p2[3];

    surface->GetOutput()->GetPoint(cell->GetPointId(0), p0);
    surface->GetOutput()->GetPoint(cell->GetPointId(1), p1);
    surface->GetOutput()->GetPoint(cell->GetPointId(2), p2);

    double t0[3], t1[3];

    vtkMath::Subtract(p1, p0, t0);
    vtkMath::Subtract(p2, p0, t1);

    double n[3];
    vtkMath::Cross(t0, t1, n);
    vtkMath::MultiplyScalar(n, 1 / vtkMath::Norm(n));
    surface->GetOutput()->GetCellData()->GetArray("Normals")->SetTuple(i, n);
  }

  // use cell2point to compute surfaces at each point
  // and compute the dot product between vector and surface normal
  vtkNew<vtkCellDataToPointData> cell2point;
  cell2point->SetInputData(surface->GetOutput());
  cell2point->SetContainerAlgorithm(this);
  cell2point->Update();

  for (int i = 0; i < surface->GetOutput()->GetNumberOfPoints(); i++)
  {
    double n[3], v[3], p;
    cell2point->GetOutput()->GetPointData()->GetArray("Normals")->GetTuple(i, n);
    cell2point->GetOutput()->GetPointData()->GetArray(vectors->GetName())->GetTuple(i, v);
    p = vtkMath::Dot(n, v);

    if (fabs(p) < 1e-10)
      cell2point->GetOutput()->GetPointData()->GetArray("ScalarProduct")->SetTuple1(i, 0);
    else
      cell2point->GetOutput()->GetPointData()->GetArray("ScalarProduct")->SetTuple1(i, p);
  }

  // use the contour filter to find lines where dot products are zeros
  vtkNew<vtkContourFilter> contourFilter;
  contourFilter->SetInputData(cell2point->GetOutput());
  contourFilter->SetValue(0, 0);
  // (id=0 for first array, port=0, connection=0, pointData=0 and cellData=1, name)
  contourFilter->SetInputArrayToProcess(0, 0, 0, 0, "ScalarProduct");
  contourFilter->SetContainerAlgorithm(this);
  contourFilter->Update();

  if (contourFilter->GetOutput()->GetNumberOfCells() == 0)
    return 1;

  // copy celldata to boundarySwitchLines
  // delete the temporary arrays "Normals" and "ScalarProduct"
  // keep the vector array and the output
  boundarySwitchLines->DeepCopy(contourFilter->GetOutput());
  boundarySwitchLines->GetPointData()->RemoveArray("Normals");
  boundarySwitchLines->GetPointData()->RemoveArray("ScalarProduct");
  vtkNew<vtkDoubleArray> BoundarySwitchTypeArray;
  BoundarySwitchTypeArray->SetNumberOfComponents(1);
  BoundarySwitchTypeArray->SetName("BoundarySwitchType");
  BoundarySwitchTypeArray->SetNumberOfTuples(boundarySwitchLines->GetNumberOfCells());
  boundarySwitchLines->GetPointData()->AddArray(BoundarySwitchTypeArray);
  for (int i = 0; i < BoundarySwitchTypeArray->GetNumberOfTuples(); i++)
    BoundarySwitchTypeArray->SetTuple1(i, -1);

  boundarySwitchLines->BuildLinks();

  vtkNew<vtkDoubleArray> lineNormals;
  lineNormals->SetNumberOfComponents(3);
  lineNormals->SetNumberOfTuples(contourFilter->GetOutput()->GetNumberOfCells());
  lineNormals->SetName("LineNormals");
  contourFilter->GetOutput()->GetCellData()->AddArray(lineNormals);

  vtkNew<vtkDoubleArray> tangents;
  tangents->SetNumberOfComponents(3);
  tangents->SetNumberOfTuples(contourFilter->GetOutput()->GetNumberOfCells());
  tangents->SetName("Tangents");
  contourFilter->GetOutput()->GetCellData()->AddArray(tangents);

  vtkNew<vtkDoubleArray> surfaceNormals;
  surfaceNormals->SetNumberOfComponents(3);
  surfaceNormals->SetNumberOfTuples(contourFilter->GetOutput()->GetNumberOfCells());
  surfaceNormals->SetName("SurfaceNormals");
  contourFilter->GetOutput()->GetCellData()->AddArray(surfaceNormals);

  vtkNew<vtkCellLocator> cellLocator;
  cellLocator->SetDataSet(surface->GetOutput());
  cellLocator->BuildLocator();
  cellLocator->Update();

  vtkNew<vtkPolyData> offsetPoints;
  vtkNew<vtkPoints> points;
  offsetPoints->SetPoints(points);

  // Because each point is associated with two normals, this step averages the normal for every
  // point
  vtkNew<vtkDoubleArray> normalArray;
  normalArray->SetNumberOfComponents(3);
  normalArray->SetNumberOfTuples(contourFilter->GetOutput()->GetNumberOfPoints());

  for (int i = 0; i < contourFilter->GetOutput()->GetNumberOfCells(); i++)
  {
    vtkCell* cell = contourFilter->GetOutput()->GetCell(i);
    double p0[3], p1[3], tangent[3], center[3];

    contourFilter->GetOutput()->GetPoint(cell->GetPointId(0), p0);
    contourFilter->GetOutput()->GetPoint(cell->GetPointId(1), p1);
    vtkMath::Subtract(p1, p0, tangent);
    vtkMath::Add(p0, p1, center);
    vtkMath::MultiplyScalar(center, 0.5);
    int cellIDCenter = cellLocator->FindCell(center);

    if (cellIDCenter != -1)
    {
      double surfaceNormal[3];
      surface->GetOutput()->GetCellData()->GetArray("Normals")->GetTuple(
        cellIDCenter, surfaceNormal);
      double lineNormal[3];
      vtkMath::Cross(tangent, surfaceNormal, lineNormal);
      vtkMath::MultiplyScalar(lineNormal, 1 / vtkMath::Norm(lineNormal));

      double x[3];
      normalArray->GetTuple(i, x);

      normalArray->SetTuple3(i, x[0] + lineNormal[0], x[1] + lineNormal[1], x[2] + lineNormal[2]);
    }
  }

  for (int i = 0; i < normalArray->GetNumberOfTuples(); i++)
  {
    double x[3];
    normalArray->GetTuple(i, x);
    vtkMath::MultiplyScalar(x, 0.5);
    normalArray->SetTuple3(i, x[0], x[1], x[2]);
  }

  // the outputs of the contour filter are potential boundary switch lines.
  // computed shifted boundary switch lines as seeds for computing separating surfaces
  for (int i = 0; i < contourFilter->GetOutput()->GetNumberOfCells(); i++)
  {
    vtkCell* cell = contourFilter->GetOutput()->GetCell(i);
    double p0[3], p1[3], tangent[3], center[3];

    contourFilter->GetOutput()->GetPoint(cell->GetPointId(0), p0);
    contourFilter->GetOutput()->GetPoint(cell->GetPointId(1), p1);

    vtkMath::Subtract(p1, p0, tangent);

    tangents->SetTuple(i, tangent);

    vtkMath::Add(p0, p1, center);
    vtkMath::MultiplyScalar(center, 0.5);
    int cellIDCenter = cellLocator->FindCell(center);

    if (cellIDCenter != -1)
    {
      double surfaceNormal[3];
      surface->GetOutput()->GetCellData()->GetArray("Normals")->GetTuple(
        cellIDCenter, surfaceNormal);
      surfaceNormals->SetTuple(i, surfaceNormal);

      double lineNormal[3];
      vtkMath::Cross(tangent, surfaceNormal, lineNormal);
      vtkMath::MultiplyScalar(lineNormal, 1 / vtkMath::Norm(lineNormal));
      lineNormals->SetTuple(i, lineNormal);

      double left[3], right[3];
      vtkMath::MultiplyScalar(lineNormal, dist);
      // vtkMath::MultiplyScalar(lineNormal,dist );
      vtkMath::Subtract(center, lineNormal, right);
      vtkMath::MultiplyScalar(lineNormal, -1);
      vtkMath::Subtract(center, lineNormal, left);

      int cellID0 = cellLocator->FindCell(left);
      int cellID1 = cellLocator->FindCell(right);

      // the points have to be inside the boundary
      if (cellID0 != -1 && cellID1 != -1)
      {
        points->InsertNextPoint(left);
        points->InsertNextPoint(right);
        points->InsertNextPoint(center);
      }
    }
  }

  // use probe filter to interpolate the vectors at seeds points and center points of  lines
  vtkNew<vtkProbeFilter> probe;
  probe->SetInputData(offsetPoints);
  probe->SetSourceData(dataset);
  probe->SetContainerAlgorithm(this);
  probe->Update();

  vtkNew<vtkPointLocator> pointLocator;
  pointLocator->SetDataSet(probe->GetOutput());
  pointLocator->BuildLocator();
  pointLocator->Update();

  // streamSurface filter for computing surface
  vtkNew<vtkStreamSurface> streamSurface;
  streamSurface->SetInputData(dataset);
  streamSurface->SetIntegratorTypeToRungeKutta4();
  streamSurface->SetIntegrationStepUnit(IntegrationStepUnit);
  streamSurface->SetInitialIntegrationStep(IntegrationStepSize);
  streamSurface->SetComputeVorticity(false);
  streamSurface->SetMaximumNumberOfSteps(maxNumSteps);
  streamSurface->SetMaximumPropagation(dist * maxNumSteps);
  streamSurface->SetTerminalSpeed(epsilon);
  streamSurface->SetUseIterativeSeeding(useIterativeSeeding);
  streamSurface->SetInterpolatorTypeToCellLocator();
  streamSurface->SetContainerAlgorithm(this);

  vtkNew<vtkPolyData> seeds;

  // main loop for deciding whether or not the outputs of the contour filter are boundary switch
  // lines.
  for (int i = 0; i < contourFilter->GetOutput()->GetNumberOfCells(); i++)
  {
    if (this->CheckAbort())
    {
      break;
    }
    vtkCell* cell = contourFilter->GetOutput()->GetCell(i);
    double p0[3], p1[3], tangent[3], center[3];

    contourFilter->GetOutput()->GetPoint(cell->GetPointId(0), p0);
    contourFilter->GetOutput()->GetPoint(cell->GetPointId(1), p1);

    vtkMath::Subtract(p1, p0, tangent);

    double surfaceNormal[3], lineNormal[3], lineNormalNormalized[3];

    surfaceNormals->GetTuple(i, surfaceNormal);

    vtkMath::Add(p0, p1, center);
    vtkMath::MultiplyScalar(center, 0.5);
    int cellIDCenter = cellLocator->FindCell(center);

    if (cellIDCenter != -1)
    {
      vtkMath::Cross(tangent, surfaceNormal, lineNormal);
      vtkMath::MultiplyScalar(lineNormal, 1 / vtkMath::Norm(lineNormal));

      lineNormals->SetTuple(i, lineNormal);
      vtkMath::Assign(lineNormal, lineNormalNormalized);

      double left[3], right[3];
      vtkMath::MultiplyScalar(lineNormal, dist);
      vtkMath::Subtract(center, lineNormal, right);
      vtkMath::MultiplyScalar(lineNormal, -1);
      vtkMath::Subtract(center, lineNormal, left);

      int cellID0 = cellLocator->FindCell(left);
      int cellID1 = cellLocator->FindCell(right);
      int centerID = pointLocator->FindClosestPoint(center);

      double v[3];
      probe->GetOutput()->GetPointData()->GetArray(vectors->GetName())->GetTuple(centerID, v);

      double lineNormalSign = vtkMath::Dot(lineNormalNormalized, v);
      if (lineNormalSign > 0)
        lineNormalSign = 1;
      else
        lineNormalSign = -1;

      if (cellID0 != -1 && cellID1 != -1)
      {
        int pointID0 = pointLocator->FindClosestPoint(left);
        int pointID1 = pointLocator->FindClosestPoint(right);

        double v0[3], v1[3];
        probe->GetOutput()->GetPointData()->GetArray(vectors->GetName())->GetTuple(pointID0, v0);
        probe->GetOutput()->GetPointData()->GetArray(vectors->GetName())->GetTuple(pointID1, v1);

        double surfaceNormal0[3], surfaceNormal1[3];
        surface->GetOutput()->GetCellData()->GetArray("Normals")->GetTuple(cellID0, surfaceNormal0);
        surface->GetOutput()->GetCellData()->GetArray("Normals")->GetTuple(cellID1, surfaceNormal1);
        double sign0 = vtkMath::Dot(v0, surfaceNormal0);
        double sign1 = vtkMath::Dot(v1, surfaceNormal1);

        if ((lineNormalSign == -1 && sign0 > 0 && sign1 < 0) ||
          (lineNormalSign == 1 && sign0 < 0 && sign1 > 0))
        {
          // inflow
          BoundarySwitchTypeArray->SetTuple1(cell->GetPointId(0), 0);
          BoundarySwitchTypeArray->SetTuple1(cell->GetPointId(1), 0);
        }
        else if ((lineNormalSign == -1 && sign0 < 0 && sign1 > 0) ||
          (lineNormalSign == 1 && sign0 > 0 && sign1 < 0))
        {
          // outflow
          BoundarySwitchTypeArray->SetTuple1(cell->GetPointId(0), 1);
          BoundarySwitchTypeArray->SetTuple1(cell->GetPointId(1), 1);
        }
        else
        {
          BoundarySwitchTypeArray->SetTuple1(cell->GetPointId(0),
            std::max((double)(-1), BoundarySwitchTypeArray->GetTuple1(cell->GetPointId(0))));
          BoundarySwitchTypeArray->SetTuple1(cell->GetPointId(1),
            std::max((double)(-1), BoundarySwitchTypeArray->GetTuple1(cell->GetPointId(1))));
        }

        if (BoundarySwitchTypeArray->GetTuple1(cell->GetPointId(0)) != -1 &&
          BoundarySwitchTypeArray->GetTuple1(cell->GetPointId(1)) != -1)
        {
          if (computeSurfaces)
          {

            for (int k = 0; k < 2; k++)
            {

              vtkNew<vtkPoints> seedPoints;
              vtkNew<vtkCellArray> seedCells;
              seeds->SetPoints(seedPoints);
              seeds->SetLines(seedCells);

              bool isBackward;

              double lineNormal0[3], lineNormal1[3];
              normalArray->GetTuple(cell->GetPointId(0), lineNormal0);
              normalArray->GetTuple(cell->GetPointId(1), lineNormal1);
              vtkMath::MultiplyScalar(lineNormal0, offsetAwayFromBoundary);
              vtkMath::MultiplyScalar(lineNormal1, offsetAwayFromBoundary);

              if (k == 0)
              {
                double seed1[3], seed2[3];

                vtkMath::Subtract(p0, lineNormal0, seed1);
                vtkMath::Subtract(p1, lineNormal1, seed2);
                seedPoints->InsertNextPoint(seed1);
                seedPoints->InsertNextPoint(seed2);

                isBackward = false;
              }
              else
              {
                double seed1[3], seed2[3];

                vtkMath::Add(p0, lineNormal0, seed1);
                vtkMath::Add(p1, lineNormal1, seed2);
                seedPoints->InsertNextPoint(seed1);
                seedPoints->InsertNextPoint(seed2);

                isBackward = true;
              }
              vtkNew<vtkLine> line;
              line->GetPointIds()->SetId(0, 0);
              line->GetPointIds()->SetId(1, 1);

              seedCells->InsertNextCell(line);

              streamSurface->SetIntegrationDirection(isBackward);

              streamSurface->SetSourceData(seeds);
              streamSurface->Update();

              vtkNew<vtkAppendPolyData> appendFilter;
              appendFilter->AddInputData(separatrices);
              appendFilter->AddInputData(streamSurface->GetOutput());
              appendFilter->SetContainerAlgorithm(this);
              appendFilter->Update();

              separatrices->DeepCopy(appendFilter->GetOutput());
            }
          }
        }
        else
          boundarySwitchLines->DeleteCell(i);
      }
      else
        boundarySwitchLines->DeleteCell(i);
    }
    else
      boundarySwitchLines->DeleteCell(i);
  }

  boundarySwitchLines->RemoveDeletedCells();

  return 1;
}

//----------------------------------------------------------------------------
int vtkVectorFieldTopology::ComputeSeparatrices(vtkPolyData* criticalPoints,
  vtkPolyData* separatrices, vtkPolyData* surfaces, vtkDataSet* dataset, vtkPoints* interestPoints,
  int integrationStepUnit, double dist, double stepSize, int maxNumSteps, bool computeSurfaces,
  bool useIterativeSeeding)
{
  // adapt dist if cell unit was selected
  if (integrationStepUnit == vtkStreamTracer::CELL_LENGTH_UNIT)
  {
    dist *= sqrt(static_cast<double>(dataset->GetCell(0)->GetLength2()));
  }

  // Compute eigenvectors & eigenvalues
  vtkNew<vtkDoubleArray> criticalPointsTypes;
  criticalPointsTypes->SetNumberOfTuples(criticalPoints->GetNumberOfPoints());
  criticalPointsTypes->SetName("type");
  criticalPoints->GetPointData()->AddArray(criticalPointsTypes);

  vtkNew<vtkDoubleArray> criticalPointsTypesDetailed;
  criticalPointsTypesDetailed->SetNumberOfTuples(criticalPoints->GetNumberOfPoints());
  criticalPointsTypesDetailed->SetName("typeDetailed");
  criticalPoints->GetPointData()->AddArray(criticalPointsTypesDetailed);

  // this prevents that vtkPStreamTracer is called, which is necessary to prevent deadlocks
  vtkObjectFactory::SetAllEnableFlags(false, "vtkStreamTracer"); // this will need to be discussed
  vtkNew<vtkStreamTracer> streamTracer;
  streamTracer->SetInputData(dataset);
  streamTracer->SetIntegratorTypeToRungeKutta4();
  streamTracer->SetIntegrationStepUnit(this->IntegrationStepUnit);
  streamTracer->SetInitialIntegrationStep(this->IntegrationStepSize);
  streamTracer->SetComputeVorticity(false);
  streamTracer->SetMaximumNumberOfSteps(maxNumSteps);
  streamTracer->SetMaximumPropagation(dist * maxNumSteps);
  streamTracer->SetTerminalSpeed(epsilon);
  streamTracer->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, this->NameOfVectorArray);
  streamTracer->SetInterpolatorType(this->InterpolatorType);
  streamTracer->SetContainerAlgorithm(this);

  int numberOfSeparatingLines = 0;
  int numberOfSeparatingSurfaces = 0;

  for (int pointId = 0; pointId < criticalPoints->GetNumberOfPoints(); pointId++)
  {
    if (this->CheckAbort())
    {
      break;
    }
    // classification
    Eigen::Matrix<double, 3, 3> eigenMatrix;
    int flatIdx = 0;
    for (int i = 0; i < 3; i++)
    {
      for (int j = 0; j < 3; j++)
      {
        eigenMatrix(i, j) =
          criticalPoints->GetPointData()->GetArray("gradient")->GetTuple(pointId)[flatIdx];
        flatIdx++;
      }
    }

    Eigen::EigenSolver<Eigen::Matrix<double, 3, 3>> eigenS(eigenMatrix);

    int countComplex = 0;
    int countPos = 0;
    int countNeg = 0;
    for (int i = 0; i < this->Dimension; i++)
    {
      if (imag(eigenS.eigenvalues()[i]) != 0.0)
      {
        countComplex++;
      }

      // compare against epsilon for spiraling critical points only, otherwise compare to zero
      if (real(eigenS.eigenvalues()[i]) < -this->EpsilonCriticalPoint * countComplex / 2)
      {
        countNeg++;
      }
      else if (real(eigenS.eigenvalues()[i]) > this->EpsilonCriticalPoint * countComplex / 2)
      {
        countPos++;
      }
    }
    if (this->Dimension == 2)
    {
      criticalPoints->GetPointData()->GetArray("type")->SetTuple1(
        pointId, this->Classify2D(countComplex, countPos, countNeg));
      criticalPoints->GetPointData()
        ->GetArray("typeDetailed")
        ->SetTuple1(pointId, this->ClassifyDetailed2D(countComplex, countPos, countNeg));
    }
    else
    {
      criticalPoints->GetPointData()->GetArray("type")->SetTuple1(
        pointId, this->Classify3D(countComplex, countPos, countNeg));
      criticalPoints->GetPointData()
        ->GetArray("typeDetailed")
        ->SetTuple1(pointId, this->ClassifyDetailed3D(countComplex, countPos, countNeg));
    }

    // separatrix
    if (criticalPoints->GetPointData()->GetArray("type")->GetTuple1(pointId) == 1 ||
      (this->Dimension == 3 &&
        criticalPoints->GetPointData()->GetArray("type")->GetTuple1(pointId) == 2))
    {
      for (int i = 0; i < this->Dimension; i++)
      {

        double normal[] = { real(eigenS.eigenvectors().col(i)[0]),
          real(eigenS.eigenvectors().col(i)[1]), real(eigenS.eigenvectors().col(i)[2]) };

        bool isForward = real(eigenS.eigenvalues()[i]) > 0 && countPos == 1;
        bool isBackward = real(eigenS.eigenvalues()[i]) < 0 && countNeg == 1;
        if (isForward || isBackward)
        {
          // insert two seeds
          for (int k = 0; k < 2; k++)
          {
            // insert seed with small offset
            vtkNew<vtkPolyData> seeds;
            vtkNew<vtkPoints> seedPoints;
            vtkNew<vtkCellArray> seedCells;
            seeds->SetPoints(seedPoints);
            seeds->SetVerts(seedCells);

            seedPoints->InsertNextPoint(pow(-1, k) * dist * real(eigenS.eigenvectors().col(i)[0]) +
                criticalPoints->GetPoint(pointId)[0],
              pow(-1, k) * dist * real(eigenS.eigenvectors().col(i)[1]) +
                criticalPoints->GetPoint(pointId)[1],
              pow(-1, k) * dist * real(eigenS.eigenvectors().col(i)[2]) +
                criticalPoints->GetPoint(pointId)[2]);
            vtkNew<vtkVertex> vertex;
            vertex->GetPointIds()->SetId(0, seedPoints->GetNumberOfPoints() - 1);
            seedCells->InsertNextCell(vertex);

            // integrate
            streamTracer->SetIntegrationDirection(isBackward);
            streamTracer->SetSourceData(seeds);
            streamTracer->Update();

            if (streamTracer->GetOutput()->GetNumberOfPoints() > 0)
            {
              // close gap to the critical point at the beginning
              streamTracer->GetOutput()->GetPoints()->InsertNextPoint(
                criticalPoints->GetPoint(pointId));
              for (int j = 0; j < streamTracer->GetOutput()->GetPointData()->GetNumberOfArrays();
                   j++)
              {
                streamTracer->GetOutput()->GetPointData()->GetArray(j)->InsertNextTuple(
                  streamTracer->GetOutput()->GetPointData()->GetArray(j)->GetTuple(0));
              }

              // this polyline with the 2 new points will replace the old polyline
              vtkSmartPointer<vtkPolyLine> polyLine = vtkSmartPointer<vtkPolyLine>::New();
              polyLine->GetPointIds()->SetNumberOfIds(
                streamTracer->GetOutput()->GetNumberOfPoints());
              polyLine->GetPointIds()->SetId(0, streamTracer->GetOutput()->GetNumberOfPoints() - 1);
              for (int l = 1; l < streamTracer->GetOutput()->GetNumberOfPoints(); l++)
              {
                polyLine->GetPointIds()->SetId(l, l - 1);
              }

              // close gap to the critical point at the end
              int closestCriticalPointToEnd = 0;
              double closestDistance[3];
              vtkMath::Subtract(streamTracer->GetOutput()->GetPoint(
                                  streamTracer->GetOutput()->GetNumberOfPoints() - 2),
                interestPoints->GetPoint(closestCriticalPointToEnd), closestDistance);
              double currentDistance[3];

              // find closest critical point to endpoint
              for (int j = 0; j < interestPoints->GetNumberOfPoints(); j++)
              {
                vtkMath::Subtract(streamTracer->GetOutput()->GetPoint(
                                    streamTracer->GetOutput()->GetNumberOfPoints() - 2),
                  interestPoints->GetPoint(j), currentDistance);
                if (vtkMath::Norm(currentDistance) < vtkMath::Norm(closestDistance))
                {
                  closestCriticalPointToEnd = j;
                  for (int d = 0; d < 3; d++)
                  {
                    closestDistance[d] = currentDistance[d];
                  }
                }
              }

              if (vtkMath::Norm(closestDistance) < dist)
              {
                // find closest point on streamline to that critical point to avoid self
                // intersection
                int firstClosePointToCriticalPoint = 0;
                for (int j = 0; j < streamTracer->GetOutput()->GetNumberOfPoints(); j++)
                {
                  vtkMath::Subtract(streamTracer->GetOutput()->GetPoint(j),
                    interestPoints->GetPoint(closestCriticalPointToEnd), currentDistance);
                  if (vtkMath::Norm(currentDistance) < dist)
                  {
                    firstClosePointToCriticalPoint = j;
                    for (int d = 0; d < 3; d++)
                    {
                      closestDistance[d] = currentDistance[d];
                    }
                    break;
                  }
                }

                // insert new point
                streamTracer->GetOutput()->GetPoints()->InsertNextPoint(
                  interestPoints->GetPoint(closestCriticalPointToEnd));
                for (int j = 0; j < streamTracer->GetOutput()->GetPointData()->GetNumberOfArrays();
                     j++)
                {
                  streamTracer->GetOutput()->GetPointData()->GetArray(j)->InsertNextTuple(
                    streamTracer->GetOutput()->GetPointData()->GetArray(j)->GetTuple(0));
                }

                // remove superfluous lines in the tail and connect to critical point instead
                polyLine->GetPointIds()->SetNumberOfIds(firstClosePointToCriticalPoint + 2);
                polyLine->GetPointIds()->SetId(firstClosePointToCriticalPoint + 1,
                  streamTracer->GetOutput()->GetNumberOfPoints() - 1);
              }
              vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
              cells->InsertNextCell(polyLine);
              streamTracer->GetOutput()->SetLines(cells);

              // fill arrays
              vtkNew<vtkDoubleArray> iterationArray;
              iterationArray->SetName("iteration");
              iterationArray->SetNumberOfTuples(streamTracer->GetOutput()->GetNumberOfPoints());
              streamTracer->GetOutput()->GetPointData()->AddArray(iterationArray);
              vtkNew<vtkDoubleArray> indexArray;
              indexArray->SetName("index");
              indexArray->SetNumberOfTuples(streamTracer->GetOutput()->GetNumberOfPoints());
              streamTracer->GetOutput()->GetPointData()->AddArray(indexArray);
              for (int j = 0; j < streamTracer->GetOutput()->GetNumberOfPoints(); j++)
              {
                iterationArray->SetTuple1(j, j + 1);
                indexArray->SetTuple1(j, numberOfSeparatingLines);
              }
              // iterationArray->SetTuple1(streamTracer->GetOutput()->GetNumberOfPoints() - 1, 0);
              // the inserted points will get iteration 0
              iterationArray->SetTuple1(streamTracer->GetOutput()->GetNumberOfPoints() - 1, 0);
              if (vtkMath::Norm(closestDistance) < dist)
              {
                iterationArray->SetTuple1(streamTracer->GetOutput()->GetNumberOfPoints() - 2, 0);
              }

              // combine lines of this separatrix with existing ones
              vtkNew<vtkAppendPolyData> appendFilter;
              appendFilter->AddInputData(separatrices);
              appendFilter->AddInputData(streamTracer->GetOutput());
              appendFilter->SetContainerAlgorithm(this);
              appendFilter->Update();
              separatrices->DeepCopy(appendFilter->GetOutput());
              numberOfSeparatingLines++;
            }
          }
          if (computeSurfaces && this->Dimension == 3)
          {
            ComputeSurface(numberOfSeparatingSurfaces++, isForward, normal,
              criticalPoints->GetPoint(pointId), surfaces, dataset, integrationStepUnit, dist,
              stepSize, maxNumSteps, useIterativeSeeding);
          }
        }
      }
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkVectorFieldTopology::RemoveBoundary(vtkSmartPointer<vtkUnstructuredGrid> tridataset)
{
  // assign id to each point
  vtkNew<vtkIdFilter> idFilter;
  idFilter->SetInputData(tridataset);
  idFilter->SetPointIdsArrayName("ids");
  idFilter->Update();

  // extract surface
  vtkSmartPointer<vtkPolyData> boundary;
  if (this->Dimension == 2)
  {
    vtkNew<vtkGeometryFilter> geometryFilter;
    geometryFilter->SetInputData(idFilter->GetOutput());
    geometryFilter->SetContainerAlgorithm(this);
    geometryFilter->Update();

    vtkNew<vtkFeatureEdges> surfaceFilter;
    surfaceFilter->SetInputData(geometryFilter->GetOutput());
    surfaceFilter->SetContainerAlgorithm(this);
    surfaceFilter->Update();
    boundary = surfaceFilter->GetOutput();
  }
  else
  {
    vtkNew<vtkDataSetSurfaceFilter> surfaceFilter;
    surfaceFilter->SetInputData(idFilter->GetOutput());
    surfaceFilter->SetContainerAlgorithm(this);
    surfaceFilter->Update();
    boundary = surfaceFilter->GetOutput();
  }

  // mark all points whose ids appear in the surface
  vtkNew<vtkDoubleArray> isBoundary;
  isBoundary->SetNumberOfTuples(tridataset->GetNumberOfPoints());
  isBoundary->SetName("isBoundary");
  tridataset->GetPointData()->AddArray(isBoundary);
  for (int ptId = 0; ptId < tridataset->GetNumberOfPoints(); ptId++)
  {
    isBoundary->SetTuple1(ptId, 0);
  }
  for (int ptId = 0; ptId < boundary->GetNumberOfPoints(); ptId++)
  {
    isBoundary->SetTuple1(boundary->GetPointData()->GetArray("ids")->GetTuple1(ptId), 1);
  }

  // copy only cells that do not contain any point that is marked as boundary point
  vtkNew<vtkCellArray> cellsWithoutBoundary;
  for (int cellId = 0; cellId < tridataset->GetNumberOfCells(); cellId++)
  {
    auto cell = tridataset->GetCell(cellId);
    if ((this->Dimension == 2 && cell->GetCellType() != VTK_TRIANGLE) ||
      (this->Dimension == 3 && cell->GetCellType() != VTK_TETRA))
    {
      continue;
    }

    bool isBoundaryCell = false;
    for (int ptId = 0; ptId < cell->GetNumberOfPoints(); ptId++)
    {
      if (tridataset->GetPointData()->GetArray("isBoundary")->GetTuple1(cell->GetPointId(ptId)) ==
        1)
      {
        isBoundaryCell = true;
        break;
      }
    }
    if (!isBoundaryCell)
    {
      cellsWithoutBoundary->InsertNextCell(cell);
    }
  }

  // set copied cells as cells
  if (this->Dimension == 2)
  {
    tridataset->SetCells(VTK_TRIANGLE, cellsWithoutBoundary);
  }
  else
  {
    tridataset->SetCells(VTK_TETRA, cellsWithoutBoundary);
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkVectorFieldTopology::ImageDataPrepare(
  vtkDataSet* dataSetInput, vtkUnstructuredGrid* tridataset)
{
  // cast input to imagedata
  vtkImageData* dataset = vtkImageData::SafeDownCast(dataSetInput);
  this->Dimension = dataset->GetDataDimension();

  // these things are necessary for probe and the integrator to work properly in the 2D setting
  if (this->Dimension == 2)
  {
    dataset->SetSpacing(dataset->GetSpacing()[0], dataset->GetSpacing()[1], 1);
    dataset->SetOrigin(dataset->GetOrigin()[0], dataset->GetOrigin()[1], 0);
    for (int i = 0; i < dataset->GetNumberOfPoints(); ++i)
    {
      auto vector = dataset->GetPointData()->GetArray(this->NameOfVectorArray)->GetTuple(i);
      dataset->GetPointData()
        ->GetArray(this->NameOfVectorArray)
        ->SetTuple3(i, vector[0], vector[1], 0);
    }
  }

  // Triangulate the input data
  vtkNew<vtkDataSetTriangleFilter> triangulateFilter;
  triangulateFilter->SetInputData(dataset);
  triangulateFilter->SetContainerAlgorithm(this);
  triangulateFilter->Update();
  tridataset->DeepCopy(triangulateFilter->GetOutput());

  return 1;
}

//------------------------------------------------------------------------------
int vtkVectorFieldTopology::UnstructuredGridPrepare(
  vtkDataSet* dataSetInput, vtkUnstructuredGrid* tridataset)
{
  // cast input to vtkUnstructuredGrid
  vtkUnstructuredGrid* dataset = vtkUnstructuredGrid::SafeDownCast(dataSetInput);

  if (dataset->GetNumberOfCells() == 0)
  {
    return 1;
  }

  // find out dimension from cell types
  for (int cellId = 0; cellId < dataset->GetNumberOfCells(); cellId++)
  {
    if (dataset->GetCell(cellId)->GetCellType() >= VTK_TETRA)
    {
      this->Dimension = 3;
      break;
    }
  }

  // find out if data is triangulated otherwise triangulate
  tridataset->DeepCopy(dataset);
  bool isTriangulated = true;
  for (int cellId = 0; cellId < dataset->GetNumberOfCells(); cellId++)
  {
    if ((this->Dimension == 2 && tridataset->GetCell(cellId)->GetCellType() > VTK_TRIANGLE) ||
      (this->Dimension == 3 && dataset->GetCell(cellId)->GetCellType() > VTK_TETRA))
    {
      isTriangulated = false;
      break;
    }
  }
  if (!isTriangulated)
  {
    // Triangulate the input data
    vtkNew<vtkDataSetTriangleFilter> triangulateFilter;
    triangulateFilter->SetInputData(dataset);
    triangulateFilter->SetContainerAlgorithm(this);
    triangulateFilter->Update();
    tridataset->DeepCopy(triangulateFilter->GetOutput());
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkVectorFieldTopology::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Flags validation
  if (Validate() == 0)
    return 0;

  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo0 = outputVector->GetInformationObject(0);
  vtkInformation* outInfo1 = outputVector->GetInformationObject(1);
  vtkInformation* outInfo2 = outputVector->GetInformationObject(2);
  vtkInformation* outInfo3 = outputVector->GetInformationObject(3);
  vtkInformation* outInfo4 = outputVector->GetInformationObject(4);

  // get the input and make sure the input data has vector-valued data
  vtkDataSet* dataset = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  int vecType(0);
  vtkSmartPointer<vtkDataArray> vectors = this->GetInputArrayToProcess(0, dataset, vecType);

  if (!vectors)
  {
    if (this->GetInputArrayInformation(0)->Get(vtkDataObject::FIELD_NAME()) != nullptr &&
      dataset->GetPointData()->GetArray(
        this->GetInputArrayInformation(0)->Get(vtkDataObject::FIELD_NAME())) == nullptr)
      vtkWarningMacro("The array chosen via GetInputArrayToProcess was not found. The algorithm "
                      "tries to use vectors instead.");

    vectors = dataset->GetPointData()->GetVectors();

    if (!vectors)
    {
      bool vectorNotFound = true;
      for (int i = 0; i < dataset->GetPointData()->GetNumberOfArrays(); i++)
        if (dataset->GetPointData()->GetArray(i)->GetNumberOfComponents() == 3)
        {
          vectors = dataset->GetPointData()->GetArray(i);
          vectorNotFound = false;
          // program stops
          vtkErrorMacro("A possible vector found in point data.");
          break;
        }

      if (vectorNotFound)
      {
        vtkErrorMacro("The input field does not contain any vectors as pointdata.");
        return 0;
      }
    }
  }

  // save the name so that it does not need to call GetInputArrayToProcess many times
  this->NameOfVectorArray = vectors->GetName();

  // Users might set the name that belongs to an existing array that is not a vector array.
  if (dataset->GetPointData()->GetArray(this->NameOfVectorArray)->GetNumberOfComponents() != 3)
  {
    vtkErrorMacro("The array that corresponds to the name of vector array is not a vector array.");
    return 0;
  }

  // make output
  vtkPolyData* criticalPoints =
    vtkPolyData::SafeDownCast(outInfo0->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* separatingLines =
    vtkPolyData::SafeDownCast(outInfo1->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* separatingSurfaces =
    vtkPolyData::SafeDownCast(outInfo2->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* boundarySwitchPoints =
    vtkPolyData::SafeDownCast(outInfo3->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* boundarySwitchSeparatrix =
    vtkPolyData::SafeDownCast(outInfo4->Get(vtkDataObject::DATA_OBJECT()));

  // run appropriate function for input data type
  vtkNew<vtkUnstructuredGrid> tridataset;
  bool success;
  switch (dataset->GetDataObjectType())
  {
    case VTK_IMAGE_DATA:
    {
      success = this->ImageDataPrepare(dataset, tridataset);
      break;
    }
    case VTK_UNSTRUCTURED_GRID:
    {
      success = this->UnstructuredGridPrepare(dataset, tridataset);
      break;
    }
    default:
    {
      vtkErrorMacro("The input field must be vtkImageData or vtkUnstructuredGrid.");
      success = false;
    }
  }

  // remove boundary cells
  if (this->ExcludeBoundary)
  {
    this->RemoveBoundary(tridataset);
  }

  // Compute critical points
  vtkNew<vtkPoints> criticalPointsPoints;
  vtkNew<vtkCellArray> criticalPointsCells;
  vtkNew<vtkDoubleArray> criticalPointsGradients;
  criticalPointsGradients->SetName("gradient");
  criticalPointsGradients->SetNumberOfComponents(9);
  criticalPoints->SetPoints(criticalPointsPoints);
  criticalPoints->SetVerts(criticalPointsCells);
  criticalPoints->GetPointData()->AddArray(criticalPointsGradients);
  if (this->Dimension == 2)
  {
    this->ComputeCriticalPoints2D(criticalPoints, tridataset);
  }
  else
  {
    this->ComputeCriticalPoints3D(criticalPoints, tridataset);
  }

  vtkNew<vtkPoints> interestPoints;

  if (this->Dimension == 2)
  {
    for (int i = 0; i < criticalPoints->GetNumberOfPoints(); i++)
      interestPoints->InsertNextPoint(criticalPoints->GetPoint(i));

    if (this->UseBoundarySwitchPoints)
    {
      vtkNew<vtkPoints> boundarySwitchPointsPoints;
      vtkNew<vtkCellArray> boundarySwitchPointsCells;
      boundarySwitchPoints->SetPoints(boundarySwitchPointsPoints);
      boundarySwitchPoints->SetVerts(boundarySwitchPointsCells);

      ComputeBoundarySwitchPoints(boundarySwitchPoints, tridataset);

      for (int i = 0; i < boundarySwitchPointsPoints->GetNumberOfPoints(); i++)
      {
        interestPoints->InsertNextPoint(boundarySwitchPointsPoints->GetPoint(i));
      }

      ComputeSeparatricesBoundarySwitchPoints(boundarySwitchPoints, boundarySwitchSeparatrix,
        tridataset, interestPoints, this->IntegrationStepUnit, this->SeparatrixDistance,
        this->MaxNumSteps);
    }
  }
  else if (this->Dimension == 3)
  {
    for (int i = 0; i < criticalPoints->GetNumberOfPoints(); i++)
      interestPoints->InsertNextPoint(criticalPoints->GetPoint(i));
  }
  else
  {
    vtkErrorMacro("Dimension has to be either 2 or 3.\n");
    return 0;
  }

  // classify critical points and compute separatrices
  this->ComputeSeparatrices(criticalPoints, separatingLines, separatingSurfaces, tridataset,
    interestPoints, this->IntegrationStepUnit, this->SeparatrixDistance, this->IntegrationStepSize,
    this->MaxNumSteps, this->ComputeSurfaces, this->UseIterativeSeeding);

  if (this->UseBoundarySwitchPoints && this->Dimension == 3)
  {
    ComputeSeparatricesBoundarySwitchLines(boundarySwitchPoints, boundarySwitchSeparatrix,
      tridataset, this->IntegrationStepUnit, this->SeparatrixDistance, this->MaxNumSteps,
      this->ComputeSurfaces, this->UseIterativeSeeding);
  }

  return success;
}
VTK_ABI_NAMESPACE_END
