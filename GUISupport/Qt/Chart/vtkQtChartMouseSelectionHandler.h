/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartMouseSelectionHandler.h

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

/// \file vtkQtChartMouseSelectionHandler.h
/// \date March 19, 2008

#ifndef _vtkQtChartMouseSelectionHandler_h
#define _vtkQtChartMouseSelectionHandler_h

#include "vtkQtChartExport.h"
#include <QObject>

class vtkQtChartContentsSpace;
class vtkQtChartMouseBox;
class QMouseEvent;
class QString;
class QStringList;


class VTKQTCHART_EXPORT vtkQtChartMouseSelectionHandler : public QObject
{
public:
  vtkQtChartMouseSelectionHandler(QObject *parent=0);
  virtual ~vtkQtChartMouseSelectionHandler() {}

  /// \brief
  ///   Gets the chart mouse box object.
  /// \return
  ///   A pointer to the chart mouse box object.
  vtkQtChartMouseBox *getMouseBox() const {return this->MouseBox;}

  /// \brief
  ///   Sets the chart mouse box object.
  /// \param box The chart mouse box object to use.
  virtual void setMouseBox(vtkQtChartMouseBox *box) {this->MouseBox = box;}

  virtual int getNumberOfModes() const=0;
  virtual void getModeList(QStringList &list) const=0;

  virtual bool mousePressEvent(const QString &mode, QMouseEvent *e,
      vtkQtChartContentsSpace *contents)=0;
  virtual bool isMouseMoveAvailable(const QString &mode) const=0;
  virtual void startMouseMove(const QString &mode)=0;
  virtual void mouseMoveEvent(const QString &mode, QMouseEvent *e,
      vtkQtChartContentsSpace *contents)=0;
  virtual void finishMouseMove(const QString &mode)=0;
  virtual bool mouseReleaseEvent(const QString &mode, QMouseEvent *e,
      vtkQtChartContentsSpace *contents)=0;
  virtual bool mouseDoubleClickEvent(const QString &mode, QMouseEvent *e,
      vtkQtChartContentsSpace *contents)=0;

protected:
  vtkQtChartMouseBox *MouseBox; ///< Stores the mouse box.
};

#endif
