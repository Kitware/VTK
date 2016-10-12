/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDecimatePro.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDecimatePro
 * @brief   reduce the number of triangles in a mesh
 *
 * vtkDecimatePro is a filter to reduce the number of triangles in a triangle
 * mesh, forming a good approximation to the original geometry. The input to
 * vtkDecimatePro is a vtkPolyData object, and only triangles are treated. If
 * you desire to decimate polygonal meshes, first triangulate the polygons
 * with vtkTriangleFilter object.
 *
 * The implementation of vtkDecimatePro is similar to the algorithm
 * originally described in "Decimation of Triangle Meshes", Proc Siggraph
 * `92, with three major differences. First, this algorithm does not
 * necessarily preserve the topology of the mesh. Second, it is guaranteed to
 * give the a mesh reduction factor specified by the user (as long as certain
 * constraints are not set - see Caveats). Third, it is set up generate
 * progressive meshes, that is a stream of operations that can be easily
 * transmitted and incrementally updated (see Hugues Hoppe's Siggraph '96
 * paper on progressive meshes).
 *
 * The algorithm proceeds as follows. Each vertex in the mesh is classified
 * and inserted into a priority queue. The priority is based on the error to
 * delete the vertex and retriangulate the hole. Vertices that cannot be
 * deleted or triangulated (at this point in the algorithm) are
 * skipped. Then, each vertex in the priority queue is processed (i.e.,
 * deleted followed by hole triangulation using edge collapse). This
 * continues until the priority queue is empty. Next, all remaining vertices
 * are processed, and the mesh is split into separate pieces along sharp
 * edges or at non-manifold attachment points and reinserted into the
 * priority queue. Again, the priority queue is processed until empty. If
 * the desired reduction is still not achieved, the remaining vertices are
 * split as necessary (in a recursive fashion) so that it is possible to
 * eliminate every triangle as necessary.
 *
 * To use this object, at a minimum you need to specify the ivar
 * TargetReduction. The algorithm is guaranteed to generate a reduced mesh
 * at this level as long as the following four conditions are met: 1)
 * topology modification is allowed (i.e., the ivar PreserveTopology is off);
 * 2) mesh splitting is enabled (i.e., the ivar Splitting is on); 3) the
 * algorithm is allowed to modify the boundary of the mesh (i.e., the ivar
 * BoundaryVertexDeletion is on); and 4) the maximum allowable error (i.e.,
 * the ivar MaximumError) is set to VTK_DOUBLE_MAX.  Other important
 * parameters to adjust include the FeatureAngle and SplitAngle ivars, since
 * these can impact the quality of the final mesh. Also, you can set the
 * ivar AccumulateError to force incremental error update and distribution
 * to surrounding vertices as each vertex is deleted. The accumulated error
 * is a conservative global error bounds and decimation error, but requires
 * additional memory and time to compute.
 *
 * @warning
 * To guarantee a given level of reduction, the ivar PreserveTopology must
 * be off; the ivar Splitting is on; the ivar BoundaryVertexDeletion is on;
 * and the ivar MaximumError is set to VTK_DOUBLE_MAX.
 *
 * @warning
 * If PreserveTopology is off, and SplitEdges is off; the mesh topology may
 * be modified by closing holes.
 *
 * @warning
 * Once mesh splitting begins, the feature angle is set to the split angle.
 *
 * @sa
 * vtkDecimate vtkQuadricClustering vtkQuadricDecimation
*/

#ifndef vtkDecimatePro_h
#define vtkDecimatePro_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#include "vtkCell.h" // Needed for VTK_CELL_SIZE

class vtkDoubleArray;
class vtkPriorityQueue;

class VTKFILTERSCORE_EXPORT vtkDecimatePro : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkDecimatePro,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Create object with specified reduction of 90% and feature angle of
   * 15 degrees. Edge splitting is on, defer splitting is on, and the
   * split angle is 75 degrees. Topology preservation is off, delete
   * boundary vertices is on, and the maximum error is set to
   * VTK_DOUBLE_MAX. The inflection point ratio is 10 and the vertex
   * degree is 25. Error accumulation is turned off.
   */
  static vtkDecimatePro *New();

  //@{
  /**
   * Specify the desired reduction in the total number of polygons (e.g., if
   * TargetReduction is set to 0.9, this filter will try to reduce the data set
   * to 10% of its original size). Because of various constraints, this level of
   * reduction may not be realized. If you want to guarantee a particular
   * reduction, you must turn off PreserveTopology, turn on SplitEdges and
   * BoundaryVertexDeletion, and set the MaximumError to VTK_DOUBLE_MAX (these
   * ivars are initialized this way when the object is instantiated).
   */
  vtkSetClampMacro(TargetReduction,double,0.0,1.0);
  vtkGetMacro(TargetReduction,double);
  //@}

  //@{
  /**
   * Turn on/off whether to preserve the topology of the original mesh. If
   * on, mesh splitting and hole elimination will not occur. This may limit
   * the maximum reduction that may be achieved.
   */
  vtkSetMacro(PreserveTopology,int);
  vtkGetMacro(PreserveTopology,int);
  vtkBooleanMacro(PreserveTopology,int);
  //@}

  //@{
  /**
   * Specify the mesh feature angle. This angle is used to define what
   * an edge is (i.e., if the surface normal between two adjacent triangles
   * is >= FeatureAngle, an edge exists).
   */
  vtkSetClampMacro(FeatureAngle,double,0.0,180.0);
  vtkGetMacro(FeatureAngle,double);
  //@}

  //@{
  /**
   * Turn on/off the splitting of the mesh at corners, along edges, at
   * non-manifold points, or anywhere else a split is required. Turning
   * splitting off will better preserve the original topology of the
   * mesh, but you may not obtain the requested reduction.
   */
  vtkSetMacro(Splitting,int);
  vtkGetMacro(Splitting,int);
  vtkBooleanMacro(Splitting,int);
  //@}

  //@{
  /**
   * Specify the mesh split angle. This angle is used to control the splitting
   * of the mesh. A split line exists when the surface normals between
   * two edge connected triangles are >= SplitAngle.
   */
  vtkSetClampMacro(SplitAngle,double,0.0,180.0);
  vtkGetMacro(SplitAngle,double);
  //@}

  //@{
  /**
   * In some cases you may wish to split the mesh prior to algorithm
   * execution. This separates the mesh into semi-planar patches, which are
   * disconnected from each other. This can give superior results in some
   * cases. If the ivar PreSplitMesh ivar is enabled, the mesh is split with
   * the specified SplitAngle. Otherwise mesh splitting is deferred as long
   * as possible.
   */
  vtkSetMacro(PreSplitMesh,int);
  vtkGetMacro(PreSplitMesh,int);
  vtkBooleanMacro(PreSplitMesh,int);
  //@}

  //@{
  /**
   * Set the largest decimation error that is allowed during the decimation
   * process. This may limit the maximum reduction that may be achieved. The
   * maximum error is specified as a fraction of the maximum length of
   * the input data bounding box.
   */
  vtkSetClampMacro(MaximumError,double,0.0,VTK_DOUBLE_MAX);
  vtkGetMacro(MaximumError,double);
  //@}

  //@{
  /**
   * The computed error can either be computed directly from the mesh
   * or the error may be accumulated as the mesh is modified. If the error
   * is accumulated, then it represents a global error bounds, and the ivar
   * MaximumError becomes a global bounds on mesh error. Accumulating the
   * error requires extra memory proportional to the number of vertices in
   * the mesh. If AccumulateError is off, then the error is not accumulated.
   */
  vtkSetMacro(AccumulateError,int);
  vtkGetMacro(AccumulateError,int);
  vtkBooleanMacro(AccumulateError,int);
  //@}

  //@{
  /**
   * The MaximumError is normally defined as a fraction of the dataset bounding
   * diagonal. By setting ErrorIsAbsolute to 1, the error is instead defined
   * as that specified by AbsoluteError. By default ErrorIsAbsolute=0.
   */
  vtkSetMacro(ErrorIsAbsolute,int);
  vtkGetMacro(ErrorIsAbsolute,int);
  //@}

  //@{
  /**
   * Same as MaximumError, but to be used when ErrorIsAbsolute is 1
   */
  vtkSetClampMacro(AbsoluteError,double,0.0,VTK_DOUBLE_MAX);
  vtkGetMacro(AbsoluteError,double);
  //@}

  //@{
  /**
   * Turn on/off the deletion of vertices on the boundary of a mesh. This
   * may limit the maximum reduction that may be achieved.
   */
  vtkSetMacro(BoundaryVertexDeletion,int);
  vtkGetMacro(BoundaryVertexDeletion,int);
  vtkBooleanMacro(BoundaryVertexDeletion,int);
  //@}

  //@{
  /**
   * If the number of triangles connected to a vertex exceeds "Degree", then
   * the vertex will be split. (NOTE: the complexity of the triangulation
   * algorithm is proportional to Degree^2. Setting degree small can improve
   * the performance of the algorithm.)
   */
  vtkSetClampMacro(Degree,int,25,VTK_CELL_SIZE);
  vtkGetMacro(Degree,int);
  //@}

  //@{
  /**
   * Specify the inflection point ratio. An inflection point occurs
   * when the ratio of reduction error between two iterations is greater
   * than or equal to the InflectionPointRatio.
   */
  vtkSetClampMacro(InflectionPointRatio,double,1.001,VTK_DOUBLE_MAX);
  vtkGetMacro(InflectionPointRatio,double);
  //@}


  /**
   * Get the number of inflection points. Only returns a valid value after
   * the filter has executed.  The values in the list are mesh reduction
   * values at each inflection point. Note: the first inflection point always
   * occurs right before non-planar triangles are decimated (i.e., as the
   * error becomes non-zero).
   */
  vtkIdType GetNumberOfInflectionPoints();

  /**
   * Get a list of inflection points. These are double values 0 < r <= 1.0
   * corresponding to reduction level, and there are a total of
   * NumberOfInflectionPoints() values. You must provide an array (of
   * the correct size) into which the inflection points are written.
   */
  void GetInflectionPoints(double *inflectionPoints);

  /**
   * Get a list of inflection points. These are double values 0 < r <= 1.0
   * corresponding to reduction level, and there are a total of
   * NumberOfInflectionPoints() values. You must provide an array (of
   * the correct size) into which the inflection points are written.
   * This method returns a pointer to a list of inflection points.
   */
  double *GetInflectionPoints();

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  vtkSetMacro(OutputPointsPrecision,int);
  vtkGetMacro(OutputPointsPrecision,int);
  //@}

protected:
  vtkDecimatePro();
  ~vtkDecimatePro() VTK_OVERRIDE;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;

  double TargetReduction;
  double FeatureAngle;
  double MaximumError;
  double AbsoluteError;
  int ErrorIsAbsolute;
  int AccumulateError;
  double SplitAngle;
  int Splitting;
  int PreSplitMesh;
  int BoundaryVertexDeletion;
  int PreserveTopology;
  int Degree;
  double InflectionPointRatio;
  vtkDoubleArray *InflectionPoints;
  int OutputPointsPrecision;

  // to replace a static object
  vtkIdList *Neighbors;
  vtkPriorityQueue *EdgeLengths;

  void SplitMesh();
  int EvaluateVertex(vtkIdType ptId, unsigned short int numTris,
                     vtkIdType *tris, vtkIdType fedges[2]);
  vtkIdType FindSplit(int type, vtkIdType fedges[2], vtkIdType& pt1,
                      vtkIdType& pt2, vtkIdList *CollapseTris);
  int IsValidSplit(int index);
  void SplitLoop(vtkIdType fedges[2], vtkIdType& n1, vtkIdType *l1,
                 vtkIdType& n2, vtkIdType *l2);
  void SplitVertex(vtkIdType ptId,int type, unsigned short int numTris,
                   vtkIdType *tris, int insert);
  int CollapseEdge(int type, vtkIdType ptId, vtkIdType collapseId,
                   vtkIdType pt1, vtkIdType pt2, vtkIdList *CollapseTris);
  void DistributeError(double error);

  //
  // Special classes for manipulating data
  //
  // Special structures for building loops
  class LocalVertex
  {
  public:
    vtkIdType     id;
    double   x[3];
    double   FAngle;
  };
  typedef LocalVertex *LocalVertexPtr;

  class LocalTri
  {
  public:
    vtkIdType     id;
    double   area;
    double   n[3];
    vtkIdType     verts[3];
  };
  typedef LocalTri *LocalTriPtr;

  class VertexArray;
  friend class VertexArray;
  class VertexArray { //;prevent man page generation
  public:
    VertexArray(const vtkIdType sz)
      {this->MaxId = -1; this->Array = new LocalVertex[sz];};
    ~VertexArray()
    {
        delete [] this->Array;
    };
    vtkIdType GetNumberOfVertices() {return this->MaxId + 1;};
    void InsertNextVertex(LocalVertex& v)
      {this->MaxId++; this->Array[this->MaxId] = v;};
    LocalVertex& GetVertex(vtkIdType i) {return this->Array[i];};
    void Reset() {this->MaxId = -1;};

    LocalVertex *Array; // pointer to data
    vtkIdType MaxId;             // maximum index inserted thus far
  };

  class TriArray;
  friend class TriArray;
  class TriArray { //;prevent man page generation
  public:
    TriArray(const vtkIdType sz)
      {this->MaxId = -1; this->Array = new LocalTri[sz];};
    ~TriArray()
    {
        delete [] this->Array;
    };
    vtkIdType GetNumberOfTriangles() {return this->MaxId + 1;};
    void InsertNextTriangle(LocalTri& t)
      {this->MaxId++; this->Array[this->MaxId] = t;};
    LocalTri& GetTriangle(vtkIdType i) {return this->Array[i];};
    void Reset() {this->MaxId = -1;};

    LocalTri *Array;  // pointer to data
    vtkIdType MaxId;           // maximum index inserted thus far
  };

private:
  void InitializeQueue(vtkIdType numPts);
  void DeleteQueue();
  void Insert(vtkIdType id, double error= -1.0);
  int Pop(double &error);
  double DeleteId(vtkIdType id);
  void Reset();

  vtkPriorityQueue *Queue;
  vtkDoubleArray *VertexError;

  VertexArray *V;
  TriArray *T;

  // Use to be static variables used by object
  vtkPolyData *Mesh; //operate on this data structure
  double Pt[3];      //least squares plane point
  double Normal[3];  //least squares plane normal
  double LoopArea;   //the total area of all triangles in a loop
  double CosAngle;   //Cosine of dihedral angle
  double Tolerance;  //Intersection tolerance
  double X[3];       //coordinates of current point
  int NumCollapses; //Number of times edge collapses occur
  int NumMerges;    //Number of times vertex merges occur
  int Split;        //Controls whether and when vertex splitting occurs
  int VertexDegree; //Maximum number of triangles that can use a vertex
  vtkIdType NumberOfRemainingTris; //Number of triangles left in the mesh
  double TheSplitAngle; //Split angle
  int SplitState;   //State of the splitting process
  double Error;      //Maximum allowable surface error

private:
  vtkDecimatePro(const vtkDecimatePro&) VTK_DELETE_FUNCTION;
  void operator=(const vtkDecimatePro&) VTK_DELETE_FUNCTION;
};

#endif


