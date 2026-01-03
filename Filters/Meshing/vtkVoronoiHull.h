// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkVoronoiHull
 * @brief   provide core 3D Voronoi hull generation capabilities
 *
 * This lightweight, supporting class is used to generate a convex polyhedron
 * from repeated half-space clipping operations (i.e., generate a Voronoi
 * hull). It also keeps track of the Voronoi flower and circumflower (aka,
 * the radius of security). These are used to determine whether a clipping
 * operation will intersect the current polyhedron.
 *
 * The algorithm proceeds as follows. A generating point is placed within an
 * initial, convex bounding box (i.e., this is the starting Voronoi
 * hull). The hull is then repeatedly clipped by planes positioned at the
 * halfway points between neighboring points, with each plane's normal
 * pointing in the direction of the edge connecting the generator point to
 * the neighboring point.
 *
 * The Voronoi hull class is represented by points and faces. Each point
 * refers to the faces that intersected to produce it; each face refers to
 * the points that define it. Because these operations are dynamic, i.e.,
 * points and faces are created, modified, and deleted frequently, a simple,
 * built-in memory management system is used to reduce the performance impact
 * of repeated allocations and deletions. Also note that, because of this
 * dynamic processing, some of the points and faces may not be valid--make
 * sure that only points/faces whose ProcessingStatus are labeled "Valid"
 * are used.
 *
 * Tolerancing capabilities are built into this class. The relative
 * "PruneTolerance" is used to discard clipping nicks--that is, clipping
 * planes that barely intersect (i.e., graze) the hull. By pruning (or
 * discarding) small hull facets, the numerical stability of the hull
 * generation process is significantly improved. Note that the PruneTolerance
 * is *relative*, it is multiplied by a representative length of the hull;
 * therefore it is adaptive to hull size.
 *
 * @sa
 * vtkVoronoiCore3D vtkVoronoi3D vtkGeneralizedSurfaceNets3D vtkVoronoiTile
 * vtkVoronoiCore2D vtkVoronoi2D
 */

#ifndef vtkVoronoiHull_h
#define vtkVoronoiHull_h

#include "vtkDoubleArray.h"          // Support double points
#include "vtkFiltersMeshingModule.h" // For export macro
#include "vtkMath.h"                 // Euclidean norm
#include "vtkVoronoiCore.h"          // Enum: clip intersection status

#include <vector> // array support

VTK_ABI_NAMESPACE_BEGIN

class vtkPolyData;

/**
 * Clipping plane operations result in the dynamic deletion, modification,
 * and addition of points and faces. The status label keeps track of the
 * computational state of the points and faces that compose the
 * polyhedron. Deleted points and faces can be reused without allocating
 * memory.
 */
enum class ProcessingStatus
{
  Deleted = 0,
  Valid = 1
};

//======= Define the convex polyhedron class used to produce Voronoi hulls.

/**
 * Represent a point/vertex of the Voronoi polyhedron. This includes the
 * position, the evaluated value against the current clipping plane, and
 * radius**2 of the distance of the point to the generating point (i.e., the
 * Voronoi petal radius**2).  Note: the id of the point is implicitly known
 * from its position in the PointsArray. The Faces[3] are the three faces
 * (defined by the three nearby point generators) whose separation planes
 * intersect to produce the point. If more than three faces meet at the point
 * then the point is degenerate. Note the Status member: due to the memory
 * management processes, the Status will change. Make sure to process only
 * points and faces with Status==ProcessingStatus::Valid.
 */
struct VTKFILTERSMESHING_EXPORT vtkHullPoint
{
  double X[3];             // position
  double Val;              // evaluated value against a half-space clipping plane
  double R2;               // Voronoi petal radius
  int PtMap;               // renumber points that are actually used by valid faces
  int Faces[3];            // the three faces defining this point
  ProcessingStatus Status; // the status of the point

  // methods to define new point
  vtkHullPoint(double x, double y, double z)
    : X{ x, y, z }
    , Val(0)
    , R2(0.0)
    , PtMap(-1)
    , Faces{ -1, -1, -1 }
    , Status(ProcessingStatus::Valid)
  {
  }
  vtkHullPoint(double x[3])
    : X{ x[0], x[1], x[2] }
    , Val(0)
    , R2(0.0)
    , PtMap(-1)
    , Faces{ -1, -1, -1 }
    , Status(ProcessingStatus::Valid)
  {
  }

  // Replace a deleted point with a new point.
  void Replace(double x, double y, double z)
  {
    this->X[0] = x;
    this->X[1] = y;
    this->X[2] = z;
    this->Val = 0.0;
    this->R2 = 0.0;
    this->PtMap = (-1);
    this->Faces[0] = (-1);
    this->Faces[1] = (-1);
    this->Faces[2] = (-1);
    this->Status = ProcessingStatus::Valid;
  }
}; // vtkHullPoint

/**
 * Represent a face composing the polyhedron. A simple memory management
 * capability (to reduce new/delete) is baked into the struct, so make sure
 * on output to only process faces with Status==Valid.
 */
struct VTKFILTERSMESHING_EXPORT vtkHullFace
{
  vtkIdType NeiId;         // the neighboring generator point that produced this face
  int NumPts;              // the number of point ids defining this face
  int Points;              // the offset into FacePoints listing the point ids of this face
  int AllocSize;           // the number of slots allocated for the Points
  int InsertPos;           // the position to add a new point in the Points array
  ProcessingStatus Status; // the status of this face

  // Construct a polyhedron face
  vtkHullFace(vtkIdType neiId)
    : NeiId(neiId)
    , NumPts(0)
    , Points(-1)
    , AllocSize(0)
    , Status(ProcessingStatus::Valid)
  {
  }

  // Replace a deleted face with a new face. Allocated memory for point ids is
  // untouched.
  void Replace(vtkIdType neiPtId)
  {
    this->NeiId = neiPtId;
    this->Status = ProcessingStatus::Valid;
  }
}; // vtkHullFace

/**
 * Represent a polyhedral edge intersected by the current clipping plane.
 * The edge tuple (the edge's two end points) plus the id of the point of
 * intersection, and the two faces intersecting the edge, are retained. The
 * edge tuple assumes that the two vertex end points are ordered v0 < v1.
 *
 * Note that after intersecting the hull with a clipping plane, the list of
 * EdgeTuple intersection points form a new face. However, the intersection
 * points must be sorted around the perimeter of the face, hence requires
 * determing the loop index which orders the points into a face loop. The
 * loop index is computed by defining a coordinate system from the clip plane
 * normal, and the fan of diagonals connecting the points of the face loop.
 * This can be used to create new planes that cut the loop into pieces. By
 * counting the number of points on either side of these each plane, (due to
 * convecity) a loop index can be determined.
 */
struct vtkHullEdgeTuple
{
  vtkIdType V0;       // min edge vertex id
  vtkIdType V1;       // max edge vertex id
  vtkIdType Id;       // point id of inserted point
  vtkIdType LoopIdx;  // order of the point around the new face.
  vtkIdType Faces[2]; // the two faces using the edge.

  vtkHullEdgeTuple(vtkIdType v0, vtkIdType v1)
    : V0(v0)
    , V1(v1)
    , Id(-1)
    , LoopIdx(-1)
    , Faces{ -1, -1 }
  {
    if (this->V0 > this->V1)
    {
      std::swap(this->V0, this->V1);
    }
  }
  bool operator==(const vtkHullEdgeTuple& et) const
  {
    return this->V0 == et.V0 && this->V1 == et.V1;
  }
  bool operator!=(const vtkHullEdgeTuple& et) const
  {
    return this->V0 != et.V0 || this->V1 != et.V1;
  }
  bool IsEdge(vtkIdType v0, vtkIdType v1) const
  {
    if (v0 < v1) // ordered properly
    {
      return (this->V0 == v0 && this->V1 == v1);
    }
    else // swap comparison required
    {
      return (this->V0 == v1 && this->V1 == v0);
    }
  }
}; // vtkHullEdgeTuple

using PointsArray = std::vector<vtkHullPoint>; // the polyhedral points (and associated attributes)
using FacesArray = std::vector<vtkHullFace>;   // a list of polyhedral faces
using FacePointsArray = std::vector<int>;      // the list of points (by id) defining the faces
using FaceScratchArray = std::vector<int>;     // temporary face point ids array
using InsertedEdgePointsArray = std::vector<vtkHullEdgeTuple>; // collect edge intersection points

/**
 * The polyhedron class proper. Since it is a supporting class, it is
 * lightweight and not a subclass of vtkObject.
 */
class VTKFILTERSMESHING_EXPORT vtkVoronoiHull
{
public: // methods and data members purposely made public
  /**
   * Constructor. After instantiation, for each new point to process, make
   * sure to initialize the polyhedron with Initialize().
   */
  vtkVoronoiHull()
    : PtId(-1)
    , X{ 0, 0, 0 }
    , NumClips(0)
    , PruneTolerance(1.0e-13)
    , RecomputeCircumFlower(true)
    , RecomputePetals(true)
    , CircumFlower2(0.0)
    , MinRadius2(0.0)
    , MaxRadius2(0.0)
  {
    // Preallocate some space
    this->Points.reserve(256);
    this->Faces.reserve(256);
    this->FacePoints.reserve(2048);
    this->InProcessPoints.reserve(256);
    this->InProcessFaces.reserve(256);
    this->DeletedPoints.Reserve(256);
    this->DeletedFaces.Reserve(256);

    // Supporting data structures
    this->SortP.reserve(256);
    this->Petals = vtkSmartPointer<vtkDoubleArray>::New();
    this->Petals->SetNumberOfComponents(4); // x-y-z-R2
    this->Petals->Allocate(256);            // initial allocation
  }

  /**
   * Method to initiate the construction of the polyhedron. Define the
   * generator point id and its position, and an initial bounding box in
   * which to place the generator point.
   */
  void Initialize(vtkIdType genPtId, const double genPt[3], double bds[6]);

  /**
   * Method to clip the current convex polyhedron/hull with a plane defined
   * by a neighboring point. The neighbor id and its position must not be
   * coincident with the current generator point. This method *does not*
   * take into account the Voronoi circumflower and flower. The method
   * returns a clip intersection status.
   */
  ClipIntersectionStatus Clip(vtkIdType neiPtId, const double neiPt[3]);

  /**
   * Methods to determine whether a point x[3] is within the Voronoi
   * flower, or Voronoi circumflower. (The Voronoi flower is the union
   * of all Delaunay spheres located at the hull points. The Voronoi
   * circumflower is the 2*radius of the largest Delaunay sphere.) These
   * methods can be used to cull points which do not intersect the hull.
   */
  double GetCircumFlower2() { return this->CircumFlower2; }
  bool InCircumFlower(double r2) // radius**2 of point from generator
  {
    // Only recompute the circumflower if necessary; that is, when
    // a maximal point is eliminated by a polyhedral plane clip.
    if (this->RecomputeCircumFlower)
    {
      this->ComputeCircumFlower();
    }
    return (r2 <= this->CircumFlower2);
  }
  bool InFlower(const double x[3]);
  void UpdatePetals(double cf2);
  vtkDoubleArray* GetPetals()
  {
    if (this->RecomputePetals)
    {
      this->UpdatePetals(this->CircumFlower2);
    }
    return (this->Petals->GetNumberOfTuples() > 0 ? this->Petals : nullptr);
  }

  /**
   * Used to produce debugging output (e.g., generate vtkPolyData). It
   * numbers (i.e., maps) the points to global point ids.
   */
  void MapPoints();

  /**
   * Produce a vtkPolyData from the current polyhedron and one specified
   * face. This is typically for debugging purposes.
   */
  void ProduceFacePolyData(vtkPolyData* pd, vtkHullFace* face);

  /**
   * Produce a vtkPolyData from the current polyhedron. This is typically for
   * debugging purposes.
   */
  void ProducePolyData(vtkPolyData* pd);

  // Data members purposely left public for using classes to extract
  // information.

  // Information used to define the polyhedron- its generating point id and
  // position, plus region classification. Indicate whether degenerate faces
  // (i.e., those having ~zero area) can be deleted (i.e., pruned).
  vtkIdType PtId;        // Generating point id
  double X[3];           // Generating point position
  vtkIdType NumClips;    // The total number of clip operations since Initialize()
  double PruneTolerance; // Specify the prune tolerance

  // Support normal jitter in the case of degeneracies.
  vtkVoronoiRandom01Range Bumper;
  void BumpNormal(int bumpNum, double normal[3], double bumpNormal[3]);

  // These data members represent the constructed polyhedron.
  vtkIdType NumPts;   // The number of valid points in the points array
  PointsArray Points; // Array of points defining this polyhedron
  vtkIdType NumFaces; // The number of valid faces in the faces array
  FacesArray Faces;   // A list of faces forming this polyhedron

  /**
   * A homebrew stack with a preferred API. It is used to keep track of
   * points and faces that have been deleted and are eligible to be overwritten
   * by the addition of a new point or face.
   */
  struct DeletionStack
  {
    std::vector<int> Stack;
    DeletionStack() { this->Stack.reserve(256); }
    void Push(int id) { this->Stack.emplace_back(id); }
    int Pop()
    {
      vtkIdType id = this->Stack.back();
      this->Stack.pop_back();
      return id;
    }
    bool IsEmpty() { return (this->Stack.empty()); }
    void Clear() { this->Stack.clear(); }
    void Reserve(int sze) { this->Stack.reserve(sze); }
  }; // DeletionStack

  /**
   * Keep track of points and faces currently being operated on.
   */
  /**
   * Unique list of faces (by id) that require processing. Note
   * that we use a std::vector rather than a std::unordered_set
   * (or equivalent). This is because the number of faces to
   * process is usually small, and the unordered_set is much
   * slower than a vector.
   */
  struct FaceProcessingArray : public std::vector<int>
  {
    void AddFace(int faceId)
    {
      if (std::find(this->begin(), this->end(), faceId) == this->end())
      {
        this->emplace_back(faceId);
      }
    }
  };

  /**
   * Add points to be processed.
   */
  struct PointProcessingArray : public std::vector<int>
  {
    // Add a non-degenerate point, and connected faces, for
    // processing.
    void AddPoint(vtkVoronoiHull* hull, vtkHullPoint& point, int ptId)
    {
      this->emplace_back(ptId);
      hull->InProcessFaces.AddFace(point.Faces[0]);
      hull->InProcessFaces.AddFace(point.Faces[1]);
      hull->InProcessFaces.AddFace(point.Faces[2]);
    }
  };

  /**
   * Methods to add and delete polyhedron points.
   */
  vtkIdType AddNewPoint(double x, double y, double z);
  vtkIdType AddNewPoint(double x[3]) { return this->AddNewPoint(x[0], x[1], x[2]); }
  void DeletePoint(vtkIdType ptId);
  void SetPointFaces(vtkIdType pId, vtkIdType f0, vtkIdType f1, vtkIdType f2);

  /**
   * Methods to create, modify, and delete polyhedron faces.
   */
  int AddNewFace(vtkIdType npts, vtkIdType neiPtId);
  vtkHullFace* GetFace(int faceId) { return &(this->Faces[faceId]); }
  /**
   * Add a point id defining the current face. This method is called
   * after AddNewFace().
   */
  void AddFacePoint(vtkHullFace* face, vtkIdType ptId)
  {
    this->FacePoints[face->Points + face->InsertPos++] = ptId;
  }
  /**
   * Add the nth point id defining the current face. This method is called
   * after AddNewFace().
   */
  void AddNthFacePoint(vtkHullFace* face, int idx, vtkIdType ptId)
  {
    this->FacePoints[face->Points + idx] = ptId;
  }
  /**
   * Return the nth point id defining the current face.
   */
  int GetFacePoint(vtkHullFace* face, int ptNum) { return this->FacePoints[face->Points + ptNum]; }

  /**
   * After clipping, rebuild the face.
   */
  void RebuildFacePoints(vtkHullFace* face, FaceScratchArray& idsBuffer);

  /**
   * After a face clipping operation, characterize the face, and provide
   * information for subsequent processing. The method returns the number of
   * edge intersections; i.e. returns ==0 if the face should be deleted (all
   * points outside of the clip plane); returns ==2 if a convex clip is to be
   * performed; and >2 if a degenerate, non-convex clip is identified. In
   * most situations, convex clips are performed, and the method arguments
   * startIdx and numKeptPts are returned (identifying the points of the face
   * which are interior to the clip). When a non-convex clip is identified,
   * special treatment is necessary to address numerical degeneracies. (Note:
   * faces are never entirely inside the clip half space because they have
   * been tagged as InProcess, meaning they are attached to an outside
   * point.)
   */
  int EvaluateFace(vtkHullFace* face, int& startIdx, int& numKeptPts);

  /**
   * Delete a face from the polyhedron. To avoid memory thrashing (i.e.,
   * avoid new/delete), the face is simply marked deleted, and the deleted
   * face (and associated memory) will be reused in the future.
   */
  void DeleteFace(int faceId);

  /**
   * Given two point ids that form the edge of a polyhedron face, intersect
   * the edge to produce a new intersection point. The id of the intersection
   * point is returned.
   */
  int IntersectFaceEdge(int faceId, int p0, int p1);

  /**
   * Rebuild a convex, intersected face after a clipping operation. The
   * parameters startIdx and numKeptPts define a portion of the face loop
   * (i.e., the points that form the face) that, together with the two new
   * clip points, form the rebuilt, modified face. This method should only
   * be invoked on convex faces with exactly two edge intersections.
   */
  void RebuildFace(int faceId, int startIdx, int numKeptPts);

  /**
   * Internal memory operation to allocate space when adding
   * new points (due to a reabuild) which define a face.
   */
  void AllocatePointIds(int npts, vtkHullFace& face);

protected:
  /**
   * Internal data members
   */
  FacePointsArray FacePoints;  // Point ids used to define faces
  int MIN_POINTIDS_ALLOC = 10; // Minimum buffer allocation for face point ids

  // Keep track of deleted points and faces so their memory can be reused. This acts as a
  // sort of poor person's memory pool.
  DeletionStack DeletedPoints;
  DeletionStack DeletedFaces;

  // Used to process and track faces and points affected by a plane clip operation.
  PointProcessingArray InProcessPoints;       // Points affected by current clip operation
  FaceProcessingArray InProcessFaces;         // Faces affected by current clip operation
  InsertedEdgePointsArray InsertedEdgePoints; // New points generated on intersected edges
  FaceScratchArray FaceIdsBuffer; // An internal buffer used to rebuild faces after clipping

  // Indicate whether the Voronoi circumflower needs recomputing, and
  // keep track of the current circumflower and related information.
  void ComputeCircumFlower();
  bool RecomputeCircumFlower;
  bool RecomputePetals;
  double CircumFlower2;
  double MinRadius2;
  double MaxRadius2;
  std::vector<vtkHullPoint*> SortP;       // Points sorted on radius**2
  vtkSmartPointer<vtkDoubleArray> Petals; // Flower petals w/ radii > shell radius

  // Internal methods for processing clip operations of various types.
  /**
   * Empty out the polyhedron: clear memory but leave allocation
   * intact.
   */
  void Clear()
  {
    this->NumPts = 0;
    this->Points.clear();
    this->NumFaces = 0;
    this->Faces.clear();
    this->FacePoints.clear();
    this->DeletedPoints.Clear();
    this->DeletedFaces.Clear();
  }

  /**
   * The core geometric intersection operation. The method returns a clip
   * intersection status.
   */
  ClipIntersectionStatus IntersectWithPlane(double origin[3], double normal[3], vtkIdType neiPtId);
}; // vtkVoronoiHull

// In the following, inlined methods for performance

//------------------------------------------------------------------------------
inline void vtkVoronoiHull::SetPointFaces(vtkIdType pId, vtkIdType f0, vtkIdType f1, vtkIdType f2)
{
  this->Points[pId].Faces[0] = f0;
  this->Points[pId].Faces[1] = f1;
  this->Points[pId].Faces[2] = f2;
}

//------------------------------------------------------------------------------
inline vtkIdType vtkVoronoiHull::AddNewPoint(double x, double y, double z)
{
  vtkIdType id;
  // If there are no empty slots in the points array, then allocate a new point.
  if (this->DeletedPoints.IsEmpty())
  {
    id = this->Points.size();
    this->Points.emplace_back(x, y, z);
    this->Points.back().R2 = vtkMath::Distance2BetweenPoints(this->Points.back().X, this->X);
  }
  else // otherwise, replace a previously deleted point and reuse its memory.
  {
    id = this->DeletedPoints.Pop();
    this->Points[id].Replace(x, y, z);
    this->Points[id].R2 = vtkMath::Distance2BetweenPoints(this->Points[id].X, this->X);
  }

  this->NumPts++;
  return id;
}

//------------------------------------------------------------------------------
inline void vtkVoronoiHull::DeletePoint(vtkIdType ptId)
{
  // The circumflower may need recomputing if this point is an extreme point.
  if ((4 * this->Points[ptId].R2) >= this->CircumFlower2)
  {
    this->RecomputeCircumFlower = true;
  }

  this->DeletedPoints.Push(ptId);
  this->Points[ptId].Status = ProcessingStatus::Deleted;
  this->NumPts--;
}

//------------------------------------------------------------------------------
inline int vtkVoronoiHull::AddNewFace(vtkIdType npts, vtkIdType neiPtId)
{
  vtkIdType id;
  // If there are no empty slots in the faces array, than allocate a new face.
  if (this->DeletedFaces.IsEmpty())
  {
    id = this->Faces.size();
    this->Faces.emplace_back(neiPtId);
    this->AllocatePointIds(npts, this->Faces.back());
  }
  else
  {
    id = this->DeletedFaces.Pop();
    this->Faces[id].Replace(neiPtId);
    this->AllocatePointIds(npts, this->Faces[id]);
  }

  this->NumFaces++;
  return id;
}

//------------------------------------------------------------------------------
inline int vtkVoronoiHull::EvaluateFace(vtkHullFace* face, int& startIdx, int& numKeptPts)
{
  startIdx = numKeptPts = 0;
  int npts = face->NumPts, numEdgeInts = 0;
  int ip, p0, p1;
  double val0, val1;

  for (int i = 0; i < npts; ++i)
  {
    p0 = this->GetFacePoint(face, i);
    ip = ((i + 1) == npts ? 0 : (i + 1));
    p1 = this->GetFacePoint(face, ip);

    val0 = this->Points[p0].Val;
    val1 = this->Points[p1].Val;

    if (val0 <= 0.0)
    {
      numKeptPts++;
    }

    if (val0 > 0 && val1 <= 0)
    {
      numEdgeInts++;
      startIdx = i;
    }
    else if (val0 <= 0 && val1 > 0)
    {
      numEdgeInts++;
    }
  }

  return numEdgeInts;
}

//------------------------------------------------------------------------------
inline void vtkVoronoiHull::AllocatePointIds(int npts, vtkHullFace& face)
{
  face.NumPts = npts;
  face.InsertPos = 0;

  // See if allocation is necessary. Otherwise use previous.
  if (npts > face.AllocSize)
  {
    int size = (npts > MIN_POINTIDS_ALLOC ? npts : MIN_POINTIDS_ALLOC);
    vtkIdType offset = this->FacePoints.size();
    this->FacePoints.insert(this->FacePoints.end(), size, (-1));
    face.AllocSize = size;
    face.Points = offset;
  }
}

//------------------------------------------------------------------------------
inline void vtkVoronoiHull::RebuildFacePoints(vtkHullFace* face, FaceScratchArray& idsBuffer)
{
  // Make sure space has been allocated for rebuilt point ids.
  int npts = static_cast<int>(idsBuffer.size());
  if (npts > face->AllocSize)
  {
    // Need to realloc space for point ids.
    this->AllocatePointIds(npts, *face);
  }

  // Copy buffer into face point ids.
  for (face->InsertPos = 0; face->InsertPos < npts; ++face->InsertPos)
  {
    this->FacePoints[face->Points + face->InsertPos] = idsBuffer[face->InsertPos];
  }
  face->NumPts = npts;
}

//------------------------------------------------------------------------------
inline void vtkVoronoiHull::DeleteFace(int faceId)
{
  this->DeletedFaces.Push(faceId);
  this->Faces[faceId].Status = ProcessingStatus::Deleted;
  this->NumFaces--;
}

//------------------------------------------------------------------------------
inline void vtkVoronoiHull::ComputeCircumFlower()
{
  // Compute the circumflower, and compute some info about
  // the flower radii.
  this->MinRadius2 = VTK_FLOAT_MAX;
  this->MaxRadius2 = VTK_FLOAT_MIN;

  // Determine the circumflower and minimal sphere radius by
  // checking against each of the flower petals.
  for (const auto& pt : this->Points)
  {
    if (pt.Status == ProcessingStatus::Valid)
    {
      this->MinRadius2 = std::min(pt.R2, this->MinRadius2);
      this->MaxRadius2 = std::max(pt.R2, this->MaxRadius2);
    }
  }
  this->CircumFlower2 = (4.0 * this->MaxRadius2); // (2*(max petal radius))**2
  this->RecomputeCircumFlower = false;            // circumflower is up to date
}

//------------------------------------------------------------------------------
inline bool vtkVoronoiHull::InFlower(const double x[3])
{
  // Check against the flower petals
  for (const auto& pt : this->Points)
  {
    if (pt.Status == ProcessingStatus::Valid)
    {
      double r2 = vtkMath::Distance2BetweenPoints(x, pt.X);
      if (r2 <= pt.R2)
      {
        return true;
      }
    }
  } // for all valid points
  // Point not in the flower since it's not in any petals
  return false;
}

//------------------------------------------------------------------------------
inline void vtkVoronoiHull::MapPoints()
{
  // Renumber the output points. Note that associated Faces should use the
  // PtMap id to ensure the the point connectivity ids are contiguous.
  vtkIdType id = 0;
  for (auto& pitr : this->Points)
  {
    if (pitr.Status == ProcessingStatus::Valid)
    {
      pitr.PtMap = id++;
    }
  }
}

//------------------------------------------------------------------------------
// Update the flower petals which are passed off to the locator.
// Only petals which exend past the minimal radius of the shell
// request are added to the list of petals. It is presumed that
// UpdateCircumFlower() has been invoked previously.
inline void vtkVoronoiHull::UpdatePetals(double cf2)
{
  // If the radii of the flower spheres (petals) is highly variable (which
  // occurs when the spacing of points is highly variable), then there is
  // likely a lot of empty search space. Only add flower petals which extend
  // past the outer shell request boundary. These petals are used to further
  // limit the point search space.
  this->Petals->Reset();
  this->RecomputePetals = false; // petals will be updated in the following

  constexpr double SphereRatio = 2.0, SphereRatio2 = SphereRatio * SphereRatio;
  if (this->MinRadius2 > 0 && ((this->MaxRadius2 / this->MinRadius2) < SphereRatio2))
  {
    return; // it's not worth using the petals
  }

  // Empirically determined
  constexpr double LargeSphereRatio = 0.25;
  int maxLargeSpheres = LargeSphereRatio * this->NumPts;

  this->SortP.clear();
  double minR2 = VTK_FLOAT_MAX, maxR2 = VTK_FLOAT_MIN;
  for (auto& pt : this->Points)
  {
    if (pt.Status == ProcessingStatus::Valid)
    {
      // (2*R)**2 >= shell request radius**2
      if ((4 * pt.R2) >= cf2)
      {
        minR2 = std::min(pt.R2, minR2);
        maxR2 = std::max(pt.R2, maxR2);
        this->SortP.emplace_back(&pt);
      }
    }
  }

  if (static_cast<int>(this->SortP.size()) > maxLargeSpheres || (maxR2 / minR2) < SphereRatio2)
  {
    return; // it's not worth using the petals
  }
  else // sort from large spheres to small
  {
    std::sort(this->SortP.begin(), this->SortP.end(),
      [](const vtkHullPoint* p0, const vtkHullPoint* p1) { return (p0->R2 > p1->R2); });
    for (auto& pt : this->SortP)
    {
      this->Petals->InsertNextTuple4(pt->X[0], pt->X[1], pt->X[2], pt->R2);
    }
  } // it's worth using large sphere culling
} // UpdatePetals

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkVoronoiHull.h
