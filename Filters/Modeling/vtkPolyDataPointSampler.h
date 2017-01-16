/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataPointSampler.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPolyDataPointSampler
 * @brief   generate points from vtkPolyData
 *
 * vtkPolyDataPointSampler generates points from input vtkPolyData. The
 * points are placed approximately a specified distance apart.
 *
 * This filter functions as follows. First, it regurgitates all input points,
 * then samples all lines, plus edges associated with the input polygons and
 * triangle strips to produce edge points. Finally, the interiors of polygons
 * and triangle strips are subsampled to produce points.  All of these
 * functiona can be enabled or disabled separately. Note that this algorithm
 * only approximately generates points the specified distance apart.
 * Generally the point density is finer than requested.
 *
 * @warning
 * Point generation can be useful in a variety of applications. For example,
 * generating seed points for glyphing or streamline generation. Another
 * useful application is generating points for implicit modeling. In many
 * cases implicit models can be more efficiently generated from points than
 * from polygons or other primitives.
 *
 * @sa
 * vtkImplicitModeller
*/

#ifndef vtkPolyDataPointSampler_h
#define vtkPolyDataPointSampler_h

#include "vtkFiltersModelingModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKFILTERSMODELING_EXPORT vtkPolyDataPointSampler : public vtkPolyDataAlgorithm
{
public:
  /**
   * Instantiate this class.
   */
  static vtkPolyDataPointSampler *New();

  //@{
  /**
   * Standard macros for type information and printing.
   */
  vtkTypeMacro(vtkPolyDataPointSampler,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Set/Get the approximate distance between points. This is an absolute
   * distance measure. The default is 0.01.
   */
  vtkSetClampMacro(Distance,double,0.0,VTK_FLOAT_MAX);
  vtkGetMacro(Distance,double);
  //@}

  //@{
  /**
   * Specify/retrieve a boolean flag indicating whether cell vertex points should
   * be output.
   */
  vtkGetMacro(GenerateVertexPoints,int);
  vtkSetMacro(GenerateVertexPoints,int);
  vtkBooleanMacro(GenerateVertexPoints,int);
  //@}

  //@{
  /**
   * Specify/retrieve a boolean flag indicating whether cell edges should
   * be sampled to produce output points. The default is true.
   */
  vtkGetMacro(GenerateEdgePoints,int);
  vtkSetMacro(GenerateEdgePoints,int);
  vtkBooleanMacro(GenerateEdgePoints,int);
  //@}

  //@{
  /**
   * Specify/retrieve a boolean flag indicating whether cell interiors should
   * be sampled to produce output points. The default is true.
   */
  vtkGetMacro(GenerateInteriorPoints,int);
  vtkSetMacro(GenerateInteriorPoints,int);
  vtkBooleanMacro(GenerateInteriorPoints,int);
  //@}

  //@{
  /**
   * Specify/retrieve a boolean flag indicating whether cell vertices should
   * be generated. Cell vertices are useful if you actually want to display
   * the points (that is, for each point generated, a vertex is generated).
   * Recall that VTK only renders vertices and not points.
   * The default is true.
   */
  vtkGetMacro(GenerateVertices,int);
  vtkSetMacro(GenerateVertices,int);
  vtkBooleanMacro(GenerateVertices,int);
  //@}

protected:
  vtkPolyDataPointSampler();
  ~vtkPolyDataPointSampler() VTK_OVERRIDE {}

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;

  double Distance;
  double Distance2;

  int GenerateVertexPoints;
  int GenerateEdgePoints;
  int GenerateInteriorPoints;
  int GenerateVertices;

  void SampleEdge(vtkPoints *pts, double x0[3], double x1[3]);
  void SampleTriangle(vtkPoints *newPts, vtkPoints *inPts,
                      vtkIdType *pts);
  void SamplePolygon(vtkPoints *newPts, vtkPoints *inPts,
                      vtkIdType npts, vtkIdType *pts);

private:
  vtkPolyDataPointSampler(const vtkPolyDataPointSampler&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPolyDataPointSampler&) VTK_DELETE_FUNCTION;
};

#endif
