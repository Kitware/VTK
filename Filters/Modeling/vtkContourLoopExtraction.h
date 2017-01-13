/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContourLoopExtraction.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkContourLoopExtraction
 * @brief   extract closed loops (polygons) from lines
 *
 * This filter takes an input consisting of lines and polylines and
 * constructs polygons (i.e., closed loops) from them. It combines some of
 * the capability of connectivity filters and the line stripper to produce
 * manifold loops that are suitable for geometric operations. For example,
 * the vtkCookieCutter works well with this filter.
 *
 * Note that the input to this filter consists of points and line or polyline
 * cells. All other topological types (verts, polygons, triangle strips) are
 * ignored. The output of this filter is manifold polygons.
 *
 * @warning
 * Although the loops are constructed in 3-space, a normal vector must be
 * supplied to help select turns when multiple choices are possible. By
 * default the normal vector is n={0,0,1} but may be user specified. Note
 * also that some filters require that the loops are located in the
 * z=constant or z=0 plane. Hence a transform filter of some sort may be
 * necesssary to project the loops to a plane.
 *
 * @warning
 * Note that lines that do not close in on themselves can be optionally
 * forced closed. This occurs when for example, 2D contours end and begin at
 * the boundaries of data. By forcing closure, the last point is joined to
 * the first point. Note that there are different closure modes: 1) do not
 * close (and hence reject the polygon); 2) close along grid boundaries
 * (vertical or horizontal x and y lines); and 3) close all open loops.
 *
 * @warning
 * Scalar thresholding can be enabled. If enabled, then only those loops with
 * *any" scalar point data within the thresholded range are extracted.
 *
 * @warning
 * Any detached lines forming degenerate loops of two points or less are
 * discarded. Non-manifold junctions are broken into separate, independent
 * loops.
 *
 * @sa
 * vtkCookieCutter vtkFlyingEdges2D vtkMarchingSquares vtkFeatureEdges
 * vtkConnectivityFilter vtkPolyDataConnectivityFilter
*/

#ifndef vtkContourLoopExtraction_h
#define vtkContourLoopExtraction_h

#include "vtkFiltersModelingModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#define VTK_LOOP_CLOSURE_OFF 0
#define VTK_LOOP_CLOSURE_BOUNDARY 1
#define VTK_LOOP_CLOSURE_ALL 2

class VTKFILTERSMODELING_EXPORT vtkContourLoopExtraction : public vtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard methods to instantiate, print and provide type information.
   */
  static vtkContourLoopExtraction *New();
  vtkTypeMacro(vtkContourLoopExtraction,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Specify whether to close loops or not. All loops can be closed; boundary
   * loops (x or y vertical or horizontal lines) can be closed (default); or
   * all loops can be closed.
   */
  vtkSetClampMacro(LoopClosure,int,VTK_LOOP_CLOSURE_OFF,VTK_LOOP_CLOSURE_ALL);
  vtkGetMacro(LoopClosure,int);
  void SetLoopClosureToOff()
    {this->SetLoopClosure(VTK_LOOP_CLOSURE_OFF);};
  void SetLoopClosureToBoundary()
    {this->SetLoopClosure(VTK_LOOP_CLOSURE_BOUNDARY);};
  void SetLoopClosureToAll()
    {this->SetLoopClosure(VTK_LOOP_CLOSURE_ALL);};
  const char *GetLoopClosureAsString();
  //@}

  //@{
  /**
   * Turn on/off the extraction of loops based on scalar thresholding.  Loops
   * with scalar values in a specified range can be extracted. If no scalars
   * are available from the input than this data member is ignored.
   */
  vtkSetMacro(ScalarThresholding,bool);
  vtkGetMacro(ScalarThresholding,bool);
  vtkBooleanMacro(ScalarThresholding,bool);
  //@}

  //@{
  /**
   * Set the scalar range to use to extract loop based on scalar
   * connectivity.  If any scalar, point data, in the loop falls into the
   * scalar range given, then the loop is extracted.
   */
  vtkSetVector2Macro(ScalarRange,double);
  vtkGetVector2Macro(ScalarRange,double);
  //@}

  //@{
  /**
   * Set the normal vector used to orient the algorithm (controlling turns
   * around the loop). By default the normal points in the +z direction.
   */
  vtkSetVector3Macro(Normal,double);
  vtkGetVector3Macro(Normal,double);
  //@}

protected:
  vtkContourLoopExtraction();
  ~vtkContourLoopExtraction();

  int LoopClosure;
  bool ScalarThresholding;
  double ScalarRange[2];
  double Normal[3];

  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *) VTK_OVERRIDE;

private:
  vtkContourLoopExtraction(const vtkContourLoopExtraction&) VTK_DELETE_FUNCTION;
  void operator=(const vtkContourLoopExtraction&) VTK_DELETE_FUNCTION;
};


#endif
