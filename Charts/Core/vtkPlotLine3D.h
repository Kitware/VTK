/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlotLine3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkPlotLine3D - Class for drawing an XYZ line plot given three columns from
// a vtkTable.
//
// .SECTION Description
// This class draws points with a line between them given three column from a
// vtkTable in a vtkChartXYZ.
//
// .SECTION See Also
// vtkPlotPoints3D
// vtkPlotLine
//

#ifndef __vtkPlotLine3D_h
#define __vtkPlotLine3D_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkPlotPoints3D.h"

class VTKCHARTSCORE_EXPORT vtkPlotLine3D : public vtkPlotPoints3D
{
public:
  vtkTypeMacro(vtkPlotLine3D, vtkPlotPoints3D);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a 3D Chart object.
  static vtkPlotLine3D *New();

  // Description:
  // Paint event for the XYZ plot, called whenever the chart needs to be drawn.
  virtual bool Paint(vtkContext2D *painter);

//BTX
protected:
  vtkPlotLine3D();
  ~vtkPlotLine3D();

private:
  vtkPlotLine3D(const vtkPlotLine3D &); // Not implemented.
  void operator=(const vtkPlotLine3D &); // Not implemented.

//ETX
};

#endif //__vtkPlotLine3D_h
