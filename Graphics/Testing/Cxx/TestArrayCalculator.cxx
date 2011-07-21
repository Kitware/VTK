/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAppendPolyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkArrayCalculator.h>
#include <vtkCompositeDataPipeline.h>
#include <vtkCellArray.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkTestUtilities.h>
#include <vtkXMLPolyDataReader.h>

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New ();

int TestArrayCalculator(int argc, char *argv[])
{
  vtkCompositeDataPipeline* prototype = vtkCompositeDataPipeline::New();
  vtkAlgorithm::SetDefaultExecutivePrototype(prototype);
  prototype->Delete();

  char* filename =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/disk_out_ref_surface.vtp");

  VTK_CREATE(vtkXMLPolyDataReader,reader);
  reader->SetFileName(filename);
  delete[] filename;
  reader->Update();

  //first calculators job is to create a property whose name could clash
  //with a function
  VTK_CREATE(vtkArrayCalculator, calc);
  calc->SetInputConnection( reader->GetOutputPort() );
  calc->SetAttributeModeToUsePointData();
  calc->AddScalarArrayName("Pres");
  calc->AddScalarArrayName("Temp");
  calc->SetFunction("Temp * Pres");
  calc->SetResultArrayName("norm");
  calc->Update();

  //now generate a vector with the second calculator
  VTK_CREATE(vtkArrayCalculator, calc2);
  calc2->SetInputConnection( calc->GetOutputPort() );
  calc2->SetAttributeModeToUsePointData();
  calc2->AddScalarArrayName("Pres");
  calc2->AddScalarArrayName("Temp");
  calc2->AddScalarArrayName("norm");
  calc2->SetFunction("(2 * (Temp*iHat + Pres*jHat + norm*kHat))/2.0");
  calc2->SetResultArrayName("PresVector");
  calc2->Update();

  //now make sure the calculator can use the vector
  //confirm that we don't use "Pres" array, but the "PresVector"
  VTK_CREATE(vtkArrayCalculator, calc3);
  calc3->SetInputConnection( calc2->GetOutputPort() );
  calc3->SetAttributeModeToUsePointData();
  calc3->AddScalarArrayName("Pres");
  calc3->AddVectorArrayName("PresVector");
  calc3->SetFunction("PresVector");
  calc3->SetResultArrayName("Result");
  calc3->Update();

  //verify the output is correct
  vtkPolyData *result = vtkPolyData::SafeDownCast( calc3->GetOutput() );
  int retCode = result->GetPointData()->HasArray("Result");
  vtkAlgorithm::SetDefaultExecutivePrototype(NULL);
  return !retCode;
}


