/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartBasicSeriesOptionsModel.h

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
#ifndef __vtkQtChartBasicSeriesOptionsModel_h
#define __vtkQtChartBasicSeriesOptionsModel_h

#include "vtkQtChartSeriesOptionsModel.h"

/// \class vtkQtChartBasicSeriesOptionsModel
/// \brief
/// vtkQtChartBasicSeriesOptionsModel is a concrete subclass of
/// vtkQtChartSeriesOptionsModel that keeps itself in-sync with the
/// vtkQtChartSeriesModel so that as new series are added/removed from the
/// vtkQtChartSeriesModel, corresponding options are added/removed as well.
class VTKQTCHART_EXPORT vtkQtChartBasicSeriesOptionsModel :
  public vtkQtChartSeriesOptionsModel
{
  Q_OBJECT
public:
  typedef vtkQtChartSeriesOptionsModel Superclass;

  /// \brief
  ///   Creates a chart series options model.
  /// \param parent The parent object.
  /// \param model The vtkQtChartSeriesModel for which this model is to provide
  ///              the options.
  vtkQtChartBasicSeriesOptionsModel(vtkQtChartSeriesModel* model,
    QObject* parent=0);
  virtual ~vtkQtChartBasicSeriesOptionsModel();

  /// \brief
  ///   Gets the series model so that options can be automatically updated as
  ///   series are added/removed.
  vtkQtChartSeriesModel* getSeriesModel() const;

  /// \brief
  ///   Gets the number of options.
  virtual int getNumberOfOptions() const;

  /// \brief
  ///   Gets the options for a particular series.
  /// \param series The series index
  /// \return
  ///   The options for the series at the given index.
  virtual vtkQtChartSeriesOptions* getOptions(int series) const;

  /// \brief
  ///   Gets the index for the given series options.
  /// \param options The series options object.
  /// \return
  ///   The index for the given series options.
  virtual int getOptionsIndex(vtkQtChartSeriesOptions *options) const;

public slots:
  /// \brief resets the model.
  virtual void reset();

protected slots:
  virtual void insertSeriesOptions(int first, int last);
  virtual void removeSeriesOptions(int first, int last);

protected:
  /// Stores the series options.
  QList<vtkQtChartSeriesOptions*> Options; 
  vtkQtChartSeriesModel* Model;

private:
  vtkQtChartBasicSeriesOptionsModel(const vtkQtChartBasicSeriesOptionsModel&); // Not implemented.
  void operator=(const vtkQtChartBasicSeriesOptionsModel&); // Not implemented.
};

#endif


