/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKitwareCutter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKitwareCutter.h"

#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkGridSynchronizedTemplates3D.h"
#include "vtkImplicitFunction.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkRectilinearSynchronizedTemplates.h"
#include "vtkStructuredGrid.h"
#include "vtkImageData.h"
#include "vtkSynchronizedTemplates2D.h"
#include "vtkSynchronizedTemplates3D.h"

vtkCxxRevisionMacro(vtkKitwareCutter, "1.6");
vtkStandardNewMacro(vtkKitwareCutter);

vtkKitwareCutter::vtkKitwareCutter()
{
}

vtkKitwareCutter::~vtkKitwareCutter()
{
}

int vtkKitwareCutter::RequestData(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (input->GetNumberOfCells() == 0)
    {
    return 1;
    }

  if (!this->CutFunction)
    {
    vtkErrorMacro("No cut function specified");
    return 0;
    }
  
  if (input->GetDataObjectType() == VTK_STRUCTURED_POINTS ||
      input->GetDataObjectType() == VTK_IMAGE_DATA)
    {    
    if ( input->GetCell(0) && input->GetCell(0)->GetCellDimension() >= 3 )
      {
      this->StructuredPointsCutter(input, output);
      return 1;
      }
    }
  
  if (input->GetDataObjectType() == VTK_STRUCTURED_GRID)
    {
    if (input->GetCell(0))
      {
      int dim = input->GetCell(0)->GetCellDimension();
      // only do 3D structured grids (to be extended in the future)
      if (dim >= 3)
        {
        this->StructuredGridCutter(input, output);
        }
      }
    }
  
  if (input->GetDataObjectType() == VTK_RECTILINEAR_GRID)
    {
    int dim = ((vtkRectilinearGrid*)input)->GetDataDimension();

    if ( dim == 3 ) 
      {
      this->RectilinearGridCutter(input, output);
      return 1;
      }
    }
  
  return this->Superclass::RequestData(request, inputVector, outputVector);
}

void vtkKitwareCutter::StructuredPointsCutter(vtkDataSet *dataSetInput,
                                              vtkPolyData *thisOutput)
{
  vtkImageData *input = vtkImageData::SafeDownCast(dataSetInput);
  vtkPolyData *output;
  vtkIdType numPts = input->GetNumberOfPoints();
  
  if (numPts < 1)
    {
    return;
    }

  vtkFloatArray *cutScalars = vtkFloatArray::New();
  cutScalars->SetNumberOfTuples(numPts);
  cutScalars->SetName("cutScalars");

  vtkImageData *contourData = vtkImageData::New();
  contourData->ShallowCopy(input);
  if (this->GenerateCutScalars)
    {
    contourData->GetPointData()->SetScalars(cutScalars);
    }
  else
    {
    contourData->GetPointData()->AddArray(cutScalars);
    }
  
  int i;
  double scalar;
  for (i = 0; i < numPts; i++)
    {
    scalar = this->CutFunction->FunctionValue(input->GetPoint(i));
    cutScalars->SetComponent(i, 0, scalar);
    }
  int numContours = this->GetNumberOfContours();
  
  vtkSynchronizedTemplates3D *contour = vtkSynchronizedTemplates3D::New();
  contour->SetInput(contourData);
  contour->SelectInputScalars("cutScalars");
  for (i = 0; i < numContours; i++)
    {
    contour->SetValue(i, this->GetValue(i));
    }
  contour->ComputeScalarsOff();
  contour->ComputeNormalsOff();
  contour->Update();
  output = contour->GetOutput();
  output->Register(this);  
  contour->Delete();

  thisOutput->CopyStructure(output);
  thisOutput->GetPointData()->ShallowCopy(output->GetPointData());
  thisOutput->GetCellData()->ShallowCopy(output->GetCellData());
  output->UnRegister(this);
  
  cutScalars->Delete();
  contourData->Delete();
}

void vtkKitwareCutter::StructuredGridCutter(vtkDataSet *dataSetInput,
                                            vtkPolyData *thisOutput)
{
  vtkStructuredGrid *input = vtkStructuredGrid::SafeDownCast(dataSetInput);
  vtkPolyData *output;
  vtkIdType numPts = input->GetNumberOfPoints();
  
  if (numPts < 1)
    {
    return;
    }
  
  vtkFloatArray *cutScalars = vtkFloatArray::New();
  cutScalars->SetNumberOfTuples(numPts);
  cutScalars->SetName("cutScalars");

  vtkStructuredGrid *contourData = vtkStructuredGrid::New();
  contourData->ShallowCopy(input);
  if (this->GenerateCutScalars)
    {
    contourData->GetPointData()->SetScalars(cutScalars);
    }
  else
    {
    contourData->GetPointData()->AddArray(cutScalars);
    }
  
  int i;
  double scalar;
  for (i = 0; i < numPts; i++)
    {
    scalar = this->CutFunction->FunctionValue(input->GetPoint(i));
    cutScalars->SetComponent(i, 0, scalar);
    }
  int numContours = this->GetNumberOfContours();
  
  vtkGridSynchronizedTemplates3D *contour =
    vtkGridSynchronizedTemplates3D::New();
  contour->SetInput(contourData);
  contour->SelectInputScalars("cutScalars");
  for (i = 0; i < numContours; i++)
    {
    contour->SetValue(i, this->GetValue(i));
    }
  contour->ComputeScalarsOff();
  contour->ComputeNormalsOff();
  contour->Update();
  output = contour->GetOutput();
  output->Register(this);
  contour->Delete();
  
  thisOutput->ShallowCopy(output);
  output->UnRegister(this);
  
  cutScalars->Delete();
  contourData->Delete();
}

void vtkKitwareCutter::RectilinearGridCutter(vtkDataSet *dataSetInput,
                                             vtkPolyData *thisOutput)
{
  vtkRectilinearGrid *input = vtkRectilinearGrid::SafeDownCast(dataSetInput);
  vtkPolyData *output;
  vtkIdType numPts = input->GetNumberOfPoints();
  
  if (numPts < 1)
    {
    return;
    }
  
  vtkFloatArray *cutScalars = vtkFloatArray::New();
  cutScalars->SetNumberOfTuples(numPts);
  cutScalars->SetName("cutScalars");

  vtkRectilinearGrid *contourData = vtkRectilinearGrid::New();
  contourData->ShallowCopy(input);
  if (this->GenerateCutScalars)
    {
    contourData->GetPointData()->SetScalars(cutScalars);
    }
  else
    {
    contourData->GetPointData()->AddArray(cutScalars);
    }
  
  int i;
  double scalar;
  for (i = 0; i < numPts; i++)
    {
    scalar = this->CutFunction->FunctionValue(input->GetPoint(i));
    cutScalars->SetComponent(i, 0, scalar);
    }
  int numContours = this->GetNumberOfContours();
  
  vtkRectilinearSynchronizedTemplates *contour =
    vtkRectilinearSynchronizedTemplates::New();
  contour->SetInput(contourData);
  contour->SelectInputScalars("cutScalars");
  for (i = 0; i < numContours; i++)
    {
    contour->SetValue(i, this->GetValue(i));
    }
  contour->ComputeScalarsOff();
  contour->ComputeNormalsOff();
  contour->Update();
  output = contour->GetOutput();
  output->Register(this);
  contour->Delete();
  
  thisOutput->ShallowCopy(output);
  output->UnRegister(this);
  
  cutScalars->Delete();
  contourData->Delete();
}

void vtkKitwareCutter::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
