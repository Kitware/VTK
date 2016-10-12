/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMultiTexturingGLSL.cxx

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
#include <vtkOpenGLHardwareSupport.h>
#include <vtkOpenGLRenderWindow.h>
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


#include <vtkTestUtilities.h>
#include <vtkRegressionTestImage.h>



int TestMultiTexturingGLSL(int argc, char *argv[])
{
  char shaders[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?> \
  <Material name=\"GenericAttributes1\"> \
    <Shader scope=\"Vertex\" name=\"VertexShader\" location=\"Inline\"\
      language=\"GLSL\" entry=\"main\"> attribute vec3 genAttrVector; \
      varying vec4 color; \
      void main(void) \
      { \
        gl_Position = gl_ModelViewProjectionMatrix *gl_Vertex; \
        color = vec4(normalize(genAttrVector), 1.0); \
      } \
    </Shader> \
    <Shader scope=\"Fragment\" name=\"FragmentShader\" location=\"Inline\" \
      language=\"GLSL\" entry=\"main\"> \
      varying vec4 color; \
      void main(void) \
      { \
        gl_FragColor = color; \
      } \
    </Shader> \
  </Material>";

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
  delete[] fname1;
  delete[] fname2;
  delete[] fname3;
  imageReaderRed->Update();
  imageReaderBlue->Update();
  imageReaderGreen->Update();

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

  TCoords->Delete();

  vtkTexture * textureRed =  vtkTexture::New();
  vtkTexture * textureBlue =  vtkTexture::New();
  vtkTexture * textureGreen =  vtkTexture::New();
  textureRed->SetInputConnection(imageReaderRed->GetOutputPort());
  textureBlue->SetInputConnection(imageReaderBlue->GetOutputPort());
  textureGreen->SetInputConnection(imageReaderGreen->GetOutputPort());

  // replace the fargments color and then accumulate the textures
  // RGBA values.
  textureRed->SetBlendingMode(vtkTexture::VTK_TEXTURE_BLENDING_MODE_REPLACE);
  textureBlue->SetBlendingMode(vtkTexture::VTK_TEXTURE_BLENDING_MODE_ADD);
  textureGreen->SetBlendingMode(vtkTexture::VTK_TEXTURE_BLENDING_MODE_ADD);

  vtkRenderer * renderer = vtkRenderer::New();
  vtkRenderWindow * renWin = vtkRenderWindow::New();

  vtkPolyDataMapper * mapper = vtkPolyDataMapper::New();
  mapper->SetInputData(polyData);

  vtkActor * actor = vtkActor::New();

  vtkOpenGLHardwareSupport * hardware =
    vtkOpenGLRenderWindow::SafeDownCast(renWin)->GetHardwareSupport();

  if(hardware->GetSupportsMultiTexturing() && hardware->GetNumberOfTextureUnits() > 2)
  {
    mapper->MapDataArrayToMultiTextureAttribute(
      vtkProperty::VTK_TEXTURE_UNIT_0, "MultTCoords", vtkDataObject::FIELD_ASSOCIATION_POINTS);
    mapper->MapDataArrayToMultiTextureAttribute(
      vtkProperty::VTK_TEXTURE_UNIT_1, "MultTCoords", vtkDataObject::FIELD_ASSOCIATION_POINTS);
    mapper->MapDataArrayToMultiTextureAttribute(
      vtkProperty::VTK_TEXTURE_UNIT_2, "MultTCoords", vtkDataObject::FIELD_ASSOCIATION_POINTS);

    actor->GetProperty()->SetTexture(vtkProperty::VTK_TEXTURE_UNIT_0,textureRed);
    actor->GetProperty()->SetTexture(vtkProperty::VTK_TEXTURE_UNIT_1,textureBlue);
    actor->GetProperty()->SetTexture(vtkProperty::VTK_TEXTURE_UNIT_2,textureGreen);
  }
  else
  {
    // no multitexturing just show the green texture.
    actor->SetTexture(textureGreen);
  }

  actor->SetMapper(mapper);

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

  polyData->Delete();
  mapper->Delete();
  actor->Delete();
  renWin->Delete();
  renderer->Delete();
  iren->Delete();
  imageReaderRed->Delete();
  imageReaderBlue->Delete();
  imageReaderGreen->Delete();
  textureRed->Delete();
  textureBlue->Delete();
  textureGreen->Delete();

  return !retVal;
}
