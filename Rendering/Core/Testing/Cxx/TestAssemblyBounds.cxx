/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAssemblyBounds.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkSmartPointer.h>

#include <vtkActor.h>
#include <vtkAssembly.h>
#include <vtkClipPolyData.h>
#include <vtkDataSetMapper.h>
#include <vtkPlane.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>

int TestAssemblyBounds (int , char *[])
{
  vtkSmartPointer<vtkSphereSource> source1 =
    vtkSmartPointer<vtkSphereSource>::New();
  source1->SetCenter(4,4,4);
  source1->SetRadius(2);

  vtkSmartPointer<vtkPlane> plane =
    vtkSmartPointer<vtkPlane>::New();
  plane->SetOrigin(6, 6, 6);
  plane->SetNormal(1, 1, 1);

  vtkSmartPointer<vtkClipPolyData> clipper1 =
    vtkSmartPointer<vtkClipPolyData>::New();
  clipper1->SetInputConnection(source1->GetOutputPort());
  clipper1->SetClipFunction(plane);
  clipper1->SetValue(0);

  vtkSmartPointer<vtkDataSetMapper> mapper1 =
    vtkSmartPointer<vtkDataSetMapper>::New();
  mapper1->SetInputConnection(clipper1->GetOutputPort());

  vtkSmartPointer<vtkActor> actor1 =
    vtkSmartPointer<vtkActor>::New();
  actor1->SetMapper(mapper1);

  vtkSmartPointer<vtkSphereSource> source2 =
    vtkSmartPointer<vtkSphereSource>::New();
  source2->SetCenter(8,8,8);
  source2->SetRadius(2);

  vtkSmartPointer<vtkClipPolyData> clipper2 =
    vtkSmartPointer<vtkClipPolyData>::New();
  clipper2->SetInputConnection(source2->GetOutputPort());
  clipper2->SetClipFunction(plane);
  clipper2->SetValue(0);

  vtkSmartPointer<vtkDataSetMapper> mapper2 =
    vtkSmartPointer<vtkDataSetMapper>::New();
  mapper2->SetInputConnection(clipper2->GetOutputPort());

  vtkSmartPointer<vtkActor> actor2 =
    vtkSmartPointer<vtkActor>::New();
  actor2->SetMapper(mapper2);

  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer<vtkRenderer>::New();

  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(renderer);

  // Create an assembly
  vtkSmartPointer<vtkAssembly> assembly =
    vtkSmartPointer<vtkAssembly>::New();
  assembly->AddPart(actor1);
  assembly->AddPart(actor2);

  renderer->AddActor(assembly);

  double clipActorBounds[6];
  actor1->GetBounds(clipActorBounds);

  std::cout << "First actor is entirely clipped, so its bounds are not valid"
            << std::endl;
  std::cout << "[" << clipActorBounds[0]
            << ", " << clipActorBounds[1] << " ]"
            << "[" << clipActorBounds[2]
            << ", " << clipActorBounds[3] << " ]"
            << "[" << clipActorBounds[4]
            << ", " << clipActorBounds[5] << "] " << std::endl;

  double clipActor2Bounds[6];
  actor2->GetBounds(clipActor2Bounds);

  std::cout << "Only the second sphere is visible with these bounds"
            << std::endl;
  std::cout << "[" << clipActor2Bounds[0]
            << ", " << clipActor2Bounds[1] << "] "
            << "[" << clipActor2Bounds[2]
            << ", " << clipActor2Bounds[3] << "] "
            << "[" << clipActor2Bounds[4]
            << ", " << clipActor2Bounds[5] << "] " << std::endl;

  double sceneBounds[6];
  renderer->ComputeVisiblePropBounds(sceneBounds);

  // Scene bounds should match the bounds of the second actor
  for (int i = 0; i < 6; ++i)
  {
    if (sceneBounds[i] != clipActor2Bounds[i])
    {
      std::cout << "Wrong visible bounds!" << std::endl;
      std::cout << "["  << sceneBounds[0]
                << ", " << sceneBounds[1]  << "] "
                << "["  << sceneBounds[2]
                << ", " << sceneBounds[3] << "] "
                << "["  << sceneBounds[4]
                << ", " << sceneBounds[5] << "] " << std::endl;
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}
