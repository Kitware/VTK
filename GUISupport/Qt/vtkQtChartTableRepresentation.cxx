/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartTableRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkQtChartTableRepresentation.h"
#include "vtkQtChartViewBase.h"
#include "vtkQtChartSeriesModelCollection.h"
#include "vtkQtChartTableSeriesModel.h"
#include "vtkQtTableModelAdapter.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

//----------------------------------------------------------------------------
class vtkQtChartTableRepresentation::vtkInternal
{
public:
  vtkInternal()
    {
    this->SeriesModel = 0;
    }

  vtkQtChartTableSeriesModel* SeriesModel;
};

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkQtChartTableRepresentation, "1.4");
vtkStandardNewMacro(vtkQtChartTableRepresentation);

//----------------------------------------------------------------------------
vtkQtChartTableRepresentation::vtkQtChartTableRepresentation()
{
  this->Internal = new vtkInternal;
  this->ColumnsAsSeries = true;

  // Set up the chart table series model. It will be deleted when the
  // model adapter is deleted in the parent destructor.
  this->Internal->SeriesModel =
    new vtkQtChartTableSeriesModel(this->ModelAdapter, this->ModelAdapter);
}

//----------------------------------------------------------------------------
vtkQtChartTableRepresentation::~vtkQtChartTableRepresentation()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkQtChartTableRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
bool vtkQtChartTableRepresentation::AddToView(vtkView* view)
{
  // Downcast the view to a chart view
  vtkQtChartViewBase* chart = vtkQtChartViewBase::SafeDownCast(view);
  if (!chart)
    {
    return false;
    }

  // Get the chart view's model collection
  vtkQtChartSeriesModelCollection* modelCollection = chart->GetChartSeriesModel();
  if (!modelCollection)
    {
    vtkDebugMacro("Representation cannot be added to chart view because the chart "
      "view has an invalid model collection.");
    return false;
    }

  // Add the our series model to the chart view's model collection.
  modelCollection->addSeriesModel(this->GetSeriesModel());
  return true;
}

//----------------------------------------------------------------------------
bool vtkQtChartTableRepresentation::RemoveFromView(vtkView* view)
{
  // Downcast the view to a chart view
  vtkQtChartViewBase* chart = vtkQtChartViewBase::SafeDownCast(view);
  if (!chart)
    {
    return false;
    }

  // Get the chart view's model collection
  vtkQtChartSeriesModelCollection* modelCollection = chart->GetChartSeriesModel();
  if (!modelCollection)
    {
    vtkDebugMacro("Representation cannot be removed from the chart view because "
      " the chart view has an invalid model collection.");
    return false;
    }

  // Remove the our series model from the chart view's model collection.
  modelCollection->removeSeriesModel(this->GetSeriesModel());
  return true;
}

//----------------------------------------------------------------------------
vtkQtChartTableSeriesModel* vtkQtChartTableRepresentation::GetSeriesModel()
{
  return this->Internal->SeriesModel;
}

//----------------------------------------------------------------------------
void vtkQtChartTableRepresentation::SetColumnsAsSeries(bool value)
{
  if (this->ColumnsAsSeries == value)
    {
    return;
    }
  this->ColumnsAsSeries = value;
  this->GetSeriesModel()->setColumnsAsSeries(value);
  this->Modified();
}
