/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartStyleAxesCorner.h

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

/// \file vtkQtChartStyleAxesCorner.h
/// \date March 19, 2009

#ifndef _vtkQtChartStyleAxesCorner_h
#define _vtkQtChartStyleAxesCorner_h


#include "vtkQtChartExport.h"
#include <QObject>
#include "vtkQtChartLayer.h" // needed for return value


/// \class vtkQtChartStyleAxesCorner
/// \brief
///   The vtkQtChartStyleAxesCorner class is the interface for series
///   axes corner options.
class VTKQTCHART_EXPORT vtkQtChartStyleAxesCorner : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a chart style axes corner.
  /// \param parent The parent object.
  vtkQtChartStyleAxesCorner(QObject *parent=0);
  virtual ~vtkQtChartStyleAxesCorner() {}

  /// \brief
  ///   Gets the axes corner for the given style index.
  /// \param index The style index.
  /// \return
  ///   The axes corner for the given style index.
  virtual vtkQtChartLayer::AxesCorner getStyleAxesCorner(int index) const = 0;

private:
  vtkQtChartStyleAxesCorner(const vtkQtChartStyleAxesCorner &);
  vtkQtChartStyleAxesCorner &operator=(const vtkQtChartStyleAxesCorner &);
};

#endif
