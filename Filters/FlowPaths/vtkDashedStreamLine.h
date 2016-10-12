/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDashedStreamLine.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDashedStreamLine
 * @brief   generate constant-time dashed streamline in arbitrary dataset
 *
 * vtkDashedStreamLine is a filter that generates a "dashed" streamline for
 * an arbitrary dataset. The streamline consists of a series of dashes, each
 * of which represents (approximately) a constant time increment. Thus, in the
 * resulting visual representation, relatively long dashes represent areas of
 * high velocity, and small dashes represent areas of low velocity.
 *
 * vtkDashedStreamLine introduces the instance variable DashFactor.
 * DashFactor interacts with its superclass' instance variable StepLength to
 * create the dashes. DashFactor is the percentage of the StepLength line
 * segment that is visible. Thus, if the DashFactor=0.75, the dashes will be
 * "three-quarters on" and "one-quarter off".
 *
 * @sa
 * vtkStreamer vtkStreamLine vtkStreamPoints
*/

#ifndef vtkDashedStreamLine_h
#define vtkDashedStreamLine_h

#include "vtkFiltersFlowPathsModule.h" // For export macro
#include "vtkStreamLine.h"

#ifndef VTK_LEGACY_REMOVE

class VTKFILTERSFLOWPATHS_EXPORT vtkDashedStreamLine : public vtkStreamLine
{
public:
  static vtkDashedStreamLine *New();
  vtkTypeMacro(vtkDashedStreamLine,vtkStreamLine);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * For each dash, specify the fraction of the dash that is "on". A factor
   * of 1.0 will result in a continuous line, a factor of 0.5 will result in
   * dashed that are half on and half off.
   */
  vtkSetClampMacro(DashFactor,double,0.01,1.0);
  vtkGetMacro(DashFactor,double);
  //@}

protected:
  vtkDashedStreamLine();
  ~vtkDashedStreamLine() {}

  // Convert streamer array into vtkPolyData
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  // the fraction of on versus off in dash
  double DashFactor;

private:
  vtkDashedStreamLine(const vtkDashedStreamLine&) VTK_DELETE_FUNCTION;
  void operator=(const vtkDashedStreamLine&) VTK_DELETE_FUNCTION;
};

#endif // VTK_LEGACY_REMOVE
#endif
