/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartContentsSpace.h

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

/// \file vtkQtChartContentsSpace.h
/// \date 2/7/2008

#ifndef _vtkQtChartContentsSpace_h
#define _vtkQtChartContentsSpace_h


#include "vtkQtChartExport.h"
#include <QObject>

class vtkQtChartContentsSpaceInternal;
class QPoint;
class QPointF;
class QRectF;


/// \class vtkQtChartContentsSpace
/// \brief
///   The vtkQtChartContentsSpace class defines the contents space
///   for a chart.
class VTKQTCHART_EXPORT vtkQtChartContentsSpace : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a chart contents space instance.
  /// \param parent The parent object.
  vtkQtChartContentsSpace(QObject *parent=0);
  virtual ~vtkQtChartContentsSpace();

  /// \name Contents Methods
  //@{
  /// \brief
  ///   Gets the x offset.
  /// \return
  ///   The x offset.
  float getXOffset() const {return this->OffsetX;}

  /// \brief
  ///   Gets the y offset.
  /// \return
  ///   The y offset.
  float getYOffset() const {return this->OffsetY;}

  /// \brief
  ///   Gets the maximum x offset.
  /// \return
  ///   The maximum x offset.
  float getMaximumXOffset() const {return this->MaximumX;}

  /// \brief
  ///   Gets the maximum y offset.
  /// \return
  ///   The maximum y offset.
  float getMaximumYOffset() const {return this->MaximumY;}

  /// \brief
  ///   Gets the contents width.
  /// \return
  ///   The contents width.
  float getContentsWidth() const;

  /// \brief
  ///   Gets the contents height.
  /// \return
  ///   The contents height.
  float getContentsHeight() const;

  /// \brief
  ///   Translates a point to layer contents coordinates.
  ///
  /// The point is adjusted from widget origin to layer bounds origin.
  /// The point is also translated to account for panning offset.
  ///
  /// \param point The point to translate.
  void translateToLayerContents(QPointF &point) const;

  /// \brief
  ///   Translates a point to layer contents coordinates.
  ///
  /// The rectangle is adjusted from widget origin to layer bounds
  /// origin. The rectangle is also translated to account for panning
  /// offset.
  ///
  /// \param area The rectangle to translate.
  void translateToLayerContents(QRectF &area) const;
  //@}

  /// \name Size Methods
  //@{
  /// \brief
  ///   Gets the width of the chart.
  /// \return
  ///   The width of the chart.
  float getChartWidth() const {return this->Width;}

  /// \brief
  ///   Gets the height of the chart.
  /// \return
  ///   The height of the chart.
  float getChartHeight() const {return this->Height;}

  /// \brief
  ///   Sets the size of the chart.
  ///
  /// The chart size must be set in order to zoom in or out. The
  /// contents size methods are only valid when the chart size is set.
  ///
  /// \param width The chart width.
  /// \param height The chart height.
  void setChartSize(float width, float height);

  /// \brief
  ///   Gets the chart layer bounds.
  /// \param bounds Used to return the chart layer bounds.
  void getChartLayerBounds(QRectF &bounds) const;

  /// \brief
  ///   Sets the chart layer bounds.
  /// \param bounds The chart layer bounds.
  void setChartLayerBounds(const QRectF &bounds);
  //@}

  /// \name Zoom Methods
  //@{
  /// \brief
  ///   Gets the x-axis zoom factor.
  /// \return
  ///   The x-axis zoom factor.
  float getXZoomFactor() const {return this->ZoomFactorX;}

  /// \brief
  ///   Gets the y-axis zoom factor.
  /// \return
  ///   The y-axis zoom factor.
  float getYZoomFactor() const {return this->ZoomFactorY;}

  /// \brief
  ///   Zooms the chart to the given factor.
  /// \param factor The new zoom factor for both axes.
  /// \sa vtkQtChartContentsSpace::zoomToFactor(float, float)
  void zoomToFactor(float factor);

  /// \brief
  ///   Zooms the chart to the given factors.
  ///
  /// The zoom factors of the chart are independent of each other. In
  /// other words, the x-axis can be zoomed to a different factor
  /// than the y-axis.
  ///
  /// When the zoom factors are changed, the new zoom viewport will
  /// be added to the zoom history. The zoom history can be navigated
  /// using the \c historyNext and \c historyPrevious methods. The
  /// user can also navigate through the history using the keyboard
  /// shortcuts.
  ///
  /// \param xFactor The x-axis zoom factor.
  /// \param yFactor The y-axis zoom factor.
  void zoomToFactor(float xFactor, float yFactor);

  /// \brief
  ///   Zooms only the x-axis to a given factor.
  /// \param factor The x-axis zoom factor.
  /// \sa vtkQtChartContentsSpace::zoomToFactor(float, float)
  void zoomToFactorX(float factor);

  /// \brief
  ///   Zooms only the y-axis to a given factor.
  /// \param factor The y-axis zoom factor.
  /// \sa vtkQtChartContentsSpace::zoomToFactor(float, float)
  void zoomToFactorY(float factor);
  //@}

  /// \name Mouse Interactions
  //@{
  /// \brief
  ///   Signals the start of a mouse move interaction.
  ///
  /// While an interaction is in progress, the zoom history will not
  /// be updated. When \c finishInteraction is called, the history is
  /// updated if the zoom factors have changed.
  ///
  /// \sa vtkQtChartContentsSpace::finishInteraction()
  void startInteraction();

  /// \brief
  ///   Gets whether or not an interaction is currently in progress.
  /// \return
  ///   True if an interaction is currently in progress.
  /// \sa vtkQtChartContentsSpace::startInteraction(),
  ///     vtkQtChartContentsSpace::finishInteraction()
  bool isInInteraction() const;

  /// \brief
  ///   Signals the end of a mouse move interaction.
  /// \sa vtkQtChartContentsSpace::startInteraction()
  void finishInteraction();
  //@}

  /// \name History Methods
  //@{
  /// \brief
  ///   Gets whether or not a previous zoom viewport is available.
  /// \return
  ///   True if a previous zoom viewport is available.
  bool isHistoryPreviousAvailable() const;

  /// \brief
  ///   Gets whether or not a forward zoom viewport is available.
  /// \return
  ///   True if a forward zoom viewport is available.
  bool isHistoryNextAvailable() const;
  //@}

public slots:
  /// \brief
  ///   Sets the x offset.
  /// \param offset The new x offset.
  void setXOffset(float offset);

  /// \brief
  ///   Sets the y offset.
  /// \param offset The new y offset.
  void setYOffset(float offset);

  /// \brief
  ///   Sets the maximum x offset.
  /// \param maximum The maximum x offset.
  void setMaximumXOffset(float maximum);

  /// \brief
  ///   Sets the maximum y offset.
  /// \param maximum The maximum y offset.
  void setMaximumYOffset(float maximum);

  /// Pans up by a predetermined amount.
  void panUp();

  /// Pans down by a predetermined amount.
  void panDown();

  /// Pans left by a predetermined amount.
  void panLeft();

  /// Pans right by a predetermined amount.
  void panRight();

  /// Resets the zoom factors to 1.
  void resetZoom();

  /// Changes the view to the next one in the history.
  void historyNext();

  /// Changes the view to the previous one in the history.
  void historyPrevious();

public:
  /// \brief
  ///   Gets the zoom factor step.
  /// \return
  ///   The zoom factor step.
  static float getZoomFactorStep();

  /// \brief
  ///   Sets the zoom factor step.
  /// \param step The new zoom factor step.
  static void setZoomFactorStep(float step);

  /// \brief
  ///   Gets the pan step.
  /// \return
  ///   The pan step.
  /// \sa vtkQtChartContentsSpace::panUp(),
  ///     vtkQtChartContentsSpace::panDown(),
  ///     vtkQtChartContentsSpace::panLeft(),
  ///     vtkQtChartContentsSpace::panRight(),
  static float getPanStep();

  /// \brief
  ///   Sets the pan step.
  /// \param step The new pan step.
  static void setPanStep(float step);

signals:
  /// \brief
  ///   Emitted when the x offset has changed.
  /// \param offset The new x offset.
  void xOffsetChanged(float offset);

  /// \brief
  ///   Emitted when the y offset has changed.
  /// \param offset The new y offset.
  void yOffsetChanged(float offset);

  /// \brief
  ///   Emitted when the maximum offsets have changed.
  ///
  /// This signal is sent when either or both of the offsets have
  /// changed. Sending the changes as one improves the chart layout.
  ///
  /// \param xMaximum The maximum x offset.
  /// \param yMaximum The maximum y offset.
  void maximumChanged(float xMaximum, float yMaximum);

  /// \brief
  ///   Emitted when the view history availability changes.
  /// \param available True if there is a history item available
  ///   before the current one.
  void historyPreviousAvailabilityChanged(bool available);

  /// \brief
  ///   Emitted when the view history availability changes.
  /// \param available True if there is a history item available
  ///   after the current one.
  void historyNextAvailabilityChanged(bool available);

private:
  /// Adds the current zoom viewport to the history.
  void addHistory();

private:
  /// Keeps track of mouse position and history.
  vtkQtChartContentsSpaceInternal *Internal;
  float OffsetX;     ///< Stores the x offset.
  float OffsetY;     ///< Stores the y offset.
  float MaximumX;    ///< Stores the maximum x offset.
  float MaximumY;    ///< Stores the maximum y offset.
  float Width;       ///< Stores the chart width.
  float Height;      ///< Stores the chart height.
  float ZoomFactorX; ///< Stores the x-axis zoom factor.
  float ZoomFactorY; ///< Stores the y-axis zoom factor.

  static float ZoomFactorStep; ///< Stores the zoom factor step.
  static float PanStep;        ///< Stores the pan step.
};

#endif
