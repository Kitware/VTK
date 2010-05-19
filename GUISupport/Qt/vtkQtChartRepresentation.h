/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartRepresentation.h

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
// .NAME vtkQtChartRepresentation - A representation for displaying a
// vtkTable in a vtkQtChartView
//
// .SECTION Description
//
// This is a specialization of vtkQtTableRepresentation to put the
// data into a vtkQtChartView.

#ifndef __vtkQtChartRepresentation_h
#define __vtkQtChartRepresentation_h

#include "vtkQtTableRepresentation.h"
class vtkIntArray;
class vtkQtChartSeriesOptionsModel;
class vtkQtChartTableSeriesModel;

class QVTK_EXPORT vtkQtChartRepresentation : public vtkQtTableRepresentation
{
public:
  static vtkQtChartRepresentation *New();
  vtkTypeMacro(vtkQtChartRepresentation, vtkQtTableRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  //BTX
  // Description:
  // Return the series model for this table representation
  vtkQtChartTableSeriesModel* GetSeriesModel();


  // Description:
  // Get/Set the series options model. By default,
  // vtkQtChartBasicSeriesOptionsModel will be used. The series options model
  // must be changed before the representation is added to a view.
  void SetOptionsModel(vtkQtChartSeriesOptionsModel*);
  vtkQtChartSeriesOptionsModel* GetOptionsModel();
  //ETX

  // Description:
  // Return the number of series.
  // This is equivalent to this->GetSeriesModel()->getNumberOfSeries().
  int GetNumberOfSeries();

  // Description:
  // Return the name of the series.  The returned const char may be null
  // if the series index is out of range.  The returned const char is only
  // valid until the next call to GetSeriesName.
  const char* GetSeriesName(int series);

  // Description:
  // Orients the table as being either columns-as-series or rows-as-series oriented.
  void SetColumnsAsSeries(bool);
  vtkGetMacro(ColumnsAsSeries,int);

protected:
  vtkQtChartRepresentation();
  ~vtkQtChartRepresentation();
  
  // Decription:
  // Adds the representation to the view.
  // This is called from vtkView::AddRepresentation().
  bool AddToView(vtkView* view);
  
  // Decription:
  // Removes the representation to the view.
  // This is called from vtkView::RemoveRepresentation().
  bool RemoveFromView(vtkView* view);

  bool ColumnsAsSeries;

private:

  //BTX
  class vtkInternal;
  vtkInternal* Internal;
  //ETX

  vtkQtChartRepresentation(const vtkQtChartRepresentation&);  // Not implemented.
  void operator=(const vtkQtChartRepresentation&);  // Not implemented.
};

#endif
