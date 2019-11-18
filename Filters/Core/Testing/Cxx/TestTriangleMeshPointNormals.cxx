/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTriangleMeshPointNormals.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "vtkActor.h"
#include "vtkArrowSource.h"
#include "vtkCamera.h"
#include "vtkCleanPolyData.h"
#include "vtkGlyph3D.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkTriangleFilter.h"
#include "vtkTriangleMeshPointNormals.h"
#include "vtkXMLPolyDataReader.h"

int TestTriangleMeshPointNormals(int argc, char* argv[])
{
  char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/cow.vtp");
  std::cout << fileName << std::endl;

  // reader
  vtkNew<vtkXMLPolyDataReader> reader;
  reader->SetFileName(fileName);
  delete[] fileName;

  // triangle filter
  vtkNew<vtkTriangleFilter> triFilter;
  triFilter->SetInputConnection(reader->GetOutputPort());

  // cleaning filter
  vtkNew<vtkCleanPolyData> cleanFilter;
  cleanFilter->SetInputConnection(triFilter->GetOutputPort());

  // normals
  vtkNew<vtkTriangleMeshPointNormals> normFilter;
  normFilter->SetInputConnection(cleanFilter->GetOutputPort());

  // mapper, actor
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(normFilter->GetOutputPort());
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  // glyphs
  vtkNew<vtkArrowSource> glyphsource;
  vtkNew<vtkGlyph3D> glyph;
  glyph->SetInputConnection(normFilter->GetOutputPort());
  glyph->SetSourceConnection(glyphsource->GetOutputPort());
  glyph->SetVectorModeToUseNormal();
  glyph->SetColorModeToColorByVector();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.5);
  vtkNew<vtkPolyDataMapper> glyphmapper;
  glyphmapper->SetInputConnection(glyph->GetOutputPort());
  vtkNew<vtkActor> glyphactor;
  glyphactor->SetMapper(glyphmapper);

  // renderer
  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->AddActor(glyphactor);
  renderer->SetBackground(0.0, 0.0, 0.0);
  renderer->ResetCamera();

  // renderwindow, interactor
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  renWin->SetSize(300, 300);
  renWin->SetMultiSamples(0);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  iren->Initialize();
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    vtkNew<vtkInteractorStyleTrackballCamera> iStyle;
    iren->SetInteractorStyle(iStyle);
    renWin->SetSize(1000, 1000);
    iren->Start();
  }

  return !retVal;
}
