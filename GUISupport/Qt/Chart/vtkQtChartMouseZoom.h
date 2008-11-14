/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartMouseZoom.h

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

/// \file vtkQtChartMouseZoom.h
/// \date March 11, 2008

#ifndef _vtkQtChartMouseZoom_h
#define _vtkQtChartMouseZoom_h


#include "vtkQtChartExport.h"
#include "vtkQtChartMouseFunction.h"

class vtkQtChartArea;
class vtkQtChartMouseZoomInternal;
class QCursor;
class QMouseEvent;


/// \class vtkQtChartMouseZoom
/// \brief
///   The vtkQtChartMouseZoom class zooms the contents in response to
///   mouse events.
class VTKQTCHART_EXPORT vtkQtChartMouseZoom : public vtkQtChartMouseFunction
{
public:
  enum ZoomFlags
    {
    ZoomBoth,  ///< Zoom in both directions.
    ZoomXOnly, ///< Zoom only in the x-direction.
    ZoomYOnly  ///< Zoom only in the y-direction.
    };

public:
  /// \brief
  ///   Creates a new mouse zoom object.
  /// \param parent The parent object.
  vtkQtChartMouseZoom(QObject *parent=0);
  virtual ~vtkQtChartMouseZoom();

  /// \name vtkQtChartMouseFunction Methods
  //@{
  virtual void setMouseOwner(bool owns);

  virtual bool mousePressEvent(QMouseEvent *e, vtkQtChartArea *chart);
  virtual bool mouseMoveEvent(QMouseEvent *e, vtkQtChartArea *chart);
  virtual bool mouseReleaseEvent(QMouseEvent *e, vtkQtChartArea *chart);
  virtual bool mouseDoubleClickEvent(QMouseEvent *e, vtkQtChartArea *chart);
  virtual bool wheelEvent(QWheelEvent *e, vtkQtChartArea *chart);
  //@}

  /// \brief
  ///   Gets the zoom flags used during interaction.
  /// \return
  ///   The zoom flags used during interaction.
  ZoomFlags getFlags() const {return this->Flags;}

protected:
  /// \brief
  ///   Sets the zoom flags to use during interaction.
  /// \param flags The zoom flags to use.
  void setFlags(ZoomFlags flags) {this->Flags = flags;}

private:
  vtkQtChartMouseZoomInternal *Internal; ///< Stores the last position.
  ZoomFlags Flags;                       ///< Stores the zoom flags.

private:
  vtkQtChartMouseZoom(const vtkQtChartMouseZoom &);
  vtkQtChartMouseZoom &operator=(const vtkQtChartMouseZoom &);
};


/// \class vtkQtChartMouseZoomX
/// \brief
///   The vtkQtChartMouseZoomX class zooms the contents in the x-direction.
class VTKQTCHART_EXPORT vtkQtChartMouseZoomX : public vtkQtChartMouseZoom
{
public:
  /// \brief
  ///   Creates a new mouse zoom-x object.
  /// \param parent The parent object.
  vtkQtChartMouseZoomX(QObject *parent=0);
  virtual ~vtkQtChartMouseZoomX() {}

private:
  vtkQtChartMouseZoomX(const vtkQtChartMouseZoomX &);
  vtkQtChartMouseZoomX &operator=(const vtkQtChartMouseZoomX &);
};


/// \class vtkQtChartMouseZoomY
/// \brief
///   The vtkQtChartMouseZoomY class zooms the contents in the y-direction.
class VTKQTCHART_EXPORT vtkQtChartMouseZoomY : public vtkQtChartMouseZoom
{
public:
  /// \brief
  ///   Creates a new mouse zoom-y object.
  /// \param parent The parent object.
  vtkQtChartMouseZoomY(QObject *parent=0);
  virtual ~vtkQtChartMouseZoomY() {}

private:
  vtkQtChartMouseZoomY(const vtkQtChartMouseZoomY &);
  vtkQtChartMouseZoomY &operator=(const vtkQtChartMouseZoomY &);
};


/// \class vtkQtChartMouseZoomBox
/// \brief
///   The vtkQtChartMouseZoomBox class zooms the contents to a rectangle.
class VTKQTCHART_EXPORT vtkQtChartMouseZoomBox : public vtkQtChartMouseFunction
{
public:
  /// \brief
  ///   Creates a new mouse zoom box object.
  /// \param parent The parent object.
  vtkQtChartMouseZoomBox(QObject *parent=0);
  virtual ~vtkQtChartMouseZoomBox();

  /// \name vtkQtChartMouseFunction Methods
  //@{
  virtual void setMouseOwner(bool owns);

  virtual bool mousePressEvent(QMouseEvent *e, vtkQtChartArea *chart);
  virtual bool mouseMoveEvent(QMouseEvent *e, vtkQtChartArea *chart);
  virtual bool mouseReleaseEvent(QMouseEvent *e, vtkQtChartArea *chart);
  virtual bool mouseDoubleClickEvent(QMouseEvent *e, vtkQtChartArea *chart);
  //@}

private:
  QCursor *ZoomCursor; ///< Stores the zoom cursor.

private:
  vtkQtChartMouseZoomBox(const vtkQtChartMouseZoomBox &);
  vtkQtChartMouseZoomBox &operator=(const vtkQtChartMouseZoomBox &);
};

#endif
