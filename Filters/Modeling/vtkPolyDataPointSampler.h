// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPolyDataPointSampler
 * @brief   generate points from vtkPolyData
 *
 * vtkPolyDataPointSampler generates points from input vtkPolyData. The
 * filter has two modes of operation: random point generation, or regular
 * point generation. In random generation mode, points are generated in each
 * polygonal entity using a random approach. In regular generation mode, the
 * points are placed approximately a specified distance apart. Optionally,
 * the points attributes can be interpolated from the generating vertices,
 * edges, and polygons.
 *
 * In regular point generation mode, this filter functions as follows. First,
 * it regurgitates all input points, then it samples all lines, plus edges
 * associated with the input polygons and triangle strips to produce edge
 * points. Finally, the interiors of polygons and triangle strips are
 * subsampled to produce points. All of these operations can be enabled or
 * disabled separately. Note that this algorithm only approximately generates
 * points the specified distance apart. Generally the point density is finer
 * than requested.
 *
 * In random point generation mode, this filter functions as follows. First,
 * it randomly regurgitates all input points (if enabled), then it randomly
 * samples all lines, plus edges associated with the input polygons and
 * triangle strips to produce edge points (if enabled). Finally, the
 * interiors of polygons and triangle strips are randomly subsampled to
 * produce points. All of these operations can be enabled or disabled
 * separately. Note that this algorithm only approximately generates points
 * the specified distance apart. Generally the point density is finer than
 * requested. Also note that the result is not truly random due to the
 * constraints of the mesh construction.
 *
 * @warning
 * Although this algorithm processes general polygons. it does so by performing
 * a fan triangulation. This may produce poor results, especially for concave
 * polygons. For better results, use a triangle filter to pre-tesselate
 * polygons.
 *
 * @warning
 * In random point generation mode, producing random edges and vertex points
 * from polygons and triangle strips is less random than is typically
 * desirable. You may wish to disable vertex and edge point generation for a
 * result that is closer to random.
 *
 * @warning
 * Point generation can be useful in a variety of applications. For example,
 * generating seed points for glyphing or streamline generation. Another
 * useful application is generating points for implicit modeling. In many
 * cases implicit models can be more efficiently generated from points than
 * from polygons or other primitives.
 *
 * @warning
 * When sampling polygons of five sides or more, the polygon is triangulated.
 * This can result in variations in point density near tessellation boundaries.
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

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSMODELING_EXPORT vtkPolyDataPointSampler : public vtkPolyDataAlgorithm
{
public:
  /**
   * Instantiate this class.
   */
  static vtkPolyDataPointSampler* New();

  ///@{
  /**
   * Standard macros for type information and printing.
   */
  vtkTypeMacro(vtkPolyDataPointSampler, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Set/Get the approximate distance between points. This is an absolute
   * distance measure. The default is 0.01.
   */
  vtkSetClampMacro(Distance, double, 0.0, VTK_FLOAT_MAX);
  vtkGetMacro(Distance, double);
  ///@}

  /**
   * Specify how points are to be generated.
   */
  enum
  {
    REGULAR_GENERATION,
    RANDOM_GENERATION
  };

  ///@{
  /**
   * Specify/retrieve the type of point generation: either regular point
   * generation or random point generation. By default, regular point
   * generation is used.
   */
  vtkSetClampMacro(PointGenerationMode, int, REGULAR_GENERATION, RANDOM_GENERATION);
  vtkGetMacro(PointGenerationMode, int);
  void SetPointGenerationModeToRegular() { this->SetPointGenerationMode(REGULAR_GENERATION); }
  void SetPointGenerationModeToRandom() { this->SetPointGenerationMode(RANDOM_GENERATION); }
  ///@}

  ///@{
  /**
   * Specify/retrieve a boolean flag indicating whether cell vertex points should
   * be output.
   */
  vtkGetMacro(GenerateVertexPoints, bool);
  vtkSetMacro(GenerateVertexPoints, bool);
  vtkBooleanMacro(GenerateVertexPoints, bool);
  ///@}

  ///@{
  /**
   * Specify/retrieve a boolean flag indicating whether cell edges should
   * be sampled to produce output points. The default is true.
   */
  vtkGetMacro(GenerateEdgePoints, bool);
  vtkSetMacro(GenerateEdgePoints, bool);
  vtkBooleanMacro(GenerateEdgePoints, bool);
  ///@}

  ///@{
  /**
   * Specify/retrieve a boolean flag indicating whether cell interiors should
   * be sampled to produce output points. The default is true.
   */
  vtkGetMacro(GenerateInteriorPoints, bool);
  vtkSetMacro(GenerateInteriorPoints, bool);
  vtkBooleanMacro(GenerateInteriorPoints, bool);
  ///@}

  ///@{
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
  ///@}

  ///@{
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
  ///@}

protected:
  vtkPolyDataPointSampler();
  ~vtkPolyDataPointSampler() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  double Distance;
  int PointGenerationMode;

  bool GenerateVertexPoints;
  bool GenerateEdgePoints;
  bool GenerateInteriorPoints;
  bool GenerateVertices;

  bool InterpolatePointData;

private:
  vtkPolyDataPointSampler(const vtkPolyDataPointSampler&) = delete;
  void operator=(const vtkPolyDataPointSampler&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
