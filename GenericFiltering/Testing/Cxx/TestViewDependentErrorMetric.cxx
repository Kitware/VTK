/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestViewDependentErrorMetric.cxx

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
#include "vtkGenericDataSetTessellator.h"
#include "vtkGenericCellTessellator.h"
#include "vtkGenericSubdivisionErrorMetric.h"
#include <assert.h>
#include "vtkLookupTable.h"
#include "vtkDataSetMapper.h"
#include "vtkLabeledDataMapper.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor2D.h"
#include "vtkCommand.h"
#include "vtkGeometricErrorMetric.h"
#include "vtkAttributesErrorMetric.h"
#include "vtkSimpleCellTessellator.h"
#include "vtkViewDependentErrorMetric.h"
#include "vtkGenericAttributeCollection.h"
#include "vtkGenericAttribute.h"
#include "vtkCamera.h"
#include "vtkGenericOutlineFilter.h"
#include "vtkGenericGeometryFilter.h"
#include "vtkPolyData.h"

#ifdef WRITE_GENERIC_RESULT
# include "vtkXMLUnstructuredGridWriter.h"
#endif // #ifdef WRITE_GENERIC_RESULT

// Remark about the lookup tables that seem different between the
// GenericGeometryFilter and GenericDataSetTessellator:
// the lookup table is set for the whole unstructured grid, the tetra plus
// the triangle. The lookup table changed because of the tetra: the
// GenericDataSetTessellator need to create inside sub-tetra that have
// minimal attributes, the GenericGeometryFilter just need to tessellate the
// face of the tetra, for which the values at points are not minimal.

class SwitchLabelsCallback
  : public vtkCommand
{
public:
  static SwitchLabelsCallback *New()
    { return new SwitchLabelsCallback; }
  
  void SetLabeledDataMapper(vtkLabeledDataMapper *aLabeledDataMapper)
    {
      this->LabeledDataMapper=aLabeledDataMapper;
    }
  void SetRenderWindow(vtkRenderWindow *aRenWin)
    {
      this->RenWin=aRenWin;
    }
  
  virtual void Execute(vtkObject *vtkNotUsed(caller), unsigned long, void*)
    { 
      if(this->LabeledDataMapper->GetLabelMode()==VTK_LABEL_SCALARS)
        {
        this->LabeledDataMapper->SetLabelMode(VTK_LABEL_IDS);
        }
      else
        {
        this->LabeledDataMapper->SetLabelMode(VTK_LABEL_SCALARS);
        }
      this->RenWin->Render();
    }
protected:
  vtkLabeledDataMapper *LabeledDataMapper;
  vtkRenderWindow *RenWin;
};

int TestViewDependentErrorMetric(int argc, char* argv[])
{
  // Standard rendering classes
  vtkRenderer *renderer = vtkRenderer::New();
  vtkRenderer *renderer2= vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(renderer);
  renWin->AddRenderer(renderer2);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  // Load the mesh geometry and data from a file
  vtkXMLUnstructuredGridReader *reader = vtkXMLUnstructuredGridReader::New();
  char *cfname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/quadraticTetra01.vtu");
//  char *cfname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Test2_Volume.vtu");
//  char *cfname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/quadTet3.vtu");
  reader->SetFileName( cfname );
  delete[] cfname;
  
  // Force reading
  reader->Update();

  // Initialize the bridge
  vtkBridgeDataSet *ds=vtkBridgeDataSet::New();
  ds->SetDataSet( reader->GetOutput() );
  reader->Delete();
#if 0
  // Set the error metric thresholds:
  // 1. for the geometric error metric
  vtkGeometricErrorMetric *geometricError=vtkGeometricErrorMetric::New();
  geometricError->SetRelativeGeometricTolerance(0.01,ds);
  
  ds->GetTessellator()->GetErrorMetrics()->AddItem(geometricError);
  geometricError->Delete();
  
  // 2. for the attribute error metric
  vtkAttributesErrorMetric *attributesError=vtkAttributesErrorMetric::New();
  attributesError->SetAttributeTolerance(0.01); // 0.11
  
  ds->GetTessellator()->GetErrorMetrics()->AddItem(attributesError);
  attributesError->Delete();
#endif

  // 3. for the view dependent error metric on the first renderer
  vtkViewDependentErrorMetric *viewError=vtkViewDependentErrorMetric::New();
  viewError->SetViewport(renderer);
  viewError->SetPixelTolerance(10000); // 0.25; 0.0625
  ds->GetTessellator()->GetErrorMetrics()->AddItem(viewError);
  viewError->Delete();

  // 4. for the view dependent error metric on the first renderer
  vtkViewDependentErrorMetric *viewError2=vtkViewDependentErrorMetric::New();
  viewError2->SetViewport(renderer2);
  viewError2->SetPixelTolerance(0.25); // 0.25; 0.0625
  ds->GetTessellator()->GetErrorMetrics()->AddItem(viewError2);
  viewError2->Delete();

  cout<<"input unstructured grid: "<<ds<<endl;

  
  static_cast<vtkSimpleCellTessellator *>(ds->GetTessellator())->SetMaxSubdivisionLevel(10);
    
  vtkIndent indent;
  ds->PrintSelf(cout,indent);
  
#if 0
  // Create the filter
  vtkGenericDataSetTessellator *tessellator = vtkGenericDataSetTessellator::New();
  tessellator->SetInput(ds);

  // DO NOT PERFORM UPDATE NOW, because the view dependent error metric
  // need the window to be realized first
  //tessellator->Update(); //So that we can call GetRange() on the scalars
  
  assert(tessellator->GetOutput()!=0);
#else
  // Create the filter
  vtkGenericGeometryFilter *tessellator = vtkGenericGeometryFilter::New();
  tessellator->SetInput(ds);

//  geom->Update(); //So that we can call GetRange() on the scalars
  
#endif
  
  // This creates a blue to red lut.
  vtkLookupTable *lut = vtkLookupTable::New(); 
  lut->SetHueRange (0.667, 0.0);
  
  vtkDataSetMapper *mapper = vtkDataSetMapper::New();
  mapper->SetLookupTable(lut);
  mapper->SetInputConnection( tessellator->GetOutputPort() );
  
  int i=0;
  int n=ds->GetAttributes()->GetNumberOfAttributes();
  int found=0;
  vtkGenericAttribute *attribute=0;
  while(i<n&&!found)
    {
    attribute=ds->GetAttributes()->GetAttribute(i);
    found=(attribute->GetCentering()==vtkPointCentered
           && attribute->GetNumberOfComponents()==1);
    ++i;
    }
  if(found)
    {
    mapper->SetScalarRange( attribute->GetRange(0));
    }
  mapper->ScalarVisibilityOff();
  
  vtkActor *actor = vtkActor::New();
  actor->SetMapper(mapper);
  
  
  vtkActor2D *actorLabel=vtkActor2D::New();
  vtkLabeledDataMapper *labeledDataMapper=vtkLabeledDataMapper::New();
  labeledDataMapper->SetLabelMode(VTK_LABEL_IDS);
  labeledDataMapper->SetInputConnection(tessellator->GetOutputPort());
  actorLabel->SetMapper(labeledDataMapper);
  labeledDataMapper->Delete();
  renderer->AddActor(actorLabel);
  actorLabel->SetVisibility(0);
  actorLabel->Delete();
  
  // Standard testing code.
  renderer->SetBackground(0.7,0.5,0.5);
  renderer->SetViewport(0,0,0.5,1);
  renderer2->SetBackground(0.5,0.5,0.8);
  renderer2->SetViewport(0.5,0,1,1);
  renWin->SetSize(600,300); // realized
  
  vtkGenericOutlineFilter *outlineFilter= vtkGenericOutlineFilter::New();
  outlineFilter->SetInput(ds);
  vtkPolyDataMapper *mapperOutline=vtkPolyDataMapper::New();
  mapperOutline->SetInputConnection(outlineFilter->GetOutputPort());
  outlineFilter->Delete();
  
  vtkActor *actorOutline=vtkActor::New();
  actorOutline->SetMapper(mapperOutline);
  mapperOutline->Delete();

  renderer->AddActor(actorOutline);
  renderer2->AddActor(actorOutline);
  actorOutline->Delete();
  // need an outline filter in the pipeline to ensure that the
  // camera are set with the bounding box of the dataset.
  
//  vtkCamera *cam1=renderer->GetActiveCamera();
  vtkCamera *cam2=renderer2->GetActiveCamera();

  renderer->ResetCamera();
  renderer2->ResetCamera();
  
  cam2->Azimuth(90);
  
  // Those two lines have to be called AFTER GetActiveCamera:
  // GetActiveCamera ask the mapper to update its input for the bounds
  // If the actor is connected it actually ask the output of tessellator
  // but the view dependent error metric are not yet initialized!
  renderer->AddActor(actor);
  renderer2->AddActor(actor);

  renWin->Render();
  
#ifdef WRITE_GENERIC_RESULT
  // BE SURE to save AFTER a first rendering!
  // Save the result of the filter in a file
  vtkXMLUnstructuredGridWriter *writer=vtkXMLUnstructuredGridWriter::New();
  writer->SetInputConnection(tessellator->GetOutputPort());
  writer->SetFileName("viewdeptessellated.vtu");
  writer->SetDataModeToAscii();
  writer->DebugOn();
  writer->Write();
  writer->Delete();
  
  // debug XML reader
  vtkXMLUnstructuredGridReader *rreader=vtkXMLUnstructuredGridReader::New();
//  rreader->SetInputConnection(tessellator->GetOutputPort());
  rreader->SetFileName("viewdeptessellated.vtu");
//  rreader->SetDataModeToAscii();
  rreader->DebugOn();
  rreader->Update();
  rreader->Delete();
  
#endif // #ifdef WRITE_GENERIC_RESULT

  
  tessellator->GetOutput()->PrintSelf(cout,indent);
  
  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    SwitchLabelsCallback *switchLabels=SwitchLabelsCallback::New();
    switchLabels->SetRenderWindow(renWin);
    switchLabels->SetLabeledDataMapper(labeledDataMapper);
    iren->AddObserver(vtkCommand::UserEvent,switchLabels);
    switchLabels->Delete();
    iren->Start();
    }

  // Cleanup
  renderer2->Delete();
  renderer->Delete();
  renWin->Delete();
  iren->Delete();
  mapper->Delete();
  actor->Delete();
  tessellator->Delete();
  ds->Delete();
  lut->Delete();
  
  return !retVal;
}
