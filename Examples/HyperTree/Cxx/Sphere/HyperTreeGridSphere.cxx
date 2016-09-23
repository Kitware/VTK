/*=========================================================================

Copyright (c) Kitware Inc.
All rights reserved.

=========================================================================*/
// .SECTION Description
// This program illustrates the use of the vtkHyperTreeGrid
// data set and various filters acting upon hyper it.
// It generates output files in VTK format.
//
// .SECTION Usage
//
// .SECTION Thanks
// This program was written by Daniel Aguilera and Philippe Pebay
// This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)

#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyData.h>
#include <vtkActor.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkUnstructuredGrid.h>
#include <vtkShrinkFilter.h>
#include <vtkProperty.h>
#include <vtkUnstructuredGridWriter.h>
#include <vtkInteractorStyleSwitch.h>

#include "Mesh.h"
#include "Cell.h"
#include "Node.h"

using namespace std;

#define SHIFT_ARGS() for (int j=i;j<(argc-1);j++) argv[j] = argv[j+1]; argc--; i--
#define SHIFT_NARGS(n) for (int j=i;j<(argc-(n));j++) argv[j] = argv[j+(n)]; argc-=(n); i--

void usage ()
{
  cout << "Usage : amr [-level <int>] [-refine <int>] [-nx <int>] [-ny <int>] [-nz <int>] [-write <file>] [-shrink] [-help]" << endl;
  cout << "   -depth  : Number of refinement levels. Defaut = 3" << endl;
  cout << "   -factor : Refinement branching factor. Defaut = 3" << endl;
  cout << "   -n[xyz] : Number of grid points in each direction. Defaut = 5" << endl;
  cout << "   -write  : Output mesh in a VTK unstructured grid file. Defaut = no output" << endl;
  cout << "   -shrink : Apply shrink filter before rendering geometry. Defaut = do not shrink" << endl;
  cout << "   -help   : Print available options" << endl;
  exit (0);
}

int main( int argc, char *argv[] )
{
  // Default values
  int nx = 5;
  int ny = 5;

  int nz = 5;
  int depth = 3;
  int factor = 3;
  bool shrink = false;
  string datafile = "";
  double R = 0.0;

  for (int i = 1; i < argc; i++)
  {
    // Refinement depth
    if (strcmp (argv[i], "-depth") == 0)
    {
      if (i+1 < argc) {depth = atoi (argv[i+1]); SHIFT_NARGS(2);}
      else usage();
    }
    // Branch factor
    else if (strcmp (argv[i], "-factor") == 0)
    {
      if (i+1 < argc) {factor = atoi (argv[i+1]); SHIFT_NARGS(2);}
      else usage();
    }
    // Dimensions
    else if (strcmp (argv[i], "-nx") == 0)
    {
      if (i+1 < argc) {nx = atoi (argv[i+1]); SHIFT_NARGS(2);}
      else usage();
    }
    else if (strcmp (argv[i], "-ny") == 0)
    {
      if (i+1 < argc) {ny = atoi (argv[i+1]); SHIFT_NARGS(2);}
      else usage();
    }
    else if (strcmp (argv[i], "-nz") == 0)
    {
      if (i+1 < argc) {nz = atoi (argv[i+1]); SHIFT_NARGS(2);}
      else usage();
    }
    else if (strcmp (argv[i], "-write") == 0)
    {
      if (i+1 < argc) {datafile = argv[i+1]; SHIFT_NARGS(2);}
      else usage();
    }
    else if (strcmp (argv[i], "-shrink") == 0)
    {
      shrink = true; SHIFT_ARGS();
    }
    else usage();
  }

  // If no radius is defined, then take the number of grid points along X axis
  if (R == 0.0) R = nx;
  Cell::setR(R);

  Node * n1 = new Node (0.0,           0.0,           0.0);
  Node * n2 = new Node ((double) nx+1, 0.0,           0.0);
  Node * n3 = new Node ((double) nx+1, 0.0,           (double) nz+1);
  Node * n4 = new Node (0.0,           0.0,           (double) nz+1);
  Node * n5 = new Node (0.0,           (double) ny+1, 0.0);
  Node * n6 = new Node ((double) nx+1, (double) ny+1, 0.0);
  Node * n7 = new Node ((double) nx+1, (double) ny+1, (double) nz+1);
  Node * n8 = new Node (0.0,           (double) ny+1, (double) nz+1);

  // Create mesh
  Mesh * mesh = new Mesh (nx, ny, nz, n1, n2, n3, n4, n5, n6, n7, n8);
  mesh->setFactor (factor);
  for (int i = 0; i < depth; i++) mesh->refine();

  // Reduce points
  mesh->mergePoints();

  // Generate dataset
  vtkDataSet * ds = mesh->getDataSet();

  // Reduce cells des mailles
  vtkShrinkFilter * shrinkFilter = vtkShrinkFilter::New();
  if (shrink)
  {
    shrinkFilter->SetShrinkFactor (0.9);
    shrinkFilter->SetInputData (ds);
    shrinkFilter->Update();
    ds = shrinkFilter->GetOutput();
  }

  // Write out dataset
  if (datafile != "")
  {
    vtkUnstructuredGridWriter * writer = vtkUnstructuredGridWriter::New();
    writer->SetInputData(ds);
    writer->SetFileName (datafile.c_str());
    writer->Write();
    writer->Delete();
  }

  // Geometry filter
  vtkDataSetSurfaceFilter * dataSetSurfaceFilter = vtkDataSetSurfaceFilter::New();
  dataSetSurfaceFilter->SetInputData(ds);

  // Mappers
  vtkPolyDataMapper * polyDataMapper1 = vtkPolyDataMapper::New();
  polyDataMapper1->SetInputConnection(dataSetSurfaceFilter->GetOutputPort());
  polyDataMapper1->SetResolveCoincidentTopologyToPolygonOffset();
  vtkPolyDataMapper * polyDataMapper2 = vtkPolyDataMapper::New();
  polyDataMapper2->SetInputConnection(dataSetSurfaceFilter->GetOutputPort());
  polyDataMapper2->SetResolveCoincidentTopologyToPolygonOffset();

  // Actors
  vtkActor *actor1 = vtkActor::New();
  actor1->GetProperty()->SetColor(.8,.2,.2);
  actor1->SetMapper (polyDataMapper1);
  vtkActor *actor2 = vtkActor::New();
  actor2->GetProperty()->SetRepresentationToWireframe();
  actor2->GetProperty()->SetColor( .5, .5, .5 );
  actor2->SetMapper (polyDataMapper2);

  // Window and interactor
  vtkRenderer * ren = vtkRenderer::New();
  ren->SetBackground (1.,1.,1.);
  ren->AddActor(actor1);
  ren->AddActor(actor2);

  vtkRenderWindow * renWindow = vtkRenderWindow::New();
  renWindow->SetSize (800,800);
  renWindow->AddRenderer(ren);

  vtkRenderWindowInteractor * interacteur = vtkRenderWindowInteractor::New();
  vtkInteractorStyleSwitch * style = vtkInteractorStyleSwitch::SafeDownCast (interacteur->GetInteractorStyle());
  interacteur->SetRenderWindow(renWindow);
  if (style) style->SetCurrentStyleToTrackballCamera ();

  // Render
  //renWindow->Render();
  //interacteur->Start();

  // Clean up
  delete mesh;
  delete n1;
  delete n2;
  delete n3;
  delete n4;
  delete n5;
  delete n6;
  delete n7;
  delete n8;

  shrinkFilter->Delete();
  dataSetSurfaceFilter->Delete();
  polyDataMapper1->Delete();
  polyDataMapper2->Delete();
  actor1->Delete();
  actor2->Delete();
  ren->Delete();
  renWindow->Delete();
  interacteur->Delete();

  return 0;
}
