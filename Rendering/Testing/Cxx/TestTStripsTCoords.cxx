/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTStripsTCoords.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME Test of vtkGLSLShaderDeviceAdapter
// .SECTION Description
// this program tests the shader support in vtkRendering.

#include "vtkPlaneSource.h"
#include "vtkTriangleFilter.h"
#include "vtkStripper.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTexture.h"
#include "vtkImageData.h"
#include "vtkJPEGReader.h"
#include "vtkPointData.h"
#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

int TestTStripsTCoords(int argc, char *argv[])
{
  char* fname = vtkTestUtilities::ExpandDataFileName(
      argc, argv, "Data/beach.jpg");

  vtkJPEGReader *JPEGReader = vtkJPEGReader::New();
  JPEGReader->SetFileName(fname);
  JPEGReader->Update();

  delete [] fname;

  vtkTexture *texture = vtkTexture::New();
  texture->SetInputConnection(JPEGReader->GetOutputPort());
  JPEGReader->Delete();
  texture->InterpolateOn();

  vtkPlaneSource *planeSource = vtkPlaneSource::New();
  planeSource->Update();

  vtkTriangleFilter *triangleFilter = vtkTriangleFilter::New();
  triangleFilter->SetInputConnection(planeSource->GetOutputPort());
  planeSource->Delete();

  vtkStripper *stripper = vtkStripper::New();
  stripper->SetInputConnection(triangleFilter->GetOutputPort());
  triangleFilter->Delete();
  stripper->Update();

  vtkPolyData *polyData = stripper->GetOutput();
  polyData->Register(NULL);
  stripper->Delete();
  polyData->GetPointData()->SetNormals(NULL);

  vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
  mapper->SetInput(polyData);
  polyData->Delete();

  vtkActor *actor = vtkActor::New();
  actor->GetProperty()->SetTexture("texture",texture);
  texture->Delete();
  actor->SetMapper(mapper);
  mapper->Delete();

  vtkRenderer *renderer = vtkRenderer::New();
  renderer->AddActor(actor);
  actor->Delete();
  renderer->SetBackground(0.5,0.7,0.7);

  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(renderer);

  vtkRenderWindowInteractor *interactor = vtkRenderWindowInteractor::New();
  interactor->SetRenderWindow(renWin);

  renWin->SetSize(400,400);
  renWin->Render();
  interactor->Initialize();
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );
  if( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    interactor->Start();
    }

  renderer->Delete();
  renWin->Delete();
  interactor->Delete();

  return !retVal;
}
