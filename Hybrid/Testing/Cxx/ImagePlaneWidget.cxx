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
#include "vtkVolume16Reader.h"
#include "vtkOutlineFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkImagePlaneWidget.h"
#include "vtkImageReader.h"
#include "vtkCellPicker.h"

#include "vtkRegressionTestImage.h"
#include "vtkDebugLeaks.h"

int main( int argc, char *argv[] )
{
  const char *temp=getenv("VTK_DATA_ROOT");
  char filename[2048];
  
  if ( ! temp )
    {
    cerr << "ERROR: Please define environment variable VTK_DATA_ROOT\n";
    return 1;
    }
  strcpy(filename,temp);
  strcat(filename,"/Data/headsq/quarter");
  
  vtkVolume16Reader* v16 =  vtkVolume16Reader::New();
    v16->SetDataDimensions( 64, 64);
    v16->SetDataByteOrderToLittleEndian();
    v16->SetFilePrefix( filename );
    v16->SetImageRange( 1, 93);
    v16->SetDataSpacing( 3.2, 3.2, 1.5);
    v16->Update();

  vtkOutlineFilter* outline = vtkOutlineFilter::New();
    outline->SetInput(v16->GetOutput());

  vtkPolyDataMapper* outlineMapper = vtkPolyDataMapper::New();
    outlineMapper->SetInput(outline->GetOutput());

  vtkActor* outlineActor =  vtkActor::New();
    outlineActor->SetMapper( outlineMapper);

  vtkRenderer*       ren1 = vtkRenderer::New();

  vtkRenderWindow* renWin = vtkRenderWindow::New();
    renWin->AddRenderer( ren1);

  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow( renWin);

  vtkCellPicker* picker = vtkCellPicker::New();
    picker->SetTolerance(0.005); 
    picker->PickFromListOn();

  vtkImagePlaneWidget* planeWidgetX = vtkImagePlaneWidget::New();
  planeWidgetX->SetInput(v16->GetOutput());  
  planeWidgetX->SetInteractor( iren);
  planeWidgetX->SetPlaneOrientationToXAxes();  
  planeWidgetX->SetSliceIndex(32);
  planeWidgetX->SetPicker(picker);
  planeWidgetX->GetPlaneProperty()->SetColor(1,0,0);
  planeWidgetX->On();

  vtkImagePlaneWidget* planeWidgetY = vtkImagePlaneWidget::New();
  planeWidgetY->SetInput(v16->GetOutput());
  planeWidgetY->SetInteractor( iren);
  planeWidgetY->SetPlaneOrientationToYAxes();
  planeWidgetY->SetSliceIndex(32);
  planeWidgetY->SetPicker(picker);
  planeWidgetY->GetPlaneProperty()->SetColor(1,1,0);
  planeWidgetY->On();

  vtkImagePlaneWidget* planeWidgetZ = vtkImagePlaneWidget::New();
  planeWidgetZ->SetInput(v16->GetOutput());
  planeWidgetZ->SetInteractor( iren);
  planeWidgetZ->SetPlaneOrientationToZAxes();
  planeWidgetZ->SetSliceIndex(46);
  planeWidgetZ->SetPicker(picker);
  planeWidgetZ->GetPlaneProperty()->SetColor(0,0,1);
  planeWidgetZ->On();

  ren1->AddActor( outlineActor);

  ren1->SetBackground( 0.1, 0.1, 0.2);
  renWin->SetSize( 300, 300);

  renWin->Render();
  iren->SetKeyCode('r');
  iren->InvokeEvent(vtkCommand::CharEvent,NULL);
  renWin->Render();

  ren1->GetActiveCamera()->Elevation(110);
  ren1->GetActiveCamera()->SetViewUp(0, 0, -1);
  ren1->GetActiveCamera()->Azimuth(45);
  ren1->ResetCameraClippingRange();

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  planeWidgetX->Delete();
  planeWidgetY->Delete();
  planeWidgetZ->Delete();
  picker->Delete();
  outlineActor->Delete();
  outlineMapper->Delete();
  outline->Delete();
  iren->Delete();
  renWin->Delete();
  ren1->Delete();
  v16->Delete();
  
  return 0;
}
