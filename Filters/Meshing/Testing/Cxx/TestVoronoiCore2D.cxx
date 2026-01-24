// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkIntArray.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkStaticPointLocator.h"
#include "vtkTimerLog.h"
#include "vtkVoronoiCore2D.h"

#include <iostream> // Print output
#include <random>   // For random number generation

/**
 * This simple test program exercise the Voronoi generation and compositing
 * process.
 */
namespace // begin anonymous namespace
{
/**
 * Gather tile points, including topological coordinates, for
 * later compositing.
 */
struct vtkPointCompositor
{
  vtkIdType NPts;        // The number of point generators
  vtkIdType TotalNumPts; // The total points produced across all tiles.

  // Metadata needed for compositing
  struct vtkCompositeInfo
  {
    // Initially these are "number of.." that are transformed to offsets
    // via a subsequent prefix sum operation.
    vtkIdType NumPts; // number of points produced
    vtkCompositeInfo()
      : NumPts(0)
    {
    }

    // Operator += provides support for prefix sum. Converts counts
    // to offsets.
    vtkCompositeInfo& operator+=(const vtkCompositeInfo& info)
    {
      this->NumPts += info.NumPts;
      return *this;
    }
  };

  /**
   * This singleton array captures global information necessary for
   * performing the compositing operation. vtkCompositeInformation is
   * a required type for performing point merging.
   */
  using vtkCompositeInformation = std::vector<vtkCompositeInfo>;
  vtkCompositeInformation Information;
  /**
   * Prepare to accumulate compositing information: specify the total
   * number of points to be processed. Also configure any singletons such
   * as compositing information.
   */
  void Initialize(vtkIdType numPts, vtkPointCompositor* vtkNotUsed(comp))
  {
    this->NPts = numPts;
    this->Information.resize(numPts + 1);
  }
  /**
   * After threaded execution, perform final processing from the
   * compositing information. In this case, perform a prefix sum
   * to determine the total number of points.
   */
  void Finalize()
  {
    vtkCompositeInfo info, totalInfo;
    for (vtkIdType id = 0; id < this->NPts; ++id)
    {
      info = this->Information[id];
      this->Information[id] = totalInfo;
      totalInfo += info;
    }
    this->Information[this->NPts] = totalInfo;
    this->TotalNumPts = totalInfo.NumPts;
  }

  /**
   * This is the data extracted from tiles/hulls and accumulated by the thread
   * local data.
   */
  struct LocalData
  {
    vtkCompositeInformation* Info;         // singleton enables prefix sum compositing
    vtkVoronoiTileVertexType Points;       // coordinates defining the tile vertices
    vtkVoronoiTopoCoords2DType TopoCoords; // points in topological space
    LocalData()
    {
      this->Points.reserve(1024);
      this->TopoCoords.reserve(1024);
    }
    void Initialize(vtkPointCompositor* c) { this->Info = &(c->Information); }
    /**
     * This method is called after the Voronoi tile/hull is constructed, so that
     * compositing information can be extracted and recorded.
     */
    void AddData(
      vtkVoronoiTile& tile, int vtkNotUsed(numSpokes), const vtkVoronoiSpoke* vtkNotUsed(spokes))
    {
      // Generate output only if tile points exist
      vtkIdType ptId = tile.GetGeneratorPointId();
      int numPts = tile.GetNumberOfPoints();
      if (numPts <= 0)
      {
        return;
      }

      // Gather information about the points
      (*this->Info)[ptId].NumPts = numPts;

      // Gather the tile points and associated topological coordinates
      const PointRingType& points = tile.GetPoints();
      for (int i = 0; i < numPts; ++i)
      {
        const vtkTilePoint& pL = points[i];
        const vtkTilePoint& pR = points[i == 0 ? numPts - 1 : i - 1];
        this->Points.emplace_back(pL.X);
        this->TopoCoords.emplace_back(pL.NeiId, pR.NeiId, ptId);
      } // for all tile points
    }   // AddData()
  };
}; // vtkPointCompositor

// Use system <random> - create a simple convenience class. This generates
// random double values in the range [0,1).
struct vtkRandom01Range
{
  std::mt19937 RNG;
  std::uniform_real_distribution<double> Dist;
  vtkRandom01Range() { this->Dist.param(typename decltype(this->Dist)::param_type(0.0, 1.0)); }
  void Seed(vtkIdType s) { this->RNG.seed(s); }
  double Next() { return this->Dist(RNG); }
};

// Generate a set of random points
// Some default values
void GenerateRandomPoints(vtkPoints* points)
{
  vtkIdType npts = points->GetNumberOfPoints();

  // Create random points
  vtkSMPThreadLocal<vtkRandom01Range> LocalGenerator;
  vtkSMPTools::For(0, npts,
    [&](vtkIdType ptId, vtkIdType endPtId)
    {
      for (; ptId < endPtId; ++ptId)
      {
        auto& localGen = LocalGenerator.Local();
        localGen.Seed(ptId);
        double x = 2.0 * localGen.Next();
        double y = 2.0 * localGen.Next();
        double z = 0.0;
        points->SetPoint(ptId, x, y, z);
      }
    }); // end lambda
}

// Assign a region id
void AssignRegion(vtkIntArray* regions, int rid)
{
  vtkIdType npts = regions->GetNumberOfTuples();
  vtkSMPTools::Fill(regions->GetPointer(0), regions->GetPointer(0) + npts, rid);
}

} // anonymous namespace

int TestVoronoiCore2D(int, char*[])
{
  // Create a set of random points
  int NPts = 1000;
  unsigned int batchSize = 100;

  vtkNew<vtkPoints> points;
  points->SetDataTypeToDouble();
  points->SetNumberOfPoints(NPts);
  GenerateRandomPoints(points);

  // Assign all points to a specified region
  vtkNew<vtkIntArray> regions;
  regions->SetName("Point Regions");
  regions->SetNumberOfTuples(NPts);
  AssignRegion(regions, 0);

  // Instantiate and execute parallel Voronoi
  vtkIdType maxClips = VTK_INT_MAX;
  double pruneTol = 1.0e-13; // assuming double, with 15-16 bits of precision
  vtkAlgorithm* filter = nullptr;

  // Build a point locator
  vtkNew<vtkPolyData> polyData;
  polyData->SetPoints(points);

  double time = 0;
  vtkNew<vtkTimerLog> timer;

  vtkNew<vtkStaticPointLocator2D> loc;
  loc->SetDataSet(polyData);
  loc->BuildLocator();
  loc->StaticOn();

  // Computational bounds and the padded bounding box
  double length = polyData->GetLength();
  double padding = 0.001 * length;

  // All this does is process the input points to generate tiles. It is
  // effectively a speed test for Voronoi generation. Spokes are not
  // generated nor classified.
  loc->Modified();
  timer->StartTimer();
  loc->BuildLocator();

  std::unique_ptr<vtkVoronoiCore2D<vtkEmptyVoronoi2DCompositor, vtkEmptyVoronoi2DClassifier>> voro =
    vtkVoronoiCore2D<vtkEmptyVoronoi2DCompositor, vtkEmptyVoronoi2DClassifier>::Execute(
      filter, batchSize, loc, points, padding, maxClips, false, pruneTol, nullptr, nullptr);

  timer->StopTimer();
  time = timer->GetElapsedTime();
  std::cout << "Elapsed Time (Speed Test): " << time << endl;

  // Some output
  std::cout << "Num Threads: " << voro->GetNumberOfThreads() << endl;
  std::cout << "Max Tile Points: " << voro->GetMaximumNumberOfPoints() << endl;
  std::cout << "Max Tile Edges: " << voro->GetMaximumNumberOfPoints() << endl;
  std::cout << "Num Prunes: " << voro->GetNumberOfPrunes() << endl;
  std::cout << "Num Wheels: " << voro->GetAdjacencyGraph().GetNumberOfWheels() << endl;
  std::cout << "Num Spokes: " << voro->GetAdjacencyGraph().GetNumberOfSpokes() << endl;
  std::cout << endl;

  // Check destructor tear down.
  voro.reset();

  // This builds the adjacency graph.
  loc->Modified();
  timer->StartTimer();
  loc->BuildLocator();

  bool validate = false;
  vtkVoronoiClassifier2D classifier(regions->GetPointer(0));
  std::unique_ptr<vtkVoronoiCore2D<vtkEmptyVoronoi2DCompositor, vtkVoronoiClassifier2D>> voro1 =
    vtkVoronoiCore2D<vtkEmptyVoronoi2DCompositor, vtkVoronoiClassifier2D>::Execute(
      filter, batchSize, loc, points, padding, maxClips, validate, pruneTol, nullptr, &classifier);

  // Threaded computation of adjacency graph.
  voro1->Graph.Validate();

  timer->StopTimer();
  time = timer->GetElapsedTime();
  std::cout << "Elapsed Time (Adjacency Graph): " << time << endl;

  // Some output
  std::cout << "Num Threads: " << voro1->GetNumberOfThreads() << endl;
  std::cout << "Max Tile Points: " << voro1->GetMaximumNumberOfPoints() << endl;
  std::cout << "Max Tile Edges: " << voro1->GetMaximumNumberOfPoints() << endl;
  std::cout << "Num Prunes: " << voro1->GetNumberOfPrunes() << endl;
  std::cout << "Num Wheels: " << voro1->GetAdjacencyGraph().GetNumberOfWheels() << endl;
  std::cout << "Num Spokes: " << voro1->GetAdjacencyGraph().GetNumberOfSpokes() << endl;
  std::cout << endl;

  // Check destructor tear down.
  voro1.reset();

  // This builds the adjacency graph, and composites tile points, including
  // topologically merging the tile points.
  loc->Modified();
  timer->StartTimer();
  loc->BuildLocator();

  auto voro2 = vtkVoronoiCore2D<vtkPointCompositor, vtkVoronoiClassifier2D>::Execute(
    filter, batchSize, loc, points, padding, maxClips, validate, pruneTol, nullptr, &classifier);

  // Topologically merge the tile points to create a map to transform local point ids
  // into global point ids.
  vtkIdType numMergedPts = 0;
  auto mergeMap =
    vtkVoronoiCore2D<vtkPointCompositor, vtkVoronoiClassifier2D>::TopologicalMerge::Execute(
      voro2.get());

  timer->StopTimer();
  time = timer->GetElapsedTime();
  std::cout << "Elapsed Time (Merged Tile Points): " << time << endl;

  // Some output
  std::cout << "Num Threads: " << voro2->GetNumberOfThreads() << endl;
  std::cout << "Max Tile Points: " << voro2->GetMaximumNumberOfPoints() << endl;
  std::cout << "Total tile points: " << voro2->Compositor.TotalNumPts << endl;
  std::cout << "Total merged points: " << numMergedPts << endl;
  std::cout << "Max Tile Edges: " << voro2->GetMaximumNumberOfPoints() << endl;
  std::cout << "Num Prunes: " << voro2->GetNumberOfPrunes() << endl;
  std::cout << "Num Wheels: " << voro2->GetAdjacencyGraph().GetNumberOfWheels() << endl;
  std::cout << "Num Spokes: " << voro2->GetAdjacencyGraph().GetNumberOfSpokes() << endl;

  // Check destructor tear down.
  voro2.reset();

  return EXIT_SUCCESS;
}
