/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartView.h

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
// .NAME vtkQtChartView - Wraps a vtkQtChartArea into a VTK view.
//
// .SECTION Description
// vtkQtChartView is a vtkView which wraps an instance of vtkQtChartArea.
// This view expects vtkChartRepresentation instances as its representation.
//
// .SECTION See Also
// vtkQtChartRepresentation

#ifndef __vtkQtChartView_h
#define __vtkQtChartView_h

#include "QVTKWin32Header.h"
#include "vtkView.h"

#include <QGraphicsView>
#include <QMouseEvent>
#include <QTimeLine>

class QGraphicsScene;
class QResizeEvent;
class vtkQtChartArea;

class QVTK_EXPORT vtkQtChartView : public vtkView
{
public:
  static vtkQtChartView *New();
  vtkTypeRevisionMacro(vtkQtChartView, vtkView);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set and get the underlying Qt view.
  virtual void SetChartView(vtkQtChartArea*);
  vtkQtChartArea* GetChartView() { return this->ChartView; }
  
  // Description:
  // Updates the view.
  virtual void Update();
  
protected:
  vtkQtChartView();
  ~vtkQtChartView();

  // Description:
  // Pointer to the view.
  vtkQtChartArea* ChartView;
  
  // Description:
  // Do I manage the lifetime of the view.
  bool IOwnChartView;
  
private:
  vtkQtChartView(const vtkQtChartView&);  // Not implemented.
  void operator=(const vtkQtChartView&);  // Not implemented.
};

#endif
