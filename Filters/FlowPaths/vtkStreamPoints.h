/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStreamPoints.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkStreamPoints
 * @brief   generate points along streamer separated by constant time increment
 *
 * vtkStreamPoints is a filter that generates points along a streamer.
 * The points are separated by a constant time increment. The resulting visual
 * effect (especially when coupled with vtkGlyph3D) is an indication of
 * particle speed.
 *
 * @sa
 * vtkStreamer vtkStreamLine vtkDashedStreamLine
*/

#ifndef vtkStreamPoints_h
#define vtkStreamPoints_h

#include "vtkFiltersFlowPathsModule.h" // For export macro
#include "vtkStreamer.h"

#ifndef VTK_LEGACY_REMOVE

class VTKFILTERSFLOWPATHS_EXPORT vtkStreamPoints : public vtkStreamer
{
public:
  vtkTypeMacro(vtkStreamPoints,vtkStreamer);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Construct object with time increment set to 1.0.
   */
  static vtkStreamPoints *New();

  //@{
  /**
   * Specify the separation of points in terms of absolute time.
   */
  vtkSetClampMacro(TimeIncrement,double,0.000001,VTK_DOUBLE_MAX);
  vtkGetMacro(TimeIncrement,double);
  //@}

protected:
  vtkStreamPoints();
  ~vtkStreamPoints() {}

  // Convert streamer array into vtkPolyData
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  // the separation of points
  double TimeIncrement;

private:
  vtkStreamPoints(const vtkStreamPoints&) VTK_DELETE_FUNCTION;
  void operator=(const vtkStreamPoints&) VTK_DELETE_FUNCTION;
};

#endif // VTK_LEGACY_REMOVE
#endif
