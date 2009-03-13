/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartKeyboardZoom.h

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

/// \file vtkQtChartKeyboardZoom.h
/// \date February 23, 2009

#ifndef _vtkQtChartKeyboardZoom_h
#define _vtkQtChartKeyboardZoom_h


#include "vtkQtChartExport.h"
#include "vtkQtChartKeyboardFunction.h"


/// \class vtkQtChartKeyboardZoom
/// \brief
///   The vtkQtChartKeyboardZoom class zooms the chart contents.
class VTKQTCHART_EXPORT vtkQtChartKeyboardZoom :
    public vtkQtChartKeyboardFunction
{
public:
  enum ZoomFlags
    {
    ZoomBoth,  ///< Zoom in both directions.
    ZoomXOnly, ///< Zoom only in the x-direction.
    ZoomYOnly  ///< Zoom only in the y-direction.
    };

  enum ZoomMethod
    {
    ZoomIn, ///< Zoom in.
    ZoomOut ///< Zoom out.
    };

public:
  /// \brief
  ///   Creates a chart keyboard zoom instance.
  /// \param parent The parent object.
  vtkQtChartKeyboardZoom(QObject *parent=0);
  virtual ~vtkQtChartKeyboardZoom() {}

  /// \brief
  ///   Zooms the chart contents according to the method and flags.
  ///
  /// The default is to zoom in on the chart in both directions.
  virtual void activate();

protected:
  /// \brief
  ///   Sets the zoom flags to use when activated.
  /// \param flags The zoom flags to use.
  void setZoomFlags(ZoomFlags flags) {this->Flags = flags;}

  /// \brief
  ///   Sets the zoom method to use when activated.
  /// \param method The zoom method to use.
  void setZoomMethod(ZoomMethod method) {this->Method = method;}

private:
  ZoomFlags Flags;   ///< Stores the zoom flags.
  ZoomMethod Method; ///< Stores the zoom method.

private:
  vtkQtChartKeyboardZoom(const vtkQtChartKeyboardZoom &);
  vtkQtChartKeyboardZoom &operator=(const vtkQtChartKeyboardZoom &);
};


/// \class vtkQtChartKeyboardZoomX
/// \brief
///   The vtkQtChartKeyboardZoomX class zooms in the chart contents in
///   the x-direction.
class VTKQTCHART_EXPORT vtkQtChartKeyboardZoomX :
    public vtkQtChartKeyboardZoom
{
public:
  /// \brief
  ///   Creates a chart keyboard zoom in x instance.
  /// \param parent The parent object.
  vtkQtChartKeyboardZoomX(QObject *parent=0);
  virtual ~vtkQtChartKeyboardZoomX() {}

private:
  vtkQtChartKeyboardZoomX(const vtkQtChartKeyboardZoomX &);
  vtkQtChartKeyboardZoomX &operator=(const vtkQtChartKeyboardZoomX &);
};


/// \class vtkQtChartKeyboardZoomY
/// \brief
///   The vtkQtChartKeyboardZoomY class zooms in the chart contents in
///   the y-direction.
class VTKQTCHART_EXPORT vtkQtChartKeyboardZoomY :
    public vtkQtChartKeyboardZoom
{
public:
  /// \brief
  ///   Creates a chart keyboard zoom in y instance.
  /// \param parent The parent object.
  vtkQtChartKeyboardZoomY(QObject *parent=0);
  virtual ~vtkQtChartKeyboardZoomY() {}

private:
  vtkQtChartKeyboardZoomY(const vtkQtChartKeyboardZoomY &);
  vtkQtChartKeyboardZoomY &operator=(const vtkQtChartKeyboardZoomY &);
};


/// \class vtkQtChartKeyboardZoomOut
/// \brief
///   The vtkQtChartKeyboardZoomOut class zooms out the chart contents
///   in both directions.
class VTKQTCHART_EXPORT vtkQtChartKeyboardZoomOut :
    public vtkQtChartKeyboardZoom
{
public:
  /// \brief
  ///   Creates a chart keyboard zoom out instance.
  /// \param parent The parent object.
  vtkQtChartKeyboardZoomOut(QObject *parent=0);
  virtual ~vtkQtChartKeyboardZoomOut() {}

private:
  vtkQtChartKeyboardZoomOut(const vtkQtChartKeyboardZoomOut &);
  vtkQtChartKeyboardZoomOut &operator=(const vtkQtChartKeyboardZoomOut &);
};


/// \class vtkQtChartKeyboardZoomOutX
/// \brief
///   The vtkQtChartKeyboardZoomOutX class zooms out the chart contents
///   in the x-direction.
class VTKQTCHART_EXPORT vtkQtChartKeyboardZoomOutX :
    public vtkQtChartKeyboardZoomOut
{
public:
  /// \brief
  ///   Creates a chart keyboard zoom out x instance.
  /// \param parent The parent object.
  vtkQtChartKeyboardZoomOutX(QObject *parent=0);
  virtual ~vtkQtChartKeyboardZoomOutX() {}

private:
  vtkQtChartKeyboardZoomOutX(const vtkQtChartKeyboardZoomOutX &);
  vtkQtChartKeyboardZoomOutX &operator=(const vtkQtChartKeyboardZoomOutX &);
};


/// \class vtkQtChartKeyboardZoomOutY
/// \brief
///   The vtkQtChartKeyboardZoomOutY class zooms out the chart contents
///   in the y-direction.
class VTKQTCHART_EXPORT vtkQtChartKeyboardZoomOutY :
    public vtkQtChartKeyboardZoomOut
{
public:
  /// \brief
  ///   Creates a chart keyboard zoom out y instance.
  /// \param parent The parent object.
  vtkQtChartKeyboardZoomOutY(QObject *parent=0);
  virtual ~vtkQtChartKeyboardZoomOutY() {}

private:
  vtkQtChartKeyboardZoomOutY(const vtkQtChartKeyboardZoomOutY &);
  vtkQtChartKeyboardZoomOutY &operator=(const vtkQtChartKeyboardZoomOutY &);
};

#endif
