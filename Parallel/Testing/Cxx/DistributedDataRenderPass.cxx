/*=========================================================================

  Program:   Visualization Toolkit
  Module:    DistributedDataRenderPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Test of vtkDistributedDataFilter and supporting classes, covering as much 
// code as possible.  This test requires 4 MPI processes.
//
// To cover ghost cell creation, use vtkDataSetSurfaceFilter.
//
// To cover clipping code:  SetBoundaryModeToSplitBoundaryCells()
//
// To run fast redistribution: SetUseMinimalMemoryOff() (Default)
// To run memory conserving code instead: SetUseMinimalMemoryOn()

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"
#include "vtkParallelFactory.h"
#include "vtkCompositeRenderManager.h"
#include "vtkDataSetReader.h"
#include "vtkUnstructuredGrid.h"
#include "vtkDistributedDataFilter.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkPieceScalars.h"
#include "vtkMPIController.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkActor.h"
#include "vtkPolyDataMapper.h"
#include "vtkCamera.h"
#include "vtkProperty.h"
#include "vtkOpaquePass.h"
#include "vtkTranslucentPass.h"
#include "vtkVolumetricPass.h"
#include "vtkOverlayPass.h"
#include "vtkLightsPass.h"
#include "vtkSequencePass.h"
#include "vtkRenderPassCollection.h"
#include "vtkCameraPass.h"
#include "vtkCompositeRGBAPass.h"
#include "vtkClearZPass.h"
#include "vtkImageRenderManager.h"
#include "vtkOpenGLRenderWindow.h"

/*
** This test only builds if MPI is in use
*/
#include "vtkMPICommunicator.h"

#include "vtkProcess.h"

class MyProcess : public vtkProcess
{
public:
  static MyProcess *New();
  vtkTypeMacro(MyProcess, vtkProcess);
  
  virtual void Execute();

  void SetArgs(int anArgc,
               char *anArgv[]);
  
protected:
  MyProcess();
  
  int Argc;
  char **Argv;
};

vtkStandardNewMacro(MyProcess);

MyProcess::MyProcess()
{
  this->Argc=0;
  this->Argv=0;
}

void MyProcess::SetArgs(int anArgc,
                        char *anArgv[])
{
  this->Argc=anArgc;
  this->Argv=anArgv;  
}

void MyProcess::Execute()
{
  this->ReturnValue=1;
  int numProcs=this->Controller->GetNumberOfProcesses();
  int me=this->Controller->GetLocalProcessId();
  
  int i, go;

//  vtkCompositeRenderManager *prm = vtkCompositeRenderManager::New();
//  vtkParallelRenderManager *prm = vtkParallelRenderManager::New();
  vtkImageRenderManager *prm = vtkImageRenderManager::New();
  
  vtkRenderWindowInteractor *iren=0;
  
  if(me==0)
    {
    iren=vtkRenderWindowInteractor::New();
    }
  
  // READER

  vtkDataSetReader *dsr = vtkDataSetReader::New();
  vtkUnstructuredGrid *ug = vtkUnstructuredGrid::New();

  vtkDataSet *ds = NULL;

  if (me == 0)
    {
    char* fname = 
      vtkTestUtilities::ExpandDataFileName(
        this->Argc, this->Argv, "Data/tetraMesh.vtk");

    dsr->SetFileName(fname);

    ds = dsr->GetOutput();

    dsr->Update();

    delete [] fname;

    go = 1;

    if ((ds == NULL) || (ds->GetNumberOfCells() == 0))
      {
      if (ds)
        {
        cout << "Failure: input file has no cells" << endl;
        }
      go = 0;
      }
    }
  else
    {
    ds = static_cast<vtkDataSet *>(ug);
    }

  vtkMPICommunicator *comm =
    vtkMPICommunicator::SafeDownCast(this->Controller->GetCommunicator());

  comm->Broadcast(&go, 1, 0);

  if (!go)
    {
    dsr->Delete();
    ug->Delete();
    prm->Delete();
    return;
    }

  // DATA DISTRIBUTION FILTER

  vtkDistributedDataFilter *dd = vtkDistributedDataFilter::New();

  dd->SetInput(ds);
  dd->SetController(this->Controller);

  dd->SetBoundaryModeToSplitBoundaryCells();  // clipping
  dd->UseMinimalMemoryOff();

  // COLOR BY PROCESS NUMBER

  vtkPieceScalars *ps = vtkPieceScalars::New();
  ps->SetInputConnection(dd->GetOutputPort());
  ps->SetScalarModeToCellData();

  // MORE FILTERING - this will request ghost cells

  vtkDataSetSurfaceFilter *dss = vtkDataSetSurfaceFilter::New();
  dss->SetInputConnection(ps->GetOutputPort());

  // COMPOSITE RENDER

  vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
  mapper->SetInputConnection(dss->GetOutputPort());

  mapper->SetColorModeToMapScalars();
  mapper->SetScalarModeToUseCellFieldData();
  mapper->SelectColorArray("Piece");
  mapper->SetScalarRange(0, numProcs-1);

  vtkActor *actor = vtkActor::New();
  actor->SetMapper(mapper);
  vtkProperty *p=actor->GetProperty();
  p->SetOpacity(0.3);

  vtkRenderer *renderer = prm->MakeRenderer();
  
#if 1
  // the rendering passes
  vtkCameraPass *cameraP=vtkCameraPass::New();
  
  vtkSequencePass *seq=vtkSequencePass::New();
  vtkOpaquePass *opaque=vtkOpaquePass::New();
//  vtkDepthPeelingPass *peeling=vtkDepthPeelingPass::New();
//  peeling->SetMaximumNumberOfPeels(200);
//  peeling->SetOcclusionRatio(0.1);
  
  vtkTranslucentPass *translucent=vtkTranslucentPass::New();
//  peeling->SetTranslucentPass(translucent);
  
  vtkVolumetricPass *volume=vtkVolumetricPass::New();
  vtkOverlayPass *overlay=vtkOverlayPass::New();
  
  vtkLightsPass *lights=vtkLightsPass::New();
  
  vtkClearZPass *clearZ=vtkClearZPass::New();
  clearZ->SetDepth(0.9);
  
  vtkCompositeRGBAPass *compositeRGBAPass=vtkCompositeRGBAPass::New();
  compositeRGBAPass->SetController(this->Controller);
  compositeRGBAPass->SetKdtree(dd->GetKdtree());
  vtkRenderPassCollection *passes=vtkRenderPassCollection::New();
  passes->AddItem(lights);
  passes->AddItem(opaque);
//  passes->AddItem(clearZ);
//  passes->AddItem(peeling);
   passes->AddItem(translucent);
  passes->AddItem(volume);
  passes->AddItem(overlay);
  passes->AddItem(compositeRGBAPass);
  seq->SetPasses(passes);
  cameraP->SetDelegatePass(seq);
  renderer->SetPass(cameraP);
  
  opaque->Delete();
//  peeling->Delete();
  translucent->Delete();
  volume->Delete();
  overlay->Delete();
  compositeRGBAPass->Delete();
  seq->Delete();
  passes->Delete();
  cameraP->Delete();
  lights->Delete();
  clearZ->Delete();
#endif
  
  
  renderer->AddActor(actor);

  vtkRenderWindow *renWin = prm->MakeRenderWindow();
  renWin->SetReportGraphicErrors(true);
  renWin->SetMultiSamples(0);
  renWin->SetAlphaBitPlanes(1);
  
  if(me==0)
    {
    iren->SetRenderWindow(renWin);
    }
  
  
  renWin->AddRenderer(renderer);

  renderer->SetBackground(0,0,0);
  renWin->SetSize(300,300);
  renWin->SetPosition(0, 360*me);

  prm->SetRenderWindow(renWin);
  prm->SetController(this->Controller);

  prm->InitializeOffScreen();   // Mesa GL only

  // We must update the whole pipeline here, otherwise node 0
  // goes into GetActiveCamera which updates the pipeline, putting
  // it into vtkDistributedDataFilter::Execute() which then hangs.
  // If it executes here, dd will be up-to-date won't have to 
  // execute in GetActiveCamera.

  mapper->SetPiece(me);
  mapper->SetNumberOfPieces(numProcs);
  mapper->Update();
  
  const int MY_RETURN_VALUE_MESSAGE=0x11;

  if (me == 0)
    {
    renderer->ResetCamera();
    vtkCamera *camera = renderer->GetActiveCamera();
    //camera->UpdateViewport(renderer);
    camera->ParallelProjectionOn();
    camera->SetParallelScale(16);

    renWin->Render();
    if(compositeRGBAPass->IsSupported(
         static_cast<vtkOpenGLRenderWindow *>(renWin)))
      {
      this->ReturnValue=vtkRegressionTester::Test(this->Argc,this->Argv,renWin,
                                                  10);
      }
    else
      {
      this->ReturnValue=vtkTesting::PASSED; // not supported.
      }
    
    if(this->ReturnValue==vtkRegressionTester::DO_INTERACTOR)
      {
      iren->Start();
      }
    prm->StopServices();
    for (i=1; i < numProcs; i++)
      {
      this->Controller->Send(&this->ReturnValue,1,i,MY_RETURN_VALUE_MESSAGE);
      }
    }
  else
    {
    prm->StartServices();
    this->Controller->Receive(&this->ReturnValue,1,0,MY_RETURN_VALUE_MESSAGE);
    }

  if (this->ReturnValue == vtkTesting::PASSED)
    {
    // Now try using the memory conserving *Lean methods.  The
    // image produced should be identical

    dd->UseMinimalMemoryOn();
    mapper->SetPiece(me);
    mapper->SetNumberOfPieces(numProcs);
    mapper->Update();

    if (me == 0)
      {
      renderer->ResetCamera();
      vtkCamera *camera = renderer->GetActiveCamera();
      camera->UpdateViewport(renderer);
      camera->ParallelProjectionOn();
      camera->SetParallelScale(16);
  
      renWin->Render();
      if(compositeRGBAPass->IsSupported(
           static_cast<vtkOpenGLRenderWindow *>(renWin)))
        {
        this->ReturnValue=vtkRegressionTester::Test(this->Argc,this->Argv,
                                                    renWin,10);
        }
      else
        {
        this->ReturnValue=vtkTesting::PASSED; // not supported.
        }
  
      for (i=1; i < numProcs; i++)
        {
        this->Controller->Send(&this->ReturnValue,1,i,MY_RETURN_VALUE_MESSAGE);
        }
  
      prm->StopServices();
      }
    else
      {
      prm->StartServices();
      this->Controller->Receive(&this->ReturnValue,1,0,
                                MY_RETURN_VALUE_MESSAGE);
      }
    }

  if(me==0)
    {
    iren->Delete();
    }
  
  // CLEAN UP 

  mapper->Delete();
  actor->Delete();
  renderer->Delete();
  renWin->Delete();

  dd->Delete();
  dsr->Delete();
  ug->Delete();

  ps->Delete();
  dss->Delete();

  prm->Delete();
}

int main(int argc, char **argv)
{
  int retVal = 1;

  vtkMPIController *contr = vtkMPIController::New();
  contr->Initialize(&argc, &argv);

  vtkMultiProcessController::SetGlobalController(contr);

  int numProcs = contr->GetNumberOfProcesses();
  int me = contr->GetLocalProcessId();

  if (numProcs < 2)
    {
    if (me == 0)
      {
      cout << "DistributedData test requires 2 processes" << endl;
      }
    contr->Delete();
    return retVal;
    }

  if (!contr->IsA("vtkMPIController"))
    {
    if (me == 0)
      {
      cout << "DistributedData test requires MPI" << endl;
      }
    contr->Delete();
    return retVal;   // is this the right error val?   TODO
    }
  
  MyProcess *p=MyProcess::New();
  p->SetArgs(argc,argv);
  contr->SetSingleProcessObject(p);
  contr->SingleMethodExecute();

  retVal=p->GetReturnValue();
  p->Delete();
  
  contr->Finalize();
  contr->Delete();

  return !retVal;
}

