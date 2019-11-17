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

int TestSegY2DReader(int argc, char* argv[])
{
  // Basic visualisation.
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(300, 300);
  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // Read file name.
  char* fname[5];
  fname[0] = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SegY/lineA.sgy");
  fname[1] = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SegY/lineB.sgy");
  fname[2] = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SegY/lineC.sgy");
  fname[3] = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SegY/lineD.sgy");
  fname[4] = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SegY/lineE.sgy");

  vtkNew<vtkColorTransferFunction> lut;
  lut->AddRGBPoint(-6.4, 0.23, 0.30, 0.75);
  lut->AddRGBPoint(0.0, 0.86, 0.86, 0.86);
  lut->AddRGBPoint(6.6, 0.70, 0.02, 0.15);

  vtkNew<vtkSegYReader> reader[5];
  vtkNew<vtkDataSetMapper> mapper[5];
  vtkNew<vtkActor> actor[5];

  for (int i = 0; i < 5; ++i)
  {
    reader[i]->SetFileName(fname[i]);
    reader[i]->Update();
    delete[] fname[i];

    mapper[i]->SetInputConnection(reader[i]->GetOutputPort());
    mapper[i]->SetLookupTable(lut);
    mapper[i]->SetColorModeToMapScalars();

    actor[i]->SetMapper(mapper[i]);

    ren->AddActor(actor[i]);
    ren->ResetCamera();
  }

  ren->GetActiveCamera()->Azimuth(50);
  ren->GetActiveCamera()->Roll(50);
  ren->GetActiveCamera()->Zoom(1.2);

  // interact with data
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);

  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
