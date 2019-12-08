/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGL2PSExporterMultipleRenderers.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkGL2PSExporter.h"
#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

#include "vtkActor.h"
#include "vtkNew.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestingInteractor.h"
#include "vtkTextActor.h"
#include "vtkTextMapper.h"

#include <string>

int TestGL2PSExporterMultipleRenderers(int, char*[])
{
  vtkNew<vtkTextActor> text1;
  text1->SetPosition(25, 25);
  text1->SetInput("String1");

  vtkNew<vtkTextActor> text2;
  text2->SetPosition(100, 100);
  text2->SetInput("String2");

  vtkNew<vtkTextMapper> textMap3;
  textMap3->SetInput("String3");
  vtkNew<vtkActor2D> text3;
  text3->SetMapper(textMap3);
  text3->SetPosition(75, 200);

  vtkNew<vtkRenderer> ren1;
  ren1->AddActor(text1);
  ren1->SetBackground(0.2, 0.2, 0.4);
  ren1->SetViewport(.5, 0, 1, 1);

  vtkNew<vtkRenderer> ren2;
  ren2->AddActor(text2);
  ren2->AddActor(text3);
  ren2->SetBackground(0.2, 0.2, 0.4);
  ren2->SetViewport(0, 0, .5, 1);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(ren1);
  renWin->AddRenderer(ren2);
  renWin->SetSize(500, 500);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renWin->Render();

  vtkNew<vtkGL2PSExporter> exp;
  exp->SetRenderWindow(renWin);
  exp->SetFileFormatToPS();
  exp->CompressOff();
  exp->SetSortToSimple();
  exp->DrawBackgroundOn();

  std::string fileprefix =
    vtkTestingInteractor::TempDirectory + std::string("/TestGL2PSExporterMultipleRenderers");

  exp->SetFilePrefix(fileprefix.c_str());
  exp->Write();

  exp->SetFileFormatToPDF();
  exp->Write();

  renWin->SetMultiSamples(0);
  renWin->GetInteractor()->Initialize();
  renWin->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
