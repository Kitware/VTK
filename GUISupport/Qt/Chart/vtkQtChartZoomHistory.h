/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartZoomHistory.h

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

/// \file vtkQtChartZoomHistory.h
/// \date 2/7/2008

#ifndef _vtkQtChartZoomHistory_h
#define _vtkQtChartZoomHistory_h


#include "vtkQtChartExport.h"

class vtkQtChartZoomHistoryInternal;


/// \class vtkQtChartZoomViewport
/// \brief
///   The vtkQtChartZoomViewport class stores the position and zoom
///   factors for a viewport.
///
/// The position stores the top-left corner of the viewport in
/// content coordinates. The zoom factors are stored as percentages.
class VTKQTCHART_EXPORT vtkQtChartZoomViewport
{
public:
  vtkQtChartZoomViewport();
  ~vtkQtChartZoomViewport() {}

  /// \brief
  ///   Sets the viewport position coordinates.
  /// \param x The x coordinate.
  /// \param y The y coordinate.
  /// \sa vtkQtChartZoomViewport::getXPosition(),
  ///     vtkQtChartZoomViewport::getYPosition()
  void setPosition(float x, float y);

  /// \brief
  ///   Sets the zoom percentages.
  /// \param x The x-axis zoom factor.
  /// \param y The y-axis zoom factor.
  /// \sa vtkQtChartZoomViewport::getXZoom(),
  ///     vtkQtChartZoomViewport::getYZoom()
  void setZoom(float x, float y);

  /// \brief
  ///   Gets the x coordinate of the viewport.
  /// \return
  ///   The x coordinate of the viewport.
  /// \sa vtkQtChartZoomViewport::setPosition(float, float)
  float getXPosition() const {return this->X;}

  /// \brief
  ///   Gets the y coordinate of the viewport.
  /// \return
  ///   The y coordinate of the viewport.
  /// \sa vtkQtChartZoomViewport::setPosition(float, float)
  float getYPosition() const {return this->Y;}

  /// \brief
  ///   Gets the x-axis zoom factor.
  /// \return
  ///   The x-axis zoom factor.
  /// \sa vtkQtChartZoomViewport::setZoom(float, float)
  float getXZoom() const {return this->XFactor;}

  /// \brief
  ///   Gets the y-axis zoom factor.
  /// \return
  ///   The y-axis zoom factor.
  /// \sa vtkQtChartZoomViewport::setZoom(float, float)
  float getYZoom() const {return this->YFactor;}

private:
  float X;        ///< Stores the x position coordinate.
  float Y;        ///< Stores the y position coordinate.
  float XFactor;  ///< Stores the x-axis zoom factor.
  float YFactor;  ///< Stores the y-axis zoom factor.
};


/// \class vtkQtChartZoomHistory
/// \brief
///   The vtkQtChartZoomHistory class stores a list of
///   vtkQtChartZoomViewport objects.
///
/// The zoom history contains a list of zoom viewports. The list is
/// ordered chronologically, and contains an index to the current item.
/// The history list is limited to a certain number of items. The
/// default limit is 10, but it can be changed using the \c setLimit
/// method.
///
/// When adding items to the history list, the new item will become
/// the current item. The front of the list may be trimmed to stay
/// within limits. If the current item is in the middle of the list,
/// the subsequent items will be removed before adding the new item
/// to the end of the list.
///
/// The history list is navigated using the \c getPrevious and
/// \c getNext methods. You can also use the \c getCurrent method to
/// get the current item without changing the index.
///
/// \sa vtkQtChartZoomHistory::setLimit(int),
///     vtkQtChartZoomHistory::addHistory(float, float, float, float),
///     vtkQtChartZoomHistory::getNext(),
///     vtkQtChartZoomHistory::getPrevious(),
///     vtkQtChartZoomHistory::getCurrent()
class VTKQTCHART_EXPORT vtkQtChartZoomHistory
{
public:
  vtkQtChartZoomHistory();
  ~vtkQtChartZoomHistory();

  /// \brief
  ///   Sets the maximum number of items in the history.
  /// \param limit The maximum number of entries.
  void setLimit(int limit);

  /// \brief
  ///   Gets the maximum number of items in the history.
  /// \return
  ///   The maximum number of entries.
  int getLimit() const {return this->Allowed;}

  /// \brief
  ///   Adds a zoom viewport to the history list.
  ///
  /// The new item will become the current item in the list. If the
  /// current item is not at the end of the list, all the subsequent
  /// items will be removed. If the list is longer than the allowed
  /// limit, items will be removed from the front of the list.
  ///
  /// \param x The x position of the viewport.
  /// \param y The y position of the viewport.
  /// \param xZoom The x-axis zoom factor for the viewport.
  /// \param yZoom The y-axis zoom factor for the viewport.
  /// \sa vtkQtChartZoomHistory::updatePosition(float, float)
  void addHistory(float x, float y, float xZoom, float yZoom);

  /// \brief
  ///   Used to update the viewport position for the current
  ///   zoom factors.
  ///
  /// This method allows the current zoom viewport to be updated when
  /// the user changes the viewport position by panning or scrolling.
  ///
  /// \param x The x position of the viewport.
  /// \param y The y position of the viewport.
  /// \sa vtkQtChartZoomHistory::addHistory(float, float, float, float)
  void updatePosition(float x, float y);

  /// \brief
  ///   Gets whether or not a zoom viewport is before the current.
  /// \return
  ///   True if a zoom viewport is before the current.
  bool isPreviousAvailable() const;

  /// \brief
  ///   Gets whether or not a zoom viewport is after the current.
  /// \return
  ///   True if a zoom viewport is after the current.
  bool isNextAvailable() const;

  /// \brief
  ///   Gets the current zoom viewport.
  /// \return
  ///   A pointer to the current zoom viewport or null if the list
  ///   is empty.
  const vtkQtChartZoomViewport *getCurrent() const;

  /// \brief
  ///   Gets the previous zoom viewport in the history.
  /// \return
  ///   A pointer to the previous zoom viewport or null if the
  ///   beginning of the list is reached.
  /// \sa vtkQtChartZoomHistory::getNext()
  const vtkQtChartZoomViewport *getPrevious();

  /// \brief
  ///   Gets the next zoom viewport in the history.
  /// \return
  ///   A pointer to the next zoom viewport or null if the end
  ///   of the list is reached.
  /// \sa vtkQtChartZoomHistory::getPrevious()
  const vtkQtChartZoomViewport *getNext();

private:
  /// Stores the zoom viewport list.
  vtkQtChartZoomHistoryInternal *Internal;

  int Current; ///< Stores the current item index.
  int Allowed; ///< Stores the list length limit.
};

#endif
