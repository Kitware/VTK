/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartTitle.h

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

/// \file vtkQtChartTitle.h
/// \date 11/17/2006

#ifndef _vtkQtChartTitle_h
#define _vtkQtChartTitle_h


#include "vtkQtChartExport.h"
#include <QWidget>
#include <QString> // Needed for return type

class QPainter;


/// \class vtkQtChartTitle
/// \brief
///   The vtkQtChartTitle class is used to draw a chart title.
///
/// The text for the title can be drawn horizontally or vertically.
/// This allows the title to be used on a vertical axis.
class VTKQTCHART_EXPORT vtkQtChartTitle : public QWidget
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a chart title instance.
  /// \param orient The orientation of the title.
  /// \param parent The parent widget.
  vtkQtChartTitle(Qt::Orientation orient=Qt::Horizontal, QWidget *parent=0);
  virtual ~vtkQtChartTitle() {}

  /// \brief
  ///   Gets the orientation of the chart title.
  /// \return
  ///   The orientation of the chart title.
  Qt::Orientation getOrientation() const {return this->Orient;}

  /// \brief
  ///   Sets the orientation of the chart title.
  /// \param orient The orientation of the title.
  void setOrientation(Qt::Orientation orient);

  /// \brief
  ///   Gets the chart title text.
  /// \return
  ///   The chart title text.
  QString getText() const {return this->Text;}

  /// \brief
  ///   Sets the chart title text.
  /// \param text The text to display.
  void setText(const QString &text);

  /// \brief
  ///   Gets the text alignment flags for the title.
  /// \return
  ///   The text alignment flags for the title.
  int getTextAlignment() const {return this->Align;}

  /// \brief
  ///   Sets the text alignment flags for the title.
  /// \param flags The text alignment flags to use.
  void setTextAlignment(int flags) {this->Align = flags;}

  /// \brief
  ///   Gets the preferred size of the chart title.
  /// \return
  ///   The preferred size of the chart title.
  virtual QSize sizeHint() const {return this->Bounds;}

  /// \brief
  ///   Draws the title using the given painter.
  /// \param painter The painter to use.
  void drawTitle(QPainter &painter);

signals:
  /// Emitted when the title orientation has changed.
  void orientationChanged();

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

private:
  /// Calculates the preferred size of the chart title.
  void calculateSize();

private:
  QString Text;           ///< Stores the display text.
  QSize Bounds;           ///< Stores the preferred size.
  Qt::Orientation Orient; ///< Stores the title orientation.
  int Align;              ///< Stores the text alignment flags.
};

#endif
