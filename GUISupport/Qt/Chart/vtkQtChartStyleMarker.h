/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartStyleMarker.h

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

/// \file vtkQtChartStyleMarker.h
/// \date March 19, 2009

#ifndef _vtkQtChartStyleMarker_h
#define _vtkQtChartStyleMarker_h


#include "vtkQtChartExport.h"
#include <QObject>
#include "vtkQtPointMarker.h" // needed for return value


/// \class vtkQtChartStyleMarker
/// \brief
///   The vtkQtChartStyleMarker class is the interface for series
///   marker style options.
class VTKQTCHART_EXPORT vtkQtChartStyleMarker : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a chart style marker.
  /// \param parent The parent object.
  vtkQtChartStyleMarker(QObject *parent=0);
  virtual ~vtkQtChartStyleMarker() {}

  /// \brief
  ///   Gets the marker style for the given style index.
  /// \param index The style index.
  /// \return
  ///   The marker style for the given style index.
  virtual vtkQtPointMarker::MarkerStyle getStyleMarker(int index) const = 0;

private:
  vtkQtChartStyleMarker(const vtkQtChartStyleMarker &);
  vtkQtChartStyleMarker &operator=(const vtkQtChartStyleMarker &);
};

#endif
