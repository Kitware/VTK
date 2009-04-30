/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartLegendManager.h

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

/// \file vtkQtChartLegendManager.h
/// \date January 8, 2009

#ifndef _vtkQtChartLegendManager_h
#define _vtkQtChartLegendManager_h

#include "vtkQtChartExport.h"
#include <QObject>

class vtkQtChartArea;
class vtkQtChartLayer;
class vtkQtChartLegend;
class vtkQtChartLegendManagerInternal;
class vtkQtChartLegendModel;
class vtkQtChartSeriesLayer;
class vtkQtChartSeriesModel;


/// \class vtkQtChartLegendManager
/// \brief
///   The vtkQtChartLegendManager class builds a chart legend from the
///   chart series layers.
class VTKQTCHART_EXPORT vtkQtChartLegendManager : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a chart legend manager instance.
  /// \param parent The parent object.
  vtkQtChartLegendManager(QObject *parent=0);
  virtual ~vtkQtChartLegendManager();

  /// \name Setup Methods
  //@{
  /// \brief
  ///   Sets the chart area that holds the chart series layers.
  /// \param area The new chart area.
  void setChartArea(vtkQtChartArea *area);

  /// \brief
  ///   Sets the chart legend to manage.
  /// \param legend The new chart legend.
  void setChartLegend(vtkQtChartLegend *legend);
  //@}

public slots:
  /// \brief
  ///   Inserts a chart layer at the given index.
  /// \param index The index of the layer.
  /// \param chart The chart layer that was inserted.
  void insertLayer(int index, vtkQtChartLayer *chart);

  /// \brief
  ///   Removes the specified chart layer from the list.
  /// \param index The index of the layer.
  /// \param chart The chart layer that will be removed.
  void removeLayer(int index, vtkQtChartLayer *chart);

  /// \brief
  ///   Sets the visibility for the series in the given chart layer.
  /// \param chart The chart layer.
  /// \param visible True if the layer series should be visible.
  void setLayerVisible(vtkQtChartLayer *chart, bool visible);

private slots:
  /// \brief
  ///   Changes the series model for a series layer.
  /// \param previous The previous series model.
  /// \param current The current series model.
  void changeModel(vtkQtChartSeriesModel *previous,
      vtkQtChartSeriesModel *current);

  /// \brief
  ///   Updates the legend model for series changes.
  /// \param first The first index of the series range.
  /// \param last The last index of the series range.
  void updateModelEntries(int first, int last);

  /// Inserts all the series for the model sending the signal.
  void insertModelEntries();

  /// \brief
  ///   Inserts the given series for the model sending the signal.
  /// \param first The first index of the series range.
  /// \param last The last index of the series range.
  void insertModelEntries(int first, int last);

  /// Removes all the series for the model sending the signal.
  void removeModelEntries();

  /// \brief
  ///   Removes the given series for the model sending the signal.
  /// \param first The first index of the series range.
  /// \param last The last index of the series range.
  void removeModelEntries(int first, int last);

private:
  /// \brief
  ///   Gets the starting legend index for the given chart layer.
  /// \param chart The chart series layer.
  /// \return
  ///   The starting legend index for the given chart layer.
  int getLegendIndex(vtkQtChartSeriesLayer *chart);

  /// \brief
  ///   Gets the starting legend index for the given chart model.
  /// \param model The chart series model.
  /// \param chart Used to return the model's chart layer.
  /// \return
  ///   The starting legend index for the given chart model.
  int getLegendIndex(vtkQtChartSeriesModel *model,
      vtkQtChartSeriesLayer **chart=0);

  /// \brief
  ///   Inserts entries into the chart legend.
  /// \param legend The chart legend model to modify.
  /// \param index The starting legend index for the chart layer.
  /// \param chart The chart series layer.
  /// \param model The chart series model.
  /// \param first The first model index in the series range.
  /// \param last The last model index in the series range.
  void insertLegendEntries(vtkQtChartLegendModel *legend, int index,
      vtkQtChartSeriesLayer *chart, vtkQtChartSeriesModel *model,
      int first, int last);

  /// \brief
  ///   Removes entries from the chart legend.
  /// \param legend The chart legend model to modify.
  /// \param index The starting legend index for the chart layer.
  /// \param first The first model index in the series range.
  /// \param last The last model index in the series range.
  void removeLegendEntries(vtkQtChartLegendModel *legend, int index,
      int first, int last);

private:
  vtkQtChartLegendManagerInternal *Internal; ///< Stores the layers.
  vtkQtChartArea *Area;                      ///< Stores the chart area.
  vtkQtChartLegend *Legend;                  ///< Stores the chart legend.

private:
  vtkQtChartLegendManager(const vtkQtChartLegendManager &);
  vtkQtChartLegendManager &operator=(const vtkQtChartLegendManager &);
};

#endif
