/*=========================================================================

  Program:   Visualization Toolkit

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
#include "vtkActor.h"
#include "vtkInteractorStyleRubberBandPick.h"
#include "vtkCommand.h"
#include "vtkHardwareSelector.h"
#include "vtkSelection.h"
#include "vtkIdTypeArray.h"
#include "vtkRenderedAreaPicker.h"
#include "vtkCamera.h"
#include "vtkImageActor.h"
#include "vtkPointData.h"
#include "vtkPlaneSource.h"
#include "vtkElevationFilter.h"
#include "vtkBitArray.h"
#include "vtkGlyph3DMapper.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include <assert.h>

vtkRenderer *renderer = NULL;

class MyEndPickCommand : public vtkCommand
{
public:
  MyEndPickCommand()
    {
    this->Renderer=0; // no reference counting
    this->Mask=0; // no reference counting
    this->DataSet=0;
    }

  virtual ~MyEndPickCommand()
    {
    // empty
    }

  virtual void Execute(vtkObject *vtkNotUsed(caller),
    unsigned long vtkNotUsed(eventId),
    void *vtkNotUsed(callData))
    {
    assert("pre: renderer_exists" && this->Renderer!=0);

    vtkHardwareSelector *sel = vtkHardwareSelector::New();
    sel->SetFieldAssociation(vtkDataObject::FIELD_ASSOCIATION_POINTS);
    sel->SetRenderer(renderer);

    double x0 = renderer->GetPickX1();
    double y0 = renderer->GetPickY1();
    double x1 = renderer->GetPickX2();
    double y1 = renderer->GetPickY2();
    sel->SetArea(static_cast<unsigned int>(x0),
      static_cast<unsigned int>(y0),
      static_cast<unsigned int>(x1),
      static_cast<unsigned int>(y1));

    vtkSelection *res = sel->Select();

#if 0
    cerr << "x0 " << x0 << " y0 " << y0 << "\t";
    cerr << "x1 " << x1 << " y1 " << y1 << endl;
    res->Print(cout);
#endif

    // Reset the mask to false.
    vtkIdType numPoints = this->Mask->GetNumberOfTuples();
    for (vtkIdType i=0; i < numPoints; i++)
      {
      this->Mask->SetValue(i,false);
      }

    vtkSelectionNode *glyphids = res->GetNode(0);
    if (glyphids!=0)
      {
      vtkAbstractArray *abs=glyphids->GetSelectionList();
      if(abs==0)
        {
        cout<<"abs is null"<<endl;
        }
      vtkIdTypeArray *ids=vtkIdTypeArray::SafeDownCast(abs);
      if(ids==0)
        {
        cout<<"ids is null"<<endl;
        }
      else
        {
        // modify mask array with selection.
        vtkIdType numSelPoints = ids->GetNumberOfTuples();
        for (vtkIdType i =0; i < numSelPoints; i++)
          {
          vtkIdType value = ids->GetValue(i);
          if (value >=0 && value < numPoints)
            {
            cout << "Turn On: " << value << endl;
            this->Mask->SetValue(value,true);
            }
          else
            {
            cout << "Ignoring: " << value << endl;
            }
          }
        }
      }
    this->DataSet->Modified();

    sel->Delete();
    res->Delete();
    }

  void SetRenderer(vtkRenderer *r)
    {
    this->Renderer=r;
    }

  vtkRenderer *GetRenderer() const
    {
    return this->Renderer;
    }

  void SetMask(vtkBitArray *m)
    {
    this->Mask=m;
    }
  void SetDataSet(vtkDataSet* ds)
    {
    this->DataSet = ds;
    }

protected:
  vtkRenderer *Renderer;
  vtkBitArray *Mask;
  vtkDataSet *DataSet;
};

int TestGlyph3DMapperPicking(int argc, char* argv[])
{
  int res=6;
  vtkPlaneSource *plane=vtkPlaneSource::New();
  plane->SetResolution(res,res);
  vtkElevationFilter *colors=vtkElevationFilter::New();
  colors->SetInputConnection(plane->GetOutputPort());
  plane->Delete();
  colors->SetLowPoint(-0.25,-0.25,-0.25);
  colors->SetHighPoint(0.25,0.25,0.25);

  vtkSphereSource *squad=vtkSphereSource::New();
  squad->SetPhiResolution(25);
  squad->SetThetaResolution(25);

  vtkGlyph3DMapper *glypher=vtkGlyph3DMapper::New();
  //  glypher->SetNestedDisplayLists(0);
  glypher->SetInputConnection(colors->GetOutputPort());
  colors->Delete();
  glypher->SetScaleFactor(0.1);
  glypher->SetSourceConnection(squad->GetOutputPort());
  squad->Delete();

  // selection is performed on actor1
  vtkActor *glyphActor1=vtkActor::New();
  glyphActor1->SetMapper(glypher);
  glypher->Delete();
  glyphActor1->PickableOn();

  // result of selection is on actor2
  vtkActor *glyphActor2=vtkActor::New();
  glyphActor2->PickableOff();
  colors->Update(); // make sure output is valid.
  vtkDataSet *selection=colors->GetOutput()->NewInstance();
  selection->ShallowCopy(colors->GetOutput());

  vtkBitArray *selectionMask=vtkBitArray::New();
  selectionMask->SetName("mask");
  selectionMask->SetNumberOfComponents(1);
  selectionMask->SetNumberOfTuples(selection->GetNumberOfPoints());
  // Initially, everything is selected
  vtkIdType i=0;
  vtkIdType c=selectionMask->GetNumberOfTuples();
  while(i<c)
    {
      selectionMask->SetValue(i,true);
      ++i;
    }
  selection->GetPointData()->AddArray(selectionMask);
  selectionMask->Delete();

  vtkGlyph3DMapper *glypher2=vtkGlyph3DMapper::New();
  //  glypher->SetNestedDisplayLists(0);
  glypher2->SetMasking(1);
  glypher2->SetMaskArray("mask");

  glypher2->SetInputConnection(0, selection->GetProducerPort());
  glypher2->SetScaleFactor(0.1);
  glypher2->SetSourceConnection(squad->GetOutputPort());
  glyphActor2->SetMapper(glypher2);
  glypher2->Delete();

  // Standard rendering classes
  renderer = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(renderer);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  //set up the view
  renderer->SetBackground(0.2,0.2,0.2);
  renWin->SetSize(300,300);

  //use the rubber band pick interactor style
  vtkRenderWindowInteractor* rwi = renWin->GetInteractor();
  vtkInteractorStyleRubberBandPick *rbp =
    vtkInteractorStyleRubberBandPick::New();
  rwi->SetInteractorStyle(rbp);

  vtkRenderedAreaPicker *areaPicker = vtkRenderedAreaPicker::New();
  rwi->SetPicker(areaPicker);

  renderer->AddActor(glyphActor1);
  renderer->AddActor(glyphActor2);
  glyphActor2->SetPosition(2,0,0);
  glyphActor1->Delete();
  glyphActor2->Delete();

  //pass pick events to the VisibleGlyphSelector
  MyEndPickCommand *cbc=new MyEndPickCommand;
  cbc->SetRenderer(renderer);
  cbc->SetMask(selectionMask);
  cbc->SetDataSet(selection);
  rwi->AddObserver(vtkCommand::EndPickEvent,cbc);
  cbc->Delete();

  ////////////////////////////////////////////////////////////

  //run the test

  renderer->ResetCamera();

  renWin->Render();
  areaPicker->AreaPick(51,78,82,273,renderer);
  cbc->Execute(NULL, 0, NULL);
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
  selection->Delete();
  return !retVal;
}
