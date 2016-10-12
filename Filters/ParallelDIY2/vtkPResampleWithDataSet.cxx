/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPResampleWithDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPResampleWithDataSet.h"

#include "vtkArrayDispatch.h"
#include "vtkCharArray.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataProbeFilter.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArrayAccessor.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMPI.h"
#include "vtkMPIController.h"
#include "vtkMPICommunicator.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkUnstructuredGrid.h"

#include "vtk_diy2.h"   // must include this before any diy header
VTKDIY2_PRE_INCLUDE
#include VTK_DIY2_HEADER(diy/assigner.hpp)
#include VTK_DIY2_HEADER(diy/link.hpp)
#include VTK_DIY2_HEADER(diy/master.hpp)
#include VTK_DIY2_HEADER(diy/mpi.hpp)
VTKDIY2_POST_INCLUDE

#include <algorithm>
#include <cmath>
#include <vector>


//---------------------------------------------------------------------------
// Algorithm of this filter:
// 1) Compute the bounds of all the blocks of Source.
// 2) Do an all_gather so that all the nodes know all the bounds.
// 3) Using Input blocks' bounds and Source bounds, find the communication
//    neighbors of each node.
// 4) Find and send the Input points that lie inside a neighbor's Source bounds.
//    The search is made faster by using a point lookup structure
//    (RegularPartition or BalancedPartition bellow).
// 5) Perform resampling on local Input blocks.
// 6) Perform resampling on points received from neighbors.
// 7) Send the resampled points back to the neighbors they were received from.
// 8) Receive resampled points from neighbors and update local blocks of output.
//    Since points of a single Input block can overlap multiple Source blocks
//    and since different Source blocks can have different arrays (Partial Arrays),
//    it is possible that the points of an output block will have different arrays.
//    Remove arrays from a block that are not valid for all its points.
//---------------------------------------------------------------------------


vtkStandardNewMacro(vtkPResampleWithDataSet);

vtkCxxSetObjectMacro(vtkPResampleWithDataSet, Controller, vtkMultiProcessController);

//---------------------------------------------------------------------------
vtkPResampleWithDataSet::vtkPResampleWithDataSet()
  : Controller(NULL), UseBalancedPartitionForPointsLookup(false)
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkPResampleWithDataSet::~vtkPResampleWithDataSet()
{
  this->SetController(NULL);
}

//----------------------------------------------------------------------------
void vtkPResampleWithDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->Controller)
  {
    this->Controller->PrintSelf(os, indent);
  }
  os << indent << "Points lookup partitioning: "
     << (this->UseBalancedPartitionForPointsLookup ? "Balanced" : "Regular")
     << endl;
}


namespace {

//----------------------------------------------------------------------------
struct Point
{
  double Position[3];
  vtkIdType PointId;
  int BlockID;
};


//----------------------------------------------------------------------------
class Partition
{
public:
  virtual void CreatePartition(const std::vector<vtkDataSet*> &blocks) = 0;
  virtual void FindPointsInBounds(const double bounds[6],
                                  std::vector<Point> &points) const = 0;
};

// Partitions the points into spatially regular sized bins. The bins may contain
// widely varying number of points.
class RegularPartition : public Partition
{
public:
  void CreatePartition(const std::vector<vtkDataSet*> &blocks)
  {
    // compute the bounds of the composite dataset
    size_t totalNumberOfPoints = 0;
    this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_DOUBLE_MAX;
    this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = VTK_DOUBLE_MIN;
    for (size_t i = 0; i < blocks.size(); ++i)
    {
      vtkDataSet *ds = blocks[i];
      if (!ds)
      {
        continue;
      }

      totalNumberOfPoints += ds->GetNumberOfPoints();
      double bounds[6];
      ds->GetBounds(bounds);

      for (int j = 0; j < 3; ++j)
      {
          this->Bounds[2*j] = std::min(this->Bounds[2*j], bounds[2*j]);
          this->Bounds[2*j + 1] = std::max(this->Bounds[2*j + 1], bounds[2*j + 1]);
      }
    }

    if (totalNumberOfPoints == 0)
    {
      return;
    }

    // compute a regualr partitioning of the space
    int nbins = 1;
    double dim = 0; // the dimensionality of the dataset
    for (int i = 0; i < 3; ++i)
    {
      if ((this->Bounds[2*i + 1] - this->Bounds[2*i]) > 0.0)
      {
        ++dim;
      }
    }
    if (dim != 0.0)
    {
      nbins = static_cast<int>(std::ceil(
        std::pow(static_cast<double>(totalNumberOfPoints), (1.0/dim)) /
        std::pow(static_cast<double>(NUM_POINTS_PER_BIN), (1.0/dim)) ));
    }
    for (int i = 0; i < 3; ++i)
    {
      this->NumBins[i] = ((this->Bounds[2*i + 1] - this->Bounds[2*i]) > 0.0) ?
                         nbins : 1;
      this->BinSize[i] = (this->Bounds[2*i + 1] - this->Bounds[2*i]) /
                         static_cast<double>(NumBins[i]);

      // slightly increase bin size to include points on this->Bounds[2*i]
      double e = 1.0/std::max(1000.0, static_cast<double>(nbins + 1));
      if (this->BinSize[i] > 0.0)
      {
        e *= this->BinSize[i]; // make e relative to binsize
      }
      this->BinSize[i] += e;
    }

    // compute the bin id of each point
    this->Nodes.reserve(totalNumberOfPoints);
    for (size_t i = 0; i < blocks.size(); ++i)
    {
      vtkDataSet *ds = blocks[i];
      if (!ds)
      {
        continue;
      }

      vtkIdType numPts = ds->GetNumberOfPoints();
      for (vtkIdType j = 0; j < numPts; ++j)
      {
        double pos[3];
        ds->GetPoint(j, pos);

        int bin[3];
        bin[0] = static_cast<int>((pos[0] - this->Bounds[0]) / (this->BinSize[0]));
        bin[1] = static_cast<int>((pos[1] - this->Bounds[2]) / (this->BinSize[1]));
        bin[2] = static_cast<int>((pos[2] - this->Bounds[4]) / (this->BinSize[2]));

        Node n;
        n.BinId = bin[0] + this->NumBins[0]*bin[1] + this->NumBins[0]*this->NumBins[1]*bin[2];
        n.Pt.BlockID = i;
        n.Pt.PointId = j;
        std::copy(pos, pos + 3, n.Pt.Position);
        this->Nodes.push_back(n);
      }
    }
    // sort by BinId
    std::sort(this->Nodes.begin(), this->Nodes.end());

    // map from bin id to first node of the bin
    size_t totalBins = this->NumBins[0] * this->NumBins[1] * this->NumBins[2];
    this->Bins.resize(totalBins + 1);
    for (size_t i = 0, j = 0; i <= totalBins; ++i)
    {
      this->Bins[i] = j;
      while (j < totalNumberOfPoints && this->Nodes[j].BinId == i)
      {
        ++j;
      }
    }
  }

  void FindPointsInBounds(const double bounds[6], std::vector<Point> &points) const
  {
    if (this->Nodes.empty())
    {
      return;
    }

    double searchBds[6];
    for (int i = 0; i < 3; ++i)
    {
      searchBds[2*i] = std::max(bounds[2*i], this->Bounds[2*i]);
      searchBds[2*i + 1] = std::min(bounds[2*i + 1], this->Bounds[2*i + 1]);
    }

    int minBin[3], maxBin[3];
    for (int i = 0; i < 3; ++i)
    {
      minBin[i] = static_cast<int>((searchBds[2*i] - this->Bounds[2*i])/(this->BinSize[i]));
      maxBin[i] = static_cast<int>((searchBds[2*i + 1] - this->Bounds[2*i])/(this->BinSize[i]));
    }

    for (int k = minBin[2]; k <= maxBin[2]; ++k)
    {
      bool passAllZ = (k > minBin[2] && k < maxBin[2]);
      for (int j = minBin[1]; j <= maxBin[1]; ++j)
      {
        bool passAllY = (j > minBin[1] && j < maxBin[1]);
        for (int i = minBin[0]; i <= maxBin[0]; ++i)
        {
          bool passAllX = (i > minBin[0] && i < maxBin[0]);

          vtkIdType bid = i + j*this->NumBins[0] + k*this->NumBins[0]*this->NumBins[1];
          size_t binBegin = this->Bins[bid];
          size_t binEnd = this->Bins[bid + 1];
          if (binBegin == binEnd) // empty bin
          {
            continue;
          }
          if (passAllX && passAllY && passAllZ)
          {
            for (size_t p = binBegin; p < binEnd; ++p)
            {
              points.push_back(this->Nodes[p].Pt);
            }
          }
          else
          {
            for (size_t p = binBegin; p < binEnd; ++p)
            {
              const double *pos = this->Nodes[p].Pt.Position;
              if (pos[0] >= searchBds[0] && pos[0] <= searchBds[1] &&
                  pos[1] >= searchBds[2] && pos[1] <= searchBds[3] &&
                  pos[2] >= searchBds[4] && pos[2] <= searchBds[5])
              {
                points.push_back(this->Nodes[p].Pt);
              }
            }
          }
        }
      }
    }
  }

private:
  enum
  {
    NUM_POINTS_PER_BIN = 512
  };

  struct Node
  {
    Point Pt;
    size_t BinId;

    bool operator<(const Node &n) const
    {
      return this->BinId < n.BinId;
    }
  };

  std::vector<Node> Nodes;
  std::vector<size_t> Bins;
  double Bounds[6];
  int NumBins[3];
  double BinSize[3];
};

// Partitions the points into balanced bins. Each bin contains similar number
// of points
class BalancedPartition : public Partition
{
public:
  void CreatePartition(const std::vector<vtkDataSet*> &blocks)
  {
    // count total number of points
    vtkIdType totalNumberOfPoints = 0;
    for (size_t i = 0; i < blocks.size(); ++i)
    {
      totalNumberOfPoints += blocks[i] ? blocks[i]->GetNumberOfPoints() : 0;
    }

    // copy points and compute dataset bounds
    this->Nodes.reserve(totalNumberOfPoints);
    this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_DOUBLE_MAX;
    this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = VTK_DOUBLE_MIN;
    for (size_t i = 0; i < blocks.size(); ++i)
    {
      vtkDataSet *ds = blocks[i];
      if (!ds)
      {
        continue;
      }

      vtkIdType numPts = ds->GetNumberOfPoints();
      for (vtkIdType j = 0; j < numPts; ++j)
      {
        double pos[3];
        ds->GetPoint(j, pos);

        Point pt;
        pt.PointId = j;
        pt.BlockID = i;
        std::copy(pos, pos + 3, pt.Position);
        this->Nodes.push_back(pt);

        for (int k = 0; k < 3; ++k)
        {
          this->Bounds[2*k] = std::min(this->Bounds[2*k], pos[k]);
          this->Bounds[2*k + 1] = std::max(this->Bounds[2*k + 1], pos[k]);
        }
      }
    }

    // approximate number of nodes in the tree
    vtkIdType splitsSize = totalNumberOfPoints/(NUM_POINTS_PER_BIN/2);
    this->Splits.resize(splitsSize);
    this->RecursiveSplit(&this->Nodes[0], &this->Nodes[totalNumberOfPoints],
                         &this->Splits[0], &this->Splits[splitsSize], 0);
  }

  void FindPointsInBounds(const double bounds[6], std::vector<Point> &points) const
  {
    int tag = 0;
    for (int i = 0; i < 3; ++i)
    {
      if (this->Bounds[2*i] > bounds[2*i + 1] || this->Bounds[2*i + 1] < bounds[2*i])
      {
        return;
      }
      tag |= (this->Bounds[2*i] >= bounds[2*i]) ? (1<<(2*i)) : 0;
      tag |= (this->Bounds[2*i + 1] <= bounds[2*i + 1]) ? (1<<(2*i + 1)) : 0;
    }

    vtkIdType numPoints = this->Nodes.size();
    vtkIdType splitSize = this->Splits.size();
    this->RecursiveSearch(bounds, &this->Nodes[0], &this->Nodes[numPoints],
                          &this->Splits[0], &this->Splits[splitSize], 0, tag,
                          points);
  }

private:
  enum
  {
    NUM_POINTS_PER_BIN = 512
  };

  struct PointComp
  {
    PointComp(int axis) : Axis(axis)
    { }

    bool operator()(const Point &p1, const Point& p2) const
    {
      return p1.Position[this->Axis] < p2.Position[this->Axis];
    }

    int Axis;
  };

  void RecursiveSplit(Point *begin, Point *end, double *sbegin, double *send,
                      int level)
  {
    if ((end - begin) <= NUM_POINTS_PER_BIN)
    {
      return;
    }

    int axis = level%3;
    Point *mid = begin + (end - begin)/2;
    std::nth_element(begin, mid, end, PointComp(axis));
    *(sbegin++) = mid->Position[axis];

    double *smid = sbegin + ((send - sbegin)/2);
    this->RecursiveSplit(begin, mid, sbegin, smid, level + 1);
    this->RecursiveSplit(mid, end, smid, send, level + 1);
  }

  void RecursiveSearch(const double bounds[6], const Point *begin, const Point *end,
                       const double *sbegin, const double *send, int level, int tag,
                       std::vector<Point> &points) const
  {
    if (tag == 63)
    {
      points.insert(points.end(), begin, end);
      return;
    }
    if ((end - begin) <= NUM_POINTS_PER_BIN)
    {
      for (; begin != end; ++begin)
      {
        const double *pos = begin->Position;
        if (pos[0] >= bounds[0] && pos[0] <= bounds[1] &&
            pos[1] >= bounds[2] && pos[1] <= bounds[3] &&
            pos[2] >= bounds[4] && pos[2] <= bounds[5])
        {
          points.push_back(*begin);
        }
      }
      return;
    }

    int axis = level%3;
    const Point *mid = begin + (end - begin)/2;
    const double split = *(sbegin++);
    const double *smid = sbegin + ((send - sbegin)/2);
    if (split >= bounds[2*axis])
    {
      int ltag = tag | ((split <= bounds[2*axis + 1]) ? (1<<(2*axis + 1)) : 0);
      this->RecursiveSearch(bounds, begin, mid, sbegin, smid, level + 1, ltag,
                            points);
    }
    if (split <= bounds[2*axis + 1])
    {
      int rtag = tag | ((split >= bounds[2*axis]) ? (1<<(2*axis)) : 0);
      this->RecursiveSearch(bounds, mid, end, smid, send, level + 1, rtag,
                            points);
    }
  }

  std::vector<double> Splits;
  std::vector<Point> Nodes;
  double Bounds[6];
};


//----------------------------------------------------------------------------
// Iterate over each dataset in a composite dataset and execute func
template <typename Functor>
void ForEachDataSetBlock(vtkDataObject *data, const Functor &func)
{
  if (data->IsA("vtkDataSet"))
  {
    func(static_cast<vtkDataSet*>(data));
  }
  else if (data->IsA("vtkCompositeDataSet"))
  {
    vtkCompositeDataSet *composite = static_cast<vtkCompositeDataSet*>(data);

    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(composite->NewIterator());
    for (iter->InitReverseTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      func(static_cast<vtkDataSet*>(iter->GetCurrentDataObject()));
    }
  }
}

// For each valid block add its bounds to boundsArray
struct GetBlockBounds
{
  GetBlockBounds(std::vector<double> &boundsArray) : BoundsArray(&boundsArray)
  { }

  void operator()(vtkDataSet *block) const
  {
    if (block)
    {
      double bounds[6];
      block->GetBounds(bounds);
      this->BoundsArray->insert(this->BoundsArray->end(), bounds, bounds + 6);
    }
  }

  std::vector<double> *BoundsArray;
};

struct FlattenCompositeDataset
{
  FlattenCompositeDataset(std::vector<vtkDataSet*> &blocks) : Blocks(&blocks)
  { }

  void operator()(vtkDataSet *block) const
  {
    this->Blocks->push_back(block);
  }

  std::vector<vtkDataSet*> *Blocks;
};


//----------------------------------------------------------------------------
void CopyDataSetStructure(vtkDataObject *input, vtkDataObject *output)
{
  if (input->IsA("vtkDataSet"))
  {
    static_cast<vtkDataSet*>(output)->CopyStructure(static_cast<vtkDataSet*>(input));
  }
  else if (input->IsA("vtkCompositeDataSet"))
  {
    vtkCompositeDataSet *compositeIn = static_cast<vtkCompositeDataSet*>(input);
    vtkCompositeDataSet *compositeOut = static_cast<vtkCompositeDataSet*>(output);
    compositeOut->CopyStructure(compositeIn);

    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(compositeIn->NewIterator());
    for (iter->InitReverseTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkDataSet *in = static_cast<vtkDataSet*>(iter->GetCurrentDataObject());
      if (in)
      {
        vtkDataSet *out = in->NewInstance();
        out->CopyStructure(in);
        compositeOut->SetDataSet(iter, out);
        out->Delete();
      }
    }
  }
}

inline bool CheckBoundsIntersect(const double b1[6], const double b2[6])
{
  double intersection[6];
  for (int i = 0; i < 3; ++i)
  {
    intersection[2*i] = std::max(b1[2*i], b2[2*i]);
    intersection[2*i + 1] = std::min(b1[2*i + 1], b2[2*i + 1]);
    if ((intersection[2*i + 1] - intersection[2*i]) < 0.0)
    {
      return false;
    }
  }
  return true;
}

// Find all the neighbors that this rank will need to send to and recv from.
// Based on the intersection of this rank's input bounds with remote's source
// bounds.
void FindNeighbors(diy::mpi::communicator comm,
                   std::vector<std::vector<double> > &sourceBounds,
                   std::vector<vtkDataSet*> &inputBlocks,
                   std::vector<int> &neighbors)
{
  for (int gid = 0; gid < comm.size(); ++gid)
  {
    if (gid == comm.rank())
    {
      continue;
    }

    std::vector<double> &boundsArray = sourceBounds[gid];
    for (size_t next = 0; next < boundsArray.size(); next += 6)
    {
      double *sbounds = &boundsArray[next];
      bool intersects = false;
      for (size_t b = 0; b < inputBlocks.size(); ++b)
      {
        vtkDataSet *ds = inputBlocks[b];
        if (ds)
        {
          double *ibounds = ds->GetBounds();
          if ((intersects = CheckBoundsIntersect(sbounds, ibounds)) == true)
          {
            break;
          }
        }
      }
      if (intersects)
      {
        neighbors.push_back(gid);
        break;
      }
    }
  }

  std::vector<std::vector<int> > allNbrs;
  diy::mpi::all_gather(comm, neighbors, allNbrs);
  for (int gid = 0; gid < comm.size(); ++gid)
  {
    if (gid == comm.rank())
    {
      continue;
    }

    std::vector<int> &nbrs = allNbrs[gid];
    if ((std::find(nbrs.begin(), nbrs.end(), comm.rank()) != nbrs.end()) &&
        (std::find(neighbors.begin(), neighbors.end(), gid) == neighbors.end()))
    {
      neighbors.push_back(gid);
    }
  }
}


//----------------------------------------------------------------------------
struct DiyBlock
{
  std::vector<std::vector<double> > SourceBlocksBounds;
  std::vector<vtkDataSet*> InputBlocks;
  std::vector<vtkDataSet*> OutputBlocks;
  Partition *PointsLookup;
};

// Send input points that overlap remote's source bounds
void FindPointsToSend(DiyBlock *block, const diy::Master::ProxyWithLink& cp,
                      void*)
{
  diy::Link *link = cp.link();

  for (int i = 0; i < link->size(); ++i)
  {
    diy::BlockID neighbor = link->target(i);
    std::vector<Point> points;
    std::vector<double> &boundsArray = block->SourceBlocksBounds[neighbor.proc];
    for (size_t next = 0; next < boundsArray.size(); next += 6)
    {
      double *sbounds = &boundsArray[next];
      block->PointsLookup->FindPointsInBounds(sbounds, points);
    }
    if (!points.empty())
    {
      cp.enqueue(neighbor, points);
    }
  }
}


class EnqueueDataArray
{
public:
  EnqueueDataArray(const diy::Master::ProxyWithLink& cp, const diy::BlockID &dest,
                   const char *masks)
    : Proxy(&cp), Dest(dest), Masks(masks)
  { }

  template <typename ArrayType>
  void operator()(ArrayType *array) const
  {
    vtkDataArrayAccessor<ArrayType> accessor(array);

    this->Proxy->enqueue(this->Dest, std::string(array->GetName()));
    this->Proxy->enqueue(this->Dest, array->GetDataType());
    this->Proxy->enqueue(this->Dest, array->GetNumberOfComponents());

    vtkIdType numTuples = array->GetNumberOfTuples();
    int numComponents = array->GetNumberOfComponents();
    for (vtkIdType i = 0; i < numTuples; ++i)
    {
      if (Masks[i])
      {
        for (int j = 0; j < numComponents; ++j)
        {
          this->Proxy->enqueue(this->Dest, accessor.Get(i, j));
        }
      }
    }
  }

private:
  const diy::Master::ProxyWithLink *Proxy;
  diy::BlockID Dest;
  const char *Masks;
};

// Perform resampling of local and remote input points
void PerformResampling(DiyBlock *block, const diy::Master::ProxyWithLink& cp,
                       void *probep)
{
  vtkCompositeDataProbeFilter *prober = static_cast<vtkCompositeDataProbeFilter*>(probep);
  diy::Link *link = cp.link();

  // local points
  for (size_t i = 0; i < block->InputBlocks.size(); ++i)
  {
    vtkDataSet *in = block->InputBlocks[i];
    if (in)
    {
      prober->SetInputData(in);
      prober->Update();
      block->OutputBlocks[i]->DeepCopy(prober->GetOutput());
    }
  }
  // remote points
  for (int i = 0; i < link->size(); ++i)
  {
    diy::BlockID bid = link->target(i);
    if (!cp.incoming(bid.gid))
    {
      continue;
    }

    std::vector<Point> points;
    cp.dequeue(bid.gid, points);

    vtkNew<vtkPoints> pts;
    pts->SetDataTypeToDouble();
    pts->Allocate(points.size());
    for (size_t j = 0; j < points.size(); ++j)
    {
      pts->InsertNextPoint(points[j].Position);
    }

    vtkNew<vtkUnstructuredGrid> ds;
    ds->SetPoints(pts.GetPointer());
    prober->SetInputData(ds.GetPointer());
    prober->Update();
    vtkDataSet *result = prober->GetOutput();


    vtkIdType numberOfValidPoints = prober->GetValidPoints()->GetNumberOfTuples();
    if (numberOfValidPoints == 0)
    {
      continue;
    }

    const char *maskArrayName = prober->GetValidPointMaskArrayName();
    vtkPointData *resPD = result->GetPointData();
    const char *masks = vtkCharArray::SafeDownCast(resPD->GetArray(maskArrayName))->GetPointer(0);

    std::vector<int> blockIds;
    std::vector<vtkIdType> pointIds;
    blockIds.reserve(numberOfValidPoints);
    pointIds.reserve(numberOfValidPoints);
    for (size_t j = 0; j < points.size(); ++j)
    {
      if (masks[j]) // send only valid points
      {
        blockIds.push_back(points[j].BlockID);
        pointIds.push_back(points[j].PointId);
      }
    }

    cp.enqueue(bid, blockIds);
    cp.enqueue(bid, pointIds);

    EnqueueDataArray enqueuer(cp, bid, masks);
    for (vtkIdType j = 0; j < resPD->GetNumberOfArrays(); ++j)
    {
      vtkDataArray *field = resPD->GetArray(j);
      if (!vtkArrayDispatch::Dispatch::Execute(field, enqueuer))
      {
        vtkGenericWarningMacro(<< "Dispatch failed, fallback to vtkDataArray Get/Set");
        enqueuer(field);
      }
    }
  }
}

class DequeueDataArrayTuple
{
public:
  DequeueDataArrayTuple(const diy::Master::ProxyWithLink &proxy, int sourceGID,
                        vtkIdType tuple)
    : Proxy(&proxy), SourceGID(sourceGID), Tuple(tuple)
  { }

  template <typename ArrayType>
  void operator()(ArrayType *array) const
  {
    vtkDataArrayAccessor<ArrayType> accessor(array);
    for (int i = 0; i < array->GetNumberOfComponents(); ++i)
    {
      typename vtkDataArrayAccessor<ArrayType>::APIType val;
      this->Proxy->dequeue(this->SourceGID, val);
      accessor.Set(this->Tuple, i, val);
    }
  }

private:
  const diy::Master::ProxyWithLink *Proxy;
  int SourceGID;
  vtkIdType Tuple;
};

// receive resampled points
void ReceiveResampledPoints(DiyBlock *block, const diy::Master::ProxyWithLink &cp,
                            void *maskArrayNamePtr)
{
  const char *maskArrayName = reinterpret_cast<char*>(maskArrayNamePtr);

  int numBlocks = block->InputBlocks.size();
  std::vector<std::map<std::string, int> > arrayReceiveCounts(numBlocks);
  std::vector<int> receiveFlags(numBlocks);

  diy::Master::IncomingQueues &in = *cp.incoming();
  for (diy::Master::IncomingQueues::iterator i = in.begin(); i != in.end(); ++i)
  {
    if (!i->second)
    {
      continue;
    }

    std::vector<int> blockIds;
    std::vector<vtkIdType> pointIds;
    cp.dequeue(i->first, blockIds);
    cp.dequeue(i->first, pointIds);
    size_t tuplesToRecv = pointIds.size();

    while (i->second)
    {
      std::string name;
      int type;
      int numComponents;
      cp.dequeue(i->first, name);
      cp.dequeue(i->first, type);
      cp.dequeue(i->first, numComponents);

      std::fill(receiveFlags.begin(), receiveFlags.end(), 0);

      for (size_t j = 0; j < tuplesToRecv; ++j)
      {
        receiveFlags[blockIds[j]] = 1; // mark the blocks that have received this array
        vtkDataSet *ds = block->OutputBlocks[blockIds[j]];
        vtkDataArray *da = ds->GetPointData()->GetArray(name.c_str());
        if (!da)
        {
          da = vtkDataArray::CreateDataArray(type);
          da->SetName(name.c_str());
          da->SetNumberOfComponents(numComponents);
          da->SetNumberOfTuples(ds->GetNumberOfPoints());
          if (name == maskArrayName)
          {
            vtkCharArray *maskArray = vtkCharArray::SafeDownCast(da);
            for (vtkIdType k = 0; k < maskArray->GetNumberOfTuples(); ++k)
            {
              maskArray->SetTypedComponent(k, 0, 0);
            }
          }
          ds->GetPointData()->AddArray(da);
        }

        DequeueDataArrayTuple dequeuer(cp, i->first, pointIds[j]);
        if (!vtkArrayDispatch::Dispatch::Execute(da, dequeuer))
        {
          vtkGenericWarningMacro(<< "Dispatch failed, fallback to vtkDataArray Get/Set");
          dequeuer(da);
        }
      }

      for (int j = 0; j < numBlocks; ++j)
      {
        if (receiveFlags[j])
        {
          // track the number of different sources an array was received from
          // for each block.
          ++arrayReceiveCounts[j][name];
        }
      }
    }
  }

  // Discard arrays that were only received from some of the sources. Such arrays
  // will have invalid values for points that have valid masks from other sources.
  for (int i = 0; i < numBlocks; ++i)
  {
    std::map<std::string, int> &recvCnt = arrayReceiveCounts[i];
    int maxCount = 0;
    for (std::map<std::string, int>::iterator it = recvCnt.begin();
         it != recvCnt.end(); ++it)
    {
      maxCount = std::max(maxCount, it->second);
    }
    for (std::map<std::string, int>::iterator it = recvCnt.begin();
         it != recvCnt.end(); ++it)
    {
      if (it->second != maxCount)
      {
        block->OutputBlocks[i]->GetPointData()->RemoveArray(it->first.c_str());
      }
    }
  }
}


//----------------------------------------------------------------------------
inline diy::mpi::communicator GetDiyCommunicator(vtkMPIController *controller)
{
  vtkMPICommunicator *vtkcomm = vtkMPICommunicator::SafeDownCast(
    controller->GetCommunicator());
  return diy::mpi::communicator(*vtkcomm->GetMPIComm()->GetHandle());
}

} // anonymous namespace


//---------------------------------------------------------------------------
int vtkPResampleWithDataSet::RequestData(vtkInformation *request,
                                     vtkInformationVector **inputVector,
                                     vtkInformationVector *outputVector)
{
  vtkMPIController *mpiCont = vtkMPIController::SafeDownCast(this->Controller);
  if (!mpiCont || mpiCont->GetNumberOfProcesses() == 1)
  {
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);


  diy::mpi::communicator comm = GetDiyCommunicator(mpiCont);

  DiyBlock block;  // one diy-block per rank
  int mygid = comm.rank();

  // compute and communicate the bounds of all the source blocks in all the ranks
  vtkDataObject *source = sourceInfo->Get(vtkDataObject::DATA_OBJECT());
  std::vector<double> srcBounds;
  ForEachDataSetBlock(source, GetBlockBounds(srcBounds));
  diy::mpi::all_gather(comm, srcBounds, block.SourceBlocksBounds);

  // copy the input structure to output
  vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject *output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  CopyDataSetStructure(input, output);
  // flatten the composite datasets to make them easier to handle
  ForEachDataSetBlock(input, FlattenCompositeDataset(block.InputBlocks));
  ForEachDataSetBlock(output, FlattenCompositeDataset(block.OutputBlocks));

  // partition the input points, using the user specified partition algorithm,
  // to make it easier to find the set of points inside a bounding-box
  RegularPartition regular;
  BalancedPartition balanced;
  if (this->UseBalancedPartitionForPointsLookup)
  {
    block.PointsLookup = &balanced;
  }
  else
  {
    block.PointsLookup = &regular;
  }
  block.PointsLookup->CreatePartition(block.InputBlocks);

  // find the neighbors of this rank for communication purposes
  std::vector<int> neighbors;
  FindNeighbors(comm, block.SourceBlocksBounds, block.InputBlocks, neighbors);

  diy::Link *link = new diy::Link;
  for (size_t i = 0; i < neighbors.size(); ++i)
  {
    diy::BlockID bid;
    bid.gid = bid.proc = neighbors[i];
    link->add_neighbor(bid);
  }

  diy::Master master(comm, 1);
  master.add(mygid, &block, link);


  this->Prober->SetSourceData(source);
  // find and send local points that overlap remote source blocks
  master.foreach<DiyBlock>(&FindPointsToSend);
  master.exchange();
  // perform resampling on local and remote points
  master.foreach<DiyBlock>(&PerformResampling, this->Prober.GetPointer());
  master.exchange();
  // receive resampled points and set the values in output
  master.foreach<DiyBlock>(&ReceiveResampledPoints,
                           this->Prober->GetValidPointMaskArrayName());

  // mark the blank points and cells of output
  for (size_t i = 0; i < block.OutputBlocks.size(); ++i)
  {
    vtkDataSet *ds = block.OutputBlocks[i];
    if (ds)
    {
      this->SetBlankPointsAndCells(ds);
    }
  }

  return 1;
}
