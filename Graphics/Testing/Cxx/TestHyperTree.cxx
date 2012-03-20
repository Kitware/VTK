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
/*
  {
  vtkCutter* cut = vtkCutter::New();
  vtkPlane* plane = vtkPlane::New();
  plane->SetOrigin(0.5, 0.5, 0.3333333);
  plane->SetNormal(0,0,1);
  cut->SetInput(tree);
  cut->SetCutFunction(plane);
  vtkPolyDataWriter* writer = vtkPolyDataWriter::New();
  writer->SetFileName("c:/tmp/hyperTreeCut2.vtk");
  writer->SetInput(cut->GetOutput());
  writer->Write();
  }
*/
  {
  vtkShrinkFilter *shrink = vtkShrinkFilter::New();
  shrink->SetInputData(tree);
  shrink->SetShrinkFactor(0.7);
  vtkUnstructuredGridWriter* writer = vtkUnstructuredGridWriter::New();
  writer->SetFileName("./hyperTest.vtk");
  writer->SetInputConnection(shrink->GetOutputPort());
  writer->Write();
  }
  return 0;
}
