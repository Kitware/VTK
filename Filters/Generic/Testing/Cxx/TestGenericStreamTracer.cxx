/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGenericStreamTracer.cxx

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
#include "vtkStructuredGrid.h"
#include "vtkStructuredGridReader.h"
#include "vtkBridgeDataSet.h"
#include "vtkGenericCellTessellator.h"
#include "vtkGenericSubdivisionErrorMetric.h"
#include <assert.h>
#include "vtkGeometricErrorMetric.h"
#include "vtkAttributesErrorMetric.h"
#include "vtkGenericOutlineFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkRungeKutta45.h"
#include "vtkGenericStreamTracer.h"
#include "vtkAssignAttribute.h"
#include "vtkPolyData.h"
#include "vtkRibbonFilter.h"
#include "vtkGenericAttributeCollection.h"
#include "vtkGenericAttribute.h"
#include "vtkCamera.h"

#ifdef WRITE_GENERIC_RESULT
# include "vtkXMLPolyDataWriter.h"
#endif // #ifdef WRITE_GENERIC_RESULT

int TestGenericStreamTracer(int argc, char* argv[])
{
  // Standard rendering classes
  vtkRenderer *renderer = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(renderer);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  // Load the mesh geometry and data from a file
  vtkStructuredGridReader *reader = vtkStructuredGridReader::New();
  char *cfname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/office.binary.vtk");
  //char *cfname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/quadTet2.vtu");
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

  vtkIndent indent;
  ds->PrintSelf(cout,indent);

  vtkGenericOutlineFilter *outline=vtkGenericOutlineFilter::New();
  outline->SetInputData(ds);
  vtkPolyDataMapper *mapOutline=vtkPolyDataMapper::New();
  mapOutline->SetInputConnection(outline->GetOutputPort());
  vtkActor *outlineActor=vtkActor::New();
  outlineActor->SetMapper(mapOutline);
  outlineActor->GetProperty()->SetColor(0,0,0);

  vtkRungeKutta45 *rk=vtkRungeKutta45::New();

  // Create source for streamtubes
  vtkGenericStreamTracer *streamer=vtkGenericStreamTracer::New();
  streamer->SetInputData(ds);
  streamer->SetStartPosition(0.1,2.1,0.5);
  streamer->SetMaximumPropagation(0,500);
  streamer->SetMinimumIntegrationStep(1,0.1);
  streamer->SetMaximumIntegrationStep(1,1.0);
  streamer->SetInitialIntegrationStep(2,0.2);
  streamer->SetIntegrationDirection(0);
  streamer->SetIntegrator(rk);
  streamer->SetRotationScale(0.5);
  streamer->SetMaximumError(1.0E-8);

  vtkAssignAttribute *aa=vtkAssignAttribute::New();
  aa->SetInputConnection(streamer->GetOutputPort());
  aa->Assign("Normals",vtkDataSetAttributes::NORMALS,
             vtkAssignAttribute::POINT_DATA);

  vtkRibbonFilter *rf1=vtkRibbonFilter::New();
  rf1->SetInputConnection(aa->GetOutputPort());
  rf1->SetWidth(0.1);
  rf1->VaryWidthOff();

  vtkPolyDataMapper *mapStream=vtkPolyDataMapper::New();
  mapStream->SetInputConnection(rf1->GetOutputPort());
  mapStream->SetScalarRange(ds->GetAttributes()->GetAttribute(0)->GetRange());
  vtkActor *streamActor=vtkActor::New();
  streamActor->SetMapper(mapStream);

  renderer->AddActor(outlineActor);
  renderer->AddActor(streamActor);

  vtkCamera *cam=renderer->GetActiveCamera();
  cam->SetPosition(-2.35599,-3.35001,4.59236);
  cam->SetFocalPoint(2.255,2.255,1.28413);
  cam->SetViewUp(0.311311,0.279912,0.908149);
  cam->SetClippingRange(1.12294,16.6226);

#ifdef WRITE_GENERIC_RESULT
  // Save the result of the filter in a file
  vtkXMLPolyDataWriter *writer=vtkXMLPolyDataWriter::New();
  writer->SetInputConnection(streamer->GetOutputPort());
  writer->SetFileName("streamed.vtu");
  writer->SetDataModeToAscii();
  writer->Write();
  writer->Delete();
#endif // #ifdef WRITE_GENERIC_RESULT

  // Standard testing code.
  renderer->SetBackground(0.4,0.4,0.5);
  renWin->SetSize(300,200);
  renWin->Render();
  streamer->GetOutput()->PrintSelf(cout,indent);
  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  // Cleanup
  renderer->Delete();
  renWin->Delete();
  iren->Delete();
  ds->Delete();

  outline->Delete();
  mapOutline->Delete();
  outlineActor->Delete();

  rk->Delete();
  streamer->Delete();
  aa->Delete();
  rf1->Delete();
  mapStream->Delete();
  streamActor->Delete();

  return !retVal;
}
