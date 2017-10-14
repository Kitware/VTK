/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSplineGraphEdges.h

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
/**
 * @class   vtkSplineGraphEdges
 * @brief   subsample graph edges to make smooth curves
 *
 *
 * vtkSplineGraphEdges uses a vtkSpline to make edges into nicely sampled
 * splines. By default, the filter will use an optimized b-spline.
 * Otherwise, it will use a custom vtkSpline instance set by the user.
*/

#ifndef vtkSplineGraphEdges_h
#define vtkSplineGraphEdges_h

#include "vtkInfovisLayoutModule.h" // For export macro
#include "vtkGraphAlgorithm.h"
#include "vtkSmartPointer.h" // For ivars

class vtkSpline;

class VTKINFOVISLAYOUT_EXPORT vtkSplineGraphEdges : public vtkGraphAlgorithm
{
public:
  static vtkSplineGraphEdges *New();
  vtkTypeMacro(vtkSplineGraphEdges,vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * If SplineType is CUSTOM, uses this spline.
   */
  virtual void SetSpline(vtkSpline* s);
  vtkGetObjectMacro(Spline, vtkSpline);
  //@}

  enum
  {
    BSPLINE = 0,
    CUSTOM
  };

  //@{
  /**
   * Spline type used by the filter.
   * BSPLINE (0) - Use optimized b-spline (default).
   * CUSTOM (1) - Use spline set with SetSpline.
   */
  vtkSetMacro(SplineType, int);
  vtkGetMacro(SplineType, int);
  //@}

  //@{
  /**
   * The number of subdivisions in the spline.
   */
  vtkSetMacro(NumberOfSubdivisions, vtkIdType);
  vtkGetMacro(NumberOfSubdivisions, vtkIdType);
  //@}

protected:
  vtkSplineGraphEdges();
  ~vtkSplineGraphEdges() override;

  int RequestData(
    vtkInformation *,
    vtkInformationVector **,
    vtkInformationVector *) override;

  vtkMTimeType GetMTime() override;

  void GeneratePoints(vtkGraph* g, vtkIdType e);
  void GenerateBSpline(vtkGraph* g, vtkIdType e);

  vtkSpline* Spline;

  int SplineType;

  vtkSmartPointer<vtkSpline> XSpline;
  vtkSmartPointer<vtkSpline> YSpline;
  vtkSmartPointer<vtkSpline> ZSpline;

  vtkIdType NumberOfSubdivisions;

private:
  vtkSplineGraphEdges(const vtkSplineGraphEdges&) = delete;
  void operator=(const vtkSplineGraphEdges&) = delete;
};

#endif
