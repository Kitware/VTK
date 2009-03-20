/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartStyleSeriesColors.h

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

/// \file vtkQtChartStyleSeriesColors.h
/// \date March 19, 2009

#ifndef _vtkQtChartStyleSeriesColors_h
#define _vtkQtChartStyleSeriesColors_h

#include "vtkQtChartExport.h"
#include <QObject>

class vtkQtChartSeriesColors;


/// \class vtkQtChartStyleSeriesColors
/// \brief
///   The vtkQtChartStyleSeriesColors class is the interface for
///   multi-colored series options.
class VTKQTCHART_EXPORT vtkQtChartStyleSeriesColors : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a chart style series colors.
  /// \param parent The parent object.
  vtkQtChartStyleSeriesColors(QObject *parent=0);
  virtual ~vtkQtChartStyleSeriesColors() {}

  /// \brief
  ///   Gets the series colors for the given style index.
  /// \param index The style index.
  /// \return
  ///   A pointer to the series colors object.
  virtual vtkQtChartSeriesColors *getStyleColors(int index) const = 0;

private:
  vtkQtChartStyleSeriesColors(const vtkQtChartStyleSeriesColors &);
  vtkQtChartStyleSeriesColors &operator=(const vtkQtChartStyleSeriesColors &);
};

#endif
