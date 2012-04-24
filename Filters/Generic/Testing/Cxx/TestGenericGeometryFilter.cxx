/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGenericGeometryFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This example demonstrates how to implement a vtkGenericDataSet
// (here vtkBridgeDataSet) and to use vtkGenericDataSetTessellator filter on
// it.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit
// -D <path> => path to the data; the data should be in <path>/Data/

//#define WRITE_GENERIC_RESULT

#include "vtkActor.h"
#include "vtkDebugLeaks.h"
#include "vtkPointData.h"
#include "vtkProperty.h"
#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLUnstructuredGridReader.h"
#include "vtkBridgeDataSet.h"
#include "vtkGenericGeometryFilter.h"
#include "vtkGenericCellTessellator.h"
#include "vtkGenericSubdivisionErrorMetric.h"
#include <assert.h>
#include "vtkLookupTable.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyData.h"
#include "vtkGeometricErrorMetric.h"
#include "vtkAttributesErrorMetric.h"
#include "vtkSimpleCellTessellator.h"

#ifdef WRITE_GENERIC_RESULT
# include "vtkXMLPolyDataWriter.h"
#endif // #ifdef WRITE_GENERIC_RESULT

// Remark about the lookup tables that seem different between the
// GenericGeometryFilter and GenericDataSetTessellator:
// the lookup table is set for the whole unstructured grid, the tetra plus
// the triangle. The lookup table changed because of the tetra: the
// GenericDataSetTessellator need to create inside sub-tetra that have
// minimal attributes, the GenericGeometryFilter just need to tessellate the
// face of the tetra, for which the values at points are not minimal.

int TestGenericGeometryFilter(int argc, char* argv[])
{
  // Standard rendering classes
  vtkRenderer *renderer = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(renderer);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  // Load the mesh geometry and data from a file
  vtkXMLUnstructuredGridReader *reader = vtkXMLUnstructuredGridReader::New();
  char *cfname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/quadraticTetra01.vtu");
//  char *cfname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/quadTet2.vtu");
// char *cfname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Test2_Volume.vtu");
// char *cfname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/quadHexa01.vtu");
//  char *cfname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/quadQuad01.vtu");

  reader->SetFileName( cfname );
  delete[] cfname;

  // Force reading
  reader->Update();

  // Initialize the bridge
  vtkBridgeDataSet *ds=vtkBridgeDataSet::New();
  ds->SetDataSet( reader->GetOutput() );
  reader->Delete();


  // Set the error metric thresholds:
  // 1. for the geometric error metric
  vtkGeometricErrorMetric *geometricError=vtkGeometricErrorMetric::New();
  geometricError->SetRelativeGeometricTolerance(0.1,ds);

  ds->GetTessellator()->GetErrorMetrics()->AddItem(geometricError);
  geometricError->Delete();

  // 2. for the attribute error metric
  vtkAttributesErrorMetric *attributesError=vtkAttributesErrorMetric::New();
  attributesError->SetAttributeTolerance(0.01);

  ds->GetTessellator()->GetErrorMetrics()->AddItem(attributesError);
  attributesError->Delete();

  cout<<"input unstructured grid: "<<ds<<endl;

  static_cast<vtkSimpleCellTessellator *>(ds->GetTessellator())->SetMaxSubdivisionLevel(10);

  vtkIndent indent;
  ds->PrintSelf(cout,indent);

  // Create the filter
  vtkGenericGeometryFilter *geom = vtkGenericGeometryFilter::New();
  geom->SetInputData(ds);
  geom->SetPassThroughCellIds(1);

  geom->Update(); //So that we can call GetRange() on the scalars

  assert(geom->GetOutput()!=0);

  // This creates a blue to red lut.
  vtkLookupTable *lut = vtkLookupTable::New();
  lut->SetHueRange (0.667, 0.0);

  vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
  mapper->SetLookupTable(lut);
  mapper->SetInputConnection( geom->GetOutputPort() );

  if(geom->GetOutput()->GetPointData()!=0)
    {
    if(geom->GetOutput()->GetPointData()->GetScalars()!=0)
      {
      mapper->SetScalarRange( geom->GetOutput()->GetPointData()->
                              GetScalars()->GetRange());
      }
    }

  vtkActor *actor = vtkActor::New();
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

#ifdef WRITE_GENERIC_RESULT
  // Save the result of the filter in a file
  vtkXMLPolyDataWriter *writer=vtkXMLPolyDataWriter::New();
  writer->SetInputConnection(geom->GetOutputPort());
  writer->SetFileName("geometry.vtp");
  writer->SetDataModeToAscii();
  writer->Write();
  writer->Delete();
#endif // #ifdef WRITE_GENERIC_RESULT

  // Standard testing code.
  renderer->SetBackground(0.5,0.5,0.5);
  renWin->SetSize(300,300);
  renWin->Render();
  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  // Cleanup
  renderer->Delete();
  renWin->Delete();
  iren->Delete();
  mapper->Delete();
  actor->Delete();
  geom->Delete();
  ds->Delete();
  lut->Delete();

  return !retVal;
}
