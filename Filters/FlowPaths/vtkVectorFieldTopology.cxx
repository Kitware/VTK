/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <vtkVectorFieldTopology.h>

// VTK includes
#include <vtkAppendPolyData.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkCellTypes.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkDataSetTriangleFilter.h>
#include <vtkDoubleArray.h>
#include <vtkFeatureEdges.h>
#include <vtkGeometryFilter.h>
#include <vtkGradientFilter.h>
#include <vtkIdFilter.h>
#include <vtkImageData.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkIntersectionPolyDataFilter.h>
#include <vtkLine.h>
#include <vtkMath.h>
#include <vtkMatrix3x3.h>
#include <vtkNew.h>
#include <vtkPointData.h>
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
#include <vtkVectorFieldTopology.h>
#include <vtkVertex.h>

// Eigen3
#include <vtk_eigen.h>
#include VTK_EIGEN(Eigenvalues)

#include <cmath>
#include <map>

#define epsilon (1e-10)

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkVectorFieldTopology);

//----------------------------------------------------------------------------
vtkVectorFieldTopology::vtkVectorFieldTopology()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(3);
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
  os << indent << "ComputeSurfaces =  " << this->ComputeSurfaces << "\n";
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
  if (port < 3)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkVectorFieldTopology::Classify2D(
  int vtkNotUsed(countReal), int countComplex, int countPos, int countNeg)
{
  // make simple type that corresponds to the number of positive eigenvalues
  // SOURCE2D 2, SADDLE2D 1, SINK2D 0, (CENTER2D 3)
  // in analogy to ttk, where the type corresponds to the down directions
  int critType = vtkVectorFieldTopology::DEGENERATE2D;
  if (countPos + countNeg == 2)
  {
    switch (countPos)
    {
      case 0:
        critType = vtkVectorFieldTopology::SINK2D;
        break;
      case 1:
        critType = vtkVectorFieldTopology::SADDLE2D;
        break;
      case 2:
        critType = vtkVectorFieldTopology::SOURCE2D;
        break;
      default:
        break;
    }
  }
  if (countComplex == 2)
  {
    critType = vtkVectorFieldTopology::CENTER2D;
  }
  return critType;
}

//----------------------------------------------------------------------------
int vtkVectorFieldTopology::Classify3D(
  int vtkNotUsed(countReal), int countComplex, int countPos, int countNeg)
{
  // make simple type that corresponds to the number of positive eigenvalues
  // SOURCE3D 3, SADDLE23D 2, SADDLE13D 1, SINK3D 0, (CENTER3D 4)
  // in analogy to ttk, where the type corresponds to the down directions
  int critType = vtkVectorFieldTopology::DEGENERATE3D;
  if (countComplex > 0)
  {
    critType = vtkVectorFieldTopology::CENTER3D;
  }
  if (countPos + countNeg == 3)
  {
    switch (countPos)
    {
      case 0:
        critType = vtkVectorFieldTopology::SINK3D;
        break;
      case 1:
        critType = vtkVectorFieldTopology::SADDLE13D;
        break;
      case 2:
        critType = vtkVectorFieldTopology::SADDLE23D;
        break;
      case 3:
        critType = vtkVectorFieldTopology::SOURCE3D;
        break;
      default:
        break;
    }
  }
  return critType;
}

//----------------------------------------------------------------------------
int vtkVectorFieldTopology::ComputeCriticalPoints2D(
  vtkSmartPointer<vtkPolyData> criticalPoints, vtkSmartPointer<vtkUnstructuredGrid> tridataset)
{
  for (int cellId = 0; cellId < tridataset->GetNumberOfCells(); cellId++)
  {
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
    vtkVector3d values[3] = { vtkVector3d(
                                tridataset->GetPointData()->GetVectors()->GetTuple(indices[0])),
      vtkVector3d(tridataset->GetPointData()->GetVectors()->GetTuple(indices[1])),
      vtkVector3d(tridataset->GetPointData()->GetVectors()->GetTuple(indices[2])) };

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

      // barycentric corrdinates of the zero: lambda = f(T)^-1 (-values[0])
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
        bool isNewPoint = 1;
        for (int i = 0; i < criticalPoints->GetNumberOfPoints(); ++i)
        {
          if (vtkMath::Distance2BetweenPoints(zeroPos, criticalPoints->GetPoint(i)) < epsilon)
          {
            isNewPoint = 0;
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

    vtkVector3d values[4] = { vtkVector3d(
                                tridataset->GetPointData()->GetVectors()->GetTuple(indices[0])),
      vtkVector3d(tridataset->GetPointData()->GetVectors()->GetTuple(indices[1])),
      vtkVector3d(tridataset->GetPointData()->GetVectors()->GetTuple(indices[2])),
      vtkVector3d(tridataset->GetPointData()->GetVectors()->GetTuple(indices[3])) };

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
        bool isNewPoint = 1;
        for (int i = 0; i < criticalPoints->GetNumberOfPoints(); ++i)
        {
          if (vtkMath::Distance2BetweenPoints(zeroPos, criticalPoints->GetPoint(i)) < epsilon)
          {
            isNewPoint = 0;
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
int vtkVectorFieldTopology::ComputeSurface(int numberOfSeparatingSurfaces, bool isBackward,
  double normal[3], double zeroPos[3], vtkSmartPointer<vtkPolyData> streamSurfaces,
  vtkSmartPointer<vtkDataSet> dataset, int vtkNotUsed(integrationStepUnit), double dist,
  double vtkNotUsed(stepSize), int maxNumSteps, bool useIterativeSeeding)
{
  // generate circle and add first point again in the back to avoid gap
  vtkNew<vtkRegularPolygonSource> circle;
  circle->GeneratePolygonOff();
  circle->SetNumberOfSides(6);
  circle->SetRadius(dist);
  circle->SetCenter(zeroPos);
  circle->SetNormal(normal);
  circle->Update();

  // close circle exactly with a point instead of an edge to correctly treat points exiting the
  // boundary
  circle->GetOutput()->GetPoints()->InsertNextPoint(circle->GetOutput()->GetPoint(0));
  vtkNew<vtkPolyData> currentCircle;
  currentCircle->SetPoints(circle->GetOutput()->GetPoints());
  vtkNew<vtkDoubleArray> integrationTimeArray;
  integrationTimeArray->SetName("IntegrationTime");
  currentCircle->GetPointData()->AddArray(integrationTimeArray);
  for (int i = 0; i < currentCircle->GetNumberOfPoints(); ++i)
  {
    integrationTimeArray->InsertNextTuple1(0);
  }

  this->StreamSurface->SetInputData(0, dataset);
  this->StreamSurface->SetInputData(1, currentCircle);
  this->StreamSurface->SetUseIterativeSeeding(useIterativeSeeding);
  this->StreamSurface->SetIntegratorTypeToRungeKutta4();
  this->StreamSurface->SetIntegrationStepUnit(this->IntegrationStepUnit);
  this->StreamSurface->SetInitialIntegrationStep(this->IntegrationStepSize);
  this->StreamSurface->SetIntegrationDirection(isBackward);
  this->StreamSurface->SetComputeVorticity(0);
  this->StreamSurface->SetMaximumNumberOfSteps(maxNumSteps);
  this->StreamSurface->SetSourceData(currentCircle);
  this->StreamSurface->SetMaximumPropagation(dist * maxNumSteps);
  this->StreamSurface->Update();

  for (int i = 0; i < StreamSurface->GetOutput()->GetNumberOfPoints(); ++i)
  {
    StreamSurface->GetOutput()->GetPointData()->GetArray("index")->SetTuple1(
      i, numberOfSeparatingSurfaces);
  }

  // add current surface to existing surfaces
  vtkNew<vtkAppendPolyData> appendSurfaces;
  appendSurfaces->AddInputData(this->StreamSurface->GetOutput());
  appendSurfaces->AddInputData(streamSurfaces);
  appendSurfaces->Update();
  streamSurfaces->DeepCopy(appendSurfaces->GetOutput());

  return (streamSurfaces != nullptr);
}

//----------------------------------------------------------------------------
int vtkVectorFieldTopology::ComputeSeparatrices(vtkSmartPointer<vtkPolyData> criticalPoints,
  vtkSmartPointer<vtkPolyData> separatrices, vtkSmartPointer<vtkPolyData> surfaces,
  vtkSmartPointer<vtkDataSet> dataset, int integrationStepUnit, double dist, double stepSize,
  int maxNumSteps, bool computeSurfaces, bool useIterativeSeeding)
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

  vtkNew<vtkStreamTracer> streamTracer;
  streamTracer->SetInputData(dataset);
  streamTracer->SetIntegratorTypeToRungeKutta4();
  streamTracer->SetIntegrationStepUnit(this->IntegrationStepUnit);
  streamTracer->SetInitialIntegrationStep(this->IntegrationStepSize);
  streamTracer->SetComputeVorticity(0);
  streamTracer->SetMaximumNumberOfSteps(maxNumSteps);
  streamTracer->SetMaximumPropagation(dist * maxNumSteps);
  streamTracer->SetTerminalSpeed(epsilon);

  int numberOfSeparatingLines = 0;
  int numberOfSeparatingSurfaces = 0;

  for (int pointId = 0; pointId < criticalPoints->GetNumberOfPoints(); pointId++)
  {
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

    int countReal = 0;
    int countComplex = 0;
    int countPos = 0;
    int countNeg = 0;
    for (int i = 0; i < this->Dimension; i++)
    {
      if (imag(eigenS.eigenvalues()[i]) == 0.0)
      {
        countReal++;
      }
      else
      {
        countComplex++;
      }

      if (real(eigenS.eigenvalues()[i]) < -epsilon)
      {
        countNeg++;
      }
      else if (real(eigenS.eigenvalues()[i]) > epsilon)
      {
        countPos++;
      }
    }
    if (this->Dimension == 2)
    {
      criticalPoints->GetPointData()->GetArray("type")->SetTuple1(
        pointId, this->Classify2D(countReal, countComplex, countPos, countNeg));
    }
    else
    {
      criticalPoints->GetPointData()->GetArray("type")->SetTuple1(
        pointId, this->Classify3D(countReal, countComplex, countPos, countNeg));
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
                criticalPoints->GetPoint(closestCriticalPointToEnd), closestDistance);
              double currentDistance[3];

              // find closest critical point to endpoint
              for (int j = 0; j < criticalPoints->GetNumberOfPoints(); j++)
              {
                vtkMath::Subtract(streamTracer->GetOutput()->GetPoint(
                                    streamTracer->GetOutput()->GetNumberOfPoints() - 2),
                  criticalPoints->GetPoint(j), currentDistance);
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
                    criticalPoints->GetPoint(closestCriticalPointToEnd), currentDistance);
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
                  criticalPoints->GetPoint(closestCriticalPointToEnd));
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
              iterationArray->SetTuple1(streamTracer->GetOutput()->GetNumberOfPoints() - 1, 0);

              // combine lines of this separatrix with existing ones
              vtkNew<vtkAppendPolyData> appendFilter;
              appendFilter->AddInputData(separatrices);
              appendFilter->AddInputData(streamTracer->GetOutput());
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

  // probe arrays to output surfaces
  if (computeSurfaces && this->Dimension == 3)
  {
    vtkNew<vtkProbeFilter> probe;
    probe->SetInputData(surfaces);
    probe->SetSourceData(dataset);
    probe->Update();
    for (int i = 0; i < dataset->GetPointData()->GetNumberOfArrays(); ++i)
    {
      if (probe->GetOutput()->GetPointData()->GetArray(i)->GetNumberOfComponents() == 3)
      {
        surfaces->GetPointData()->SetVectors(probe->GetOutput()->GetPointData()->GetArray(i));
        break;
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
    geometryFilter->Update();

    vtkNew<vtkFeatureEdges> surfaceFilter;
    surfaceFilter->SetInputData(geometryFilter->GetOutput());
    surfaceFilter->Update();
    boundary = surfaceFilter->GetOutput();
  }
  else
  {
    vtkNew<vtkDataSetSurfaceFilter> surfaceFilter;
    surfaceFilter->SetInputData(idFilter->GetOutput());
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
      auto vector = dataset->GetPointData()->GetVectors()->GetTuple(i);
      dataset->GetPointData()->GetVectors()->SetTuple3(i, vector[0], vector[1], 0);
    }
  }

  // Triangulate the input data
  vtkNew<vtkDataSetTriangleFilter> triangulateFilter;
  triangulateFilter->SetInputData(dataset);
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

  // find out domension from cell types
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
    triangulateFilter->Update();
    tridataset->DeepCopy(triangulateFilter->GetOutput());
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkVectorFieldTopology::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo0 = outputVector->GetInformationObject(0);
  vtkInformation* outInfo1 = outputVector->GetInformationObject(1);
  vtkInformation* outInfo2 = outputVector->GetInformationObject(2);

  // get the input and make sure it has vector data
  vtkDataSet* dataset = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (dataset->GetPointData()->GetVectors() == nullptr)
  {
    for (int i = 0; i < dataset->GetPointData()->GetNumberOfArrays(); i++)
    {
      if (dataset->GetPointData()->GetArray(i)->GetNumberOfComponents() == 3)
      {
        dataset->GetPointData()->SetVectors(dataset->GetPointData()->GetArray(i));
        break;
      }
    }
  }
  if (dataset->GetPointData()->GetVectors() == nullptr)
  {
    vtkErrorMacro("The input field does not contain any vectors as pointdata.");
    return 0;
  }

  // make output
  vtkPolyData* criticalPoints =
    vtkPolyData::SafeDownCast(outInfo0->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* separatingLines =
    vtkPolyData::SafeDownCast(outInfo1->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* separatingSurfaces =
    vtkPolyData::SafeDownCast(outInfo2->Get(vtkDataObject::DATA_OBJECT()));

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
      success = 0;
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

  // classify critical points and compute separatrices
  this->ComputeSeparatrices(criticalPoints, separatingLines, separatingSurfaces, dataset,
    this->IntegrationStepUnit, this->SeparatrixDistance, this->IntegrationStepSize,
    this->MaxNumSteps, this->ComputeSurfaces, this->UseIterativeSeeding);

  return success;
}
