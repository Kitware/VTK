/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartViewBase.h

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
// .NAME vtkQtChartViewBase - Wraps a vtkQtChartArea into a VTK view.
//
// .SECTION Description
// vtkQtChartViewBase is a vtkView which wraps an instance of vtkQtChartArea.
// This view expects vtkQtChartTableRepresentation instances as its representation.
//
// .SECTION See Also
// vtkQtChartTableRepresentation

#ifndef __vtkQtChartViewBase_h
#define __vtkQtChartViewBase_h

#include "QVTKWin32Header.h"
#include "vtkView.h"

class vtkQtChartWidget;
class vtkQtChartArea;
class vtkQtChartSeriesLayer;
class vtkQtChartSeriesModelCollection;
class vtkQtChartLegendModel;
class vtkQtChartLegend;
class vtkTable;

class QVTK_EXPORT vtkQtChartViewBase : public vtkView
{
public:
  static vtkQtChartViewBase *New();
  vtkTypeRevisionMacro(vtkQtChartViewBase, vtkView);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Updates the view.
  virtual void Update();

  // Description:
  // Updates representations and then repaints the chart widget.
  virtual void Render();

  // Description:
  // Calls show() on the chart widget.
  void Show();

  // Description:
  // A convenience method to add a table to the chart view.
  // This is here for now to work around a python wrapping bug.
  void AddTableToView(vtkTable* table);

  // Description:
  // Set the chart's title
  void SetTitle(const char* title);

  //BTX
  // Description:
  // Gets the chart widget, this is the main widget to display.
  vtkQtChartWidget* GetChartWidget();

  // Description:
  // Gets the chart area from the chart widget.  This method is equivalent
  // to GetChartWidget()->getChartArea().
  vtkQtChartArea* GetChartArea();

  // Description:
  // Gets the chart series layer
  vtkQtChartSeriesLayer* GetChartLayer();

  // Description:
  // Gets the chart series model
  vtkQtChartSeriesModelCollection* GetChartSeriesModel();

  // Description:
  // Gets the chart legend model
  vtkQtChartLegendModel* GetLegendModel();

  // Description:
  // Gets the chart legend widget
  vtkQtChartLegend* GetLegend();
  //ETX
  
protected:

  // Description:
  // Sets the chart series layer.
  void SetChartLayer(vtkQtChartSeriesLayer* chartLayer);

  // Description:
  // Initializes the chart by setting defaults.
  virtual void Initialize();

  // Description:
  // Called from Initialize() to setup the default axes
  virtual void SetupDefaultAxes();

  // Description:
  // Called from Initialize() to setup the default color scheme
  virtual void SetupDefaultColorScheme();

  // Description:
  // Called from Initialize() to setup the default interactor
  virtual void SetupDefaultInteractor();

  // Description:
  // Create a vtkQtChartRepresentation for the given input connection.
  virtual vtkDataRepresentation* CreateDefaultRepresentation(vtkAlgorithmOutput* conn);

  vtkQtChartViewBase();
  ~vtkQtChartViewBase();

  //BTX
  class vtkInternal;
  vtkInternal* Internal;
  //ETX
  
private:
  vtkQtChartViewBase(const vtkQtChartViewBase&);  // Not implemented.
  void operator=(const vtkQtChartViewBase&);  // Not implemented.
};

#endif
