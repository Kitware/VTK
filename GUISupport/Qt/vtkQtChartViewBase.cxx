/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartViewBase.cxx

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

#include "vtkQtChartViewBase.h"
#include "vtkQtChartTableRepresentation.h"
#include "vtkQtChartArea.h"
#include "vtkQtChartWidget.h"
#include "vtkQtChartSeriesLayer.h"
#include "vtkQtChartAxisLayer.h"
#include "vtkQtChartAxis.h"
#include "vtkQtChartTitle.h"
#include "vtkQtChartStyleManager.h"
#include "vtkQtChartColorStyleGenerator.h"
#include "vtkQtChartMouseSelection.h"
#include "vtkQtChartInteractorSetup.h"
#include "vtkQtChartSeriesSelectionHandler.h"
#include "vtkQtChartSeriesModelCollection.h"
#include "vtkQtChartLegendModel.h"
#include "vtkQtChartLegend.h"
#include "vtkQtChartLegendManager.h"
#include "vtkQtChartSeriesOptions.h"
#include "vtkTable.h"

#include "vtkObjectFactory.h"

class vtkQtChartViewBase::vtkInternal
{
public:

  vtkInternal()
    {
    this->ChartLayer = 0;
    this->ModelCollection = new vtkQtChartSeriesModelCollection;
    }

  ~vtkInternal()
    {
    delete this->ModelCollection;
    }

  vtkQtChartSeriesLayer*           ChartLayer;
  vtkQtChartSeriesModelCollection* ModelCollection;
  vtkQtChartWidget                 ChartWidget;
  vtkQtChartLegend*                Legend;
  vtkQtChartLegendManager*         LegendManager;
};

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkQtChartViewBase, "1.3");
vtkStandardNewMacro(vtkQtChartViewBase);

//----------------------------------------------------------------------------
vtkQtChartViewBase::vtkQtChartViewBase()
{
  this->Internal = new vtkInternal;

  // Setup the legend
  vtkQtChartWidget *chart = this->GetChartWidget();
  this->Internal->Legend = new vtkQtChartLegend;
  this->Internal->LegendManager = new vtkQtChartLegendManager(
    this->Internal->Legend);
  this->Internal->LegendManager->setChartLegend(this->Internal->Legend);
  this->Internal->LegendManager->setChartArea(chart->getChartArea());
  chart->setLegend(this->Internal->Legend);
}

//----------------------------------------------------------------------------
vtkQtChartViewBase::~vtkQtChartViewBase()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
// TODO-
// SetChartLayer should properly remove and destroy the previous chart layer.
// For now we assume that this method is only called once (from the
// subclass's constructor)
void vtkQtChartViewBase::SetChartLayer(vtkQtChartSeriesLayer* chartLayer)
{
  if (!chartLayer)
    {
    return;
    }

  
  this->Internal->ChartLayer = chartLayer;
  this->Internal->ChartLayer->setModel(this->Internal->ModelCollection);

  // The chart area takes ownership of the chart layer and will delete it
  // when the area is destroyed.  (The area is destroyed when the chart
  // widget is destroyed during this classes destructor.)
  this->GetChartArea()->addLayer(chartLayer);
}

//----------------------------------------------------------------------------
void vtkQtChartViewBase::Show()
{
  this->GetChartWidget()->show();
}

//----------------------------------------------------------------------------
void vtkQtChartViewBase::AddTableToView(vtkTable* table)
{
  this->AddRepresentationFromInput(table);
}

//----------------------------------------------------------------------------
void vtkQtChartViewBase::SetTitle(const char* title)
{
  vtkQtChartWidget* cw = this->GetChartWidget();
  vtkQtChartTitle* titleObj = cw->getTitle();
  if (!titleObj)
    {
    titleObj = new vtkQtChartTitle;
    cw->setTitle(titleObj);
    }
  titleObj->setText(title);
}

//----------------------------------------------------------------------------
vtkQtChartSeriesLayer* vtkQtChartViewBase::GetChartLayer()
{
  return this->Internal->ChartLayer;
}

//----------------------------------------------------------------------------
vtkQtChartWidget* vtkQtChartViewBase::GetChartWidget()
{
  return &this->Internal->ChartWidget;
}

//----------------------------------------------------------------------------
vtkQtChartArea* vtkQtChartViewBase::GetChartArea()
{
  return this->GetChartWidget()->getChartArea();
}

//----------------------------------------------------------------------------
vtkQtChartSeriesModelCollection* vtkQtChartViewBase::GetChartSeriesModel()
{
  return this->Internal->ModelCollection;
}

//----------------------------------------------------------------------------
vtkQtChartLegendModel* vtkQtChartViewBase::GetLegendModel()
{
  return this->Internal->Legend->getModel();
}

//----------------------------------------------------------------------------
vtkQtChartLegend* vtkQtChartViewBase::GetLegend()
{
  return this->Internal->Legend;
}

//----------------------------------------------------------------------------
void vtkQtChartViewBase::Update()
{
  for (int i = 0; i < this->GetNumberOfRepresentations(); ++i)
    {
    vtkQtChartTableRepresentation* rep =
      vtkQtChartTableRepresentation::SafeDownCast(this->GetRepresentation(i));
    if (rep)
      {
      rep->Update();
      }
    }
  
  if (this->GetChartLayer())
    {
    this->GetChartLayer()->update();
    }
}

//----------------------------------------------------------------------------
void vtkQtChartViewBase::Render()
{

}

//----------------------------------------------------------------------------
void vtkQtChartViewBase::SetupDefaultAxes()
{
  /* Don't do anything, for now.

  // Get the axes layer
  vtkQtChartAxisLayer *axesLayer = this->GetChartArea()->getAxisLayer();

  // Set the left axis behavior to best fit
  axesLayer->setAxisBehavior(vtkQtChartAxis::Left,
                                vtkQtChartAxisLayer::BestFit);

  // Set the bottom axis behavior to best fit
  axesLayer->setAxisBehavior(vtkQtChartAxis::Bottom,
                                vtkQtChartAxisLayer::BestFit);

  // Set the left axis best fit range
  vtkQtChartAxis* leftAxis = axesLayer->getAxis(vtkQtChartAxis::Left);
  leftAxis->setBestFitRange(QVariant((float)0.0), QVariant((float)10));

  // Set the bottom axis best fit range
  vtkQtChartAxis* bottomAxis = axesLayer->getAxis(vtkQtChartAxis::Bottom);
  bottomAxis->setBestFitRange(QVariant((float)0.0), QVariant((float)10));
  */
}

//----------------------------------------------------------------------------
void vtkQtChartViewBase::SetupDefaultColorScheme()
{

  vtkQtChartStyleManager* style = this->GetChartArea()->getStyleManager();
  vtkQtChartColorStyleGenerator* generator =
      qobject_cast<vtkQtChartColorStyleGenerator*>(style->getGenerator());
  if(generator)
    {
    generator->getColors()->setColorScheme(vtkQtChartColors::Blues);
    }
  else
    {
    style->setGenerator(new vtkQtChartColorStyleGenerator(
              this->GetChartArea(), vtkQtChartColors::Blues));
    }
}

//----------------------------------------------------------------------------
void vtkQtChartViewBase::SetupDefaultInteractor()
{
  vtkQtChartMouseSelection *selector =
      vtkQtChartInteractorSetup::createDefault(this->GetChartArea());
  vtkQtChartSeriesSelectionHandler *handler =
      new vtkQtChartSeriesSelectionHandler(selector);
  handler->setModeNames("Series", "Single");
  handler->setMousePressModifiers(Qt::ControlModifier, Qt::ControlModifier);
  handler->setLayer(this->GetChartLayer());
  selector->addHandler(handler);
  selector->setSelectionMode("Single");
}

//----------------------------------------------------------------------------
void vtkQtChartViewBase::Initialize()
{
  this->SetupDefaultAxes();
  this->SetupDefaultColorScheme();
  this->SetupDefaultInteractor();
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkQtChartViewBase::CreateDefaultRepresentation(vtkAlgorithmOutput* conn)
{
  vtkDataRepresentation* rep = vtkQtChartTableRepresentation::New();
  rep->SetInputConnection(conn);
  return rep;
}

//----------------------------------------------------------------------------
void vtkQtChartViewBase::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
