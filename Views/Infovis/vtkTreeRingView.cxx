// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#include "vtkTreeRingView.h"

#include "vtkObjectFactory.h"
#include "vtkRenderedTreeAreaRepresentation.h"
#include "vtkStackedTreeLayoutStrategy.h"
#include "vtkTreeRingToPolyData.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkTreeRingView);
//------------------------------------------------------------------------------
vtkTreeRingView::vtkTreeRingView() = default;

//------------------------------------------------------------------------------
vtkTreeRingView::~vtkTreeRingView() = default;

//------------------------------------------------------------------------------
void vtkTreeRingView::SetRootAngles(double start, double end)
{
  vtkStackedTreeLayoutStrategy* s =
    vtkStackedTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (s)
  {
    s->SetRootStartAngle(start);
    s->SetRootEndAngle(end);
  }
}

//------------------------------------------------------------------------------
void vtkTreeRingView::SetRootAtCenter(bool center)
{
  vtkStackedTreeLayoutStrategy* st =
    vtkStackedTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (st)
  {
    st->SetReverse(!center);
  }
}

//------------------------------------------------------------------------------
bool vtkTreeRingView::GetRootAtCenter()
{
  vtkStackedTreeLayoutStrategy* st =
    vtkStackedTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (st)
  {
    return !st->GetReverse();
  }
  return false;
}

//------------------------------------------------------------------------------
void vtkTreeRingView::SetLayerThickness(double thickness)
{
  vtkStackedTreeLayoutStrategy* st =
    vtkStackedTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (st)
  {
    st->SetRingThickness(thickness);
  }
}

//------------------------------------------------------------------------------
double vtkTreeRingView::GetLayerThickness()
{
  vtkStackedTreeLayoutStrategy* st =
    vtkStackedTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (st)
  {
    return st->GetRingThickness();
  }
  return 0.0;
}

//------------------------------------------------------------------------------
void vtkTreeRingView::SetInteriorRadius(double rad)
{
  vtkStackedTreeLayoutStrategy* st =
    vtkStackedTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (st)
  {
    st->SetInteriorRadius(rad);
  }
}

//------------------------------------------------------------------------------
double vtkTreeRingView::GetInteriorRadius()
{
  vtkStackedTreeLayoutStrategy* st =
    vtkStackedTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (st)
  {
    return st->GetInteriorRadius();
  }
  return 0.0;
}

//------------------------------------------------------------------------------
void vtkTreeRingView::SetInteriorLogSpacingValue(double value)
{
  vtkStackedTreeLayoutStrategy* st =
    vtkStackedTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (st)
  {
    st->SetInteriorLogSpacingValue(value);
  }
}

//------------------------------------------------------------------------------
double vtkTreeRingView::GetInteriorLogSpacingValue()
{
  vtkStackedTreeLayoutStrategy* st =
    vtkStackedTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (st)
  {
    return st->GetInteriorLogSpacingValue();
  }
  return 0.0;
}

//------------------------------------------------------------------------------
void vtkTreeRingView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
