/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartView.cxx

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

#include "vtkQtChartView.h"

#include "vtkQtChartArea.h"
#include "vtkQtChartAxis.h"
#include "vtkQtChartAxisLayer.h"
#include "vtkQtChartAxisOptions.h"
#include "vtkQtChartBasicStyleManager.h"
#include "vtkQtChartColors.h"
#include "vtkQtChartInteractorSetup.h"
#include "vtkQtChartLegend.h"
#include "vtkQtChartLegendManager.h"
#include "vtkQtChartLegendModel.h"
#include "vtkQtChartMouseSelection.h"
#include "vtkQtChartSeriesOptionsModelCollection.h"
#include "vtkQtChartStyleManager.h"
#include "vtkQtChartRepresentation.h"
#include "vtkQtChartTitle.h"
#include "vtkQtChartWidget.h"
#include "vtkTable.h"

#include "vtkObjectFactory.h"

#include <QPointer>
#include <QVector>

class vtkQtChartView::vtkInternal
{
public:

  vtkInternal() :
    Chart(0),
    Legend(0),
    Title(0),
    AxisTitles()
    {
    this->LegendManager = 0;
    this->ShowLegend = true;

    // Set up space for the axis title widgets.
    this->AxisTitles.reserve(4);
    this->AxisTitles.append(0);
    this->AxisTitles.append(0);
    this->AxisTitles.append(0);
    this->AxisTitles.append(0);
    }

  ~vtkInternal()
    {
    // Clean up the leftover widgets.
    if(!this->Chart.isNull())
      {
      delete this->Chart;
      }

    if(!this->Legend.isNull())
      {
      delete this->Legend;
      }

    if(!this->Title.isNull())
      {
      delete this->Title;
      }

    QVector<QPointer<vtkQtChartTitle> >::Iterator iter =
      this->AxisTitles.begin();
    for( ; iter != this->AxisTitles.end(); ++iter)
      {
      if(!iter->isNull())
        {
        delete *iter;
        }
      }
    }

  QPointer<vtkQtChartWidget>          Chart;
  QPointer<vtkQtChartLegend>          Legend;
  QPointer<vtkQtChartTitle>           Title;
  QPointer<vtkQtChartSeriesOptionsModelCollection> OptionsModel;
  QVector<QPointer<vtkQtChartTitle> > AxisTitles;

  vtkQtChartLegendManager*            LegendManager;
  bool                                ShowLegend;
  
private:
  vtkInternal(const vtkInternal&);  // Not implemented.
  vtkInternal& operator=(const vtkInternal&);  // Not implemented.
};

//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
vtkQtChartView::vtkQtChartView()
{
  this->Internal = new vtkInternal();

  // Create the chart widget.
  this->Internal->Chart = new vtkQtChartWidget();
  vtkQtChartArea *area = this->Internal->Chart->getChartArea();

  // Setup the chart legend.
  this->Internal->Legend = new vtkQtChartLegend();
  this->Internal->LegendManager = new vtkQtChartLegendManager(
    this->Internal->Legend);
  this->Internal->LegendManager->setChartLegend(this->Internal->Legend);
  this->Internal->LegendManager->setChartArea(area);
  this->Internal->Chart->setLegend(this->Internal->Legend);

  // Set up the chart titles. The axis titles should be in the same
  // order as the properties: left, bottom, right, top.
  this->Internal->Title = new vtkQtChartTitle();
  this->Internal->AxisTitles[0] = new vtkQtChartTitle(Qt::Vertical);
  this->Internal->AxisTitles[1] = new vtkQtChartTitle();
  this->Internal->AxisTitles[2] = new vtkQtChartTitle(Qt::Vertical);
  this->Internal->AxisTitles[3] = new vtkQtChartTitle();

  this->Internal->OptionsModel =
    new vtkQtChartSeriesOptionsModelCollection(area);
}

//----------------------------------------------------------------------------
vtkQtChartView::~vtkQtChartView()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
vtkQtChartSeriesOptionsModelCollection* vtkQtChartView::GetChartOptionsModel()
{
  return this->Internal->OptionsModel;
}

//----------------------------------------------------------------------------
void vtkQtChartView::Show()
{
  this->Internal->Chart->show();
}

//----------------------------------------------------------------------------
void vtkQtChartView::AddTableToView(vtkTable* table)
{
  this->AddRepresentationFromInput(table);
}

//----------------------------------------------------------------------------
void vtkQtChartView::SetTitle(const char* title)
{
  QString titleText(title);
  if(titleText.isEmpty() && this->Internal->Chart->getTitle() != 0)
    {
    // Remove the chart title.
    this->Internal->Chart->setTitle(0);
    }
  else if(!titleText.isEmpty() && this->Internal->Chart->getTitle() == 0)
    {
    // Add the title to the chart.
    this->Internal->Chart->setTitle(this->Internal->Title);
    }

  this->Internal->Title->setText(titleText);
}

//----------------------------------------------------------------------------
void vtkQtChartView::SetTitleFont(const char *family, int pointSize,
  bool bold, bool italic)
{
  this->Internal->Title->setFont(QFont(family, pointSize,
    bold ? QFont::Bold : -1, italic));
}

//----------------------------------------------------------------------------
void vtkQtChartView::SetTitleColor(double red, double green, double blue)
{
  QPalette palette = this->Internal->Title->palette();
  palette.setColor(QPalette::Text, QColor::fromRgbF(red, green, blue));
  this->Internal->Title->setPalette(palette);
}

//----------------------------------------------------------------------------
void vtkQtChartView::SetTitleAlignment(int alignment)
{
  if(alignment == 0)
    {
    alignment = Qt::AlignLeft;
    }
  else if(alignment == 2)
    {
    alignment = Qt::AlignRight;
    }
  else
    {
    alignment = Qt::AlignCenter;
    }

  this->Internal->Title->setTextAlignment(alignment);
}

//----------------------------------------------------------------------------
void vtkQtChartView::SetAxisTitle(int index, const char* title)
{
  if(index < 0 || index >= 4)
    {
    return;
    }

  vtkQtChartAxis::AxisLocation axes[] =
    {
    vtkQtChartAxis::Left,
    vtkQtChartAxis::Bottom,
    vtkQtChartAxis::Right,
    vtkQtChartAxis::Top
    };

  QString titleText(title);
  if(titleText.isEmpty() &&
    this->Internal->Chart->getAxisTitle(axes[index]) != 0)
    {
    // Remove the chart title.
    this->Internal->Chart->setAxisTitle(axes[index], 0);
    }
  else if(!titleText.isEmpty() &&
    this->Internal->Chart->getAxisTitle(axes[index]) == 0)
    {
    // Add the title to the chart.
    this->Internal->Chart->setAxisTitle(axes[index],
      this->Internal->AxisTitles[index]);
    }

  this->Internal->AxisTitles[index]->setText(titleText);
}

//----------------------------------------------------------------------------
void vtkQtChartView::SetAxisTitleFont(int index, const char* family,
  int pointSize, bool bold, bool italic)
{
  if(index >= 0 && index < 4)
    {
    this->Internal->AxisTitles[index]->setFont(QFont(family, pointSize,
      bold ? QFont::Bold : -1, italic));
    }
}

//----------------------------------------------------------------------------
void vtkQtChartView::SetAxisTitleColor(int index, double red, double green,
  double blue)
{
  if(index >= 0 && index < 4)
    {
    QPalette palette = this->Internal->AxisTitles[index]->palette();
    palette.setColor(QPalette::Text, QColor::fromRgbF(red, green, blue));
    this->Internal->AxisTitles[index]->setPalette(palette);
    }
}

//----------------------------------------------------------------------------
void vtkQtChartView::SetAxisTitleAlignment(int index, int alignment)
{
  if(index < 0 || index >= 4)
    {
    return;
    }

  if(alignment == 0)
    {
    alignment = Qt::AlignLeft;
    }
  else if(alignment == 2)
    {
    alignment = Qt::AlignRight;
    }
  else
    {
    alignment = Qt::AlignCenter;
    }

  this->Internal->AxisTitles[index]->setTextAlignment(alignment);
}

//----------------------------------------------------------------------------
void vtkQtChartView::SetLegendVisibility(bool visible)
{
  this->Internal->ShowLegend = visible;
  if (!this->Internal->ShowLegend && this->Internal->Chart->getLegend() != 0)
    {
    // Remove the legend from the chart since it is not needed.
    this->Internal->Chart->setLegend(0);
    }
  else if (this->Internal->ShowLegend && this->Internal->Chart->getLegend() == 0)
    {
    // Add the legend to the chart since it is needed.
    this->Internal->Chart->setLegend(this->Internal->Legend);
    }
}

//----------------------------------------------------------------------------
void vtkQtChartView::SetLegendLocation(int location)
{
  this->Internal->Legend->setLocation(
    (vtkQtChartLegend::LegendLocation)location);
}

//----------------------------------------------------------------------------
void vtkQtChartView::SetLegendFlow(int flow)
{
  this->Internal->Legend->setFlow((vtkQtChartLegend::ItemFlow)flow);
}

//----------------------------------------------------------------------------
void vtkQtChartView::SetAxisVisibility(int index, bool visible)
{
  vtkQtChartAxis *axis = this->GetAxis(index);
  vtkQtChartAxisOptions *options = axis ? axis->getOptions() : 0;
  if(options)
    {
    options->setVisible(visible);
    }
}

//----------------------------------------------------------------------------
void vtkQtChartView::SetAxisColor(int index, double red, double green,
  double blue)
{
  vtkQtChartAxis *axis = this->GetAxis(index);
  vtkQtChartAxisOptions *options = axis ? axis->getOptions() : 0;
  if(options)
    {
    options->setAxisColor(QColor::fromRgbF(red, green, blue));
    }
}

//----------------------------------------------------------------------------
void vtkQtChartView::SetGridVisibility(int index, bool visible)
{
  vtkQtChartAxis *axis = this->GetAxis(index);
  vtkQtChartAxisOptions *options = axis ? axis->getOptions() : 0;
  if(options)
    {
    options->setGridVisible(visible);
    }
}

//----------------------------------------------------------------------------
void vtkQtChartView::SetGridColorType(int index, int gridColorType)
{
  vtkQtChartAxis *axis = this->GetAxis(index);
  vtkQtChartAxisOptions *options = axis ? axis->getOptions() : 0;
  if(options)
    {
    options->setGridColorType(
      (vtkQtChartAxisOptions::AxisGridColor)gridColorType);
    }
}

//----------------------------------------------------------------------------
void vtkQtChartView::SetGridColor(int index, double red, double green,
  double blue)
{
  vtkQtChartAxis *axis = this->GetAxis(index);
  vtkQtChartAxisOptions *options = axis ? axis->getOptions() : 0;
  if(options)
    {
    options->setGridColor(QColor::fromRgbF(red, green, blue));
    }
}

//----------------------------------------------------------------------------
void vtkQtChartView::SetAxisLabelVisibility(int index, bool visible)
{
  vtkQtChartAxis *axis = this->GetAxis(index);
  vtkQtChartAxisOptions *options = axis ? axis->getOptions() : 0;
  if(options)
    {
    options->setLabelsVisible(visible);
    }
}

//----------------------------------------------------------------------------
void vtkQtChartView::SetAxisLabelFont(int index, const char* family,
  int pointSize, bool bold, bool italic)
{
  vtkQtChartAxis *axis = this->GetAxis(index);
  vtkQtChartAxisOptions *options = axis ? axis->getOptions() : 0;
  if(options)
    {
    options->setLabelFont(QFont(family, pointSize,
      bold ? QFont::Bold : -1, italic));
    }
}

//----------------------------------------------------------------------------
void vtkQtChartView::SetAxisLabelColor(int index, double red, double green,
  double blue)
{
  vtkQtChartAxis *axis = this->GetAxis(index);
  vtkQtChartAxisOptions *options = axis ? axis->getOptions() : 0;
  if(options)
    {
    options->setLabelColor(QColor::fromRgbF(red, green, blue));
    }
}

//----------------------------------------------------------------------------
void vtkQtChartView::SetAxisLabelNotation(int index, int notation)
{
  vtkQtChartAxis *axis = this->GetAxis(index);
  vtkQtChartAxisOptions *options = axis ? axis->getOptions() : 0;
  if(options)
    {
    options->setPrecision((vtkQtChartAxisOptions::NotationType)notation);
    }
}

//----------------------------------------------------------------------------
void vtkQtChartView::SetAxisLabelPrecision(int index, int precision)
{
  vtkQtChartAxis *axis = this->GetAxis(index);
  vtkQtChartAxisOptions *options = axis ? axis->getOptions() : 0;
  if(options)
    {
    options->setPrecision(precision);
    }
}

//----------------------------------------------------------------------------
void vtkQtChartView::SetAxisScale(int index, int scale)
{
  vtkQtChartAxis* axis = this->GetAxis(index);
  vtkQtChartAxisOptions* options = axis ? axis->getOptions() : 0;
  if(options)
    {
    options->setAxisScale((vtkQtChartAxisOptions::AxisScale)scale);
    }
}

//----------------------------------------------------------------------------
void vtkQtChartView::SetAxisBehavior(int index, int behavior)
{
  vtkQtChartAxis* axis = this->GetAxis(index);
  if(axis)
    {
    vtkQtChartArea* area = this->Internal->Chart->getChartArea();
    area->getAxisLayer()->setAxisBehavior(axis->getLocation(),
      (vtkQtChartAxisLayer::AxisBehavior)behavior);
    area->updateLayout();
    }
}

//----------------------------------------------------------------------------
void vtkQtChartView::SetAxisRange(int index, double minimum,
  double maximum)
{
  vtkQtChartAxis* axis = this->GetAxis(index);
  if(axis)
    {
    axis->setBestFitRange(QVariant(minimum), QVariant(maximum));
    vtkQtChartArea* area = this->Internal->Chart->getChartArea();
    if(area->getAxisLayer()->getAxisBehavior(axis->getLocation()) ==
      vtkQtChartAxisLayer::BestFit)
      {
      area->updateLayout();
      }
    }
}

//----------------------------------------------------------------------------
void vtkQtChartView::SetAxisRange(int index, int minimum, int maximum)
{
  vtkQtChartAxis* axis = this->GetAxis(index);
  if(axis)
    {
    axis->setBestFitRange(QVariant(minimum), QVariant(maximum));
    vtkQtChartArea* area = this->Internal->Chart->getChartArea();
    if(area->getAxisLayer()->getAxisBehavior(axis->getLocation()) ==
      vtkQtChartAxisLayer::BestFit)
      {
      area->updateLayout();
      }
    }
}

//----------------------------------------------------------------------------
void vtkQtChartView::AddChartSelectionHandlers(vtkQtChartMouseSelection*)
{
}

//----------------------------------------------------------------------------
QWidget* vtkQtChartView::GetWidget()
{
  return this->Internal->Chart;
}

//----------------------------------------------------------------------------
vtkQtChartArea* vtkQtChartView::GetChartArea()
{
  return this->Internal->Chart->getChartArea();
}

//----------------------------------------------------------------------------
vtkQtChartAxis* vtkQtChartView::GetAxis(int index)
{
  if(index >= 0 && index < 4)
    {
    vtkQtChartArea *area = this->Internal->Chart->getChartArea();
    vtkQtChartAxis::AxisLocation axes[] =
      {
      vtkQtChartAxis::Left,
      vtkQtChartAxis::Bottom,
      vtkQtChartAxis::Right,
      vtkQtChartAxis::Top
      };

    return area->getAxisLayer()->getAxis(axes[index]);
    }

  return 0;
}

//----------------------------------------------------------------------------
vtkQtChartLegend* vtkQtChartView::GetLegend()
{
  return this->Internal->Legend;
}

//----------------------------------------------------------------------------
void vtkQtChartView::Update()
{
  int i = 0;
  for ( ; i < this->GetNumberOfRepresentations(); ++i)
    {
    vtkQtChartRepresentation* rep =
      vtkQtChartRepresentation::SafeDownCast(this->GetRepresentation(i));
    if (rep)
      {
      rep->Update();
      }
    }
}

//----------------------------------------------------------------------------
void vtkQtChartView::Render()
{
  this->Internal->Chart->update();
}

//----------------------------------------------------------------------------
void vtkQtChartView::SetupDefaultInteractor()
{
  vtkQtChartMouseSelection *selector =
    vtkQtChartInteractorSetup::createDefault(this->GetChartArea());
  this->AddChartSelectionHandlers(selector);
  vtkQtChartInteractorSetup::setupDefaultKeys(
    this->GetChartArea()->getInteractor());
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkQtChartView::CreateDefaultRepresentation(vtkAlgorithmOutput* conn)
{
  vtkDataRepresentation* rep = vtkQtChartRepresentation::New();
  rep->SetInputConnection(conn);
  return rep;
}


//----------------------------------------------------------------------------
// Internal helper method to switch color schemes.
// This method may have the side effect of changing the style manager's generator.
namespace {
void SetColorScheme(vtkQtChartStyleManager* styleManager,
                    vtkQtChartColors::ColorScheme scheme)
{
  vtkQtChartBasicStyleManager *manager =
      qobject_cast<vtkQtChartBasicStyleManager *>(styleManager);
  if(manager)
    {
    manager->getColors()->setColorScheme(scheme);
    }
}
}

//----------------------------------------------------------------------------
#define vtkQtChartView_SetColorScheme_macro(scheme)         \
void vtkQtChartView::SetColorSchemeTo##scheme()             \
{                                                               \
  SetColorScheme(this->GetChartArea()->getStyleManager(),       \
                                  vtkQtChartColors::scheme);    \
  this->Update();                                               \
}                                                               

//----------------------------------------------------------------------------
vtkQtChartView_SetColorScheme_macro(Spectrum);
vtkQtChartView_SetColorScheme_macro(Warm);
vtkQtChartView_SetColorScheme_macro(Cool);
vtkQtChartView_SetColorScheme_macro(Blues);
vtkQtChartView_SetColorScheme_macro(WildFlower);
vtkQtChartView_SetColorScheme_macro(Citrus);

//----------------------------------------------------------------------------
void vtkQtChartView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
