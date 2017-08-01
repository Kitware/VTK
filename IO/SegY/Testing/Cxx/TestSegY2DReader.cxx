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
// .NAME Test of vtkSegY2DReader
// .SECTION Description
//

#include "vtkDebugLeaks.h"
#include "vtkSegY2DReader.h"

#include "vtkActor.h"
#include "vtkCamera.h"
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
  renWin->AddRenderer(ren.GetPointer());
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  // Read file name.
  char* fname[5];
  fname[0] =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SegY/lineA.sgy");
  fname[1] =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SegY/lineB.sgy");
  fname[2] =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SegY/lineC.sgy");
  fname[3] =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SegY/lineD.sgy");
  fname[4] =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SegY/lineE.sgy");

  vtkNew<vtkSegY2DReader> reader[5];
  vtkNew<vtkDataSetMapper> mapper[5];
  vtkNew<vtkActor> actor[5];

  for (int i = 0; i < 5; ++i)
  {
    reader[i]->SetFileName(fname[i]);
    reader[i]->Update();
    delete[] fname[i];

    mapper[i]->SetInputConnection(reader[i]->GetOutputPort());
    mapper[i]->ScalarVisibilityOn();

    actor[i]->SetMapper(mapper[i].GetPointer());

    ren->AddActor(actor[i].GetPointer());
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
