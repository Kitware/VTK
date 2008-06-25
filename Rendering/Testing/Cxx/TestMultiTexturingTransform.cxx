/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMultiTexturingTransform.cxx

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

#include <vtkActor.h>
#include <vtkCellArray.h>
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkImageData.h>
#include <vtkPlaneSource.h>
#include <vtkProperty.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPointSet.h>
#include <vtkPolyData.h>
#include <vtkPainterPolyDataMapper.h>
#include <vtkPNGReader.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkStripper.h>
#include <vtkTriangleFilter.h>
#include <vtkTexture.h>
#include <vtkTransform.h>


#include <vtkTestUtilities.h>
#include <vtkRegressionTestImage.h>

int TestMultiTexturingTransform(int argc, char *argv[])
{
  char* fname1 =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/RedCircle.png");
  char* fname2 =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/BlueCircle.png");
  char* fname3 =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/GreenCircle.png");

  vtkPNGReader * imageReaderRed = vtkPNGReader::New();
  vtkPNGReader * imageReaderBlue = vtkPNGReader::New();
  vtkPNGReader * imageReaderGreen = vtkPNGReader::New();

  imageReaderRed->SetFileName(fname1);
  imageReaderBlue->SetFileName(fname2);
  imageReaderGreen->SetFileName(fname3);
  imageReaderRed->Update();
  imageReaderBlue->Update();
  imageReaderGreen->Update();

  /*vtkPolyData * polyData = vtkPolyData::New();
  vtkPoints * points = vtkPoints::New();
  points->Allocate(16);

  points->InsertNextPoint(-1.0, -1.0, 0.0);
  points->InsertNextPoint(1.0, -1.0, 0.0);
  points->InsertNextPoint(1.0, 1.0, 0.0);
  points->InsertNextPoint(-1.0, 1.0, 0.0);

  polyData->SetPoints(points);

  vtkCellArray * cells = vtkCellArray::New();
  cells->Allocate(cells->EstimateSize(1, 4));
  cells->InsertNextCell(4);
  cells->InsertCellPoint(0);
  cells->InsertCellPoint(1);
  cells->InsertCellPoint(2);
  cells->InsertCellPoint(3);

  polyData->SetPolys(cells);*/


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


  vtkFloatArray *TCoords = vtkFloatArray::New();
  TCoords->SetNumberOfComponents(2);
  TCoords->Allocate(8);

  TCoords->InsertNextTuple2(0.0, 0.0);
  TCoords->InsertNextTuple2(0.0, 1.0);
  TCoords->InsertNextTuple2(1.0, 0.0);
  TCoords->InsertNextTuple2(1.0, 1.0);
  TCoords->SetName("MultTCoords");

  polyData->GetPointData()->AddArray(TCoords);

  vtkTexture * textureRed =  vtkTexture::New();
  vtkTexture * textureBlue =  vtkTexture::New();
  vtkTexture * textureGreen =  vtkTexture::New();
  textureRed->SetInputConnection(imageReaderRed->GetOutputPort());
  textureBlue->SetInputConnection(imageReaderBlue->GetOutputPort());
  textureGreen->SetInputConnection(imageReaderGreen->GetOutputPort());
  //textureRed->RepeatOff();
  //textureBlue->RepeatOff();
  //textureGreen->RepeatOff();

  textureRed->SetTextureUnit(VTK_TEXTURE_UNIT_0);
  textureBlue->SetTextureUnit(VTK_TEXTURE_UNIT_1);
  textureGreen->SetTextureUnit(VTK_TEXTURE_UNIT_2);

  // replace the fargments color and then accumulate the textures
  // RGBA values.
  textureRed->SetBlendingMode(VTK_TEXTURE_BLENDING_MODE_REPLACE);
  textureBlue->SetBlendingMode(VTK_TEXTURE_BLENDING_MODE_ADD);
  textureGreen->SetBlendingMode(VTK_TEXTURE_BLENDING_MODE_ADD);

  vtkTransform * transformRed = vtkTransform::New();
  vtkTransform * transformBlue = vtkTransform::New();
  vtkTransform * transformGreen = vtkTransform::New();

  transformRed->Translate(0.0, 0.125, 0.0);
  transformRed->Scale(2.0, 2.0, 0.0);
  transformBlue->Translate(0.5, 0.0, 0.0);
  
  textureRed->SetTransform(transformRed);
  textureBlue->SetTransform(transformBlue);
  textureGreen->SetTransform(transformGreen);

  vtkPolyDataMapper * mapper = vtkPolyDataMapper::New();
  mapper->SetInput(polyData);
  mapper->MapDataArrayToMultiTextureAttribute(
    VTK_TEXTURE_UNIT_0, "MultTCoords", vtkDataObject::FIELD_ASSOCIATION_POINTS);
  mapper->MapDataArrayToMultiTextureAttribute(
    VTK_TEXTURE_UNIT_1, "MultTCoords", vtkDataObject::FIELD_ASSOCIATION_POINTS);
  mapper->MapDataArrayToMultiTextureAttribute(
    VTK_TEXTURE_UNIT_2, "MultTCoords", vtkDataObject::FIELD_ASSOCIATION_POINTS);

  vtkActor * actor = vtkActor::New();
  actor->GetProperty()->SetTexture(VTK_TEXTURE_UNIT_0,textureRed);
  actor->GetProperty()->SetTexture(VTK_TEXTURE_UNIT_1,textureBlue);
  actor->GetProperty()->SetTexture(VTK_TEXTURE_UNIT_2,textureGreen);
  actor->SetMapper(mapper);

  vtkRenderer * renderer = vtkRenderer::New();
  vtkRenderWindow * renWin = vtkRenderWindow::New();
  renWin->SetSize(300, 300);
  renWin->AddRenderer(renderer);
  renderer->SetBackground(1.0, 0.5, 1.0);

  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

  renderer->AddActor(actor);
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  //points->Delete();
  //cells->Delete();
  polyData->Delete();
  imageReaderRed->Delete();
  imageReaderBlue->Delete();
  imageReaderGreen->Delete();

  return !retVal;
}
