/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartSeriesOptionsModel.h

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

#ifndef __vtkQtChartSeriesOptionsModel_h
#define __vtkQtChartSeriesOptionsModel_h

#include "vtkQtChartExport.h"
#include <QObject>

class vtkQtChartSeriesLayer;
class vtkQtChartSeriesModel;
class vtkQtChartSeriesOptions;

/// \class vtkQtChartSeriesOptionsModel
/// \brief
/// vtkQtChartSeriesOptionsModel is the base class for all chart series options
/// models. This can be considered analogous to vtkQtChartSeriesModel except
/// that instead of providing details about the series, it provides the options
/// for the series.
class VTKQTCHART_EXPORT vtkQtChartSeriesOptionsModel : public QObject
{
  Q_OBJECT

public:
  typedef QObject Superclass;

  /// \brief
  ///   Creates a chart series options model.
  /// \param param The parent object.
  vtkQtChartSeriesOptionsModel(QObject* parent=0);
  virtual ~vtkQtChartSeriesOptionsModel() {}

  /// \brief
  ///   Get/Set the vtkQtChartSeriesLayer. The layer is needed to create new
  ///   options of the right type.
  /// FIXME: We need to get rid of this dependency on the layer. Right now, I
  /// don't see how that can be done esp. since the type of options depends on
  /// the type of layer.
  virtual void setChartSeriesLayer(vtkQtChartSeriesLayer*);
  virtual vtkQtChartSeriesLayer* getChartSeriesLayer() const;

  /// \brief
  ///   Gets the number of options.
  virtual int getNumberOfOptions() const = 0;

  /// \brief
  ///   Gets the options for a particular series.
  /// \param series The series index
  /// \return
  ///   The options for the series at the given index.
  virtual vtkQtChartSeriesOptions* getOptions(int series) const = 0;

  /// \brief
  ///   Gets the index for the given series options.
  /// \param options The series options object.
  /// \return
  ///   The index for the given series options.
  virtual int getOptionsIndex(vtkQtChartSeriesOptions *options) const = 0;

public slots:
  /// \brief
  ///   Resets the model.
  virtual void reset()=0;

signals:
  /// Emitted when the model is about to be reset.
  void modelAboutToBeReset();

  /// Emitted when the model has been reset.
  void modelReset();

  /// \brief
  ///   Emitted when options will be inserted into the model.
  /// \param first The first index to be added.
  /// \param last The last index to be added.
  void optionsAboutToBeInserted(int first, int last);

  /// \brief
  ///   Emitted when options have been inserted into the model.
  /// \param first The first index that was inserted.
  /// \param last The last index that was inserted.
  void optionsInserted(int first, int last);

  /// \brief
  ///   Emitted when options will be removed from the model.
  /// \param first The first index to be removed.
  /// \param last The last index to be removed.
  void optionsAboutToBeRemoved(int first, int last);

  /// \brief
  ///   Emitted when options have been removed from the model.
  /// \param first The first index that was removed.
  /// \param last The last index that was removed.
  void optionsRemoved(int first, int last);

protected:
  /// \brief Creates a new options object.
  /// \param parent The parent QObject for the options.
  /// \return
  ///   The new instance of vtkQtChartSeriesOptions subclass suitable for the
  ///   layer set using setChartSeriesLayer().
  vtkQtChartSeriesOptions* newOptions(QObject* parent);

  /// \brief Releases the options. This will delete the options instance.
  void releaseOptions(vtkQtChartSeriesOptions* options);

protected:
  vtkQtChartSeriesLayer* Layer;
};

#endif


