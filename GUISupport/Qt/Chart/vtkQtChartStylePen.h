/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartStylePen.h

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

/// \file vtkQtChartStylePen.h
/// \date March 16, 2009

#ifndef _vtkQtChartStylePen_h
#define _vtkQtChartStylePen_h


#include "vtkQtChartExport.h"
#include <QObject>
#include <QPen> // needed for return type


/// \class vtkQtChartStylePen
/// \brief
///   The vtkQtChartStylePen class is the interface for series pen
///   options.
class VTKQTCHART_EXPORT vtkQtChartStylePen : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a chart style pen.
  /// \param parent The parent object.
  vtkQtChartStylePen(QObject *parent=0);
  virtual ~vtkQtChartStylePen() {}

  /// \brief
  ///   Gets the pen for the given style index.
  /// \param index The style index.
  /// \return
  ///   The pen for the given style index.
  virtual QPen getStylePen(int index) const = 0;

private:
  vtkQtChartStylePen(const vtkQtChartStylePen &);
  vtkQtChartStylePen &operator=(const vtkQtChartStylePen &);
};

#endif
