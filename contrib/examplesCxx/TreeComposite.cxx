/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    TreeComposite.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  
Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkConeSource.h"
#include "vtkSphereSource.h"
#include "vtkPieceScalars.h"
#include "vtkMultiProcessController.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkMath.h"
#include "vtkTreeComposite.h"
#include <unistd.h>

void process(vtkMultiProcessController *controller, void *arg )
{
  vtkSphereSource *sphere;
  vtkConeSource *cone;
  vtkPieceScalars *color;
  vtkPolyDataMapper *mapper;
  vtkActor *actor;
  vtkCamera *cam;
  int myid, numProcs;
  float val;
  
  
  myid = controller->GetLocalProcessId();
  numProcs = controller->GetNumberOfProcesses();
    
  // Compute a different color for each process.
  sphere = vtkSphereSource::New();
  sphere->SetPhiResolution(40);
  sphere->SetThetaResolution(60);
  
  cone = vtkConeSource::New();
  cone->SetResolution(40);

  color = vtkPieceScalars::New();
  color->SetInput(sphere->GetOutput());
  //color->SetInput(cone->GetOutput());

  mapper = vtkPolyDataMapper::New();
  mapper->SetInput(color->GetOutput());
  mapper->SetScalarRange(0, 3);
  
  actor = vtkActor::New();
  actor->SetMapper(mapper);
  
  vtkRenderer *ren = vtkRenderer::New();
  vtkRenderWindow *renWindow = vtkRenderWindow::New();
  renWindow->AddRenderer(ren);

  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWindow);

  ren->SetBackground(0.9, 0.9, 0.9);
  renWindow->SetSize( 400, 400);
  
  
  // assign our actor to the renderer
  ren->AddActor(actor);
  
  // The only thing we have to do to get parallel execution.
  vtkTreeComposite*  treeComp = vtkTreeComposite::New();
  treeComp->SetRenderWindow(renWindow);
  // Tell the mappers to only update a piece (based on process) of their inputs.
  treeComp->InitializePieces();
  
  treeComp->InitializeOffScreen();
  
  
  //  Begin mouse interaction (for proc 0, others start rmi loop).
  iren->Start();
}


void main( int argc, char *argv[] )
{
  vtkMultiProcessController *controller;
  char save_filename[100]="\0";

    
  controller = vtkMultiProcessController::New();

  controller->Initialize(argc, argv);
  // Needed for threaded controller.
  // controller->SetNumberOfProcesses(2);
  controller->SetSingleMethod(process, save_filename);
  if (controller->IsA("vtkThreadedController"))
    {
    controller->SetNumberOfProcesses(8);
    } 
  controller->SingleMethodExecute();

  controller->Delete();  
}


