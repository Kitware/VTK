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
 * points are placed approximately a specified distance apart. Optionally,
 * the points attributes can be interpolated from the generating vertices,
 * edges, and polygons.
 *
 * This filter functions as follows. First, it regurgitates all input points,
 * then it samples all lines, plus edges associated with the input polygons
 * and triangle strips to produce edge points. Finally, the interiors of
 * polygons and triangle strips are subsampled to produce points. All of
 * these operations can be enabled or disabled separately. Note that this
 * algorithm only approximately generates points the specified distance
 * apart. Generally the point density is finer than requested.
 *
 * @warning
 * While this algorithm processes general polygons. it does so by performing
 * a fan triangulation. This may produce poor results, especially for convave
 * polygons. For better results, use a triangle filter to pre-tesselate
 * polygons.
 *
 * @warning
 * Point generation can be useful in a variety of applications. For example,
 * generating seed points for glyphing or streamline generation. Another
 * useful application is generating points for implicit modeling. In many
 * cases implicit models can be more efficiently generated from points than
 * from polygons or other primitives.
 *
 * @warning
 * When sampling polygons of 5 sides or more, the polygon is triangulated.
 * This can result in variations in point density near tesselation boudaries.
 *
 * @sa
 * vtkTriangleFilter vtkImplicitModeller
 */

#ifndef vtkPolyDataPointSampler_h
#define vtkPolyDataPointSampler_h

#include "vtkEdgeTable.h"             // for sampling edges
#include "vtkFiltersModelingModule.h" // For export macro
#include "vtkNew.h"                   // for data members
#include "vtkPolyDataAlgorithm.h"

class VTKFILTERSMODELING_EXPORT vtkPolyDataPointSampler : public vtkPolyDataAlgorithm
{
public:
  /**
   * Instantiate this class.
   */
  static vtkPolyDataPointSampler* New();

  //@{
  /**
   * Standard macros for type information and printing.
   */
  vtkTypeMacro(vtkPolyDataPointSampler, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  //@{
  /**
   * Set/Get the approximate distance between points. This is an absolute
   * distance measure. The default is 0.01.
   */
  vtkSetClampMacro(Distance, double, 0.0, VTK_FLOAT_MAX);
  vtkGetMacro(Distance, double);
  //@}

  //@{
  /**
   * Specify/retrieve a boolean flag indicating whether cell vertex points should
   * be output.
   */
  vtkGetMacro(GenerateVertexPoints, bool);
  vtkSetMacro(GenerateVertexPoints, bool);
  vtkBooleanMacro(GenerateVertexPoints, bool);
  //@}

  //@{
  /**
   * Specify/retrieve a boolean flag indicating whether cell edges should
   * be sampled to produce output points. The default is true.
   */
  vtkGetMacro(GenerateEdgePoints, bool);
  vtkSetMacro(GenerateEdgePoints, bool);
  vtkBooleanMacro(GenerateEdgePoints, bool);
  //@}

  //@{
  /**
   * Specify/retrieve a boolean flag indicating whether cell interiors should
   * be sampled to produce output points. The default is true.
   */
  vtkGetMacro(GenerateInteriorPoints, bool);
  vtkSetMacro(GenerateInteriorPoints, bool);
  vtkBooleanMacro(GenerateInteriorPoints, bool);
  //@}

  //@{
  /**
   * Specify/retrieve a boolean flag indicating whether cell vertices should
   * be generated. Cell vertices are useful if you actually want to display
   * the points (that is, for each point generated, a vertex is generated).
   * Recall that VTK only renders vertices and not points.  The default is
   * true.
   */
  vtkGetMacro(GenerateVertices, bool);
  vtkSetMacro(GenerateVertices, bool);
  vtkBooleanMacro(GenerateVertices, bool);
  //@}

  //@{
  /**
   * Specify/retrieve a boolean flag indicating whether point data should be
   * interpolated onto the newly generated points. If enabled, points
   * generated from existing vertices will carry the vertex point data;
   * points generated from edges will interpolate point data along each edge;
   * and interior point data (inside triangles, polygons cells) will be
   * interpolated from the cell vertices. By default this is off.
   */
  vtkGetMacro(InterpolatePointData, bool);
  vtkSetMacro(InterpolatePointData, bool);
  vtkBooleanMacro(InterpolatePointData, bool);
  //@}

protected:
  vtkPolyDataPointSampler();
  ~vtkPolyDataPointSampler() override {}

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  double Distance;
  double Distance2;

  bool GenerateVertexPoints;
  bool GenerateEdgePoints;
  bool GenerateInteriorPoints;
  bool GenerateVertices;

  bool InterpolatePointData;

  // Internal scratch arrays supporting point data interpolation, and
  // sampling edges.
  vtkNew<vtkEdgeTable> EdgeTable;
  double TriWeights[3];
  vtkNew<vtkIdList> TriIds;
  double QuadWeights[4];
  vtkNew<vtkIdList> QuadIds;

  // Internal methods for sampling edges, triangles, and polygons
  void SampleEdge(
    vtkPoints* pts, vtkIdType p0, vtkIdType p1, vtkPointData* inPD, vtkPointData* outPD);
  void SampleTriangle(vtkPoints* newPts, vtkPoints* inPts, const vtkIdType* pts, vtkPointData* inPD,
    vtkPointData* outPD);
  void SamplePolygon(vtkPoints* newPts, vtkPoints* inPts, vtkIdType npts, const vtkIdType* pts,
    vtkPointData* inPD, vtkPointData* outPD);

private:
  vtkPolyDataPointSampler(const vtkPolyDataPointSampler&) = delete;
  void operator=(const vtkPolyDataPointSampler&) = delete;
};

#endif
