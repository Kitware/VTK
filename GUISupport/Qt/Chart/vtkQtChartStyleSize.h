/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartStyleSize.h

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

/// \file vtkQtChartStyleSize.h
/// \date March 19, 2009

#ifndef _vtkQtChartStyleSize_h
#define _vtkQtChartStyleSize_h


#include "vtkQtChartExport.h"
#include <QObject>
#include <QSizeF> // needed for return type


/// \class vtkQtChartStyleSize
/// \brief
///   The vtkQtChartStyleSize class is the interface for series size
///   options.
class VTKQTCHART_EXPORT vtkQtChartStyleSize : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a chart style size.
  /// \param parent The parent object.
  vtkQtChartStyleSize(QObject *parent=0);
  virtual ~vtkQtChartStyleSize() {}

  /// \brief
  ///   Gets the size for the given style index.
  /// \param index The style index.
  /// \return
  ///   The size for the given style index.
  virtual QSizeF getStyleSize(int index) const = 0;

private:
  vtkQtChartStyleSize(const vtkQtChartStyleSize &);
  vtkQtChartStyleSize &operator=(const vtkQtChartStyleSize &);
};

#endif
