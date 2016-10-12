/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSCurveSpline.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/
/**
 * @class   vtkSCurveSpline
 * @brief   computes an interpolating spline using a
 * a SCurve basis.
 *
 *
 * vtkSCurveSpline is a concrete implementation of vtkSpline using a
 * SCurve basis.
 *
 * @sa
 * vtkSpline vtkKochanekSpline
*/

#ifndef vtkSCurveSpline_h
#define vtkSCurveSpline_h

#include "vtkViewsInfovisModule.h" // For export macro
#include "vtkSpline.h"

class VTKVIEWSINFOVIS_EXPORT vtkSCurveSpline : public vtkSpline
{
public:
  static vtkSCurveSpline *New();

  vtkTypeMacro(vtkSCurveSpline,vtkSpline);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Compute SCurve Splines for each dependent variable
   */
  void Compute ();

  /**
   * Evaluate a 1D SCurve spline.
   */
  virtual double Evaluate (double t);

  /**
   * Deep copy of SCurve spline data.
   */
  virtual void DeepCopy(vtkSpline *s);

  vtkSetMacro(NodeWeight,double);
  vtkGetMacro(NodeWeight,double);
protected:
  vtkSCurveSpline();
  ~vtkSCurveSpline() {}

  double NodeWeight;

private:
  vtkSCurveSpline(const vtkSCurveSpline&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSCurveSpline&) VTK_DELETE_FUNCTION;
};

#endif
