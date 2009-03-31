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

#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkQtChartBasicSeriesOptionsModel.h"
#include "vtkQtChartSeriesLayer.h"
#include "vtkQtChartSeriesModelCollection.h"
#include "vtkQtChartSeriesOptions.h"
#include "vtkQtChartSeriesOptionsModelCollection.h"
#include "vtkQtChartTableSeriesModel.h"
#include "vtkQtChartViewBase.h"
#include "vtkQtTableModelAdapter.h"
#include "vtkSmartPointer.h"

#include <QPen>
#include <QBrush>

//----------------------------------------------------------------------------
class vtkQtChartTableRepresentation::vtkInternal
{
public:
  vtkInternal()
    {
    this->SeriesModel = 0;
    this->OptionsModel = 0;
    }

  vtkQtChartTableSeriesModel* SeriesModel;
  vtkQtChartSeriesOptionsModel* OptionsModel;
  vtkstd::string LastSeriesName;
};

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkQtChartTableRepresentation, "1.5");
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
  this->Internal->OptionsModel =
    new vtkQtChartBasicSeriesOptionsModel(this->Internal->SeriesModel,
      this->Internal->SeriesModel);
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
void vtkQtChartTableRepresentation::SetOptionsModel(
  vtkQtChartSeriesOptionsModel* model)
{
  if (model == NULL)
    {
    vtkErrorMacro("OptionsModel cannot be NULL.");
    return;
    }

  if (this->Internal->OptionsModel != model)
    {
    this->Internal->OptionsModel = model;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
vtkQtChartSeriesOptionsModel* vtkQtChartTableRepresentation::GetOptionsModel()
{
  return this->Internal->OptionsModel;
}

//----------------------------------------------------------------------------
int vtkQtChartTableRepresentation::GetNumberOfSeries()
{
  return this->GetSeriesModel()->getNumberOfSeries();
}

//----------------------------------------------------------------------------
const char* vtkQtChartTableRepresentation::GetSeriesName(int series)
{
  if (series >= this->GetNumberOfSeries())
    {
    return 0;
    }

  this->Internal->LastSeriesName =
    this->GetSeriesModel()->getSeriesName(series).toString().toStdString();
  return this->Internal->LastSeriesName.c_str();
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
    vtkErrorMacro("Representation cannot be added to chart view because the chart "
      "view has an invalid model collection.");
    return false;
    }

  vtkQtChartSeriesOptionsModelCollection* optionsCollection =
    chart->GetChartOptionsModel();
  if (!optionsCollection)
    {
    vtkErrorMacro("Representation cannot be added to chart view because the "
      "chart view has an invalid options model collection.");
    return false;
    }

  // Add the our series model to the chart view's model collection.
  this->Internal->OptionsModel->setChartSeriesLayer(chart->GetChartSeriesLayer());
  optionsCollection->addSeriesOptionsModel(this->Internal->OptionsModel);
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
    vtkErrorMacro("Representation cannot be removed from the chart view because "
      " the chart view has an invalid model collection.");
    return false;
    }

  vtkQtChartSeriesOptionsModelCollection* optionsCollection =
    chart->GetChartOptionsModel();
  if (!optionsCollection)
    {
    vtkErrorMacro("Representation cannot be removed to chart view because the "
      "chart view has an invalid options model collection.");
    return false;
    }

  // Remove the our series model from the chart view's model collection.
  modelCollection->removeSeriesModel(this->GetSeriesModel());
  optionsCollection->removeSeriesOptionsModel(this->Internal->OptionsModel);
  this->Internal->OptionsModel->setChartSeriesLayer(0);
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
