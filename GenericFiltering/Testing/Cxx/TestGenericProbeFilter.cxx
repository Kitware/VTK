/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGenericProbeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This example demonstrates how to implement a vtkGenericDataSet
// (here vtkBridgeDataSet) and to use vtkGenericProbeFilter on it.
// 
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit
// -D <path> => path to the data; the data should be in <path>/Data/

//#define ADD_GEOMETRY
//#define STD_PROBE

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
#include "vtkPlaneSource.h"
#include "vtkTransform.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkGenericProbeFilter.h"
#include "vtkDataSetMapper.h"
#include "vtkPlane.h"
#include "vtkProbeFilter.h" // std probe filter, to compare


int TestGenericProbeFilter(int argc, char* argv[])
{
  // Standard rendering classes
  vtkRenderer *renderer = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(renderer);
  renderer->Delete();
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
  renWin->Delete();
  
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
  
#ifdef ADD_GEOMETRY
  // Geometry
   
  // Create the filter
  vtkGenericGeometryFilter *geom = vtkGenericGeometryFilter::New();
  geom->SetInput(ds);
  
  geom->Update(); //So that we can call GetRange() on the scalars
  
  assert(geom->GetOutput()!=0);
  
  // This creates a blue to red lut.
  vtkLookupTable *lut2 = vtkLookupTable::New(); 
  lut2->SetHueRange (0.667, 0.0);
  
  vtkPolyDataMapper *mapper2 = vtkPolyDataMapper::New();
  mapper2->SetLookupTable(lut2);
  lut2->Delete();
  mapper2->SetInputConnection(0, geom->GetOutputPort(0) );
  geom->Delete();
  
  if(geom->GetOutput()->GetPointData()!=0)
    {
    if(geom->GetOutput()->GetPointData()->GetScalars()!=0)
      {
      mapper2->SetScalarRange( geom->GetOutput()->GetPointData()->
                              GetScalars()->GetRange());
      }
    }
  vtkActor *actor2 = vtkActor::New();
  actor2->SetMapper(mapper2);
  mapper2->Delete();
  renderer->AddActor(actor2); // the surface
  actor2->Delete();
#endif
  
  // Create the probe plane
  vtkPlaneSource *plane=vtkPlaneSource::New();
  plane->SetResolution(100,100);
  vtkTransform *transp=vtkTransform::New();
  transp->Translate(0.5,0.5 ,0);
  transp->Scale(5,5,5);
  vtkTransformPolyDataFilter *tpd=vtkTransformPolyDataFilter::New();
  tpd->SetInputConnection(0,plane->GetOutputPort(0));
  plane->Delete();
  tpd->SetTransform(transp);
  transp->Delete();
  
#ifndef STD_PROBE
  // Create the filter
  vtkGenericProbeFilter *probe = vtkGenericProbeFilter::New();
  probe->SetInputConnection(0,tpd->GetOutputPort(0));
  tpd->Delete();
  probe->SetSource(ds);
  
  probe->Update(); //So that we can call GetRange() on the scalars
  
  assert(probe->GetOutput()!=0);
  
  // This creates a blue to red lut.
  vtkLookupTable *lut = vtkLookupTable::New(); 
  lut->SetHueRange (0.667, 0.0);
  
  vtkDataSetMapper *mapper = vtkDataSetMapper::New();
  mapper->SetLookupTable(lut);
  lut->Delete();
  mapper->SetInputConnection(0, probe->GetOutputPort(0) );
  probe->Delete();
  
  if(probe->GetOutput()->GetPointData()!=0)
    {
    if(probe->GetOutput()->GetPointData()->GetScalars()!=0)
      {
      mapper->SetScalarRange( probe->GetOutput()->GetPointData()->
                              GetScalars()->GetRange());
      }
    }
  
  vtkActor *actor = vtkActor::New();
  actor->SetMapper(mapper);
  mapper->Delete();
  renderer->AddActor(actor);
  actor->Delete();
  
#else
  // std probe, to compare
 
  vtkProbeFilter *stdProbe= vtkProbeFilter::New();
  stdProbe->SetInputConnection(0,tpd->GetOutputPort(0));
  tpd->Delete();
  stdProbe->SetSource(ds->GetDataSet());
  
  stdProbe->Update(); //So that we can call GetRange() on the scalars
  
  assert(stdProbe->GetOutput()!=0);
  
  // This creates a blue to red lut.
  vtkLookupTable *lut4 = vtkLookupTable::New(); 
  lut4->SetHueRange (0.667, 0.0);
  
  vtkDataSetMapper *mapper4 = vtkDataSetMapper::New();
  mapper4->SetLookupTable(lut4);
  lut4->Delete();
  mapper4->SetInputConnection(0, stdProbe->GetOutputPort(0) );
  stdProbe->Delete();
  
  if(stdProbe->GetOutput()->GetPointData()!=0)
    {
    if(stdProbe->GetOutput()->GetPointData()->GetScalars()!=0)
      {
      mapper4->SetScalarRange( stdProbe->GetOutput()->GetPointData()->
                               GetScalars()->GetRange());
      }
    }
  
  vtkActor *actor4 = vtkActor::New();
  actor4->SetMapper(mapper4);
  mapper4->Delete();
  renderer->AddActor(actor4);
  actor4->Delete();
#endif // #ifdef STD_PROBE

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
  iren->Delete();
  ds->Delete();
  
  return !retVal;
}
