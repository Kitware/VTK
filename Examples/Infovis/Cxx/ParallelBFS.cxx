/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ParallelBFS.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <mpi.h>

#include "vtkEdgeListIterator.h"
#include "vtkGraphLayoutView.h"
#include "vtkInEdgeIterator.h"
#include "vtkInformation.h"
#include "vtkMPIController.h"
#include "vtkPBGLCollectGraph.h"
#include "vtkPBGLDistributedGraphHelper.h"
#include "vtkPBGLBreadthFirstSearch.h"
#include "vtkPBGLRandomGraphSource.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUndirectedGraph.h"
#include "vtkViewTheme.h"

int main(int argc, char** argv)
{
  MPI_Init(&argc, &argv);

  vtkSmartPointer<vtkPBGLRandomGraphSource> source =
    vtkSmartPointer<vtkPBGLRandomGraphSource>::New();
  source->DirectedOff();
  source->SetNumberOfVertices(100000);
  source->SetNumberOfEdges(10000);
  source->StartWithTreeOn();
  vtkSmartPointer<vtkPBGLBreadthFirstSearch> bfs =
    vtkSmartPointer<vtkPBGLBreadthFirstSearch>::New();
  bfs->SetInputConnection(source->GetOutputPort());
  vtkSmartPointer<vtkPBGLCollectGraph> collect =
    vtkSmartPointer<vtkPBGLCollectGraph>::New();
  collect->SetInputConnection(bfs->GetOutputPort());
  collect->UpdateInformation();

  // Setup pipeline request
  vtkStreamingDemandDrivenPipeline* exec =
    vtkStreamingDemandDrivenPipeline::SafeDownCast(collect->GetExecutive());
  vtkSmartPointer<vtkMPIController> controller =
    vtkSmartPointer<vtkMPIController>::New();
  controller->Initialize(&argc, &argv, 1);
  int rank = controller->GetLocalProcessId();
  int procs = controller->GetNumberOfProcesses();
  exec->SetUpdateNumberOfPieces(exec->GetOutputInformation(0), procs);
  exec->SetUpdatePiece(exec->GetOutputInformation(0), rank);
  collect->Update();

  if (rank == 0)
    {
    vtkSmartPointer<vtkUndirectedGraph> g =
      vtkSmartPointer<vtkUndirectedGraph>::New();
    g->ShallowCopy(collect->GetOutput());
    vtkSmartPointer<vtkGraphLayoutView> view =
      vtkSmartPointer<vtkGraphLayoutView>::New();
    vtkSmartPointer<vtkViewTheme> theme;
    theme.TakeReference(vtkViewTheme::CreateMellowTheme());
    view->ApplyViewTheme(theme);
    view->SetRepresentationFromInput(g);
    view->SetVertexColorArrayName("BFS");
    view->ColorVerticesOn();
    vtkRenderWindow* win = view->GetRenderWindow();
    view->Update();
    view->GetRenderer()->ResetCamera();
    win->GetInteractor()->Initialize();
    win->GetInteractor()->Start();
    }

  controller->Finalize();
  return 0;
}
