/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAreaSelections.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests vtkVisibleCellSelector, vtkExtractSelectedFrustum, 
// vtkRenderedAreaPicker, and vtkInteractorStyleRubberBandPick.
// 
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkInteractorStyleRubberBandPick.h"
#include "vtkCallbackCommand.h"
#include "vtkVisibleCellSelector.h"
#include "vtkSelection.h"
#include "vtkExtractSelectedPolyDataIds.h"
#include "vtkIdTypeArray.h"
#include "vtkRenderedAreaPicker.h"
#include "vtkCamera.h"
#include "vtkImageMandelbrotSource.h"
#include "vtkImageActor.h"
#include "vtkExtractSelectedFrustum.h"
#include "vtkDataSetMapper.h"

#include "vtkDataSetReader.h"

vtkRenderer *renderer = NULL;
vtkSphereSource* SS1 = NULL;
vtkDataSetMapper* sMap = NULL;
vtkPolyData* emptyPD = NULL;

static void EndPick(vtkObject *vtkNotUsed( caller ),
                    unsigned long vtkNotUsed(eventId), 
                    void *, void *)
{
  vtkVisibleCellSelector *sel = vtkVisibleCellSelector::New();
  sel->SetRenderer(renderer);

  double x0 = renderer->GetPickX1();
  double y0 = renderer->GetPickY1();
  double x1 = renderer->GetPickX2();
  double y1 = renderer->GetPickY2();

  sel->SetRenderPasses(0,1,0,1,1);
  sel->SetArea(static_cast<int>(x0),static_cast<int>(y0),static_cast<int>(x1),
               static_cast<int>(y1));
  sel->Select();
  vtkSelection *res = vtkSelection::New();
  sel->GetSelectedIds(res);

  /*
  cerr << "x0 " << x0 << " y0 " << y0 << "\t";
  cerr << "x1 " << x1 << " y1 " << y1 << endl;
  vtkIdTypeArray *a = vtkIdTypeArray::New();
  sel->GetSelectedIds(a);
  cerr << "numhits = " << a->GetNumberOfTuples() << endl;
  sel->PrintSelectedIds(a);
  a->Delete();
  */

  vtkSelection *cellids = res->GetChild(0);
  vtkExtractSelectedPolyDataIds *extr = vtkExtractSelectedPolyDataIds::New();
  if (cellids)
    {
    extr->SetInput(0, SS1->GetOutput());
    extr->SetInput(1, cellids);
    extr->Update();
    sMap->SetInput(extr->GetOutput());
    }
  else
    {
    cerr << "Empty color buffer selection -" << endl;
    cerr << "Check display color depth. Must be at least 24 bit." << endl;
    sMap->SetInput(emptyPD);
    }

  sel->Delete();
  res->Delete();
  extr->Delete();
}

int TestAreaSelections(int argc, char* argv[])
{
  // Standard rendering classes
  renderer = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(renderer);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);  

  //set up the view
  renderer->GetActiveCamera()->SetPosition(  1.5, -0.75, 7);
  renderer->GetActiveCamera()->SetFocalPoint(1.5, -0.75, 0);
  renderer->GetActiveCamera()->SetViewUp(     0,   1,   0);
  renderer->SetBackground(0.0,0.0,0.0); 
  renWin->SetSize(300,300);

  //use the rubber band pick interactor style
  vtkRenderWindowInteractor* rwi = renWin->GetInteractor();
  vtkInteractorStyleRubberBandPick *rbp = 
    vtkInteractorStyleRubberBandPick::New();
  rwi->SetInteractorStyle(rbp);

  vtkRenderedAreaPicker *areaPicker = vtkRenderedAreaPicker::New();
  rwi->SetPicker(areaPicker);

  ////////////////////////////////////////////////////////////
  //Create a unstructured grid data source to test FrustumExtractor with.
  vtkDataSetReader *reader = vtkDataSetReader::New();
  char *cfname=vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SampleStructGrid.vtk");
  reader->SetFileName(cfname);
  
  vtkDataSetMapper *map1 = vtkDataSetMapper::New();
  map1->SetInput(reader->GetOutput());

  vtkActor *act1 = vtkActor::New();
  act1->SetMapper(map1);
  act1->PickableOff(); //prevents the visible cell selector from trying
  renderer->AddActor(act1);

  //frustum extractor works on geometry and doesn't care about pickability
  vtkExtractSelectedFrustum *extractor = vtkExtractSelectedFrustum::New();
  extractor->SetInputConnection(reader->GetOutputPort());
  extractor->PassThroughOff();
  extractor->SetFrustum(areaPicker->GetFrustum());

  vtkDataSetMapper *eMap = vtkDataSetMapper::New();
  eMap->SetInput(extractor->GetOutput());

  vtkActor *eAct = vtkActor::New();
  eAct->SetPosition(2,0,0);
  eAct->SetMapper(eMap);
  eAct->PickableOff();
  renderer->AddActor(eAct);

  ////////////////////////////////////////////////////////////
  emptyPD = vtkPolyData::New();

  int res = 20;
  SS1 = vtkSphereSource::New();
  SS1->SetThetaResolution(res);
  SS1->SetPhiResolution(res);
  SS1->SetRadius(0.5);
  SS1->SetCenter(0.5,-1.5,0);
  vtkPolyDataMapper *map2 = vtkPolyDataMapper::New();
  map2->SetInput(SS1->GetOutput());
  
  vtkActor* act2 = vtkActor::New();
  act2->SetMapper(map2);
  act2->PickableOn(); //lets the VisibleCellSelector select in it
  renderer->AddActor(act2);

  sMap = vtkDataSetMapper::New();
  sMap->SetInput(SS1->GetOutput());

  vtkActor *sAct = vtkActor::New();
  sAct->SetMapper(sMap);
  sAct->SetPosition(2,0,0);
  sAct->PickableOff();
  renderer->AddActor(sAct);

  //pass pick events to the VisibleCellSelector
  vtkCallbackCommand *cbc = vtkCallbackCommand::New();
  cbc->SetCallback(EndPick);
  cbc->SetClientData(renderer);
  rwi->AddObserver(vtkCommand::EndPickEvent,cbc);
  cbc->Delete();

  ////////////////////////////////////////////////////////////

  //run the test

  renWin->Render();
  areaPicker->AreaPick(51,78,82,273,renderer);
  EndPick(NULL, 0, NULL, NULL);
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
  rbp->Delete();
  areaPicker->Delete();
  reader->Delete();
  map1->Delete();
  act1->Delete();
  extractor->Delete();
  eMap->Delete();
  eAct->Delete();
  emptyPD->Delete();
  SS1->Delete();
  map2->Delete();
  act2->Delete();
  sMap->Delete();
  sAct->Delete();
  delete [] cfname;
  return !retVal;
}
