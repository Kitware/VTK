/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartWidget.h

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

/// \file vtkQtChartWidget.h
/// \date 11/21/2006

#ifndef _vtkQtChartWidget_h
#define _vtkQtChartWidget_h


#include "vtkQtChartExport.h"
#include <QWidget>
#include "vtkQtChartAxis.h" // Needed for enum

class vtkQtChartLegend;
class vtkQtChartTitle;
class vtkQtChartArea;
class QGridLayout;
class QHBoxLayout;
class QPrinter;
class QString;
class QStringList;
class QVBoxLayout;



/// \class vtkQtChartWidget
/// \brief
///   The vtkQtChartWidget class is a container for the chart widgets.
///
/// The main charting widget is the chart area. It holds the chart
/// layers. The title and legend widgets are arranged around the
/// chart area. A title can be added for each axis as well as an
/// overall title for the chart.
///
/// The main chart area is created and owned by the chart widget. The
/// other widgets should be created and passed in.
class VTKQTCHART_EXPORT vtkQtChartWidget : public QWidget
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a chart widget instance.
  /// \param parent The parent widget.
  vtkQtChartWidget(QWidget *parent=0);
  virtual ~vtkQtChartWidget();

  /// \brief
  ///   Gets the overall title for the chart.
  /// \return
  ///   A pointer to the overall title for the chart.
  vtkQtChartTitle *getTitle() const {return this->Title;}

  /// \brief
  ///   Sets the overall title for the chart.
  /// \param title The new chart title.
  void setTitle(vtkQtChartTitle *title);

  /// \brief
  ///   Gets the chart legend.
  /// \return
  ///   A pointer to the chart legend.
  vtkQtChartLegend *getLegend() const {return this->Legend;}

  /// \brief
  ///   Sets the chart legend.
  /// \param legend The new chart legend.
  void setLegend(vtkQtChartLegend *legend);

  /// \brief
  ///   Gets the main chart area.
  /// \return
  ///   A pointer to the main chart area.
  vtkQtChartArea *getChartArea() const {return this->Charts;}

  /// \brief
  ///   Gets the title for the given axis location.
  /// \param axis The axis location on the chart.
  /// \return
  ///   A pointer to the axis title.
  vtkQtChartTitle *getAxisTitle(vtkQtChartAxis::AxisLocation axis) const;

  /// \brief
  ///   Sets the title for the given axis location.
  /// \param axis The axis location on the chart.
  /// \param title The new axis title.
  void setAxisTitle(vtkQtChartAxis::AxisLocation axis, vtkQtChartTitle *title);

  /// \brief
  ///   Gets the preferred size of the chart.
  /// \return
  ///   The preferred size of the chart.
  virtual QSize sizeHint() const;

public slots:
  /// \brief
  ///   Prints the chart using the given printer.
  /// \param printer The printer to use.
  void printChart(QPrinter &printer);

  /// \brief
  ///   Saves a screenshot of the chart to the given files.
  /// \param files The list of files to write.
  void saveChart(const QStringList &files);

  /// \brief
  ///   Saves a screenshot of the chart to the given file.
  /// \param filename The name of the file to write.
  void saveChart(const QString &filename);

signals:
  /// \brief
  ///   Emitted when a new chart title has been set.
  /// \param title The new chart title.
  void newChartTitle(vtkQtChartTitle *title);

  /// \brief
  ///   Emitted when a new chart legend has been set.
  /// \param legend The new chart legend.
  void newChartLegend(vtkQtChartLegend *legend);

  /// \brief
  ///   Emitted when a new axis title has been set.
  /// \param axis The axis location.
  /// \param title The new axis title.
  void newAxisTitle(vtkQtChartAxis::AxisLocation axis, vtkQtChartTitle *title);

private slots:
  /// Moves the legend when the location changes.
  void changeLegendLocation();

private:
  vtkQtChartTitle *Title;       ///< Stores the chart title.
  vtkQtChartLegend *Legend;     ///< Stores the chart legend.
  vtkQtChartArea *Charts;       ///< Stores the chart area.
  vtkQtChartTitle *LeftTitle;   ///< Stores the left axis title.
  vtkQtChartTitle *TopTitle;    ///< Stores the top axis title.
  vtkQtChartTitle *RightTitle;  ///< Stores the right axis title.
  vtkQtChartTitle *BottomTitle; ///< Stores the bottom axis title.
  QVBoxLayout *TitleLayout;     ///< Layout for the chart title.
  QGridLayout *LegendLayout;    ///< Layout for the chart legend.
  QVBoxLayout *TopLayout;       ///< Layout for the top and bottom titles.
  QHBoxLayout *ChartLayout;     ///< Layout for the chart and other titles.
};

#endif
