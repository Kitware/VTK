/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    MandelbrotVol.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  
Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

// Here I am trying to test the vtkBranchExtentTranslator wich will synchronize
// pieces requested from different branches of a pipeline.


#include "vtkBranchExtentTranslator.h"
#include "vtkImageMandelbrotSource.h"
#include "vtkSynchronizedTemplates3D.h"
#include "vtkImageClip.h"
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
  int myId, numProcs;
  float val;
  
  myId = controller->GetLocalProcessId();
  numProcs = controller->GetNumberOfProcesses();

  vtkImageMandelbrotSource *mandelbrot = vtkImageMandelbrotSource::New();
  mandelbrot->SetWholeExtent(-50, 50, -50, 50, 0, 90);
  mandelbrot->SetProjectionAxes(2, 3, 1);
  mandelbrot->SetSampleCX(0.025, 0.025, 0.025, 0.025);
  mandelbrot->SetOriginCX(0, 0, 0, 0);
  
  vtkBranchExtentTranslator *chunker = vtkBranchExtentTranslator::New();
  chunker->SetOriginalSource(mandelbrot->GetOutput());
  
  vtkSynchronizedTemplates3D *iso = vtkSynchronizedTemplates3D::New();
  iso->SetInput(mandelbrot->GetOutput());
  iso->ComputeScalarsOff();
  iso->SetValue(0, 5);
  iso->GetInput()->SetExtentTranslator(chunker);
  
  vtkPieceScalars *color = vtkPieceScalars::New();
  color->SetInput(iso->GetOutput());
  
  vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
  mapper->SetInput(color->GetOutput());
  mapper->SetScalarRange(0, numProcs-0.9);
  
  vtkActor *actor = vtkActor::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetAmbient(0.2);
  actor->GetProperty()->SetDiffuse(0.7);
  
  // Branch
  vtkImageClip *clip = vtkImageClip::New();
  clip->ClipDataOn();
  clip->SetInput(mandelbrot->GetOutput());
  clip->SetOutputWholeExtent(-50, 30, -50, 30, 0, 80);
  
  vtkSynchronizedTemplates3D *iso2 = vtkSynchronizedTemplates3D::New();
  iso2->SetInput(clip->GetOutput());
  iso2->ComputeScalarsOff();
  iso2->SetValue(0, 3);
  iso2->GetInput()->SetExtentTranslator(chunker);
  
  vtkPieceScalars *color2 = vtkPieceScalars::New();
  color2->SetInput(iso2->GetOutput());
  
  vtkPolyDataMapper *mapper2 = vtkPolyDataMapper::New();
  mapper2->SetInput(color2->GetOutput());
  mapper2->SetScalarRange(0, numProcs-0.9);
  
  vtkActor *actor2 = vtkActor::New();
  actor2->SetMapper(mapper2);
  
  
  
  vtkRenderer *ren = vtkRenderer::New();
  vtkRenderWindow *renWindow = vtkRenderWindow::New();
  renWindow->AddRenderer(ren);

  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWindow);

  ren->SetBackground(0.9, 0.9, 0.9);
  renWindow->SetSize( 500, 500);
  
  
  // assign our actor to the renderer
  ren->AddActor(actor);
  ren->AddActor(actor2);
  
  // The only thing we have to do to get parallel execution.
  vtkTreeComposite*  treeComp = vtkTreeComposite::New();
  treeComp->SetRenderWindow(renWindow);
  // Tell the mappers to only update a piece (based on process) of their inputs.
  treeComp->InitializePieces();
  treeComp->InitializeOffScreen();

  vtkCamera *cam = ren->GetActiveCamera();
  cam->SetViewUp(0, 0, 1);
  cam->SetPosition(3, 3, 6);
  ren->ResetCameraClippingRange();
  
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


