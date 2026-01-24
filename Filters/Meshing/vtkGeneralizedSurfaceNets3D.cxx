// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkGeneralizedSurfaceNets3D.h"

#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLabelMapLookup.h"
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
#include "vtkUnstructuredGrid.h"
#include "vtkVoronoiCore3D.h"

#include <stack>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkGeneralizedSurfaceNets3D);

namespace // anonymous
{
//======= Define the compositing and classification classes used to
//======= generate the Voronoi tesellation.

/**
 * Gather hull points, topological coordinates, and face connectivity
 * for later compositing.
 */
struct SNCompositor
{
  vtkIdType NPts;          // The number of point generators
  vtkIdType TotalNumPts;   // The total points extracted from all hulls.
  vtkIdType TotalNumFaces; // The total faces extracted from all hulls.
  bool BoundaryCapping;    // whether to produce boundary surfaces
  SNCompositor()
    : NPts(0)
    , TotalNumPts(0)
    , TotalNumFaces(0)
    , BoundaryCapping(true)
  {
  }
  SNCompositor(bool capping)
    : SNCompositor()
  {
    this->BoundaryCapping = capping;
  }

  // Metadata needed for compositing
  struct vtkCompositeInfo
  {
    // Initially these are local counts that are transformed to offsets
    // via a subsequent prefix sum operation.
    vtkIdType NumPts;   // number of points produced
    vtkIdType NumFaces; // number of polygon faces produced
    vtkIdType ConnSize; // face connectivity size
    vtkCompositeInfo()
      : NumPts(0)
      , NumFaces(0)
      , ConnSize(0)
    {
    }

    // Operator += provides support for prefix sum. Converts counts
    // to offsets.
    vtkCompositeInfo& operator+=(const vtkCompositeInfo& info)
    {
      this->NumPts += info.NumPts;
      this->NumFaces += info.NumFaces;
      this->ConnSize += info.ConnSize;
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
  void Initialize(vtkIdType numPts, SNCompositor* comp)
  {
    this->NPts = numPts;
    this->Information.resize(numPts + 1);
    this->BoundaryCapping = comp->BoundaryCapping;
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
    vtkVoronoiHullVertexType Points;       // coordinates defining the hull points
    vtkVoronoiTopoCoords3DType TopoCoords; // points in topological space
    vtkVoronoiCellConnType FaceConn;       // face connectivity
    bool BoundaryCapping;

    LocalData()
    {
      this->Points.reserve(1024);
      this->TopoCoords.reserve(1024);
      this->FaceConn.reserve(1024);
    }
    void Initialize(SNCompositor* c)
    {
      this->Info = &(c->Information);
      this->BoundaryCapping = c->BoundaryCapping;
    }
    /**
     * This method is called after the Voronoi tile/hull is constructed, so that
     * compositing information can be extracted and recorded.
     */
    void AddData(vtkVoronoiHull& hull, int vtkNotUsed(numSpokes), const vtkVoronoiSpoke* spokes)
    {
      // Generate output only if hull faces exist
      if (hull.NumFaces <= 0)
      {
        return;
      }

      // Loop over all the hull faces, extracting the requested points and faces
      int numOutputPts = 0;
      int numOutputFaces = 0;
      int connSize = 0;

      // Process all valid faces. Note that while the number of spokes is equal to
      // the number of valid faces, the hull Faces array may have invalid faces.
      int spokeNum = 0;
      for (int faceId = 0; faceId < static_cast<int>(hull.Faces.size()); ++faceId)
      {
        // Note the 1:1 correspondance between spokes and valid faces
        vtkHullFace* face = hull.GetFace(faceId);
        if (face->Status == ProcessingStatus::Valid)
        {
          int numFacePts = face->NumPts;
          int backfaceId = face->NeiId;

          if ((spokes[spokeNum].Classification & vtkSpokeClassification::FORWARD_SPOKE &&
                spokes[spokeNum].Classification & vtkSpokeClassification::REGION_BOUNDARY) ||
            (this->BoundaryCapping &&
              spokes[spokeNum].Classification & vtkSpokeClassification::DOMAIN_BOUNDARY))
          {
            numOutputFaces++;
            this->FaceConn.emplace_back(numFacePts);
            this->FaceConn.emplace_back(backfaceId);
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
                this->TopoCoords.emplace_back(p0, p1, p2, hull.PtId);
              }
              this->FaceConn.emplace_back(p.PtMap);
            }
          } // specified face type matches
          spokeNum++;
        } // if valid face
      }   // for all polyhedral faces

      // Gather information about the extracted geometry from this hull
      (*this->Info)[hull.PtId].NumPts = numOutputPts;
      (*this->Info)[hull.PtId].NumFaces = numOutputFaces;
      (*this->Info)[hull.PtId].ConnSize = connSize;

    } // AddData()
  };  // LocalData
};    // SNCompositor

/**
 * Classify spokes (and associated dual faces). Classification is based on
 * region labels. The classifier is provided as a template argument to
 * the vtkVoronoiCore3D class.
 */
struct SNClassifier
{
  // Optional regions ids for point classification. Implementation note:
  // unique_ptr<vtkLabelMapLookup> doesn't work cleanly because of
  // vtkSMPTools use of copy constructor and operator=.
  const int* Regions;
  const double* LabelValues;
  vtkIdType NumLabels;
  vtkLabelMapLookup<int>* LMap;

  // Constructors
  SNClassifier()
    : Regions(nullptr)
    , LabelValues(nullptr)
    , NumLabels(0)
    , LMap(nullptr)
  {
  }
  SNClassifier(const int* regions, const double* labels, vtkIdType numLabels)
    : Regions(regions)
    , LabelValues(labels)
    , NumLabels(numLabels)
  {
    this->LMap = vtkLabelMapLookup<int>::CreateLabelLookup(labels, numLabels);
  }

  // Free label map - nullptr delete is okay.
  ~SNClassifier() { delete this->LMap; }

  // Method required by vtkVoronoiCore3D.
  void Initialize(SNClassifier* c)
  {
    if (c)
    {
      this->Regions = c->Regions;
      this->LabelValues = c->LabelValues;
      this->NumLabels = c->NumLabels;
      delete this->LMap;
      this->LMap = vtkLabelMapLookup<int>::CreateLabelLookup(this->LabelValues, this->NumLabels);
    }
  }

  // Required method.
  bool IsInsideRegion(vtkIdType ptId)
  {
    if (ptId < 0)
    {
      return false;
    }
    else
    {
      return (this->Regions[ptId] >= 0 && this->LMap->IsLabelValue(this->Regions[ptId]));
    }
  }

  // Required method.
  bool IsSameRegion(vtkIdType ptId, vtkIdType neiId)
  {
    return (this->Regions[ptId] == this->Regions[neiId]);
  }

  // Required method - classify spokes.
  const vtkVoronoiSpoke* AddAdjacencyInformation(vtkVoronoiHull& hull, vtkVoronoiWheelsType& wheels,
    vtkVoronoiSpokesType& spokes, int& vtkNotUsed(numSpokes), int& maxPoints, int& maxFaces)
  {
    // Keep track of the starting position at which
    // spokes willbe added.
    vtkIdType startPos = spokes.size();

    // Loop over all valid faces. Note that ptId is always >=0 (i.e., inside).
    vtkIdType ptId = hull.PtId;
    for (auto& fitr : hull.Faces)
    {
      if (fitr.Status == ProcessingStatus::Valid)
      {
        vtkIdType neiId = fitr.NeiId;
        unsigned char spokeClass = (ptId < neiId ? vtkSpokeClassification::FORWARD_SPOKE
                                                 : vtkSpokeClassification::BACKWARD_SPOKE);

        if (!this->IsInsideRegion(neiId))
        {
          spokeClass |= vtkSpokeClassification::DOMAIN_BOUNDARY;
        }
        else if (!this->IsSameRegion(ptId, neiId))
        {
          spokeClass |= vtkSpokeClassification::REGION_BOUNDARY;
        }

        // Create the spoke and add it the spokes vector
        spokes.emplace_back(neiId, spokeClass);
      } // if Valid face
    }   // for all polyhedral faces

    wheels[ptId] = hull.NumFaces; // numFaces == numSpokes
    maxPoints = (hull.NumPts > maxPoints ? hull.NumPts : maxPoints);
    maxFaces = (hull.NumFaces > maxFaces ? hull.NumFaces : maxFaces);

    // Spokes are added with emplace_back(), so may cause
    // reallocations. So we wait for all spokes to be added
    // before returning the pointer to the list of spokes.
    return spokes.data() + startPos;
  } // AddAdjacencyInformation
};  // vtkSNClassifier

//======= Some generic helper functions

// Types for smoothing, and writing output
using SmoothPointType = std::vector<unsigned char>; // type of points for smoothing
using PtsWrittenFlags = std::vector<unsigned char>; // track writing merged points

// Used to produce smoothing stencils. Note that the stencil edges
// have direction, so the vertices of the edge (V0,V1) are not
// ordered. The smoothing stencils are represented by a vtkCellArray:
// for each point p, a list of connected points are used to weighted
// average p's position. Here, p is considered the cell, and the
// connected points the cell's points - an overload of vtkCellArray.
struct StencilEdge
{
  vtkIdType V0;
  vtkIdType V1;
  StencilEdge(vtkIdType v0, vtkIdType v1)
    : V0(v0)
    , V1(v1)
  {
  }

  // Sort on V0 first, then V1. Here V0 is not assumed <V1, since
  // the smoothing edges may by assymetric.
  bool operator<(const StencilEdge& se) const
  {
    if (this->V0 < se.V0)
      return true;
    if (se.V0 < this->V0)
      return false;
    if (this->V1 < se.V1)
      return true;
    return false;
  }
}; // StencilEdge
using StencilEdgeType = StencilEdge;
using StencilEdgesType = std::vector<StencilEdgeType>;

// This produces smoothing stencils if requested. It examines the local
// point geometry/topology to generate a point classification. This is
// followed by a process of adjusting the stencils as appropriate.
struct ProduceStencils
{
  vtkCellArray* Polys;
  const vtkMergeTuples3DType& MTuples;
  const vtkMergeTupleOffsets& MOffsets;
  const SmoothPointType& SPtsType;
  vtkCellArray* Stencils;
  unsigned char Constraints[4];

  vtkSMPThreadLocal<StencilEdgesType> StencilEdges;
  vtkSMPThreadLocal<vtkSmartPointer<vtkCellArrayIterator>> PolysIterator;

  // Indicate the allowed movement of a point while smoothing.
  enum PointSmoothType
  {
    FIXED = 4,
    EDGE = 3,
    FACE = 2,
    UNCONSTRAINED = 1
  };

  ProduceStencils(vtkCellArray* polys, const vtkMergeTuples3DType& mtuples,
    const vtkMergeTupleOffsets& moffsets, const SmoothPointType& spts, vtkCellArray* stencils,
    unsigned char constraints[4])
    : Polys(polys)
    , MTuples(mtuples)
    , MOffsets(moffsets)
    , SPtsType(spts)
    , Stencils(stencils)
    , Constraints{ constraints[0], constraints[1], constraints[2], constraints[3] }
  {
  }

  // Methods to support threading
  void Initialize() { this->PolysIterator.Local().TakeReference(this->Polys->NewIterator()); }

  // Process polygons, examining their edges. Based on the smoothing
  // type of each edge, add edges to the smoothing stencil.
  void operator()(vtkIdType polysId, vtkIdType endPolysId)
  {
    auto& sptsType = this->SPtsType;
    auto& edges = this->StencilEdges.Local();
    vtkCellArrayIterator* polysIter = this->PolysIterator.Local();
    vtkIdType npts;
    const vtkIdType* pts;

    // To avoid revisiting edges, process only edges (v0,v1) when
    // (v0<v1). Evaluate  both directions of the edge.
    for (; polysId < endPolysId; ++polysId)
    {
      polysIter->GetCellAtId(polysId, npts, pts);
      for (auto i = 0; i < npts; ++i)
      {
        vtkIdType v0 = pts[i];
        vtkIdType v1 = pts[(i + 1) % npts];
        if (v0 < v1)
        {
          unsigned char v0Type = sptsType[v0];
          unsigned char v1Type = sptsType[v1];

          // Add stencil connections. Depending on topology,
          // we add connections one way, both ways, or none.
          if (v0Type < v1Type) // from v0 -> v1
          {
            edges.emplace_back(v0, v1);
          }
          else if (v1Type < v0Type) // from v1 -> v0
          {
            edges.emplace_back(v1, v0);
          }
          else // both ways v0Type == v1Type
          {
            edges.emplace_back(v0, v1);
            edges.emplace_back(v1, v0);
          }
        } // if min edge
      }   // for all edges
    }     // for all polygons in this batch
  }

  // Composite the stencil edges into the output stencils vtkCellArray.
  void Reduce()
  {
    // Start by compositing the stencil edges from thread local storage.
    StencilEdgesType edges;
    vtkSMPThreadLocal<StencilEdgesType>::iterator ldItr;
    vtkSMPThreadLocal<StencilEdgesType>::iterator ldEnd = this->StencilEdges.end();
    for (ldItr = this->StencilEdges.begin(); ldItr != ldEnd; ++ldItr)
    {
      edges.insert(edges.end(), (*ldItr).begin(), (*ldItr).end());
    } // over all threads

    // Sort the edges to gather them into stencils for each point.
    // Note that some points (e.g., fixed points at corners) have no
    // connections, and hence will not be moved during smoothing.
    vtkSMPTools::Sort(edges.begin(), edges.end());

    // We have enough information to build the output stencils vtkCellArray
    vtkIdType numMergedPts = this->SPtsType.size();
    vtkIdType connSize = edges.size();

    vtkNew<vtkIdTypeArray> offsets;
    offsets->SetNumberOfTuples(numMergedPts + 1);
    vtkIdType* offsetsPtr = offsets->GetPointer(0);
    std::fill_n(offsetsPtr, numMergedPts, 0);

    vtkNew<vtkIdTypeArray> conn;
    conn->SetNumberOfTuples(connSize);
    vtkIdType* connPtr = conn->GetPointer(0);

    // Loop over sorted edges, and update the number of stencil edges
    // associated with each point.
    for (vtkIdType eid = 0; eid < connSize;)
    {
      vtkIdType v0 = edges[eid].V0;
      vtkIdType numEdges = 1;
      ++eid;
      while (eid < connSize && edges[eid].V0 == v0)
      {
        ++numEdges;
        ++eid;
      }
      offsetsPtr[v0] = numEdges;
    }

    // Prefix sum the stencil edge offsets.
    vtkIdType offset = 0;
    for (vtkIdType i = 0; i < numMergedPts; ++i)
    {
      vtkIdType numEdges = offsetsPtr[i];
      offsetsPtr[i] = offset;
      offset += numEdges;
    }
    offsetsPtr[numMergedPts] = offset; // should be equal to connSize

    // Now threaded copy the stencil edges into the connectivity array.
    vtkSMPTools::For(0, numMergedPts,
      [&edges, offsetsPtr, connPtr](vtkIdType ptId, vtkIdType endPtId)
      {
        for (; ptId < endPtId; ++ptId)
        {
          vtkIdType off = offsetsPtr[ptId];
          vtkIdType* c = connPtr + off;
          vtkIdType numEdges = offsetsPtr[ptId + 1] - offsetsPtr[ptId];
          for (int i = 0; i < numEdges; ++i)
          {
            c[i] = edges[off + i].V1;
          }
        }
      });

    // Finally construct the stencils vtkCellArray.
    this->Stencils->SetData(offsets, conn);
  }

  // Static dispatch to produce smoothing stencils.
  static void Execute(vtkVoronoiCore3D<SNCompositor, SNClassifier>::TopologicalMerge* topoMerge,
    const int* regions, vtkPolyData* output, unsigned char constraints[4], vtkCellArray* stencils)
  {
    // Gather some information. At this point the output points have
    // been merged.
    const vtkMergeTuples3DType mtuples = topoMerge->MergeTuples;
    vtkIdType numMergedPts = topoMerge->NumMergedPts;

    // Create the merge tuple point offsets, this is for faster O(n) lookup.
    // This is basically a prefix sum.
    vtkMergeTupleOffsets moffsets(numMergedPts + 1);
    moffsets[0] = 0;
    vtkVoronoiMergeTuple3D currentMT = mtuples[0];
    vtkIdType mergedPtId = 1;
    for (vtkIdType i = 1; i < static_cast<vtkIdType>(mtuples.size()); ++i)
    {
      if (currentMT != mtuples[i])
      {
        moffsets[mergedPtId++] = i;
        currentMT = mtuples[i];
      }
    }                                      // for all hull vertex merge tuples
    moffsets[mergedPtId] = mtuples.size(); // ending offset

    // Classify each merged output point. Do this by computing the number
    // of degrees of freedom DOF that its associated topological coordinate
    // describes. Recall that the topological coordinate's 4-tuple is sorted
    // in ascending order--negative values mean an "outside" point--so
    // so negative/outside points are listed first.
    SmoothPointType sptsType(numMergedPts);
    vtkSMPTools::For(0, numMergedPts,
      [&mtuples, &moffsets, regions, &sptsType, constraints](vtkIdType ptId, vtkIdType endPtId)
      {
        for (; ptId < endPtId; ++ptId)
        {
          const vtkVoronoiMergeTuple3D& mtuple = mtuples[moffsets[ptId]];
          unsigned char psType = 1; // unconstrained
          for (int i = 0; i < 3; ++i)
          {
            if ((mtuple.Ids[i] < 0 && mtuple.Ids[i] != mtuple.Ids[i + 1]) ||
              (mtuple.Ids[i] >= 0 && regions[mtuple.Ids[i]] != regions[mtuple.Ids[i + 1]]))
            {
              psType++; // reduce DOF
            }
          }
          // Depending on constraints, change the DOF
          psType = (psType == FIXED && !constraints[0] ? psType - 1 : psType);
          psType = (psType == EDGE && !constraints[1] ? psType - 1 : psType);
          psType = (psType == FACE && !constraints[2] ? psType - 1 : psType);

          sptsType[ptId] = psType;
        }
      });

    // The polygonal edges of the merged output are now examined. Edges that
    // connect points consistent with their smooth points classification are
    // added to a vector of edges. These are later sorted to produce the stencil.
    vtkCellArray* polys = output->GetPolys();
    vtkIdType numPolys = polys->GetNumberOfCells();
    ProduceStencils pss(polys, mtuples, moffsets, sptsType, stencils, constraints);
    vtkSMPTools::For(0, numPolys, pss);
  }
}; // ProduceStencils

// Superclass for classes that produce VTK output. Note that the output
// classes must be consistent with the information gathered previously or
// memory issues will result.
struct VOutput
{
  const vtkVoronoiCore3D<SNCompositor, SNClassifier>* VC;
  double* OutPoints;
  vtkIdType* Conn;
  vtkIdType* ConnOffsets;
  vtkIdType* CellScalars; // 2-tuple regions on either side

  VOutput(const vtkVoronoiCore3D<SNCompositor, SNClassifier>* vc)
    : VC(vc)
    , OutPoints(nullptr)
    , Conn(nullptr)
    , ConnOffsets(nullptr)
    , CellScalars(nullptr)
  {
  }

  // Add a point to the output
  void AddPoint(vtkIdType ptId, double* x)
  {
    double* p = this->OutPoints + 3 * ptId;
    *p++ = x[0];
    *p++ = x[1];
    *p = x[2];
  }

  // Add a merged point to the output. We just write
  // the value of the first vertex hull point - it's possible
  // to average these conincident points - maybe if necessary.
  void AddMergedPoint(
    const vtkMergeMapType& mergeMap, PtsWrittenFlags& ptsWritten, vtkIdType ptId, double* x)
  {
    vtkIdType pId = mergeMap[ptId];
    if (!ptsWritten[pId])
    {
      double* p = this->OutPoints + 3 * pId;
      *p++ = x[0];
      *p++ = x[1];
      *p = x[2];
      ptsWritten[pId] = 1;
    }
  }

  // Add a primitive cell to the output. This should be
  // followed by AddFacePoint() calls.
  void AddFace(vtkIdType faceId, vtkIdType connOffset) { this->ConnOffsets[faceId] = connOffset; }

  // Add a primitive cell point to the output
  void AddFacePoint(vtkIdType connOffset, vtkIdType ptId) { this->Conn[connOffset] = ptId; }

  // Add a merged primitive cell point to the output
  void AddMergedFacePoint(const vtkMergeMapType& mergeMap, vtkIdType connOffset, vtkIdType ptId)
  {
    vtkIdType pId = mergeMap[ptId];
    this->Conn[connOffset] = pId;
  }

}; // VOutput

// Class responsible for generating output polydata.
struct SurfaceOutput : public VOutput
{
  const vtkVoronoiCore3D<SNCompositor, SNClassifier>::TopologicalMerge* TopoMerge;
  bool MergePoints;
  bool Smoothing;
  int* Regions;
  int BackgroundLabel;
  vtkPolyData* Output;
  PtsWrittenFlags* PtsWritten;

  SurfaceOutput(const vtkVoronoiCore3D<SNCompositor, SNClassifier>* vc,
    const vtkVoronoiCore3D<SNCompositor, SNClassifier>::TopologicalMerge* merge, bool mergePoints,
    bool smoothing, int* regions, int background, vtkPolyData* output, PtsWrittenFlags* ptsWritten)
    : VOutput(vc)
    , TopoMerge(merge)
    , MergePoints(mergePoints)
    , Smoothing(smoothing)
    , Regions(regions)
    , BackgroundLabel(background)
    , Output(output)
    , PtsWritten(ptsWritten)
  {
  }

  // Retrieve information for a specified hull. Invoke this after the prefix sum.
  void GetSurfaceInformation(vtkIdType ptId, vtkIdType& numPts, vtkIdType& numFaces,
    vtkIdType& connSize, vtkIdType& startPtId, vtkIdType& startFaceId, vtkIdType& startConn)
  {
    const vtkVoronoiCore3D<SNCompositor, SNClassifier>* vc = this->VC;
    const SNCompositor& compositor = vc->Compositor;
    const SNCompositor::vtkCompositeInformation& info = compositor.Information;

    numPts = (info[ptId + 1].NumPts - info[ptId].NumPts);
    numFaces = (info[ptId + 1].NumFaces - info[ptId].NumFaces);
    connSize = (info[ptId + 1].ConnSize - info[ptId].ConnSize);
    startPtId = info[ptId].NumPts;
    startFaceId = info[ptId].NumFaces;
    startConn = info[ptId].ConnSize;
  }

  // Produce polygonal output for the generating point specified.
  void ProduceSurfaceFaces(vtkIdType vtkNotUsed(threadId), vtkIdType ptId,
    vtkVoronoiHullVertexType::iterator& pItr, vtkVoronoiCellConnType::iterator& cItr)
  {
    // Retrieve offset information
    vtkIdType numPts, numFaces, connSize;
    vtkIdType startPtId, startFaceId, startConn;
    this->GetSurfaceInformation(
      ptId, numPts, numFaces, connSize, startPtId, startFaceId, startConn);

    // If nothing is to be produced, return.
    if (numFaces <= 0)
    {
      return;
    }

    // Might need this to mark one side of a surface primitive outside.
    int background = this->BackgroundLabel;

    // Point merging may be in effect
    bool merging = this->Smoothing || this->MergePoints;
    const vtkMergeMapType& mergeMap = this->TopoMerge->MergeMap;
    PtsWrittenFlags& ptsWritten = *(this->PtsWritten);

    // Output the points
    vtkIdType pId = startPtId;
    if (merging)
    {
      for (int i = 0; i < numPts; ++i, ++pId, ++pItr)
      {
        this->AddMergedPoint(mergeMap, ptsWritten, pId, pItr->X);
      }
    }
    else
    {
      for (int i = 0; i < numPts; ++i, ++pId, ++pItr)
      {
        this->AddPoint(pId, pItr->X);
      }
    }

    // Output the cell connectivity. Note that the cell point ids need to be
    // transformed into global point id space. Also output optional cell data.
    vtkIdType primId = startFaceId;
    for (int i = 0; i < numFaces; ++primId, ++i)
    {
      this->AddFace(primId, startConn);
      vtkIdType numFacePts = *cItr++;
      vtkIdType backfaceId = *cItr++;
      if (merging)
      {
        for (int j = 0; j < numFacePts; ++j)
        {
          this->AddMergedFacePoint(mergeMap, startConn++, startPtId + *cItr++);
        }
      }
      else
      {
        for (int j = 0; j < numFacePts; ++j)
        {
          this->AddFacePoint(startConn++, startPtId + *cItr++);
        }
      }

      // Scalars are 2-tuples, region ids on either side of face prim.
      // Order the tuples so that (s0<s1).
      vtkIdType s0 = this->Regions[ptId];
      vtkIdType s1 = (backfaceId >= 0 ? this->Regions[backfaceId] : background);
      // if (s0 > s1 && s1 != background) // make sure s1 is non-outside region
      // {
      //   std::swap(s0, s1);
      // }
      this->CellScalars[2 * primId] = s0;
      this->CellScalars[2 * primId + 1] = s1;
    } // for all output cell primitives
  }

  // Each thread transforms and writes its own data.
  void operator()(vtkIdType threadId, vtkIdType endThreadId)
  {
    const vtkVoronoiBatchManager& batcher = this->VC->Batcher;
    vtkVoronoiAbortCheck abortCheck(threadId, endThreadId, this->VC->Filter);
    // Loop over all threads
    for (; threadId < endThreadId; ++threadId)
    {
      if (abortCheck(threadId))
      {
        break;
      }

      // Get the current local thread data. Also get indices into
      // the local data
      vtkVoronoi3DLocalData<SNCompositor, SNClassifier>& localData =
        *(this->VC->ThreadMap[threadId]);
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
          this->ProduceSurfaceFaces(threadId, ptId, pItr, cItr);
        } // for all points in this batch
      }   // for all batches
    }     // for all threads
  }       // operator()

  // A factory method to instantiate and threaded execute an instance
  // of SurfaceOutput to produce polygonal output.
  static void Execute(vtkVoronoiCore3D<SNCompositor, SNClassifier>* vc,
    vtkVoronoiCore3D<SNCompositor, SNClassifier>::TopologicalMerge* merge, bool merging,
    bool smoothing, int* regions, int background, vtkPolyData* output)
  {
    // Grab the global surface information.
    const SNCompositor& compositor = vc->Compositor;
    const SNCompositor::vtkCompositeInformation& info = compositor.Information;

    // Create the output dataset arrays (points and cells) and allocate them.
    // The number of points varies depending on whether point merging has been
    // performed.
    vtkIdType NPts = vc->GetNumberOfPoints();
    vtkNew<vtkPoints> outPts;
    outPts->SetDataTypeToDouble();
    std::unique_ptr<PtsWrittenFlags> ptsWritten;
    if (merging || smoothing) // point merging was performed
    {
      ptsWritten = std::make_unique<PtsWrittenFlags>(merge->NumMergedPts, 0);
      outPts->SetNumberOfPoints(merge->NumMergedPts);
    }
    else // no point merging
    {
      outPts->SetNumberOfPoints(info[NPts].NumPts);
    }

    // Instantiate the surface output class.
    SurfaceOutput so(vc, merge, merging, smoothing, regions, background, output, ptsWritten.get());
    so.OutPoints = vtkDoubleArray::FastDownCast(outPts->GetData())->GetPointer(0);

    // The polygonal faces are assembled manually from the connectivity list and
    // offsets.
    vtkNew<vtkIdTypeArray> conn;
    conn->SetNumberOfTuples(info[NPts].ConnSize);
    so.Conn = conn->GetPointer(0);
    vtkNew<vtkIdTypeArray> offsets;
    offsets->SetNumberOfTuples(info[NPts].NumFaces + 1);
    so.ConnOffsets = offsets->GetPointer(0);
    so.ConnOffsets[info[NPts].NumFaces] = info[NPts].ConnSize; // cap off the offsets array

    // Assemble the output
    vtkNew<vtkCellArray> prims;
    prims->SetData(offsets, conn);
    output->SetPoints(outPts);
    output->SetPolys(prims);

    // Generate the output cell data 2-tuple, noting region ids on either
    // side of each polygonal face.
    vtkNew<vtkIdTypeArray> cellScalars;
    cellScalars->SetName("Surface Net Scalars");
    cellScalars->SetNumberOfComponents(2);
    cellScalars->SetNumberOfTuples(info[NPts].NumFaces);
    int idx = output->GetCellData()->AddArray(cellScalars);
    output->GetCellData()->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
    so.CellScalars = cellScalars->GetPointer(0);

    // Now parallel thread the creation of the surface output.
    vtkSMPTools::For(0, vc->GetNumberOfThreads(), so);
  };

}; // SurfaceOutput

// This functor transforms a convex polyhedral mesh into triangles.
struct TransformMesh
{
  // The previous (old) output to be triangulated
  vtkIdType* OOPtr;
  vtkIdType* OCPtr;
  vtkIdType* OSPtr;

  // The new triangles and scalars.
  vtkIdType* OPtr;
  vtkIdType* CPtr;
  vtkIdType* SPtr;

  TransformMesh(vtkIdType* ooPtr, vtkIdType* ocPtr, vtkIdType* osPtr, vtkIdType* oPtr,
    vtkIdType* cPtr, vtkIdType* sPtr)
    : OOPtr(ooPtr)
    , OCPtr(ocPtr)
    , OSPtr(osPtr)
    , OPtr(oPtr)
    , CPtr(cPtr)
    , SPtr(sPtr)
  {
  }

  // Triangulate cells. Used a ladder pattern: it's a little better for
  // smoothing.
  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    // Traverse over existing input cells (convex polygons). Transform
    // these into triangles.
    for (; cellId < endCellId; ++cellId)
    {
      // Grab the current points and cell.
      vtkIdType currentOffset = this->OOPtr[cellId];
      vtkIdType nextOffset = this->OOPtr[cellId + 1];
      vtkIdType npts = nextOffset - currentOffset;
      vtkIdType numTris = npts - 2;
      vtkIdType* pts = this->OCPtr + currentOffset;
      vtkIdType* s = this->OSPtr + (2 * cellId);

      // Prepare to generate new triangles.
      vtkIdType* newTris = this->CPtr + 3 * (currentOffset - 2 * cellId);
      vtkIdType* newS = this->SPtr + 2 * (currentOffset - 2 * cellId);

      // Ladder triangulate the cell. Two passes are used: for
      // the left side, and then right side triangles.
      for (int t = 0; t < std::ceil(static_cast<double>(numTris) / 2.0); ++t)
      {
        *newTris++ = pts[t];
        *newTris++ = pts[t + 1];
        *newTris++ = pts[npts - 1 - t];
        *newS++ = s[0];
        *newS++ = s[1];
      }
      for (int t = 0; t < (numTris / 2); ++t)
      {
        *newTris++ = pts[t + 1];
        *newTris++ = pts[npts - 2 - t];
        *newTris++ = pts[npts - 1 - t];
        *newS++ = s[0];
        *newS++ = s[1];
      }
    }
  }

  static void Execute(int outputMeshType, vtkPolyData* output)
  {
    if (outputMeshType == vtkGeneralizedSurfaceNets3D::MESH_TYPE_POLYGONS)
    {
      return; // nothing needs to be done
    }

    // Generate triangles.
    vtkCellArray* outPolys = output->GetPolys();
    vtkIdType numPolys = output->GetNumberOfCells();
    vtkIdTypeArray* outOffsets = vtkIdTypeArray::FastDownCast(outPolys->GetOffsetsArray());
    vtkIdType* ooPtr = outOffsets->GetPointer(0);
    vtkIdTypeArray* outConn = vtkIdTypeArray::FastDownCast(outPolys->GetConnectivityArray());
    vtkIdType* ocPtr = outConn->GetPointer(0);
    vtkIdTypeArray* outScalars = vtkIdTypeArray::FastDownCast(output->GetCellData()->GetScalars());
    vtkIdType* osPtr = outScalars->GetPointer(0);

    // Determine the number of output triangles, and set new offsets. A
    // simple calculation determines the number of output triangles.
    vtkIdType numTris = ooPtr[numPolys] - (2 * numPolys);

    vtkNew<vtkIdTypeArray> offsets;
    offsets->SetNumberOfTuples(numTris + 1);
    vtkIdType* oPtr = offsets->GetPointer(0);
    vtkIdType oId = (-3); // lambda increments then returns
    std::generate(oPtr, (oPtr + numTris + 1), [&oId] { return oId += 3; });
    oPtr[numTris] = 3 * numTris; // cap it off

    vtkNew<vtkIdTypeArray> conn;
    conn->SetNumberOfTuples(numTris * 3);
    vtkIdType* cPtr = conn->GetPointer(0);

    vtkNew<vtkIdTypeArray> cellScalars;
    cellScalars->SetName("Surface Net Scalars");
    cellScalars->SetNumberOfComponents(2);
    cellScalars->SetNumberOfTuples(numTris);
    vtkIdType* sPtr = cellScalars->GetPointer(0);

    // Threaded generate the triangle connectivity and the scalars.
    TransformMesh tm(ooPtr, ocPtr, osPtr, oPtr, cPtr, sPtr);
    vtkSMPTools::For(0, numPolys, 5000, tm);

    // Assemble everything
    vtkNew<vtkCellArray> newPolys;
    newPolys->SetData(offsets, conn);
    output->SetPolys(newPolys);
    int idx = output->GetCellData()->AddArray(cellScalars);
    output->GetCellData()->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
  } // Execute

}; // TransformMesh

} // anonymous namespace

//================= Begin VTK class proper =====================================
//------------------------------------------------------------------------------
// Construct object
vtkGeneralizedSurfaceNets3D::vtkGeneralizedSurfaceNets3D()
{
  this->Labels = vtkSmartPointer<vtkContourValues>::New();
  this->BackgroundLabel = (-100);

  this->BoundaryCapping = true;
  this->MergePoints = true;

  this->Smoothing = true;
  this->Smoother = vtkSmartPointer<vtkConstrainedSmoothingFilter>::New();
  this->Smoother->SetNumberOfIterations(30);
  this->Smoother->SetRelaxationFactor(0.5);
  this->Smoother->SetConstraintDistance(0.01);
  this->GenerateSmoothingStencils = true;
  this->SmoothingConstraints[0] = 0;
  this->SmoothingConstraints[1] = 0;
  this->SmoothingConstraints[2] = 0;
  this->SmoothingConstraints[3] = 0;

  this->OutputMeshType = MESH_TYPE_DEFAULT;

  this->Padding = 0.001;
  this->Validate = false;
  this->Locator = vtkSmartPointer<vtkStaticPointLocator>::New();
  this->Locator->SetNumberOfPointsPerBucket(2);
  this->PointOfInterest = (-1);
  // this->PointsOfInterest empty on instantiation
  this->MaximumNumberOfHullClips = VTK_ID_MAX;
  this->PruneTolerance = 1.0e-13;
  this->BatchSize = 1000;
  this->NumberOfThreads = 0;

  // By default process active point scalars to obtain region ids
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
}

//------------------------------------------------------------------------------
int vtkGeneralizedSurfaceNets3D::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo0 = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPointSet* input = vtkPointSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo0->Get(vtkDataObject::DATA_OBJECT()));

  vtkDebugMacro(<< "Generating 3D Generalized Surface Net");

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
  // A region id < 0 means that the associated point is "outside" and does
  // not contribute to the output. We can use this capability to process a
  // specified "PointOfInterest" (if any). Otherwise, we check the input for
  // segmented regions via a regions ids array.
  //
  // If region ids are provided,  array must be a single component tuple,
  // signed integer of type vtkIntArray with the number of tuples == number
  // of input points. (Implementation note: this could be expanded with
  // templates - not sure its worth the object bloat.)
  vtkSmartPointer<vtkIntArray> regions;

  // Limit processing to points of interested if so specified.
  if ((this->PointOfInterest >= 0 && this->PointOfInterest < numPts) || this->PointsOfInterest)
  {
    regions = vtkSmartPointer<vtkIntArray>::New();
    regions->SetName("PointsOfInterest");
    regions->SetNumberOfTuples(numPts);
    vtkSMPTools::Fill(regions->GetPointer(0), regions->GetPointer(0) + numPts, -100);
    if (this->PointOfInterest >= 0)
    {
      regions->SetValue(this->PointOfInterest, numPts); // mark POI in region numPts
    }
    if (this->PointsOfInterest)
    {
      vtkIdType numPOI = this->PointsOfInterest->GetNumberOfTuples();
      for (vtkIdType i = 0; i < numPOI; ++i)
      {
        vtkIdType poi = this->PointsOfInterest->GetValue(i);
        if (poi >= 0 && poi < numPts)
        {
          regions->SetValue(poi, numPts); // mark POI in region numPts
        }
      }
    }
  }
  else
  {
    vtkDataArray* rIds = this->GetInputArrayToProcess(0, inputVector);
    regions = vtkIntArray::FastDownCast(rIds);
    if (rIds && !regions)
    {
      vtkWarningMacro("Region Ids array must be of type vtkIntArray");
      regions = ConvertRegionLabels(rIds);
    }
    if (regions)
    {
      if (regions->GetNumberOfComponents() > 1)
      {
        vtkErrorMacro("Region Ids must have 1 component");
        regions = nullptr;
      }
    }
    // Surface nets requires region ids
    if (!regions)
    {
      vtkErrorMacro("Region Ids array must be defined");
      return 1;
    }
  }

  // Ensure that segmentation labels have been specified. If not, then they
  // are automatically generated from the region ids. Automatic label
  // generation can be slow....
  double* labels = this->GetLabels();
  vtkIdType numLabels = this->GetNumberOfLabels();
  std::vector<double> autoLabels;
  if (!labels || numLabels <= 0)
  {
    vtkWarningMacro("Automatically generating labels");
    for (vtkIdType i = 0; i < numPts; ++i)
    {
      double regionId = static_cast<double>(regions->GetValue(i));
      if (regionId >= 0 &&
        std::find(autoLabels.begin(), autoLabels.end(), regionId) == autoLabels.end())
      {
        autoLabels.push_back(regionId);
      }
    }
    labels = autoLabels.data();
    if ((numLabels = autoLabels.size()) <= 0)
    {
      vtkErrorMacro("Region ids are all negative (i.e., outside)");
      return 1;
    }
  }

  // Process the points to generate Voronoi information, including the adjacency
  // (wheels and spokes) data structure. Information is also gathered to allocate
  // memory for the output, and then generate the VTK filter output.
  SNCompositor comp(this->BoundaryCapping);
  SNClassifier classifier(regions->GetPointer(0), labels, numLabels);

  auto voro = vtkVoronoiCore3D<SNCompositor, SNClassifier>::Execute(this, this->BatchSize,
    this->Locator, tPoints, padding, this->MaximumNumberOfHullClips, this->Validate,
    this->PruneTolerance, &comp, &classifier);
  this->NumberOfThreads = voro->GetNumberOfThreads();
  this->NumberOfPrunes = voro->GetNumberOfPrunes();

  // If smoothing and/or point merging is requested, composite the
  // topological point 4-tuples, sort them, and then create a point
  // renumbering map. We do this prior to creating output points and
  // surface primitives so that when they are created, we can renumber them
  // appropriately.
  std::unique_ptr<vtkVoronoiCore3D<SNCompositor, SNClassifier>::TopologicalMerge> topoMerge;
  if (this->MergePoints || this->Smoothing)
  {
    topoMerge = vtkVoronoiCore3D<SNCompositor, SNClassifier>::TopologicalMerge::Execute(voro.get());
    // vtkIdType numMergedPts = topoMerge->NumMergedPts;
  }

  // With the information gathered, now build the surface net. This produces output
  // convex polyhedra.
  SurfaceOutput::Execute(voro.get(), topoMerge.get(), this->MergePoints, this->Smoothing,
    regions->GetPointer(0), this->BackgroundLabel, output);

  // For smoothing, it's best to convert the mesh to triangles.
  if ((this->Smoothing && this->OutputMeshType != MESH_TYPE_POLYGONS) ||
    (!this->Smoothing && this->OutputMeshType == MESH_TYPE_TRIANGLES))
  {
    TransformMesh::Execute(this->OutputMeshType, output);
  }

  // If smoothing is enabled, then invoke the filter and shallow copy its output
  // to this filter's output. Make sure there is output.
  if (this->Smoothing && output->GetNumberOfPoints() > 0)
  {
    this->Smoother->SetInputData(output);
    if (this->GenerateSmoothingStencils)
    {
      vtkNew<vtkCellArray> stencils;
      ProduceStencils::Execute(topoMerge.get(), regions->GetPointer(0), output,
        this->SmoothingConstraints, stencils.Get());
      this->Smoother->SetSmoothingStencils(stencils);
    }
    else
    {
      // Let the smoothing filter compute stencils
      this->Smoother->SetSmoothingStencils(nullptr);
    }
    this->Smoother->Update();
    vtkPolyData* smoothOutput = vtkPolyData::SafeDownCast(this->Smoother->GetOutput());
    output->ShallowCopy(smoothOutput);
  } // if Smoothing

  // Make sure the locator returns to a normal processing mode.
  this->Locator->StaticOff();
  this->Locator->FreeSearchStructure();

  return 1;
} // RequestData

//------------------------------------------------------------------------------
int vtkGeneralizedSurfaceNets3D::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  return 1;
}

//------------------------------------------------------------------------------
// Overload standard modified time function. Since users have access to
// the locator, smoother, and contour labels, we need to take into account
// the modified time of each instance.
vtkMTimeType vtkGeneralizedSurfaceNets3D::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  vtkMTimeType labelsTime = this->Labels->GetMTime();
  vtkMTimeType locatorTime = this->Locator->GetMTime();
  vtkMTimeType smootherTime = this->Smoother->GetMTime();

  mTime = (mTime > labelsTime ? mTime : labelsTime);
  mTime = (mTime > locatorTime ? mTime : locatorTime);
  mTime = (mTime > smootherTime ? mTime : smootherTime);

  return mTime;
}

//------------------------------------------------------------------------------
void vtkGeneralizedSurfaceNets3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  this->Labels->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Boundary Capping: " << (this->BoundaryCapping ? "On\n" : "Off\n");
  os << indent << "Merge Points: " << (this->MergePoints ? "On\n" : "Off\n");

  os << indent << "Smoothing: " << (this->Smoothing ? "On\n" : "Off\n");
  os << indent << "Smoother: " << this->Smoother.Get() << endl;
  os << indent
     << "Generate Smoothing Stencils: " << (this->GenerateSmoothingStencils ? "On\n" : "Off\n");

  os << indent << "Output Mesh Type: " << this->OutputMeshType << endl;

  os << indent << "Padding: " << this->Padding << "\n";
  os << indent << "Validate: " << (this->Validate ? "On\n" : "Off\n");
  os << indent << "Locator: " << this->Locator << "\n";
  os << indent << "Point Of Interest: " << this->PointOfInterest << "\n";
  os << indent << "Points Of Interest: " << this->PointsOfInterest << "\n";
  os << indent << "Maximum Number Of Hull Clips: " << this->MaximumNumberOfHullClips << "\n";
  os << indent << "Batch Size: " << this->BatchSize << "\n";
}

VTK_ABI_NAMESPACE_END
