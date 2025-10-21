// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHierarchicalBinningFilter.h"

#include "vtkArrayDispatch.h"
#include "vtkArrayDispatchDataSetArrayList.h"
#include "vtkDataArrayRange.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkSMPTools.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkHierarchicalBinningFilter);

namespace
{

//------------------------------------------------------------------------------
// Number ^ index: power function for integers.
int power(int number, int level)
{
  if (level == 0)
  {
    return 1;
  }
  int num = number;
  for (int i = 1; i < level; i++)
  {
    number = number * num;
  }
  return number;
}

//------------------------------------------------------------------------------
int GetLevelOffset(int level, int divs[3])
{
  int block = divs[0] * divs[1] * divs[2];
  int offset = 0;
  for (int i = 0; i < level; ++i)
  {
    offset += power(block, i);
  }
  return offset;
}

//------------------------------------------------------------------------------
// The hierarchy of uniform subdivided binning grids.
struct UniformBinning
{
  int Level;
  int Divs[3];
  double Bounds[6];
  int NumBins;     // number of bins in this level of the tree
  int LevelOffset; // offset from root bin

  // These are internal data members used for performance reasons
  double H[3];
  double hX, hY, hZ;
  double fX, fY, fZ, bX, bY, bZ;
  vtkIdType xD, yD, zD, xyD;

  // Construction. Provide the current level, and the global binning
  // divisions, and the global bounds.
  UniformBinning(int level, int divs[3], double bounds[6])
  {
    this->Level = level;

    this->Divs[0] = power(divs[0], level);
    this->Divs[1] = power(divs[1], level);
    this->Divs[2] = power(divs[2], level);
    this->NumBins = this->Divs[0] * this->Divs[1] * this->Divs[2];

    this->Bounds[0] = bounds[0];
    this->Bounds[1] = bounds[1];
    this->Bounds[2] = bounds[2];
    this->Bounds[3] = bounds[3];
    this->Bounds[4] = bounds[4];
    this->Bounds[5] = bounds[5];

    this->H[0] = (this->Bounds[1] - this->Bounds[0]) / static_cast<double>(this->Divs[0]);
    this->H[1] = (this->Bounds[3] - this->Bounds[2]) / static_cast<double>(this->Divs[1]);
    this->H[2] = (this->Bounds[5] - this->Bounds[4]) / static_cast<double>(this->Divs[2]);

    this->LevelOffset = GetLevelOffset(level, divs);

    // Setup internal data members for more efficient processing.
    this->hX = this->H[0];
    this->hY = this->H[1];
    this->hZ = this->H[2];
    this->fX = 1.0 / this->H[0];
    this->fY = 1.0 / this->H[1];
    this->fZ = 1.0 / this->H[2];
    this->bX = this->Bounds[0];
    this->bY = this->Bounds[2];
    this->bZ = this->Bounds[4];
    this->xD = this->Divs[0];
    this->yD = this->Divs[1];
    this->zD = this->Divs[2];
    this->xyD = this->Divs[0] * this->Divs[1];
  }

  //-----------------------------------------------------------------------------
  // Inlined for performance. These function invocations must be called after
  // BuildLocator() is invoked, otherwise the output is indeterminate.
  void GetBinIndices(const double* x, int ijk[3]) const
  {
    // Compute point index. Make sure it lies within range of locator.
    ijk[0] = static_cast<int>(((x[0] - bX) * fX));
    ijk[1] = static_cast<int>(((x[1] - bY) * fY));
    ijk[2] = static_cast<int>(((x[2] - bZ) * fZ));

    ijk[0] = std::clamp<int>(ijk[0], 0, xD - 1);
    ijk[1] = std::clamp<int>(ijk[1], 0, yD - 1);
    ijk[2] = std::clamp<int>(ijk[2], 0, zD - 1);
  }

  //-----------------------------------------------------------------------------
  // The bin offset is used to uniquefy the id across the hierarchy of binning grids
  vtkIdType GetBinIndex(const double* x) const
  {
    int ijk[3];
    this->GetBinIndices(x, ijk);
    return (this->LevelOffset + ijk[0] + ijk[1] * xD + ijk[2] * xyD);
  }

  //-----------------------------------------------------------------------------
  // Get the bounds for a particular bin at this level
  void GetBinBounds(int localBin, double bounds[6])
  {
    int i = localBin % this->xD;
    int j = (localBin / this->xD) % this->yD;
    int k = localBin / this->xyD;
    bounds[0] = this->Bounds[0] + i * hX;
    bounds[1] = bounds[0] + this->hX;
    bounds[2] = this->Bounds[2] + j * hY;
    bounds[3] = bounds[2] + this->hY;
    bounds[4] = this->Bounds[4] + k * hZ;
    bounds[5] = bounds[4] + this->hZ;
  }
};

} // anonymous namespace

//------------------------------------------------------------------------------
// This non-templated class provides virtual functions to simplify the access
// to the templated subclass. Note this is not in anonymous namespace because the
// VTK class refers to it in the header file (PIMPLd).
struct vtkBinTree
{
  vtkPoints* InPts; // the points to be binned
  vtkIdType NumPts;

  int NumLevels;
  int Divs[3];
  double Bounds[6];
  UniformBinning* Tree[VTK_MAX_LEVEL + 1]; // a uniform binning for each level
  int NumBins;                             // the total number of bins (from all levels) in the tree
  int BatchSize;                           // build the offsets in parallel

  int OffsetsType;
  vtkDataArray* OffsetsArray; // container for offset array

  // Construction
  vtkBinTree(
    vtkIdType npts, vtkPoints* pts, int numLevels, int divs[3], double bounds[6], int offsetsType)
    : InPts(pts)
    , NumPts(npts)
    , NumLevels(numLevels)
    , OffsetsType(offsetsType)
  {
    this->Divs[0] = divs[0];
    this->Divs[1] = divs[1];
    this->Divs[2] = divs[2];

    this->Bounds[0] = bounds[0];
    this->Bounds[1] = bounds[1];
    this->Bounds[2] = bounds[2];
    this->Bounds[3] = bounds[3];
    this->Bounds[4] = bounds[4];
    this->Bounds[5] = bounds[5];

    // Build the levels. We create an extra one; it simplifies things later.
    this->NumBins = 0;
    for (int level = 0; level < this->NumLevels; ++level)
    {
      this->Tree[level] = new UniformBinning(level, divs, bounds);
      this->NumBins += this->Tree[level]->NumBins;
    }
    this->Tree[this->NumLevels] = new UniformBinning(this->NumLevels, divs, bounds);

    this->BatchSize = 0;

    this->OffsetsType = offsetsType;
    this->OffsetsArray = nullptr;
  }

  // Virtual functions supporting convenience methods in templated subclass.
  virtual ~vtkBinTree()
  {
    for (int i = 0; i <= this->NumLevels; ++i)
    {
      delete this->Tree[i];
    }
    if (this->OffsetsArray)
    {
      this->OffsetsArray->Delete();
      this->OffsetsArray = nullptr;
    }
  }
  virtual void Execute(vtkPointSet* input, vtkPolyData* output) = 0;
  int GetNumberOfGlobalBins() { return this->NumBins; }
  int GetNumberOfBins(int level) { return this->Tree[level]->NumBins; }
  virtual vtkIdType GetLevelOffset(int level, vtkIdType& npts) = 0;
  virtual vtkIdType GetBinOffset(int globalBin, vtkIdType& npts) = 0;
  virtual vtkIdType GetLocalBinOffset(int level, int localBin, vtkIdType& npts) = 0;
  // Sometimes the global bin needs to be expressed as a local bin number +
  // tree level.
  void TranslateGlobalBinToLocalBin(int globalBin, int& level, int& localBin)
  {
    for (level = this->NumLevels - 1; globalBin < this->Tree[level]->LevelOffset; --level)
    {
    }
    localBin = globalBin - this->Tree[level]->LevelOffset;
  }
  void GetBinBounds(int globalBin, double bounds[6])
  {
    int level, localBin;
    this->TranslateGlobalBinToLocalBin(globalBin, level, localBin);
    this->Tree[level]->GetBinBounds(localBin, bounds);
  }
  void GetLocalBinBounds(int level, int localBin, double bounds[6])
  {
    this->Tree[level]->GetBinBounds(localBin, bounds);
  }

  void ExportMetaData(vtkPolyData* output)
  {
    this->OffsetsArray->SetName("BinOffsets");
    output->GetFieldData()->AddArray(this->OffsetsArray);

    // Bounding box
    vtkDoubleArray* da = vtkDoubleArray::New();
    da->SetName("BinBounds");
    da->SetNumberOfTuples(6);
    for (int i = 0; i < 6; ++i)
    {
      da->SetValue(i, this->Bounds[i]);
    }
    output->GetFieldData()->AddArray(da);
    da->Delete();

    // Divisions
    vtkIntArray* ia = vtkIntArray::New();
    ia->SetName("BinDivisions");
    ia->SetNumberOfTuples(3);
    for (int i = 0; i < 3; ++i)
    {
      ia->SetValue(i, this->Divs[i]);
    }
    output->GetFieldData()->AddArray(ia);
    ia->Delete();
  }
};

//------------------------------------------------------------------------------
// Helper classes to support efficient computing, and threaded execution.
namespace
{

//------------------------------------------------------------------------------
// The following tuple is what is sorted in the map. Note that it is templated
// because depending on the number of points / bins to process we may want
// to use vtkIdType. Otherwise for performance reasons it's best to use an int
// (or other integral type). Typically sort() is 25-30% faster on smaller
// integral types, plus it takes a heck less memory (when vtkIdType is 64-bit
// and int is 32-bit).
template <typename TTuple>
class BinTuple
{
public:
  TTuple PtId; // originating point id
  TTuple Bin;  // i-j-k index into bin space

  // Operator< used to support the subsequent sort operation.
  bool operator<(const BinTuple& tuple) const { return Bin < tuple.Bin; }
};

//------------------------------------------------------------------------------
// This templated class manages the creation of the binning tree. It also
// implements the operator() functors which are supplied to vtkSMPTools for
// threaded processesing.
template <typename TIds>
struct BinTree : public vtkBinTree
{
  BinTuple<TIds>* Map; // the map to be sorted
  TIds* Offsets;       // offsets for each bin into the map

  // Construction
  BinTree(
    vtkIdType npts, vtkPoints* pts, int numLevels, int divs[3], double bounds[6], int offsetsType)
    : vtkBinTree(npts, pts, numLevels, divs, bounds, offsetsType)
  {
    // one extra allocation to simplify traversal
    this->Map = new BinTuple<TIds>[this->NumPts + 1];
    this->Map[this->NumPts].Bin = this->NumBins;
    this->OffsetsArray = vtkAOSDataArrayTemplate<TIds>::New();
    this->OffsetsArray->SetNumberOfTuples(this->NumBins + 1);
    this->Offsets = vtkAOSDataArrayTemplate<TIds>::FastDownCast(this->OffsetsArray)->GetPointer(0);
    this->Offsets[this->NumBins] = this->NumPts;
  }

  // Release allocated memory
  ~BinTree() override
  {
    delete[] this->Map;
    // Offsets data array deleted by superclass
  }

  // The number of point ids in a bin is determined by computing the
  // difference between the offsets into the sorted points array.
  vtkIdType GetNumberOfIds(vtkIdType binNum)
  {
    return (this->Offsets[binNum + 1] - this->Offsets[binNum]);
  }

  // Given a bin number, return the point ids in that bin.
  const BinTuple<TIds>* GetIds(vtkIdType binNum) { return this->Map + this->Offsets[binNum]; }

  // Explicit point representation (e.g., vtkPointSet), faster path
  template <typename T, typename TPts>
  class MapPointsFunctor
  {
  public:
    BinTree<T>* Tree;
    TPts* Points;
    int Thresh[VTK_MAX_LEVEL];

    MapPointsFunctor(BinTree<T>* tree, TPts* pts)
      : Tree(tree)
      , Points(pts)
    {
      for (int i = 0; i < this->Tree->NumLevels; ++i)
      {
        this->Thresh[i] = this->Tree->Tree[i]->LevelOffset;
      }
    }

    void operator()(vtkIdType ptId, vtkIdType end)
    {
      double p[3];
      auto points = vtk::DataArrayTupleRange<3>(this->Points);
      auto x = points.begin() + ptId;
      BinTuple<T>* t = this->Tree->Map + ptId;
      int numLevels = this->Tree->NumLevels;
      int level, idx, numBins = this->Tree->NumBins;
      for (; ptId < end; ++ptId, ++x, ++t)
      {
        t->PtId = ptId;
        x->GetTuple(p);
        idx = ptId % numBins;

        for (level = numLevels - 1; idx < this->Thresh[level]; --level)
        {
        }
        t->Bin = this->Tree->Tree[level]->GetBinIndex(p);
      } // for all points in this batch
    }
  };

  struct MapPointsWorker
  {
    template <typename TPts>
    void operator()(TPts* inPts, BinTree<TIds>* tree)
    {
      MapPointsFunctor<TIds, TPts> functor(tree, inPts);
      vtkSMPTools::For(0, tree->NumPts, functor);
    }
  };

  // A clever way to build offsets in parallel. Basically each thread builds
  // offsets across a range of the sorted map. Recall that offsets are an
  // integral value referring to the locations of the sorted points that
  // reside in each bin.
  template <typename T>
  class MapOffsets
  {
  public:
    BinTree<T>* Tree;
    vtkIdType NumPts;
    int NumBins;
    int BatchSize;

    MapOffsets(BinTree<T>* tree, int numBatches)
      : Tree(tree)
    {
      this->NumPts = this->Tree->NumPts;
      this->NumBins = this->Tree->NumBins;
      this->BatchSize = ceil(static_cast<double>(this->NumPts) / numBatches);
    }

    // Traverse sorted points (i.e., tuples) and update bin offsets.
    void operator()(vtkIdType batch, vtkIdType batchEnd)
    {
      T* offsets = this->Tree->Offsets;
      const BinTuple<T>* curPt = this->Tree->Map + batch * this->BatchSize;
      const BinTuple<T>* endBatchPt = this->Tree->Map + batchEnd * this->BatchSize;
      const BinTuple<T>* endPt = this->Tree->Map + this->NumPts;
      const BinTuple<T>* prevPt;
      endBatchPt = (endBatchPt > endPt ? endPt : endBatchPt);

      // Special case at the very beginning of the mapped points array.  If
      // the first point is in bin# N, then all bins up and including
      // N must refer to the first point.
      if (curPt == this->Tree->Map)
      {
        prevPt = this->Tree->Map;
        std::fill_n(offsets, curPt->Bin + 1, 0); // point to the first points
      } // at the very beginning of the map (sorted points array)

      // We are entering this functor somewhere in the interior of the
      // mapped points array. All we need to do is point to the entry
      // position because we are interested only in prevPt->Bin.
      else
      {
        prevPt = curPt;
      } // else in the middle of a batch

      // Okay we have a starting point for a bin run. Now we can begin
      // filling in the offsets in this batch. A previous thread should
      // have/will have completed the previous and subsequent runs outside
      // of the [batch,batchEnd) range
      for (curPt = prevPt; curPt < endBatchPt;)
      {
        for (; curPt->Bin == prevPt->Bin && curPt <= endBatchPt; ++curPt)
        {
          // advance
        }
        // Fill in any gaps in the offset array
        std::fill_n(offsets + prevPt->Bin + 1, curPt->Bin - prevPt->Bin, curPt - this->Tree->Map);
        prevPt = curPt;
      } // for all batches in this range
    }   // operator()
  };

  //----------------------------------------------------------------------------
  // Copy points to output
  template <typename T, typename TPts>
  struct ShufflePoints
  {
    BinTree<T>* Tree;
    vtkIdType NumPts;
    TPts* InPoints;
    TPts* OutPoints;

    ShufflePoints(BinTree<T>* tree, vtkIdType numPts, TPts* inPts, TPts* outPts)
      : Tree(tree)
      , NumPts(numPts)
      , InPoints(inPts)
      , OutPoints(outPts)
    {
    }

    void operator()(vtkIdType ptId, vtkIdType endPtId)
    {
      BinTuple<TIds>* map = this->Tree->Map + ptId;
      TPts* py = this->OutPoints + 3 * ptId;
      TPts* px;

      for (; ptId < endPtId; ++ptId, ++map)
      {
        px = this->InPoints + 3 * map->PtId;
        *py++ = *px++;
        *py++ = *px++;
        *py++ = *px;
      }
    }
  }; // ShufflePoints

  //----------------------------------------------------------------------------
  // Copy data arrays to output
  template <typename T, typename TInArray, typename TOutArray,
    int TupleSize = vtk::detail::DynamicTupleSize>
  struct ShuffleArrayFunctor
  {
    BinTree<T>* Tree;
    TInArray* InArray;
    TOutArray* OutArray;

    ShuffleArrayFunctor(BinTree<T>* tree, TInArray* in, TOutArray* out)
      : Tree(tree)
      , InArray(in)
      , OutArray(out)
    {
    }

    void operator()(vtkIdType ptId, vtkIdType endPtId)
    {
      BinTuple<TIds>* map = this->Tree->Map + ptId;
      auto in = vtk::DataArrayTupleRange<TupleSize>(this->InArray);
      auto out = vtk::DataArrayTupleRange<TupleSize>(this->OutArray);
      auto y = out.begin() + ptId;

      for (; ptId < endPtId; ++ptId, ++map, ++y)
      {
        auto x = in.begin() + map->PtId;
        *y = *x;
      }
    }
  }; // ShuffleArrayFunctor

  template <typename T, int TupleSize = vtk::detail::DynamicTupleSize>
  struct ShuffleArrayWorker
  {
    template <typename TInArray, typename TOutArray>
    void operator()(TInArray* inArray, TOutArray* outArray, BinTree<T>* tree)
    {
      ShuffleArrayFunctor<T, TInArray, TOutArray, TupleSize> functor(tree, inArray, outArray);
      vtkSMPTools::For(0, outArray->GetNumberOfTuples(), functor);
    }
  };

  // Bin the points, produce output
  void Execute(vtkPointSet* input, vtkPolyData* output) override
  {
    vtkDataArray* inPts = input->GetPoints()->GetData();
    vtkDataArray* outPts = output->GetPoints()->GetData();

    MapPointsWorker mapWorker;
    if (!vtkArrayDispatch::DispatchByArray<vtkArrayDispatch::PointArrays>::Execute(
          inPts, mapWorker, this))
    {
      mapWorker(inPts, this);
    }

    // Now gather the points into contiguous runs in bins
    //
    vtkSMPTools::Sort(this->Map, this->Map + this->NumPts);

    // Build the offsets into the Map. The offsets are the positions of
    // each bin into the sorted list. They mark the beginning of the
    // list of points in each bin. Amazingly, this can be done in
    // parallel.
    //
    int numBatches =
      static_cast<int>(ceil(static_cast<double>(this->NumPts) / (5 * this->NumBins)));
    MapOffsets<TIds> offMapper(this, numBatches);
    vtkSMPTools::For(0, numBatches, offMapper);

    // Put the offset into the output for downstream filters
    this->ExportMetaData(output);

    // Shuffle the points around
    ShuffleArrayWorker<TIds, 3> pointsWorker;
    if (!vtkArrayDispatch::Dispatch2ByArrayWithSameValueType<vtkArrayDispatch::PointArrays,
          vtkArrayDispatch::AOSPointArrays>::Execute(inPts, outPts, pointsWorker, this))
    {
      pointsWorker(inPts, outPts, this);
    }

    // Now shuffle the data arrays
    vtkPointData* inPD = input->GetPointData();
    vtkPointData* outPD = output->GetPointData();
    outPD->CopyAllocate(inPD, this->NumPts);

    for (int i = 0, numArrays = inPD->GetNumberOfArrays(); i < numArrays; ++i)
    {
      vtkDataArray* iArray = inPD->GetArray(i);
      if (iArray)
      {
        const char* name = iArray->GetName();
        vtkDataArray* oArray = outPD->GetArray(name);
        if (!oArray)
        {
          continue;
        }
        oArray->SetNumberOfTuples(this->NumPts);
        ShuffleArrayWorker<TIds> arrayWorker;
        if (!vtkArrayDispatch::Dispatch2ByArrayWithSameValueType<vtkArrayDispatch::PointArrays,
              vtkArrayDispatch::AOSPointArrays>::Execute(iArray, oArray, arrayWorker, this))
        {
          arrayWorker(iArray, oArray, this);
        }
      } // have valid array
    }   // for each candidate array
  }

  vtkIdType GetLevelOffset(int level, vtkIdType& npts) override
  {
    vtkIdType offset = this->Offsets[this->Tree[level]->LevelOffset];
    vtkIdType offset2 = this->Offsets[this->Tree[level + 1]->LevelOffset];

    npts = offset2 - offset;
    return offset;
  }

  vtkIdType GetBinOffset(int globalBin, vtkIdType& npts) override
  {
    vtkIdType offset = this->Offsets[globalBin];
    vtkIdType offset2 = this->Offsets[globalBin + 1];

    npts = offset2 - offset;
    return offset;
  }

  vtkIdType GetLocalBinOffset(int level, int localBin, vtkIdType& npts) override
  {
    vtkIdType offset = this->Offsets[this->Tree[level]->LevelOffset] + localBin;
    vtkIdType offset2 = this->Offsets[this->Tree[level]->LevelOffset] + localBin + 1;

    npts = offset2 - offset;
    return offset;
  }

}; // BinTree

} // anonymous namespace

//================= Begin VTK class proper =======================================
//------------------------------------------------------------------------------
vtkHierarchicalBinningFilter::vtkHierarchicalBinningFilter()
{

  this->NumberOfLevels = 3;
  this->Automatic = true;

  this->Divisions[0] = this->Divisions[1] = this->Divisions[2] = 2;
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = 0.0;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = 1.0;

  this->Tree = nullptr;
}

//------------------------------------------------------------------------------
vtkHierarchicalBinningFilter::~vtkHierarchicalBinningFilter()
{
  if (this->Tree)
  {
    delete this->Tree;
    this->Tree = nullptr;
  }
}

//------------------------------------------------------------------------------
// Produce the output data
int vtkHierarchicalBinningFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPointSet* input = vtkPointSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Check the input
  if (!input || !output)
  {
    return 1;
  }
  vtkIdType numPts = input->GetNumberOfPoints();
  if (numPts < 1)
  {
    return 1;
  }

  // Set up the binning operation
  vtkPoints* inPts = input->GetPoints();
  int dataType = inPts->GetDataType();
  vtkPoints* outPts = inPts->NewInstance();
  outPts->SetDataType(dataType);
  outPts->SetNumberOfPoints(numPts);
  output->SetPoints(outPts);
  outPts->UnRegister(this);

  int numLevels = this->NumberOfLevels;
  int* divs = this->Divisions;
  double* bounds = this->Bounds;

  // If automatic, try and create uniform-sized bins; cubes are ideal.
  if (this->Automatic)
  {
    inPts->GetBounds(this->Bounds);
    double h[3];
    h[0] = this->Bounds[1] - this->Bounds[0];
    h[1] = this->Bounds[3] - this->Bounds[2];
    h[2] = this->Bounds[5] - this->Bounds[4];
    int min = static_cast<int>(std::min_element(h, h + 3) - h);
    divs[min] = (h[min] > 0.0 ? 2 : 1);
    h[min] = (divs[min] == 1 ? 1.0 : h[min]);
    for (int i = 0; i < 3; ++i)
    {
      if (i != min)
      {
        divs[i] = static_cast<int>(std::round(divs[min] * h[i] / h[min]));
        divs[i] = (divs[i] <= 0 ? 1 : divs[i]);
      }
    }
  }

  // Bin the points, produce output
  if (numPts >= VTK_INT_MAX)
  {
    this->Tree = new BinTree<vtkIdType>(numPts, inPts, numLevels, divs, bounds, VTK_ID_TYPE);
    this->Tree->Execute(input, output);
  }
  else
  {
    this->Tree = new BinTree<int>(numPts, inPts, numLevels, divs, bounds, VTK_INT);
    this->Tree->Execute(input, output);
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkHierarchicalBinningFilter::GetNumberOfGlobalBins()
{
  if (this->Tree)
  {
    return this->Tree->GetNumberOfGlobalBins();
  }
  else
  {
    return 0;
  }
}

//------------------------------------------------------------------------------
int vtkHierarchicalBinningFilter::GetNumberOfBins(int level)
{
  if (this->Tree)
  {
    return this->Tree->GetNumberOfBins(level);
  }
  else
  {
    return 0;
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkHierarchicalBinningFilter::GetLevelOffset(int level, vtkIdType& npts)
{
  if (this->Tree)
  {
    return this->Tree->GetLevelOffset(level, npts);
  }
  else
  {
    return -1;
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkHierarchicalBinningFilter::GetBinOffset(int globalBin, vtkIdType& npts)
{
  if (this->Tree)
  {
    return this->Tree->GetBinOffset(globalBin, npts);
  }
  else
  {
    return -1;
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkHierarchicalBinningFilter::GetLocalBinOffset(int level, int localBin, vtkIdType& npts)
{
  if (this->Tree)
  {
    return this->Tree->GetLocalBinOffset(level, localBin, npts);
  }
  else
  {
    return -1;
  }
}

//------------------------------------------------------------------------------
void vtkHierarchicalBinningFilter::GetBinBounds(int globalBin, double bounds[6])
{
  if (this->Tree)
  {
    this->Tree->GetBinBounds(globalBin, bounds);
    return;
  }
}

//------------------------------------------------------------------------------
void vtkHierarchicalBinningFilter::GetLocalBinBounds(int level, int localBin, double bounds[6])
{
  if (this->Tree)
  {
    this->Tree->GetLocalBinBounds(level, localBin, bounds);
  }
}

//------------------------------------------------------------------------------
int vtkHierarchicalBinningFilter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  return 1;
}

//------------------------------------------------------------------------------
void vtkHierarchicalBinningFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Number of Levels: " << this->NumberOfLevels << endl;

  os << indent << "Automatic: " << (this->Automatic ? "On\n" : "Off\n");

  for (int i = 0; i < 6; i++)
  {
    os << indent << "Bounds[" << i << "]: " << this->Bounds[i] << "\n";
  }

  os << indent << "Divisions: (" << this->Divisions[0] << "," << this->Divisions[1] << ","
     << this->Divisions[2] << ")\n";
}
VTK_ABI_NAMESPACE_END
