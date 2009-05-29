/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartArea.h

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

/// \file vtkQtChartArea.h
/// \date February 1, 2008

#ifndef _vtkQtChartArea_h
#define _vtkQtChartArea_h

#include "vtkQtChartExport.h"
#include <QGraphicsView>

class vtkQtChartAreaInternal;
class vtkQtChartAxisLayer;
class vtkQtChartContentsSpace;
class vtkQtChartInteractor;
class vtkQtChartLayer;
class vtkQtChartMouseBox;
class vtkQtChartStyleManager;
class QCursor;
class QString;


/// \class vtkQtChartArea
/// \brief
///   The vtkQtChartArea class manages the chart axes and layers.
class VTKQTCHART_EXPORT vtkQtChartArea : public QGraphicsView
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a chart area instance.
  /// \param parent The parent widget.
  vtkQtChartArea(QWidget *parent=0);
  virtual ~vtkQtChartArea();

  /// \name Layer Methods
  //@{
  /// \brief
  ///   Adds a chart layer to the top of the layer list.
  /// \param chart The chart to add to the list.
  /// \sa vtkQtChartArea::insertLayer(int, vtkQtChartLayer *)
  void addLayer(vtkQtChartLayer *chart);

  /// \brief
  ///   Inserts a chart layer in the layer list.
  ///
  /// The chart layers are drawn in the order they reside in the list
  /// of layers. A chart at the beginning of the list is drawn
  /// underneath the other layers. A chart at the end of the list is
  /// drawn on top of the other layers.
  ///
  /// The chart area has two built in chart layers: the grid and axis
  /// layers. By default, the grid layer is on the bottom and the axis
  /// layer is on the next layer. The index of these layers can be
  /// used to place other layers in the chart.
  ///
  /// \param index Where to insert the chart.
  /// \param chart The chart to insert in the list.
  /// \sa
  ///   vtkQtChartArea::getGridLayerIndex(),
  ///   vtkQtChartArea::getAxisLayerIndex()
  void insertLayer(int index, vtkQtChartLayer *chart);

  /// \brief
  ///   Removes a chart layer from the layer list.
  /// \param chart The chart to remove from the list.
  void removeLayer(vtkQtChartLayer *chart);

  /// \brief
  ///   Gets the number of chart layers in the list.
  /// \return
  ///   The number of chart layers in the list.
  int getNumberOfLayers() const;

  /// \brief
  ///   Gets the chart layer at the specified index.
  /// \param index The index of the layer.
  /// \return
  ///   A pointer to the chart layer at the specified index.
  vtkQtChartLayer *getLayer(int index) const;

  /// \brief
  ///   Gets the chart layer with the specified chart type.
  /// \param chartName The chart type name to find.
  /// \return
  ///   A pointer to the chart layer with the specified chart type.
  vtkQtChartLayer *getLayer(const QString &chartName) const;

  /// \brief
  ///   Gets the chart axis layer.
  /// \return
  ///   A pointer to the chart axis layer.
  vtkQtChartAxisLayer *getAxisLayer() const;

  /// \brief
  ///   Gets the index for the axis layer.
  /// \return
  ///   The index for the axis layer.
  int getAxisLayerIndex() const;

  /// \brief
  ///   Gets the index for the grid layer.
  /// \return
  ///   The index for the grid layer.
  int getGridLayerIndex() const;
  //@}

  /// \name Interaction Methods
  //@{
  /// \brief
  ///   Gets the current chart interactor.
  /// \return
  ///   A pointer to the current chart interactor.
  vtkQtChartInteractor *getInteractor() const;

  /// \brief
  ///   Sets the chart interactor.
  ///
  /// This method sets up the interactor to work with the chart. The
  /// contents space and mouse box are set on the interactor.
  ///
  /// \param interactor The new chart interactor.
  void setInteractor(vtkQtChartInteractor *interactor);

  /// \brief
  ///   Gets the contents space object.
  /// \return
  ///   A pointer to the contents space object.
  vtkQtChartContentsSpace *getContentsSpace() const;

  /// \brief
  ///   Gets the mouse box object.
  /// \return
  ///   A pointer to the mouse box object.
  vtkQtChartMouseBox *getMouseBox() const;

  /// \brief
  ///   Notifies the chart layers that a resize interaction has started.
  ///
  /// Chart layers can use this method to reduce the processing load
  /// during an interaction.
  void startInteractiveResize();

  /// \brief
  ///   Gets whether or no the chart is interactively resizing.
  /// \return
  ///   True if the chart is interactively resizing.
  bool isInteractivelyResizing() const;

  /// Notifies the chart layers that a resize interaction has finished.
  void finishInteractiveResize();
  //@}

  /// \name Style Methods
  //@{
  /// \brief
  ///   Gets the style manager for layers that want unique styles.
  /// \return
  ///   A pointer to the style manager.
  vtkQtChartStyleManager *getStyleManager() const;

  /// \brief
  ///   Sets the style manager for layers that want unique styles.
  /// \param manager The new style manager.
  void setStyleManager(vtkQtChartStyleManager *manager);
  //@}

  /// \brief 
  ///   Enabled OpenGL is available.
  void setUseOpenGLIfAvailable(bool useOpenGL);

public slots:
  /// Calculates the axis and chart layout.
  void layoutChart();

  /// Merges layout requests into one delayed layout event.
  void updateLayout();

signals:
  /// Emitted when a delayed chart layout is needed.
  void delayedLayoutNeeded();

  /// \brief
  ///   Emitted when a chart layer has been inserted.
  /// \param index The index of the layer.
  /// \param chart The chart layer that was inserted.
  void layerInserted(int index, vtkQtChartLayer *chart);

  /// \brief
  ///   Emitted before a chart layer is removed.
  /// \param index The index of the layer.
  /// \param chart The chart layer that will be removed.
  void removingLayer(int index, vtkQtChartLayer *chart);

  /// \brief
  ///   Emitted when a chart layer has been removed.
  /// \param index The index of the layer.
  /// \param chart The chart layer that was removed.
  void layerRemoved(int index, vtkQtChartLayer *chart);

protected:
  /// \brief
  ///   Updates the layout when the font changes.
  /// \param e Event specific information.
  /// \return
  ///   True if the event was handled.
  virtual bool viewportEvent(QEvent *e);

  /// \brief
  ///   Updates the layout when the size changes.
  /// \param e Event specific information.
  virtual void resizeEvent(QResizeEvent *e);

  /// \name Interaction Methods
  //@{
  /// \brief
  ///   Handles the key press events for the chart.
  ///
  /// All the interaction events are forwarded to the vtkQtChartInteractor.
  /// It is up to the interactor object to accept or ignore the events.
  ///
  /// \param e Event specific information.
  virtual void keyPressEvent(QKeyEvent *e);

  /// \brief
  ///   Handles the mouse press events for the chart.
  /// \param e Event specific information.
  virtual void mousePressEvent(QMouseEvent *e);

  /// \brief
  ///   Handles the mouse move events for the chart.
  /// \param e Event specific information.
  virtual void mouseMoveEvent(QMouseEvent *e);

  /// \brief
  ///   Handles the mouse release events for the chart.
  /// \param e Event specific information.
  virtual void mouseReleaseEvent(QMouseEvent *e);

  /// \brief
  ///   Handles the mouse double click events for the chart.
  /// \param e Event specific information.
  virtual void mouseDoubleClickEvent(QMouseEvent *e);

  /// \brief
  ///   Handles the mouse wheel events for the chart.
  /// \param e Event specific information.
  virtual void wheelEvent(QWheelEvent *e);
  //@}

private slots:
  /// Updates the layout after a zoom change.
  void handleZoomChange();

  /// \brief
  ///   Changes the view cursor to the requested one.
  /// \param newCursor The new view cursor.
  void changeCursor(const QCursor &newCursor);

private:
  vtkQtChartAreaInternal *Internal; ///< Stores the list of chart layers.

private:
  vtkQtChartArea(const vtkQtChartArea &);
  vtkQtChartArea &operator=(const vtkQtChartArea &);
};

#endif
