/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartStyleManager.h

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

/// \file vtkQtChartStyleManager.h
/// \date February 15, 2008

#ifndef _vtkQtChartStyleManager_h
#define _vtkQtChartStyleManager_h


#include "vtkQtChartExport.h"
#include <QObject>

class vtkQtChartSeriesLayer;
class vtkQtChartSeriesOptions;
class vtkQtChartStyleManagerInternal;
class QString;


/// \class vtkQtChartStyleManager
/// \brief
///   The vtkQtChartStyleManager class allows several chart layers
///   to share the same style generators.
///
/// Sharing style generators keeps the style from repeating. This is
/// useful when several chart layers are displayed in the same chart.
/// For example, a line chart and a bar chart can share a style
/// generator to make sure that none of the series are the same color.
class VTKQTCHART_EXPORT vtkQtChartStyleManager : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a chart style manager.
  /// \param parent The parent object.
  vtkQtChartStyleManager(QObject *parent=0);
  virtual ~vtkQtChartStyleManager();

  /// \name Style Setup Methods
  //@{
  virtual int getStyleIndex(vtkQtChartSeriesLayer *layer,
      vtkQtChartSeriesOptions *options) const = 0;

  virtual int insertStyle(vtkQtChartSeriesLayer *layer,
      vtkQtChartSeriesOptions *options) = 0;

  virtual void removeStyle(vtkQtChartSeriesLayer *layer,
      vtkQtChartSeriesOptions *options) = 0;
  //@}

  /// \name Generator Methods
  //@{
  QObject *getGenerator(const QString &name) const;

  void setGenerator(const QString &name, QObject *generator);

  void removeGenerator(const QString &name);

  void removeGenerator(QObject *generator);
  //@}

private:
  /// Stores the style generators.
  vtkQtChartStyleManagerInternal *Internal;

private:
  vtkQtChartStyleManager(const vtkQtChartStyleManager &);
  vtkQtChartStyleManager &operator=(const vtkQtChartStyleManager &);
};

#endif
