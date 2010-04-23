/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSimple3DCirclesStrategy.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSimple3DCirclesStrategy - places vertices on circles in 3D
//
// .SECTION Description
// Places vertices on circles depending on the graph vertices hierarchy level.
// The source graph could be vtkDirectedAcyclicGraph or vtkDirectedGraph if MarkedStartPoints array was added.
// The algorithm collects the standalone points, too and take them to a separated circle. If method is FixedRadiusMethod,
// the radius of the circles will be equal. If method is FixedDistanceMethod, the distance beetwen the points on circles will
// be equal.
//
// In first step initial points are searched. A point is initial, if its in degree equal zero and out degree is greater than zero (or
// marked by MarkedStartVertices and out degree is greater than zero). Independent vertices (in and out degree equal zero) are collected
// separatelly. In second step the hierarchical level is generated for every vertex. In third step the hierarchical order is generated.
// If a vertex has no hierarchical level and it is not independent, the graph has loop so the algorithm exit with error message. Finally
// the vertices positions are calculated by the hierarchical order and by the vertices hierarchy levels.
//
// .SECTION Thanks
// Ferenc Nasztanovics, naszta@naszta.hu, Budapest University of Technology and Economics, Department of Structural Mechanics
//
// .SECTION References
// in 3D rotation was used: http://en.citizendium.org/wiki/Rotation_matrix

#ifndef vtkSimple3DCirclesStrategyH
#define vtkSimple3DCirclesStrategyH 1

#include "vtkGraphLayoutStrategy.h"

class vtkDirectedGraph;
class vtkIdTypeArray;
class vtkIntArray;
class vtkSimple3DCirclesStrategyInternal;

class VTK_INFOVIS_EXPORT vtkSimple3DCirclesStrategy : public vtkGraphLayoutStrategy
  {
public:
  static vtkSimple3DCirclesStrategy * New();
  vtkTypeMacro(vtkSimple3DCirclesStrategy,vtkGraphLayoutStrategy);
  void PrintSelf( ostream& os, vtkIndent indent );

//BTX
  enum
    {
    FixedRadiusMethod = 0, FixedDistanceMethod = 1
    };
//ETX
  // Description:
  // Set or get cicrle generating method (FixedRadiusMethod/FixedDistanceMethod). Default is FixedRadiusMethod.
  vtkSetMacro(Method,int);
  vtkGetMacro(Method,int);
  // Description:
  // If Method is FixedRadiusMethod: Set or get the radius of the circles.
  // If Method is FixedDistanceMethod: Set or get the distance of the points in the circle.
  vtkSetMacro(Radius,double);
  vtkGetMacro(Radius,double);
  // Description:
  // Set or get the vertical (local z) distance between the circles. If AutoHeight is on, this is the minimal height between
  // the circle layers
  vtkSetMacro(Height,double);
  vtkGetMacro(Height,double);
  // Description:
  // Set or get the orign of the geometry. This is the center of the first circle. SetOrign(x,y,z)
  vtkSetVector3Macro(Orign,double);
  vtkGetVector3Macro(Orign,double);
  // Description:
  // Set or get the normal vector of the circles plain. The height is growing in this direction. The direction must not be zero vector.
  // The default vector is (0.0,0.0,1.0)
  virtual void SetDirection( double dx, double dy, double dz );
  virtual void SetDirection( double d[3] );
  vtkGetVector3Macro(Direction,double);
  // Description:
  // Set or get initial vertices. If MarkedStartVertices is added, loop is accepted in the graph. (If all of the loop start vertices are
  // marked in MarkedStartVertices array.) MarkedStartVertices size must be equal with the number of the vertices in the graph. Start
  // vertices must be marked by MarkedValue. (E.g.: if MarkedValue=3 and MarkedStartPoints is { 0, 3, 5, 3 }, the start points ids will
  // be {1,3}.) )
  virtual void SetMarkedStartVertices( vtkIntArray * _arg );
  vtkGetObjectMacro(MarkedStartVertices,vtkIntArray);
  // Description:
  // Set or get MarkedValue. See: MarkedStartVertices.
  vtkSetMacro(MarkedValue,int);
  vtkGetMacro(MarkedValue,int);
  // Description:
  // Set or get ForceToUseUniversalStartPointsFinder. If ForceToUseUniversalStartPointsFinder is true, MarkedStartVertices won't be used.
  // In this case the input graph must be vtkDirectedAcyclicGraph (Defualt: false).
  vtkSetMacro(ForceToUseUniversalStartPointsFinder,int);
  vtkGetMacro(ForceToUseUniversalStartPointsFinder,int);
  vtkBooleanMacro(ForceToUseUniversalStartPointsFinder,int);
  // Description:
  // Set or get auto height (Default: false). If AutoHeight is true, (r(i+1) - r(i-1))/Height will be smaller than tan(MinimumRadian).
  // If you want equal distances and parallel circles, you should turn off AutoHeight.
  vtkSetMacro(AutoHeight,int);
  vtkGetMacro(AutoHeight,int);
  vtkBooleanMacro(AutoHeight,int);
  // Description:
  // Set or get minimum radian (used by auto height).
  vtkSetMacro(MinimumRadian,double);
  vtkGetMacro(MinimumRadian,double);
  // Description:
  // Set or get minimum degree (used by auto height). There is no separated minimum degree, so minimum radian will be changed.
  virtual void SetMinimumDegree( double degree );
  virtual double GetMinimumDegree( void );
  // Description:
  // Set or get hierarchical layers id by vertices (An usual vertex's layer id is greater or equal to zero. If a vertex is standalone, its
  // layer id is -2.) If no HierarchicalLayers array is defined, vtkSimple3DCirclesStrategy will generate it automatically (default).
  virtual void SetHierarchicalLayers( vtkIntArray * _arg );
  vtkGetObjectMacro(HierarchicalLayers,vtkIntArray);
  // Description:
  // Set or get hierarchical ordering of vertices (The array starts from the first vertex's id. All id must be greater or equal to zero!)
  // If no HierarchicalOrder is defined, vtkSimple3DCirclesStrategy will generate it automatically (default).
  virtual void SetHierarchicalOrder( vtkIdTypeArray * _arg );
  vtkGetObjectMacro(HierarchicalOrder,vtkIdTypeArray);
  // Description:
  // Standard layout method
  virtual void Layout( void );
  // Description:
  // Set graph (warning: HierarchicalOrder and HierarchicalLayers will set to zero. These reference counts will be decreased!)
  virtual void SetGraph( vtkGraph * graph );
protected:
//BTX
  vtkSimple3DCirclesStrategy( void );
  virtual ~vtkSimple3DCirclesStrategy( void );

  inline void Transform( double Local[], double Global[] );

  double Radius;
  double Height;
  double Orign[3];
  double Direction[3];
  int Method;
  vtkIntArray * MarkedStartVertices;
  int MarkedValue;
  int ForceToUseUniversalStartPointsFinder;
  int AutoHeight;
  double MinimumRadian;

  vtkIntArray * HierarchicalLayers;
  vtkIdTypeArray * HierarchicalOrder;
//ETX
private:
//BTX
  // Description:
  // Search and fill in target all zero input degree vertices where the output degree is more than zero. The finded vertices hierarchical
  // layer ID will be zero.
  virtual int UniversalStartPoints( vtkDirectedGraph * input, vtkSimple3DCirclesStrategyInternal * target, vtkSimple3DCirclesStrategyInternal *StandAlones, vtkIntArray * layers );
  // Description:
  // Build hierarchical layers in the graph. A vertices hierarchical layer number is equal the maximum of its inputs hierarchical layer numbers plus one.
  virtual int BuildLayers( vtkDirectedGraph * input, vtkSimple3DCirclesStrategyInternal * source, vtkIntArray * layers );
  // Description:
  // Build hierarchical ordering of the graph points.
  virtual void BuildPointOrder( vtkDirectedGraph * input, vtkSimple3DCirclesStrategyInternal *source, vtkSimple3DCirclesStrategyInternal *StandAlones, vtkIntArray * layers, vtkIdTypeArray * order );

  double T[3][3];
//ETX

  vtkSimple3DCirclesStrategy(const vtkSimple3DCirclesStrategy&);  // Not implemented.
  void operator=(const vtkSimple3DCirclesStrategy&);  // Not implemented.
  };

#endif
