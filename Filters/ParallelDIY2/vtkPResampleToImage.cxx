/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPResampleToImage.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPResampleToImage.h"

#include "vtkArrayDispatch.h"
#include "vtkCharArray.h"
#include "vtkCompositeDataProbeFilter.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArrayAccessor.h"
#include "vtkDataSetAttributes.h"
#include "vtkExtentTranslator.h"
#include "vtkIdList.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMPI.h"
#include "vtkMPIController.h"
#include "vtkMPICommunicator.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"

#include "vtk_diy2.h"   // must include this before any diy header
#include VTK_DIY2_HEADER(diy/assigner.hpp)
#include VTK_DIY2_HEADER(diy/link.hpp)
#include VTK_DIY2_HEADER(diy/master.hpp)
#include VTK_DIY2_HEADER(diy/mpi.hpp)
#include VTK_DIY2_HEADER(diy/reduce.hpp)
#include VTK_DIY2_HEADER(diy/partners/swap.hpp)
#include VTK_DIY2_HEADER(diy/decomposition.hpp)

#include <algorithm>


vtkStandardNewMacro(vtkPResampleToImage);

vtkCxxSetObjectMacro(vtkPResampleToImage, Controller, vtkMultiProcessController);

namespace {

//----------------------------------------------------------------------------
template <typename T, std::size_t Len>
struct Array
{
public:
  T& operator[](std::size_t idx)
  {
    return this->Data[idx];
  }

  const T& operator[](std::size_t idx) const
  {
    return this->Data[idx];
  }

  T* data()
  {
    return this->Data;
  }

  const T* data() const
  {
    return this->Data;
  }

  std::size_t size() const
  {
    return Len;
  }

private:
  T Data[Len];
};


//-----------------------------------------------------------------------------
struct FieldMetaData
{
  std::string Name;
  int DataType;
  int NumComponents;
  int AttributeType;
};

inline void ExtractFieldMetaData(vtkDataSetAttributes *data,
                                 std::vector<FieldMetaData> *metadata)
{
  std::size_t numFields = static_cast<std::size_t>(data->GetNumberOfArrays());
  metadata->resize(numFields);

  for (std::size_t i = 0; i < numFields; ++i)
    {
    FieldMetaData &md = (*metadata)[i];
    vtkDataArray *da = data->GetArray(static_cast<int>(i));

    md.Name = da->GetName();
    md.DataType = da->GetDataType();
    md.NumComponents = da->GetNumberOfComponents();
    md.AttributeType = data->IsArrayAnAttribute(static_cast<int>(i));
    }
}

inline void InitializeFieldData(const std::vector<FieldMetaData> &metadata,
                                vtkIdType numTuples,
                                vtkDataSetAttributes *data)
{
  std::size_t numFields = metadata.size();
  for (std::size_t i = 0; i < numFields; ++i)
    {
    const FieldMetaData &md = metadata[i];
    vtkDataArray *da = vtkDataArray::CreateDataArray(md.DataType);
    da->SetName(md.Name.c_str());
    da->SetNumberOfComponents(md.NumComponents);
    da->SetNumberOfTuples(numTuples);

    double null_value = 0.0;
    for (int j = 0; j < da->GetNumberOfComponents(); ++j)
      {
      da->FillComponent(j, null_value);
      }
    data->AddArray(da);
    da->Delete();

    if (md.AttributeType >= 0)
      {
      data->SetActiveAttribute(static_cast<int>(i), md.AttributeType);
      }
    }
}


//----------------------------------------------------------------------------
class SerializeWorklet
{
public:
  SerializeWorklet(vtkIdType tuple, int numComponents, diy::MemoryBuffer &buffer)
    : Tuple(tuple), NumComponents(numComponents), Buffer(&buffer)
  { }

  template <typename ArrayType>
  void operator()(ArrayType *array) const
  {
    vtkDataArrayAccessor<ArrayType> accessor(array);
    for (int i = 0; i < this->NumComponents; ++i)
      {
      diy::save(*this->Buffer, accessor.Get(this->Tuple, i));
      }
  }

private:
  vtkIdType Tuple;
  int NumComponents;
  diy::MemoryBuffer *Buffer;
};

inline void SerializeFieldData(vtkFieldData *field, vtkIdType tuple,
                               std::vector<char> &bytestream)
{
  diy::MemoryBuffer bb;
  int numFields = field->GetNumberOfArrays();
  for (int i = 0; i < numFields; ++i)
    {
    vtkDataArray *da = field->GetArray(i);
    std::size_t numComponents = static_cast<std::size_t>(da->GetNumberOfComponents());
    SerializeWorklet worklet(tuple, numComponents, bb);
    if (!vtkArrayDispatch::Dispatch::Execute(da, worklet))
      {
      vtkGenericWarningMacro(<< "Dispatch failed, fallback to vtkDataArray Get/Set");
      worklet(da);
      }
    }
  std::swap(bytestream, bb.buffer);
}

class DeserializeWorklet
{
public:
  DeserializeWorklet(vtkIdType tuple, int numComponents, diy::MemoryBuffer &buffer)
    : Tuple(tuple), NumComponents(numComponents), Buffer(&buffer)
  { }

  template <typename ArrayType>
  void operator()(ArrayType *array) const
  {
    vtkDataArrayAccessor<ArrayType> accessor(array);
    for (int i = 0; i < this->NumComponents; ++i)
      {
      typename vtkDataArrayAccessor<ArrayType>::APIType val;
      diy::load(*this->Buffer, val);
      accessor.Set(this->Tuple, i, val);
      }
  }

private:
  vtkIdType Tuple;
  int NumComponents;
  diy::MemoryBuffer *Buffer;
};

inline void DeserializeFieldData(const std::vector<char> &bytestream,
                                 vtkFieldData *field, vtkIdType tuple)
{
  diy::MemoryBuffer bb;
  bb.buffer = bytestream;

  int numFields = field->GetNumberOfArrays();
  for (int i = 0; i < numFields; ++i)
    {
    vtkDataArray *da = field->GetArray(i);
    std::size_t numComponents = static_cast<std::size_t>(da->GetNumberOfComponents());
    DeserializeWorklet worklet(tuple, numComponents, bb);
    if (!vtkArrayDispatch::Dispatch::Execute(da, worklet))
      {
      vtkGenericWarningMacro(<< "Dispatch failed, fallback to vtkDataArray Get/Set");
      worklet(da);
      }
    }
}


//----------------------------------------------------------------------------
struct Point
{
  int idx[3];
  std::vector<char> data;
};


//----------------------------------------------------------------------------
struct Block
{
  std::vector<Point> Points;
  int Extent[6];
};

inline void* CreateBlock()
{
  return new Block;
}

inline void DestroyBlock(void *blockp)
{
  delete static_cast<Block*>(blockp);
}


//---------------------------------------------------------------------------
inline void GetPointsFromImage(vtkImageData *img, const char *maskArrayName,
                               std::vector<Point> *points)
{
  if (img->GetNumberOfPoints() <= 0)
    {
    return;
    }

  vtkPointData *pd = img->GetPointData();
  vtkCharArray *maskArray = vtkCharArray::SafeDownCast(pd->GetArray(maskArrayName));
  char *mask = maskArray->GetPointer(0);

  int extent[6];
  img->GetExtent(extent);
  for (int k = extent[4]; k <= extent[5]; ++k)
    {
    for (int j = extent[2]; j <= extent[3]; ++j)
      {
      for (int i = extent[0]; i <= extent[1]; ++i)
        {
        int ijk[3] = { i, j, k };
        vtkIdType id = img->ComputePointId(ijk);
        if (mask[id])
          {
          Point pt;
          std::copy(ijk, ijk + 3, pt.idx);
          SerializeFieldData(pd, id, pt.data);
          points->push_back(pt);
          }
        }
      }
    }
}

void SetPointsToImage(const std::vector<FieldMetaData> &fieldMetaData,
                      const std::vector<Point> &points, vtkImageData *img)
{
  vtkPointData *pd = img->GetPointData();
  InitializeFieldData(fieldMetaData, img->GetNumberOfPoints(), pd);

  for (std::size_t i = 0; i < points.size(); ++i)
    {
    const Point &p = points[i];
    int idx[3] = { p.idx[0], p.idx[1], p.idx[2] };
    vtkIdType id = img->ComputePointId(idx);
    DeserializeFieldData(p.data, pd, id);
    }
}

//----------------------------------------------------------------------------
inline void ComputeGlobalBounds(diy::mpi::communicator &comm,
                                const double lbounds[6], double gbounds[6])
{
  Array<double, 3> localBoundsMin, localBoundsMax;
  for (std::size_t i = 0; i < 3; ++i)
    {
    localBoundsMin[i] = lbounds[2*i];
    localBoundsMax[i] = lbounds[2*i + 1];
    }

  Array<double, 3> globalBoundsMin, globalBoundsMax;
  diy::mpi::all_reduce(comm, localBoundsMin, globalBoundsMin,
                       diy::mpi::minimum<double>());
  diy::mpi::all_reduce(comm, localBoundsMax, globalBoundsMax,
                       diy::mpi::maximum<double>());

  for (std::size_t i = 0; i < 3; ++i)
    {
    gbounds[2*i] = globalBoundsMin[i];
    gbounds[2*i + 1] = globalBoundsMax[i];
    }
}

inline void GetGlobalFieldMetaData(diy::mpi::communicator &comm,
                                   vtkDataSetAttributes *data,
                                   std::vector<FieldMetaData> *metadata)
{
  std::vector<FieldMetaData> local;
  ExtractFieldMetaData(data, &local);

  // find a process that has field meta data information (choose the process with
  // minimum rank)
  int rank = local.size() ? comm.rank() : comm.size();
  int source;
  diy::mpi::all_reduce(comm, rank, source, diy::mpi::minimum<int>());

  if (source < comm.size()) // atleast one process has field meta data
    {
    diy::MemoryBuffer bb;
    if (comm.rank() == source)
      {
      diy::save(bb, local);
      bb.reset();
      }
    diy::mpi::broadcast(comm, bb.buffer, source);
    diy::load(bb, *metadata);
    }
}


//---------------------------------------------------------------------------
void Redistribute(void* blockp, const diy::ReduceProxy& srp,
                  const diy::RegularSwapPartners& partners)
{
  Block *b = static_cast<Block*>(blockp);
  unsigned round = srp.round();

  // step 1: dequeue all the incoming points and add them to this block's vector
  diy::Master::IncomingQueues &in = *srp.incoming();
  for (diy::Master::IncomingQueues::iterator i = in.begin(); i != in.end(); ++i)
    {
    while (i->second)
      {
      Point pt;
      srp.dequeue(i->first, pt);
      b->Points.push_back(pt);
      }
    }

  // final round
  if (srp.out_link().size() == 0)
    {
    return;
    }

  // find this block's position in the group
  int groupSize = srp.out_link().size();
  int myPos = 0;
  for (; myPos < groupSize; ++myPos)
    {
    if (srp.out_link().target(myPos).gid == srp.gid())
      {
      break;
      }
    }

  // step 2: redistribute this block's points among the blocks in the group
  int axis = partners.dim(round);
  int minIdx = b->Extent[2 * axis];
  int maxIdx = b->Extent[2 * axis + 1];
  int length = (maxIdx - minIdx + 1 + groupSize - 1) / groupSize;

  std::vector<Point> myPoints;
  for (size_t i = 0; i < b->Points.size(); ++i)
    {
    Point &pt = b->Points[i];

    int nlocs = 1;
    int loc[2] = { (pt.idx[axis] - minIdx)/length, 0 };

    // duplicate shared point
    if (((pt.idx[axis] - minIdx)%length == 0) && (loc[0] != 0))
      {
      loc[1] = loc[0] - 1;
      ++nlocs;
      }

    for (int j = 0; j < nlocs; ++j)
      {
      if (loc[j] == myPos)
        {
        myPoints.push_back(pt);
        }
      else
        {
        srp.enqueue(srp.out_link().target(loc[j]), pt);
        }
      }
    }
  b->Points.swap(myPoints);

  // step 3: readjust extents for next round
  b->Extent[2*axis] = minIdx + (length * myPos);
  b->Extent[2*axis + 1] = std::min(b->Extent[2*axis] + length, maxIdx);
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
vtkPResampleToImage::vtkPResampleToImage()
  : Controller(NULL)
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkPResampleToImage::~vtkPResampleToImage()
{
  this->SetController(NULL);
}

//----------------------------------------------------------------------------
void vtkPResampleToImage::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->Controller)
    {
    this->Controller->PrintSelf(os, indent);
    }
}

//---------------------------------------------------------------------------
int vtkPResampleToImage::RequestData(vtkInformation *request,
                                     vtkInformationVector **inputVector,
                                     vtkInformationVector *outputVector)
{
  vtkMPIController *mpiCont = vtkMPIController::SafeDownCast(this->Controller);
  if (!mpiCont || mpiCont->GetNumberOfProcesses() == 1)
    {
    return this->Superclass::RequestData(request, inputVector, outputVector);
    }

  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkImageData *output = vtkImageData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));


  diy::mpi::communicator comm = GetDiyCommunicator(mpiCont);

  double localBounds[6];
  ComputeDataBounds(input, localBounds);

  double samplingBounds[6];
  if (this->UseInputBounds)
    {
    ComputeGlobalBounds(comm, localBounds, samplingBounds);
    }
  else
    {
    std::copy(this->SamplingBounds, this->SamplingBounds + 6, samplingBounds);
    }

  vtkNew<vtkImageData> mypiece;
  this->PerformResampling(input, samplingBounds, true, localBounds,
                          mypiece.GetPointer());


  // Ensure every node has fields' metadata information
  std::vector<FieldMetaData> pointFieldMetaData;
  GetGlobalFieldMetaData(comm, mypiece->GetPointData(), &pointFieldMetaData);

  // perform swap-reduce partitioning on probed points to decompose the domain
  // into non-overlapping rectangular regions
  diy::RoundRobinAssigner assigner(comm.size(), comm.size());

  int *updateExtent = this->GetUpdateExtent();
  diy::DiscreteBounds domain;
  for (int i = 0; i < 3; ++i)
    {
    domain.min[i] = updateExtent[2*i];
    domain.max[i] = updateExtent[2*i + 1];
    }

  diy::Master master(comm, 1, -1, &CreateBlock, &DestroyBlock);

  diy::RegularDecomposer<diy::DiscreteBounds> decomposer(3, domain, comm.size());
  decomposer.decompose(comm.rank(), assigner, master);

  Block *block = master.block<Block>(0);
  std::copy(updateExtent, updateExtent + 6, block->Extent);
  GetPointsFromImage(mypiece.GetPointer(), this->GetMaskArrayName(),
                     &block->Points);

  diy::RegularSwapPartners  partners(decomposer, 2, false);
  diy::reduce(master, assigner, partners, &Redistribute);

  output->SetOrigin(mypiece->GetOrigin());
  output->SetSpacing(mypiece->GetSpacing());
  output->SetExtent(block->Extent);
  SetPointsToImage(pointFieldMetaData, block->Points, output);
  this->SetBlankPointsAndCells(output);

  return 1;
}


//----------------------------------------------------------------------------
namespace diy {

namespace mpi {
namespace detail {

template <class T, std::size_t Len>
struct mpi_datatype<Array<T, Len> >
{
  typedef Array<T, Len> ArrayType;

  static MPI_Datatype datatype() { return get_mpi_datatype<T>(); }
  static const void* address(const ArrayType& x) { return &x[0]; }
  static void* address(ArrayType& x) { return &x[0]; }
  static int count(const ArrayType&) { return Len; }
};

}
} // namespace mpi::detail


template<>
struct Serialization<Point>
{
  static void save(BinaryBuffer& bb, const Point& p)
  {
    diy::save(bb, p.idx, 3);
    diy::save(bb, p.data);
  }

  static void load(BinaryBuffer& bb, Point& p)
  {
    diy::load(bb, p.idx, 3);
    diy::load(bb, p.data);
  }
};

template<>
struct Serialization<FieldMetaData>
{
  static void save(BinaryBuffer& bb, const FieldMetaData& f)
  {
    diy::save(bb, f.Name);
    diy::save(bb, f.DataType);
    diy::save(bb, f.NumComponents);
    diy::save(bb, f.AttributeType);
  }

  static void load(BinaryBuffer& bb, FieldMetaData& f)
  {
    diy::load(bb, f.Name);
    diy::load(bb, f.DataType);
    diy::load(bb, f.NumComponents);
    diy::load(bb, f.AttributeType);
  }
};

} // namespace diy
