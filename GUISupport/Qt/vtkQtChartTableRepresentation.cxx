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

#include "vtkQtChartSeriesModelCollection.h"
#include "vtkQtChartSeriesLayer.h"
#include "vtkQtChartTableSeriesModel.h"
#include "vtkQtChartArea.h"
#include "vtkQtChartSeriesOptions.h"
#include "vtkQtChartSeriesSelection.h"
#include "vtkQtChartSeriesSelectionModel.h"
#include "vtkQtChartStyleManager.h"
#include "vtkQtChartStyleGenerator.h"
#include "vtkQtChartColorStyleGenerator.h"
#include "vtkQtChartPenBrushGenerator.h"

#include "vtkAlgorithmOutput.h"
#include "vtkCommand.h"
#include "vtkConvertSelection.h"
#include "vtkDataObjectToTable.h"
#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkQtChartViewBase.h"
#include "vtkQtItemView.h"
#include "vtkQtListView.h"
#include "vtkQtTreeView.h"
#include "vtkSelectionLink.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkQtTableModelAdapter.h"
#include "vtkTree.h"
#include "vtkQtTreeModelAdapter.h"

/*
// TODO Move the signal helper to its own class so we don't have to MOC compile
// vtkQtChartTableRepresentation.h
// Signal helper class
void vtkQtChartRepresentationSignalHandler::selectedSeriesChanged(const vtkQtChartSeriesSelection &list)
{
  this->Target->QtSelectedSeriesChanged(list);
}

// Signal helper class
void vtkQtChartRepresentationSignalHandler::modelChanged()
{
  this->Target->QtModelChanged();
}
*/

//----------------------------------------------------------------------------
class vtkQtChartTableRepresentation::vtkInternal
{
public:

  vtkInternal()
    {
    this->ChartView = 0;
    this->SeriesModel = 0;
    }

  ~vtkInternal()
    {
    }

  vtkQtChartViewBase* ChartView;
  vtkQtChartTableSeriesModel* SeriesModel;
};

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkQtChartTableRepresentation, "1.3");
vtkStandardNewMacro(vtkQtChartTableRepresentation);

//----------------------------------------------------------------------------
vtkQtChartTableRepresentation::vtkQtChartTableRepresentation()
{
  this->Internal = new vtkInternal;
  this->ColumnsAsSeries = true;
  //this->Handler = new vtkQtChartTableRepresentationSignalHandler();
  //this->Handler->setTarget(this);

  // Set up the chart table series model. It will be deleted when the
  // model adapter is deleted in the parent destructor.
  this->Internal->SeriesModel =
    new vtkQtChartTableSeriesModel(this->ModelAdapter, this->ModelAdapter);
}

//----------------------------------------------------------------------------

vtkQtChartTableRepresentation::~vtkQtChartTableRepresentation()
{
  // If we are still in a view, then remove self from the view
  if (this->Internal->ChartView)
    {
    this->RemoveFromView(this->Internal->ChartView);
    }

  //delete this->Handler;
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkQtChartTableRepresentation::SetInputConnection(vtkAlgorithmOutput* conn)
{
  this->Superclass::SetInputConnection(conn);
}


//----------------------------------------------------------------------------
void vtkQtChartTableRepresentation::QtModelChanged()
{

}

//----------------------------------------------------------------------------
void vtkQtChartTableRepresentation::
QtSelectedSeriesChanged(const vtkQtChartSeriesSelection& vtkNotUsed(list))
{

}

//----------------------------------------------------------------------------
bool 
vtkQtChartTableRepresentation::AddToView(vtkView* view)
{
  // Don't add to the same view twice
  if (view == this->Internal->ChartView)
    {
    return false;
    }

  // Remove self from current view if needed
  if (this->Internal->ChartView)
    {
    this->RemoveFromView(this->Internal->ChartView);
    }

  // Downcast the view to a chart view
  vtkQtChartViewBase* chart = vtkQtChartViewBase::SafeDownCast(view);

  // If view if null then return false
  if (!chart)
    {
    return false;
    }

  // If we dont have a valid model adapter than it means we have
  // no data, so there is nothing we can do.
  if (!this->ModelAdapter)
    {
    vtkDebugMacro("Representation cannot be added to chart view because the model "
      " adapter is null, probably because table data has not been set.");
    return false;
    }


  // Get the chart view's model collection
  vtkQtChartSeriesModelCollection* modelCollection =
    qobject_cast<vtkQtChartSeriesModelCollection*>(chart->GetChartSeriesModel());
  if (!modelCollection)
    {
    vtkDebugMacro("Representation cannot be added to chart view because the chart "
      "view has an invalid model collection.");
    return false;
    }

  // Add the new series model to the chart view's model collection.
  modelCollection->addSeriesModel(this->Internal->SeriesModel);

  this->Internal->ChartView = chart;
  return true;
}

//----------------------------------------------------------------------------

bool 
vtkQtChartTableRepresentation::RemoveFromView(vtkView* view)
{
  // Only remove self from view if we have previously been added to the view
  if (view != this->Internal->ChartView)
    {
    return false;
    }

  vtkQtChartViewBase* chart = vtkQtChartViewBase::SafeDownCast(view);
  if (!chart)
    {
    return false;
    }

  // Get the chart view's model collection
  vtkQtChartSeriesModelCollection* modelCollection =
    qobject_cast<vtkQtChartSeriesModelCollection*>(chart->GetChartSeriesModel());
  if (!modelCollection)
    {
    vtkDebugMacro("Representation cannot be removed from the chart view because "
      " the chart view has an invalid model collection.");
    return false;
    }

  modelCollection->removeSeriesModel(this->Internal->SeriesModel);

  this->Internal->ChartView = 0;
  return true;
}

//----------------------------------------------------------------------------
void vtkQtChartTableRepresentation::Update()
{
  if(!this->Internal->ChartView)
    {
    return;
    }

  this->Superclass::Update();
}

// ----------------------------------------------------------------------
void vtkQtChartTableRepresentation::SetColumnsAsSeries(bool value)
{
  if (this->ColumnsAsSeries == value)
    {
    return;
    }
  this->ColumnsAsSeries = value;
  this->Internal->SeriesModel->setColumnsAsSeries(value);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkQtChartTableRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
