/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartSeriesLayer.h

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

/// \file vtkQtChartSeriesLayer.h
/// \date February 14, 2008

#ifndef _vtkQtChartSeriesLayer_h
#define _vtkQtChartSeriesLayer_h

#include "vtkQtChartExport.h"
#include "vtkQtChartLayer.h"
#include <QPixmap> // needed for return type

class vtkQtChartContentsArea;
class vtkQtChartSeriesModel;
class vtkQtChartSeriesOptions;
class vtkQtChartSeriesOptionsModel;
class vtkQtChartSeriesSelection;
class vtkQtChartSeriesSelectionModel;
class QPointF;
class QRectF;


/// \class vtkQtChartSeriesLayer
/// \brief
///   The vtkQtChartSeriesLayer class is the base class for chart
///   layers that use the chart series model.
///
/// It stores the pointer to the chart series model and the list of
/// options for the series. The series options are created using the
/// \c createOptions method. Subclasses should overload this method to
/// create the appropriate type of options object.
class VTKQTCHART_EXPORT vtkQtChartSeriesLayer : public vtkQtChartLayer
{
  Q_OBJECT

public:
  enum {Type = vtkQtChart_SeriesLayerType};

public:
  vtkQtChartSeriesLayer(bool useContents=true);
  virtual ~vtkQtChartSeriesLayer() {}

  /// \brief
  ///   Sets the chart area for the chart layer.
  ///
  /// If the model is set before the chart layer is added to a chart
  /// area, series options will not be available. Setting the chart
  /// area will create the series options for the model in this case.
  /// Subclasses can extend this method to handle the new options.
  ///
  /// \param area The new chart area.
  virtual void setChartArea(vtkQtChartArea *area);

  /// \brief
  ///   Gets the chart series model.
  /// \return
  ///   A pointer to the chart series model.
  vtkQtChartSeriesModel *getModel() const {return this->Model;}

  /// \brief
  ///   Sets the chart series model.
  /// \param model The new chart series model.
  virtual void setModel(vtkQtChartSeriesModel *model);

  /// \brief
  ///   Gets the chart series options model. 
  /// \return
  ///   A pointer to the current chart series options model.
  vtkQtChartSeriesOptionsModel* getOptionsModel() const
    { return this->Options; }

  /// \brief
  ///   Sets the chart series options model.
  /// \param model The new chart series options model.
  virtual void setOptionsModel(vtkQtChartSeriesOptionsModel* model);

  /// \brief
  ///   Gets the drawing options for the given series.
  /// \param series The index of the series.
  /// \return
  ///   A pointer to the drawing options for the given series.
  vtkQtChartSeriesOptions *getSeriesOptions(int series) const;

  /// \brief
  ///   Gets the index for the given series options.
  /// \param options The series options object.
  /// \return
  ///   The index for the given series options.
  int getSeriesOptionsIndex(vtkQtChartSeriesOptions *options) const;

  /// \brief
  ///   Gets the icon for a given series.
  ///
  /// The icon is used by the chart legend.
  ///
  /// \param series The index of the series.
  /// \return
  ///   A pixmap representation of the series.
  virtual QPixmap getSeriesIcon(int series) const;

  /// \brief
  ///   Gets the chart series selection model.
  /// \return
  ///   A pointer to the chart series selection model.
  vtkQtChartSeriesSelectionModel *getSelectionModel() const;

  /// \brief
  ///   Gets the list of series at a given position.
  /// \param point The position in scene coordinates.
  /// \param selection Used to return the list of series.
  virtual void getSeriesAt(const QPointF &point,
      vtkQtChartSeriesSelection &selection) const;

  /// \brief
  ///   Gets the list of points at a given position.
  /// \param point The position in scene coordinates.
  /// \param selection Used to return the list of points.
  virtual void getPointsAt(const QPointF &point,
      vtkQtChartSeriesSelection &selection) const;

  /// \brief
  ///   Gets the list of series in a given area.
  /// \param area The rectangle in scene coordinates.
  /// \param selection Used to return the list of series.
  virtual void getSeriesIn(const QRectF &area,
      vtkQtChartSeriesSelection &selection) const;

  /// \brief
  ///   Gets the list of points in a given area.
  /// \param area The rectangle in scene coordinates.
  /// \param selection Used to return the list of points.
  virtual void getPointsIn(const QRectF &area,
      vtkQtChartSeriesSelection &selection) const;

  /// \brief
  ///   Creates a new options object (by calling createOptions()) and then
  /// initializes it (by calling setupOptions()).
  /// \param parent The parent QObject.
  vtkQtChartSeriesOptions* newOptions(QObject* parent);

  /// \brief 
  ///   Releases the options. This will delete the options.
  /// \param options The options object to be freed.
  void releaseOptions(vtkQtChartSeriesOptions* options);

public slots:
  /// \brief
  ///   Sets the contents x-axis offset.
  /// \param offset The new x-axis offset.
  void setXOffset(float offset);

  /// \brief
  ///   Sets the contents y-axis offset.
  /// \param offset The new y-axis offset.
  void setYOffset(float offset);

signals:
  /// \brief
  ///   Emitted when the series model is changed.
  /// \param previous The previous series model.
  /// \param current The current series model.
  void modelChanged(vtkQtChartSeriesModel *previous,
      vtkQtChartSeriesModel *current);

  /// \brief
  ///   Emitted when the name or icon changes for a set of series.
  /// \param first The first series index of the range.
  /// \param last The last series index of the range.
  void modelSeriesChanged(int first, int last);

  /// \brief
  ///   Emitted when the visibility for a series has changed.
  /// \param series The index of the series.
  /// \param visible True if the series is visible.
  void modelSeriesVisibilityChanged(int series, bool visible);

protected:
  /// \brief
  ///   Creates an options object for a series.
  /// \note
  ///   Subclasses should only create the options in this method. Use
  ///   the \c setupOptions to set up connections.
  /// \param parent The parent of the options object.
  /// \return
  ///   A pointer to a new options object.
  /// \sa
  ///   vtkQtChaerSeriesLayer::setupOptions(vtkQtChartSeriesOptions *)
  virtual vtkQtChartSeriesOptions *createOptions(QObject *parent) = 0;

  /// \brief
  ///   Sets up the series options object.
  ///
  /// The style manager should be used to help set up the series options.
  /// Signal connections should be set up here in order to avoid slot
  /// calls during setup.
  ///
  /// \param style The series style index.
  /// \param options The newly created series options.
  virtual void setupOptions(int style, vtkQtChartSeriesOptions *options) = 0;

protected:
  /// Stores the series/point selection.
  vtkQtChartSeriesSelectionModel *Selection;
  vtkQtChartSeriesModel *Model;     ///< Stores the series model.
  vtkQtChartContentsArea *Contents; ///< Used for panning.
  vtkQtChartSeriesOptionsModel* Options; ///< Stores the series options.

private:
  vtkQtChartSeriesLayer(const vtkQtChartSeriesLayer &);
  vtkQtChartSeriesLayer &operator=(const vtkQtChartSeriesLayer &);
};

#endif

