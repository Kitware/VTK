/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTextBoundingBox.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <iostream>
#include <vtkVersion.h>
#include <vtkSmartPointer.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkNew.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkTextRenderer.h>
#include <vtkPolyData.h>
#include <vtkSphereSource.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>

int TestTextBoundingBox(int, char *[])
{
  // Create a renderer
  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer<vtkRenderer>::New();
  renderer->SetBackground ( 1, 1, 1 ); // Set background color to white

  // Create a render window
  vtkSmartPointer<vtkRenderWindow> renderWindow =
    vtkSmartPointer<vtkRenderWindow>::New();
  renderWindow->AddRenderer ( renderer );

 // Create an interactor
  vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  renderWindowInteractor->SetRenderWindow ( renderWindow );

  const char* first = "no descenders";
  // Setup the text and add it to the renderer
  vtkSmartPointer<vtkTextActor> textActor =
    vtkSmartPointer<vtkTextActor>::New();
  textActor->SetInput (first);
  textActor->GetTextProperty()->SetFontSize ( 24 );
  textActor->GetTextProperty()->SetColor ( 1.0, 0.0, 0.0 );
  renderer->AddActor2D ( textActor );

  // get the bounding box
  double bbox[4];
  textActor->GetBoundingBox(renderer, bbox);

  // get the bounding box a different way
  vtkNew<vtkTextRenderer> tren;
  int tbox[4];
  tren->GetBoundingBox(textActor->GetTextProperty(),
                       std::string(first), tbox, renderWindow->GetDPI());

  // get the bounding box for a string with descenders
  // it should have the same height
  const char* second = "a couple of good descenders";
  int tbox2[4];
  tren->GetBoundingBox(textActor->GetTextProperty(),
                       std::string(second), tbox2, renderWindow->GetDPI());
  if (tbox[2] != tbox2[2] || tbox[3] != tbox2[3])
  {
    std::cout << "vtkTextRenderer height (" << first << "):\n"
              << tbox[2] << ", " << tbox[3] << std::endl;
    std::cout << "vtkTextRenderer height (" << second << "):\n"
              << tbox2[2] << ", " << tbox2[3] << std::endl;
    return EXIT_FAILURE;
  }



  if (bbox[0] == tbox[0] && bbox[1] == tbox[1] &&
      bbox[2] == tbox[2] && bbox[3] == tbox[3])
    return EXIT_SUCCESS;
  else
  {
    std::cout << "vtkTextActor GetBoundingBox:\n"
              << bbox[0] << ", " << bbox[1] << ", "
              << bbox[2] << ", " << bbox[3] << std::endl;
    std::cout << "vtkTextRenderer GetBoundingBox:\n"
              << tbox[0] << ", " << tbox[1] << ", "
              << tbox[2] << ", " << tbox[3] << std::endl;
    return EXIT_FAILURE;
  }
}
