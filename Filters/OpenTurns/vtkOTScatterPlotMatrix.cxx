/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOTScatterPlotMatrix.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOTScatterPlotMatrix.h"

#include "vtkAxis.h"
#include "vtkBrush.h"
#include "vtkChart.h"
#include "vtkChartXY.h"
#include "vtkColor.h"
#include "vtkCompositeDataIterator.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkOTDensityMap.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPlotPoints.h"
#include "vtkTable.h"
#include "vtkTextProperty.h"

#include <vector>

// Static density values for now
static const int nDensityValues = 3;
static const double densityValues[3] = { 0.1, 0.5, 0.9 };

// An internal class to store density map settings
class vtkOTScatterPlotMatrix::DensityMapSettings
{
public:
  DensityMapSettings()
  {
    this->PlotPen->SetColor(0, 0, 0, 255);
    this->ShowDensityMap = false;
    this->DensityLineSize = 2;
    for (int i = 0; i < nDensityValues; i++)
    {
      this->DensityMapValues.push_back(densityValues[i]);
      double* rgb = vtkMath::HSVToRGB(densityValues[i], 1, 0.75);
      this->DensityMapColorMap.insert(
        std::make_pair(densityValues[i], vtkColor4ub(rgb[0] * 255, rgb[1] * 255, rgb[2] * 255)));
    }
  }
  ~DensityMapSettings() {}

  vtkNew<vtkPen> PlotPen;
  bool ShowDensityMap;
  float DensityLineSize;
  std::vector<double> DensityMapValues;
  std::map<double, vtkColor4ub> DensityMapColorMap;
};

vtkStandardNewMacro(vtkOTScatterPlotMatrix)

  vtkOTScatterPlotMatrix::vtkOTScatterPlotMatrix()
{
  this->DensityMapsSettings[vtkScatterPlotMatrix::SCATTERPLOT] =
    new vtkOTScatterPlotMatrix::DensityMapSettings;
  this->DensityMapsSettings[vtkScatterPlotMatrix::ACTIVEPLOT] = new DensityMapSettings();
}

vtkOTScatterPlotMatrix::~vtkOTScatterPlotMatrix()
{
  delete this->DensityMapsSettings[vtkScatterPlotMatrix::SCATTERPLOT];
  delete this->DensityMapsSettings[vtkScatterPlotMatrix::ACTIVEPLOT];
}

//---------------------------------------------------------------------------
void vtkOTScatterPlotMatrix::AddSupplementaryPlot(vtkChart* chart,
  int plotType,
  vtkStdString row,
  vtkStdString column,
  int plotCorner)
{
  if (plotType != NOPLOT && plotType != HISTOGRAM &&
    this->DensityMapsSettings[plotType]->ShowDensityMap && !this->Animating)
  {
    DensityMapCacheMap::iterator it = this->DensityMapCache.find(std::make_pair(row, column));
    vtkOTDensityMap* density;
    if (it != this->DensityMapCache.end())
    {
      density = it->second;
    }
    else
    {
      vtkSmartPointer<vtkOTDensityMap> densityPt = vtkSmartPointer<vtkOTDensityMap>::New();
      this->DensityMapCache[std::make_pair(row, column)] = densityPt;
      density = densityPt;
    }

    // Compute density map
    density->SetInputData(this->Input);
    density->SetNumberOfContours(3);
    density->SetValue(0, 0.1);
    density->SetValue(1, 0.5);
    density->SetValue(2, 0.9);
    density->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_ROWS, row);
    density->SetInputArrayToProcess(1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_ROWS, column);
    density->Update();

    // Iterate over multiblock output to drow the density maps
    vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::SafeDownCast(density->GetOutput());
    vtkCompositeDataIterator* iter = mb->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkTable* densityLineTable = vtkTable::SafeDownCast(iter->GetCurrentDataObject());
      if (densityLineTable)
      {
        vtkPlot* densityPlot = chart->AddPlot(vtkChart::LINE);
        vtkChartXY* xy = vtkChartXY::SafeDownCast(chart);
        if (xy)
        {
          xy->AutoAxesOff();
          xy->SetPlotCorner(densityPlot, plotCorner);
          xy->RaisePlot(densityPlot);
        }
        densityPlot->SetInputData(densityLineTable, densityLineTable->GetColumnName(1), row);
        double densityVal = iter->GetCurrentMetaData()->Get(vtkOTDensityMap::DENSITY());
        vtkPen* plotPen = vtkPen::New();
        plotPen->DeepCopy(this->DensityMapsSettings[plotType]->PlotPen);
        plotPen->SetColor(this->DensityMapsSettings[plotType]->DensityMapColorMap[densityVal]);
        densityPlot->SetPen(plotPen);
        plotPen->Delete();
        vtkPlotPoints* plotPoints = vtkPlotPoints::SafeDownCast(densityPlot);
        plotPoints->SetWidth(this->DensityMapsSettings[plotType]->DensityLineSize);
      }
    }
    iter->Delete();
  }
}

//----------------------------------------------------------------------------
void vtkOTScatterPlotMatrix::SetDensityMapVisibility(int plotType, bool visible)
{
  if (plotType != NOPLOT && plotType != HISTOGRAM &&
    this->DensityMapsSettings[plotType]->ShowDensityMap != visible)
  {
    this->DensityMapsSettings[plotType]->ShowDensityMap = visible;
    this->Modified();
    if (plotType == ACTIVEPLOT)
    {
      this->ActivePlotValid = false;
    }
  }
}

//----------------------------------------------------------------------------
void vtkOTScatterPlotMatrix::SetDensityLineSize(int plotType, float size)
{
  if (plotType != NOPLOT && plotType != HISTOGRAM &&
    this->DensityMapsSettings[plotType]->DensityLineSize != size)
  {
    this->DensityMapsSettings[plotType]->DensityLineSize = size;
    this->Modified();
    if (plotType == ACTIVEPLOT)
    {
      this->ActivePlotValid = false;
    }
  }
}

//----------------------------------------------------------------------------
void vtkOTScatterPlotMatrix::SetDensityMapColor(int plotType,
  unsigned int densityLineIndex,
  const vtkColor4ub& color)
{
  if (plotType != NOPLOT && plotType != HISTOGRAM &&
    densityLineIndex < this->DensityMapsSettings[plotType]->DensityMapValues.size())
  {
    double density = this->DensityMapsSettings[plotType]->DensityMapValues[densityLineIndex];
    if (this->DensityMapsSettings[plotType]->DensityMapColorMap[density] != color)
    {
      this->DensityMapsSettings[plotType]->DensityMapColorMap[density] = color;
      this->Modified();
      if (plotType == ACTIVEPLOT)
      {
        this->ActivePlotValid = false;
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkOTScatterPlotMatrix::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}
