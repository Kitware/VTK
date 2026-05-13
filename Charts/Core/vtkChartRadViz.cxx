// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkChartRadViz.h"

#include "vtkContext2D.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextScene.h"
#include "vtkObjectFactory.h"
#include "vtkPlotRadViz.h"
#include "vtkPoints2D.h"
#include "vtkTextProperty.h"

#include <sstream>

VTK_ABI_NAMESPACE_BEGIN
class vtkChartRadVizPrivate
{
public:
  vtkChartRadVizPrivate() = default;

  vtkSmartPointer<vtkPlotRadViz> Plot;
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkChartRadViz);

//------------------------------------------------------------------------------
vtkChartRadViz::vtkChartRadViz()
{
  this->Private = new vtkChartRadVizPrivate();
}

//------------------------------------------------------------------------------
vtkChartRadViz::~vtkChartRadViz()
{
  delete this->Private;
}

//------------------------------------------------------------------------------
void vtkChartRadViz::Update()
{
  if (this->Private->Plot && this->Private->Plot->GetVisible())
  {
    this->Private->Plot->Update();
  }
}

//------------------------------------------------------------------------------
bool vtkChartRadViz::Paint(vtkContext2D* painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  vtkDebugMacro(<< "Paint event called.");

  int geometry[] = { this->GetScene()->GetSceneWidth(), this->GetScene()->GetSceneHeight() };
  if (geometry[0] == 0 || geometry[1] == 0 || !this->Visible)
  {
    // The geometry of the chart must be valid before anything can be drawn
    return false;
  }

  this->Update();

  if (this->LayoutStrategy == vtkChart::FILL_SCENE &&
    (geometry[0] != this->Geometry[0] || geometry[1] != this->Geometry[1]))
  {
    this->SetSize(vtkRectf(0.0, 0.0, geometry[0], geometry[1]));
  }

  vtkVector2i tileScale = this->Scene->GetLogicalTileScale();
  const int BORDER_SIZE = 20;
  this->SetBorders(BORDER_SIZE * tileScale.GetX(), BORDER_SIZE * tileScale.GetY(),
    BORDER_SIZE * tileScale.GetX(), BORDER_SIZE * tileScale.GetY());

  // Set the dimensions of the Plot
  if (this->Private->Plot)
  {
    this->Private->Plot->SetDimensions(this->Size.GetX() + BORDER_SIZE,
      this->Size.GetY() + BORDER_SIZE, this->Geometry[0] - 2 * BORDER_SIZE,
      this->Geometry[1] - 2 * BORDER_SIZE);
  }

  this->PaintChildren(painter);

  if (!this->Title.empty())
  {
    int fontSize = this->TitleProperties->GetFontSize();
    vtkPoints2D* rect = vtkPoints2D::New();
    rect->InsertNextPoint(this->Point1[0], this->Point2[1]);
    rect->InsertNextPoint(this->Point2[0] - this->Point1[0], fontSize);
    painter->ApplyTextProp(this->TitleProperties);
    painter->DrawStringRect(rect, this->Title);
    rect->Delete();
  }

  return true;
}

//------------------------------------------------------------------------------
void vtkChartRadViz::SetScene(vtkContextScene* scene)
{
  this->vtkAbstractContextItem::SetScene(scene);
}

//------------------------------------------------------------------------------
vtkPlot* vtkChartRadViz::AddPlot(int /* type */)
{
  if (!this->Private->Plot)
  {
    this->Private->Plot = vtkSmartPointer<vtkPlotRadViz>::New();
    this->AddItem(this->Private->Plot);
  }
  return this->Private->Plot;
}

//------------------------------------------------------------------------------
void vtkChartRadViz::SetPlot(vtkPlotRadViz* plot)
{
  if (this->Private->Plot)
  {
    this->RemoveItem(this->Private->Plot);
  }
  this->Private->Plot = plot;
  this->AddItem(this->Private->Plot);
  // Ensure legend remains on top
  this->Modified();
}

//------------------------------------------------------------------------------
vtkPlot* vtkChartRadViz::GetPlot(vtkIdType index)
{
  if (index == 0)
  {
    return this->Private->Plot;
  }

  return nullptr;
}

//------------------------------------------------------------------------------
vtkIdType vtkChartRadViz::GetNumberOfPlots()
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

//------------------------------------------------------------------------------
bool vtkChartRadViz::Hit(const vtkContextMouseEvent& mouse)
{
  vtkVector2f pos(mouse.GetScenePos());
  return pos[0] > this->Point1[0] && pos[0] < this->Point2[0] && pos[1] > this->Point1[1] &&
    pos[1] < this->Point2[1];
}

//------------------------------------------------------------------------------
bool vtkChartRadViz::MouseEnterEvent(const vtkContextMouseEvent&)
{
  return true;
}

//------------------------------------------------------------------------------
bool vtkChartRadViz::MouseMoveEvent(const vtkContextMouseEvent& mouse)
{
  if (mouse.GetButton() == vtkContextMouseEvent::NO_BUTTON)
  {
    this->Scene->SetDirty(true);
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkChartRadViz::MouseLeaveEvent(const vtkContextMouseEvent&)
{
  return true;
}

//------------------------------------------------------------------------------
bool vtkChartRadViz::MouseButtonPressEvent(const vtkContextMouseEvent& /*mouse*/)
{
  return true;
}

//------------------------------------------------------------------------------
bool vtkChartRadViz::MouseButtonReleaseEvent(const vtkContextMouseEvent& /*mouse*/)
{
  return true;
}

//------------------------------------------------------------------------------
bool vtkChartRadViz::MouseWheelEvent(const vtkContextMouseEvent&, int /*delta*/)
{
  return true;
}

bool vtkChartRadViz::LocatePointInPlots(const vtkContextMouseEvent& mouse)
{
  if (!this->Private->Plot || !this->Private->Plot->GetVisible())
  {
    return false;
  }
  else
  {
    int dimensions[4];
    vtkVector2f position(mouse.GetScenePos());
    vtkVector2f tolerance(5, 5);
    vtkVector2f plotPos(0, 0);
    this->Private->Plot->GetDimensions(dimensions);

    vtkVector2f pos(mouse.GetScenePos());
    if (pos[0] >= dimensions[0] && pos[0] <= dimensions[0] + dimensions[2] &&
      pos[1] >= dimensions[1] && pos[1] <= dimensions[1] + dimensions[3])
    {
      vtkIdType segmentId;
      int labelIndex =
        this->Private->Plot->GetNearestPoint(position, tolerance, &plotPos, &segmentId);
      if (labelIndex >= 0)
      {
        auto label = this->Private->Plot->GetLabel(labelIndex);
        std::ostringstream ostr;
        ostr << label << ": " << plotPos.GetY();
        return true;
      }
    }
  }
  return false;
}

//------------------------------------------------------------------------------
void vtkChartRadViz::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->Private->Plot)
  {
    os << indent << "Plot: " << endl;
    this->Private->Plot->PrintSelf(os, indent.GetNextIndent());
  }
}
VTK_ABI_NAMESPACE_END
