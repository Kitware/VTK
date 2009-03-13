/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartKeyboardFunction.h

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

/// \file vtkQtChartKeyboardFunction.h
/// \date February 20, 2009

#ifndef _vtkQtChartKeyboardFunction_h
#define _vtkQtChartKeyboardFunction_h


#include "vtkQtChartExport.h"
#include <QObject>

class vtkQtChartArea;


/// \class vtkQtChartKeyboardFunction
/// \brief
///   The vtkQtChartKeyboardFunction class is the base class for all
///   chart keyboard functions.
class VTKQTCHART_EXPORT vtkQtChartKeyboardFunction : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a chart keyboard function instance.
  /// \param parent The parent object.
  vtkQtChartKeyboardFunction(QObject *parent=0);
  virtual ~vtkQtChartKeyboardFunction() {}

  /// \brief
  ///   Gets the chart area for the keyboard function.
  /// \return
  ///   A pointer to the chart area.
  vtkQtChartArea *getChartArea() const {return this->Chart;}

  /// \brief
  ///   Sets the chart area for the keyboard function.
  /// \param chart The new chart area.
  void setChartArea(vtkQtChartArea *chart) {this->Chart = chart;}

public slots:
  /// \brief
  ///   Called to activate the function.
  ///
  /// This method is called when the key is pressed. Sub-classes should
  /// override this method to provide the functionality.
  virtual void activate();

protected:
  vtkQtChartArea *Chart; ///< Stores the chart area.

private:
  vtkQtChartKeyboardFunction(const vtkQtChartKeyboardFunction &);
  vtkQtChartKeyboardFunction &operator=(const vtkQtChartKeyboardFunction &);
};

#endif
