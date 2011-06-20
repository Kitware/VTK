/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkResliceImageViewer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkResliceImageViewer.h"

#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkImageActor.h"
#include "vtkResliceCursorWidget.h"
#include "vtkResliceCursorLineRepresentation.h"
#include "vtkResliceCursorThickLineRepresentation.h"
#include "vtkResliceCursorActor.h"
#include "vtkResliceCursorPolyDataAlgorithm.h"
#include "vtkPlane.h"
#include "vtkResliceCursor.h"
#include "vtkImageData.h"
#include "vtkImageMapToWindowLevelColors.h"
#include "vtkInteractorStyleImage.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkImageReslice.h"
#include "vtkLookupTable.h"

vtkStandardNewMacro(vtkResliceImageViewer);

//----------------------------------------------------------------------------
vtkResliceImageViewer::vtkResliceImageViewer()
{
  // Default is to not use the reslice cursor widget, ie use fast
  // 3D texture mapping to display slices.
  this->ResliceMode = vtkResliceImageViewer::RESLICE_AXIS_ALIGNED;

  // Set up the reslice cursor widget, should it be used.

  this->ResliceCursorWidget = vtkResliceCursorWidget::New();

  vtkSmartPointer< vtkResliceCursor > resliceCursor =
    vtkSmartPointer< vtkResliceCursor >::New();
  resliceCursor->SetThickMode(0);
  resliceCursor->SetThickness(10, 10, 10);

  vtkSmartPointer< vtkResliceCursorLineRepresentation >
    resliceCursorRep = vtkSmartPointer<
      vtkResliceCursorLineRepresentation >::New();
  resliceCursorRep->GetResliceCursorActor()->
      GetCursorAlgorithm()->SetResliceCursor(resliceCursor);
  resliceCursorRep->GetResliceCursorActor()->
      GetCursorAlgorithm()->SetReslicePlaneNormal(this->SliceOrientation);
  this->ResliceCursorWidget->SetRepresentation(resliceCursorRep);

  this->InstallPipeline();
}

//----------------------------------------------------------------------------
vtkResliceImageViewer::~vtkResliceImageViewer()
{
  if (this->ResliceCursorWidget)
    {
    this->ResliceCursorWidget->Delete();
    this->ResliceCursorWidget = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkResliceImageViewer::SetThickMode( int t )
{
  vtkSmartPointer< vtkResliceCursor > rc = this->GetResliceCursor();

  if (t == this->GetThickMode())
    {
    return;
    }

  vtkSmartPointer< vtkResliceCursorLineRepresentation >
    resliceCursorRepOld = vtkResliceCursorLineRepresentation::SafeDownCast(
                          this->ResliceCursorWidget->GetRepresentation());
  vtkSmartPointer< vtkResliceCursorLineRepresentation > resliceCursorRepNew;

  this->GetResliceCursor()->SetThickMode(t);

  if (t)
    {
    resliceCursorRepNew = vtkSmartPointer<
        vtkResliceCursorThickLineRepresentation >::New();
    }
  else
    {
    resliceCursorRepNew = vtkSmartPointer<
        vtkResliceCursorLineRepresentation >::New();
    }

  int e = this->ResliceCursorWidget->GetEnabled();
  this->ResliceCursorWidget->SetEnabled(0);

  resliceCursorRepNew->GetResliceCursorActor()->
      GetCursorAlgorithm()->SetResliceCursor(rc);
  resliceCursorRepNew->GetResliceCursorActor()->
      GetCursorAlgorithm()->SetReslicePlaneNormal(this->SliceOrientation);
  this->ResliceCursorWidget->SetRepresentation(resliceCursorRepNew);
  resliceCursorRepNew->SetLookupTable(resliceCursorRepOld->GetLookupTable());

  resliceCursorRepNew->SetWindowLevel(
      resliceCursorRepOld->GetWindow(),
      resliceCursorRepOld->GetLevel(), 1);

  this->ResliceCursorWidget->SetEnabled(e);
}

//----------------------------------------------------------------------------
int vtkResliceImageViewer::GetThickMode()
{
  return (vtkResliceCursorThickLineRepresentation::
    SafeDownCast(this->ResliceCursorWidget->GetRepresentation())) ? 1 : 0;
}

//----------------------------------------------------------------------------
void vtkResliceImageViewer::SetLookupTable( vtkLookupTable * l )
{
  if (vtkResliceCursorRepresentation *rep =
        vtkResliceCursorRepresentation::SafeDownCast(
          this->ResliceCursorWidget->GetRepresentation()))
    {
    rep->SetLookupTable(l);
    }

  if (this->WindowLevel)
    {
    this->WindowLevel->SetLookupTable(l);
    this->WindowLevel->SetOutputFormatToRGBA();
    this->WindowLevel->PassAlphaToOutputOn();
    }
}

//----------------------------------------------------------------------------
vtkLookupTable * vtkResliceImageViewer::GetLookupTable()
{
  if (vtkResliceCursorRepresentation *rep =
        vtkResliceCursorRepresentation::SafeDownCast(
          this->ResliceCursorWidget->GetRepresentation()))
    {
    return rep->GetLookupTable();
    }

  return NULL;
}

//----------------------------------------------------------------------------
void vtkResliceImageViewer::UpdateOrientation()
{
  if (this->ResliceMode == RESLICE_AXIS_ALIGNED)
    {
    this->Superclass::UpdateOrientation();
    }
}

//----------------------------------------------------------------------------
void vtkResliceImageViewer::UpdateDisplayExtent()
{
  if (this->ResliceMode == RESLICE_AXIS_ALIGNED)
    {
    this->Superclass::UpdateDisplayExtent();
    }
}

//----------------------------------------------------------------------------
void vtkResliceImageViewer::InstallPipeline()
{
  this->Superclass::InstallPipeline();

  if (this->Interactor)
    {
    this->ResliceCursorWidget->SetInteractor(this->Interactor);
    }

  if (this->Renderer)
    {
    this->ResliceCursorWidget->SetDefaultRenderer(this->Renderer);
    vtkCamera *cam = this->Renderer->GetActiveCamera();
    cam->ParallelProjectionOn();
    }

  if (this->ResliceMode == RESLICE_OBLIQUE)
    {
    this->ResliceCursorWidget->SetEnabled(1);
    this->ImageActor->SetVisibility(0);
    this->Superclass::UpdateOrientation();

    double bounds[6];

    vtkCamera *cam = this->Renderer->GetActiveCamera();
    this->GetResliceCursor()->GetImage()->GetBounds(bounds);
    double *spacing = this->GetResliceCursor()->GetImage()->GetSpacing();
    double avg_spacing =
      (spacing[0] + spacing[1] + spacing[2]) / 3.0;
    cam->SetClippingRange(
      bounds[this->SliceOrientation * 2] - 100 * avg_spacing,
      bounds[this->SliceOrientation * 2 + 1] + 100 * avg_spacing);
    }
  else
    {
    this->ResliceCursorWidget->SetEnabled(0);
    this->ImageActor->SetVisibility(1);
    this->Superclass::UpdateOrientation();
    }

  if (this->WindowLevel)
    {
    this->WindowLevel->SetLookupTable(this->GetLookupTable());
    }
}

//----------------------------------------------------------------------------
void vtkResliceImageViewer::UnInstallPipeline()
{
  this->ResliceCursorWidget->SetEnabled(0);

  this->Superclass::UnInstallPipeline();
}

//----------------------------------------------------------------------------
void vtkResliceImageViewer::Render()
{
  this->Superclass::Render();
}

//----------------------------------------------------------------------------
vtkResliceCursor * vtkResliceImageViewer::GetResliceCursor()
{
  if (vtkResliceCursorRepresentation *rep =
        vtkResliceCursorRepresentation::SafeDownCast(
          this->ResliceCursorWidget->GetRepresentation()))
    {
    return rep->GetResliceCursor();
    }

  return NULL;
}

//----------------------------------------------------------------------------
void vtkResliceImageViewer::SetInput(vtkImageData *in)
{
  this->WindowLevel->SetInput(in);
  this->GetResliceCursor()->SetImage(in);
  this->GetResliceCursor()->SetCenter(in->GetCenter());
  this->UpdateDisplayExtent();

  double range[2];
  in->GetScalarRange(range);
  if (vtkResliceCursorRepresentation *rep =
        vtkResliceCursorRepresentation::SafeDownCast(
          this->ResliceCursorWidget->GetRepresentation()))
    {
    if (vtkImageReslice *reslice =
        vtkImageReslice::SafeDownCast(rep->GetReslice()))
      {
      // default background color is the min value of the image scalar range
      reslice->SetBackgroundColor(range[0],range[0],range[0],range[0]);
      this->SetColorWindow(range[1]-range[0]);
      this->SetColorLevel((range[0]+range[1])/2.0);
      }
    }
}

//----------------------------------------------------------------------------
void vtkResliceImageViewer::SetInputConnection(vtkAlgorithmOutput* input)
{
  vtkErrorMacro( << "Use SetInput instead. " );
  this->WindowLevel->SetInputConnection(input);
  this->UpdateDisplayExtent();
}

//----------------------------------------------------------------------------
void vtkResliceImageViewer::SetResliceMode( int r )
{
  if (r == this->ResliceMode)
    {
    return;
    }

  this->ResliceMode = r;
  this->Modified();

  this->InstallPipeline();
}

//----------------------------------------------------------------------------
void vtkResliceImageViewer::SetColorWindow( double w )
{
  double rmin = this->GetColorLevel() - 0.5*fabs( w );
  double rmax = rmin + fabs( w );
  this->GetLookupTable()->SetTableRange( rmin, rmax );

  this->WindowLevel->SetWindow(w);
  if (vtkResliceCursorRepresentation *rep =
        vtkResliceCursorRepresentation::SafeDownCast(
          this->ResliceCursorWidget->GetRepresentation()))
    {
    rep->SetWindowLevel(w, rep->GetLevel(), 1);
    }
}

//----------------------------------------------------------------------------
void vtkResliceImageViewer::SetColorLevel( double w )
{
  double rmin = w - 0.5*fabs( this->GetColorWindow() );
  double rmax = rmin + fabs( this->GetColorWindow() );
  this->GetLookupTable()->SetTableRange( rmin, rmax );

  this->WindowLevel->SetLevel(w);
  if (vtkResliceCursorRepresentation *rep =
        vtkResliceCursorRepresentation::SafeDownCast(
          this->ResliceCursorWidget->GetRepresentation()))
    {
    rep->SetWindowLevel(rep->GetWindow(), w, 1);
    }
}

//----------------------------------------------------------------------------
void vtkResliceImageViewer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ResliceCursorWidget:\n";
  this->ResliceCursorWidget->PrintSelf(os,indent.GetNextIndent());
  os << indent << "ResliceMode: " << this->ResliceMode << endl;
}
