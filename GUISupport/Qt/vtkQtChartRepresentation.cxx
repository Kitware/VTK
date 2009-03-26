/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartRepresentation.cxx

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

#include "vtkQtChartRepresentation.h"

#include "vtkQtChartSeriesLayer.h"
#include "vtkQtChartTableSeriesModel.h"
#include "vtkQtChartArea.h"
#include "vtkQtChartSeriesOptions.h"
#include "vtkQtChartSeriesSelection.h"
#include "vtkQtChartSeriesSelectionModel.h"
#include "vtkQtChartStyleManager.h"
#include "vtkQtChartColorStyleGenerator.h"

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
#include "vtkQtChartView.h"
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

#include <QIcon>
#include <QItemSelection>
#include <QPixmap>
#include <QTreeView>

#include <cassert>

#define VTK_CREATE(type,name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

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

vtkCxxRevisionMacro(vtkQtChartRepresentation, "1.4");
vtkStandardNewMacro(vtkQtChartRepresentation);

//----------------------------------------------------------------------------
vtkQtChartRepresentation::vtkQtChartRepresentation()
{
  this->ChartLayer = 0;
  this->ColumnsAsSeries = true;
  this->Handler = new vtkQtChartRepresentationSignalHandler();
  this->Handler->setTarget(this);
  this->SeriesToVTKMap = vtkIdTypeArray::New();
  this->VTKToSeriesMap = vtkIntArray::New();
}

//----------------------------------------------------------------------------

vtkQtChartRepresentation::~vtkQtChartRepresentation()
{
  if (this->ChartLayer)
    {
    delete this->ChartLayer;
    }
  if (this->SeriesToVTKMap)
    {
    this->SeriesToVTKMap->Delete();
    }
  if (this->VTKToSeriesMap)
    {
    this->VTKToSeriesMap->Delete();
    }

  delete this->Handler;
}

//----------------------------------------------------------------------------

void 
vtkQtChartRepresentation::SetupInputConnections()
{
  this->Superclass::SetupInputConnections();

  // We still need to set the series colors on the chart layer (if
  // there is one) since the superclass knows nothing about that part.
  if (this->ChartLayer)
    {
    vtkQtChartTableSeriesModel *chartModel = new vtkQtChartTableSeriesModel(this->ModelAdapter);
    this->ChartLayer->setModel(chartModel);
    }
}


//----------------------------------------------------------------------------

void 
vtkQtChartRepresentation::QtModelChanged()
{
  if (!this->ChartLayer || !this->ModelAdapter)
    {
    return;
    }

  // Make mapping from series (via QModelIndex) to VTK ID and vice
  // versa.  Actually, we're mapping from the *internal ID* of the
  // QModelIndex to the VTK ID and vice versa.  QChartLayer knows how
  // to convert this back into a valid QModelIndex object (I hope).
/*
  this->SeriesToVTKMap->SetNumberOfTuples(this->ChartLayer->getNumberOfSeries());
  this->VTKToSeriesMap->SetNumberOfTuples(0);
  for (int s = 0; s < this->ChartLayer->getNumberOfSeries(); ++s)
    {
    vtkIdType id = static_cast<vtkIdType>(this->ChartLayer->getSeriesModelIndex(s).internalId());
    this->SeriesToVTKMap->SetValue(s, id);
    while (id >= this->VTKToSeriesMap->GetNumberOfTuples())
      {
      this->VTKToSeriesMap->InsertNextValue(-1);
      }
    this->VTKToSeriesMap->SetValue(id, s);
    }
*/
}

//----------------------------------------------------------------------------
void 
vtkQtChartRepresentation::SetChartLayer(vtkQtChartSeriesLayer* layer)
{
  if (this->ChartLayer)
    {
    //QObject::disconnect(this->ChartLayer->getSelectionModel(), 
    //                 SIGNAL(selectionChanged(const vtkQtChartSeriesSelection &)),
    //                 this->Handler, 
    //                 SLOT(selectedSeriesChanged(const vtkQtChartSeriesSelection &)));
    delete this->ChartLayer;
    this->ChartLayer = 0;
    }

  this->ChartLayer = layer;
  if (this->ChartLayer && this->ModelAdapter)
    {
    vtkQtChartTableSeriesModel *chartModel = new vtkQtChartTableSeriesModel(this->ModelAdapter);
    this->ChartLayer->setModel(chartModel);
    }

  if (this->ChartLayer)
    {
    //QObject::connect(this->ChartLayer->getSelectionModel(), 
    //                 SIGNAL(selectionChanged(const vtkQtChartSeriesSelection &)),
    //                 this->Handler, 
    //                 SLOT(selectedSeriesChanged(const vtkQtChartSeriesSelection &)));
    }
}

//----------------------------------------------------------------------------

void 
vtkQtChartRepresentation::QtSelectedSeriesChanged(const vtkQtChartSeriesSelection &list)
{
  QList<QPair<int, int> > ranges = list.getSeries();

  VTK_CREATE(vtkIdTypeArray, ids);
  for (int i = 0; i < ranges.size(); ++i)
    {
    for (int j = ranges[i].first; j <= ranges[i].second; ++j)
      {
      ids->InsertNextValue(j);
      }
    }

  VTK_CREATE(vtkSelection, sel);
  VTK_CREATE(vtkSelectionNode, node);
  node->SetSelectionList(ids);
  node->SetContentType(vtkSelectionNode::INDICES);
  sel->AddNode(node);
  this->Select(0, sel);
}

//----------------------------------------------------------------------------
bool 
vtkQtChartRepresentation::AddToView(vtkView* view)
{
  vtkQtChartView* chart = vtkQtChartView::SafeDownCast(view);
  if (chart)
    {
    vtkQtChartArea* cv = chart->GetChartView();
    if (cv)
      {
      cv->addLayer(this->ChartLayer);
      }
    }
  return true;
}

//----------------------------------------------------------------------------

bool 
vtkQtChartRepresentation::RemoveFromView(vtkView* view)
{
  vtkQtChartView* chart = vtkQtChartView::SafeDownCast(view);
  if (chart)
    {
    vtkQtChartArea* cv = chart->GetChartView();
    if (cv)
      {
      cv->removeLayer(this->ChartLayer);
      }
    }
  return true;
}

//----------------------------------------------------------------------------
void vtkQtChartRepresentation::Update()
{
  if(!this->ChartLayer)
    {
    return;
    }

  this->Superclass::Update();

/*
  // Get the latest selection.
  this->GetSelectionLink()->Update();
  vtkAlgorithm* alg = this->GetInputConnection()->GetProducer();
  alg->Update();
  vtkDataObject* output = alg->GetOutputDataObject(this->GetInputConnection()->GetIndex());
  vtkAlgorithm* selectionAlg = this->GetSelectionConnection()->GetProducer();
  selectionAlg->Update();
  vtkSelection* s = vtkSelection::SafeDownCast(
    selectionAlg->GetOutputDataObject(this->GetSelectionConnection()->GetIndex()));

  // Convert to an index selection.
  vtkSelection* converted = vtkConvertSelection::ToIndexSelection(s, output);

  // Convert to a QList.
  vtkIdTypeArray* indexArr = vtkIdTypeArray::SafeDownCast(converted->GetSelectionList());
  if (indexArr && indexArr->GetNumberOfTuples() > 0)
    {
    for(int idx = 0; idx < this->ChartLayer->getModel()->getNumberOfSeries(); ++idx)
      {
      this->ChartLayer->getSeriesOptions(idx)->setVisible(false);
      }

    for (vtkIdType i = 0; i < indexArr->GetNumberOfTuples(); ++i)
      {
      this->ChartLayer->getSeriesOptions(indexArr->GetValue(i))->setVisible(true);
      }
    }
*/

  /*
  // Update the color scheme
  if(this->ChartLayer->getChartArea())
    {
    vtkQtChartStyleManager *manager = this->ChartLayer->getChartArea()->getStyleManager();
    if(manager)
      {
      vtkQtChartColorStyleGenerator *colors =
        qobject_cast<vtkQtChartColorStyleGenerator *>(manager->getGenerator());
      vtkQtChartPenBrushGenerator *brushes =
        qobject_cast<vtkQtChartPenBrushGenerator *>(manager->getGenerator());
      if(colors)
        {
        colors->getColors()->clearColors();
        }
      else if(brushes)
        {
        brushes->clearBrushes();
        }

      this->CreateSeriesColors();
      for (int i = 0; i < this->SeriesColors->GetNumberOfTuples(); ++i)
        {
        QColor c;
        double tuple[4];
        this->SeriesColors->GetTuple(i, tuple);
        c.setRgbF(tuple[0], tuple[1], tuple[2], tuple[3]);
        if(colors)
          {
          colors->getColors()->addColor(c);
          }
        else if(brushes)
          {
          brushes->addBrush(QBrush(c));
          }

        vtkQtChartSeriesOptions *options = this->ChartLayer->getSeriesOptions(i);
        options->setStyle(i, manager->getGenerator());
        }
      }
    }*/
  //converted->Delete();

}

// ----------------------------------------------------------------------

void
vtkQtChartRepresentation::SetColumnsAsSeries(bool value)
{
  if(this->ChartLayer && this->ColumnsAsSeries != value)
    {
    qobject_cast<vtkQtChartTableSeriesModel *>
      (this->ChartLayer->getModel())->setColumnsAsSeries(value);
    this->ColumnsAsSeries = value;
    }
}

// ----------------------------------------------------------------------

void
vtkQtChartRepresentation::CreateSeriesColors()
{
  this->SeriesColors->Reset();
  this->SeriesColors->SetNumberOfComponents(4);

  int size = 0;
  if(this->ColumnsAsSeries)
    {
    size = this->ModelAdapter->columnCount(QModelIndex());
    }
  else
    {
    size = this->ModelAdapter->rowCount(QModelIndex());
    }

  this->SeriesColors->SetNumberOfTuples(size);

  for (int i = 0; i < size; ++i)
    {
    double seriesValue = 1;
    if (size > 1)
      {
      seriesValue = static_cast<double>(i) / (size-1);
      }
    if(seriesValue == 1)
      {
      seriesValue = 1 - 0.5 * (static_cast<double>(1)/size);
      }
    QColor c;
    if (this->ColorTable)
      {
      double rgb[3];
      double opacity;
      this->ColorTable->GetColor(seriesValue, rgb);
      opacity = this->ColorTable->GetOpacity(seriesValue);
      c.setRgbF(rgb[0], rgb[1], rgb[2], opacity);
      }
    else
      {
      c.setHsvF(seriesValue, 1, 0.7);
      }

    this->SeriesColors->SetComponent(i, 0, c.redF());
    this->SeriesColors->SetComponent(i, 1, c.greenF());
    this->SeriesColors->SetComponent(i, 2, c.blueF());
    this->SeriesColors->SetComponent(i, 3, c.alphaF());
    }
}

//----------------------------------------------------------------------------

void 
vtkQtChartRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
