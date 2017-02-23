/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGL2PSExporterRaster.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"
#include "vtkGL2PSExporter.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCubeAxesActor2D.h"
#include "vtkConeSource.h"
#include "vtkLogoRepresentation.h"
#include "vtkNew.h"
#include "vtkPNGReader.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkProperty2D.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkTestingInteractor.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"

#include <string>

int TestGL2PSExporterRaster(int argc, char * argv[])
{
  vtkNew<vtkConeSource> coneSource;
  vtkNew<vtkPolyDataMapper> coneMapper;
  vtkNew<vtkActor> coneActor;
  coneSource->SetResolution(25);
  coneMapper->SetInputConnection(coneSource->GetOutputPort());
  coneActor->SetMapper(coneMapper.GetPointer());
  coneActor->GetProperty()->SetColor(0.5, 0.5, 1.0);

  vtkNew<vtkCubeAxesActor2D> axes;
  axes->SetInputConnection(coneSource->GetOutputPort());
  axes->SetFontFactor(2.0);
  axes->SetCornerOffset(0.0);
  axes->GetProperty()->SetColor(0.0, 0.0, 0.0);

  vtkNew<vtkTextActor> text1;
  text1->SetDisplayPosition(250, 435);
  text1->SetInput("Test\nmultiline\ntext"); // Won't render properly
  text1->GetTextProperty()->SetFontSize(18);
  text1->GetTextProperty()->SetFontFamilyToArial();
  text1->GetTextProperty()->SetJustificationToCentered();
  text1->GetTextProperty()->BoldOn();
  text1->GetTextProperty()->ItalicOn();
  text1->GetTextProperty()->SetColor(0.0, 0.0, 1.0);

  vtkNew<vtkTextActor> text2;
  text2->SetDisplayPosition(400, 250);
  text2->SetInput("Test rotated text");
  text2->GetTextProperty()->SetFontSize(22);
  text2->GetTextProperty()->SetFontFamilyToTimes();
  text2->GetTextProperty()->SetJustificationToCentered();
  text2->GetTextProperty()->SetVerticalJustificationToCentered();
  text2->GetTextProperty()->BoldOn();
  text2->GetTextProperty()->SetOrientation(45);
  text2->GetTextProperty()->SetColor(1.0, 0.0, 0.0);

  vtkNew<vtkTextActor> text3;
  text3->SetDisplayPosition(20, 40);
  text3->SetInput("Bag");
  text3->GetTextProperty()->SetFontSize(45);
  text3->GetTextProperty()->SetFontFamilyToCourier();
  text3->GetTextProperty()->SetJustificationToLeft();
  text3->GetTextProperty()->SetVerticalJustificationToBottom();
  text3->GetTextProperty()->BoldOn();
  text3->GetTextProperty()->SetOrientation(0);
  text3->GetTextProperty()->SetColor(0.2, 1.0, 0.2);

  vtkNew<vtkTextActor> text4;
  text4->SetDisplayPosition(120, 40);
  text4->SetInput("Bag");
  text4->GetTextProperty()->SetFontSize(45);
  text4->GetTextProperty()->SetFontFamilyToCourier();
  text4->GetTextProperty()->SetJustificationToLeft();
  text4->GetTextProperty()->SetVerticalJustificationToCentered();
  text4->GetTextProperty()->BoldOn();
  text4->GetTextProperty()->SetOrientation(0);
  text4->GetTextProperty()->SetColor(0.2, 1.0, 0.2);

  vtkNew<vtkTextActor> text5;
  text5->SetDisplayPosition(220, 40);
  text5->SetInput("Bag");
  text5->GetTextProperty()->SetFontSize(45);
  text5->GetTextProperty()->SetFontFamilyToCourier();
  text5->GetTextProperty()->SetJustificationToLeft();
  text5->GetTextProperty()->SetVerticalJustificationToTop();
  text5->GetTextProperty()->BoldOn();
  text5->GetTextProperty()->SetOrientation(0);
  text5->GetTextProperty()->SetColor(0.2, 1.0, 0.2);


  vtkNew<vtkRenderer> ren;
  axes->SetCamera(ren->GetActiveCamera());
  ren->AddActor(coneActor.GetPointer());
  ren->AddActor(axes.GetPointer());
  ren->AddActor(text1.GetPointer());
  ren->AddActor(text2.GetPointer());
  ren->AddActor(text3.GetPointer());
  ren->AddActor(text4.GetPointer());
  ren->AddActor(text5.GetPointer());
  ren->SetBackground(0.8, 0.8, 0.8);

  // logo
  char* fname = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/vtk-transparent.png");

  vtkNew<vtkPNGReader> reader;
  reader->SetFileName(fname);
  reader->Update();
  delete [] fname;

  vtkNew<vtkLogoRepresentation> logo;
  logo->SetImage(reader->GetOutput());
  logo->ProportionalResizeOn();
  logo->SetPosition(0.8, 0.0);
  logo->SetPosition2(0.1, 0.1);
  logo->GetImageProperty()->SetOpacity(0.8);
  logo->SetRenderer(ren.GetPointer());
  ren->AddActor(logo.GetPointer());



  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(ren.GetPointer());

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  vtkSmartPointer<vtkCamera> camera = ren->GetActiveCamera();
  ren->ResetCamera();
  camera->Azimuth(30);

  renWin->SetSize(500, 500);
  renWin->Render();

  vtkNew<vtkGL2PSExporter> exp;
  exp->SetRenderWindow(renWin.GetPointer());
  exp->SetFileFormatToPS();
  exp->CompressOff();
  exp->SetSortToBSP();
  exp->DrawBackgroundOn();
  exp->Write3DPropsAsRasterImageOn();

  std::string fileprefix = vtkTestingInteractor::TempDirectory +
      std::string("/TestGL2PSExporterRaster");

  exp->SetFilePrefix(fileprefix.c_str());
  exp->Write();

  exp->SetFileFormatToPDF();
  exp->Write();

  renWin->SetMultiSamples(0);
  renWin->GetInteractor()->Initialize();
  renWin->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
