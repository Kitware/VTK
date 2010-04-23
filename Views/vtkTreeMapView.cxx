/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeMapView.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkTreeMapView.h"

#include "vtkBoxLayoutStrategy.h"
#include "vtkDataRepresentation.h"
#include "vtkInteractorStyleTreeMapHover.h"
#include "vtkLabeledTreeMapDataMapper.h"
#include "vtkObjectFactory.h"
#include "vtkSliceAndDiceLayoutStrategy.h"
#include "vtkSmartPointer.h"
#include "vtkSquarifyLayoutStrategy.h"
#include "vtkTextProperty.h"
#include "vtkTreeMapToPolyData.h"

vtkStandardNewMacro(vtkTreeMapView);
//----------------------------------------------------------------------------
vtkTreeMapView::vtkTreeMapView()
{
  this->BoxLayout = vtkSmartPointer<vtkBoxLayoutStrategy>::New();
  this->SquarifyLayout = vtkSmartPointer<vtkSquarifyLayoutStrategy>::New();
  this->SliceAndDiceLayout = vtkSmartPointer<vtkSliceAndDiceLayoutStrategy>::New();

  this->SetLayoutStrategyToSquarify();
  vtkSmartPointer<vtkTreeMapToPolyData> poly =
    vtkSmartPointer<vtkTreeMapToPolyData>::New();
  this->SetAreaToPolyData(poly);
  this->SetUseRectangularCoordinates(true);
  vtkSmartPointer<vtkLabeledTreeMapDataMapper> mapper =
    vtkSmartPointer<vtkLabeledTreeMapDataMapper>::New();
  this->SetAreaLabelMapper(mapper);
}

//----------------------------------------------------------------------------
vtkTreeMapView::~vtkTreeMapView()
{
}

//----------------------------------------------------------------------------
void vtkTreeMapView::SetLayoutStrategyToBox()
{
  this->SetLayoutStrategy("Box");
}

//----------------------------------------------------------------------------
void vtkTreeMapView::SetLayoutStrategyToSliceAndDice()
{
  this->SetLayoutStrategy("Slice And Dice");
}

//----------------------------------------------------------------------------
void vtkTreeMapView::SetLayoutStrategyToSquarify()
{
  this->SetLayoutStrategy("Squarify");
}

//----------------------------------------------------------------------------
void vtkTreeMapView::SetLayoutStrategy(vtkAreaLayoutStrategy* s)
{
  if (!vtkTreeMapLayoutStrategy::SafeDownCast(s))
    {
    vtkErrorMacro("Strategy must be a treemap layout strategy.");
    return;
    }
  this->Superclass::SetLayoutStrategy(s);
}

//----------------------------------------------------------------------------
void vtkTreeMapView::SetLayoutStrategy(const char* name)
{
  if (!strcmp(name, "Box"))
    {
    this->BoxLayout->SetShrinkPercentage(this->GetShrinkPercentage());
    this->SetLayoutStrategy(this->BoxLayout);
    }
  else if (!strcmp(name, "Slice And Dice"))
    {
    this->SliceAndDiceLayout->SetShrinkPercentage(this->GetShrinkPercentage());
    this->SetLayoutStrategy(this->SliceAndDiceLayout);
    }
  else if (!strcmp(name, "Squarify"))
    {
    this->SquarifyLayout->SetShrinkPercentage(this->GetShrinkPercentage());
    this->SetLayoutStrategy(this->SquarifyLayout);
    }
  else
    {
    vtkErrorMacro("Unknown layout name: " << name);
    }
}

//----------------------------------------------------------------------------
void vtkTreeMapView::SetFontSizeRange(const int maxSize, const int minSize, const int delta)
{
  vtkLabeledTreeMapDataMapper* mapper =
    vtkLabeledTreeMapDataMapper::SafeDownCast(this->GetAreaLabelMapper());
  if (mapper)
    {
    mapper->SetFontSizeRange(maxSize, minSize, delta);
    }
}

//----------------------------------------------------------------------------
void vtkTreeMapView::GetFontSizeRange(int range[3])
{
  vtkLabeledTreeMapDataMapper* mapper =
    vtkLabeledTreeMapDataMapper::SafeDownCast(this->GetAreaLabelMapper());
  if (mapper)
    {
    mapper->GetFontSizeRange(range);
    }
}

//----------------------------------------------------------------------------
void vtkTreeMapView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

