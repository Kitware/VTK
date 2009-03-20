/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartStyleBoolean.h

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

/// \file vtkQtChartStyleBoolean.h
/// \date March 19, 2009

#ifndef _vtkQtChartStyleBoolean_h
#define _vtkQtChartStyleBoolean_h


#include "vtkQtChartExport.h"
#include <QObject>


/// \class vtkQtChartStyleBoolean
/// \brief
///   The vtkQtChartStyleBoolean class is the interface for series
///   boolean options.
class VTKQTCHART_EXPORT vtkQtChartStyleBoolean : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a chart style boolean.
  /// \param parent The parent object.
  vtkQtChartStyleBoolean(QObject *parent=0);
  virtual ~vtkQtChartStyleBoolean() {}

  /// \brief
  ///   Gets the boolean for the given style index.
  /// \param index The style index.
  /// \return
  ///   The boolean for the given style index.
  virtual bool getStyleBoolean(int index) const = 0;

private:
  vtkQtChartStyleBoolean(const vtkQtChartStyleBoolean &);
  vtkQtChartStyleBoolean &operator=(const vtkQtChartStyleBoolean &);
};

#endif
