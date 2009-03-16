/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeRingView3.cxx

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

#include "vtkTreeRingView3.h"

#include "vtkObjectFactory.h"
#include "vtkStackedTreeLayoutStrategy.h"
#include "vtkTreeRingToPolyData.h"

#include "vtkQtLabelMapper.h"
#include "vtkTexturedActor2D.h"
#include "vtkAreaLayout.h"
#include "vtkQtTreeRingLabelMapper.h"

vtkCxxRevisionMacro(vtkTreeRingView3, "1.1");
vtkStandardNewMacro(vtkTreeRingView3);
//----------------------------------------------------------------------------
vtkTreeRingView3::vtkTreeRingView3()
{
//  bool test1 = true;
  bool test1 = false;
  bool test2 = true;
//  bool test2 = false;
  if( test1 )
    {
    this->AreaLabelActor = vtkSmartPointer<vtkTexturedActor2D>::New();
    this->AreaLabelActor->PickableOff();
    
    vtkSmartPointer<vtkQtLabelMapper> mapper = 
      vtkSmartPointer<vtkQtLabelMapper>::New();
    this->SetAreaLabelMapper(mapper);
    
    this->AreaLabelMapper->SetInputConnection(this->AreaLayout->GetOutputPort());
    this->AreaLabelActor->SetMapper(this->AreaLabelMapper);
    }
  else if( test2 )
    {
    this->AreaLabelActor = vtkSmartPointer<vtkTexturedActor2D>::New();
    this->AreaLabelActor->PickableOff();

    vtkSmartPointer<vtkQtTreeRingLabelMapper> mapper = 
      vtkSmartPointer<vtkQtTreeRingLabelMapper>::New();
    mapper->SetRenderer( this->Renderer );
    this->SetAreaLabelMapper(mapper);
    
    this->AreaLabelMapper->SetInputConnection(this->AreaLayout->GetOutputPort());
    this->AreaLabelActor->SetMapper(this->AreaLabelMapper);
    }
}

//----------------------------------------------------------------------------
vtkTreeRingView3::~vtkTreeRingView3()
{
}

//----------------------------------------------------------------------------
void vtkTreeRingView3::SetRootAngles(double start, double end)
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
void vtkTreeRingView3::SetRootAtCenter(bool center)
{
  vtkStackedTreeLayoutStrategy* st =
    vtkStackedTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (st)
    {
    st->SetReverse(!center);
    }
}

//----------------------------------------------------------------------------
bool vtkTreeRingView3::GetRootAtCenter()
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
void vtkTreeRingView3::SetLayerThickness(double thickness)
{
  vtkStackedTreeLayoutStrategy* st =
    vtkStackedTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (st)
    {
    st->SetRingThickness(thickness);
    }
}

//----------------------------------------------------------------------------
double vtkTreeRingView3::GetLayerThickness()
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
void vtkTreeRingView3::SetInteriorRadius(double rad)
{
  vtkStackedTreeLayoutStrategy* st =
    vtkStackedTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (st)
    {
    st->SetInteriorRadius(rad);
    }
}

//----------------------------------------------------------------------------
double vtkTreeRingView3::GetInteriorRadius()
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
void vtkTreeRingView3::SetInteriorLogSpacingValue(double value)
{
  vtkStackedTreeLayoutStrategy* st =
    vtkStackedTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (st)
    {
    st->SetInteriorLogSpacingValue(value);
    }
}

//----------------------------------------------------------------------------
double vtkTreeRingView3::GetInteriorLogSpacingValue()
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
void vtkTreeRingView3::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

