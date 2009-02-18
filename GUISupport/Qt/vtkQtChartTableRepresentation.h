/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartTableRepresentation.h

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
// .NAME vtkQtChartTableRepresentation - A representation for displaying a
// vtkTable in a vtkQtChartViewBase
//
// .SECTION Description
//
// This is a specialization of vtkQtTableDataRepresentation to put the
// data into a vtkQtChartViewBase.

#ifndef __vtkQtChartTableRepresentation_h
#define __vtkQtChartTableRepresentation_h

#include "vtkQtTableDataRepresentation.h"
class vtkQtChartTableSeriesModel;

class QVTK_EXPORT vtkQtChartTableRepresentation : public vtkQtTableDataRepresentation
{
public:
  static vtkQtChartTableRepresentation *New();
  vtkTypeRevisionMacro(vtkQtChartTableRepresentation, vtkQtTableDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Return the series model for this table representation
  vtkQtChartTableSeriesModel* GetSeriesModel();
 
  // Description:
  // Orients the table as being either columns-as-series or rows-as-series oriented.
  void SetColumnsAsSeries(bool);
  vtkGetMacro(ColumnsAsSeries,int);

protected:
  vtkQtChartTableRepresentation();
  ~vtkQtChartTableRepresentation();
  
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

  vtkQtChartTableRepresentation(const vtkQtChartTableRepresentation&);  // Not implemented.
  void operator=(const vtkQtChartTableRepresentation&);  // Not implemented.
};

#endif
