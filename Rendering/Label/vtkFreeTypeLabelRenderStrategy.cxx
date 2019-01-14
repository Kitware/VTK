/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFreeTypeLabelRenderStrategy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFreeTypeLabelRenderStrategy.h"

#include "vtkActor2D.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTextMapper.h"
#include "vtkTextProperty.h"
#include "vtkTextRenderer.h"
#include "vtkTimerLog.h"
#include "vtkWindow.h"

vtkStandardNewMacro(vtkFreeTypeLabelRenderStrategy);

//----------------------------------------------------------------------------
vtkFreeTypeLabelRenderStrategy::vtkFreeTypeLabelRenderStrategy()
{
  this->TextRenderer = vtkTextRenderer::GetInstance();
  this->Mapper = vtkTextMapper::New();
  this->Actor = vtkActor2D::New();
  this->Actor->SetMapper(this->Mapper);
}

//----------------------------------------------------------------------------
vtkFreeTypeLabelRenderStrategy::~vtkFreeTypeLabelRenderStrategy()
{
  this->Mapper->Delete();
  this->Actor->Delete();
}

void vtkFreeTypeLabelRenderStrategy::ReleaseGraphicsResources(vtkWindow *window)
{
  this->Actor->ReleaseGraphicsResources(window);
}

//double compute_bounds_time1 = 0;
//int compute_bounds_iter1 = 0;
//----------------------------------------------------------------------------
void vtkFreeTypeLabelRenderStrategy::ComputeLabelBounds(
  vtkTextProperty* tprop, vtkUnicodeString label, double bds[4])
{
  //vtkTimerLog* timer = vtkTimerLog::New();
  //timer->StartTimer();

  // Check for empty string.
  vtkStdString str;
  label.utf8_str(str);
  if (str.length() == 0)
  {
    bds[0] = 0;
    bds[1] = 0;
    bds[2] = 0;
    bds[3] = 0;
    return;
  }

  if (!tprop)
  {
    tprop = this->DefaultTextProperty;
  }
  vtkSmartPointer<vtkTextProperty> copy = tprop;
  if (tprop->GetOrientation() != 0.0)
  {
    copy = vtkSmartPointer<vtkTextProperty>::New();
    copy->ShallowCopy(tprop);
    copy->SetOrientation(0.0);
  }

  int dpi = 72;
  if (this->Renderer && this->Renderer->GetVTKWindow())
  {
    dpi = this->Renderer->GetVTKWindow()->GetDPI();
  }
  else
  {
    vtkWarningMacro(<<"No Renderer set. Assuming DPI of " << dpi << ".");
  }

  int bbox[4];
  this->TextRenderer->GetBoundingBox(copy, label.utf8_str(), bbox, dpi);

  // Take line offset into account
  bds[0] = bbox[0];
  bds[1] = bbox[1];
  bds[2] = bbox[2] - tprop->GetLineOffset();
  bds[3] = bbox[3] - tprop->GetLineOffset();

  // Take justification into account
  double sz[2] = {bds[1] - bds[0], bds[3] - bds[2]};
  switch (tprop->GetJustification())
  {
    case VTK_TEXT_LEFT:
      break;
    case VTK_TEXT_CENTERED:
      bds[0] -= sz[0]/2;
      bds[1] -= sz[0]/2;
      break;
    case VTK_TEXT_RIGHT:
      bds[0] -= sz[0];
      bds[1] -= sz[0];
      break;
  }
  switch (tprop->GetVerticalJustification())
  {
    case VTK_TEXT_BOTTOM:
      break;
    case VTK_TEXT_CENTERED:
      bds[2] -= sz[1]/2;
      bds[3] -= sz[1]/2;
      break;
    case VTK_TEXT_TOP:
      bds[2] -= sz[1];
      bds[3] -= sz[1];
      break;
  }
  //timer->StopTimer();
  //compute_bounds_time1 += timer->GetElapsedTime();
  //compute_bounds_iter1++;
  //if (compute_bounds_iter1 % 10000 == 0)
  //  {
  //  cerr << "ComputeLabelBounds time: " << (compute_bounds_time1 / compute_bounds_iter1) << endl;
  //  }
}

//double render_label_time1 = 0;
//int render_label_iter1 = 0;
//----------------------------------------------------------------------------
void vtkFreeTypeLabelRenderStrategy::RenderLabel(
  int x[2], vtkTextProperty* tprop, vtkUnicodeString label)
{
  //vtkTimerLog* timer = vtkTimerLog::New();
  //timer->StartTimer();

  if (!this->Renderer)
  {
    vtkErrorMacro("Renderer must be set before rendering labels.");
    return;
  }
  if (!tprop)
  {
    tprop = this->DefaultTextProperty;
  }
  this->Mapper->SetTextProperty(tprop);
  this->Mapper->SetInput(label.utf8_str());
  this->Actor->GetPositionCoordinate()->SetCoordinateSystemToDisplay();
  this->Actor->GetPositionCoordinate()->SetValue(x[0], x[1], 0.0);
  this->Mapper->RenderOverlay(this->Renderer, this->Actor);
  //timer->StopTimer();
  //render_label_time1 += timer->GetElapsedTime();
  //render_label_iter1++;
  //if (render_label_iter1 % 100 == 0)
  //  {
  //  cerr << "RenderLabel time: " << (render_label_time1 / render_label_iter1) << endl;
  //  }
}

//----------------------------------------------------------------------------
void vtkFreeTypeLabelRenderStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
