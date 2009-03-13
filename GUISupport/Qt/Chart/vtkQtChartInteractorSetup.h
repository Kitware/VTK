/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartInteractorSetup.h

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

/// \file vtkQtChartInteractorSetup.h
/// \date March 11, 2008

#ifndef _vtkQtChartInteractorSetup_h
#define _vtkQtChartInteractorSetup_h


#include "vtkQtChartExport.h"

class vtkQtChartArea;
class vtkQtChartInteractor;
class vtkQtChartMouseSelection;


/// \class vtkQtChartInteractorSetup
/// \brief
///   The vtkQtChartInteractorSetup class is used to set up the chart
///   interactor.
class VTKQTCHART_EXPORT vtkQtChartInteractorSetup
{
public:
  vtkQtChartInteractorSetup() {}
  ~vtkQtChartInteractorSetup() {}

  /// \brief
  ///   Creates the default interactor setup for the given chart.
  ///
  /// Selection is set on the left mouse button. All the zoom
  /// functionality is added to the middle button. The panning
  /// capability is added to the right button. The separate zooming
  /// functions are accessed using keyboard modifiers.
  ///   \li No modifiers: regular drag zoom.
  ///   \li Control: x-only drag zoom.
  ///   \li Alt: y-only drag zoom.
  ///   \li Shift: zoom box.
  ///
  /// The interactor is created as a child of the chart area. The
  /// mouse functions are created as children of the interactor.
  ///
  /// \param area The chart to add the interactor to.
  /// \return
  ///   A pointer to the mouse selection handler.
  static vtkQtChartMouseSelection *createDefault(vtkQtChartArea *area);

  /// \brief
  ///   Creates an interactor with the zoom functionality on separate
  ///   buttons.
  ///
  /// The panning capability is added to the left button along with
  /// selection. The left button interaction mode must be set to
  /// access the different functionality. The zoom box function is
  /// set on the right button. The rest of the zoom capability is
  /// added to the middle button. X-only and y-only zooms are
  /// accessed using the control and alt modifiers respectively. If
  /// no modifiers are pressed, regular drag zoom is activated.
  ///
  /// The interactor is created as a child of the chart area. The
  /// mouse functions are created as children of the interactor.
  ///
  /// \param area The chart to add the interactor to.
  /// \return
  ///   A pointer to the mouse selection handler.
  static vtkQtChartMouseSelection *createSplitZoom(vtkQtChartArea *area);

  /// \brief
  ///   Sets up the default keyboard functions.
  ///
  /// The keyboard shortcuts are as follows:
  /// \code
  /// Plus/Equal.............Zoom in.
  /// Minus..................Zoom out.
  /// Ctrl+Plus..............Horizontally zoom in.
  /// Ctrl+minus.............Horizontally zoom out.
  /// Alt+Plus...............Vertically zoom in.
  /// Alt+minus..............Vertically zoom out.
  /// Up.....................Pan up.
  /// Down...................Pan down.
  /// Left...................Pan left.
  /// Right..................Pan right.
  /// Alt+Left...............Go to previous view in the history.
  /// Alt+Right..............Go to next view in the history.
  /// \endcode
  ///
  /// \param interactor The interactor to set up.
  static void setupDefaultKeys(vtkQtChartInteractor *interactor);
};

#endif
