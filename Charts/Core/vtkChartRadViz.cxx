// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkChartRadViz.h"

#include "vtkCommand.h"
#include "vtkContext2D.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextScene.h"
#include "vtkObjectFactory.h"
#include "vtkPlotRadViz.h"
#include "vtkPoints2D.h"
#include "vtkTextProperty.h"
#include <sstream>

VTK_ABI_NAMESPACE_BEGIN
class vtkChartRadViz::vtkInternals
{
public:
  vtkInternals() = default;

  vtkSmartPointer<vtkPlotRadViz> Plot;
  int DragVertex = -1;
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkChartRadViz);

//------------------------------------------------------------------------------
vtkChartRadViz::vtkChartRadViz()
{
  this->Internals = new vtkChartRadViz::vtkInternals();
}

//------------------------------------------------------------------------------
vtkChartRadViz::~vtkChartRadViz()
{
  delete this->Internals;
}

//------------------------------------------------------------------------------
void vtkChartRadViz::Update()
{
  if (this->Internals->Plot && this->Internals->Plot->GetVisible())
  {
    this->Internals->Plot->Update();
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
  const int TITLE_SPACE = 5;
  this->SetBorders(BORDER_SIZE * tileScale.GetX(), BORDER_SIZE * tileScale.GetY(),
    BORDER_SIZE * tileScale.GetX(), BORDER_SIZE * tileScale.GetY());

  int titleSize = 0;
  if (!this->Title.empty())
  {
    int fontSize = this->TitleProperties->GetFontSize();
    titleSize = fontSize + TITLE_SPACE;
    vtkPoints2D* rect = vtkPoints2D::New();
    rect->InsertNextPoint(this->Point1[0], this->Point2[1]);
    rect->InsertNextPoint(this->Point2[0] - this->Point1[0], fontSize);
    painter->ApplyTextProp(this->TitleProperties);
    painter->DrawStringRect(rect, this->Title);
    rect->Delete();
  }

  // Set the dimensions of the Plot
  if (this->Internals->Plot)
  {
    this->Internals->Plot->SetDimensions(this->Size.GetX() + BORDER_SIZE,
      this->Size.GetY() + BORDER_SIZE, this->Geometry[0] - 2 * BORDER_SIZE,
      this->Geometry[1] - 2 * BORDER_SIZE - titleSize);
  }

  this->PaintChildren(painter);

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
  if (!this->Internals->Plot)
  {
    this->Internals->Plot = vtkSmartPointer<vtkPlotRadViz>::New();
    this->AddItem(this->Internals->Plot);
  }
  return this->Internals->Plot;
}

//------------------------------------------------------------------------------
void vtkChartRadViz::SetPlot(vtkPlotRadViz* plot)
{
  if (this->Internals->Plot)
  {
    this->RemoveItem(this->Internals->Plot);
  }
  this->Internals->Plot = plot;
  this->AddItem(this->Internals->Plot);
  // Ensure legend remains on top
  this->Modified();
}

//------------------------------------------------------------------------------
vtkPlot* vtkChartRadViz::GetPlot(vtkIdType index)
{
  if (index == 0)
  {
    return this->Internals->Plot;
  }

  return nullptr;
}

//------------------------------------------------------------------------------
vtkIdType vtkChartRadViz::GetNumberOfPlots()
{
  if (this->Internals->Plot)
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
  return pos[0] > this->Point1[0] - 10 && pos[0] < this->Point2[0] + 10 &&
    pos[1] > this->Point1[1] && pos[1] < this->Point2[1];
}

//------------------------------------------------------------------------------
bool vtkChartRadViz::MouseEnterEvent(const vtkContextMouseEvent&)
{
  return true;
}

//------------------------------------------------------------------------------
bool vtkChartRadViz::MouseMoveEvent(const vtkContextMouseEvent& mouse)
{
  if (this->Internals->DragVertex >= 0 && !this->Scene->GetDirty())
  {
    auto vertices = this->Internals->Plot->GetVertices();
    double vertex[2];
    vertices->GetPoint(this->Internals->DragVertex, vertex);
    vtkVector2f mp{ mouse.GetScenePos() };
    double mousePos[2] = { mp[0], mp[1] };
    double angle = this->Internals->Plot->GetAngle(vertex, mousePos);
    vtkDebugMacro("vtkChartRadViz::MouseMoveEvent, angle:"
      << angle << " VerticesRotation: " << this->Internals->Plot->GetVerticesRotation());
    this->Internals->Plot->SetVerticesRotation(
      angle + this->Internals->Plot->GetVerticesRotation());
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
bool vtkChartRadViz::MouseButtonPressEvent(const vtkContextMouseEvent& mouse)
{
  if (this->Internals->Plot)
  {
    const int SIZE = 5;
    auto vertices = this->Internals->Plot->GetVertices();
    for (int i = 0; i < vertices->GetNumberOfPoints(); ++i)
    {
      double xy[2];
      vertices->GetPoint(i, xy);
      vtkVector2f pos{ mouse.GetScenePos() };
      if (pos[0] <= xy[0] + SIZE && pos[0] >= xy[0] - SIZE && pos[1] <= xy[1] + SIZE &&
        pos[1] >= xy[1] - SIZE)
      {
        // mouse position is inside the square around the vertex
        this->Internals->DragVertex = i;
        break;
      }
    }
    return true;
  }
  else
  {
    return false;
  }
}

//------------------------------------------------------------------------------
bool vtkChartRadViz::MouseButtonReleaseEvent(const vtkContextMouseEvent& mouse)
{
  this->Internals->DragVertex = -1;
  return true;
}

//------------------------------------------------------------------------------
bool vtkChartRadViz::MouseWheelEvent(const vtkContextMouseEvent&, int)
{
  return true;
}

//------------------------------------------------------------------------------
void vtkChartRadViz::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->Internals->Plot)
  {
    os << indent << "Plot: " << endl;
    this->Internals->Plot->PrintSelf(os, indent.GetNextIndent());
  }
}

VTK_ABI_NAMESPACE_END
