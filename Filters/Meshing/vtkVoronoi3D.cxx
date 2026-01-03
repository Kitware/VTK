// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkVoronoi3D.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLocatorInterface.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPolyData.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkStaticPointLocator.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTriangle.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVoronoiCore3D.h"

#include <random>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkVoronoi3D);

namespace // anonymous
{
// Utility classes to generate VTK filter output.

// Superclass for classes that produce VTK output.
struct VOutput
{
  vtkPointSet* Input;
  vtkVoronoi3D* Filter;
  const int* Regions; // optional segmentation labels

  vtkIdType* CellConn;    // Output cell connectivity
  vtkIdType* CellOffsets; // Output cell offsets into connectivity

  bool PassPointData;
  int GenerateCellScalars;
  vtkIdType* CellScalars;

  // Optionally generate random numbers for cell scalars.
  vtkSMPThreadLocal<vtkVoronoiRandomColors> LocalGenerator;

  VOutput(vtkPointSet* input, vtkVoronoi3D* filter)
    : Input(input)
    , Filter(filter)
    , Regions(nullptr)
    , CellConn(nullptr)
    , CellOffsets(nullptr)
    , CellScalars(nullptr)
  {
    // Manage attributes
    this->PassPointData = filter->GetPassPointData();
    this->GenerateCellScalars = filter->GetGenerateCellScalars();
  }

  // Add a primitive cell (line or face) to the output. This should be
  // followed by AddPrimPoint() calls.
  void AddPrim(vtkIdType primId, vtkIdType connOffset) { this->CellOffsets[primId] = connOffset; }

  // Add a primitive (line or face) cell point to the output.
  void AddPrimPoint(vtkIdType connOffset, vtkIdType pId) { this->CellConn[connOffset] = pId; }

  // Create the output cell scalar array.
  void CreateCellScalars(vtkIdType numCells, vtkPointSet* output)
  {
    vtkNew<vtkIdTypeArray> cellScalars;
    cellScalars->SetNumberOfComponents(1);
    cellScalars->SetName("Voronoi Cell Scalars");
    cellScalars->SetNumberOfTuples(numCells);
    int idx = output->GetCellData()->AddArray(cellScalars);
    output->GetCellData()->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
    this->CellScalars = cellScalars->GetPointer(0);
  }

  // Produce a cell attribute scalar
  vtkIdType ProduceCellScalar(vtkIdType ptId, vtkIdType numSpokes, vtkIdType primId,
    vtkIdType threadId, bool& firstRandomScalar)
  {
    vtkIdType s = 0;
    switch (this->GenerateCellScalars)
    {
      case vtkVoronoi3D::POINT_IDS:
        s = ptId;
        break;
      case vtkVoronoi3D::REGION_IDS:
        s = (this->Regions ? this->Regions[ptId] : 0);
        break;
      case vtkVoronoi3D::NUMBER_FACES:
        s = numSpokes;
        break;
      case vtkVoronoi3D::PRIM_IDS:
        s = primId;
        break;
      case vtkVoronoi3D::THREAD_IDS:
        s = threadId;
        break;
      case vtkVoronoi3D::RANDOM:
        auto& localGen = this->LocalGenerator.Local();
        // Make this repeatable, seed based on prim id
        if (firstRandomScalar)
        {
          localGen.Seed(primId);
          firstRandomScalar = false;
        }
        s = localGen.Next();
        break;
    }
    return s;
  } // Produce cell scalars

}; // VOutput

// Used to ensure merged output points are only written once.
using PtsWrittenFlags = std::vector<unsigned char>;

// This derived output class generates new points (merged or unmerged),
// and point attribute data.
struct PtsOutput : public VOutput
{
  const double* InPoints; // input points
  double* OutPoints;
  const vtkMergeMapType* MergeMap; // used to merge points if requested
  vtkIdType NumMergedPts;          // the number of points after merging
  int GeneratePointScalars;
  double* PointScalars;

  // Used for merging points. Ensure that points are only written once.
  std::unique_ptr<PtsWrittenFlags> PtsWritten;

  PtsOutput(
    vtkPointSet* input, vtkMergeMapType* mergeMap, vtkIdType numMergedPts, vtkVoronoi3D* filter)
    : VOutput(input, filter)
    , InPoints(nullptr) // update in derived class
    , OutPoints(nullptr)
    , MergeMap(mergeMap)
    , NumMergedPts(numMergedPts)
    , GeneratePointScalars(false)
    , PointScalars(nullptr)
  {
    // Allocate some point merging related structure if necessary.
    if (this->MergeMap)
    {
      this->PtsWritten = std::unique_ptr<PtsWrittenFlags>(new PtsWrittenFlags(numMergedPts, 0));
    }

    this->GeneratePointScalars = filter->GetGeneratePointScalars();
  }

  // Add a point to the output
  void AddPoint(double* pointScalars, vtkIdType genPtId, vtkIdType ptId, double* x)
  {
    double* p = this->OutPoints + 3 * ptId;
    *p++ = x[0];
    *p++ = x[1];
    *p = x[2];

    if (pointScalars)
    {
      pointScalars[ptId] = this->ProducePointScalar(genPtId, x);
    }
  }

  // Add a merged point to the output. We just write
  // the value of the first vertex hull point - it's possible
  // to average these conincident points - maybe if necessary.
  void AddMergedPoint(const vtkMergeMapType& mergeMap, PtsWrittenFlags& ptsWritten,
    double* pointScalars, vtkIdType genPtId, vtkIdType ptId, double* x)
  {
    vtkIdType pId = mergeMap[ptId];
    if (!ptsWritten[pId])
    {
      double* p = this->OutPoints + 3 * pId;
      *p++ = x[0];
      *p++ = x[1];
      *p = x[2];

      if (pointScalars)
      {
        pointScalars[pId] = this->ProducePointScalar(genPtId, x);
      }

      ptsWritten[pId] = 1;
    }
  }

  // Add a merged primitive cell point to the output
  void AddMergedPrimPoint(const vtkMergeMapType& mergeMap, vtkIdType connOffset, vtkIdType ptId)
  {
    vtkIdType pId = mergeMap[ptId];
    this->CellConn[connOffset] = pId;
  }

  // Create the output point scalar array.
  void CreatePointScalars(vtkIdType numPts, vtkPointSet* output)
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
  // flower / hull. Otherwise merged points have multiple possible scalar
  // values.
  double ProducePointScalar(vtkIdType ptId, double hullVertX[3])
  {
    const double* generatorX = this->InPoints + 3 * ptId;
    return (std::sqrt(vtkMath::Distance2BetweenPoints(generatorX, hullVertX)));
  }
}; // PtsOutput

// The vtkVoronoi3D class produces different outputs depending on how it is
// configured/instantiated. This means under the hood different Voronoi
// compositors and classifiers are used in combination with the templated
// vtkVoronoiCore3D class. In the following, these compositors and
// classifiers, as well as driver functions, are defined.

// ============================================================================
// For the OutputType == SPEED_TEST, the  filter simply generates all of the
// hulls produced from the input point generators. The pre-defined
// vtkEmptyVoronoi3DCompositor and vtkEmptyVoronoi3DClassifier are used.
// This is used to assess the raw speed of Voronoi tessellation. It does not
// produce any output.
void SpeedTestOutput(vtkVoronoi3D* filter, int batchSize, vtkStaticPointLocator* loc,
  vtkPoints* tPoints, double padding, vtkIdType maxClips, bool validate, double pruneTol,
  const int* regions)
{
  vtkEmptyVoronoi3DClassifier initializeClassifier(regions);
  auto voro = vtkVoronoiCore3D<vtkEmptyVoronoi3DCompositor, vtkEmptyVoronoi3DClassifier>::Execute(
    filter, batchSize, loc, tPoints, padding, maxClips, validate, pruneTol, nullptr,
    &initializeClassifier);

  voro->UpdateExecutionInfo(filter->NumberOfThreadsUsed, filter->MaximumNumberOfPoints,
    filter->MaximumNumberOfFaces, filter->NumberOfPrunes);
} // SpeedTestOutput()

// ============================================================================
// For the OutputType == ADJACENCY_GRAPH, the filter simply produces the
// connecting (forward) spokes between neighboring generator points. The
// input point generators are reused. A vtkPolyData composed of lines is
// output.
struct AGCompositor
{
  vtkIdType NPts;          // The number of point generators
  vtkIdType TotalNumLines; // The total lines produced across all hulls.

  // Metadata needed for compositing
  struct vtkCompositeInfo
  {
    // Initially these are "number of.." that are transformed to offsets
    // via a subsequent prefix sum operation.
    vtkIdType NumLines; // number of lines produced
    vtkCompositeInfo()
      : NumLines(0)
    {
    }

    // Operator += provides support for prefix sum. Converts counts
    // to offsets.
    vtkCompositeInfo& operator+=(const vtkCompositeInfo& info)
    {
      this->NumLines += info.NumLines;
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
  void Initialize(vtkIdType numPts, AGCompositor*)
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
    this->TotalNumLines = totalInfo.NumLines;
  }

  /**
   * This is the data extracted from the hulls and accumulated by the thread
   * local data.
   */
  struct LocalData
  {
    vtkCompositeInformation* Info;   // singleton enables prefix sum compositing
    vtkVoronoiCellConnType LineConn; // line connectivity ids
    LocalData() { this->LineConn.reserve(1024); }
    void Initialize(AGCompositor* c) { this->Info = &(c->Information); }
    /**
     * This method is called after the Voronoi tile/hull is generated, so that
     * compositing information can be extracted and recorded.
     */
    void AddData(vtkVoronoiHull& hull, int vtkNotUsed(numSpokes), const vtkVoronoiSpoke* spokes)
    {
      // Generate output only if hull faces (i.e., spokes) exist
      if (hull.NumFaces <= 0)
      {
        return;
      }

      // Determine the number of lines (i.e., spokes) produced by this hull
      int numOutputLines = 0;

      // Process all valid faces. Note that while the number of spokes is equal to
      // the number of valid faces, the hull Faces array may have invalid faces. Only
      // forward spokes which connect point generators are output.
      int spokeNum = 0;
      for (int faceId = 0; faceId < static_cast<int>(hull.Faces.size()); ++faceId)
      {
        // Note the 1:1 correspondance between spokes and valid faces
        vtkHullFace* face = hull.GetFace(faceId);
        if (face->Status == ProcessingStatus::Valid)
        {
          if ((spokes[spokeNum].Classification & vtkSpokeClassification::FORWARD_SPOKE) &&
            !(spokes[spokeNum].Classification & vtkSpokeClassification::DOMAIN_BOUNDARY))
          {
            numOutputLines++;
            this->LineConn.emplace_back(hull.PtId);
            this->LineConn.emplace_back(spokes[spokeNum].NeiId);
          } // specified face type matches
          spokeNum++;
        } // if valid face
      }   // for all hull faces

      // Record information about the collected data.
      (*this->Info)[hull.PtId].NumLines = numOutputLines;
    } // AddData()
  };
}; // AGCompositor

// Interface with vtkSMPTools to threaded generate the adjacency graph.
struct AGOutput : public VOutput
{
  const vtkVoronoiCore3D<AGCompositor, vtkVoronoiClassifier3D>* VC;
  AGOutput(const vtkVoronoiCore3D<AGCompositor, vtkVoronoiClassifier3D>* vc, vtkPointSet* input,
    vtkVoronoi3D* filter)
    : VOutput(input, filter)
    , VC(vc)
  {
  }

  // Each thread transforms and writes its own data.
  void operator()(vtkIdType threadId, vtkIdType endThreadId)
  {
    const vtkVoronoiBatchManager& batcher = this->VC->Batcher;
    vtkVoronoiAbortCheck abortCheck(threadId, endThreadId, this->Filter);
    // Loop over all threads
    for (; threadId < endThreadId; ++threadId)
    {
      if (abortCheck(threadId))
      {
        break;
      }

      // Get the current local thread data including the batches processed by
      // this thread.
      vtkVoronoi3DLocalData<AGCompositor, vtkVoronoiClassifier3D>& localData =
        *(this->VC->ThreadMap[threadId]);
      const AGCompositor& compositor = this->VC->Compositor;
      const AGCompositor::vtkCompositeInformation& info = compositor.Information;
      vtkVoronoiCellConnType::iterator cItr = localData.Compositor.LineConn.begin();

      // Loop over the batches that the current thread processed earlier. The batch
      // data is ordered and consistent with the local data.
      for (auto& batchId : localData.LocalBatches)
      {
        vtkIdType ptId, endPtId;
        batcher.GetBatchItemRange(batchId, ptId, endPtId);
        for (; ptId < endPtId; ++ptId) // output all data in this batch
        {
          bool firstRandomScalar = true;
          vtkIdType numLines = (info[ptId + 1].NumLines - info[ptId].NumLines);
          if (numLines > 0) // if hull produced lines
          {
            vtkIdType startLineId = info[ptId].NumLines;
            vtkIdType startConn = 2 * startLineId;
            vtkIdType lineId = startLineId;
            for (int i = 0; i < numLines; ++lineId, ++i)
            {
              this->AddPrim(lineId, startConn);
              this->AddPrimPoint(startConn++, *cItr++);
              this->AddPrimPoint(startConn++, *cItr++);
              if (this->CellScalars)
              {
                this->CellScalars[lineId] = this->ProduceCellScalar(
                  ptId, this->VC->Graph.Wheels[ptId], lineId, threadId, firstRandomScalar);
              } // if cell scalars
            }   // for all spokes generated from this hull
          }     // if any lines produced by hull
        }       // for all points in this batch
      }         // for all batches
    }           // for all threads
  }             // operator()

  // Driver function to output the adjacency graph. It operates on the classified spokes
  // and outputs interior, forward spokes.
  static void Execute(vtkVoronoi3D* filter, int batchSize, vtkStaticPointLocator* loc,
    vtkPoints* tPoints, double padding, vtkIdType maxClips, bool validate, double pruneTol,
    vtkPointSet* input, const int* regions, vtkPolyData* output)
  {
    vtkVoronoiClassifier3D initializeClassifier(regions);
    auto voro = vtkVoronoiCore3D<AGCompositor, vtkVoronoiClassifier3D>::Execute(filter, batchSize,
      loc, tPoints, padding, maxClips, validate, pruneTol, nullptr, &initializeClassifier);

    voro->UpdateExecutionInfo(filter->NumberOfThreadsUsed, filter->MaximumNumberOfPoints,
      filter->MaximumNumberOfFaces, filter->NumberOfPrunes);

    // Prepare to produce output
    AGOutput agout(voro.get(), input, filter);

    // Reuse the input points, and optionally the point data.
    output->SetPoints(tPoints);
    if (agout.PassPointData)
    {
      output->GetPointData()->PassData(agout.Input->GetPointData());
    }

    // Now generate the output lines. First gather some information.
    AGCompositor& compositor = voro->Compositor;
    vtkIdType NLines = compositor.TotalNumLines; // the total number of lines

    // Allocate cell array to hold lines.
    vtkNew<vtkIdTypeArray> conn;
    conn->SetNumberOfTuples(2 * NLines);
    agout.CellConn = conn->GetPointer(0);

    vtkNew<vtkIdTypeArray> offsets;
    offsets->SetNumberOfTuples(NLines + 1);
    agout.CellOffsets = offsets->GetPointer(0);
    agout.CellOffsets[NLines] = compositor.TotalNumLines = 2 * NLines; // cap off the offsets array

    // If requested, generate cell scalars
    if (agout.GenerateCellScalars)
    {
      agout.CreateCellScalars(NLines, output);
    }

    // Populate the connectivity and offsets array.
    vtkSMPTools::For(0, voro->GetNumberOfThreads(), agout);

    // Assemble the output
    vtkNew<vtkCellArray> lines;
    lines->SetData(offsets, conn);
    output->SetLines(lines);

  } // OutputAdjacencyGraph()
};  // AGOutput

// ============================================================================
// For the OutputType == DELAUNAY, the filter composites the topological
// coordinates (i.e., tetrahedral connectivity). The input point generators
// are reused (i.e., no new points are created). An unstructured grid of
// tetrahedra is output.
//
// Implementation note: there are two ways to generate the Delaunay
// tetrahedralization depending on whether Validate is enabled. If disabled,
// then DelOutput is used, a simple, relatively fast algorithm to produce the
// Delaunay tetrahedralization from the topological coordinates. If Validate
// is enabled, more work to performed to ensure that the Delaunay
// tetrahedralization is valid. The reason for this is that Validate==false
// presumes that there are no Delaunay degeneracies. In practice this means
// that the each topological coordinate must be complete, i.e., each of the
// four components of the subgraph represented by the topological coordinate
// is connected to the other three components of the subgraph. If
// Validate==true, then a sort is performed on the topological coordinates to
// determine if each subgraph is complete, and if not, the incomplete
// topological coordinates are used to build loops/faces and explicitly
// construct a Delaunay tetrahedralization.
struct DelCompositor
{
  vtkIdType NPts;         // The number of point generators
  vtkIdType TotalNumTets; // The total tetrahedra produced across all hulls.
  const int* Regions;     // Optional segmentation region ids
  DelCompositor()
    : NPts(0)
    , TotalNumTets(0)
    , Regions(nullptr)
  {
  }
  DelCompositor(const int* regions)
    : DelCompositor()
  {
    this->Regions = regions;
  }

  // Metadata needed for compositing
  struct vtkCompositeInfo
  {
    // Initially these are "number of.." that are transformed to offsets
    // via a subsequent prefix sum operation.
    vtkIdType NumTets; // number of tetrahedra produced
    vtkCompositeInfo()
      : NumTets(0)
    {
    }

    // Operator += provides support for prefix sum. Converts counts
    // to offsets.
    vtkCompositeInfo& operator+=(const vtkCompositeInfo& info)
    {
      this->NumTets += info.NumTets;
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
  void Initialize(vtkIdType numPts, DelCompositor* comp)
  {
    this->NPts = numPts;
    this->Information.resize(numPts + 1);
    this->Regions = comp->Regions;
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
    this->TotalNumTets = totalInfo.NumTets;
  }

  /**
   * This is the data extracted from the hulls and accumulated by the thread
   * local data.
   */
  struct LocalData
  {
    vtkCompositeInformation* Info;         // singleton enables prefix sum compositing
    vtkVoronoiTopoCoords3DType TopoCoords; // topological coordinates
    const int* Regions;
    LocalData()
      : Info(nullptr)
      , Regions(nullptr)
    {
      this->TopoCoords.reserve(1024);
    }
    void Initialize(DelCompositor* c)
    {
      this->Info = &(c->Information);
      this->Regions = c->Regions;
    }
    /**
     * This method is called after the Voronoi tile/hull is generated, so that
     * compositing information can be extracted and recorded.
     */
    void AddData(
      vtkVoronoiHull& hull, int vtkNotUsed(numSpokes), const vtkVoronoiSpoke* vtkNotUsed(spokes))
    {
      // Generate output only if hull faces (i.e., spokes) exist
      if (hull.NumPts <= 0)
      {
        return;
      }

      // Gather information about the points. Here we are just compositing
      // topological coordinates, which also define tetrahedra connectivity.
      vtkIdType ptId = hull.PtId;
      const int* regions = this->Regions;

      // Gather the valid hull points: the associated topological coordinates
      vtkIdType numTets = 0;
      for (const auto& pt : hull.Points)
      {
        if (pt.Status == ProcessingStatus::Valid)
        {
          // Get three neighbor point + current point defining a tetrahedron
          vtkIdType p0 = hull.GetFace(pt.Faces[0])->NeiId;
          vtkIdType p1 = hull.GetFace(pt.Faces[1])->NeiId;
          vtkIdType p2 = hull.GetFace(pt.Faces[2])->NeiId;

          // Gather all interior tetrahedra
          if (((ptId < p0 && p0 >= 0) && (ptId < p1 && p1 >= 0) &&
                (ptId < p2 && p2 >= 0)) && // minimal id test
            (!regions ||
              (regions[p0] >= 0 && // all regions labeled interior
                regions[p1] >= 0 && regions[p2] >= 0)))
          {
            this->TopoCoords.emplace_back(p0, p1, p2, ptId);
            numTets++;
          }

        } // if valid hull point
      }   // for all hull points
      (*this->Info)[ptId].NumTets = numTets;
    } // AddData()
  };
}; // DelCompositor

// Interface with vtkSMPTools to threaded generate the Delaunay triangulation.
// This functor interfaces with the merged topological coordinates.
struct DelOutput : public VOutput
{
  const vtkVoronoiCore3D<DelCompositor, vtkVoronoiClassifier3D>* VC;
  DelOutput(const vtkVoronoiCore3D<DelCompositor, vtkVoronoiClassifier3D>* vc, vtkPointSet* input,
    vtkVoronoi3D* filter)
    : VOutput(input, filter)
    , VC(vc)
  {
  }

  // Each thread transforms and writes its own data.
  void operator()(vtkIdType threadId, vtkIdType endThreadId)
  {
    const vtkVoronoiBatchManager& batcher = this->VC->Batcher;
    vtkVoronoiAbortCheck abortCheck(threadId, endThreadId, this->Filter);
    // Loop over all threads
    for (; threadId < endThreadId; ++threadId)
    {
      if (abortCheck(threadId))
      {
        break;
      }

      // Get the current local thread data including the batches processed by
      // this thread.
      vtkVoronoi3DLocalData<DelCompositor, vtkVoronoiClassifier3D>& localData =
        *(this->VC->ThreadMap[threadId]);
      const DelCompositor& compositor = this->VC->Compositor;
      const DelCompositor::vtkCompositeInformation& info = compositor.Information;
      vtkVoronoiTopoCoords3DType::iterator tItr = localData.Compositor.TopoCoords.begin();

      // Loop over the batches that the current thread processed earlier. The batches
      // are ordered and consistent with the local data vectors.
      for (auto& batchId : localData.LocalBatches)
      {
        vtkIdType ptId, endPtId;
        batcher.GetBatchItemRange(batchId, ptId, endPtId);
        for (; ptId < endPtId; ++ptId) // output all data in this batch
        {
          bool firstRandomScalar = true;
          vtkIdType numTets = (info[ptId + 1].NumTets - info[ptId].NumTets);
          if (numTets > 0) // if hull produced tets
          {
            vtkIdType startTetId = info[ptId].NumTets;
            vtkIdType startConn = 4 * startTetId;
            vtkIdType tetId = startTetId;
            for (int i = 0; i < numTets; ++tetId, ++tItr, ++i)
            {
              this->AddPrim(tetId, startConn);
              this->AddPrimPoint(startConn++, tItr->Ids[0]);
              this->AddPrimPoint(startConn++, tItr->Ids[1]);
              this->AddPrimPoint(startConn++, tItr->Ids[2]);
              this->AddPrimPoint(startConn++, tItr->Ids[3]);
              if (this->CellScalars)
              {
                this->CellScalars[tetId] = this->ProduceCellScalar(
                  ptId, this->VC->Graph.Wheels[ptId], tetId, threadId, firstRandomScalar);
              } // if cell scalars
            }   // for all tets generated from this hull
          }     // if any tets produced by hull
        }       // for all points in this batch
      }         // for all batches
    }           // for all threads
  }             // operator()

  // Driver function to output the Delaunay triangulation.
  static void Execute(vtkVoronoi3D* filter, int batchSize, vtkStaticPointLocator* loc,
    vtkPoints* tPoints, double padding, vtkIdType maxClips, bool validate, double pruneTol,
    vtkPointSet* input, const int* regions, vtkUnstructuredGrid* output)
  {
    DelCompositor initializeCompositor(regions);
    vtkVoronoiClassifier3D initializeClassifier(regions);
    auto voro = vtkVoronoiCore3D<DelCompositor, vtkVoronoiClassifier3D>::Execute(filter, batchSize,
      loc, tPoints, padding, maxClips, validate, pruneTol, &initializeCompositor,
      &initializeClassifier);

    voro->UpdateExecutionInfo(filter->NumberOfThreadsUsed, filter->MaximumNumberOfPoints,
      filter->MaximumNumberOfFaces, filter->NumberOfPrunes);

    // Prepare to produce output
    DelOutput delout(voro.get(), input, filter);

    // Placeholder for validated Delaunay
    // TODO: Validate Delaunay - sorting, building loops when degenerate,
    // and tesellating them.
    // ValidDelOutput()

    // Reuse the input point generators, and optionally the point data.
    output->SetPoints(tPoints);
    if (delout.PassPointData)
    {
      output->GetPointData()->PassData(delout.Input->GetPointData());
    }

    // Now access the composited information
    DelCompositor& compositor = voro->Compositor;
    vtkIdType NTets = compositor.TotalNumTets;

    // Allocate cell array to hold tets.
    vtkNew<vtkIdTypeArray> conn;
    conn->SetNumberOfTuples(4 * NTets);
    delout.CellConn = conn->GetPointer(0);

    vtkNew<vtkIdTypeArray> offsets;
    offsets->SetNumberOfTuples(NTets + 1);
    delout.CellOffsets = offsets->GetPointer(0);
    delout.CellOffsets[NTets] = 4 * NTets; // cap off the offsets array

    // If requested, generate cell scalars
    if (delout.GenerateCellScalars)
    {
      delout.CreateCellScalars(NTets, output);
    }

    // Populate the connectivity and offsets array.
    vtkSMPTools::For(0, voro->GetNumberOfThreads(), delout);

    // Assemble the output
    vtkNew<vtkCellArray> tets;
    tets->SetData(offsets, conn);
    output->SetCells(VTK_TETRA, tets);
  } // Execute()
};  // DelOutput

// ============================================================================
// For the output types BOUNDARY, POLYGONAL_COMPLEX, and SURFACE_NET, the
// filter composites hull points and faces, where the faces to be extraced
// depends on the specified face (i.e., spoke) classification. The points may
// be optionally topologically merged. A vtkPolyData of convex polygons is
// produced.
// These functions match the spoke / classification - a match produces a
// face. Implementation note: this could also be done with templates, just
// trying to avoid a little bloat, but templates might be slightly faster.
bool MatchesBoundary(unsigned char classification, bool)
{
  return (classification & vtkSpokeClassification::DOMAIN_BOUNDARY ? true : false);
}
bool MatchesPolygonalComplex(unsigned char classification, bool)
{
  return (classification == vtkSpokeClassification::FORWARD_SPOKE ||
        classification == vtkSpokeClassification::DOMAIN_BOUNDARY
      ? true
      : false);
}
bool MatchesSurfaceNet(unsigned char classification, bool capping)
{
  constexpr unsigned char FORWARD_BOUNDARY_SPOKE =
    vtkSpokeClassification::FORWARD_SPOKE | vtkSpokeClassification::REGION_BOUNDARY;
  return (classification == FORWARD_BOUNDARY_SPOKE ||
        (capping && classification == vtkSpokeClassification::DOMAIN_BOUNDARY)
      ? true
      : false);
}

struct SurfaceCompositor
{
  vtkIdType NPts;              // The number of point generators
  vtkIdType TotalNumPts;       // The total points produced across all hulls.
  vtkIdType TotalNumFaces;     // The total faces extracted from all hulls.
  vtkIdType TotalFaceConnSize; // The size of the faces connectivity array
  bool BoundaryCapping;        // whether to produce boundary surfaces
  std::function<bool(unsigned char classification, bool capping)> MatchesFaceType;

  // Constructors
  SurfaceCompositor()
    : NPts(0)
    , TotalNumPts(0)
    , TotalNumFaces(0)
    , TotalFaceConnSize(0)
    , BoundaryCapping(true)
    , MatchesFaceType(MatchesBoundary)
  {
  }
  SurfaceCompositor(int outputType, bool capping)
    : SurfaceCompositor()
  {
    if (outputType == vtkVoronoi3D::POLYGONAL_COMPLEX)
    {
      this->MatchesFaceType = MatchesPolygonalComplex;
    }
    else if (outputType == vtkVoronoi3D::SURFACE_NET)
    {
      this->MatchesFaceType = MatchesSurfaceNet;
    }
    else // outputType == vtkVoronoi3D::BOUNDARY
    {
      this->MatchesFaceType = MatchesBoundary;
    }
    this->BoundaryCapping = capping;
  }

  // Metadata needed for compositing
  struct vtkCompositeInfo
  {
    // Initially these are "number of.." that are transformed to offsets
    // via a subsequent prefix sum operation.
    vtkIdType NumPts;       // number of points produced
    vtkIdType NumFaces;     // number of polygon faces produced
    vtkIdType FaceConnSize; // face connectivity size
    vtkCompositeInfo()
      : NumPts(0)
      , NumFaces(0)
      , FaceConnSize(0)
    {
    }

    // Operator += provides support for prefix sum. Converts counts
    // to offsets.
    vtkCompositeInfo& operator+=(const vtkCompositeInfo& info)
    {
      this->NumPts += info.NumPts;
      this->NumFaces += info.NumFaces;
      this->FaceConnSize += info.FaceConnSize;
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
  void Initialize(vtkIdType numPts, SurfaceCompositor* comp)
  {
    this->NPts = numPts;
    this->Information.resize(numPts + 1);
    this->BoundaryCapping = comp->BoundaryCapping;
    this->MatchesFaceType = comp->MatchesFaceType;
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
    this->TotalNumFaces = totalInfo.NumFaces;
    this->TotalFaceConnSize = totalInfo.FaceConnSize;
  }

  /**
   * This is the data extracted from the hulls and accumulated by the thread
   * local data.
   */
  struct LocalData
  {
    vtkCompositeInformation* Info;         // singleton enables prefix sum compositing
    vtkVoronoiHullVertexType Points;       // coordinates defining the hull vertices
    vtkVoronoiTopoCoords3DType TopoCoords; // topological coordinates
    vtkVoronoiCellConnType FaceConn;       // cell face connectivity
    bool BoundaryCapping;
    std::function<bool(unsigned char classification, bool capping)> MatchesFaceType;

    LocalData()
    {
      this->Points.reserve(1024);
      this->TopoCoords.reserve(1024);
      this->FaceConn.reserve(1024);
    }
    void Initialize(SurfaceCompositor* c)
    {
      this->Info = &(c->Information);
      this->BoundaryCapping = c->BoundaryCapping;
      this->MatchesFaceType = c->MatchesFaceType;
    }
    /**
     * This method is called after the Voronoi tile/hull is generated, so that
     * compositing information can be extracted and recorded.
     */
    void AddData(vtkVoronoiHull& hull, int vtkNotUsed(numSpokes), const vtkVoronoiSpoke* spokes)
    {
      // Generate output only if hull faces (i.e., spokes) exist
      if (hull.NumPts <= 0)
      {
        return;
      }

      // Gather information about appropriately classified faces.
      vtkIdType ptId = hull.PtId;
      vtkIdType numOutputPts = 0;
      vtkIdType numOutputFaces = 0;
      vtkIdType connSize = 0;

      // Gather the valid hull points: the associated topological coordinates
      int spokeNum = 0;
      for (int faceId = 0; faceId < static_cast<int>(hull.Faces.size()); ++faceId)
      {
        // Note the 1:1 correspondance between spokes and valid faces
        vtkHullFace* face = hull.GetFace(faceId);
        if (face->Status == ProcessingStatus::Valid)
        {
          if (this->MatchesFaceType(spokes[spokeNum++].Classification, this->BoundaryCapping))
          {
            numOutputFaces++;
            int numFacePts = face->NumPts;
            this->FaceConn.emplace_back(numFacePts);
            connSize += numFacePts;

            for (int i = 0; i < numFacePts; ++i)
            {
              vtkHullPoint& p = hull.Points[hull.GetFacePoint(face, i)];
              if (p.PtMap < 0)
              {
                p.PtMap = numOutputPts++;
                this->Points.emplace_back(p.X);
                vtkIdType p0 = hull.GetFace(p.Faces[0])->NeiId;
                vtkIdType p1 = hull.GetFace(p.Faces[1])->NeiId;
                vtkIdType p2 = hull.GetFace(p.Faces[2])->NeiId;
                this->TopoCoords.emplace_back(p0, p1, p2, ptId);
              }
              this->FaceConn.emplace_back(p.PtMap);
            }
          } // specified face type matches
        }   // if valid hull point
      }     // for all hull points

      (*this->Info)[ptId].NumPts = numOutputPts;
      (*this->Info)[ptId].NumFaces = numOutputFaces;
      (*this->Info)[ptId].FaceConnSize = connSize;
    } // AddData()
  };
}; // SurfaceCompositor

// Interface with vtkSMPTools to threaded generate the Delaunay triangulation.
struct SurfaceOutput : public PtsOutput
{
  const vtkVoronoiCore3D<SurfaceCompositor, vtkVoronoiClassifier3D>* VC;

  SurfaceOutput(vtkVoronoiCore3D<SurfaceCompositor, vtkVoronoiClassifier3D>* vc, vtkPointSet* input,
    vtkMergeMapType* mergeMap, vtkIdType numMergedPts, vtkVoronoi3D* filter)
    : PtsOutput(input, mergeMap, numMergedPts, filter)
    , VC(vc)
  {
    this->InPoints = vc->GetPoints();
  }

  // Each thread transforms and writes its own data.
  void operator()(vtkIdType threadId, vtkIdType endThreadId)
  {
    // Set up for execution
    const vtkVoronoiBatchManager& batcher = this->VC->Batcher;
    vtkVoronoiAbortCheck abortCheck(threadId, endThreadId, this->Filter);

    bool mergePts = this->MergeMap ? true : false;
    const vtkMergeMapType& mergeMap = *(this->MergeMap);
    PtsWrittenFlags& ptsWritten = *(this->PtsWritten);

    // Loop over all threads
    for (; threadId < endThreadId; ++threadId)
    {
      if (abortCheck(threadId))
      {
        break;
      }

      // Get the current local thread data including the batches processed by
      // this thread.
      vtkVoronoi3DLocalData<SurfaceCompositor, vtkVoronoiClassifier3D>& localData =
        *(this->VC->ThreadMap[threadId]);
      const SurfaceCompositor& compositor = this->VC->Compositor;
      const SurfaceCompositor::vtkCompositeInformation& info = compositor.Information;
      vtkVoronoiHullVertexType::iterator pItr = localData.Compositor.Points.begin();
      vtkVoronoiCellConnType::iterator cItr = localData.Compositor.FaceConn.begin();

      // Loop over the batches that the current thread processed earlier. The batches
      // are ordered and consistent with the local data vectors.
      for (auto& batchId : localData.LocalBatches)
      {
        vtkIdType ptId, endPtId;
        batcher.GetBatchItemRange(batchId, ptId, endPtId);
        for (; ptId < endPtId; ++ptId) // output all data in this batch
        {
          vtkIdType numFaces = (info[ptId + 1].NumFaces - info[ptId].NumFaces);
          if (numFaces <= 0)
          {
            continue; // nothing to see here
          }
          vtkIdType numPts = (info[ptId + 1].NumPts - info[ptId].NumPts);
          vtkIdType startPtId = info[ptId].NumPts;
          vtkIdType startFaceId = info[ptId].NumFaces;
          vtkIdType startConn = info[ptId].FaceConnSize;

          // Output the points
          vtkIdType pId = startPtId;
          if (mergePts)
          {
            for (int i = 0; i < numPts; ++i, ++pId, ++pItr)
            {
              this->AddMergedPoint(mergeMap, ptsWritten, this->PointScalars, ptId, pId, pItr->X);
            }
          }
          else
          {
            for (int i = 0; i < numPts; ++i, ++pId, ++pItr)
            {
              this->AddPoint(this->PointScalars, ptId, pId, pItr->X);
            }
          }
          // Output the cell connectivity. Note that the cell point ids need to be
          // transformed into global point id space. Also output optional cell data.
          bool firstRandomScalar = true;
          vtkIdType faceId = startFaceId;
          for (int i = 0; i < numFaces; ++faceId, ++i)
          {
            this->AddPrim(faceId, startConn);
            vtkIdType numFacePts = *cItr++;
            if (mergePts)
            {
              for (int j = 0; j < numFacePts; ++j)
              {
                this->AddMergedPrimPoint(mergeMap, startConn++, startPtId + *cItr++);
              }
            }
            else
            {
              for (int j = 0; j < numFacePts; ++j)
              {
                this->AddPrimPoint(startConn++, startPtId + *cItr++);
              }
            }

            if (this->CellScalars)
            {
              this->CellScalars[faceId] = this->ProduceCellScalar(
                ptId, this->VC->Graph.Wheels[ptId], faceId, threadId, firstRandomScalar);
            } // if cell scalars
          }   // for all output cell primitives
        }     // for all points in this batch
      }       // for all batches
    }         // for all threads
  }           // operator()

  // Driver function for producing output surface primitives.
  static void Execute(int faceType, vtkVoronoi3D* filter, int batchSize, vtkStaticPointLocator* loc,
    vtkPoints* tPoints, double padding, vtkIdType maxClips, bool validate, double pruneTol,
    vtkPointSet* input, const int* regions, vtkPolyData* output)
  {
    SurfaceCompositor initializeCompositor(faceType, filter->GetBoundaryCapping());
    vtkVoronoiClassifier3D initializeClassifier(regions);
    auto voro = vtkVoronoiCore3D<SurfaceCompositor, vtkVoronoiClassifier3D>::Execute(filter,
      batchSize, loc, tPoints, padding, maxClips, validate, pruneTol, &initializeCompositor,
      &initializeClassifier);

    voro->UpdateExecutionInfo(filter->NumberOfThreadsUsed, filter->MaximumNumberOfPoints,
      filter->MaximumNumberOfFaces, filter->NumberOfPrunes);

    // Now access the composited information
    SurfaceCompositor& compositor = voro->Compositor;
    vtkIdType NumPts = compositor.TotalNumPts;
    vtkIdType NumFaces = compositor.TotalNumFaces;
    vtkIdType FaceConnSize = compositor.TotalFaceConnSize;

    // If point merging is enabled, create a toopological merge map
    vtkNew<vtkPoints> outPts;
    outPts->SetDataTypeToDouble();
    std::unique_ptr<vtkVoronoiCore3D<SurfaceCompositor, vtkVoronoiClassifier3D>::TopologicalMerge>
      topoMerge;

    vtkMergeMapType* mergeMap = nullptr;
    vtkIdType numMergedPts = 0;
    if (filter->GetMergePoints())
    {
      topoMerge =
        vtkVoronoiCore3D<SurfaceCompositor, vtkVoronoiClassifier3D>::TopologicalMerge::Execute(
          voro.get());
      mergeMap = &(topoMerge->MergeMap);

      numMergedPts = NumPts = topoMerge->GetNumberOfMergedPoints();
      outPts->SetNumberOfPoints(numMergedPts);
    }
    else
    {
      outPts->SetNumberOfPoints(NumPts);
    }

    // Prepare to produce surface output
    SurfaceOutput sout(voro.get(), input, mergeMap, numMergedPts, filter);
    sout.OutPoints = vtkDoubleArray::FastDownCast(outPts->GetData())->GetPointer(0);

    // Allocate cell array to hold face connectivity.
    vtkNew<vtkIdTypeArray> conn;
    conn->SetNumberOfTuples(FaceConnSize);
    sout.CellConn = conn->GetPointer(0);

    vtkNew<vtkIdTypeArray> offsets;
    offsets->SetNumberOfTuples(NumFaces + 1);
    sout.CellOffsets = offsets->GetPointer(0);
    sout.CellOffsets[NumFaces] = FaceConnSize; // cap off the offsets array

    // If requested, generate cell scalars
    if (sout.GenerateCellScalars)
    {
      sout.CreateCellScalars(NumFaces, output);
    }

    // If auxiliary point scalars are to be generated, create the
    // scalars now.
    if (sout.GeneratePointScalars)
    {
      sout.CreatePointScalars(NumPts, output);
    }

    // Populate the connectivity and offsets array.
    vtkSMPTools::For(0, voro->GetNumberOfThreads(), sout);

    // Assemble the polygonal output
    output->SetPoints(outPts);
    vtkNew<vtkCellArray> polys;
    polys->SetData(offsets, conn);
    output->SetPolys(polys);

  } // OutputSurface()
};  // SurfaceOutput

// ============================================================================
// For the OutputType == VORONOI, the filter composites boundary hull
// points and faces, as well as polyhedral connectivity information, to produce
// an unstructued grid consisting of polyhedral cells. The points may be
// topologically merged.
struct PolyHCompositor
{
  vtkIdType NPts;              // The number of point generators
  vtkIdType TotalNumPts;       // The total points produced across all hulls.
  vtkIdType TotalNumFaces;     // The total faces extracted from all hulls.
  vtkIdType TotalNumCells;     // The total cells generated from all hulls.
  vtkIdType TotalFaceConnSize; // The size of the hull faces connectivity array
  vtkIdType TotalCellConnSize; // The size of the polyhedron cells connectivity array

  // Constructors
  PolyHCompositor()
    : NPts(0)
    , TotalNumPts(0)
    , TotalNumFaces(0)
    , TotalNumCells(0)
    , TotalFaceConnSize(0)
    , TotalCellConnSize(0)
  {
  }

  // Metadata needed for compositing
  struct vtkCompositeInfo
  {
    // Initially these are "number of.." that are transformed to offsets
    // via a subsequent prefix sum operation.
    vtkIdType NumPts;       // number of points produced
    vtkIdType NumCells;     // number of polyhedron produced
    vtkIdType NumFaces;     // number of polygon faces produced
    vtkIdType CellConnSize; // face locations size
    vtkIdType FaceConnSize; // face connectivity size
    vtkCompositeInfo()
      : NumPts(0)
      , NumCells(0)
      , NumFaces(0)
      , CellConnSize(0)
      , FaceConnSize(0)
    {
    }

    // Operator += provides support for prefix sum. Converts counts
    // to offsets.
    vtkCompositeInfo& operator+=(const vtkCompositeInfo& info)
    {
      this->NumPts += info.NumPts;
      this->NumCells += info.NumCells;
      this->NumFaces += info.NumFaces;
      this->CellConnSize += info.CellConnSize;
      this->FaceConnSize += info.FaceConnSize;
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
  void Initialize(vtkIdType numPts, PolyHCompositor* vtkNotUsed(comp))
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
    this->TotalNumCells = totalInfo.NumCells;
    this->TotalNumFaces = totalInfo.NumFaces;
    this->TotalCellConnSize = totalInfo.CellConnSize;
    this->TotalFaceConnSize = totalInfo.FaceConnSize;
  }

  /**
   * This is the data extracted from the hulls and accumulated by the thread
   * local data.
   */
  struct LocalData
  {
    vtkCompositeInformation* Info;         // singleton enables prefix sum compositing
    vtkVoronoiHullVertexType Points;       // coordinates defining the hull vertices
    vtkVoronoiTopoCoords3DType TopoCoords; // topological coordinates
    vtkVoronoiCellConnType CellConn;       // hull face connectivity

    LocalData()
    {
      this->Points.reserve(1024);
      this->TopoCoords.reserve(1024);
      this->CellConn.reserve(1024);
    }
    void Initialize(PolyHCompositor* c) { this->Info = &(c->Information); }
    /**
     * This method is called after the Voronoi tile/hull is generated, so that
     * compositing information can be extracted and recorded.
     */
    void AddData(
      vtkVoronoiHull& hull, int vtkNotUsed(numSpokes), const vtkVoronoiSpoke* vtkNotUsed(spokes))
    {
      // Generate output only if hull points exist.
      if (hull.NumPts <= 0)
      {
        return;
      }

      // Gather information about appropriately classified faces.
      vtkIdType numHullPts = hull.NumPts;
      vtkIdType ptId = hull.PtId;

      // Start by outputting all Voronoi hull points and associated
      // topological coordinates. This has the side effect of (locally)
      // numbering the points.
      int npts = 0;
      for (auto& p : hull.Points)
      {
        if (p.Status == ProcessingStatus::Valid)
        {
          p.PtMap = npts++; // number the point (local numbering)
          this->Points.emplace_back(p.X);
          vtkIdType p0 = hull.GetFace(p.Faces[0])->NeiId;
          vtkIdType p1 = hull.GetFace(p.Faces[1])->NeiId;
          vtkIdType p2 = hull.GetFace(p.Faces[2])->NeiId;
          this->TopoCoords.emplace_back(p0, p1, p2, ptId);
        } // if valid point
      }   // for all hull points

      // Now output all of the Voronoi hull faces.
      int numFacePts;
      vtkIdType faceConnSize = 0;

      for (int faceId = 0; faceId < static_cast<int>(hull.Faces.size()); ++faceId)
      {
        vtkHullFace* face = hull.GetFace(faceId);
        if (face->Status == ProcessingStatus::Valid)
        {
          numFacePts = face->NumPts;
          this->CellConn.emplace_back(numFacePts);
          faceConnSize += numFacePts;
          for (vtkIdType i = 0; i < numFacePts; ++i)
          {
            vtkHullPoint& p = hull.Points[hull.GetFacePoint(face, i)];
            this->CellConn.emplace_back(p.PtMap);
          }
        } // if valid face
      }   // for all hull faces

      (*this->Info)[ptId].NumPts = numHullPts;
      (*this->Info)[ptId].NumCells = 1;
      (*this->Info)[ptId].NumFaces = hull.NumFaces;
      (*this->Info)[ptId].CellConnSize = numHullPts;
      (*this->Info)[ptId].FaceConnSize = faceConnSize;
    } // AddData()
  };
}; // PolyHCompositor

// Interface with vtkSMPTools to threaded generate polyhedron cells.
struct PolyHOutput : public PtsOutput
{
  const vtkVoronoiCore3D<PolyHCompositor, vtkVoronoiClassifier3D>* VC;
  vtkIdType* FaceConn;    // polyhedral face connectivity
  vtkIdType* FaceOffsets; // polyhedral face offsets
  vtkIdType* LocConn;     // polyhedral face locations
  vtkIdType* LocOffsets;  // polyhedral face location offsets

  PolyHOutput(vtkVoronoiCore3D<PolyHCompositor, vtkVoronoiClassifier3D>* vc, vtkPointSet* input,
    vtkMergeMapType* mergeMap, vtkIdType numMergedPts, vtkVoronoi3D* filter)
    : PtsOutput(input, mergeMap, numMergedPts, filter)
    , VC(vc)
    , FaceConn(nullptr)
    , FaceOffsets(nullptr)
    , LocConn(nullptr)
    , LocOffsets(nullptr)
  {
    this->InPoints = vc->GetPoints();
  }

  // Add a polyhedral face to the output. This should be followed by
  // AddFacePoint() calls.
  void AddFace(vtkIdType faceId, vtkIdType connOffset) { this->FaceOffsets[faceId] = connOffset; }

  // Add a polyhedral face point to the output
  void AddFacePoint(vtkIdType connOffset, vtkIdType pId) { this->FaceConn[connOffset] = pId; }

  // Add a polyhedral face point to the output
  void AddMergedFacePoint(const vtkMergeMapType& mergeMap, vtkIdType connOffset, vtkIdType ptId)
  {
    vtkIdType pId = mergeMap[ptId];
    this->FaceConn[connOffset] = pId;
  }

  // Each thread transforms and writes its own data.
  void operator()(vtkIdType threadId, vtkIdType endThreadId)
  {
    // Prepare for execution
    const vtkVoronoiBatchManager& batcher = this->VC->Batcher;
    vtkVoronoiAbortCheck abortCheck(threadId, endThreadId, this->Filter);

    bool mergePts = this->MergeMap ? true : false;
    const vtkMergeMapType& mergeMap = *(this->MergeMap);
    PtsWrittenFlags& ptsWritten = *(this->PtsWritten);

    // Loop over all threads
    for (; threadId < endThreadId; ++threadId)
    {
      if (abortCheck(threadId))
      {
        break;
      }

      // Get the current local thread data including the batches processed by
      // this thread.
      vtkVoronoi3DLocalData<PolyHCompositor, vtkVoronoiClassifier3D>& localData =
        *(this->VC->ThreadMap[threadId]);
      const PolyHCompositor& compositor = this->VC->Compositor;
      const PolyHCompositor::vtkCompositeInformation& info = compositor.Information;
      vtkVoronoiHullVertexType::iterator pItr = localData.Compositor.Points.begin();
      vtkVoronoiCellConnType::iterator cItr = localData.Compositor.CellConn.begin();

      // Loop over the batches that the current thread processed earlier. The batches
      // are ordered and consistent with the local data vectors.
      for (auto& batchId : localData.LocalBatches)
      {
        vtkIdType ptId, endPtId;
        batcher.GetBatchItemRange(batchId, ptId, endPtId);
        for (; ptId < endPtId; ++ptId) // output all data in this batch
        {
          vtkIdType numCells = (info[ptId + 1].NumCells - info[ptId].NumCells);
          if (numCells <= 0)
          {
            continue; // nothing to see here
          }
          vtkIdType numPts = (info[ptId + 1].NumPts - info[ptId].NumPts);
          vtkIdType numFaces = (info[ptId + 1].NumFaces - info[ptId].NumFaces);
          vtkIdType startPtId = info[ptId].NumPts;
          vtkIdType startFaceId = info[ptId].NumFaces;
          vtkIdType startCellId = info[ptId].NumCells;
          vtkIdType startFaceConn = info[ptId].FaceConnSize;
          vtkIdType startCellConn = info[ptId].CellConnSize;

          // Output the points
          vtkIdType pId = startPtId;
          if (mergePts)
          {
            for (int i = 0; i < numPts; ++i, ++pId, ++pItr)
            {
              this->AddMergedPoint(mergeMap, ptsWritten, this->PointScalars, ptId, pId, pItr->X);
            }
          }
          else
          {
            for (int i = 0; i < numPts; ++i, ++pId, ++pItr)
            {
              this->AddPoint(this->PointScalars, ptId, pId, pItr->X);
            }
          }

          // Output the polyhedral cell connectivity. Note that the cell
          // point ids need to be transformed into global point id
          // space. Also output optional cell data.
          bool firstRandomScalar = true;
          vtkIdType cellId = startCellId;
          this->AddPrim(cellId, startCellConn);
          if (mergePts)
          {
            for (int j = 0; j < numPts; ++j)
            {
              this->AddMergedPrimPoint(mergeMap, startCellConn++, startPtId + j);
            }
          }
          else
          {
            for (int j = 0; j < numPts; ++j)
            {
              this->AddPrimPoint(startCellConn++, startPtId + j);
            }
          }

          if (this->CellScalars)
          {
            this->CellScalars[cellId] = this->ProduceCellScalar(
              ptId, this->VC->Graph.Wheels[ptId], cellId, threadId, firstRandomScalar);
          } // if cell scalars

          // Update the face locations.
          this->LocOffsets[cellId] = startFaceId;

          // Output the polyhedral faces.
          for (int fId = 0; fId < numFaces; ++fId)
          {
            this->AddFace(startFaceId + fId, startFaceConn);
            vtkIdType numFacePts = *cItr++;
            if (mergePts)
            {
              for (int j = 0; j < numFacePts; ++j)
              {
                this->AddMergedFacePoint(mergeMap, startFaceConn++, startPtId + *cItr++);
              }
            }
            else
            {
              for (int j = 0; j < numFacePts; ++j)
              {
                this->AddFacePoint(startFaceConn++, startPtId + *cItr++);
              }
            }
          } // for all output polyhedral cell primitives
        }   // for all points in this batch
      }     // for all batches
    }       // for all threads
  }         // operator()

  // Driver function for polyhedral output.
  static void Execute(vtkVoronoi3D* filter, int batchSize, vtkStaticPointLocator* loc,
    vtkPoints* tPoints, double padding, vtkIdType maxClips, bool validate, double pruneTol,
    vtkPointSet* input, const int* regions, vtkUnstructuredGrid* output)
  {
    vtkVoronoiClassifier3D initializeClassifier(regions);
    auto voro =
      vtkVoronoiCore3D<PolyHCompositor, vtkVoronoiClassifier3D>::Execute(filter, batchSize, loc,
        tPoints, padding, maxClips, validate, pruneTol, nullptr, &initializeClassifier);

    voro->UpdateExecutionInfo(filter->NumberOfThreadsUsed, filter->MaximumNumberOfPoints,
      filter->MaximumNumberOfFaces, filter->NumberOfPrunes);

    // Now access the composited information
    PolyHCompositor& compositor = voro->Compositor;
    vtkIdType NumPts = compositor.TotalNumPts;
    vtkIdType NumCells = compositor.TotalNumCells;
    vtkIdType NumFaces = compositor.TotalNumFaces;
    vtkIdType CellConnSize = compositor.TotalCellConnSize;
    vtkIdType FaceConnSize = compositor.TotalFaceConnSize;

    // If point merging is enabled, create a toopological merge map
    vtkNew<vtkPoints> outPts;
    outPts->SetDataTypeToDouble();
    std::unique_ptr<vtkVoronoiCore3D<PolyHCompositor, vtkVoronoiClassifier3D>::TopologicalMerge>
      topoMerge;

    vtkMergeMapType* mergeMap = nullptr;
    vtkIdType numMergedPts = 0;
    if (filter->GetMergePoints())
    {
      topoMerge =
        vtkVoronoiCore3D<PolyHCompositor, vtkVoronoiClassifier3D>::TopologicalMerge::Execute(
          voro.get());
      mergeMap = &(topoMerge->MergeMap);
      numMergedPts = NumPts = topoMerge->GetNumberOfMergedPoints();
      outPts->SetNumberOfPoints(numMergedPts);
    }
    else
    {
      outPts->SetNumberOfPoints(NumPts);
    }

    // Prepare to produce surface output
    PolyHOutput pout(voro.get(), input, mergeMap, numMergedPts, filter);
    pout.OutPoints = vtkDoubleArray::FastDownCast(outPts->GetData())->GetPointer(0);

    // In special cases, the input point data can be passed as output cell data.
    vtkIdType numInputPts = voro->GetNumberOfPoints();
    if (pout.PassPointData && numInputPts == NumCells)
    { // pass point data as cell data
      output->GetCellData()->PassData(pout.Input->GetPointData());
    }

    // Cell types - they are all polyhedra. This can be created here.
    vtkNew<vtkUnsignedCharArray> cellTypes;
    cellTypes->SetNumberOfTuples(NumCells);
    vtkSMPTools::Fill(cellTypes->GetPointer(0), cellTypes->GetPointer(0) + NumCells,
      static_cast<unsigned char>(VTK_POLYHEDRON));

    // Define polyhedral cells to be filled in later.
    vtkNew<vtkIdTypeArray> cellConn;
    cellConn->SetNumberOfTuples(CellConnSize);
    pout.CellConn = cellConn->GetPointer(0);

    vtkNew<vtkIdTypeArray> cellOffsets;
    cellOffsets->SetNumberOfTuples(NumCells + 1);
    pout.CellOffsets = cellOffsets->GetPointer(0);
    pout.CellOffsets[NumCells] = CellConnSize; // cap off the offsets array

    vtkNew<vtkCellArray> cells;
    cells->SetData(cellOffsets, cellConn);

    // Allocate cell array to hold face connectivity.
    vtkNew<vtkIdTypeArray> faceConn;
    faceConn->SetNumberOfTuples(FaceConnSize);
    pout.FaceConn = faceConn->GetPointer(0);

    vtkNew<vtkIdTypeArray> faceOffsets;
    faceOffsets->SetNumberOfTuples(NumFaces + 1);
    pout.FaceOffsets = faceOffsets->GetPointer(0);
    pout.FaceOffsets[NumFaces] = FaceConnSize; // cap off the offsets array

    vtkNew<vtkCellArray> faces;
    faces->SetData(faceOffsets, faceConn);

    // The face locations are basically an enumeration of the face ids. We
    // can partially complete this cell array (using the lambda) here.
    // The offsets are filled in later.
    vtkNew<vtkIdTypeArray> locConn;
    locConn->SetNumberOfTuples(NumFaces);
    pout.LocConn = locConn->GetPointer(0);
    vtkIdType pId = 0;
    std::generate(pout.LocConn, pout.LocConn + NumFaces, [&pId] { return pId++; });

    vtkNew<vtkIdTypeArray> locOffsets;
    locOffsets->SetNumberOfTuples(NumCells + 1);
    pout.LocOffsets = locOffsets->GetPointer(0);
    pout.LocOffsets[NumCells] = NumFaces;

    vtkNew<vtkCellArray> faceLocs;
    faceLocs->SetData(locOffsets, locConn);

    // Finally, assemble the output.
    output->SetPoints(outPts);
    output->SetPolyhedralCells(cellTypes, cells, faceLocs, faces);

    // If requested, generate cell scalars
    if (pout.GenerateCellScalars)
    {
      pout.CreateCellScalars(NumCells, output);
    }

    // If auxiliary point scalars are to be generated, create the
    // scalars now.
    if (pout.GeneratePointScalars)
    {
      pout.CreatePointScalars(NumPts, output);
    }

    // Now parallel thread the creation of the volume output.
    vtkSMPTools::For(0, voro->GetNumberOfThreads(), pout);

  } // OutputPolyH()
};  // PolyHOutput

} // anonymous namespace

//================= Begin VTK class proper =====================================
//------------------------------------------------------------------------------
// Construct object
vtkVoronoi3D::vtkVoronoi3D()
{
  this->OutputType = vtkVoronoi3D::BOUNDARY;
  this->Padding = 0.001;
  this->Validate = false;
  this->Locator = vtkSmartPointer<vtkStaticPointLocator>::New();
  this->Locator->SetNumberOfPointsPerBucket(2);
  this->PassPointData = true;
  this->GeneratePointScalars = NO_POINT_SCALARS;
  this->GenerateCellScalars = POINT_IDS;
  this->MergePoints = true;

  this->PointOfInterest = (-1);
  // this->PointsOfInterest empty on instantiation
  this->MaximumNumberOfHullClips = VTK_ID_MAX;
  this->PruneTolerance = 1.0e-13;
  this->BatchSize = 1000;
  this->BoundaryCapping = true;

  this->MaximumNumberOfPoints = 0;
  this->MaximumNumberOfFaces = 0;
  this->NumberOfThreadsUsed = 0;
  this->NumberOfPrunes = 0;

  // By default process active point scalars to obtain region ids
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
}

//------------------------------------------------------------------------------
int vtkVoronoi3D::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo0 = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPointSet* input = vtkPointSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo0->Get(vtkDataObject::DATA_OBJECT()));

  // Cast to proper output type
  vtkUnstructuredGrid* volOutput = nullptr;
  vtkPolyData* surfOutput = nullptr;
  if (this->OutputType == vtkVoronoi3D::VORONOI || this->OutputType == vtkVoronoi3D::DELAUNAY)
  {
    volOutput = vtkUnstructuredGrid::SafeDownCast(output);
  }
  else
  {
    surfOutput = vtkPolyData::SafeDownCast(output);
  }

  vtkDebugMacro(<< "Generating 3D Voronoi Tessellation");

  // Check the input, at least one point is needed.
  vtkIdType pId, numPts;
  vtkPoints* inPoints;
  if ((inPoints = input->GetPoints()) == nullptr || (numPts = inPoints->GetNumberOfPoints()) < 1)
  {
    vtkDebugMacro("Cannot tessellate; need at least 1 input point");
    return 1;
  }

  // Input points must be of type double
  vtkSmartPointer<vtkPoints> tPoints;
  if (inPoints->GetDataType() == VTK_DOUBLE)
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

  // A locator is used to locate closest points.
  if (!this->Locator)
  {
    vtkErrorMacro(<< "Point locator required\n");
    return 0;
  }
  this->Locator->SetDataSet(tInput);
  this->Locator->BuildLocator();
  this->Locator->StaticOn();

  // Computational bounds and the padded bounding box
  double length = input->GetLength();
  double padding = this->Padding * length;

  // Region ids can be used to control which input points are processed.
  // A region id < 0 means that the associated point is "outside" (or
  // background) and does not contribute to the output. We can use this
  // capability to process a specified "PointOfInterest" (if any).
  // Otherwise, we check the input for segmented regions via a regions
  // ids array.
  //
  // If region ids are provided,  array must be a single component tuple,
  // signed integer of type vtkIntArray with the number of tuples == number
  // of input points. (Implementation note: this could be expanded with
  // templates - not sure its worth the object bloat.)
  vtkSmartPointer<vtkIntArray> regionIds;
  int* regions = nullptr;

  // Limit processing to points of interested if so specified.
  this->GeneratePointScalars = NO_POINT_SCALARS;
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
    // Can be useful information when generating a point of interest
    this->GeneratePointScalars = FLOWER_RADII;
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
  if (regionIds)
  {
    regions = regionIds->GetPointer(0);
  }

  // Prepare to execute Voronoi
  int batchSize = this->BatchSize;
  vtkStaticPointLocator* loc = this->Locator;
  vtkIdType maxClips = this->MaximumNumberOfHullClips;
  bool validate = this->Validate;
  double pruneTol = this->PruneTolerance;

  // Perform a speed test. No output is produced, but all of the hulls (from
  // the input point generators) are processed.
  if (this->OutputType == vtkVoronoi3D::SPEED_TEST)
  {
    SpeedTestOutput(this, batchSize, loc, tPoints, padding, maxClips, validate, pruneTol, regions);
  }

  // Produce the (wheel and spokes) adjacency graph.
  else if (this->OutputType == vtkVoronoi3D::ADJACENCY_GRAPH)
  {
    AGOutput::Execute(this, batchSize, loc, tPoints, padding, maxClips, validate, pruneTol, tInput,
      regions, surfOutput);
  }

  // Produce a Delaunay triangulation of the input points.
  else if (this->OutputType == vtkVoronoi3D::DELAUNAY)
  {
    DelOutput::Execute(this, batchSize, loc, tPoints, padding, maxClips, validate, pruneTol, tInput,
      regions, volOutput);
  }

  // Produce a polyhedral unstructured mesh. Each convex polyhedron is
  // produced from one generator point.
  else if (this->OutputType == vtkVoronoi3D::VORONOI)
  {
    PolyHOutput::Execute(this, batchSize, loc, tPoints, padding, maxClips, validate, pruneTol,
      tInput, regions, volOutput);
  }

  // By default, produce a vtkPolyData surface.
  //
  else
  {
    SurfaceOutput::Execute(this->OutputType, this, batchSize, loc, tPoints, padding, maxClips,
      validate, pruneTol, tInput, regions, surfOutput);
  }

  // Return the locator to a normal processing mode.
  this->Locator->StaticOff();
  this->Locator->FreeSearchStructure();

  return 1;
} // RequestData

//------------------------------------------------------------------------------
vtkIdType vtkVoronoi3D::FindHull(double x[3])
{
  // Make sure the filter has executed (i.e., a locator is available), and the
  // request is within the bounding box of the input points.
  if (this->Locator == nullptr)
  {
    return (-1);
  }

  double bounds[6];
  this->Locator->GetBounds(bounds);
  if (x[0] < bounds[0] || x[0] > bounds[1] || x[1] < bounds[2] || x[1] > bounds[3] ||
    x[2] < bounds[4] || x[2] > bounds[5])
  {
    return -1;
  }

  // Now simply request the closest point.
  return this->Locator->FindClosestPoint(x);
}

//------------------------------------------------------------------------------
int vtkVoronoi3D::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkVoronoi3D::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  if (this->OutputType == vtkVoronoi3D::VORONOI || this->OutputType == vtkVoronoi3D::DELAUNAY)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkUnstructuredGrid");
  }
  else
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  }

  return 1;
}

//------------------------------------------------------------------------------
// Since users have access to the locator we need to take into account the
// locator's modified time.
vtkMTimeType vtkVoronoi3D::GetMTime()
{
  vtkMTimeType mTime = this->vtkObject::GetMTime();
  vtkMTimeType time = this->Locator->GetMTime();
  return (time > mTime ? time : mTime);
}

//------------------------------------------------------------------------------
void vtkVoronoi3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Output Type: " << this->OutputType << "\n";
  os << indent << "Padding: " << this->Padding << "\n";
  os << indent << "Validate: " << (this->Validate ? "On\n" : "Off\n");
  os << indent << "Locator: " << this->Locator << "\n";
  os << indent << "Pass Point Data: " << (this->PassPointData ? "On\n" : "Off\n");
  os << indent << "Generate Cell Scalars: " << this->GenerateCellScalars << "\n";
  os << indent << "Merge Points: " << (this->MergePoints ? "On\n" : "Off\n");

  os << indent << "Point Of Interest: " << this->PointOfInterest << "\n";
  os << indent << "Points Of Interest: " << this->PointsOfInterest << "\n";
  os << indent << "Maximum Number Of Hull Clips: " << this->MaximumNumberOfHullClips << "\n";

  os << indent << "Prune Tolerance: " << this->PruneTolerance << "\n";
  os << indent << "Batch Size: " << this->BatchSize << "\n";
  os << indent << "Boundary Capping: " << (this->BoundaryCapping ? "On\n" : "Off\n");
}

VTK_ABI_NAMESPACE_END
