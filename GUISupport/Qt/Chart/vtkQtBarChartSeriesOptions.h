/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtBarChartSeriesOptions.h

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

/// \file vtkQtBarChartSeriesOptions.h
/// \date February 22, 2008

#ifndef _vtkQtBarChartSeriesOptions_h
#define _vtkQtBarChartSeriesOptions_h

#include "vtkQtChartExport.h"
#include "vtkQtChartSeriesOptions.h"


/// \class vtkQtBarChartSeriesOptions
/// \brief
///   The vtkQtBarChartSeriesOptions class stores the drawing options
///   for a bar chart series.
class VTKQTCHART_EXPORT vtkQtBarChartSeriesOptions :
  public vtkQtChartSeriesOptions
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a bar chart series options object.
  /// \param parent The parent object.
  vtkQtBarChartSeriesOptions(QObject *parent=0);
  virtual ~vtkQtBarChartSeriesOptions() {}

  /// \brief
  ///   Gets whether or not the series uses multiple colors.
  /// \return
  ///   True if the series uses multiple colors.
  bool isMultiColored() const {return this->MultiColored;}

  /// \brief
  ///   Sets whether or not the series uses multiple colors.
  /// \param multiColored True if the series should use multiple colors.
  void setMultiColored(bool multiColored);

signals:
  /// Emitted when the multi-colored property changes.
  void multiColoredChanged(bool multiColored);

private:
  bool MultiColored; ///< True if the series uses multiple colors.
};

#endif
