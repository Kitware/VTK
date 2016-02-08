/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOspraySceneGraph.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test verifies that ospray scene graph classes are constructed OK
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "vtkOsprayViewNodeFactory.h"

#include "vtkActor.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkSphereSource.h"
#include "vtkViewNode.h"
#include "vtkViewNodeFactory.h"

int TestOspraySceneGraph( int argc, char *argv[] )
{
  vtkRenderWindow *rwin = vtkRenderWindow::New();
  vtkRenderer *ren = vtkRenderer::New();
  vtkActor *actor = vtkActor::New();

  vtkSphereSource *sphere = vtkSphereSource::New();
  vtkPolyDataMapper *pmap = vtkPolyDataMapper::New();
  pmap->SetInputConnection(sphere->GetOutputPort());
  actor->SetMapper(pmap);
  ren->AddActor(actor);
  rwin->AddRenderer(ren);

  vtkOsprayViewNodeFactory *vnf = vtkOsprayViewNodeFactory::New();
  vtkViewNode *vn = vnf->CreateNode(rwin);

  //HERE WE GO!
  vn->Build();
  vn->Synchronize();
  //vn->Render();

  vn->Delete();
  vnf->Delete();
  pmap->Delete();
  sphere->Delete();
  actor->Delete();
  ren->Delete();
  rwin->Delete();

  return 0;
}
