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
#include <vtkStreamSurface.h>

#include <vtkAppendPolyData.h>
#include <vtkCellArray.h>
#include <vtkDoubleArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkRuledSurfaceFilter.h>
#include <vtkStreamTracer.h>
#include <vtkTriangle.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkStreamSurface);

//----------------------------------------------------------------------------
vtkStreamSurface::vtkStreamSurface()
{
  this->RuledSurface->SetInputConnection(this->StreamTracer->GetOutputPort());
  this->RuledSurface->SetRuledModeToResample();
}

//----------------------------------------------------------------------------
vtkStreamSurface::~vtkStreamSurface() = default;

//----------------------------------------------------------------------------
void vtkStreamSurface::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UseIterativeSeeding =  " << this->UseIterativeSeeding << "\n";
  os << indent << "vtkRuledSurfaceFilter: \n";
  this->RuledSurface->PrintSelf(os, indent.GetNextIndent());
  os << indent << "vtkStreamTracer: \n";
  this->StreamTracer->PrintSelf(os, indent.GetNextIndent());
  os << indent << "vtkAppendPolyData: \n";
  this->AppendSurfaces->PrintSelf(os, indent.GetNextIndent());
}

//----------------------------------------------------------------------------
int vtkStreamSurface::AdvectIterative(vtkDataSet* field, vtkPolyData* seeds, vtkPolyData* output)
{
  // adapt dist if cell unit was selected
  double distThreshold = this->InitialIntegrationStep;
  if (this->IntegrationStepUnit == CELL_LENGTH_UNIT)
  {
    distThreshold *= sqrt(static_cast<double>(field->GetCell(0)->GetLength2()));
  }

  vtkNew<vtkPolyData> currentSeeds;
  currentSeeds->ShallowCopy(seeds);
  vtkNew<vtkDoubleArray> seedIntegrationTimeArray;
  seedIntegrationTimeArray->SetName("IntegrationTime");
  seedIntegrationTimeArray->SetNumberOfTuples(currentSeeds->GetNumberOfPoints());
  seedIntegrationTimeArray->Fill(0.0);
  currentSeeds->GetPointData()->AddArray(seedIntegrationTimeArray);

  for (int currentIteration = 0; currentIteration < this->MaximumNumberOfSteps; currentIteration++)
  {
    // advect currentSeeds
    // the output will be ordered: 0, advect(0), 1, advect(1), 2...
    // but if a point reaches the boundary, its advected point is just missing
    this->StreamTracer->SetInputData(field);
    this->StreamTracer->SetSourceData(currentSeeds);
    this->StreamTracer->SetIntegratorType(this->GetIntegratorType());
    this->StreamTracer->SetComputeVorticity(this->ComputeVorticity);
    this->StreamTracer->SetMaximumPropagation(this->MaximumPropagation);
    this->StreamTracer->SetIntegrationStepUnit(this->IntegrationStepUnit);
    this->StreamTracer->SetInitialIntegrationStep(this->InitialIntegrationStep);
    this->StreamTracer->SetIntegrationDirection(this->IntegrationDirection);
    // setting this to zero makes the tracer do 1 step
    this->StreamTracer->SetMaximumNumberOfSteps(0);
    this->StreamTracer->Update();

    if (StreamTracer->GetOutput()->GetNumberOfPoints() == 0)
    {
      return 1;
    }

    // fill in points that were not advected because they reached the boundary
    // i.e. copy a point k with integrationtime(k)==0 if its successor also has
    // integrationtime(k+1)==0
    vtkNew<vtkPolyData> orderedSurface;
    vtkNew<vtkPoints> orderedSurfacePoints;
    vtkNew<vtkCellArray> orderedSurfaceCells;
    orderedSurface->SetPoints(orderedSurfacePoints);
    orderedSurface->SetPolys(orderedSurfaceCells);

    vtkNew<vtkDoubleArray> integrationTimeArray;
    integrationTimeArray->SetName("IntegrationTime");
    orderedSurface->GetPointData()->AddArray(integrationTimeArray);

    int currentCircleIndex = -1;
    int numPts = this->StreamTracer->GetOutput()->GetNumberOfPoints() - 1;
    for (int k = 0; k < numPts; k++)
    {
      if (this->StreamTracer->GetOutput()
            ->GetPointData()
            ->GetArray("IntegrationTime")
            ->GetTuple1(k) == 0)
      {
        currentCircleIndex++;
      }
      orderedSurfacePoints->InsertNextPoint(this->StreamTracer->GetOutput()->GetPoint(k));
      integrationTimeArray->InsertNextTuple1(
        this->StreamTracer->GetOutput()->GetPointData()->GetArray("IntegrationTime")->GetTuple1(k) +
        currentSeeds->GetPointData()->GetArray("IntegrationTime")->GetTuple1(currentCircleIndex));

      if (this->StreamTracer->GetOutput()
            ->GetPointData()
            ->GetArray("IntegrationTime")
            ->GetTuple1(k) == 0)
      {
        if (this->StreamTracer->GetOutput()
              ->GetPointData()
              ->GetArray("IntegrationTime")
              ->GetTuple1(k + 1) == 0)
        {
          orderedSurfacePoints->InsertNextPoint(this->StreamTracer->GetOutput()->GetPoint(k));
          integrationTimeArray->InsertNextTuple1(currentSeeds->GetPointData()
                                                   ->GetArray("IntegrationTime")
                                                   ->GetTuple1(currentCircleIndex));
        }
      }
    }
    orderedSurfacePoints->InsertNextPoint(this->StreamTracer->GetOutput()->GetPoint(numPts));
    integrationTimeArray->InsertNextTuple1(this->StreamTracer->GetOutput()
                                             ->GetPointData()
                                             ->GetArray("IntegrationTime")
                                             ->GetTuple1(numPts) +
      currentSeeds->GetPointData()->GetArray("IntegrationTime")->GetTuple1(currentCircleIndex));
    if (this->StreamTracer->GetOutput()
          ->GetPointData()
          ->GetArray("IntegrationTime")
          ->GetTuple1(numPts) == 0)
    {
      orderedSurfacePoints->InsertNextPoint(this->StreamTracer->GetOutput()->GetPoint(numPts));
      integrationTimeArray->InsertNextTuple1(
        currentSeeds->GetPointData()->GetArray("IntegrationTime")->GetTuple1(currentCircleIndex));
    }

    // add arrays
    vtkNew<vtkDoubleArray> iterationArray;
    iterationArray->SetName("iteration");
    iterationArray->SetNumberOfTuples(orderedSurface->GetNumberOfPoints());
    iterationArray->Fill(currentIteration);
    orderedSurface->GetPointData()->AddArray(iterationArray);

    vtkNew<vtkDoubleArray> indexArray;
    indexArray->SetName("index");
    indexArray->SetNumberOfTuples(orderedSurface->GetNumberOfPoints());
    orderedSurface->GetPointData()->AddArray(indexArray);
    for (int k = 0; k < orderedSurface->GetNumberOfPoints(); k++)
    {
      indexArray->SetTuple1(k, k);
    }

    // insert cells
    for (int k = 0; k < orderedSurface->GetNumberOfPoints() - 2; k += 2)
    {
      if (std::abs(orderedSurface->GetPointData()->GetArray("IntegrationTime")->GetTuple1(k + 1) -
            orderedSurface->GetPointData()->GetArray("IntegrationTime")->GetTuple1(k)) > 1e-10 &&
        std::abs(orderedSurface->GetPointData()->GetArray("IntegrationTime")->GetTuple1(k + 3) -
          orderedSurface->GetPointData()->GetArray("IntegrationTime")->GetTuple1(k + 2)) > 1e-10)
      {
        double p0[3];
        orderedSurface->GetPoint(k, p0);
        double p1[3];
        orderedSurface->GetPoint(k + 1, p1);
        double p2[3];
        orderedSurface->GetPoint(k + 2, p2);
        double p3[3];
        orderedSurface->GetPoint(k + 3, p3);
        vtkNew<vtkTriangle> triangle1;
        vtkNew<vtkTriangle> triangle2;

        // make the triangles across the shorter diagonal
        if (sqrt(vtkMath::Distance2BetweenPoints(p0, p3)) >
          sqrt(vtkMath::Distance2BetweenPoints(p1, p2)))
        {
          triangle1->GetPointIds()->SetId(0, k);
          triangle1->GetPointIds()->SetId(1, k + 1);
          triangle1->GetPointIds()->SetId(2, k + 2);

          triangle2->GetPointIds()->SetId(0, k + 1);
          triangle2->GetPointIds()->SetId(1, k + 3);
          triangle2->GetPointIds()->SetId(2, k + 2);
        }
        else
        {
          triangle1->GetPointIds()->SetId(0, k);
          triangle1->GetPointIds()->SetId(1, k + 3);
          triangle1->GetPointIds()->SetId(2, k + 2);

          triangle2->GetPointIds()->SetId(0, k);
          triangle2->GetPointIds()->SetId(1, k + 1);
          triangle2->GetPointIds()->SetId(2, k + 3);
        }
        orderedSurfaceCells->InsertNextCell(triangle1);
        orderedSurfaceCells->InsertNextCell(triangle2);
      }
    }

    // adaptively insert new points where neighbors have diverged
    vtkNew<vtkPoints> newCirclePoints;
    currentSeeds->SetPoints(newCirclePoints);
    vtkNew<vtkDoubleArray> newIntegrationTimeArray;
    newIntegrationTimeArray->SetName("IntegrationTime");
    currentSeeds->GetPointData()->AddArray(newIntegrationTimeArray);
    for (int k = 0; k < orderedSurface->GetNumberOfPoints() - 2; k += 2)
    {
      newCirclePoints->InsertNextPoint(orderedSurface->GetPoint(k + 1));
      newIntegrationTimeArray->InsertNextTuple1(
        orderedSurface->GetPointData()->GetArray("IntegrationTime")->GetTuple1(k + 1));

      double p0[3];
      orderedSurface->GetPoint(k + 1, p0);
      double p1[3];
      orderedSurface->GetPoint(k + 3, p1);

      if (sqrt(vtkMath::Distance2BetweenPoints(p0, p1)) > distThreshold &&
        std::abs(orderedSurface->GetPointData()->GetArray("IntegrationTime")->GetTuple1(k + 1) -
          orderedSurface->GetPointData()->GetArray("IntegrationTime")->GetTuple1(k)) > 1e-10 &&
        std::abs(orderedSurface->GetPointData()->GetArray("IntegrationTime")->GetTuple1(k + 3) -
          orderedSurface->GetPointData()->GetArray("IntegrationTime")->GetTuple1(k + 2)) > 1e-10)
      {
        newCirclePoints->InsertNextPoint(
          (p0[0] + p1[0]) / 2, (p0[1] + p1[1]) / 2, (p0[2] + p1[2]) / 2);
        newIntegrationTimeArray->InsertNextTuple1(
          (orderedSurface->GetPointData()->GetArray("IntegrationTime")->GetTuple1(k + 1) +
            orderedSurface->GetPointData()->GetArray("IntegrationTime")->GetTuple1(k + 3)) /
          2);
      }
    }
    newCirclePoints->InsertNextPoint(
      orderedSurface->GetPoint(orderedSurface->GetNumberOfPoints() - 1));
    newIntegrationTimeArray->InsertNextTuple1(
      orderedSurface->GetPointData()
        ->GetArray("IntegrationTime")
        ->GetTuple1(orderedSurface->GetNumberOfPoints() - 1));

    // add current surface strip to the so far computed stream surface
    this->AppendSurfaces->RemoveAllInputs();
    this->AppendSurfaces->AddInputData(orderedSurface);
    this->AppendSurfaces->AddInputData(output);
    this->AppendSurfaces->Update();
    output->ShallowCopy(this->AppendSurfaces->GetOutput());

    // stop criterion if all points have left the boundary
    if (this->StreamTracer->GetOutput()
          ->GetPointData()
          ->GetArray("IntegrationTime")
          ->GetRange()[1 - this->IntegrationDirection] == 0)
    {
      vtkDebugMacro("Surface stagnates. All particles have left the boundary.");
      break;
    }
    if (currentSeeds == nullptr)
    {
      vtkErrorMacro("Circle is empty, output may not be correct.");
      return 0;
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkStreamSurface::AdvectSimple(vtkDataSet* field, vtkPolyData* seeds, vtkPolyData* output)
{
  //  this is for comparison with the standard ruled surface
  this->StreamTracer->SetInputData(field);
  this->StreamTracer->SetSourceData(seeds);
  this->StreamTracer->SetIntegratorType(this->GetIntegratorType());
  this->StreamTracer->SetComputeVorticity(this->ComputeVorticity);
  this->StreamTracer->SetMaximumPropagation(this->MaximumPropagation);
  this->StreamTracer->SetIntegrationStepUnit(this->IntegrationStepUnit);
  this->StreamTracer->SetInitialIntegrationStep(this->InitialIntegrationStep);
  this->StreamTracer->SetIntegrationDirection(this->IntegrationDirection);
  this->StreamTracer->SetMaximumNumberOfSteps(this->MaximumNumberOfSteps);

  this->RuledSurface->SetResolution(this->MaximumNumberOfSteps, 1);
  this->RuledSurface->Update();

  output->ShallowCopy(this->RuledSurface->GetOutput());

  return (output != nullptr);
}

//----------------------------------------------------------------------------
int vtkStreamSurface::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* fieldInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* seedsInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet* field = vtkDataSet::SafeDownCast(fieldInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* seeds = vtkPolyData::SafeDownCast(seedsInfo->Get(vtkDataObject::DATA_OBJECT()));

  // make output
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int finishedSuccessfully = 0;
  if (this->UseIterativeSeeding)
  {
    finishedSuccessfully = AdvectIterative(field, seeds, output);
  }
  else
  {
    finishedSuccessfully = AdvectSimple(field, seeds, output);
  }
  return finishedSuccessfully;
}
