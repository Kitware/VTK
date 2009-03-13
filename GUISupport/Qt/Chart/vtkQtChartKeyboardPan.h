/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartKeyboardPan.h

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

/// \file vtkQtChartKeyboardPan.h
/// \date February 23, 2009

#ifndef _vtkQtChartKeyboardPan_h
#define _vtkQtChartKeyboardPan_h


#include "vtkQtChartExport.h"
#include "vtkQtChartKeyboardFunction.h"


/// \class vtkQtChartKeyboardPan
/// \brief
///   The vtkQtChartKeyboardPan class pans the chart contents right.
class VTKQTCHART_EXPORT vtkQtChartKeyboardPan :
    public vtkQtChartKeyboardFunction
{
public:
  /// \brief
  ///   Creates a chart keyboard pan right instance.
  /// \param parent The parent object.
  vtkQtChartKeyboardPan(QObject *parent=0);
  virtual ~vtkQtChartKeyboardPan() {}

  /// Pans the chart contents right.
  virtual void activate();

private:
  vtkQtChartKeyboardPan(const vtkQtChartKeyboardPan &);
  vtkQtChartKeyboardPan &operator=(const vtkQtChartKeyboardPan &);
};


/// \class vtkQtChartKeyboardPanLeft
/// \brief
///   The vtkQtChartKeyboardPanLeft class pans the chart contents left.
class VTKQTCHART_EXPORT vtkQtChartKeyboardPanLeft :
    public vtkQtChartKeyboardFunction
{
public:
  /// \brief
  ///   Creates a chart keyboard pan left instance.
  /// \param parent The parent object.
  vtkQtChartKeyboardPanLeft(QObject *parent=0);
  virtual ~vtkQtChartKeyboardPanLeft() {}

  /// Pans the chart contents left.
  virtual void activate();

private:
  vtkQtChartKeyboardPanLeft(const vtkQtChartKeyboardPanLeft &);
  vtkQtChartKeyboardPanLeft &operator=(const vtkQtChartKeyboardPanLeft &);
};


/// \class vtkQtChartKeyboardPanDown
/// \brief
///   The vtkQtChartKeyboardPanDown class pans the chart contents down.
class VTKQTCHART_EXPORT vtkQtChartKeyboardPanDown :
    public vtkQtChartKeyboardFunction
{
public:
  /// \brief
  ///   Creates a chart keyboard pan down instance.
  /// \param parent The parent object.
  vtkQtChartKeyboardPanDown(QObject *parent=0);
  virtual ~vtkQtChartKeyboardPanDown() {}

  /// Pans the chart contents down.
  virtual void activate();

private:
  vtkQtChartKeyboardPanDown(const vtkQtChartKeyboardPanDown &);
  vtkQtChartKeyboardPanDown &operator=(const vtkQtChartKeyboardPanDown &);
};


/// \class vtkQtChartKeyboardPanUp
/// \brief
///   The vtkQtChartKeyboardPanUp class pans the chart contents up.
class VTKQTCHART_EXPORT vtkQtChartKeyboardPanUp :
    public vtkQtChartKeyboardFunction
{
public:
  /// \brief
  ///   Creates a chart keyboard pan up instance.
  /// \param parent The parent object.
  vtkQtChartKeyboardPanUp(QObject *parent=0);
  virtual ~vtkQtChartKeyboardPanUp() {}

  /// Pans the chart contents up.
  virtual void activate();

private:
  vtkQtChartKeyboardPanUp(const vtkQtChartKeyboardPanUp &);
  vtkQtChartKeyboardPanUp &operator=(const vtkQtChartKeyboardPanUp &);
};

#endif
