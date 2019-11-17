/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSegY2DReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of vtkSegYReader
// .SECTION Description
//

#include "vtkSegYReader.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkDataSetMapper.h"
#include "vtkIdList.h"
#include "vtkNew.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStructuredGrid.h"
#include "vtkTestUtilities.h"

int TestSegY3DReader(int argc, char* argv[])
{
  // Basic visualisation.
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(300, 300);
  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // Read file name.
  char* fname;
  fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SegY/waha8.sgy");

  vtkNew<vtkColorTransferFunction> lut;
  lut->AddRGBPoint(-127, 0.23, 0.30, 0.75);
  lut->AddRGBPoint(0.0, 0.86, 0.86, 0.86);
  lut->AddRGBPoint(126, 0.70, 0.02, 0.15);

  vtkNew<vtkSegYReader> reader;
  vtkNew<vtkDataSetMapper> mapper;
  vtkNew<vtkActor> actor;

  reader->SetFileName(fname);
  reader->Update();
  delete[] fname;

  mapper->SetInputConnection(reader->GetOutputPort());
  mapper->SetLookupTable(lut);
  mapper->SetColorModeToMapScalars();

  actor->SetMapper(mapper);

  ren->AddActor(actor);
  ren->ResetCamera();
  ren->GetActiveCamera()->Azimuth(180);

  // interact with data
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);

  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
