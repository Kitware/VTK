/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartSeriesOptions.h

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

/// \file vtkQtChartSeriesOptions.h
/// \date February 15, 2008

#ifndef _vtkQtChartSeriesOptions_h
#define _vtkQtChartSeriesOptions_h

#include "vtkQtChartExport.h"
#include <QObject>

class QBrush;
class QPen;


/// \class vtkQtChartSeriesOptions
/// \brief
///   The vtkQtChartSeriesOptions class stores the common series
///   drawing options.
class VTKQTCHART_EXPORT vtkQtChartSeriesOptions : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a series options object.
  /// \param parent The parent object.
  vtkQtChartSeriesOptions(QObject *parent=0);
  vtkQtChartSeriesOptions(const vtkQtChartSeriesOptions &other);
  virtual ~vtkQtChartSeriesOptions();

  vtkQtChartSeriesOptions &operator=(const vtkQtChartSeriesOptions &other);

  /// \brief
  ///   Gets whether or not the series should be visible.
  /// \return
  ///   True if the series should be visible.
  bool isVisible() const {return this->Visible;}

  /// \brief
  ///   Sets whether or not the series should be visible.
  /// \param visible True if the series should be visible.
  void setVisible(bool visible);

  /// \brief
  ///   Gets the series pen.
  /// \return
  ///   A reference to the series pen.
  const QPen &getPen() const;

  /// \brief
  ///   Sets the series pen.
  /// \param pen The new series pen.
  void setPen(const QPen &pen);

  /// \brief
  ///   Gets the series brush.
  /// \return
  ///   A reference to the series brush.
  const QBrush &getBrush() const;

  /// \brief
  ///   Sets the series brush.
  /// \param brush The new series brush.
  void setBrush(const QBrush &brush);

signals:
  /// \brief
  ///   Emitted when the series visibility option has changed.
  /// \param visible True if the series should be displayed.
  void visibilityChanged(bool visible);

  /// \brief
  ///   Emitted when the series pen option has changed.
  /// \param pen The new series pen.
  void penChanged(const QPen &pen);

  /// \brief
  ///   Emitted when the series brush option has changed.
  /// \param brush The new series brush.
  void brushChanged(const QBrush &brush);

private:
  QPen *Pen;     ///< Stores the series pen.
  QBrush *Brush; ///< Stores the series brush.
  bool Visible;  ///< True if the series should be displayed.
};

#endif

