// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkChartRadViz.h"

#include "vtkCommand.h"
#include "vtkContext2D.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextScene.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPlotRadViz.h"
#include "vtkPoints2D.h"
#include "vtkStringArray.h"
#include "vtkTextProperty.h"
#include <sstream>
#include <vtk_fmt.h>
// clang-format off
#include VTK_FMT(fmt/format.h)
#include VTK_FMT(fmt/ranges.h)
// clang-format on

VTK_ABI_NAMESPACE_BEGIN
class vtkChartRadViz::vtkInternals
{
public:
  vtkInternals() = default;

  vtkSmartPointer<vtkPlotRadViz> Plot;
  /**
   * The index of the vertex dragged after left mouse press, -1 when no vertex
   * is dragged.
   */
  int DragVertexIndex = -1;
  /**
   * Set to true on left mouse press on the lens outline and set to false
   * on left mouse release.
   */
  bool ResizeLens = false;
  /**
   * Set to true on left mouse press on the lens interior and set to false
   * on mouse release.
   */
  bool DragLens = false;
  /**
   * Set to the mouse position on left mouse press.
   */
  float DragLensStart[2] = { 0.0f, 0.0f };
  /**
   * Vertices rotation on left mouse press on a vertex.
   */
  double DragStartRotation = 0.0;
  /**
   * Mouse position angle in degrees on left mouse press on a vertex (0 for (1,
   * 0), 90 for (0, 1) and so on).
   */
  double DragStartMouseAngle = 0.0;
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkChartRadViz);

//------------------------------------------------------------------------------
vtkChartRadViz::vtkChartRadViz()
{
  this->Internals = std::make_unique<vtkChartRadViz::vtkInternals>();
  // same style as the vertex labels (see vtkPlotRadViz), one step larger
  this->TitleProperties->SetFontSize(18);
  this->TitleProperties->BoldOn();
  this->AddPlot(0 /*type*/);
}

//------------------------------------------------------------------------------
vtkChartRadViz::~vtkChartRadViz() = default;

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

  const int TITLE_SPACE = 10;
  this->SetBorders(0, 0, 0, 0);

  int titleSize = 0;
  if (!this->Title.empty())
  {
    int fontSize = this->TitleProperties->GetFontSize();
    titleSize = fontSize + TITLE_SPACE;
    vtkPoints2D* rect = vtkPoints2D::New();
    rect->InsertNextPoint(this->Point1[0], this->Point2[1] - titleSize);
    rect->InsertNextPoint(this->Point2[0] - this->Point1[0], fontSize);
    painter->ApplyTextProp(this->TitleProperties);
    painter->DrawStringRect(rect, this->Title);
    rect->Delete();
  }

  if (this->Internals->Plot)
  {
    this->Internals->Plot->SetDimensions(
      this->Size.GetX(), this->Size.GetY(), this->Geometry[0], this->Geometry[1] - titleSize);
    this->Internals->Plot->Paint(painter);
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
  if (!this->Internals->Plot)
  {
    this->Internals->Plot = vtkSmartPointer<vtkPlotRadViz>::New();
    this->Internals->Plot->SetParent(this);
    this->AddItem(this->Internals->Plot);
  }
  return this->Internals->Plot;
}

//------------------------------------------------------------------------------
vtkIdType vtkChartRadViz::AddPlot(vtkPlot* plot)
{
  auto* radviz = vtkPlotRadViz::SafeDownCast(plot);
  if (!radviz)
  {
    // ignored, like vtkChart does: ParaView's chart view adds its time plot
    // to every chart type
    return -1;
  }
  this->SetPlot(radviz);
  return 0;
}

//------------------------------------------------------------------------------
bool vtkChartRadViz::RemovePlot(vtkIdType index)
{
  if (index != 0 || !this->Internals->Plot)
  {
    return false;
  }
  this->SetPlot(nullptr);
  return true;
}

//------------------------------------------------------------------------------
void vtkChartRadViz::ClearPlots()
{
  if (this->Internals->Plot)
  {
    this->SetPlot(nullptr);
  }
}

//------------------------------------------------------------------------------
void vtkChartRadViz::SetPlot(vtkPlotRadViz* plot)
{
  if (this->Internals->Plot)
  {
    this->RemoveItem(this->Internals->Plot);
  }
  this->Internals->Plot = plot;
  if (plot)
  {
    plot->SetParent(this);
    this->AddItem(plot);
  }
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkChartRadViz::SetVariableVisibility(const std::string& name, bool visible)
{
  if (this->Internals->Plot)
  {
    this->Internals->Plot->SetVariableVisibility(name, visible);
  }
}

//------------------------------------------------------------------------------
void vtkChartRadViz::SetVariableVisibility(int index, bool visible)
{
  if (this->Internals->Plot)
  {
    this->Internals->Plot->SetVariableVisibility(index, visible);
  }
}

//------------------------------------------------------------------------------
void vtkChartRadViz::SetVariableVisibilityAll(bool visible)
{
  if (this->Internals->Plot)
  {
    this->Internals->Plot->SetVariableVisibilityAll(visible);
  }
}

//------------------------------------------------------------------------------
bool vtkChartRadViz::GetVariableVisibility(const std::string& name)
{
  return this->Internals->Plot ? this->Internals->Plot->GetVariableVisibility(name) : false;
}

//------------------------------------------------------------------------------
bool vtkChartRadViz::GetVariableVisibility(int index)
{
  return this->Internals->Plot ? this->Internals->Plot->GetVariableVisibility(index) : false;
}

//------------------------------------------------------------------------------
vtkStringArray* vtkChartRadViz::GetVisibleVariables()
{
  return this->Internals->Plot ? this->Internals->Plot->GetVisibleVariables() : nullptr;
}

//------------------------------------------------------------------------------
void vtkChartRadViz::SetVisibleVariables(vtkStringArray* names)
{
  if (this->Internals->Plot)
  {
    this->Internals->Plot->SetVisibleVariables(names);
  }
}

//------------------------------------------------------------------------------
void vtkChartRadViz::SetVisibleVariableIndices(const std::vector<int>& indices)
{
  if (this->Internals->Plot)
  {
    this->Internals->Plot->SetVisibleVariableIndices(indices);
  }
}

//------------------------------------------------------------------------------
std::vector<int> vtkChartRadViz::GetVisibleVariableIndices()
{
  return this->Internals->Plot ? this->Internals->Plot->GetVisibleVariableIndices()
                               : std::vector<int>();
}

//------------------------------------------------------------------------------
void vtkChartRadViz::ResetVisibleVariableIndices()
{
  if (this->Internals->Plot)
  {
    this->Internals->Plot->ResetVisibleVariableIndices();
  }
}

//------------------------------------------------------------------------------
void vtkChartRadViz::AddVisibleVariableIndex(int index)
{
  if (this->Internals->Plot)
  {
    this->Internals->Plot->AddVisibleVariableIndex(index);
  }
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
  if (this->Internals->Plot)
  {
    vtkVector2f pos{ mouse.GetScenePos() };
    if (this->Internals->DragVertexIndex >= 0)
    {
      // rotate vertices by the angle swept by the mouse since the press
      const double* center = this->Internals->Plot->GetSceneCenter();
      double mouseAngle =
        vtkMath::DegreesFromRadians(std::atan2(pos[1] - center[1], pos[0] - center[0]));
      this->Internals->Plot->SetVerticesRotation(
        this->Internals->DragStartRotation + mouseAngle - this->Internals->DragStartMouseAngle);
      this->Scene->SetDirty(true);
      this->Internals->Plot->InvokeEvent(vtkCommand::InteractionEvent);
    }
    else if (this->Internals->ResizeLens)
    {
      double* lensCenter = this->Internals->Plot->GetSceneLensCenter();
      // distance between the lensCenter and the mouse point
      double r =
        std::sqrt(std::pow(pos[0] - lensCenter[0], 2) + std::pow(pos[1] - lensCenter[1], 2));
      this->Internals->Plot->SetSceneLensRadius(r);
      this->Scene->SetDirty(true);
      this->Internals->Plot->InvokeEvent(vtkCommand::InteractionEvent);
    }
    else if (this->Internals->DragLens)
    {
      double d[2];

      d[0] = pos[0] - this->Internals->DragLensStart[0];
      d[1] = pos[1] - this->Internals->DragLensStart[1];
      double newLensCenter[2];
      double* lensCenter = this->Internals->Plot->GetSceneLensCenter();
      newLensCenter[0] = lensCenter[0] + d[0];
      ;
      newLensCenter[1] = lensCenter[1] + d[1];
      this->Internals->Plot->SetSceneLensCenter(newLensCenter);
      std::copy(pos.GetData(), pos.GetData() + 2, this->Internals->DragLensStart);
      this->Scene->SetDirty(true);
      this->Internals->Plot->InvokeEvent(vtkCommand::InteractionEvent);
    }
    else
    {
      // we don't drag anything
      int activeVertexIndex;
      bool activeLensOutline;
      bool activeLensInterior;
      this->GetActiveElement(pos, activeVertexIndex, activeLensOutline, activeLensInterior);
      if (activeVertexIndex != this->Internals->Plot->GetActiveVertexIndex() ||
        activeLensOutline != this->Internals->Plot->GetActiveLensOutline() ||
        activeLensInterior != this->Internals->Plot->GetActiveLensInterior())
      {
        this->Scene->SetDirty(true);
      }
      // highlight the active element
      this->Internals->Plot->SetActiveVertexIndex(activeVertexIndex);
      this->Internals->Plot->SetActiveLensOutline(activeLensOutline);
      this->Internals->Plot->SetActiveLensInterior(activeLensInterior);
    }
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkChartRadViz::MouseLeaveEvent(const vtkContextMouseEvent&)
{
  vtkPlotRadViz* plot = this->Internals->Plot;
  bool dragging = this->Internals->DragVertexIndex >= 0 || this->Internals->ResizeLens ||
    this->Internals->DragLens;
  if (plot && !dragging)
  {
    // clear the hover highlight, so it does not go stale
    if (plot->GetActiveVertexIndex() >= 0 || plot->GetActiveLensOutline() ||
      plot->GetActiveLensInterior())
    {
      this->Scene->SetDirty(true);
    }
    plot->SetActiveVertexIndex(-1);
    plot->SetActiveLensOutline(false);
    plot->SetActiveLensInterior(false);
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkChartRadViz::MouseButtonPressEvent(const vtkContextMouseEvent& mouse)
{
  vtkDebugMacro("vtkChartRadViz::MouseButtonPressEvent");
  if (this->Internals->Plot && mouse.GetButton() == vtkContextMouseEvent::LEFT_BUTTON)
  {
    vtkVector2f pos{ mouse.GetScenePos() };
    this->GetActiveElement(pos, this->Internals->DragVertexIndex, this->Internals->ResizeLens,
      this->Internals->DragLens);
    if (this->Internals->DragVertexIndex >= 0)
    {
      const double* center = this->Internals->Plot->GetSceneCenter();
      this->Internals->DragStartMouseAngle =
        vtkMath::DegreesFromRadians(std::atan2(pos[1] - center[1], pos[0] - center[0]));
      this->Internals->DragStartRotation = this->Internals->Plot->GetVerticesRotation();
    }
    if (this->Internals->DragLens)
    {
      std::copy(pos.GetData(), pos.GetData() + 2, this->Internals->DragLensStart);
    }
    if (this->Internals->DragVertexIndex >= 0 || this->Internals->ResizeLens ||
      this->Internals->DragLens)
    {
      this->Internals->Plot->InvokeEvent(vtkCommand::StartInteractionEvent);
    }
    return true;
  }
  else
  {
    return false;
  }
}

//------------------------------------------------------------------------------
void vtkChartRadViz::GetActiveElement(
  const vtkVector2f& pos, int& activeVertexIndex, bool& activeLensOutline, bool& activeLensInterior)
{
  activeVertexIndex = -1;
  activeLensOutline = false;
  activeLensInterior = false;
  const int SIZE = 5;
  auto vertices = this->Internals->Plot->GetVertices();
  if (!vertices || vertices->GetNumberOfPoints() < 3)
  {
    return;
  }
  for (int i = 0; i < vertices->GetNumberOfPoints(); ++i)
  {
    double xy[2];
    vertices->GetPoint(i, xy);
    if (pos[0] <= xy[0] + SIZE && pos[0] >= xy[0] - SIZE && pos[1] <= xy[1] + SIZE &&
      pos[1] >= xy[1] - SIZE)
    {
      activeVertexIndex = i;
      vtkDebugMacro("vtkChartRadViz::GetActiveElement: Vertex: " << i);
      break;
    }
  }
  if (activeVertexIndex < 0 && this->Internals->Plot->GetLensVisibility())
  {
    double* lensCenter = this->Internals->Plot->GetSceneLensCenter();
    double lensRadius = this->Internals->Plot->GetSceneLensRadius();
    // distance between the lensCenter and the mouse point
    double r = std::sqrt(std::pow(pos[0] - lensCenter[0], 2) + std::pow(pos[1] - lensCenter[1], 2));
    if (std::abs(lensRadius - r) < SIZE)
    {
      activeLensOutline = true;
      vtkDebugMacro("vtkChartRadViz::GetActiveElement: Lens Outline");
    }
    else if (lensRadius - r > 0)
    {
      activeLensInterior = true;
      vtkDebugMacro("vtkChartRadViz::GetActiveElement: Lens Interior");
    }
  }
}

//------------------------------------------------------------------------------
bool vtkChartRadViz::MouseButtonReleaseEvent(const vtkContextMouseEvent& mouse)
{
  if (mouse.GetButton() != vtkContextMouseEvent::LEFT_BUTTON)
  {
    return false;
  }
  if (this->Internals->Plot &&
    (this->Internals->DragVertexIndex >= 0 || this->Internals->ResizeLens ||
      this->Internals->DragLens))
  {
    this->Internals->Plot->InvokeEvent(vtkCommand::EndInteractionEvent);
  }
  this->Internals->DragVertexIndex = -1;
  this->Internals->ResizeLens = false;
  this->Internals->DragLens = false;
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
