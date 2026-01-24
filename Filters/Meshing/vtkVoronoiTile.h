// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkVoronoiTile
 * @brief   provide core 2D Voronoi tile generation capabilities
 *
 * This lightweight, supporting class is used to generate a convex polygon
 * (or tile) from repeated half-space clipping operations (i.e., generate a
 * Voronoi tile). It also keeps track of the Voronoi flower and circumflower
 * (i.e., the radius of security). These are used to determine whether a
 * clipping operation will intersect the current Voronoi polygon.
 *
 * The algorithm proceeds as follows. A generating point is placed within an
 * initial, convex bounding box (i.e., this is the starting Voronoi
 * tile). The hull is then repeatedly clipped by lines positioned at the
 * halfway points between neighboring points, with each line's normal
 * pointing in the direction of the edge connecting the generator point to
 * a neighboring point.
 *
 * The Voronoi tile class is represented by a counterclockwise ordered list
 * of points. This also implicitly defines the Voronoi tile edges that form
 * the polygon. In addition, the neighboring point ids--those which generated
 * each polygon edge--are also maintained. This neighboring point information
 * enables the production of topological constructs such as the Voronoi
 * adjacency graph, which supports topological analysis capabilities.
 *
 * Tolerancing capabilities are built into this class. The relative
 * "PruneTolerance" is used to discard clipping nicks--that is, clipping
 * lines that barely intersect (i.e., graze) the tile. By pruning (or
 * discarding) small hull facets, the numerical stability of the tile
 * generation process is significantly improved. Note that the PruneTolerance
 * is *relative*, it is multiplied by a representative length of the tile;
 * therefore it is adaptive to tile size.
 *
 * @note
 * The tile is constructed in the x-y plane.
 *
 * @sa
 * vtkVoronoiHull vtkVoronoiCore2D vtkVoronoi2D vtkVoronoiCore3D vtkVoronoi3D
 * vtkGeneralizedSurfaceNets3D
 */

#ifndef vtkVoronoiTile_h
#define vtkVoronoiTile_h

#include "vtkDoubleArray.h"          // Support double points
#include "vtkFiltersMeshingModule.h" // For export macro
#include "vtkMath.h"                 // Euclidean norm
#include "vtkVoronoiCore.h"          // Enum: clip intersection status

#include <vector> // array support

VTK_ABI_NAMESPACE_BEGIN

class vtkPolyData;
class vtkSpheres;

//======= Define the convex polygon class used to produce Voronoi tiles.

// The data structure for representing a Voronoi tile vertex and implicitly,
// the connected Voronoi tile edge. The tile vertex has a position X, and the
// current value of the half-space clipping function. In the counterclockwise
// direction, the NeiId refers to the point id in the neighboring tile that,
// together with this tile's point id, produced a tile edge between the two
// points (i.e., a spoke).
struct vtkTilePoint
{
  double X[2];     // position of this vertex
  vtkIdType NeiId; // generating point id for the associated edge
  double Val;      // current value of the current half-space clipping function
  double R2;       // Radius**2 of circumcircle / flower petal

  vtkTilePoint(double tileX[2], double x[2], vtkIdType neiId)
    : X{ x[0], x[1] }
    , NeiId(neiId)
    , Val(0.0)
  {
    this->R2 = vtkMath::Distance2BetweenPoints2D(X, tileX);
  }

  vtkTilePoint(const vtkTilePoint& v) = default;
};

// Typedefs defined for convenience.
using PointRingType = std::vector<vtkTilePoint>;
using PointRingIterator = std::vector<vtkTilePoint>::iterator;

/**
 * The convex polygon tile class proper. Since it is a supporting class, it is
 * lightweight and not a subclass of vtkObject.
 */
class VTKFILTERSMESHING_EXPORT vtkVoronoiTile
{
public:
  /**
   * Constructor. After instantiation, for each point, make sure to
   * initialize the tile with Initialize().
   */
  vtkVoronoiTile()
    : PtId(-1)
    , X{ 0, 0, 0 } // z-component specifies location in z-plane
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
    this->NewPoints.reserve(256);

    // Supporting data structures
    this->SortP.reserve(256);
    this->Petals = vtkSmartPointer<vtkDoubleArray>::New();
    this->Petals->SetNumberOfComponents(3); // x-y-R2
    this->Petals->Allocate(256);            // initial allocation
  }

  /**
   * Method to initiate the construction of the polygon. Define the
   * generator point id and position, and an initial bounding box in
   * which to place the generator.
   */
  void Initialize(vtkIdType genPtId, const double genPt[3], double bds[4]);

  /**
   * Initialize with a convex polygon. The points must be in counterclockwise
   * order (normal in the z-direction). Points must not be coincident. The
   * polygon must be convex.
   */
  void Initialize(
    vtkIdType ptId, const double x[3], vtkPoints* pts, vtkIdType nPts, const vtkIdType* p);

  /**
   * Method to clip the current convex tile with a plane defined
   * by a neighboring point. The neighbor id and its position must not be
   * coincident with the current generator point. This method *does not*
   * take into account the Voronoi circumflower and flower.
   */
  ClipIntersectionStatus Clip(vtkIdType neiPtId, const double neiPt[2]);

  /**
   * Methods to determine whether a point x[2] is within the Voronoi
   * flower, or Voronoi circumflower. (The Voronoi flower is the union
   * of all Delaunay circles located at the tile points. The Voronoi
   * circumflower is the 2*radius of the largest Delaunay circle.) These
   * methods can be used to cull points which do not intersect the tile.
   */
  double GetCircumFlower2()
  {
    if (this->RecomputeCircumFlower)
    {
      this->ComputeCircumFlower();
    }
    return this->CircumFlower2;
  }
  bool InCircumFlower(double r2) // radius**2 of point from generator
  {
    // Only recompute the circumflower if necessary; that is, when
    // a maximal point is eliminated by a plane clip.
    if (this->RecomputeCircumFlower)
    {
      this->ComputeCircumFlower();
    }
    return (r2 <= this->CircumFlower2);
  }
  bool InFlower(const double x[2]);
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
   * Produce a vtkPolyData (and optional implicit function) from the current
   * polygon. If the spheres pointer is nullptr, it will not be generated.
   * This method is typically used for debugging purposes.
   */
  void ProducePolyData(vtkPolyData* pd, vtkSpheres* spheres);
  void ProducePolyData(vtkPolyData* pd) { this->ProducePolyData(pd, nullptr); }

  /**
   * Obtain information about the generated tile. Note that in 2D, the number of
   * points equals the number of convex polygon tile edges.
   */
  vtkIdType GetGeneratorPointId() { return this->PtId; }
  double* GetGeneratorPosition() { return this->X; }
  vtkIdType GetNumberOfPoints() { return this->Points.size(); }
  const PointRingType& GetPoints() { return this->Points; }

  // Data members purposely left public for using classes to extract
  // information.

  // Information used to define the polyhedron- its generating point id and
  // position, plus region classification. Indicate whether degenerate faces
  // (i.e., those having ~zero area) can be deleted (i.e., pruned).
  vtkIdType PtId;        // Generating point id
  double X[3];           // Generating point position. Note X[2] is z-plane position
  vtkIdType NumClips;    // The total number of clip operations since Initialize()
  double PruneTolerance; // Specify the prune tolerance

  // These data members represent the constructed polygon.
  PointRingType Points;    // counterclockwise ordered loop of points/vertices defining the tile
  PointRingType NewPoints; // accumulate new points/vertices to construct the tile

protected:
  // Convenience method for moving around the modulo ring of the tile
  // vertices.
  PointRingIterator Next(PointRingIterator& itr)
  {
    if (itr == (this->Points.end() - 1))
    {
      return this->Points.begin();
    }
    return (itr + 1);
  }

  // The core geometric intersection operation.
  ClipIntersectionStatus IntersectWithLine(double origin[2], double normal[2], vtkIdType neiPtId);

  // These tolerances are for managing degeneracies.
  double Tol;
  double Tol2;

  // Indicate whether the Voronoi circumflower needs recomputing, and
  // keep track of the current circumflower and related information.
  void ComputeCircumFlower();
  bool RecomputeCircumFlower;
  bool RecomputePetals;
  double CircumFlower2;
  double MinRadius2;
  double MaxRadius2;
  std::vector<vtkTilePoint*> SortP;       // Points sorted on radius**2
  vtkSmartPointer<vtkDoubleArray> Petals; // Flower petals w/ radii > annular radius

}; // vtkVoronoiTile

//------------------------------------------------------------------------------
inline void vtkVoronoiTile::ComputeCircumFlower()
{
  // Compute the circumflower, and compute some info about
  // the flower radii.
  this->MinRadius2 = VTK_FLOAT_MAX;
  this->MaxRadius2 = VTK_FLOAT_MIN;

  // Determine the circumflower and minimal sphere radius by
  // checking against each of the flower petals.
  for (const auto& p : this->Points)
  {
    double r2 = p.R2;
    this->MinRadius2 = std::min(r2, this->MinRadius2);
    this->MaxRadius2 = std::max(r2, this->MaxRadius2);
  }
  this->CircumFlower2 = (4.0 * this->MaxRadius2); // (2*(max petal radius))**2
  this->RecomputeCircumFlower = false;            // circumflower is up to date
}

//------------------------------------------------------------------------------
inline bool vtkVoronoiTile::InFlower(const double x[2])
{
  // Check against the flower petals
  for (const auto& p : this->Points)
  {
    double r2 = vtkMath::Distance2BetweenPoints2D(p.X, x);
    if (r2 <= p.R2)
    {
      return true;
    }
  }

  // Point not in the flower
  return false;
}

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkVoronoiTile.h
