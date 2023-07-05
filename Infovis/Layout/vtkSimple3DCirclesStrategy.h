// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSimple3DCirclesStrategy
 * @brief   places vertices on circles in 3D
 *
 *
 * Places vertices on circles depending on the graph vertices hierarchy level.
 * The source graph could be vtkDirectedAcyclicGraph or vtkDirectedGraph if MarkedStartPoints array
 * was added. The algorithm collects the standalone points, too and take them to a separated circle.
 * If method is FixedRadiusMethod, the radius of the circles will be equal. If method is
 * FixedDistanceMethod, the distance between the points on circles will be equal.
 *
 * In first step initial points are searched. A point is initial, if its in degree equal zero and
 * out degree is greater than zero (or marked by MarkedStartVertices and out degree is greater than
 * zero). Independent vertices (in and out degree equal zero) are collected separately. In second
 * step the hierarchical level is generated for every vertex. In third step the hierarchical order
 * is generated. If a vertex has no hierarchical level and it is not independent, the graph has loop
 * so the algorithm exit with error message. Finally the vertices positions are calculated by the
 * hierarchical order and by the vertices hierarchy levels.
 *
 * @par Thanks:
 * Ferenc Nasztanovics, naszta@naszta.hu, Budapest University of Technology and Economics,
 * Department of Structural Mechanics
 *
 * @par References:
 * in 3D rotation was used: http://en.citizendium.org/wiki/Rotation_matrix
 */

#ifndef vtkSimple3DCirclesStrategy_h
#define vtkSimple3DCirclesStrategy_h

#include "vtkGraphLayoutStrategy.h"
#include "vtkInfovisLayoutModule.h" // For export macro
#include "vtkVariant.h"             // For variant API

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractArray;
class vtkDirectedGraph;
class vtkIdTypeArray;
class vtkIntArray;
class vtkSimple3DCirclesStrategyInternal;

class VTKINFOVISLAYOUT_EXPORT vtkSimple3DCirclesStrategy : public vtkGraphLayoutStrategy
{
public:
  static vtkSimple3DCirclesStrategy* New();
  vtkTypeMacro(vtkSimple3DCirclesStrategy, vtkGraphLayoutStrategy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum
  {
    FixedRadiusMethod = 0,
    FixedDistanceMethod = 1
  };

  ///@{
  /**
   * Set or get circle generating method (FixedRadiusMethod/FixedDistanceMethod). Default is
   * FixedRadiusMethod.
   */
  vtkSetMacro(Method, int);
  vtkGetMacro(Method, int);
  ///@}
  ///@{
  /**
   * If Method is FixedRadiusMethod: Set or get the radius of the circles.
   * If Method is FixedDistanceMethod: Set or get the distance of the points in the circle.
   */
  vtkSetMacro(Radius, double);
  vtkGetMacro(Radius, double);
  ///@}
  ///@{
  /**
   * Set or get the vertical (local z) distance between the circles. If AutoHeight is on, this is
   * the minimal height between the circle layers
   */
  vtkSetMacro(Height, double);
  vtkGetMacro(Height, double);
  ///@}
  ///@{
  /**
   * Set or get the origin of the geometry. This is the center of the first circle. SetOrigin(x,y,z)
   */
  vtkSetVector3Macro(Origin, double);
  vtkGetVector3Macro(Origin, double);
  ///@}
  ///@{
  /**
   * Set or get the normal vector of the circles plain. The height is growing in this direction. The
   * direction must not be zero vector. The default vector is (0.0,0.0,1.0)
   */
  virtual void SetDirection(double dx, double dy, double dz);
  virtual void SetDirection(double d[3]);
  vtkGetVector3Macro(Direction, double);
  ///@}
  ///@{
  /**
   * Set or get initial vertices. If MarkedStartVertices is added, loop is accepted in the graph.
   * (If all of the loop start vertices are marked in MarkedStartVertices array.)
   * MarkedStartVertices size must be equal with the number of the vertices in the graph. Start
   * vertices must be marked by MarkedValue. (E.g.: if MarkedValue=3 and MarkedStartPoints is { 0,
   * 3, 5, 3 }, the start points ids will be {1,3}.) )
   */
  virtual void SetMarkedStartVertices(vtkAbstractArray* _arg);
  vtkGetObjectMacro(MarkedStartVertices, vtkAbstractArray);
  ///@}
  ///@{
  /**
   * Set or get MarkedValue. See: MarkedStartVertices.
   */
  virtual void SetMarkedValue(vtkVariant _arg);
  virtual vtkVariant GetMarkedValue();
  ///@}
  ///@{
  /**
   * Set or get ForceToUseUniversalStartPointsFinder. If ForceToUseUniversalStartPointsFinder is
   * true, MarkedStartVertices won't be used. In this case the input graph must be
   * vtkDirectedAcyclicGraph (Default: false).
   */
  vtkSetMacro(ForceToUseUniversalStartPointsFinder, vtkTypeBool);
  vtkGetMacro(ForceToUseUniversalStartPointsFinder, vtkTypeBool);
  vtkBooleanMacro(ForceToUseUniversalStartPointsFinder, vtkTypeBool);
  ///@}
  ///@{
  /**
   * Set or get auto height (Default: false). If AutoHeight is true, (r(i+1) - r(i-1))/Height will
   * be smaller than tan(MinimumRadian). If you want equal distances and parallel circles, you
   * should turn off AutoHeight.
   */
  vtkSetMacro(AutoHeight, vtkTypeBool);
  vtkGetMacro(AutoHeight, vtkTypeBool);
  vtkBooleanMacro(AutoHeight, vtkTypeBool);
  ///@}
  ///@{
  /**
   * Set or get minimum radian (used by auto height).
   */
  vtkSetMacro(MinimumRadian, double);
  vtkGetMacro(MinimumRadian, double);
  ///@}
  ///@{
  /**
   * Set or get minimum degree (used by auto height). There is no separated minimum degree, so
   * minimum radian will be changed.
   */
  virtual void SetMinimumDegree(double degree);
  virtual double GetMinimumDegree();
  ///@}
  ///@{
  /**
   * Set or get hierarchical layers id by vertices (An usual vertex's layer id is greater or equal
   * to zero. If a vertex is standalone, its layer id is -2.) If no HierarchicalLayers array is
   * defined, vtkSimple3DCirclesStrategy will generate it automatically (default).
   */
  virtual void SetHierarchicalLayers(vtkIntArray* _arg);
  vtkGetObjectMacro(HierarchicalLayers, vtkIntArray);
  ///@}
  ///@{
  /**
   * Set or get hierarchical ordering of vertices (The array starts from the first vertex's id. All
   * id must be greater or equal to zero!) If no HierarchicalOrder is defined,
   * vtkSimple3DCirclesStrategy will generate it automatically (default).
   */
  virtual void SetHierarchicalOrder(vtkIdTypeArray* _arg);
  vtkGetObjectMacro(HierarchicalOrder, vtkIdTypeArray);
  ///@}
  /**
   * Standard layout method
   */
  void Layout() override;
  /**
   * Set graph (warning: HierarchicalOrder and HierarchicalLayers will set to zero. These reference
   * counts will be decreased!)
   */
  void SetGraph(vtkGraph* graph) override;

protected:
  vtkSimple3DCirclesStrategy();
  ~vtkSimple3DCirclesStrategy() override;

  inline void Transform(double Local[], double Global[]);

  double Radius;
  double Height;
  double Origin[3];
  double Direction[3];
  int Method;
  vtkAbstractArray* MarkedStartVertices;
  vtkVariant MarkedValue;
  vtkTypeBool ForceToUseUniversalStartPointsFinder;
  vtkTypeBool AutoHeight;
  double MinimumRadian;

  vtkIntArray* HierarchicalLayers;
  vtkIdTypeArray* HierarchicalOrder;

private:
  /**
   * Search and fill in target all zero input degree vertices where the output degree is more than
   * zero. The found vertices hierarchical layer ID will be zero.
   */
  virtual int UniversalStartPoints(vtkDirectedGraph* input,
    vtkSimple3DCirclesStrategyInternal* target, vtkSimple3DCirclesStrategyInternal* StandAlones,
    vtkIntArray* layers);
  /**
   * Build hierarchical layers in the graph. A vertices hierarchical layer number is equal the
   * maximum of its inputs hierarchical layer numbers plus one.
   */
  virtual int BuildLayers(
    vtkDirectedGraph* input, vtkSimple3DCirclesStrategyInternal* source, vtkIntArray* layers);
  /**
   * Build hierarchical ordering of the graph points.
   */
  virtual void BuildPointOrder(vtkDirectedGraph* input, vtkSimple3DCirclesStrategyInternal* source,
    vtkSimple3DCirclesStrategyInternal* StandAlones, vtkIntArray* layers, vtkIdTypeArray* order);

  double T[3][3];

  vtkSimple3DCirclesStrategy(const vtkSimple3DCirclesStrategy&) = delete;
  void operator=(const vtkSimple3DCirclesStrategy&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
