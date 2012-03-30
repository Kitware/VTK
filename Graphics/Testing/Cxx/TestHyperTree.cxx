/*=========================================================================

  Copyright (c) Kitware Inc.
  All rights reserved.

=========================================================================*/
#include "vtksys/CommandLineArguments.hxx"
#include "vtksys/Directory.hxx"
#include "vtksys/SystemTools.hxx"
#include "vtksys/Glob.hxx"
#include <vtksys/Process.h>
#include <vtkstd/string>
#include "vtkHyperTree.h"
#include "vtkHyperTreeFractalSource.h"

#include "vtkContourFilter.h"
#include "vtkCutter.h"
#include "vtkDataSetMapper.h"
#include "vtkGeometryFilter.h"
#include "vtkNew.h"
#include "vtkPlane.h"
#include "vtkShrinkFilter.h"
#include "vtkPolyDataWriter.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkUnstructuredGridWriter.h"
#include "vtkXMLUnstructuredGridWriter.h"
#include "vtkUnstructuredGrid.h"
#include "vtkDataSetWriter.h"

int TestHyperTree(int argc, char** argv)
{
  vtkNew<vtkHyperTreeFractalSource> fractal;
  fractal->SetMaximumLevel(3);
  fractal->DualOn();
  fractal->SetDimension(3);
  fractal->SetAxisBranchFactor(2);
  vtkHyperTree* tree = fractal->NewHyperTree();

  vtkNew<vtkCutter> cut;
  vtkNew<vtkPlane> plane;
  plane->SetOrigin(0.5, 0.5, 0.3333333);
  plane->SetNormal(0,0,1);
  cut->SetInputData(tree);
  cut->SetCutFunction(plane.GetPointer());
  vtkPolyDataWriter* writer = vtkPolyDataWriter::New();
  writer->SetFileName("./hyperTreeCut.vtk");
  writer->SetInputConnection(cut->GetOutputPort());
  writer->Write();
  writer->Delete();

  vtkNew<vtkContourFilter> contour;
  contour->SetInputData( tree );
  contour->SetNumberOfContours( 2 );
  contour->SetValue( 0, 2. );
  contour->SetValue( 1, 3. );
  contour->SetInputArrayToProcess(
    0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,"Test");
  vtkPolyDataWriter* writer2 = vtkPolyDataWriter::New();
  writer2->SetFileName("./hyperTreeContour.vtk");
  writer2->SetInputConnection(contour->GetOutputPort());
  writer2->Write();
  writer2->Delete();

  vtkNew<vtkShrinkFilter> shrink;
  shrink->SetInputData(tree);
  shrink->SetShrinkFactor(1);
  vtkUnstructuredGridWriter* writer3 = vtkUnstructuredGridWriter::New();
  writer3->SetFileName("./hyperTreeShrink.vtk");
  writer3->SetInputConnection(shrink->GetOutputPort());
  writer3->Write();
  writer3->Delete();

  vtkNew<vtkDataSetMapper> treeMapper;
  treeMapper->SetInputConnection( shrink->GetOutputPort() );
  vtkNew<vtkActor> treeActor;
  treeActor->SetMapper( treeMapper.GetPointer() );

  // Create a renderer, add actors to it
  vtkNew<vtkRenderer> ren1;
  ren1->AddActor( treeActor.GetPointer() );
  ren1->SetBackground( 1., 1., 1. );

  // Create a renderWindow
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer( ren1.GetPointer() );
  renWin->SetSize( 300, 300 );
  renWin->SetMultiSamples( 0 );

  // Create interactor
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow( renWin.GetPointer() );

  // Render and test
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin.GetPointer() );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  tree->Delete();
  return 0;
}
