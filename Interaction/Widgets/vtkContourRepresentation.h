/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContourRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkContourRepresentation
 * @brief   represent the vtkContourWidget
 *
 * The vtkContourRepresentation is a superclass for various types of
 * representations for vtkContourWidget.
 *
 * @par Managing contour points:
 * The classes vtkContourRepresentationNode, vtkContourRepresentationInternals,
 * and vtkContourRepresentationPoint manage the data structure used to represent
 * nodes and points on a contour. A contour may contain several nodes and
 * several additional points. Nodes are usually the result of user-clicked points
 * on the contour. Additional points are created between nodes to generate a
 * smooth curve using some Interpolator -- see the method \c SetLineInterpolator.
 *
 * @par Managing contour points:
 * \par
 * The data structure stores both the world and display positions for every
 * point. (This may seem like a duplication.) The default behaviour of this
 * class is to use the WorldPosition to do all the math. Typically a point is
 * added at a given display position. Its corresponding world position is
 * computed using the point placer, and stored. Any query of the display
 * position of a stored point is done via the Renderer, which computes the
 * display position given a world position.
 *
 * @par Managing contour points:
 * \par
 * So why maintain the display position? Consider drawing a contour on a
 * volume widget. You might want the contour to be located at a certain world
 * position in the volume or you might want to be overlayed over the window
 * like an Actor2D. The default behaviour of this class is to provide the
 * former behaviour.
 *
 * @par Managing contour points:
 * \par
 * To achieve the latter behaviour, override the methods that return the display
 * position (to return the set display position instead of computing it from
 * the world positions) and the method \c BuildLines() to interpolate lines
 * using their display positions instead of world positions.
 *
 * @sa
 * vtkContourWidget
*/

#ifndef vtkContourRepresentation_h
#define vtkContourRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWidgetRepresentation.h"
#include <vector> // STL Header; Required for vector

class vtkContourLineInterpolator;
class vtkIncrementalOctreePointLocator;
class vtkPointPlacer;
class vtkPolyData;
class vtkIdList;

//----------------------------------------------------------------------
class vtkContourRepresentationPoint
{
public:
  double        WorldPosition[3];
  double        NormalizedDisplayPosition[2];

  // The point id. This is blank except in the case of
  // vtkPolygonalSurfaceContourLineInterpolator
  vtkIdType     PointId;
};

class vtkContourRepresentationNode
{
public:
  double        WorldPosition[3];
  double        WorldOrientation[9];
  double        NormalizedDisplayPosition[2];
  int           Selected;
  std::vector<vtkContourRepresentationPoint*> Points;

  // The point id. This is blank except in the case of
  // vtkPolygonalSurfaceContourLineInterpolator
  vtkIdType     PointId;
};

class vtkContourRepresentationInternals
{
public:
  std::vector<vtkContourRepresentationNode*> Nodes;
  void ClearNodes()
  {
    for(unsigned int i=0;i<this->Nodes.size();i++)
    {
      for (unsigned int j=0;j<this->Nodes[i]->Points.size();j++)
      {
        delete this->Nodes[i]->Points[j];
      }
      this->Nodes[i]->Points.clear();
      delete this->Nodes[i];
    }
    this->Nodes.clear();
  }
};

class VTKINTERACTIONWIDGETS_EXPORT vtkContourRepresentation : public vtkWidgetRepresentation
{
  friend class vtkContourWidget;
public:
  //@{
  /**
   * Standard VTK methods.
   */
  vtkTypeMacro(vtkContourRepresentation,vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Add a node at a specific world position. Returns 0 if the
   * node could not be added, 1 otherwise.
   */
  virtual int AddNodeAtWorldPosition( double x, double y, double z);
  virtual int AddNodeAtWorldPosition( double worldPos[3] );
  virtual int AddNodeAtWorldPosition( double worldPos[3],
                                      double worldOrient[9] );
  //@}

  //@{
  /**
   * Add a node at a specific display position. This will be
   * converted into a world position according to the current
   * constraints of the point placer. Return 0 if a point could
   * not be added, 1 otherwise.
   */
  virtual int AddNodeAtDisplayPosition( double displayPos[2] );
  virtual int AddNodeAtDisplayPosition( int displayPos[2] );
  virtual int AddNodeAtDisplayPosition( int X, int Y );
  //@}

  //@{
  /**
   * Given a display position, activate a node. The closest
   * node within tolerance will be activated. If a node is
   * activated, 1 will be returned, otherwise 0 will be
   * returned.
   */
  virtual int ActivateNode( double displayPos[2] );
  virtual int ActivateNode( int displayPos[2] );
  virtual int ActivateNode( int X, int Y );
  //@}

  // Descirption:
  // Move the active node to a specified world position.
  // Will return 0 if there is no active node or the node
  // could not be moved to that position. 1 will be returned
  // on success.
  virtual int SetActiveNodeToWorldPosition( double pos[3] );
  virtual int SetActiveNodeToWorldPosition( double pos[3],
                                            double orient[9] );

  //@{
  /**
   * Move the active node based on a specified display position.
   * The display position will be converted into a world
   * position. If the new position is not valid or there is
   * no active node, a 0 will be returned. Otherwise, on
   * success a 1 will be returned.
   */
  virtual int SetActiveNodeToDisplayPosition( double pos[2] );
  virtual int SetActiveNodeToDisplayPosition( int pos[2] );
  virtual int SetActiveNodeToDisplayPosition( int X, int Y );
  //@}

  //@{
  /**
   * Set/Get whether the active or nth node is selected.
   */
  virtual int ToggleActiveNodeSelected();
  virtual int GetActiveNodeSelected();
  virtual int GetNthNodeSelected(int);
  virtual int SetNthNodeSelected(int);
  //@}

  /**
   * Get the world position of the active node. Will return
   * 0 if there is no active node, or 1 otherwise.
   */
  virtual int GetActiveNodeWorldPosition( double pos[3] );

  /**
   * Get the world orientation of the active node. Will return
   * 0 if there is no active node, or 1 otherwise.
   */
  virtual int GetActiveNodeWorldOrientation( double orient[9] );

  /**
   * Get the display position of the active node. Will return
   * 0 if there is no active node, or 1 otherwise.
   */
  virtual int GetActiveNodeDisplayPosition( double pos[2] );

  /**
   * Get the number of nodes.
   */
  virtual int GetNumberOfNodes();

  /**
   * Get the nth node's display position. Will return
   * 1 on success, or 0 if there are not at least
   * (n+1) nodes (0 based counting).
   */
  virtual int GetNthNodeDisplayPosition( int n, double pos[2] );

  /**
   * Get the nth node's world position. Will return
   * 1 on success, or 0 if there are not at least
   * (n+1) nodes (0 based counting).
   */
  virtual int GetNthNodeWorldPosition( int n, double pos[3] );

  /**
   * Get the nth node.
   */
  virtual vtkContourRepresentationNode *GetNthNode(int n);

  /**
   * Get the nth node's world orientation. Will return
   * 1 on success, or 0 if there are not at least
   * (n+1) nodes (0 based counting).
   */
  virtual int GetNthNodeWorldOrientation( int n, double orient[9] );

  //@{
  /**
   * Set the nth node's display position. Display position
   * will be converted into world position according to the
   * constraints of the point placer. Will return
   * 1 on success, or 0 if there are not at least
   * (n+1) nodes (0 based counting) or the world position
   * is not valid.
   */
  virtual int SetNthNodeDisplayPosition( int n, int X, int Y );
  virtual int SetNthNodeDisplayPosition( int n, int pos[2] );
  virtual int SetNthNodeDisplayPosition( int n, double pos[2] );
  //@}

  //@{
  /**
   * Set the nth node's world position. Will return
   * 1 on success, or 0 if there are not at least
   * (n+1) nodes (0 based counting) or the world
   * position is not valid according to the point
   * placer.
   */
  virtual int SetNthNodeWorldPosition( int n, double pos[3] );
  virtual int SetNthNodeWorldPosition( int n, double pos[3],
                                       double orient[9] );
  //@}

  /**
   * Get the nth node's slope. Will return
   * 1 on success, or 0 if there are not at least
   * (n+1) nodes (0 based counting).
   */
  virtual int  GetNthNodeSlope( int idx, double slope[3] );

  // Descirption:
  // For a given node n, get the number of intermediate
  // points between this node and the node at
  // (n+1). If n is the last node and the loop is
  // closed, this is the number of intermediate points
  // between node n and node 0. 0 is returned if n is
  // out of range.
  virtual int GetNumberOfIntermediatePoints( int n );

  /**
   * Get the world position of the intermediate point at
   * index idx between nodes n and (n+1) (or n and 0 if
   * n is the last node and the loop is closed). Returns
   * 1 on success or 0 if n or idx are out of range.
   */
  virtual int GetIntermediatePointWorldPosition( int n,
                                                 int idx, double point[3] );

  /**
   * Add an intermediate point between node n and n+1
   * (or n and 0 if n is the last node and the loop is closed).
   * Returns 1 on success or 0 if n is out of range.
   */
  virtual int AddIntermediatePointWorldPosition( int n,
                                                 double point[3] );

  /**
   * Add an intermediate point between node n and n+1
   * (or n and 0 if n is the last node and the loop is closed).
   * Returns 1 on success or 0 if n is out of range. The added point is
   * assigned a ptId as supplied.
   */
  virtual int AddIntermediatePointWorldPosition( int n,
                               double point[3], vtkIdType ptId );

  /**
   * Delete the last node. Returns 1 on success or 0 if
   * there were not any nodes.
   */
  virtual int DeleteLastNode();

  /**
   * Delete the active node. Returns 1 on success or 0 if
   * the active node did not indicate a valid node.
   */
  virtual int DeleteActiveNode();

  /**
   * Delete the nth node. Return 1 on success or 0 if n
   * is out of range.
   */
  virtual int DeleteNthNode( int n );

  /**
   * Delete all nodes.
   */
  virtual void ClearAllNodes();

  /**
   * Given a specific X, Y pixel location, add a new node
   * on the contour at this location.
   */
  virtual int AddNodeOnContour( int X, int Y );

  //@{
  /**
   * The tolerance to use when calculations are performed in
   * display coordinates
   */
  vtkSetClampMacro(PixelTolerance,int,1,100);
  vtkGetMacro(PixelTolerance,int);
  //@}

  //@{
  /**
   * The tolerance to use when calculations are performed in
   * world coordinates
   */
  vtkSetClampMacro(WorldTolerance, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(WorldTolerance, double);
  //@}

  // Used to communicate about the state of the representation
  enum {
    Outside=0,
    Nearby
  };

  enum {
    Inactive = 0,
    Translate,
    Shift,
    Scale
  };

  //@{
  /**
   * Set / get the current operation. The widget is either
   * inactive, or it is being translated.
   */
  vtkGetMacro( CurrentOperation, int );
  vtkSetClampMacro( CurrentOperation, int,
                    vtkContourRepresentation::Inactive,
                    vtkContourRepresentation::Scale );
  void SetCurrentOperationToInactive()
    { this->SetCurrentOperation( vtkContourRepresentation::Inactive ); }
  void SetCurrentOperationToTranslate()
    { this->SetCurrentOperation( vtkContourRepresentation::Translate ); }
  void SetCurrentOperationToShift()
    {this->SetCurrentOperation( vtkContourRepresentation::Shift ); }
  void SetCurrentOperationToScale()
    {this->SetCurrentOperation( vtkContourRepresentation::Scale ); }
  //@}

  // Descirption:
  // Set / get the Point Placer. The point placer is
  // responsible for converting display coordinates into
  // world coordinates according to some constraints, and
  // for validating world positions.
  void SetPointPlacer( vtkPointPlacer * );
  vtkGetObjectMacro( PointPlacer, vtkPointPlacer );

  //@{
  /**
   * Set / Get the Line Interpolator. The line interpolator
   * is responsible for generating the line segments connecting
   * nodes.
   */
  void SetLineInterpolator( vtkContourLineInterpolator *);
  vtkGetObjectMacro( LineInterpolator, vtkContourLineInterpolator );
  //@}

  //@{
  /**
   * These are methods that satisfy vtkWidgetRepresentation's API.
   */
  void BuildRepresentation() VTK_OVERRIDE =0;
  int ComputeInteractionState(int X, int Y, int modified=0) VTK_OVERRIDE =0;
  void StartWidgetInteraction(double e[2]) VTK_OVERRIDE =0;
  void WidgetInteraction(double e[2]) VTK_OVERRIDE =0;
  //@}

  //@{
  /**
   * Methods required by vtkProp superclass.
   */
  void ReleaseGraphicsResources(vtkWindow *w) VTK_OVERRIDE =0;
  int RenderOverlay(vtkViewport *viewport) VTK_OVERRIDE =0;
  int RenderOpaqueGeometry(vtkViewport *viewport) VTK_OVERRIDE =0;
  int RenderTranslucentPolygonalGeometry(vtkViewport *viewport) VTK_OVERRIDE =0;
  int HasTranslucentPolygonalGeometry() VTK_OVERRIDE =0;
  //@}

  //@{
  /**
   * Set / Get the ClosedLoop value. This ivar indicates whether the contour
   * forms a closed loop.
   */
  void SetClosedLoop( int val );
  vtkGetMacro( ClosedLoop, int );
  vtkBooleanMacro( ClosedLoop, int );
  //@}

  //@{
  /**
   * A flag to indicate whether to show the Selected nodes
   * Default is to set it to false.
   */
  virtual void SetShowSelectedNodes(int);
  vtkGetMacro( ShowSelectedNodes, int );
  vtkBooleanMacro( ShowSelectedNodes, int );
  //@}

  /**
   * Get the points in this contour as a vtkPolyData.
   */
  virtual vtkPolyData* GetContourRepresentationAsPolyData() = 0;

  /**
   * Get the nodes and not the intermediate points in this
   * contour as a vtkPolyData.
   */
  void GetNodePolyData( vtkPolyData* poly );

  vtkSetMacro(RebuildLocator,bool);

protected:
  vtkContourRepresentation();
  ~vtkContourRepresentation() VTK_OVERRIDE;

  // Selection tolerance for the handles
  int    PixelTolerance;
  double WorldTolerance;

  vtkPointPlacer             *PointPlacer;
  vtkContourLineInterpolator *LineInterpolator;

  int ActiveNode;

  int CurrentOperation;
  int ClosedLoop;

  // A flag to indicate whether to show the Selected nodes
  int                   ShowSelectedNodes;

  vtkContourRepresentationInternals *Internal;

  void AddNodeAtPositionInternal( double worldPos[3],
                                  double worldOrient[9], int displayPos[2] );
  void AddNodeAtPositionInternal( double worldPos[3],
                                  double worldOrient[9], double displayPos[2] );
  void SetNthNodeWorldPositionInternal( int n, double worldPos[3],
                                        double worldOrient[9] );

  //@{
  /**
   * Given a world position and orientation, this computes the display position
   * using the renderer of this class.
   */
  void GetRendererComputedDisplayPositionFromWorldPosition( double worldPos[3],
                                    double worldOrient[9], int displayPos[2] );
  void GetRendererComputedDisplayPositionFromWorldPosition( double worldPos[3],
                                    double worldOrient[9], double displayPos[2] );
  //@}

  virtual void UpdateLines( int index );
  void UpdateLine( int idx1, int idx2 );

  virtual int FindClosestPointOnContour( int X, int Y,
                                 double worldPos[3],
                                 int *idx );

  virtual void BuildLines()=0;

  // This method is called when something changes in the point placer.
  // It will cause all points to be updated, and all lines to be regenerated.
  // It should be extended to detect changes in the line interpolator too.
  virtual int  UpdateContour();
  vtkTimeStamp ContourBuildTime;

  void ComputeMidpoint( double p1[3], double p2[3], double mid[3] )
  {
      mid[0] = (p1[0] + p2[0])/2;
      mid[1] = (p1[1] + p2[1])/2;
      mid[2] = (p1[2] + p2[2])/2;
  }

  /**
   * Build a contour representation from externally supplied PolyData. This
   * is very useful when you use an external program to compute a set of
   * contour nodes (let's say based on image features) and subsequently want
   * to build and display a contour that runs through those points.
   * This method is protected and accessible only from
   * vtkContourWidget::Initialize. The idlist here may be used to initialize
   * a contour widget that uses a vtkPolygonalSurfacePointPlacer. This stores
   * the point id's of the nodes, since the contour is drawn on the vertices
   * of a surface mesh.
   */
  virtual void Initialize( vtkPolyData *, vtkIdList *);

  /**
   * Overloaded initialize method, that is called when the vtkIdList is NULL
   * to mantain backwards compatibility.
   */
  virtual void Initialize( vtkPolyData *);

  /**
   * Internal implementation, delegated to another method, so that users who
   * override the method Initialize that takes in one argument are supported.
   */
  virtual void InitializeContour( vtkPolyData *, vtkIdList * );

  /**
   * Adding a point locator to the representation to speed
   * up lookup of the active node when dealing with large datasets (100k+)
   */
  vtkIncrementalOctreePointLocator *Locator;

  /**
   * Deletes the previous locator if it exists and creates
   * a new locator. Also deletes / recreates the attached data set.
   */
  void ResetLocator();

  void BuildLocator();

  bool RebuildLocator;


private:
  vtkContourRepresentation(const vtkContourRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkContourRepresentation&) VTK_DELETE_FUNCTION;
};

#endif

