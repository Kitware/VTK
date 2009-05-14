/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartLegend.h

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

/// \file vtkQtChartLegend.h
/// \date February 12, 2008

#ifndef _vtkQtChartLegend_h
#define _vtkQtChartLegend_h


#include "vtkQtChartExport.h"
#include <QWidget>

class vtkQtChartLegendInternal;
class vtkQtChartLegendModel;
class QFont;
class QPainter;
class QPoint;
class QRect;


/// \class vtkQtChartLegend
/// \brief
///   The vtkQtChartLegend class displays a chart legend.
///
/// A vtkQtChartLegendModel is used to describe the entries. Each entry
/// can have an icon and a label. The icon is used to visually
/// identify the series on the chart. For a line chart series, the
/// image should be drawn in the same color and line style.
class VTKQTCHART_EXPORT vtkQtChartLegend : public QWidget
{
  Q_OBJECT

public:
  enum LegendLocation
    {
    Left = 0,  ///< Place the legend on the left of the chart.
    Top,       ///< Place the legend on the top of the chart.
    Right,     ///< Place the legend on the right of the chart.
    Bottom     ///< Place the legend on the bottom of the chart.
    };

  enum ItemFlow
    {
    LeftToRight = 0, ///< Items are arranged left to right.
    TopToBottom      ///< Items are arranged top to bottom.
    };

public:
  /// \brief
  ///   Creates a chart legend instance.
  /// \param parent The parent widget.
  vtkQtChartLegend(QWidget *parent=0);
  virtual ~vtkQtChartLegend();

  /// \name Setup Methods
  //@{
  /// \brief
  ///   Gets the legend model.
  /// \return
  ///   A pointer to the legend model.
  vtkQtChartLegendModel *getModel() const {return this->Model;}

  /// \brief
  ///   Gets the legend location.
  /// \return
  ///   The legend location.
  LegendLocation getLocation() const {return this->Location;}

  /// \brief
  ///   Sets the legend location.
  ///
  /// The chart uses the location to place the legend in the
  /// appropriate place. The combination of location and flow
  /// determine how the legend looks.
  ///
  /// \param location The new legend location.
  void setLocation(LegendLocation location);

  /// \brief
  ///   Gets the legend item flow.
  /// \return
  ///   The legend item flow.
  ItemFlow getFlow() const {return this->Flow;}

  /// \brief
  ///   Sets the legend item flow.
  ///
  /// The flow is used to determine the layout direction of the
  /// legend entries. Depending on the location, the same flow type
  /// can look different.
  ///
  /// \param flow The new item flow.
  void setFlow(ItemFlow flow);
  //@}

  /// \brief
  ///   Gets the panning offset.
  /// \return
  ///   The current panning offset.
  int getOffset() const;

  /// \brief
  ///   Gets the preferred size of the chart legend.
  /// \return
  ///   The preferred size of the chart legend.
  virtual QSize sizeHint() const {return this->Bounds;}

  /// \brief
  ///   Draws the legend using the given painter.
  /// \param painter The painter to use.
  void drawLegend(QPainter &painter);

signals:
  /// Emitted when the legend location is changed.
  void locationChanged();

public slots:
  /// Resets the chart legend.
  void reset();

  /// \brief
  ///   Sets the panning offset.
  ///
  /// The offset is applied to the x or y axis depending on the
  /// legend's location.
  ///
  /// \param offset The new panning offset.
  void setOffset(int offset);

protected slots:
  /// \brief
  ///   Inserts a new entry in the legend.
  /// \param index Where to insert the entry.
  void insertEntry(int index);

  /// \brief
  ///   Starts the entry removal process.
  /// \param index The entry being removed.
  void startEntryRemoval(int index);

  /// \brief
  ///   Finishes the entry removal process.
  /// \param index The entry that was removed.
  void finishEntryRemoval(int index);

  /// \brief
  ///   Updates the text for the given entry.
  /// \param index The index of the modified entry.
  void updateEntryText(int index);

  /// \brief
  ///   Updates the visibility for the given entry.
  /// \param index The index of the modified entry.
  void updateEntryVisible(int index);

protected:
  /// \brief
  ///   Updates the layout when the font changes.
  /// \param e Event specific information.
  /// \return
  ///   True if the event was handled.
  virtual bool event(QEvent *e);

  /// \brief
  ///   Draws the chart title.
  /// \param e Event specific information.
  virtual void paintEvent(QPaintEvent *e);

  /// \brief
  ///   Updates the maximum offset when the size changes.
  /// \param e Event specific information.
  virtual void resizeEvent(QResizeEvent *e);

  /// \brief
  ///   Used for panning the contents of the legend.
  ///
  /// The widget cursor is set for panning.
  ///
  /// \param e Event specific information.
  virtual void mousePressEvent(QMouseEvent *e);

  /// \brief
  ///   Used for panning the contents of the legend.
  ///
  /// The conents are moved by changing the offset.
  ///
  /// \param e Event specific information.
  virtual void mouseMoveEvent(QMouseEvent *e);

  /// \brief
  ///   Used for panning the contents of the legend.
  ///
  /// The widget cursor is reset after panning.
  ///
  /// \param e Event specific information.
  virtual void mouseReleaseEvent(QMouseEvent *e);

private:
  /// Calculates the preferred size of the chart legend.
  void calculateSize();

  /// Sets the maximum offset using the contents size.
  void updateMaximum();

private:
  vtkQtChartLegendInternal *Internal; ///< Stores the graphical items.
  vtkQtChartLegendModel *Model;       ///< A pointer to the model.
  LegendLocation Location;            ///< Stores the legend location.
  ItemFlow Flow;                      ///< Stores the order of the items.
  QSize Bounds;                       ///< Stores the prefered size.
  int IconSize;                       ///< Stores the icon size.
  int TextSpacing;                    ///< The space between icon and text.
  int Margin;                         ///< The margin around the entries.

private:
  vtkQtChartLegend(const vtkQtChartLegend &);
  vtkQtChartLegend &operator=(const vtkQtChartLegend &);
};

#endif
