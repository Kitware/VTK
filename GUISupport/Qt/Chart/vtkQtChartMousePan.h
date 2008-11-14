/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartMousePan.h

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

/// \file vtkQtChartMousePan.h
/// \date March 11, 2008

#ifndef _vtkQtChartMousePan_h
#define _vtkQtChartMousePan_h


#include "vtkQtChartExport.h"
#include "vtkQtChartMouseFunction.h"

class vtkQtChartContentsSpace;
class vtkQtChartMousePanInternal;
class QMouseEvent;


/// \class vtkQtChartMousePan
/// \brief
///   The vtkQtChartMousePan class pans the contents in response to
///   mouse events.
class VTKQTCHART_EXPORT vtkQtChartMousePan : public vtkQtChartMouseFunction
{
public:
  /// \brief
  ///   Creates a mouse pan instance.
  /// \param parent Te parent object.
  vtkQtChartMousePan(QObject *parent=0);
  virtual ~vtkQtChartMousePan();

  /// \name vtkQtChartMouseFunction Methods
  //@{
  virtual void setMouseOwner(bool owns);

  virtual bool mousePressEvent(QMouseEvent *e, vtkQtChartArea *chart);
  virtual bool mouseMoveEvent(QMouseEvent *e, vtkQtChartArea *chart);
  virtual bool mouseReleaseEvent(QMouseEvent *e, vtkQtChartArea *chart);
  virtual bool mouseDoubleClickEvent(QMouseEvent *e, vtkQtChartArea *chart);
  //@}

private:
  vtkQtChartMousePanInternal *Internal; ///< Stores the last mouse position.

private:
  vtkQtChartMousePan(const vtkQtChartMousePan &);
  vtkQtChartMousePan &operator=(const vtkQtChartMousePan &);
};

#endif
