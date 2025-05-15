// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkGridAxesActor3D.h"

#include "vtkBoundingBox.h"
#include "vtkDoubleArray.h"
#include "vtkGridAxesActor2D.h"
#include "vtkGridAxesHelper.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"

#include <map>

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkGridAxesActor3D);
//----------------------------------------------------------------------------
vtkGridAxesActor3D::vtkGridAxesActor3D()
  : UseCustomLabels(false)
{
  for (int cc = 0; cc < 6; cc++)
  {
    this->GridAxes2DActors[cc]->SetFace((0x01 << cc));
    if (cc > 0)
    {
      // share the text properties among all planes.
      this->GridAxes2DActors[cc]->SetTitleTextProperty(
        0, this->GridAxes2DActors[0]->GetTitleTextProperty(0));
      this->GridAxes2DActors[cc]->SetTitleTextProperty(
        1, this->GridAxes2DActors[0]->GetTitleTextProperty(1));
      this->GridAxes2DActors[cc]->SetTitleTextProperty(
        2, this->GridAxes2DActors[0]->GetTitleTextProperty(2));

      this->GridAxes2DActors[cc]->SetLabelTextProperty(
        0, this->GridAxes2DActors[0]->GetLabelTextProperty(0));
      this->GridAxes2DActors[cc]->SetLabelTextProperty(
        1, this->GridAxes2DActors[0]->GetLabelTextProperty(1));
      this->GridAxes2DActors[cc]->SetLabelTextProperty(
        2, this->GridAxes2DActors[0]->GetLabelTextProperty(2));
    }
  }

  this->SetFaceMask(
    vtkGridAxesHelper::MIN_XY | vtkGridAxesHelper::MIN_YZ | vtkGridAxesHelper::MIN_ZX);
  this->SetLabelMask(0xff);
}

//----------------------------------------------------------------------------
vtkGridAxesActor3D::~vtkGridAxesActor3D() = default;

//----------------------------------------------------------------------------
void vtkGridAxesActor3D::GetActors(vtkPropCollection* props)
{
  if (this->GetVisibility())
  {
    vtkViewport* vp = nullptr;
    if (this->NumberOfConsumers)
    {
      vp = vtkViewport::SafeDownCast(this->Consumers[0]);
      if (vp)
      {
        this->UpdateGeometry(vp);
      }
    }

    for (int i = 0; i < this->GridAxes2DActors.GetSize(); ++i)
    {
      this->GridAxes2DActors[i]->GetActors(props);
    }
  }
}

//----------------------------------------------------------------------------
void vtkGridAxesActor3D::SetFaceMask(unsigned int mask)
{
  if (this->FaceMask != mask)
  {
    this->FaceMask = mask;
    for (int cc = 0; cc < 6; cc++)
    {
      this->GridAxes2DActors[cc]->SetVisibility(((this->FaceMask & (0x01 << cc)) != 0) ? 1 : 0);
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkGridAxesActor3D::SetLabelMask(unsigned int mask)
{
  if (this->GetLabelMask() != mask)
  {
    for (int cc = 0; cc < 6; cc++)
    {
      this->GridAxes2DActors[cc]->SetLabelMask(mask);
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------------
unsigned int vtkGridAxesActor3D::GetLabelMask()
{
  return this->GridAxes2DActors[0]->GetLabelMask();
}

//----------------------------------------------------------------------------
void vtkGridAxesActor3D::SetTitleTextProperty(int axis, vtkTextProperty* tprop)
{
  if (this->GetTitleTextProperty(axis) != tprop)
  {
    for (int cc = 0; cc < 6; cc++)
    {
      this->GridAxes2DActors[cc]->SetTitleTextProperty(axis, tprop);
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------------
vtkTextProperty* vtkGridAxesActor3D::GetTitleTextProperty(int axis)
{
  return this->GridAxes2DActors[0]->GetTitleTextProperty(axis);
}

//----------------------------------------------------------------------------
void vtkGridAxesActor3D::SetTitle(int axis, const std::string& title)
{
  if (this->GetTitle(axis) != title)
  {
    for (int cc = 0; cc < 6; cc++)
    {
      this->GridAxes2DActors[cc]->SetTitle(axis, title);
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------------
const std::string& vtkGridAxesActor3D::GetTitle(int axis)
{
  return this->GridAxes2DActors[0]->GetTitle(axis);
}

//-----------------------------------------------------------------------------
void vtkGridAxesActor3D::SetUseCustomLabels(int axis, bool val)
{
  if (axis >= 0 && axis < 3 && this->UseCustomLabels[axis] != val)
  {
    this->UseCustomLabels[axis] = val;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
void vtkGridAxesActor3D::SetNumberOfLabels(int axis, vtkIdType val)
{
  if (axis >= 0 && axis < 3 && this->CustomLabels[axis]->GetNumberOfTuples() != val)
  {
    this->CustomLabels[axis]->SetNumberOfTuples(val);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkGridAxesActor3D::SetLabel(int axis, vtkIdType index, double value)
{
  if (axis >= 0 && axis < 3 && index < this->CustomLabels[axis]->GetNumberOfTuples() &&
    this->CustomLabels[axis]->GetValue(index) != value)
  {
    this->CustomLabels[axis]->SetValue(index, value);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkGridAxesActor3D::SetLabelTextProperty(int axis, vtkTextProperty* tprop)
{
  if (this->GetLabelTextProperty(axis) != tprop)
  {
    for (int cc = 0; cc < 6; cc++)
    {
      this->GridAxes2DActors[cc]->SetLabelTextProperty(axis, tprop);
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------------
vtkTextProperty* vtkGridAxesActor3D::GetLabelTextProperty(int axis)
{
  return this->GridAxes2DActors[0]->GetLabelTextProperty(axis);
}

//----------------------------------------------------------------------------
void vtkGridAxesActor3D::SetNotation(int axis, int notation)
{
  if (this->GetNotation(axis) != notation)
  {
    for (int cc = 0; cc < 6; cc++)
    {
      this->GridAxes2DActors[cc]->SetNotation(axis, notation);
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkGridAxesActor3D::GetNotation(int axis)
{
  return this->GridAxes2DActors[0]->GetNotation(axis);
}

//----------------------------------------------------------------------------
void vtkGridAxesActor3D::SetPrecision(int axis, int val)
{
  if (this->GetPrecision(axis) != val)
  {
    for (int cc = 0; cc < 6; cc++)
    {
      this->GridAxes2DActors[cc]->SetPrecision(axis, val);
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkGridAxesActor3D::SetTickLabelFunction(int axis, std::function<double(double)> func)
{
  for (int cc = 0; cc < 6; cc++)
  {
    this->GridAxes2DActors[cc]->SetTickLabelFunction(axis, func);
  }
  this->Modified();
}

//----------------------------------------------------------------------------
std::function<double(double)> vtkGridAxesActor3D::GetTickLabelFunction(int axis)
{
  return this->GridAxes2DActors[0]->GetTickLabelFunction(axis);
}

//----------------------------------------------------------------------------
int vtkGridAxesActor3D::GetPrecision(int axis)
{
  return this->GridAxes2DActors[0]->GetPrecision(axis);
}

//----------------------------------------------------------------------------
void vtkGridAxesActor3D::SetGenerateGrid(bool val)
{
  if (this->GetGenerateGrid() != val)
  {
    for (int cc = 0; cc < 6; cc++)
    {
      this->GridAxes2DActors[cc]->SetGenerateGrid(val);
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------------
bool vtkGridAxesActor3D::GetGenerateGrid()
{
  return this->GridAxes2DActors[0]->GetGenerateGrid();
}

//----------------------------------------------------------------------------
void vtkGridAxesActor3D::SetGenerateEdges(bool val)
{
  if (this->GetGenerateEdges() != val)
  {
    for (int cc = 0; cc < 6; cc++)
    {
      this->GridAxes2DActors[cc]->SetGenerateEdges(val);
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------------
bool vtkGridAxesActor3D::GetGenerateEdges()
{
  return this->GridAxes2DActors[0]->GetGenerateEdges();
}

//----------------------------------------------------------------------------
void vtkGridAxesActor3D::SetGenerateTicks(bool val)
{
  if (this->GetGenerateTicks() != val)
  {
    for (int cc = 0; cc < 6; cc++)
    {
      this->GridAxes2DActors[cc]->SetGenerateTicks(val);
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------------
bool vtkGridAxesActor3D::GetGenerateTicks()
{
  return this->GridAxes2DActors[0]->GetGenerateTicks();
}

//----------------------------------------------------------------------------
void vtkGridAxesActor3D::SetProperty(vtkProperty* prop)
{
  if (this->GetProperty() != prop)
  {
    for (int cc = 0; cc < 6; cc++)
    {
      this->GridAxes2DActors[cc]->SetProperty(prop);
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------------
vtkProperty* vtkGridAxesActor3D::GetProperty()
{
  return this->GridAxes2DActors[0]->GetProperty();
}

//----------------------------------------------------------------------------
double* vtkGridAxesActor3D::GetBounds()
{
  vtkMTimeType mtime = this->GetMTime();
  if (mtime == this->GetBoundsMTime)
  {
    return this->Bounds;
  }
  if (!vtkMath::AreBoundsInitialized(this->GridBounds))
  {
    vtkMath::UninitializeBounds(this->Bounds);
    this->GetBoundsMTime = mtime;
    return this->Bounds;
  }

  vtkMatrix4x4* matrix = this->GetMatrix();
  vtkBoundingBox bbox;
  for (int z = 0; z < 2; z++)
  {
    for (int y = 0; y < 2; y++)
    {
      for (int x = 0; x < 2; x++)
      {
        double point[4] = { this->GridBounds[x], this->GridBounds[2 + y], this->GridBounds[4 + z],
          1.0 };
        matrix->MultiplyPoint(point, point);
        point[0] /= point[3];
        point[1] /= point[3];
        point[2] /= point[3];
        bbox.AddPoint(point[0], point[1], point[2]);
      }
    }
  }
  bbox.GetBounds(this->Bounds);
  this->GetBoundsMTime = mtime;
  return this->Bounds;
}

//----------------------------------------------------------------------------
void vtkGridAxesActor3D::GetRenderedBounds(double bounds[6])
{
  this->GetBounds(bounds);

  // Use the same trick as the old vtkCubeAxesActor:
  vtkBoundingBox bbox(bounds);
  bbox.Inflate(bbox.GetMaxLength());
  bbox.GetBounds(bounds);
}

//----------------------------------------------------------------------------
int vtkGridAxesActor3D::RenderOpaqueGeometry(vtkViewport* viewport)
{
  vtkRenderWindow* rWin = vtkRenderWindow::SafeDownCast(viewport->GetVTKWindow());
  if (rWin == nullptr || rWin->GetDesiredUpdateRate() < 1.0)
  {
    this->Update(viewport);
  }

  int counter = 0;
  for (int cc = 0; cc < 6; cc++)
  {
    this->GridAxes2DActors[cc]->SetPropertyKeys(this->GetPropertyKeys());
    counter += this->GridAxes2DActors[cc]->GetVisibility()
      ? this->GridAxes2DActors[cc]->RenderOpaqueGeometry(viewport)
      : 0;
  }
  return counter;
}

//----------------------------------------------------------------------------
void vtkGridAxesActor3D::UpdateGeometry(vtkViewport* viewport)
{
  vtkRenderWindow* rWin = vtkRenderWindow::SafeDownCast(viewport->GetVTKWindow());
  if (rWin == nullptr || rWin->GetDesiredUpdateRate() < 1.0)
  {
    this->Update(viewport);
  }

  for (int cc = 0; cc < 6; cc++)
  {
    if (this->GridAxes2DActors[cc]->GetVisibility())
      this->GridAxes2DActors[cc]->UpdateGeometry(viewport, false);
  }
}

//----------------------------------------------------------------------------
int vtkGridAxesActor3D::RenderTranslucentPolygonalGeometry(vtkViewport* viewport)
{
  int counter = 0;
  for (int cc = 0; cc < 6; cc++)
  {
    this->GridAxes2DActors[cc]->SetPropertyKeys(this->GetPropertyKeys());
    counter += this->GridAxes2DActors[cc]->GetVisibility()
      ? this->GridAxes2DActors[cc]->RenderTranslucentPolygonalGeometry(viewport)
      : 0;
  }
  return counter;
}

//----------------------------------------------------------------------------
int vtkGridAxesActor3D::RenderOverlay(vtkViewport* viewport)
{
  int counter = 0;
  for (int cc = 0; cc < 6; cc++)
  {
    this->GridAxes2DActors[cc]->SetPropertyKeys(this->GetPropertyKeys());
    counter += this->GridAxes2DActors[cc]->GetVisibility()
      ? this->GridAxes2DActors[cc]->RenderOverlay(viewport)
      : 0;
  }
  return counter;
}

//----------------------------------------------------------------------------
vtkTypeBool vtkGridAxesActor3D::HasTranslucentPolygonalGeometry()
{
  for (int cc = 0; cc < 6; cc++)
  {
    if (this->GridAxes2DActors[cc]->GetVisibility() &&
      this->GridAxes2DActors[cc]->HasTranslucentPolygonalGeometry() != 0)
    {
      return 1;
    }
  }

  return this->Superclass::HasTranslucentPolygonalGeometry();
}

//----------------------------------------------------------------------------
void vtkGridAxesActor3D::ReleaseGraphicsResources(vtkWindow* win)
{
  for (int cc = 0; cc < 6; cc++)
  {
    this->GridAxes2DActors[cc]->ReleaseGraphicsResources(win);
  }
  this->Superclass::ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------------
void vtkGridAxesActor3D::Update(vtkViewport* viewport)
{
  vtkTuple<bool, 6> faces_to_render(false);
  for (int cc = 0; cc < 6; cc++)
  {
    if (!this->GridAxes2DActors[cc]->GetVisibility())
    {
      continue;
    }

    this->GridAxes2DActors[cc]->SetGridBounds(this->GridBounds);
    this->GridAxes2DActors[cc]->SetUserMatrix(this->GetMatrix());
    this->GridAxes2DActors[cc]->SetForceOpaque(this->ForceOpaque);
    this->GridAxes2DActors[cc]->Helper->SetLabelVisibilityOverrides(vtkTuple<bool, 4>(true));
    if (this->GetMTime() > this->CustomLabelsMTime)
    {
      for (int axis = 0; axis < 3; axis++)
      {
        this->GridAxes2DActors[cc]->SetCustomTickPositions(
          axis, this->UseCustomLabels[axis] ? this->CustomLabels[axis].GetPointer() : nullptr);
      }
    }

    // FIXME: We call vtkGridAxesActor3D::Update() here and then
    // vtkGridAxesActor3D::RenderOpaqueGeometry() will also call the same
    // method. We can avoid the second call to speed things up.
    faces_to_render[cc] = this->GridAxes2DActors[cc]->Update(viewport);
  }
  this->CustomLabelsMTime = this->GetMTime();

  // Now determine which labels to hide based on this->LabelUniqueEdgesOnly.
  if (!this->LabelUniqueEdgesOnly)
  {
    return;
  }

  // The algorithm I am using is a simple one: don't label any edge that's
  // shared between multiple faces. So we first create a map of all the edges
  // and count them. Then, turn off labeling for any edge that a count > 1.
  typedef std::pair<vtkVector2i, vtkVector2i> EdgeType;
  std::map<EdgeType, int> edge_count;
  for (int face = 0; face < 6; face++)
  {
    if (!faces_to_render[face])
    {
      continue;
    }

    const vtkTuple<vtkVector2i, 4>& viewportPoints =
      this->GridAxes2DActors[face]->Helper->GetViewportPoints();
    for (int vertex = 0; vertex < 4; vertex++)
    {
      int me = vertex;
      int next = (vertex + 1) % 4;
      if (viewportPoints[next] < viewportPoints[me])
      {
        me = next;
        next = vertex;
      }
      edge_count[EdgeType(viewportPoints[me], viewportPoints[next])]++;
    }
  }
  assert(edge_count.size() <= 12);
  for (int face = 0; face < 6; face++)
  {
    if (!faces_to_render[face])
    {
      continue;
    }

    const vtkTuple<vtkVector2i, 4>& viewportPoints =
      this->GridAxes2DActors[face]->Helper->GetViewportPoints();
    vtkTuple<bool, 4> overrides(true);
    for (int vertex = 0; vertex < 4; vertex++)
    {
      int me = vertex;
      int next = (vertex + 1) % 4;
      if (viewportPoints[next] < viewportPoints[me])
      {
        me = next;
        next = vertex;
      }
      // edge_count is 1 for no sharing, 2 for sharing between faces and
      // 3 if there is both sharing between faces and within the same face
      // (for instance for rendering a box aligned with the ortoghonal
      // axes in parallel projection
      if (edge_count[EdgeType(viewportPoints[me], viewportPoints[next])] == 2)
      {
        overrides[vertex] = false;
      }
    }
    this->GridAxes2DActors[face]->Helper->SetLabelVisibilityOverrides(overrides);
  }
}

//----------------------------------------------------------------------------
void vtkGridAxesActor3D::ShallowCopy(vtkProp* prop)
{
  this->Superclass::ShallowCopy(prop);
  vtkGridAxesActor3D* other = vtkGridAxesActor3D::SafeDownCast(prop);
  if (other == nullptr)
  {
    return;
  }

  this->SetGridBounds(other->GetGridBounds());
  this->SetFaceMask(other->GetFaceMask());
  this->SetLabelMask(other->GetLabelMask());
  this->SetLabelUniqueEdgesOnly(other->GetLabelUniqueEdgesOnly());
  this->SetGenerateGrid(other->GetGenerateGrid());
  this->SetGenerateEdges(other->GetGenerateEdges());
  this->SetGenerateTicks(other->GetGenerateTicks());
  this->SetProperty(other->GetProperty());
  this->SetForceOpaque(other->GetForceOpaque());
  for (int cc = 0; cc < 3; cc++)
  {
    this->SetTitleTextProperty(cc, other->GetTitleTextProperty(cc));
    this->SetTitle(cc, other->GetTitle(cc));
    this->SetUseCustomLabels(cc, other->UseCustomLabels[cc]);
    this->CustomLabels[cc]->DeepCopy(other->CustomLabels[cc].GetPointer());
    this->SetLabelTextProperty(cc, other->GetLabelTextProperty(cc));
    this->SetNotation(cc, other->GetNotation(cc));
    this->SetPrecision(cc, other->GetPrecision(cc));
  }
}

//----------------------------------------------------------------------------
void vtkGridAxesActor3D::SetLabelDisplayOffset(int xoffset, int yoffset)
{
  int* off = this->GetLabelDisplayOffset();
  if ((off[0] != xoffset) || (off[1] != yoffset))
  {
    for (int cc = 0; cc < 6; ++cc)
    {
      this->GridAxes2DActors[cc]->SetLabelDisplayOffset(xoffset, yoffset);
    }
  }
}

//----------------------------------------------------------------------------
void vtkGridAxesActor3D::SetLabelDisplayOffset(const int off[2])
{
  this->SetLabelDisplayOffset(off[0], off[1]);
}

//----------------------------------------------------------------------------
int* vtkGridAxesActor3D::GetLabelDisplayOffset()
{
  return this->GridAxes2DActors[0]->GetLabelDisplayOffset();
}

//----------------------------------------------------------------------------
void vtkGridAxesActor3D::GetLabelDisplayOffset(int& xoff, int& yoff)
{
  this->GridAxes2DActors[0]->GetLabelDisplayOffset(xoff, yoff);
}

//----------------------------------------------------------------------------
void vtkGridAxesActor3D::GetLabelDisplayOffset(int off[2])
{
  this->GridAxes2DActors[0]->GetLabelDisplayOffset(off);
}

//----------------------------------------------------------------------------
void vtkGridAxesActor3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
