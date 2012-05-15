/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestInteractorStyleImagePropertyMultiplePropSliceFirst.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkActor2D.h"
#include "vtkInteractorStyleImage.h"
#include "vtkImageProperty.h"
#include "vtkImageData.h"
#include "vtkImageSliceMapper.h"
#include "vtkImageSlice.h"
#include "vtkPNGReader.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkTextMapper.h"

int TestInteractorStyleImagePropertyInternal(int argc, char *argv[], int sliceOrder)
{
  vtkSmartPointer<vtkPNGReader> reader =
    vtkSmartPointer<vtkPNGReader>::New();

  char *fileName = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/GreenCircle.png");
  reader->SetFileName(fileName);
  delete[] fileName;

  vtkSmartPointer<vtkImageSliceMapper> mapper =
    vtkSmartPointer<vtkImageSliceMapper>::New();
  mapper->SetInputConnection(reader->GetOutputPort());

  vtkSmartPointer<vtkImageSlice> imageSlice =
    vtkSmartPointer<vtkImageSlice>::New();
  imageSlice->SetMapper(mapper);

  vtkSmartPointer<vtkImageProperty> property =
    vtkSmartPointer<vtkImageProperty>::New();
  property->SetColorWindow(4000);
  property->SetColorLevel(2000);

  imageSlice->SetProperty(property);

  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer<vtkRenderer>::New();
  renderer->ResetCamera();

  vtkSmartPointer<vtkRenderWindow> renderWindow =
    vtkSmartPointer<vtkRenderWindow>::New();
  renderWindow->AddRenderer(renderer);

  vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();

  vtkSmartPointer<vtkTextMapper> text =
    vtkSmartPointer<vtkTextMapper>::New();
  text->SetInput("Text");

  vtkSmartPointer<vtkActor2D> textActor =
    vtkSmartPointer<vtkActor2D>::New();
  textActor->SetMapper(text);
  textActor->PickableOff();

  switch(sliceOrder)
    {
    case 0:
      //Adding the slice to the renderer before the other prop.
      renderer->AddViewProp(imageSlice);
      renderer->AddViewProp(textActor);
      break;

    case 1:
      //Only add the slice if there should not be a prop.
      renderer->AddViewProp(imageSlice);
      break;

    case 2:
      //Adding the slice to the renderer after the other prop.
      renderer->AddViewProp(textActor);
      renderer->AddViewProp(imageSlice);
      break;

    default:
      cerr << "Invalid sliceOrder parameter in TestInteractorStyleImagePropertyInternal." << std::endl;
      return 1;
    }

  vtkSmartPointer<vtkInteractorStyleImage> style =
    vtkSmartPointer<vtkInteractorStyleImage>::New();
  style->SetCurrentRenderer(renderer);

  renderWindowInteractor->SetInteractorStyle(style);
  renderWindowInteractor->SetRenderWindow(renderWindow);
  renderWindowInteractor->Initialize();

  //The StartWindowLevel event is not activated until the function
  //OnLeftButtonDown is called. Call it to force the event to trigger
  //the chain of methods to set the ImageProperty.
  style->OnLeftButtonDown();

  int retVal = ((style->GetCurrentImageProperty()) ? 1:0);

  if (retVal)
    {
    cerr << "TestInteractorStyleImagePropertyInternal failed with sliceOrder parameter " << sliceOrder << "." << std::endl;
    }

  //retVal is 0 if property not found. Returns 1 on failure, 0 on success.
  return !retVal;
}

int TestInteractorStyleImageProperty(int argc, char *argv[])
{
  bool res = true;

  // Tests with slice before prop.
  if (TestInteractorStyleImagePropertyInternal(argc, argv, 0) != 0)
    {
    res = false;
    }
  // Tests with no additional prop.
  if (TestInteractorStyleImagePropertyInternal(argc, argv, 1) != 0)
    {
    res = false;
    }
  // Tests with slice after prop.
  if (TestInteractorStyleImagePropertyInternal(argc, argv, 2) != 0)
    {
    res = false;
    }

  return (res ? EXIT_SUCCESS : EXIT_FAILURE);
}
