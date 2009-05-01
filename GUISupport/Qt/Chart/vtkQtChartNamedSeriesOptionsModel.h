/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartNamedSeriesOptionsModel.h

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

#ifndef __vtkQtChartNamedSeriesOptionsModel_h
#define __vtkQtChartNamedSeriesOptionsModel_h

#include "vtkQtChartSeriesOptionsModel.h"
#include <QMap>

class VTKQTCHART_EXPORT vtkQtChartNamedSeriesOptionsModel :
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
  vtkQtChartNamedSeriesOptionsModel(vtkQtChartSeriesModel* model,
    QObject* parent=0);
  virtual ~vtkQtChartNamedSeriesOptionsModel();

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

  // this will create new options if none exist.
  vtkQtChartSeriesOptions* getOptions(const QString& name);

  /// Returns the name for series at the given index.
  QString getSeriesName(int series) const;

  void addOptions(const QString& name, vtkQtChartSeriesOptions* options);
  void removeOptions(const QString& name);
  void removeAllOptions();

protected slots:
  virtual void reset();
  virtual void insertSeriesOptions(int first, int last);

protected:
  QMap<QString, vtkQtChartSeriesOptions*> Options;
  vtkQtChartSeriesModel* Model;

private:
  vtkQtChartNamedSeriesOptionsModel(const vtkQtChartNamedSeriesOptionsModel&); // Not implemented.
  void operator=(const vtkQtChartNamedSeriesOptionsModel&); // Not implemented.
};

#endif


