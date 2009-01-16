/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtLineChartView.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
// .NAME vtkQtLineChartView - Wraps a vtkQtChartArea into a VTK view.
//
// .SECTION Description
// vtkQtLineChartView is a type vtkQtChartView designed for line charts.
//
// .SECTION See Also
// vtkQtChartView

#ifndef __vtkQtLineChartView_h
#define __vtkQtLineChartView_h

#include "vtkQtChartViewBase.h"

class vtkQtChartArea;

class QVTK_EXPORT vtkQtLineChartView : public vtkQtChartViewBase
{
public:
  static vtkQtLineChartView *New();
  vtkTypeRevisionMacro(vtkQtLineChartView, vtkQtChartViewBase);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Updates the view.
  virtual void Update();
  
protected:
  vtkQtLineChartView();
  ~vtkQtLineChartView();
  
private:
  vtkQtLineChartView(const vtkQtLineChartView&);  // Not implemented.
  void operator=(const vtkQtLineChartView&);  // Not implemented.
};

#endif
