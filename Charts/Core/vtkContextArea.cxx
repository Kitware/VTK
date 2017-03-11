/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextArea.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkContextArea.h"

#include "vtkContext2D.h"
#include "vtkContextClip.h"
#include "vtkContextDevice2D.h"
#include "vtkContextTransform.h"
#include "vtkObjectFactory.h"
#include "vtkPlotGrid.h"

#include <algorithm>
#include <cstdlib>

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkContextArea)

//------------------------------------------------------------------------------
vtkContextArea::vtkContextArea()
  : Geometry(0, 0, 300, 300),
    DrawAreaBounds(0, 0, 300, 300),
    DrawAreaGeometry(0, 0, 300, 300),
    DrawAreaResizeBehavior(DARB_Expand),
    FixedAspect(1.f),
    FixedRect(0, 0, 300, 300),
    FixedMargins(0),
    FillViewport(true)
{
  this->Axes[vtkAxis::TOP] = this->TopAxis.GetPointer();
  this->Axes[vtkAxis::BOTTOM] = this->BottomAxis.GetPointer();
  this->Axes[vtkAxis::LEFT] = this->LeftAxis.GetPointer();
  this->Axes[vtkAxis::RIGHT] = this->RightAxis.GetPointer();

  this->Grid->SetXAxis(this->BottomAxis.GetPointer());
  this->Grid->SetYAxis(this->LeftAxis.GetPointer());

  this->Axes[vtkAxis::TOP]->SetPosition(vtkAxis::TOP);
  this->Axes[vtkAxis::BOTTOM]->SetPosition(vtkAxis::BOTTOM);
  this->Axes[vtkAxis::LEFT]->SetPosition(vtkAxis::LEFT);
  this->Axes[vtkAxis::RIGHT]->SetPosition(vtkAxis::RIGHT);

  this->InitializeDrawArea();
}

//------------------------------------------------------------------------------
vtkContextArea::~vtkContextArea()
{
}

//------------------------------------------------------------------------------
void vtkContextArea::InitializeDrawArea()
{
  for (int i = 0; i < 4; ++i)
  {
    this->AddItem(this->Axes[i]);
  }

  this->Clip->AddItem(this->Transform.GetPointer());
  this->Clip->AddItem(this->Grid.GetPointer());
  this->AddItem(this->Clip.GetPointer());
}

//------------------------------------------------------------------------------
void vtkContextArea::LayoutAxes(vtkContext2D *painter)
{
  // Shorter names for compact readability:
  vtkRectd &data = this->DrawAreaBounds;
  vtkRecti &draw = this->DrawAreaGeometry;

  this->SetAxisRange(data);
  draw = this->ComputeDrawAreaGeometry(painter);

  // Set axes locations to the most recent draw rect:
  this->TopAxis->SetPoint1(draw.GetTopLeft().Cast<float>());
  this->TopAxis->SetPoint2(draw.GetTopRight().Cast<float>());
  this->BottomAxis->SetPoint1(draw.GetBottomLeft().Cast<float>());
  this->BottomAxis->SetPoint2(draw.GetBottomRight().Cast<float>());
  this->LeftAxis->SetPoint1(draw.GetBottomLeft().Cast<float>());
  this->LeftAxis->SetPoint2(draw.GetTopLeft().Cast<float>());
  this->RightAxis->SetPoint1(draw.GetBottomRight().Cast<float>());
  this->RightAxis->SetPoint2(draw.GetTopRight().Cast<float>());

  // Regenerate ticks, labels, etc:
  for (int i = 0; i < 4; ++i)
  {
    this->Axes[i]->Update();
  }
}

//------------------------------------------------------------------------------
void vtkContextArea::SetAxisRange(vtkRectd const& data)
{
  // Set the data bounds
  this->TopAxis->SetRange(data.GetLeft(), data.GetRight());
  this->BottomAxis->SetRange(data.GetLeft(), data.GetRight());
  this->LeftAxis->SetRange(data.GetBottom(), data.GetTop());
  this->RightAxis->SetRange(data.GetBottom(), data.GetTop());
}

//------------------------------------------------------------------------------
vtkRecti vtkContextArea::ComputeDrawAreaGeometry(vtkContext2D *p)
{
  switch (this->DrawAreaResizeBehavior)
  {
    case vtkContextArea::DARB_Expand:
      return this->ComputeExpandedDrawAreaGeometry(p);
    case vtkContextArea::DARB_FixedAspect:
      return this->ComputeFixedAspectDrawAreaGeometry(p);
    case vtkContextArea::DARB_FixedRect:
      return this->ComputeFixedRectDrawAreaGeometry(p);
    case vtkContextArea::DARB_FixedMargins:
      return this->ComputeFixedMarginsDrawAreaGeometry(p);
    default:
      vtkErrorMacro("Invalid resize behavior enum value: "
                    << this->DrawAreaResizeBehavior);
      break;
  }

  return vtkRecti();
}

//------------------------------------------------------------------------------
vtkRecti vtkContextArea::ComputeExpandedDrawAreaGeometry(vtkContext2D *painter)
{
  // Shorter names for compact readability:
  vtkRecti &geo = this->Geometry;

  // Set the axes positions. We iterate up to 3 times to converge on the margins.
  vtkRecti draw(this->DrawAreaGeometry); // Start with last attempt
  vtkRecti lastDraw;
  for (int pass = 0; pass < 3; ++pass)
  {
    // Set axes locations to the current draw rect:
    this->TopAxis->SetPoint1(draw.GetTopLeft().Cast<float>());
    this->TopAxis->SetPoint2(draw.GetTopRight().Cast<float>());
    this->BottomAxis->SetPoint1(draw.GetBottomLeft().Cast<float>());
    this->BottomAxis->SetPoint2(draw.GetBottomRight().Cast<float>());
    this->LeftAxis->SetPoint1(draw.GetBottomLeft().Cast<float>());
    this->LeftAxis->SetPoint2(draw.GetTopLeft().Cast<float>());
    this->RightAxis->SetPoint1(draw.GetBottomRight().Cast<float>());
    this->RightAxis->SetPoint2(draw.GetTopRight().Cast<float>());

    // Calculate axes bounds compute new draw geometry:
    vtkVector2i bottomLeft = draw.GetBottomLeft();
    vtkVector2i topRight = draw.GetTopRight();
    for (int i = 0; i < 4; ++i)
    {
      this->Axes[i]->Update();
      vtkRectf bounds = this->Axes[i]->GetBoundingRect(painter);
      switch (static_cast<vtkAxis::Location>(i))
      {
        case vtkAxis::LEFT:
          bottomLeft.SetX(geo.GetLeft() +
                          static_cast<int>(bounds.GetWidth()));
          break;
        case vtkAxis::BOTTOM:
          bottomLeft.SetY(geo.GetBottom() +
                          static_cast<int>(bounds.GetHeight()));
          break;
        case vtkAxis::RIGHT:
          topRight.SetX(geo.GetRight() -
                        static_cast<int>(bounds.GetWidth()));
          break;
        case vtkAxis::TOP:
          topRight.SetY(geo.GetTop() -
                        static_cast<int>(bounds.GetHeight()));
          break;
        default:
          abort(); // Shouldn't happen unless vtkAxis::Location is changed.
      }
    }

    // Update draw geometry:
    lastDraw = draw;
    draw.Set(bottomLeft.GetX(), bottomLeft.GetY(),
             topRight.GetX() - bottomLeft.GetX(),
             topRight.GetY() - bottomLeft.GetY());
    if (draw == lastDraw)
    {
      break; // converged
    }
  }

  return draw;
}

//------------------------------------------------------------------------------
vtkRecti vtkContextArea::ComputeFixedAspectDrawAreaGeometry(vtkContext2D *p)
{
  vtkRecti draw = this->ComputeExpandedDrawAreaGeometry(p);
  float aspect = draw.GetWidth() / static_cast<float>(draw.GetHeight());

  if (aspect > this->FixedAspect) // Too wide:
  {
    int targetWidth = vtkContext2D::FloatToInt(
          this->FixedAspect * draw.GetHeight());
    int delta = draw.GetWidth() - targetWidth;
    draw.SetX(draw.GetX() + (delta/2));
    draw.SetWidth(targetWidth);
  }
  else if (aspect < this->FixedAspect) // Too tall:
  {
    int targetHeight = vtkContext2D::FloatToInt(
          draw.GetWidth() / this->FixedAspect);
    int delta = draw.GetHeight() - targetHeight;
    draw.SetY(draw.GetY() + (delta/2));
    draw.SetHeight(targetHeight);
  }

  return draw;
}

//------------------------------------------------------------------------------
vtkRecti vtkContextArea::ComputeFixedRectDrawAreaGeometry(vtkContext2D *)
{
  return this->FixedRect;
}

//------------------------------------------------------------------------------
vtkRecti vtkContextArea::ComputeFixedMarginsDrawAreaGeometry(vtkContext2D *)
{
  return vtkRecti(this->FixedMargins[0], this->FixedMargins[2],
                  this->Geometry.GetWidth() - (this->FixedMargins[0] +
                                               this->FixedMargins[1]),
                  this->Geometry.GetHeight() - (this->FixedMargins[2] +
                                                this->FixedMargins[3]));
}

//------------------------------------------------------------------------------
void vtkContextArea::UpdateDrawArea()
{
  // Shorter names for compact readability:
  vtkRecti &draw = this->DrawAreaGeometry;

  // Setup clipping:
  this->Clip->SetClip(static_cast<float>(draw.GetX()),
                      static_cast<float>(draw.GetY()),
                      static_cast<float>(draw.GetWidth()),
                      static_cast<float>(draw.GetHeight()));

  this->ComputeViewTransform();
}

//------------------------------------------------------------------------------
void vtkContextArea::ComputeViewTransform()
{
  vtkRectd const& data = this->DrawAreaBounds;
  vtkRecti const& draw = this->DrawAreaGeometry;

  this->Transform->Identity();
  this->Transform->Translate(draw.GetX(), draw.GetY());
  this->Transform->Scale(draw.GetWidth() / data.GetWidth(),
                         draw.GetHeight() / data.GetHeight());
  this->Transform->Translate(-data.GetX(), -data.GetY());
}

//------------------------------------------------------------------------------
void vtkContextArea::PrintSelf(std::ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

#define vtkContextAreaPrintMemberObject(name) \
  os << indent << #name ":\n"; \
  this->name->PrintSelf(os, indent.GetNextIndent())
#define vtkContextAreaPrintMemberPOD(name) \
  os << indent << #name ": " << this->name << "\n"

  vtkContextAreaPrintMemberObject(TopAxis);
  vtkContextAreaPrintMemberObject(BottomAxis);
  vtkContextAreaPrintMemberObject(LeftAxis);
  vtkContextAreaPrintMemberObject(RightAxis);
  vtkContextAreaPrintMemberObject(Grid);
  vtkContextAreaPrintMemberObject(Transform);
  vtkContextAreaPrintMemberPOD(Geometry);
  vtkContextAreaPrintMemberPOD(DrawAreaBounds);
  vtkContextAreaPrintMemberPOD(DrawAreaGeometry);
  os << indent << "DrawAreaResizeBehavior: ";
  switch (this->DrawAreaResizeBehavior)
  {
    case vtkContextArea::DARB_Expand:
      os << "DARB_Expand\n";
      break;
    case vtkContextArea::DARB_FixedAspect:
      os << "DARB_FixedAspect\n";
      break;
    case vtkContextArea::DARB_FixedRect:
      os << "DARB_FixedRect\n";
      break;
    case vtkContextArea::DARB_FixedMargins:
      os << "DARB_FixedMargins\n";
      break;
    default:
      os << "(Invalid enum value: " << this->DrawAreaResizeBehavior << ")\n";
      break;
  }
  vtkContextAreaPrintMemberPOD(FixedAspect);
  vtkContextAreaPrintMemberPOD(FixedRect);
  vtkContextAreaPrintMemberPOD(FixedMargins);
  vtkContextAreaPrintMemberPOD(FillViewport);

#undef vtkContextAreaPrintMemberPOD
#undef vtkContextAreaPrintMemberObject
}

//------------------------------------------------------------------------------
vtkAxis *vtkContextArea::GetAxis(vtkAxis::Location location)
{
  return location < 4 ? this->Axes[location] : NULL;
}

//------------------------------------------------------------------------------
vtkAbstractContextItem *vtkContextArea::GetDrawAreaItem()
{
  return this->Transform.GetPointer();
}

//------------------------------------------------------------------------------
bool vtkContextArea::Paint(vtkContext2D *painter)
{
  if (this->FillViewport)
  {
    vtkVector2i vpSize = painter->GetDevice()->GetViewportSize();
    this->SetGeometry(vtkRecti(0, 0, vpSize[0], vpSize[1]));
  }

  this->LayoutAxes(painter);
  this->UpdateDrawArea();
  return this->Superclass::Paint(painter);
}

//------------------------------------------------------------------------------
void vtkContextArea::SetFixedAspect(float aspect)
{
  this->SetDrawAreaResizeBehavior(DARB_FixedAspect);
  if (this->FixedAspect != aspect)
  {
    this->FixedAspect = aspect;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkContextArea::SetFixedRect(vtkRecti rect)
{
  this->SetDrawAreaResizeBehavior(DARB_FixedRect);
  if (this->FixedRect != rect)
  {
    this->FixedRect = rect;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkContextArea::SetFixedRect(int x, int y, int width, int height)
{
  this->SetFixedRect(vtkRecti(x, y, width, height));
}

//------------------------------------------------------------------------------
void vtkContextArea::GetFixedMarginsArray(int margins[4])
{
  std::copy(this->FixedMargins.GetData(), this->FixedMargins.GetData() + 4,
            margins);
}

//------------------------------------------------------------------------------
const int* vtkContextArea::GetFixedMarginsArray()
{
  return this->FixedMargins.GetData();
}

//------------------------------------------------------------------------------
void vtkContextArea::SetFixedMargins(vtkContextArea::Margins margins)
{
  this->SetDrawAreaResizeBehavior(DARB_FixedMargins);
  if (margins != this->FixedMargins)
  {
    this->FixedMargins = margins;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkContextArea::SetFixedMargins(int margins[4])
{
  this->SetFixedMargins(Margins(margins));
}

//------------------------------------------------------------------------------
void vtkContextArea::SetFixedMargins(int left, int right, int bottom, int top)
{
  Margins margins;
  margins[0] = left;
  margins[1] = right;
  margins[2] = bottom;
  margins[3] = top;
  this->SetFixedMargins(margins);
}

//------------------------------------------------------------------------------
void vtkContextArea::SetShowGrid(bool show)
{
  this->Grid->SetVisible(show);
}

//------------------------------------------------------------------------------
bool vtkContextArea::GetShowGrid()
{
  return this->Grid->GetVisible();
}
