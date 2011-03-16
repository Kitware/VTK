/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkChartPie.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkChartPie.h"

#include "vtkObjectFactory.h"

#include "vtkContext2D.h"
#include "vtkTransform2D.h"
#include "vtkContextScene.h"
#include "vtkContextMouseEvent.h"
#include "vtkPoints2D.h"

#include "vtkPlotPie.h"

#include "vtkChartLegend.h"
#include "vtkTooltipItem.h"

#include "vtksys/ios/sstream"

class vtkChartPiePrivate
{
  public:
    vtkChartPiePrivate()
      {
      }

  vtkSmartPointer<vtkPlotPie> Plot;
};


//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkChartPie);

//-----------------------------------------------------------------------------
vtkChartPie::vtkChartPie()
{
  this->Legend = vtkChartLegend::New();
  this->Legend->SetChart(this);
  this->Legend->SetVisible(false);
  this->AddItem(this->Legend);
  this->Legend->Delete();

  this->Tooltip = vtkTooltipItem::New();
  this->Tooltip->SetVisible(false);

  this->Private = new vtkChartPiePrivate();
}

//-----------------------------------------------------------------------------
vtkChartPie::~vtkChartPie()
{
  this->Tooltip->Delete();
  delete this->Private;
}

//-----------------------------------------------------------------------------
void vtkChartPie::Update()
{
  if (this->Private->Plot && this->Private->Plot->GetVisible())
    {
    this->Private->Plot->Update();
    }

  this->Legend->Update();
  this->Legend->SetVisible(this->ShowLegend);
}

//-----------------------------------------------------------------------------
bool vtkChartPie::Paint(vtkContext2D *painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  vtkDebugMacro(<< "Paint event called.");

  int geometry[] = { this->GetScene()->GetSceneWidth(),
                     this->GetScene()->GetSceneHeight() };
  if (geometry[0] == 0 || geometry[1] == 0 || !this->Visible)
    {
    // The geometry of the chart must be valid before anything can be drawn
    return false;
    }

  this->Update();

  if ( geometry[0] != this->Geometry[0] || geometry[1] != this->Geometry[1] )
    {
    // Take up the entire window right now, this could be made configurable
    this->SetGeometry(geometry);
    this->SetBorders(20, 20, 20, 20);
    // Put the legend in the top corner of the chart
    vtkRectf rect = this->Legend->GetBoundingRect(painter);
    this->Legend->SetPoint(this->Point2[0] - rect.Width(),
                           this->Point2[1] - rect.Height());

    // Set the dimensions of the Plot
    if (this->Private->Plot)
      {
      this->Private->Plot->SetDimensions(20, 20, this->Geometry[0]-40,
                                         this->Geometry[1]-40);
      }
    }

  this->PaintChildren(painter);

  if (this->Title)
    {
    vtkPoints2D *rect = vtkPoints2D::New();
    rect->InsertNextPoint(this->Point1[0], this->Point2[1]);
    rect->InsertNextPoint(this->Point2[0]-this->Point1[0], 10);
    painter->ApplyTextProp(this->TitleProperties);
    painter->DrawStringRect(rect, this->Title);
    rect->Delete();
    }

  this->Tooltip->Paint(painter);

  return true;
}

//-----------------------------------------------------------------------------
void vtkChartPie::SetScene(vtkContextScene *scene)
{
  this->vtkAbstractContextItem::SetScene(scene);
  this->Tooltip->SetScene(scene);
}

//-----------------------------------------------------------------------------
vtkPlot * vtkChartPie::AddPlot(int /* type */)
{
  if (!this->Private->Plot)
    {
    this->Private->Plot = vtkSmartPointer<vtkPlotPie>::New();
    this->AddItem(this->Private->Plot);
    }
  return this->Private->Plot;
}

//-----------------------------------------------------------------------------
vtkPlot* vtkChartPie::GetPlot(vtkIdType index)
{
  if (index == 0)
    {
    return this->Private->Plot;
    }

  return 0;
}

//-----------------------------------------------------------------------------
vtkIdType vtkChartPie::GetNumberOfPlots()
{
  if (this->Private->Plot)
    {
    return 1;
    }
  else
    {
    return 0;
    }
}

//-----------------------------------------------------------------------------
void vtkChartPie::SetShowLegend(bool visible)
{
  this->vtkChart::SetShowLegend(visible);
  this->Legend->SetVisible(visible);
}

//-----------------------------------------------------------------------------
vtkChartLegend * vtkChartPie::GetLegend()
{
  return this->Legend;
}

//-----------------------------------------------------------------------------
bool vtkChartPie::Hit(const vtkContextMouseEvent &mouse)
{
  if (mouse.ScreenPos[0] > this->Point1[0] &&
      mouse.ScreenPos[0] < this->Point2[0] &&
      mouse.ScreenPos[1] > this->Point1[1] &&
      mouse.ScreenPos[1] < this->Point2[1])
    {
    return true;
    }
  else
    {
    return false;
    }
}

//-----------------------------------------------------------------------------
bool vtkChartPie::MouseEnterEvent(const vtkContextMouseEvent &)
{
  return true;
}

//-----------------------------------------------------------------------------
bool vtkChartPie::MouseMoveEvent(const vtkContextMouseEvent &mouse)
{
  if (mouse.Button == vtkContextMouseEvent::NO_BUTTON)
    {
    this->Scene->SetDirty(true);
    this->Tooltip->SetVisible(this->LocatePointInPlots(mouse));
    }

  return true;
}

//-----------------------------------------------------------------------------
bool vtkChartPie::MouseLeaveEvent(const vtkContextMouseEvent &)
{
  return true;
}

//-----------------------------------------------------------------------------
bool vtkChartPie::MouseButtonPressEvent(const vtkContextMouseEvent &/*mouse*/)
{
  return true;
}

//-----------------------------------------------------------------------------
bool vtkChartPie::MouseButtonReleaseEvent(const vtkContextMouseEvent &/*mouse*/)
{
  return true;
}

//-----------------------------------------------------------------------------
bool vtkChartPie::MouseWheelEvent(const vtkContextMouseEvent &, int /*delta*/)
{
  return true;
}

bool vtkChartPie::LocatePointInPlots(const vtkContextMouseEvent &mouse)
{
  if (!this->Private->Plot || !this->Private->Plot->GetVisible())
    {
    return false;
    }
  else
    {
    int dimensions[4];
    vtkVector2f position(mouse.ScreenPos[0],mouse.ScreenPos[1]);
    vtkVector2f tolerance(5,5);
    vtkVector2f plotPos;
    this->Private->Plot->GetDimensions(dimensions);
    if (mouse.ScreenPos[0] >= dimensions[0] &&
        mouse.ScreenPos[0] <= dimensions[0] + dimensions[2] &&
        mouse.ScreenPos[1] >= dimensions[1] &&
        mouse.ScreenPos[1] <= dimensions[1] + dimensions[3])
      {
      int labelIndex = this->Private->Plot->GetNearestPoint(position,tolerance,&plotPos);
      if (labelIndex >= 0)
        {
        const char *label = this->Private->Plot->GetLabel(labelIndex);
        vtksys_ios::ostringstream ostr;
        ostr << label << ": " << plotPos.X();
        this->Tooltip->SetText(ostr.str().c_str());
        this->Tooltip->SetPosition(mouse.ScreenPos[0]+2,mouse.ScreenPos[1]+2);
        return true;
        }
      }
    }
  return false;
}

//-----------------------------------------------------------------------------
void vtkChartPie::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->Private->Plot)
    {
    os << indent << "Plot: " << endl;
    this->Private->Plot->PrintSelf(os,indent.GetNextIndent());
    }
}
