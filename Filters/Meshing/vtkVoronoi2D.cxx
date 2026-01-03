// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkVoronoi2D.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDelaunay2D.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLocatorInterface.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTransform.h"
#include "vtkVoronoiCore2D.h"

#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkVoronoi2D);

//------------------------------------------------------------------------------
namespace // anonymous
{

// The struct DelTri is the ordered connectivity of a Delaunay traingle.  It
// is used when producing an output Delaunay triangulation. Note that unlike
// the topological coordinates, we do not sort the tuple because we want to
// preserve the winding order (consistent with the normal).
struct DelTri
{
  // Three ordered points defining a triangle.
  std::array<vtkIdType, 3> Ids;

  // The tuple (v0,v1,v2) is expected to be ordered. We presume v0 is the
  // id of the generating tile which produced the Delaunay triangle.
  DelTri(vtkIdType v0, vtkIdType v1, vtkIdType v2)
    : Ids{ v0, v1, v2 }
  {
  }

  // Copy constructor
  DelTri(const DelTri& tt) { this->Ids = tt.Ids; }
};                                      // DelTri
using DelTriType = std::vector<DelTri>; // Delaunay triangles

// Compositing information. Note that a lot of the information needed for
// composition is represented in the adjacency graph (e.g., the number of
// points/edges per output convex polygon).
struct Del2DCompositor
{
  vtkIdType NPts;         // The number of input point generators
  vtkIdType TotalNumPts;  // The total tile points produced across all tiles
  vtkIdType TotalNumTris; // The Delaunay triangles
  const int* Regions;     // Optional segmentation region ids

  // Constructors
  Del2DCompositor()
    : NPts(0)
    , TotalNumPts(0)
    , TotalNumTris(0)
    , Regions(nullptr)
  {
  }
  Del2DCompositor(const int* regions)
    : Del2DCompositor()
  {
    this->Regions = regions;
  }

  // Metadata needed for compositing.
  struct vtkCompositeInfo
  {
    // Initially these are "number of.." that are transformed to offsets
    // via a subsequent prefix sum operation.
    vtkIdType NumPts;  // number of points produced
    vtkIdType NumTris; // number of triangles produced
    vtkCompositeInfo()
      : NumPts(0)
      , NumTris(0)
    {
    }

    // Operator += provides support for prefix sum. Converts counts
    // to offsets.
    vtkCompositeInfo& operator+=(const vtkCompositeInfo& info)
    {
      this->NumPts += info.NumPts;
      this->NumTris += info.NumTris;
      return *this;
    }
  };

  /**
   * This singleton array captures global information necessary for
   * performing the compositing operation. vtkCompositeInformation is
   * a required type for gathering global information.
   */
  using vtkCompositeInformation = std::vector<vtkCompositeInfo>;
  vtkCompositeInformation Information;
  /**
   * Prepare to accumulate compositing information: specify the total
   * number of points to be processed. Also configure any singletons such
   * as compositing information.
   */
  void Initialize(vtkIdType numPts, Del2DCompositor* vtkNotUsed(comp))
  {
    this->NPts = numPts;
    this->Information.resize(numPts + 1);
  }
  /**
   * After threaded execution, perform final processing from the
   * compositing information. In this case, perform a prefix sum
   * to determine the total number of points. TODO: for very large
   * scale, the prefix sum could be threaded.
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
    this->TotalNumTris = totalInfo.NumTris;
  }

  /**
   * This is the data extracted from the tiles and accumulated by the thread
   * local data.
   */
  struct LocalData
  {
    vtkCompositeInformation* Info;         // singleton enables prefix sum compositing
    vtkVoronoiTileVertexType Points;       // coordinates defining the hull vertices
    vtkVoronoiTopoCoords2DType TopoCoords; // topological coordinates
    DelTriType Tris;                       // Delaunay triangles
    const int* Regions;

    LocalData()
      : Info(nullptr)
      , Regions(nullptr)
    {
      this->Points.reserve(1024);
      this->TopoCoords.reserve(1024);
      this->Tris.reserve(1024);
    }
    void Initialize(Del2DCompositor* c)
    {
      this->Info = &(c->Information);
      this->Regions = c->Regions;
    }
    /**
     * This method is called after the Voronoi tileis generated, so that
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

      // Gather the tile points and associated topological coordinates
      const int* regions = this->Regions;
      const PointRingType& points = tile.GetPoints();
      vtkIdType numTris = 0;
      for (int i = 0; i < numPts; ++i)
      {
        const vtkTilePoint& pL = points[i];
        const vtkTilePoint& pR = points[i == 0 ? numPts - 1 : i - 1];
        this->Points.emplace_back(pL.X);
        this->TopoCoords.emplace_back(pL.NeiId, pR.NeiId, ptId);
        if (ptId < pL.NeiId && ptId < pR.NeiId && // minimal id tri
          pL.NeiId >= 0 && pR.NeiId >= 0 &&       // all neighbors non-boundary
          (!regions ||
            (regions[pL.NeiId] >= 0 && // regions inside
              regions[pR.NeiId] >= 0)))
        {
          numTris++;
          this->Tris.emplace_back(ptId, pR.NeiId, pL.NeiId);
        }
      } // for all tile points

      (*this->Info)[ptId].NumPts = numPts;
      (*this->Info)[ptId].NumTris = numTris;
    } // AddData()
  };
}; // Del2DCompositor

// Classes for generating the Voronoi and Delaunay output data.
struct VOutput
{
  vtkVoronoiCore2D<Del2DCompositor, vtkVoronoiClassifier2D>* VC;
  vtkPointSet* Input;
  vtkPolyData* Output;
  const double* InPoints; // raw input points
  double* OutPoints;      // raw output points
  vtkVoronoi2D* Filter;
  const int* Regions; // optional segmentation labels

  const vtkMergeMapType* MergeMap; // used to merge points if requested
  vtkIdType NumMergedPts;          // the number of points after merging

  vtkIdType* Offsets;
  vtkIdType* Conn;

  bool PassPointData;
  int GeneratePointScalars;
  double* PointScalars;

  int GenerateCellScalars;
  vtkIdType* CellScalars;

  VOutput(vtkVoronoiCore2D<Del2DCompositor, vtkVoronoiClassifier2D>* vc, vtkPointSet* input,
    vtkVoronoi2D* filter, vtkMergeMapType* mergeMap, vtkIdType numMergedPts, vtkPolyData* output)
    : VC(vc)
    , Input(input)
    , Output(output)
    , InPoints(nullptr) // update in derived class
    , OutPoints(nullptr)
    , Filter(filter)
    , Regions(nullptr)
    , MergeMap(mergeMap)
    , NumMergedPts(numMergedPts)
    , Offsets(nullptr) // output polygons
    , Conn(nullptr)
    , PointScalars(nullptr)
    , CellScalars(nullptr)
  {
    this->InPoints = vc->GetPoints();

    this->Regions = vc->Classifier.Regions;

    this->PassPointData = filter->GetPassPointData();
    this->GeneratePointScalars = filter->GetGeneratePointScalars();
    this->GenerateCellScalars = filter->GetGenerateCellScalars();
  }

  // Optionally generate random numbers for cell scalars.
  vtkSMPThreadLocal<vtkVoronoiRandomColors> LocalGenerator;

  // Create the output cell scalar array.
  void CreateCellScalars(vtkIdType numCells, vtkPolyData* output)
  {
    vtkNew<vtkIdTypeArray> cellScalars;
    cellScalars->SetNumberOfComponents(1);
    cellScalars->SetName("Voronoi Cell Scalars");
    cellScalars->SetNumberOfTuples(numCells);
    int idx = output->GetCellData()->AddArray(cellScalars);
    output->GetCellData()->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
    this->CellScalars = cellScalars->GetPointer(0);
  }

  // Produce a cell attribute scalar. Recall that a generating point id
  // is also a Voronoi tile (cell) id.
  vtkIdType ProduceCellScalar(
    vtkIdType ptId, vtkIdType numSpokes, vtkIdType primId, vtkIdType threadId)
  {
    vtkIdType s = 0;
    switch (this->GenerateCellScalars)
    {
      case vtkVoronoi2D::POINT_IDS:
        s = ptId;
        break;
      case vtkVoronoi2D::REGION_IDS:
        s = (this->Regions ? this->Regions[ptId] : 0);
        break;
      case vtkVoronoi2D::NUMBER_SIDES:
        s = numSpokes;
        break;
      case vtkVoronoi2D::PRIM_IDS:
        s = primId;
        break;
      case vtkVoronoi2D::THREAD_IDS:
        s = threadId;
        break;
      case vtkVoronoi2D::RANDOM:
        auto& localGen = this->LocalGenerator.Local();
        // Make this repeatable, seed based on prim id
        localGen.Seed(primId);
        s = localGen.Next();
        break;
    }
    return s;
  } // Produce cell scalars

  // Create the output point scalar array.
  void CreatePointScalars(vtkIdType numPts, vtkPolyData* output)
  {
    vtkNew<vtkDoubleArray> pointScalars;
    pointScalars->SetNumberOfComponents(1);
    pointScalars->SetName("Voronoi Point Scalars");
    pointScalars->SetNumberOfTuples(numPts);
    int idx = output->GetPointData()->AddArray(pointScalars);
    output->GetPointData()->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
    this->PointScalars = pointScalars->GetPointer(0);
  }

  // Produce a point attribute scalar based on distance to the hull
  // generator. Typically this is only used when outputting a single Voronoi
  // flower / tile. Otherwise merged points have multiple possible scalar
  // values.
  double ProducePointScalar(vtkIdType ptId, double hullVertX[3])
  {
    const double* generatorX = this->InPoints + 3 * ptId;
    return (std::sqrt(vtkMath::Distance2BetweenPoints(generatorX, hullVertX)));
  }
}; // VOutput

// Used to ensure merged output points are only written once. This is
// important when point merging is enabled.
using PtsWrittenFlags = std::vector<unsigned char>;

// A class for outputting the Voronoi tessellation. Point numbering, defined
// by an optional merge map, may be required if point merging is enabled.
struct OutputVoronoi : public VOutput
{
  std::unique_ptr<PtsWrittenFlags> PtsWritten;
  double Z;

  OutputVoronoi(vtkVoronoiCore2D<Del2DCompositor, vtkVoronoiClassifier2D>* vc, vtkPointSet* input,
    vtkVoronoi2D* filter, vtkMergeMapType* mergeMap, vtkIdType numMergedPts, vtkPolyData* output)
    : VOutput(vc, input, filter, mergeMap, numMergedPts, output)
  {
    // Allocate some point merging related structure if necessary.
    if (this->MergeMap)
    {
      this->PtsWritten = std::unique_ptr<PtsWrittenFlags>(new PtsWrittenFlags(numMergedPts, 0));
    }

    this->InPoints = vc->GetPoints();
    this->Z = this->InPoints[2];
  }

  // Add a (non-merged) point to the output.
  void AddPoint(vtkIdType ptId, double* x)
  {
    double* p = this->OutPoints + 3 * ptId;
    *p++ = x[0];
    *p++ = x[1];
    *p = this->Z;
  }

  // Add a primitive cell (polygon) to the output. This should be followed by
  // AddPrimPoint() calls.
  void AddPrim(vtkIdType primId, vtkIdType connOffset) { this->Offsets[primId] = connOffset; }

  // Add a polygon cell point to the output. This assumes that no point
  // merging has occured.
  void AddPrimPoint(vtkIdType connOffset, vtkIdType pId) { this->Conn[connOffset] = pId; }

  // Add a merged point to the output.
  void AddMergedPoint(
    const vtkMergeMapType& mergeMap, PtsWrittenFlags& ptsWritten, vtkIdType ptId, double* x)
  {
    vtkIdType pId = mergeMap[ptId];
    if (!ptsWritten[pId])
    {
      double* p = this->OutPoints + 3 * pId;
      *p++ = x[0];
      *p++ = x[1];
      *p = this->Z;
      ptsWritten[pId] = 1;
    }
  }

  // Add a merged polygon cell point to the output.
  void AddMergedPrimPoint(const vtkMergeMapType& mergeMap, vtkIdType connOffset, vtkIdType ptId)
  {
    this->Conn[connOffset] = mergeMap[ptId];
  }

  // Produce Voronoi tiles by compositing local thread data.
  void operator()(vtkIdType threadId, vtkIdType endThreadId)
  {
    const vtkVoronoiBatchManager& batcher = this->VC->Batcher;
    const vtkVoronoiWheelsType& wheels = this->VC->Graph.GetWheels();
    vtkVoronoiAbortCheck abortCheck(threadId, endThreadId, this->Filter);

    bool mergePts = this->MergeMap ? true : false;
    const vtkMergeMapType& mergeMap = *(this->MergeMap);
    PtsWrittenFlags& ptsWritten = *(this->PtsWritten);

    for (; threadId < endThreadId; ++threadId)
    {
      if (abortCheck(threadId))
      {
        break;
      }

      // Get the current local thread data including the batches processed by
      // this thread.
      vtkVoronoi2DLocalData<Del2DCompositor, vtkVoronoiClassifier2D>& localData =
        *(this->VC->ThreadMap[threadId]);
      const Del2DCompositor& compositor = this->VC->Compositor;
      const Del2DCompositor::vtkCompositeInformation& info = compositor.Information;
      vtkVoronoiTileVertexType::iterator pItr = localData.Compositor.Points.begin();

      // Process all point batches in the current thread. Recall that
      // a batch consists of a set of contiguous point ids. Also recall
      // that the point id and the tile id are the same (i.e., for every
      // generating point, a tile is created).
      for (auto& batchId : localData.LocalBatches)
      {
        vtkIdType ptId, endPtId;
        batcher.GetBatchItemRange(batchId, ptId, endPtId);

        // Obtain the starting point id, and the total number of points in
        // the entire batch of points.
        vtkIdType startPtId = info[ptId].NumPts;
        vtkIdType numPts = info[endPtId].NumPts - info[ptId].NumPts;

        // Copy the local batch points into the global points
        vtkIdType pId = startPtId;
        if (!mergePts)
        {
          for (int i = 0; i < numPts; ++i, ++pId, ++pItr)
          {
            this->AddPoint(pId, pItr->X);
          }
        }
        else
        {
          for (int i = 0; i < numPts; ++i, ++pId, ++pItr)
          {
            this->AddMergedPoint(mergeMap, ptsWritten, pId, pItr->X);
          }
        }

        // Output the cell connectivity. Note that the cell point ids need to be
        // transformed into global point id space. Also output optional cell data.
        // Note that in 2D, each ptId creates a Voronoi cell.
        pId = startPtId;
        vtkIdType startConn = wheels[ptId];

        for (; ptId < endPtId; ++ptId)
        {
          this->AddPrim(ptId, wheels[ptId]);

          vtkIdType numEdges = this->VC->Graph.GetNumberOfSpokes(ptId);
          if (!mergePts)
          {
            for (int j = 0; j < numEdges; ++j)
            {
              this->AddPrimPoint(startConn++, pId++);
            }
          }
          else
          {
            for (int j = 0; j < numEdges; ++j)
            {
              this->AddMergedPrimPoint(mergeMap, startConn++, pId++);
            }
          }

          if (this->CellScalars)
          {
            this->CellScalars[ptId] = this->ProduceCellScalar(ptId, numEdges, ptId, threadId);
          } // if cell scalars
        }   // for points in this batch
      }     // for each batch in this thread
    }       // for all local thread data
  }         // operator()

  // Factory method to produce the Voronoi tessellation.
  static void Execute(vtkVoronoiCore2D<Del2DCompositor, vtkVoronoiClassifier2D>* vc,
    vtkPointSet* input, vtkPolyData* output)
  {
    // Grab some setup information
    vtkIdType NPts = vc->GetNumberOfPoints();
    Del2DCompositor& compositor = vc->Compositor;
    vtkIdType TotalPts = compositor.TotalNumPts;
    vtkIdType NumSpokes = vc->Graph.GetNumberOfSpokes();
    vtkVoronoi2D* filter = static_cast<vtkVoronoi2D*>(vc->Filter);

    // Composite the data into the global filter output. Depending on merging, create
    // a merge map.
    vtkNew<vtkPoints> outPts;
    outPts->SetDataTypeToDouble();

    std::unique_ptr<vtkVoronoiCore2D<Del2DCompositor, vtkVoronoiClassifier2D>::TopologicalMerge>
      topoMerge;
    vtkMergeMapType* mergeMap = nullptr;
    vtkIdType numMergedPts = 0;
    if (filter->GetMergePoints())
    {
      topoMerge =
        vtkVoronoiCore2D<Del2DCompositor, vtkVoronoiClassifier2D>::TopologicalMerge::Execute(vc);
      mergeMap = &(topoMerge->MergeMap);
      numMergedPts = topoMerge->GetNumberOfMergedPoints();
      outPts->SetNumberOfPoints(numMergedPts);
    }
    else
    {
      outPts->SetNumberOfPoints(TotalPts);
    }

    // Prepare to produce Voronoi output
    OutputVoronoi vout(vc, input, filter, mergeMap, numMergedPts, output);
    vout.OutPoints = vtkDoubleArray::FastDownCast(outPts->GetData())->GetPointer(0);

    // Structures for cell definitions. Directly create the offsets and
    // connectivity for efficiency.
    vtkNew<vtkIdTypeArray> connectivity;
    connectivity->SetNumberOfTuples(NumSpokes);
    vout.Conn = connectivity->GetPointer(0);

    vtkNew<vtkIdTypeArray> offsets;
    offsets->SetNumberOfTuples(NPts + 1);
    vout.Offsets = offsets->GetPointer(0);
    vout.Offsets[NPts] = NumSpokes;

    vtkNew<vtkCellArray> tiles;
    tiles->SetData(offsets, connectivity);

    // If requested, pass input point data as output cell data.
    if (vout.PassPointData)
    {
      output->GetCellData()->PassData(input->GetPointData());
    }

    // If requested, generate cell scalars
    if (vout.GenerateCellScalars)
    {
      vout.CreateCellScalars(NPts, output);
    }

    // Parallel copy the Voronoi-related local thread data (points, cells,
    // scalars) into the filter output.
    vtkIdType numThreads = vc->GetNumberOfThreads();
    vtkSMPTools::For(0, numThreads, vout);

    // Assemble the output
    output->SetPoints(outPts);
    output->SetPolys(tiles);
  } // Execute()

}; // OutputVoronoi

// An class for outputting the Delaunay triangulation.
struct OutputDelaunay : public VOutput
{
  OutputDelaunay(vtkVoronoiCore2D<Del2DCompositor, vtkVoronoiClassifier2D>* vc, vtkPointSet* input,
    vtkVoronoi2D* filter, vtkPolyData* output)
    : VOutput(vc, input, filter, nullptr, 0, output)
  {
  }

  // Produce Delaunay triangles by compositing local thread data.
  void operator()(vtkIdType threadId, vtkIdType endThreadId)
  {
    const vtkVoronoiBatchManager& batcher = this->VC->Batcher;
    vtkVoronoiAbortCheck abortCheck(threadId, endThreadId, this->Filter);

    for (; threadId < endThreadId; ++threadId)
    {
      if (abortCheck(threadId))
      {
        break;
      }

      vtkVoronoi2DLocalData<Del2DCompositor, vtkVoronoiClassifier2D>& localData =
        *(this->VC->ThreadMap[threadId]);
      const Del2DCompositor& compositor = this->VC->Compositor;
      const Del2DCompositor::vtkCompositeInformation& info = compositor.Information;
      DelTriType::iterator tItr = localData.Compositor.Tris.begin();

      // Process all batches in the current thread.
      for (auto& batchId : localData.LocalBatches)
      {
        vtkIdType ptId, endPtId;
        batcher.GetBatchItemRange(batchId, ptId, endPtId);
        vtkIdType triId = info[ptId].NumTris;
        vtkIdType totalTris = info[endPtId].NumTris - info[ptId].NumTris;
        vtkIdType* conn = this->Conn + (3 * triId);

        // Composite the triangles from the thread local data
        for (auto i = 0; i < totalTris; ++i, ++tItr, triId++)
        {
          *conn++ = tItr->Ids[0];
          *conn++ = tItr->Ids[1];
          *conn++ = tItr->Ids[2];

          if (this->CellScalars)
          {
            // Note that tri.Ids[0] is the originating Voronoi tile.
            // triId is the actual Deluanay triangle id.
            this->CellScalars[triId] = this->ProduceCellScalar(tItr->Ids[0], 3, triId, threadId);
          } // if cell scalars
        }   // for local triangles
      }     // for all batches in this thread output
    }       // for all thread output
  }         // operator()

  // Generate the Delaunay triangulation. This means compositing
  // the triangles found previously.
  static void Execute(vtkVoronoiCore2D<Del2DCompositor, vtkVoronoiClassifier2D>* vc,
    vtkPointSet* input, vtkPolyData* output)
  {
    // Grab some setup information
    Del2DCompositor& compositor = vc->Compositor;
    vtkIdType TotalTris = compositor.TotalNumTris;
    vtkVoronoi2D* filter = static_cast<vtkVoronoi2D*>(vc->Filter);

    // Setup for generating Delaunay output
    OutputDelaunay dout(vc, input, filter, output);

    // Generate triangle offsets and connectivity.
    vtkNew<vtkIdTypeArray> offsets;
    offsets->SetNumberOfTuples(TotalTris + 1);
    dout.Offsets = offsets->GetPointer(0);
    vtkIdType offset = (-3); // lambda increments then returns
    std::generate(dout.Offsets, dout.Offsets + (TotalTris + 1), [&] { return offset += 3; });

    vtkNew<vtkIdTypeArray> connectivity;
    connectivity->SetNumberOfTuples(TotalTris * 3);
    dout.Conn = connectivity->GetPointer(0);

    vtkNew<vtkCellArray> tris;
    tris->SetData(offsets, connectivity);

    // The Delaunay triangulation reuses the input points.
    // Pass the input point data if requested.
    if (dout.PassPointData)
    {
      output->GetPointData()->PassData(input->GetPointData());
    }

    // If requested, generate cell scalars
    if (dout.GenerateCellScalars)
    {
      dout.CreateCellScalars(TotalTris, output);
    }

    // Now populate the Delaunay triangles vtkCellArray.
    vtkIdType numThreads = vc->GetNumberOfThreads();
    vtkSMPTools::For(0, numThreads, dout);

    // Assemble the output
    output->SetPoints(input->GetPoints());
    output->SetPolys(tris);

  } // Execute()

}; // OutputDelaunay

// Produce debugging information if requested. Could be threaded
// if desired, but rarely used.
struct OutputFlower
{
  static void Execute(vtkIdType pointOfInterest, double tileX[3], vtkPolyData* vorOutput,
    vtkPolyData* flowerOutput, vtkPolyData* tileOutput, vtkSpheres* spheres)
  {
    // Make sure there is Voronoi data to operate on.
    if (!vorOutput || vorOutput->GetNumberOfCells() < pointOfInterest)
    {
      return;
    }

    // Populate a Voronoi tile with the output tile (PointOfInterest). This
    // assumes a single convex polygon has been output.
    double bds[6], x[3];
    vtkNew<vtkIdList> ptIds;

    vtkCellArray* tiles = vorOutput->GetPolys();
    tiles->GetCellAtId(pointOfInterest, ptIds);
    vtkVoronoiTile tile;
    tile.Initialize(pointOfInterest, tileX, vorOutput->GetPoints(), ptIds->GetNumberOfIds(),
      ptIds->GetPointer(0));

    // Bounding box from Circumflower. Radius is padded out a bit.
    double cf = 1.10 * sqrt(tile.GetCircumFlower2());
    bds[0] = tileX[0] - cf;
    bds[1] = tileX[0] + cf;
    bds[2] = tileX[1] - cf;
    bds[3] = tileX[1] + cf;
    bds[4] = tileX[2];
    bds[5] = tileX[2];

    // For now generate a zillion points and keep those that intersect the
    // tile.
    vtkNew<vtkPoints> fPts;
    fPts->SetDataTypeToDouble();
    vtkNew<vtkCellArray> fVerts;
    fVerts->InsertNextCell(1);
    vtkIdType i, pid;
    vtkVoronoiRandom01Range ran01Range;
    ran01Range.Seed(1177);
    vtkIdType npts;
    for (i = 0, npts = 0; i < 1000000; ++i)
    {
      x[0] = bds[0] + ran01Range.Next() * (bds[1] - bds[0]);
      x[1] = bds[2] + ran01Range.Next() * (bds[3] - bds[2]);
      x[2] = tileX[2];

      if (tile.InFlower(x))
      {
        pid = fPts->InsertNextPoint(x);
        fVerts->InsertCellPoint(pid);
        npts++;
      }
    }

    fVerts->UpdateCellCount(npts);
    flowerOutput->SetPoints(fPts);
    flowerOutput->SetVerts(fVerts);

    // Update tile polydata (fourth output)
    tile.ProducePolyData(tileOutput, spheres);
  }
}; // OutputFlower

} // anonymous namespace

//================= Begin VTK class proper =====================================
//------------------------------------------------------------------------------
// Construct object
vtkVoronoi2D::vtkVoronoi2D()
{
  this->OutputType = VORONOI; // Voronoi tessellation placed in output 0
  this->Validate = false;
  this->PassPointData = true;
  this->GeneratePointScalars = NO_POINT_SCALARS;
  this->GenerateCellScalars = NO_CELL_SCALARS;
  this->MergePoints = true;

  this->Padding = 0.001;
  this->Locator = vtkSmartPointer<vtkStaticPointLocator2D>::New();
  this->Locator->SetNumberOfPointsPerBucket(2);
  this->Transform = nullptr;
  this->ProjectionPlaneMode = XY_PLANE;
  this->PointOfInterest = (-1);
  // this->PointsOfInterest empty on instantiation
  this->MaximumNumberOfTileClips = VTK_ID_MAX;
  this->GenerateVoronoiFlower = false;
  this->Spheres = vtkSmartPointer<vtkSpheres>::New();
  this->BatchSize = 1000;
  this->PruneTolerance = 1.0e-13;

  this->NumberOfThreadsUsed = 0;
  this->MaximumNumberOfPoints = 0;
  this->NumberOfPrunes = 0;

  // Optional outputs 2-4 for 2) Delaunay triangulation, 3) Voronoi flower,
  // and 4) Voronoi tile at point of interest
  this->SetNumberOfOutputPorts(4);

  // By default process active point scalars to obtain region ids
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
}

//------------------------------------------------------------------------------
int vtkVoronoi2D::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPointSet* input = vtkPointSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDebugMacro(<< "Generating 2D Voronoi Tessellation");

  // Initialize; check input
  vtkIdType pId, numPts;
  vtkPoints* inPoints;
  if ((inPoints = input->GetPoints()) == nullptr || (numPts = inPoints->GetNumberOfPoints()) < 1)
  {
    vtkDebugMacro("Cannot tessellate; need at least 1 input point");
    return 1;
  }

  // If the user specified a transform, apply it to the input data.
  // Only the input points are transformed. Note points are always
  // converted to double.
  vtkSmartPointer<vtkPoints> tPoints;
  if (this->Transform)
  {
    tPoints = vtkSmartPointer<vtkPoints>::New();
    tPoints->SetDataTypeToDouble();
    this->Transform->TransformPoints(inPoints, tPoints);
  }
  else if (this->ProjectionPlaneMode == VTK_BEST_FITTING_PLANE)
  {
    // If the user asked this filter to compute the best fitting plane,
    // proceed to compute the plane and generate a transform that will
    // map the input points into that plane.
    this->SetTransform(vtkDelaunay2D::ComputeBestFittingPlane(input));
    tPoints = vtkSmartPointer<vtkPoints>::New();
    tPoints->SetDataTypeToDouble();
    this->Transform->TransformPoints(inPoints, tPoints);
  }
  else if (inPoints->GetDataType() == VTK_DOUBLE)
  { // fast path no conversion
    tPoints = inPoints;
  }
  else
  { // convert points to double
    tPoints = vtkSmartPointer<vtkPoints>::New();
    tPoints->SetDataTypeToDouble();
    tPoints->SetNumberOfPoints(numPts);
    for (pId = 0; pId < numPts; ++pId)
    {
      tPoints->SetPoint(pId, inPoints->GetPoint(pId));
    }
  }

  // Temporary data object holds points to be tessellated
  vtkNew<vtkPolyData> tInput;
  tInput->SetPoints(tPoints);

  // Optional second output (output #1) the Delaunay triangulation if
  // requested.
  vtkPolyData* delOutput = nullptr;
  if (this->OutputType == DELAUNAY || this->OutputType == VORONOI_AND_DELAUNAY)
  {
    vtkInformation* outInfo2 = outputVector->GetInformationObject(1);
    delOutput = vtkPolyData::SafeDownCast(outInfo2->Get(vtkDataObject::DATA_OBJECT()));
  }

  // A locator is used to locate closest points.
  if (!this->Locator)
  {
    vtkErrorMacro(<< "Point locator required\n");
    return 0;
  }
  this->Locator->SetDataSet(tInput);
  this->Locator->BuildLocator();
  this->Locator->StaticOn();

  // Computational bounds
  double length = tInput->GetLength();
  double padding = this->Padding * length;

  // Region ids can be used to control which input points are processed.
  // A region id < 0 means that the associated point is "outside" (or
  // background) and does not contribute to the output. We can use this
  // capability to process a specified "PointOfInterest" (if any). Otherwise,
  // we check the input for segmented regions via a regions ids array.
  //
  // If region ids are provided,  array must be a single component tuple,
  // signed integer of type vtkIntArray with the number of tuples == number
  // of input points. (Implementation note: this could be expanded with
  // templates - not sure its worth the object bloat.)
  vtkSmartPointer<vtkIntArray> regionIds;

  // Limit processing to points of interested if so specified.
  if ((this->PointOfInterest >= 0 && this->PointOfInterest < numPts) || this->PointsOfInterest)
  {
    regionIds = vtkSmartPointer<vtkIntArray>::New();
    regionIds->SetName("Points of Interest");
    regionIds->SetNumberOfTuples(numPts);
    vtkSMPTools::Fill(regionIds->GetPointer(0), regionIds->GetPointer(0) + numPts, -100);
    if (this->PointOfInterest >= 0)
    {
      regionIds->SetValue(this->PointOfInterest, numPts); // mark POI in region numPts
    }
    if (this->PointsOfInterest)
    {
      vtkIdType numPOI = this->PointsOfInterest->GetNumberOfTuples();
      for (vtkIdType i = 0; i < numPOI; ++i)
      {
        vtkIdType poi = this->PointsOfInterest->GetValue(i);
        if (poi >= 0 && poi < numPts)
        {
          regionIds->SetValue(poi, numPts); // mark POI in region numPts
        }
      }
    }
  }
  else
  {
    vtkDataArray* rIds = this->GetInputArrayToProcess(0, inputVector);
    regionIds = vtkIntArray::FastDownCast(rIds);
    if (rIds && !regionIds)
    {
      vtkWarningMacro("Region Ids array must be of type vtkIntArray");
      regionIds = ConvertRegionLabels(rIds);
    }
    if (regionIds)
    {
      if (regionIds->GetNumberOfComponents() > 1)
      {
        vtkErrorMacro("Region Ids must have 1 component");
        regionIds = nullptr;
      }
    }
  }

  // Simple speed test process the input points to produce tiles. No compositing is
  // performed. This is used for benchmarking / debugging. Note that includes
  // validation and pruning (if enabled).
  int outputType = this->GetOutputType();
  if (outputType == vtkVoronoi2D::SPEED_TEST)
  {
    auto speed =
      vtkVoronoiCore2D<vtkEmptyVoronoi2DCompositor, vtkEmptyVoronoi2DClassifier>::Execute(this,
        this->BatchSize, this->Locator, tPoints, padding, this->MaximumNumberOfTileClips,
        this->Validate, this->PruneTolerance, nullptr, nullptr);

    speed->UpdateExecutionInfo(
      this->NumberOfThreadsUsed, this->MaximumNumberOfPoints, this->NumberOfPrunes);
    return 1;
  }

  // Generate the 2D Voronoi tessellation.
  auto voro = vtkVoronoiCore2D<Del2DCompositor, vtkVoronoiClassifier2D>::Execute(this,
    this->BatchSize, this->Locator, tPoints, padding, this->MaximumNumberOfTileClips,
    this->Validate, this->PruneTolerance, nullptr, nullptr);

  voro->UpdateExecutionInfo(
    this->NumberOfThreadsUsed, this->MaximumNumberOfPoints, this->NumberOfPrunes);

  vtkDebugMacro(<< "Produced " << output->GetNumberOfCells() << " tiles and "
                << output->GetNumberOfPoints() << " points");

  // If requested, produce the Voronoi output.
  if (outputType == vtkVoronoi2D::VORONOI || outputType == vtkVoronoi2D::VORONOI_AND_DELAUNAY)
  {
    OutputVoronoi::Execute(voro.get(), tInput, output);
  } // Produce Voronoi output

  // If requested, produce the Delaunay output.
  if (outputType == vtkVoronoi2D::DELAUNAY || outputType == vtkVoronoi2D::VORONOI_AND_DELAUNAY)
  {
    OutputDelaunay::Execute(voro.get(), tInput, delOutput);
  } // Produce Voronoi output

  // If requested, sample the Voronoi flower and place it into
  // the third output.
  if (!this->CheckAbort() && this->GenerateVoronoiFlower && this->PointOfInterest >= 0 &&
    this->PointOfInterest < numPts)
  {

  } // Produce sampled Voronoi flower

  // If requested, sample the Voronoi flower and place it into
  // the third output. Create the debugging output (tile) for the
  // PointOfInterest and place it in the fourth output.
  if (!this->CheckAbort() && this->GenerateVoronoiFlower && this->PointOfInterest >= 0 &&
    this->PointOfInterest < numPts)
  {
    vtkInformation* outInfo3 = outputVector->GetInformationObject(2);
    vtkPolyData* flowerOutput =
      vtkPolyData::SafeDownCast(outInfo3->Get(vtkDataObject::DATA_OBJECT()));

    vtkInformation* outInfo4 = outputVector->GetInformationObject(3);
    vtkPolyData* tileOutput =
      vtkPolyData::SafeDownCast(outInfo4->Get(vtkDataObject::DATA_OBJECT()));

    double tileX[3];
    tPoints->GetPoint(this->PointOfInterest, tileX);
    OutputFlower::Execute(
      this->PointOfInterest, tileX, output, flowerOutput, tileOutput, this->Spheres.Get());
  } // Produce sampled Voronoi flower

  // Return the locator to a normal processing mode.
  this->Locator->StaticOff();

  return 1;
}

//------------------------------------------------------------------------------
vtkIdType vtkVoronoi2D::FindTile(double x[3])
{
  // Make sure the filter has executed (i.e., a locator is available), and the
  // request is within the bounding box of the input points.
  if (this->Locator == nullptr)
  {
    return (-1);
  }

  double bounds[6];
  this->Locator->GetBounds(bounds);
  if (x[0] < bounds[0] || x[0] > bounds[1] || x[1] < bounds[2] || x[1] > bounds[3])
  {
    return -1;
  }

  // Now simply request the closest point.
  return this->Locator->FindClosestPoint(x);
}

//------------------------------------------------------------------------------
void vtkVoronoi2D::GetTileData(vtkIdType tileId, vtkPolyData* tileData)
{
  // Initialize the tile polydata
  if (tileData)
  {
    tileData->Initialize();
  }
  else
  {
    return;
  }

  // Make sure the input is valid, a locator is available (i.e., the filter
  // has executed), and a Voronoi output has been produced.
  if (tileId < 0 || this->Locator == nullptr ||
    (this->OutputType != vtkVoronoi2D::VORONOI &&
      this->OutputType != vtkVoronoi2D::VORONOI_AND_DELAUNAY))
  {
    return;
  }

  // Get the output (this is output# 0).
  vtkPolyData* output = this->GetOutput();
  vtkPoints* vPts = output->GetPoints();
  vtkCellArray* vCells = output->GetPolys();

  // Define points. Reuse the locator's points.
  tileData->SetPoints(vPts);

  // Now grab the output tile
  vtkNew<vtkCellArray> tile;
  vtkNew<vtkIdList> pts;
  vCells->GetCellAtId(tileId, pts);
  tile->InsertNextCell(pts);
  tileData->SetPolys(tile);

  // Finally, copy the scalar cell (tile) data if created.
  vtkIdTypeArray* auxCellScalars =
    vtkIdTypeArray::FastDownCast(output->GetCellData()->GetArray("Voronoi Cell Scalars"));
  if (auxCellScalars)
  {
    vtkNew<vtkIdTypeArray> tileScalar;
    tileScalar->SetNumberOfComponents(1);
    tileScalar->SetNumberOfTuples(1);
    tileScalar->SetTuple1(0, auxCellScalars->GetComponent(tileId, 0));
    tileData->GetCellData()->SetScalars(tileScalar);
  }
}

//------------------------------------------------------------------------------
int vtkVoronoi2D::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  }
  else if (port == 1) // optional second input
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return 1;
}

//------------------------------------------------------------------------------
// Since users have access to the locator we need to take into account the
// locator's modified time.
vtkMTimeType vtkVoronoi2D::GetMTime()
{
  vtkMTimeType mTime = this->vtkObject::GetMTime();
  vtkMTimeType time = this->Locator->GetMTime();
  return (time > mTime ? time : mTime);
}

//------------------------------------------------------------------------------
void vtkVoronoi2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Output Type: " << this->OutputType << "\n";
  os << indent << "Validate: " << (this->Validate ? "On\n" : "Off\n");
  os << indent << "Pass Point Data: " << (this->PassPointData ? "On\n" : "Off\n");
  os << indent << "Generate Point Scalars: " << this->GeneratePointScalars << "\n";
  os << indent << "Generate Cell Scalars: " << this->GenerateCellScalars << "\n";
  os << indent << "Padding: " << this->Padding << "\n";
  os << indent << "Locator: " << this->Locator << "\n";
  os << indent << "Projection Plane Mode: " << this->ProjectionPlaneMode << "\n";
  os << indent << "Transform: " << (this->Transform ? "specified" : "none") << "\n";
  os << indent << "Point Of Interest: " << this->PointOfInterest << "\n";
  os << indent << "Points Of Interest: " << this->PointsOfInterest << "\n";
  os << indent << "Maximum Number Of Tile Clips: " << this->MaximumNumberOfTileClips << "\n";
  os << indent << "Generate Voronoi Flower: " << (this->GenerateVoronoiFlower ? "On\n" : "Off\n");
  os << indent << "Prune Tolerance: " << this->PruneTolerance << "\n";
  os << indent << "Batch Size: " << this->BatchSize << "\n";
}
VTK_ABI_NAMESPACE_END
