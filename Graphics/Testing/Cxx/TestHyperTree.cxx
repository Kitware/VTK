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

#include "vtkCutter.h"
#include "vtkPlane.h"
#include "vtkShrinkFilter.h"
#include "vtkContourFilter.h"

#include "vtkThreshold.h"


#include "vtkPolyDataWriter.h"
#include "vtkUnstructuredGridWriter.h"
#include "vtkXMLUnstructuredGridWriter.h"
#include "vtkUnstructuredGrid.h"
#include "vtkDataSetWriter.h"

int TestHyperTree(int argc, char** argv)
{
  vtkHyperTreeFractalSource* fractal = vtkHyperTreeFractalSource::New();
  fractal->SetMaximumLevel(3);
  fractal->DualOn();
  fractal->SetDimension(3);
  fractal->SetAxisBranchFactor(3);
  vtkHyperTree* tree = fractal->NewHyperTree();

  vtkCutter* cut = vtkCutter::New();
  vtkPlane* plane = vtkPlane::New();
  plane->SetOrigin(0.5, 0.5, 0.3333333);
  plane->SetNormal(0,0,1);
  cut->SetInputData(tree);
  cut->SetCutFunction(plane);
  vtkPolyDataWriter* writer = vtkPolyDataWriter::New();
  writer->SetFileName("./hyperTreeCut.vtk");
  writer->SetInputConnection(cut->GetOutputPort());
  writer->Write();
  
  vtkContourFilter *contour = vtkContourFilter::New();
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

  vtkShrinkFilter *shrink = vtkShrinkFilter::New();
  shrink->SetInputData(tree);
  shrink->SetShrinkFactor(0.7);
  vtkUnstructuredGridWriter* writer3 = vtkUnstructuredGridWriter::New();
  writer3->SetFileName("./hyperTreeShrink.vtk");
  writer3->SetInputConnection(shrink->GetOutputPort());
  writer3->Write();

  return 0;
}
