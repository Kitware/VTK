/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImagePlaneWidget.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkActor.h"
#include "vtkActor2D.h"
#include "vtkCellPicker.h"
#include "vtkImageActor.h"
#include "vtkImageMapToColors.h"
#include "vtkImagePlaneWidget.h"
#include "vtkImageReader.h"
#include "vtkLookupTable.h"
#include "vtkOutlineFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTextMapper.h"
#include "vtkVolume16Reader.h"

#include "vtkRegressionTestImage.h"
#include "vtkDebugLeaks.h"

// Callback for the interaction
class vtkMyCallback : public vtkCommand
{
public:
  static vtkMyCallback *New()
    { return new vtkMyCallback; }
  void Delete()
    { delete this; }
  virtual void Execute(vtkObject *caller, unsigned long, void*)
    {
      vtkImagePlaneWidget *planeWidget = reinterpret_cast<vtkImagePlaneWidget*>(caller);

      sprintf(this->buffer,"%.2f, %d",planeWidget->GetSlicePosition(),planeWidget->GetSliceIndex());
      this->Mapper->SetInput(buffer);
      this->Mapper->Modified();
    }
  vtkMyCallback():Mapper(0){};
  vtkTextMapper* Mapper;
  char buffer[128];
};

int main( int argc, char *argv[] )
{
  vtkDebugLeaks::PromptUserOff();

  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");

  vtkVolume16Reader* v16 =  vtkVolume16Reader::New();
    v16->SetDataDimensions( 64, 64);
    v16->SetDataByteOrderToLittleEndian();    
    v16->SetImageRange( 1, 93);
    v16->SetDataSpacing( 3.2, 3.2, 1.5);
    v16->SetFilePrefix( fname );
    v16->SetDataMask( 0x7fff);
    v16->Update();

  delete[] fname;

  vtkOutlineFilter* outline = vtkOutlineFilter::New();
    outline->SetInput(v16->GetOutput());

  vtkPolyDataMapper* outlineMapper = vtkPolyDataMapper::New();
    outlineMapper->SetInput(outline->GetOutput());

  vtkActor* outlineActor =  vtkActor::New();
    outlineActor->SetMapper( outlineMapper);

  vtkRenderer* ren1 = vtkRenderer::New();
  vtkRenderer* ren2 = vtkRenderer::New();

  vtkRenderWindow* renWin = vtkRenderWindow::New();
    renWin->AddRenderer(ren2);
    renWin->AddRenderer(ren1);

  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

  vtkCellPicker* picker = vtkCellPicker::New();
    picker->SetTolerance(0.005);

  vtkImagePlaneWidget* planeWidgetX = vtkImagePlaneWidget::New();
    planeWidgetX->SetInteractor( iren);
    planeWidgetX->SetKeyPressActivationValue('x');
    planeWidgetX->SetPicker(picker);
    planeWidgetX->GetPlaneProperty()->SetColor(1,0,0);
    planeWidgetX->TextureInterpolateOff();
    planeWidgetX->SetResliceInterpolateToNearestNeighbour();
    planeWidgetX->SetInput(v16->GetOutput());
    planeWidgetX->SetPlaneOrientationToXAxes();
    planeWidgetX->SetSliceIndex(32);
    planeWidgetX->On();

  vtkImagePlaneWidget* planeWidgetY = vtkImagePlaneWidget::New();
    planeWidgetY->SetInteractor( iren);
    planeWidgetY->SetKeyPressActivationValue('y');
    planeWidgetY->SetPicker(picker);
    planeWidgetY->GetPlaneProperty()->SetColor(1,1,0);
    planeWidgetY->TextureInterpolateOn();
    planeWidgetY->SetResliceInterpolateToLinear();
    planeWidgetY->SetInput(v16->GetOutput());
    planeWidgetY->SetPlaneOrientationToYAxes();
    planeWidgetY->SetSlicePosition(102.4);
    planeWidgetY->SetLookupTable( planeWidgetX->GetLookupTable());
    planeWidgetY->On();

  vtkImagePlaneWidget* planeWidgetZ = vtkImagePlaneWidget::New();
    planeWidgetZ->SetInteractor( iren);
    planeWidgetZ->SetKeyPressActivationValue('z');
    planeWidgetZ->SetPicker(picker);
    planeWidgetZ->GetPlaneProperty()->SetColor(0,0,1);
    planeWidgetZ->TextureInterpolateOn();
    planeWidgetZ->SetResliceInterpolateToCubic();
    planeWidgetZ->SetInput(v16->GetOutput());
    planeWidgetZ->SetPlaneOrientationToZAxes();
    planeWidgetZ->SetSliceIndex(25);
    planeWidgetZ->SetLookupTable( planeWidgetX->GetLookupTable());
    planeWidgetZ->On();

  // add text display of planeWidgetZ's slice position and slice index to ren2
  vtkTextMapper* textMapper = vtkTextMapper::New();
    textMapper->SetInput("NA");
    textMapper->SetFontSize(18);
    textMapper->SetFontFamilyToArial();
    textMapper->SetJustificationToLeft();
    textMapper->BoldOn();
    textMapper->ItalicOff();
    textMapper->ShadowOff();

  vtkActor2D* textActor = vtkActor2D::New();
    textActor->SetMapper(textMapper);
    textActor->GetProperty()->SetColor(0,1,1);

  // link the widget to the text by callback
  vtkMyCallback *myCallback = vtkMyCallback::New();
    myCallback->Mapper = textMapper;

  planeWidgetZ->AddObserver(vtkCommand::InteractionEvent,myCallback);

  // add a 2D image to test the GetReslice method
  vtkImageMapToColors* colorMap = vtkImageMapToColors::New();
    colorMap->PassAlphaToOutputOff();
    colorMap->SetActiveComponent(0);
    colorMap->SetOutputFormatToLuminance();
    colorMap->SetInput(planeWidgetZ->GetResliceOutput());
    colorMap->SetLookupTable(planeWidgetX->GetLookupTable());

  vtkImageActor* imageActor = vtkImageActor::New();
    imageActor->PickableOff();
    imageActor->SetInput(colorMap->GetOutput());

  // add the actors
  ren1->AddActor( outlineActor);
  ren2->AddActor( imageActor);
  ren2->AddActor( textActor);

  ren1->SetBackground( 0.1, 0.1, 0.2);
  ren2->SetBackground( 0.2, 0.1, 0.2);

  renWin->SetSize( 600, 350);

  ren1->SetViewport(0,0,0.58333,1);
  ren2->SetViewport(0.58333,0,1,1);

  // set the actors' postions
  renWin->Render();
  iren->SetEventPosition( 175,175);
  iren->SetKeyCode('r');
  iren->InvokeEvent(vtkCommand::CharEvent,NULL);
  iren->SetEventPosition( 475,175);
  iren->SetKeyCode('r');
  iren->InvokeEvent(vtkCommand::CharEvent,NULL);
  int *o = ren2->GetOrigin();
  textActor->SetDisplayPosition(o[0] + 5, o[1] + 5);
  renWin->Render();

  // test SetKeyPressActivationValue for one of the widgets
  iren->SetKeyCode('z');
  iren->InvokeEvent(vtkCommand::CharEvent,NULL);
  iren->SetKeyCode('z');
  iren->InvokeEvent(vtkCommand::CharEvent,NULL);

  ren1->GetActiveCamera()->Elevation(110);
  ren1->GetActiveCamera()->SetViewUp(0, 0, -1);
  ren1->GetActiveCamera()->Azimuth(45);
  ren1->GetActiveCamera()->Dolly(1.15);
  ren1->ResetCameraClippingRange();

  // make the window level work
  int pos1[2] = { 193,187};
  iren->SetEventPosition(pos1);
  iren->InvokeEvent(vtkCommand::RightButtonPressEvent, NULL);
  int pos2[2] = { 105,197};
  iren->SetEventPosition(pos2);
  iren->InvokeEvent(vtkCommand::MouseMoveEvent, NULL);
  iren->InvokeEvent(vtkCommand::RightButtonReleaseEvent, NULL);

  int retVal = vtkRegressionTestImage( renWin );

  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  planeWidgetX->Delete();
  planeWidgetY->Delete();
  planeWidgetZ->Delete();
  colorMap->Delete();
  imageActor->Delete();
  picker->Delete();
  outlineActor->Delete();
  outlineMapper->Delete();
  outline->Delete();
  iren->Delete();
  renWin->Delete();
  ren1->Delete();
  ren2->Delete();
  v16->Delete();
  textMapper->Delete();
  textActor->Delete();
  myCallback->Delete();

  return !retVal;
}
