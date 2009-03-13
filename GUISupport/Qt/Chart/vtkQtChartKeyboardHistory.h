/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartKeyboardHistory.h

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

/// \file vtkQtChartKeyboardHistory.h
/// \date February 23, 2009

#ifndef _vtkQtChartKeyboardHistory_h
#define _vtkQtChartKeyboardHistory_h


#include "vtkQtChartExport.h"
#include "vtkQtChartKeyboardFunction.h"


/// \class vtkQtChartKeyboardHistory
/// \brief
///   The vtkQtChartKeyboardHistory class navigates backwards in the
///   chart view history.
class VTKQTCHART_EXPORT vtkQtChartKeyboardHistory :
    public vtkQtChartKeyboardFunction
{
public:
  /// \brief
  ///   Creates a chart keyboard history instance.
  /// \param parent The parent object.
  vtkQtChartKeyboardHistory(QObject *parent=0);
  virtual ~vtkQtChartKeyboardHistory() {}

  /// Changes the chart view to the previous view in the history.
  virtual void activate();

private:
  vtkQtChartKeyboardHistory(const vtkQtChartKeyboardHistory &);
  vtkQtChartKeyboardHistory &operator=(const vtkQtChartKeyboardHistory &);
};


/// \class vtkQtChartKeyboardHistoryNext
/// \brief
///   The vtkQtChartKeyboardHistoryNext class navigates forwards in the
///   chart view history.
class VTKQTCHART_EXPORT vtkQtChartKeyboardHistoryNext :
    public vtkQtChartKeyboardFunction
{
public:
  /// \brief
  ///   Creates a chart keyboard history next instance.
  /// \param parent The parent object.
  vtkQtChartKeyboardHistoryNext(QObject *parent=0);
  virtual ~vtkQtChartKeyboardHistoryNext() {}

  /// Changes the chart view to the next view in the history.
  virtual void activate();

private:
  vtkQtChartKeyboardHistoryNext(const vtkQtChartKeyboardHistoryNext &);
  vtkQtChartKeyboardHistoryNext &operator=(
      const vtkQtChartKeyboardHistoryNext &);
};

#endif
