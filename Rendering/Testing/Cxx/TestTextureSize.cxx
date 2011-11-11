/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOpenGLPolyDataMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME Test different sizes with vtkTexture
// .SECTION Description
// This program tests 1D and 2D texture sizes.

#include <vtkCellArray.h>
#include <vtkFloatArray.h>
#include <vtkImageData.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper2D.h>
#include <vtkPNGWriter.h>
#include <vtkSmartPointer.h>
#include <vtkTexture.h>
#include <vtkTexturedActor2D.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>

vtkImageData* createTexture2D(int width, int height, int comp)
{
  void* data = malloc(width*height*comp*sizeof(unsigned char));
  if (!data)
    {
    return 0;
    }
  free(data);
  vtkImageData* image = vtkImageData::New();
  image->SetExtent(0, width - 1,
                   0, height - 1,
                   0, 0);
  image->SetNumberOfScalarComponents(comp);
  image->SetScalarTypeToUnsignedChar();
  image->AllocateScalars();

  unsigned char* ptr =
    reinterpret_cast<unsigned char*>(image->GetScalarPointer(0,0,0));
  double value = 0.;
  double valueIncr = 255. / (width*height > 1 ? width*height-1 : 1);
  for (int y = 0; y < height; ++y)
    {
    for (int x = 0; x < width; ++x)
      {
      for (int c = 0; c < comp; ++c)
        {
        *ptr++ = static_cast<char>(value);
        }
      value += valueIncr;
      }
    }
  return image;
}

int TestTextureSize(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  //Create a renderer, render window, and interactor
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer.GetPointer());
  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renderWindow.GetPointer());

  vtkNew<vtkPoints> points;
  points->InsertPoint(0, 0., 0., 0.);
  points->InsertPoint(1, 200., 0., 0.);
  points->InsertPoint(2, 200., 200., 0.);
  points->InsertPoint(3, 0., 200., 0.);

  vtkNew<vtkCellArray> cells;
  cells->InsertNextCell(4);
  cells->InsertCellPoint(0);
  cells->InsertCellPoint(1);
  cells->InsertCellPoint(2);
  cells->InsertCellPoint(3);

  vtkNew<vtkFloatArray> tcoords;
  tcoords->SetNumberOfComponents(2);
  tcoords->InsertNextTuple2(0.f, 0.f);
  tcoords->InsertNextTuple2(1.f, 0.f);
  tcoords->InsertNextTuple2(1.f, 1.f);
  tcoords->InsertNextTuple2(0.f, 1.f);

  vtkNew<vtkPolyData> textureCoords;
  textureCoords->SetPoints(points.GetPointer());
  textureCoords->SetPolys(cells.GetPointer());
  textureCoords->GetPointData()->SetTCoords(tcoords.GetPointer());

  vtkNew<vtkPolyDataMapper2D> polyDataMapper; 
  polyDataMapper->SetInput( textureCoords.GetPointer() );

  int textureSizes[23][2] =
    {{1,2}, {1,3}, {1,4}, {1,5}, {1,255}, {1,256}, {257,1}, 
     {2,1}, {3,1}, {4,1}, {5,1}, {255,1}, {256,1}, {257,1},
     {1,1}, {2,2}, {3,3}, {3,3}, {255,255}, {256,256}, {257,257},
     {2047,2047}, {4097,4097}};
  int componentSizes[3] = {1, 3, 4};
  for (int i = 0; i < 23; ++i)
    {
    for (int c = 0; c < 3; ++c)
      {
      int* size = textureSizes[i];
      vtkSmartPointer<vtkImageData> image = 
        vtkSmartPointer<vtkImageData>::Take(
          createTexture2D(size[0], size[1], componentSizes[c]));
      if (image.GetPointer() == 0)
        {
        return EXIT_SUCCESS;
        }
      vtkNew<vtkTexture> texture;
      texture->SetInput(image);
      // You can play with the parameters
      //texture->SetRepeat(false);
      //texture->SetEdgeClamp(true);
      //texture->SetInterpolate(true);

      vtkNew<vtkTexturedActor2D> textureActor;
      textureActor->SetTexture(texture.GetPointer());
      textureActor->SetMapper(polyDataMapper.GetPointer());
      renderer->AddActor(textureActor.GetPointer());

      texture->SetRestrictPowerOf2ImageSmaller(false);
      renderWindow->Render();

      texture->SetRestrictPowerOf2ImageSmaller(true);
      renderWindow->Render();
      }
    }

  return EXIT_SUCCESS;
}
