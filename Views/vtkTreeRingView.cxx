/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeRingView.cxx

  -------------------------------------------------------------------------
    Copyright 2008 Sandia Corporation.
    Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
    the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTreeRingView.h"

#include "vtkObjectFactory.h"
#include "vtkRenderedTreeAreaRepresentation.h"
#include "vtkStackedTreeLayoutStrategy.h"
#include "vtkTreeRingToPolyData.h"

vtkStandardNewMacro(vtkTreeRingView);
//----------------------------------------------------------------------------
vtkTreeRingView::vtkTreeRingView()
{
}

//----------------------------------------------------------------------------
vtkTreeRingView::~vtkTreeRingView()
{
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void vtkTreeRingView::SetRootAtCenter(bool center)
{
  vtkStackedTreeLayoutStrategy* st =
    vtkStackedTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (st)
    {
    st->SetReverse(!center);
    }
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void vtkTreeRingView::SetLayerThickness(double thickness)
{
  vtkStackedTreeLayoutStrategy* st =
    vtkStackedTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (st)
    {
    st->SetRingThickness(thickness);
    }
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void vtkTreeRingView::SetInteriorRadius(double rad)
{
  vtkStackedTreeLayoutStrategy* st =
    vtkStackedTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (st)
    {
    st->SetInteriorRadius(rad);
    }
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void vtkTreeRingView::SetInteriorLogSpacingValue(double value)
{
  vtkStackedTreeLayoutStrategy* st =
    vtkStackedTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (st)
    {
    st->SetInteriorLogSpacingValue(value);
    }
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void vtkTreeRingView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

