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
  ///   Sets the series brush using the style generator.
  /// \param style The style index for the generator.
  /// \param generator The style generator to use.
  virtual void setStyle(int style, vtkQtChartStyleGenerator *generator);
};

#endif
