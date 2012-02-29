/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImagePlaneWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSmartPointer.h"
#include "vtkRegressionTestImage.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellPicker.h"
#include "vtkCommand.h"
#include "vtkImageActor.h"
#include "vtkImageReslice.h"
#include "vtkInteractorStyleImage.h"
#include "vtkImageMapToColors.h"
#include "vtkImagePlaneWidget.h"
#include "vtkImageReader.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkLookupTable.h"
#include "vtkOutlineFilter.h"
#include "vtkDICOMImageReader.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkVolume16Reader.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkPlaneSource.h"
#include "vtkPlane.h"
#include "vtkResliceCursorActor.h"
#include "vtkResliceCursorPolyDataAlgorithm.h"
#include "vtkResliceCursor.h"
#include "vtkResliceCursorWidget.h"
#include "vtkResliceCursorLineRepresentation.h"
#include "vtkBiDimensionalWidget.h"
//#include "vtkResliceCursorThickLineRepresentation.h"

#include "vtkTestUtilities.h"


//----------------------------------------------------------------------------
class vtkResliceCursorCallback : public vtkCommand
{
public:
  static vtkResliceCursorCallback *New()
  { return new vtkResliceCursorCallback; }

  void Execute( vtkObject *caller, unsigned long /*ev*/,
                void *callData )
    {
    vtkImagePlaneWidget* ipw =
      dynamic_cast< vtkImagePlaneWidget* >( caller );
    if (ipw)
      {
      double* wl = static_cast<double*>( callData );

      if ( ipw == this->IPW[0] )
        {
        this->IPW[1]->SetWindowLevel(wl[0],wl[1],1);
        this->IPW[2]->SetWindowLevel(wl[0],wl[1],1);
        }
      else if( ipw == this->IPW[1] )
        {
        this->IPW[0]->SetWindowLevel(wl[0],wl[1],1);
        this->IPW[2]->SetWindowLevel(wl[0],wl[1],1);
        }
      else if (ipw == this->IPW[2])
        {
        this->IPW[0]->SetWindowLevel(wl[0],wl[1],1);
        this->IPW[1]->SetWindowLevel(wl[0],wl[1],1);
        }
      }

    vtkResliceCursorWidget *rcw = dynamic_cast<
      vtkResliceCursorWidget * >(caller);
    if (rcw)
      {
      vtkResliceCursorLineRepresentation *rep = dynamic_cast<
        vtkResliceCursorLineRepresentation * >(rcw->GetRepresentation());
      vtkResliceCursor *rc = rep->GetResliceCursorActor()->
                  GetCursorAlgorithm()->GetResliceCursor();
      for (int i = 0; i < 3; i++)
        {
        vtkPlaneSource *ps = static_cast< vtkPlaneSource * >(
            this->IPW[i]->GetPolyDataAlgorithm());
        ps->SetNormal(rc->GetPlane(i)->GetNormal());
        ps->SetCenter(rc->GetPlane(i)->GetOrigin());

        // If the reslice plane has modified, update it on the 3D widget
        this->IPW[i]->UpdatePlacement();

        //std::cout << "Updating placement of plane: " << i << " " <<
        //  rc->GetPlane(i)->GetNormal()[0] << " " <<
        //  rc->GetPlane(i)->GetNormal()[1] << " " <<
        //  rc->GetPlane(i)->GetNormal()[2] << std::endl;
        //this->IPW[i]->GetReslice()->Print(cout);
        //rep->GetReslice()->Print(cout);
        //std::cout << "---------------------" << std::endl;
        }
      }

    // Render everything
    this->RCW[0]->Render();
    }

  vtkResliceCursorCallback() {}
  vtkImagePlaneWidget* IPW[3];
  vtkResliceCursorWidget *RCW[3];
};


//----------------------------------------------------------------------------
int TestResliceCursorWidget2( int argc, char *argv[] )
{
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");

  vtkSmartPointer<vtkVolume16Reader> reader =
    vtkSmartPointer<vtkVolume16Reader>::New();
  reader->SetDataDimensions( 64, 64);
  reader->SetDataByteOrderToLittleEndian();
  reader->SetImageRange( 1, 93);
  reader->SetDataSpacing( 3.2, 3.2, 1.5);
  reader->SetFilePrefix( fname );
  reader->ReleaseDataFlagOn();
  reader->SetDataMask( 0x7fff);
  reader->Update();
  delete[] fname;

  vtkSmartPointer<vtkOutlineFilter> outline =
    vtkSmartPointer<vtkOutlineFilter>::New();
  outline->SetInputConnection(reader->GetOutputPort());

  vtkSmartPointer<vtkPolyDataMapper> outlineMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  outlineMapper->SetInputConnection(outline->GetOutputPort());

  vtkSmartPointer<vtkActor> outlineActor =
    vtkSmartPointer<vtkActor>::New();
  outlineActor->SetMapper( outlineMapper);

  vtkSmartPointer<vtkRenderer> ren[4];

  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->SetMultiSamples(0);

  for (int i = 0; i < 4; i++)
    {
    ren[i] = vtkSmartPointer<vtkRenderer>::New();
    renWin->AddRenderer(ren[i]);
    }

  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  vtkSmartPointer<vtkCellPicker> picker =
    vtkSmartPointer<vtkCellPicker>::New();
  picker->SetTolerance(0.005);

  vtkSmartPointer<vtkProperty> ipwProp =
    vtkSmartPointer<vtkProperty>::New();


  //assign default props to the ipw's texture plane actor
  vtkSmartPointer<vtkImagePlaneWidget> planeWidget[3];
  int imageDims[3];
  reader->GetOutput()->GetDimensions(imageDims);

  for (int i = 0; i < 3; i++)
    {
    planeWidget[i] = vtkSmartPointer<vtkImagePlaneWidget>::New();
    planeWidget[i]->SetInteractor( iren );
    planeWidget[i]->SetPicker(picker);
    planeWidget[i]->RestrictPlaneToVolumeOn();
    double color[3] = {0, 0, 0};
    color[i] = 1;
    planeWidget[i]->GetPlaneProperty()->SetColor(color);
    planeWidget[i]->SetTexturePlaneProperty(ipwProp);
    planeWidget[i]->TextureInterpolateOff();
    planeWidget[i]->SetResliceInterpolateToLinear();
    planeWidget[i]->SetInputConnection(reader->GetOutputPort());
    planeWidget[i]->SetPlaneOrientation(i);
    planeWidget[i]->SetSliceIndex(imageDims[i]/2);
    planeWidget[i]->DisplayTextOn();
    planeWidget[i]->SetDefaultRenderer(ren[3]);
    planeWidget[i]->SetWindowLevel(1358, -27);
    planeWidget[i]->On();
    planeWidget[i]->InteractionOn();
    }

  planeWidget[1]->SetLookupTable(planeWidget[0]->GetLookupTable());
  planeWidget[2]->SetLookupTable(planeWidget[0]->GetLookupTable());


  vtkSmartPointer<vtkResliceCursorCallback> cbk =
    vtkSmartPointer<vtkResliceCursorCallback>::New();

  // Create the reslice cursor, widget and rep

  vtkSmartPointer< vtkResliceCursor > resliceCursor =
    vtkSmartPointer< vtkResliceCursor >::New();
  resliceCursor->SetCenter(reader->GetOutput()->GetCenter());
  resliceCursor->SetThickMode(0);
  resliceCursor->SetThickness(10, 10, 10);
  resliceCursor->SetImage(reader->GetOutput());

  vtkSmartPointer< vtkResliceCursorWidget > resliceCursorWidget[3];
  vtkSmartPointer< vtkResliceCursorLineRepresentation > resliceCursorRep[3];

  for (int i = 0; i < 3; i++)
    {
    resliceCursorWidget[i] = vtkSmartPointer< vtkResliceCursorWidget >::New();
    resliceCursorWidget[i]->SetInteractor(iren);

    resliceCursorRep[i] =
      vtkSmartPointer< vtkResliceCursorLineRepresentation >::New();
    resliceCursorWidget[i]->SetRepresentation(resliceCursorRep[i]);
    resliceCursorRep[i]->GetResliceCursorActor()->
      GetCursorAlgorithm()->SetResliceCursor(resliceCursor);
    resliceCursorRep[i]->GetResliceCursorActor()->
      GetCursorAlgorithm()->SetReslicePlaneNormal(i);

    const double minVal = reader->GetOutput()->GetScalarRange()[0];
    if (vtkImageReslice *reslice =
        vtkImageReslice::SafeDownCast(resliceCursorRep[i]->GetReslice()))
      {
      reslice->SetBackgroundColor(minVal,minVal,minVal,minVal);
      }

    resliceCursorWidget[i]->SetDefaultRenderer(ren[i]);
    resliceCursorWidget[i]->SetEnabled(1);

    ren[i]->GetActiveCamera()->SetFocalPoint(0,0,0);
    double camPos[3] = {0,0,0};
    camPos[i] = 1;
    ren[i]->GetActiveCamera()->SetPosition(camPos);

    ren[i]->GetActiveCamera()->ParallelProjectionOn();
    ren[i]->ResetCamera();
    //ren[i]->ResetCameraClippingRange();

    // Tie the Image plane widget and the reslice cursor widget together
    cbk->IPW[i] = planeWidget[i];
    cbk->RCW[i] = resliceCursorWidget[i];
    resliceCursorWidget[i]->AddObserver(
        vtkResliceCursorWidget::ResliceAxesChangedEvent, cbk );

    // Initialize the window level to a sensible value
    double range[2];
    reader->GetOutput()->GetScalarRange(range);
    resliceCursorRep[i]->SetWindowLevel(range[1]-range[0], (range[0]+range[1])/2.0);
    planeWidget[i]->SetWindowLevel(range[1]-range[0], (range[0]+range[1])/2.0);

    // Make them all share the same color map.
    resliceCursorRep[i]->SetLookupTable(resliceCursorRep[0]->GetLookupTable());
    planeWidget[i]->GetColorMap()->SetLookupTable(resliceCursorRep[0]->GetLookupTable());
    }


  // Add the actors
  //
  ren[0]->SetBackground( 0.3, 0.1, 0.1 );
  ren[1]->SetBackground( 0.1, 0.3, 0.1 );
  ren[2]->SetBackground( 0.1, 0.1, 0.3 );
  ren[3]->AddActor( outlineActor );
  ren[3]->SetBackground( 0.1, 0.1, 0.1 );
  renWin->SetSize( 600, 600);
  //renWin->SetFullScreen(1);

  ren[0]->GetActiveCamera()->SetViewUp( 0, 0, -1 );
  ren[1]->GetActiveCamera()->SetViewUp( 0, 0, 1 );
  ren[2]->GetActiveCamera()->SetViewUp( 0, 1, 0 );

  ren[0]->SetViewport(0,0,0.5,0.5);
  ren[1]->SetViewport(0.5,0,1,0.5);
  ren[2]->SetViewport(0,0.5,0.5,1);
  ren[3]->SetViewport(0.5,0.5,1,1);

  // Set the actors' postions
  //
  renWin->Render();

  ren[3]->GetActiveCamera()->Elevation(110);
  ren[3]->GetActiveCamera()->SetViewUp(0, 0, -1);
  ren[3]->GetActiveCamera()->Azimuth(45);
  ren[3]->GetActiveCamera()->Dolly(1.15);
  ren[3]->ResetCameraClippingRange();

  vtkSmartPointer< vtkInteractorStyleImage > style =
      vtkSmartPointer< vtkInteractorStyleImage >::New();
  iren->SetInteractorStyle(style);

  iren->Initialize();

  int retVal = vtkRegressionTestImage(renWin);
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  return !retVal;
}
