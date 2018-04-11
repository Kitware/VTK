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
 * @brief   extract closed loops (polygons) from lines and polylines
 *
 * This filter takes an input consisting of lines and polylines and
 * constructs polygons (i.e., closed loops) from them. It combines some of
 * the capability of connectivity filters and the line stripper to produce
 * manifold loops that are suitable for geometric operations. For example,
 * the vtkCookieCutter works well with this filter.
 *
 * Note that the input structure for this filter consists of points and line
 * or polyline cells. All other topological types (verts, polygons, triangle
 * strips) are ignored. The output of this filter is by default manifold
 * polygons. Note however, that optionally polyline loops may also be output
 * if requested.
 *
 * @warning
 * Although the loops are constructed in 3-space, a normal vector must be
 * supplied to help choose a turn direction when multiple choices are
 * possible. By default the normal vector is n={0,0,1} but may be user
 * specified. Note also that some filters require that the loops are located
 * in the z=constant or z=0 plane. Hence a transform filter of some sort may
 * be necesssary to project the loops to a plane.
 *
 * @warning
 * Note that lines that do not close in on themselves can be optionally
 * forced closed. This occurs when for example, 2D contours end and begin at
 * the boundaries of data. By forcing closure, the last point is joined to
 * the first point (with boundary points possibly added). Note that there are
 * different closure modes: 1) do not close (and hence reject the polygon);
 * 2) close along the dataset boundaries (i.e., the bounding box of a dataset
 * used to generate the contour lines); and 3) close all open loops by
 * connectiong the first and last point. If Option #2 is chosen, only loops
 * that start and end on either a horizontal or vertical boundary are closed.
 *
 * @warning
 * Scalar thresholding can be enabled. If enabled, then only those loops with
 * *any" scalar point data within the thresholded range are extracted.
 *
 * @warning
 * Any detached lines forming degenerate loops of defined by two points or
 * less are discarded. Non-manifold junctions are broken into separate,
 * independent loops.
 *
 * @warning
 * Boundary closure only works if the end points are both on a vertical
 * boundary or horizontal boundary. Otherwise new points would have to be
 * added which this filter does not (currently) do.
 *
 * @sa
 * vtkCookieCutter vtkFlyingEdges2D vtkMarchingSquares vtkFeatureEdges
 * vtkConnectivityFilter vtkPolyDataConnectivityFilter
 * vtkDiscreteFlyingEdges2D vtkStripper
 */

#ifndef vtkContourLoopExtraction_h
#define vtkContourLoopExtraction_h

#include "vtkFiltersModelingModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#define VTK_LOOP_CLOSURE_OFF 0
#define VTK_LOOP_CLOSURE_BOUNDARY 1
#define VTK_LOOP_CLOSURE_ALL 2

#define VTK_OUTPUT_POLYGONS 0
#define VTK_OUTPUT_POLYLINES 1
#define VTK_OUTPUT_BOTH 2

class VTKFILTERSMODELING_EXPORT vtkContourLoopExtraction : public vtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard methods to instantiate, print and provide type information.
   */
  static vtkContourLoopExtraction *New();
  vtkTypeMacro(vtkContourLoopExtraction,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify whether to close loops or not. All non-closed loops can be
   * rejected; boundary loops (end points lie on vertical or horizontal
   * porions of the boundary) can be closed (default); or all loops can be
   * forced closed by connecting first and last points.
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
   * thresholding.  If any scalar, point data, in the loop falls into the
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

  //@{
  /**
   * Specify the form of the output. Polygons can be output (default);
   * polylines can be output (the first and last point is repeated); or both
   * can be output.
   */
  vtkSetClampMacro(OutputMode,int,VTK_OUTPUT_POLYGONS,VTK_OUTPUT_BOTH);
  vtkGetMacro(OutputMode,int);
  void SetOutputModeToPolygons()
    {this->SetOutputMode(VTK_OUTPUT_POLYGONS);};
  void SetOutputModeToPolylines()
    {this->SetOutputMode(VTK_OUTPUT_POLYLINES);};
  void SetOutputModeToBoth()
    {this->SetOutputMode(VTK_OUTPUT_BOTH);};
  const char *GetOutputModeAsString();
  //@}


protected:
  vtkContourLoopExtraction();
  ~vtkContourLoopExtraction() override;

  int LoopClosure;
  bool ScalarThresholding;
  double ScalarRange[2];
  double Normal[3];
  int OutputMode;

  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *) override;

private:
  vtkContourLoopExtraction(const vtkContourLoopExtraction&) = delete;
  void operator=(const vtkContourLoopExtraction&) = delete;
};


#endif
