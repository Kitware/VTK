/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKitwareCutter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
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
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkRectilinearSynchronizedTemplates.h"
#include "vtkStructuredGrid.h"
#include "vtkImageData.h"
#include "vtkSynchronizedTemplates2D.h"
#include "vtkSynchronizedTemplates3D.h"

vtkCxxRevisionMacro(vtkKitwareCutter, "1.2");
vtkStandardNewMacro(vtkKitwareCutter);

vtkKitwareCutter::vtkKitwareCutter()
{
}

vtkKitwareCutter::~vtkKitwareCutter()
{
}

void vtkKitwareCutter::Execute()
{
  vtkDataSet *input = this->GetInput();

  if (!input)
    {
    vtkErrorMacro("No input specified");
    return;
    }
  
  if (input->GetNumberOfCells() == 0)
    {
    return;
    }

  if (!this->CutFunction)
    {
    vtkErrorMacro("No cut function specified");
    return;
    }
  
  if (input->GetDataObjectType() == VTK_STRUCTURED_POINTS ||
      input->GetDataObjectType() == VTK_IMAGE_DATA)
    {    
    if ( input->GetCell(0)->GetCellDimension() >= 3 )
      {
      this->StructuredPointsCutter();
      return;
      }
    }
  
  if (input->GetDataObjectType() == VTK_STRUCTURED_GRID)
    {
    int dim = input->GetCell(0)->GetCellDimension();
    // only do 3D structured grids (to be extended in the future)
    if (dim >= 3)
      {
      this->StructuredGridCutter();
      }
    }
  
  if (input->GetDataObjectType() == VTK_RECTILINEAR_GRID)
    {
    int dim = ((vtkRectilinearGrid*)input)->GetDataDimension();

    if ( dim == 3 ) 
      {
      this->RectilinearGridCutter();
      return;
      }
    }
  
  this->Superclass::Execute();
}

void vtkKitwareCutter::StructuredPointsCutter()
{
  vtkImageData *input = vtkImageData::SafeDownCast(this->GetInput());
  vtkPolyData *output;
  vtkPolyData *thisOutput = this->GetOutput();
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
  float scalar;
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

void vtkKitwareCutter::StructuredGridCutter()
{
  vtkStructuredGrid *input =
    vtkStructuredGrid::SafeDownCast(this->GetInput());
  vtkPolyData *output;
  vtkPolyData *thisOutput = this->GetOutput();
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
  float scalar;
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

void vtkKitwareCutter::RectilinearGridCutter()
{
  vtkRectilinearGrid *input =
    vtkRectilinearGrid::SafeDownCast(this->GetInput());
  vtkPolyData *output;
  vtkPolyData *thisOutput = this->GetOutput();
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
  float scalar;
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
