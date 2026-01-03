// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkVoronoiCore3D.h"

#ifndef vtkVoronoiCore3D_txx
#define vtkVoronoiCore3D_txx

#include "vtkDoubleArray.h"

VTK_ABI_NAMESPACE_BEGIN

//----------------------------------------------------------------------------
// Constructor
template <class TCompositor, class TClassifier>
vtkVoronoiCore3D<TCompositor, TClassifier>::vtkVoronoiCore3D(vtkAlgorithm* filter,
  vtkVoronoiBatchManager& batcher, vtkStaticPointLocator* loc, vtkPoints* inPts, double padding,
  vtkIdType maxClips, bool validate, double pruneTol, TCompositor* comp, TClassifier* cl)
  : Batcher(batcher)
  , Filter(filter)
  , InPoints(inPts)
  , Locator(loc)
  , Padding(padding)
  , MaxClips(maxClips)
  , Validate(validate)
  , NumPrunes(0)
  , NumThreads(0)
  , MaxPoints(0)
  , MaxFaces(0)
{
  // Set up points for processing. The points must be of type double.
  this->NPts = this->InPoints->GetNumberOfPoints();
  this->Points = vtkDoubleArray::FastDownCast(inPts->GetData())->GetPointer(0);
  if (!this->Points)
  {
    vtkGenericWarningMacro("vtkVoronoiCore3D requires input double points");
  }

  // Wheels: one per input generating point. This is transformed
  // into a vector of offsets into the spokes vector for later processing,
  // hence the +1 allocation.
  this->Graph.Wheels.resize(this->NPts + 1, 0); // initialized to zero

  // Information: depending on compositor output, prepare to gather
  // data as the input points are processed.
  this->Compositor.Initialize(this->NPts, comp);

  // The classifier must be updated with the regions.
  this->Classifier.Initialize(cl);

  // Pre-compute some local data in preparation for processing.
  // Define the Voronoi domain by padding out from bounds.
  loc->GetBounds(this->Bounds);
  for (int i = 0; i < 3; ++i)
  {
    this->PaddedBounds[2 * i] = this->Bounds[2 * i] - padding;
    this->PaddedBounds[2 * i + 1] = this->Bounds[2 * i + 1] + padding;
  }

  // Control spoke pruning (used if requested)
  this->PruneTolerance = pruneTol;
}

//----------------------------------------------------------------------------
template <class TCompositor, class TClassifier>
std::unique_ptr<vtkVoronoiCore3D<TCompositor, TClassifier>>
vtkVoronoiCore3D<TCompositor, TClassifier>::Execute(vtkAlgorithm* filter, unsigned int batchSize,
  vtkStaticPointLocator* loc, vtkPoints* inPts, double padding, vtkIdType maxClips, bool validate,
  double pruneTol, TCompositor* comp, TClassifier* cl)
{
  // Set up batch processing and process the data. This
  vtkVoronoiBatchManager batcher(inPts->GetNumberOfPoints(), batchSize);

  // Generate the Voronoi tessellation. The core algorithm contains output
  // information used later.
  auto voro = std::make_unique<vtkVoronoiCore3D<TCompositor, TClassifier>>(
    filter, batcher, loc, inPts, padding, maxClips, validate, pruneTol, comp, cl);

  // Threaded processing of batches of points.
  vtkSMPTools::For(0, batcher.GetNumberOfBatches(), *voro);

  // Optionally validate and repair the tessellation.
  if (validate)
  {
    if (!voro->Graph.Validate())
    {
      // TODO: If not valid, delete invalid spokes, and rebuild the Voronoi
      // tesselation omitting degenerate points.
    }
  }

  return voro;
}

//----------------------------------------------------------------------------
// In support of the vtkSMPTools::For() related methods.
template <class TCompositor, class TClassifier>
void vtkVoronoiCore3D<TCompositor, TClassifier>::Initialize()
{
  // Update the local data instance of the Voronoi Hull class, as well as
  // the classifier.
  vtkVoronoi3DLocalData<TCompositor, TClassifier>& localData = this->LocalData.Local();
  localData.SIter.Initialize(this->Locator);
  localData.Hull.PruneTolerance = this->PruneTolerance;
  localData.Compositor.Initialize(&this->Compositor);
  localData.Classifier.Initialize(&this->Classifier);
}

//----------------------------------------------------------------------------
// Threaded functor invoked by vtkSMPTools.
template <class TCompositor, class TClassifier>
void vtkVoronoiCore3D<TCompositor, TClassifier>::operator()(vtkIdType batchId, vtkIdType endBatchId)
{
  const vtkVoronoiBatchManager& batcher = this->Batcher;
  vtkDist2TupleArray& d2Tuples = this->Dist2Tuples.Local();
  vtkVoronoi3DLocalData<TCompositor, TClassifier>& localData = this->LocalData.Local();
  vtkShellBinIterator& siter = localData.SIter;
  vtkVoronoiHull& hull = localData.Hull;
  TClassifier& classifier = localData.Classifier;
  typename TCompositor::LocalData& compositor = localData.Compositor;
  int& maxPoints = localData.MaxPoints;
  int& maxFaces = localData.MaxFaces;
  int& numPrunes = localData.NumPrunes;
  vtkBatchIdsType& lBatches = localData.LocalBatches;
  vtkVoronoiWheelsType& wheels = this->Graph.Wheels;
  vtkVoronoiSpokesType& lSpokes = localData.LocalSpokes;
  vtkVoronoiAbortCheck abortCheck(batchId, endBatchId, this->Filter);

  // Process the hull generating points in batches. This performs
  // a little better than independent point-by-point processing, and saves
  // some memory as well.
  for (; batchId < endBatchId; ++batchId)
  {
    if (abortCheck(batchId))
    {
      break;
    }

    // Process all points in this batch. Record the batch being
    // processed. Remember that the point ids are contiguous in this
    // batch.
    vtkIdType ptId, endPtId;
    batcher.GetBatchItemRange(batchId, ptId, endPtId);
    const double* x = this->Points + 3 * ptId;

    for (; ptId < endPtId; ++ptId, x += 3)
    {
      // If the generating point is an outside region, we do not need to
      // process this hull.
      if (!classifier.IsInsideRegion(ptId))
      {
        continue;
      }

      // Initialize the Voronoi hull with the padded bounding box.
      hull.Initialize(ptId, x, this->PaddedBounds);

      // If the hull is successfully built, accumulate information in
      // thread local storage.
      int maxClips = (this->MaxClips < this->NPts ? this->MaxClips : this->NPts);
      maxClips = (this->NPts > 1 ? (this->NPts - 1) : 0);
      if (this->BuildHull(hull, &siter, this->Points, maxClips, d2Tuples, numPrunes))
      {
        // Now accumulate the hull-related information in this thread.
        // First gather the neighborhood points (i.e., spokes) that generated
        // this hull. Also classify the spokes - this will be useful later
        // for compositing hull-related geometric information.
        int numSpokes;
        const vtkVoronoiSpoke* spokes =
          classifier.AddAdjacencyInformation(hull, wheels, lSpokes, numSpokes, maxPoints, maxFaces);

        // Collect the local data to be composited / reduced.
        compositor.AddData(hull, numSpokes, spokes);

      } // if hull successfully generated
    }   // for all points in this batch
    lBatches.emplace_back(batchId);
  } // for all batches of points
}

//----------------------------------------------------------------------------
// Final compositing occurs here.
template <class TCompositor, class TClassifier>
void vtkVoronoiCore3D<TCompositor, TClassifier>::Reduce()
{
  // Build the wheels and spokes adjacency information.
  this->NumThreads = 0;
  vtkIdType numSpokes = 0;
  this->MaxPoints = 0;
  this->MaxFaces = 0;
  this->NumPrunes = 0;

  // Gather information along with a prefix sum of some information
  // across all the threads.
  for (auto& localData : this->LocalData)
  {
    this->ThreadMap.push_back(&localData);
    numSpokes += localData.LocalSpokes.size();
    this->NumThreads++;
    this->MaxPoints =
      (localData.MaxPoints > this->MaxPoints ? localData.MaxPoints : this->MaxPoints);
    this->MaxFaces = (localData.MaxFaces > this->MaxFaces ? localData.MaxFaces : this->MaxFaces);
    this->NumPrunes += localData.NumPrunes;
  } // loop over local thread output

  // Prefix sum over wheels to determine spokes offsets,
  // as well total number of spokes. In some cases no spokes are
  // generated, in which case we skip this code.
  if (numSpokes > 0)
  {
    vtkIdType offset, totalSpokes = 0;
    for (vtkIdType id = 0; id < this->NPts; ++id)
    {
      offset = this->Graph.Wheels[id];
      this->Graph.Wheels[id] = totalSpokes;
      totalSpokes += offset;
    }

    // Initialize the adjacency graph
    this->Graph.Initialize(this->NPts, numSpokes);

    // Parallel build the adjacency (wheel and spokes) structure.
    vtkVoronoiCore3D<TCompositor, TClassifier>::ProduceWheelsAndSpokes::Execute(this);
  }

  // Now build the prefix sum for the data added to the compositor.
  this->Compositor.Finalize();

} // Reduce()

//----------------------------------------------------------------------------
// Generate a 3D Voronoi hull by iterative clipping of the hull with nearby
// points.  Termination of the clipping process occurs when the neighboring
// points become "far enough" away from the generating point (i.e., the
// Voronoi Circumflower error metric is satisfied).
template <class TCompositor, class TClassifier>
bool vtkVoronoiCore3D<TCompositor, TClassifier>::BuildHull(vtkVoronoiHull& hull,
  vtkBinIterator* siter, const double* pts, vtkIdType maxClips, vtkDist2TupleArray& results,
  int& numPrunes)
{
  // Ensure there are clips to be performed.
  if (maxClips <= 0)
  {
    return true;
  }

  const double* v;
  vtkIdType numClips = 0;
  vtkIdType numPtIds;

  // Request batches of neighboring points around the generating point from
  // the underlying locator. The batches of points are retrieved in
  // monotonically increasing distance, eventually they get far enough away
  // to terminate the BuildHull process. The returned neighboring points
  // are used to perform half-space clipping of the Voronoi hull. (The
  // original polyhedral hull around the generating point is defined from
  // the bounding box of the domain.) The Voronoi flower and circumflower
  // neighborhood metrics are used to terminate the clipping process.  The
  // Voronoi Flower is the set of all flower petals (i.e., Delaunay
  // circumspheres) located at the Voronoi hull vertices. The Circumflower
  // is the sphere that bounds all petals of the Voronoi flower.

  // Add points until they are outside of the Voronoi flower. Note that
  // in the while() loop below, if the number of points pIds<=0, then all
  // points have been exhausted and the loop is exited. Similarly, if all
  // bins have been iterated over, the loop is exited.
  bool bins = siter->Begin(hull.PtId, hull.X, results);
  while (bins && numClips < maxClips)
  {
    numPtIds = results.size();
    for (vtkIdType i = 0; i < numPtIds && numClips < maxClips; ++i)
    {
      const vtkIdType& ptId = results[i].Id;
      const double& ptRadii2 = results[i].Dist2;
      // check circumflower
      if (hull.InCircumFlower(ptRadii2))
      {
        v = pts + 3 * ptId;
        if (hull.InFlower(v))
        {
          ClipIntersectionStatus retStatus = hull.Clip(ptId, v);
          if (retStatus == ClipIntersectionStatus::Intersection)
          {
            numClips++;
          }
          else if (retStatus == ClipIntersectionStatus::Pruned)
          {
            numPrunes++;
          }
        } // InFlower
      }   // in circumflower
    }     // for all points in the current request

    // See if circumflower radius is less then the radius of the batch of
    // points; if so, the Voronoi hull has been formed.
    if (!hull.InCircumFlower(siter->GetMinD2()))
    {
      break;
    }

    // Grab the next batch of points
    bins = siter->Next(hull.GetCircumFlower2(), hull.GetPetals(), results);
  } // while points still in Voronoi circumflower

  return true;
} // BuildHull

//----------------------------------------------------------------------------
// Add the adjacency information for the specified hull, i.e., the neighboring
// points (or spokes) that created this hull. Place the information into the
// local hull's wheels and spokes data structure, and return a list of this
// hull's spokes and the number of spokes found. Also keep track of the maximum
// number of points and faces created across all hulls processed by this thread.
// Note: the spokes is a thread local SpokeType vector, the returned list
// const Spoke* is ephemeral and only valid until AddAdjacency() is called
// again (within the current thread).
inline const vtkVoronoiSpoke* vtkVoronoiClassifier3D::AddAdjacencyInformation(vtkVoronoiHull& hull,
  vtkVoronoiWheelsType& wheels, vtkVoronoiSpokesType& spokes, int& vtkNotUsed(numSpokes),
  int& maxPoints, int& maxFaces)
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

//----------------------------------------------------------------------------
template <class TCompositor, class TClassifier>
vtkVoronoiCore3D<TCompositor, TClassifier>::ProduceWheelsAndSpokes::ProduceWheelsAndSpokes(
  vtkVoronoiCore3D<TCompositor, TClassifier>* vc)
  : VC(vc)
{
}

//----------------------------------------------------------------------------
template <class TCompositor, class TClassifier>
void vtkVoronoiCore3D<TCompositor, TClassifier>::ProduceWheelsAndSpokes::operator()(
  vtkIdType threadId, vtkIdType endThreadId)
{
  const vtkVoronoiBatchManager& batcher = this->VC->Batcher;
  vtkVoronoiSpoke* spokes;
  vtkIdType numSpokes;

  // Now copy the spokes from thread local into the right (global) spot
  for (; threadId < endThreadId; ++threadId)
  {
    vtkVoronoi3DLocalData<TCompositor, TClassifier>* localData = this->VC->GetThreadData(threadId);

    // Loop over all batches in this thread
    auto spItr = localData->LocalSpokes.begin();
    vtkIdType ptId, endPtId;
    for (auto& batchId : localData->LocalBatches)
    {
      batcher.GetBatchItemRange(batchId, ptId, endPtId);

      // Loop over all contiguous spokes in this batch
      for (; ptId < endPtId; ++ptId)
      {
        spokes = this->VC->Graph.GetSpokes(ptId, numSpokes);
        for (auto i = 0; i < numSpokes; ++i, ++spItr, ++spokes)
        {
          spokes->NeiId = spItr->NeiId;
          spokes->Classification = spItr->Classification;
        }
      } // for all contiguous points in this batch
    }   // for all batches
  }     // across all threads in this batch
} // ProduceWheelsAndSpokes::operator()

//----------------------------------------------------------------------------
// Invoke the threaded process of building wheels and spokes.
template <class TCompositor, class TClassifier>
void vtkVoronoiCore3D<TCompositor, TClassifier>::ProduceWheelsAndSpokes::Execute(
  vtkVoronoiCore3D<TCompositor, TClassifier>* vc)
{
  vtkVoronoiCore3D<TCompositor, TClassifier>::ProduceWheelsAndSpokes genSpokes(vc);
  vtkSMPTools::For(0, vc->NumThreads, genSpokes);
}

//----------------------------------------------------------------------------
// Set up for merging coincident points. Note that the compositor must have a
// data member "TotalNumPts" with vtkCompositeInfo data member NumPts.
template <class TCompositor, class TClassifier>
vtkVoronoiCore3D<TCompositor, TClassifier>::TopologicalMerge::TopologicalMerge(
  vtkVoronoiCore3D<TCompositor, TClassifier>* vc)
  : VC(vc)
{
  vtkIdType totalPts = vc->Compositor.TotalNumPts;
  this->MergeTuples.resize(totalPts);
  this->MergeMap.resize(totalPts);
  this->NumMergedPts = 0;
}

//----------------------------------------------------------------------------
// Composite the topological coordinates in preparation for sorting them. This
// requires processing the topological coordinates, and assigning global point
// tile/hull ids.
template <class TCompositor, class TClassifier>
void vtkVoronoiCore3D<TCompositor, TClassifier>::TopologicalMerge::operator()(
  vtkIdType threadId, vtkIdType endThreadId)
{
  const TCompositor& compositor = this->VC->Compositor;
  const vtkVoronoiBatchManager& batcher = this->VC->Batcher;
  vtkMergeTuples3DType& mergeTuples = this->MergeTuples;
  const typename TCompositor::vtkCompositeInformation& info = compositor.Information;

  for (; threadId < endThreadId; ++threadId)
  {
    // Get the current local thread data. Also get indices into
    // the local data.
    vtkVoronoi3DLocalData<TCompositor, TClassifier>* localData = this->VC->GetThreadData(threadId);
    vtkVoronoiTopoCoords3DType::iterator tItr = localData->Compositor.TopoCoords.begin();

    // Loop over the batches that the current thread processed earlier. The batches
    // are ordered and consistent with the local data. Copy these into a global
    // merge tuples array.
    for (auto& batchId : localData->LocalBatches)
    {
      vtkIdType ptId, endPtId;
      batcher.GetBatchItemRange(batchId, ptId, endPtId);
      for (; ptId < endPtId; ++ptId) // produce tuples for this batch
      {
        vtkIdType numPts = (info[ptId + 1].NumPts - info[ptId].NumPts);
        if (numPts > 0) // process if tuples were produced by this point generator
        {
          vtkIdType startPtId = info[ptId].NumPts;
          vtkIdType pId = startPtId;
          for (int i = 0; i < numPts; ++i, ++pId, ++tItr)
          {
            mergeTuples[pId].Ids = tItr->Ids; // composite 4-tuple
            mergeTuples[pId].PtId = pId;      // assign a global, unmerged point id
          }
        }
      } // for all points in this batch
    }   // for all batches
  }     // for all threads
} // operator()

//----------------------------------------------------------------------------
// Now create the point renumbering map. It maps from the hull vertices
// (which are disconnected and coincident) to the topologically merged
// output points.
template <class TCompositor, class TClassifier>
void vtkVoronoiCore3D<TCompositor, TClassifier>::TopologicalMerge::Reduce()
{
  // Make sure there is data
  vtkMergeTuples3DType& mergeTuples = this->MergeTuples;
  if (mergeTuples.empty())
  {
    return;
  }

  // First we sort the tuples. This will create groups of 4-tuples with
  // the same tuple values. Each group is assigned an id (using a prefix
  // sum) to create the final point map.
  vtkSMPTools::Sort(mergeTuples.begin(), mergeTuples.end());

  // Count the number of merged points. Merged points have the same
  // hull vertex tuple ids.
  vtkIdType numMergedPts = 1;
  vtkVoronoiMergeTuple3D currentMT = mergeTuples[0];
  for (vtkIdType i = 1; i < static_cast<vtkIdType>(mergeTuples.size()); ++i)
  {
    if (currentMT != mergeTuples[i])
    {
      numMergedPts++;
      currentMT = mergeTuples[i];
    }
  } // for all hull vertex merge tuples

  // Update the total number of points in the output.
  this->NumMergedPts = numMergedPts;

  // Allocate merge map, and populate it with the merged point ids.
  vtkMergeMapType& mergeMap = this->MergeMap;

  // Traverse the hull vertex tuples again and record the merged point
  // id of each tuple.
  vtkIdType currentMergeId = 0;
  currentMT = mergeTuples[0];
  for (vtkIdType i = 0; i < static_cast<vtkIdType>(mergeTuples.size()); ++i)
  {
    if (currentMT != mergeTuples[i])
    {
      ++currentMergeId;
      currentMT = mergeTuples[i];
    }
    mergeMap[mergeTuples[i].PtId] = currentMergeId;
  } // for all hull vertex merge tuples
} // Reduce

//----------------------------------------------------------------------------
// Merge topolical tuples to create a map which transforms local thread ids
// to global point ids.
template <class TCompositor, class TClassifier>
std::unique_ptr<typename vtkVoronoiCore3D<TCompositor, TClassifier>::TopologicalMerge>
vtkVoronoiCore3D<TCompositor, TClassifier>::TopologicalMerge::Execute(
  vtkVoronoiCore3D<TCompositor, TClassifier>* vc)
{
  auto merge = std::make_unique<vtkVoronoiCore3D<TCompositor, TClassifier>::TopologicalMerge>(vc);
  vtkSMPTools::For(0, vc->NumThreads, *merge);
  return merge;
}

VTK_ABI_NAMESPACE_END
#endif
