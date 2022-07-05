/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDIYUtilities.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDIYUtilities.h"

#include "vtkAbstractArray.h"
#include "vtkArrayDispatch.h"
#include "vtkBoundingBox.h"
#include "vtkCellCenters.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArray.h"
#include "vtkDataObjectTypes.h"
#include "vtkFieldData.h"
#include "vtkImageData.h"
#include "vtkImageDataToPointSet.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkRectilinearGrid.h"
#include "vtkRectilinearGridToPointSet.h"
#include "vtkSOADataArrayTemplate.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLDataObjectWriter.h"
#include "vtkXMLGenericDataObjectReader.h"

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPI.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#endif

#include <cassert>
#include <string>
#include <vector>

namespace
{
//==============================================================================
struct SaveArrayWorker
{
  SaveArrayWorker(diy::BinaryBuffer& bb)
    : BB(bb)
  {
  }

  template <class ArrayT>
  void operator()(ArrayT* array)
  {
    using ValueType = vtk::GetAPIType<ArrayT>;
    ValueType* data(nullptr);
    if (array->HasStandardMemoryLayout())
    {
      // get the void pointer, this is OK for standard memory layout
      data = static_cast<ValueType*>(array->GetVoidPointer(0));
    }
    else
    {
      // create a temporary data array for saving.
      data = new ValueType[array->GetNumberOfValues()];

      const auto range = vtk::DataArrayTupleRange(array);
      using ConstTupleRef = typename decltype(range)::ConstTupleReferenceType;
      using ComponentType = typename decltype(range)::ComponentType;

      vtkIdType i(0);
      for (ConstTupleRef tpl : range)
      {
        for (ComponentType comp : tpl)
        {
          data[i++] = comp;
        }
      }
      assert(i == array->GetNumberOfValues());
    }

    diy::save(this->BB, data, array->GetNumberOfValues());
    if (!array->HasStandardMemoryLayout())
    {
      delete[] data;
    }
  }

  diy::BinaryBuffer& BB;
};

//==============================================================================
struct LoadArrayWorker
{
  LoadArrayWorker(diy::BinaryBuffer& bb)
    : BB(bb)
  {
  }

  template <typename ArrayT>
  void operator()(ArrayT* array)
  {
    using ValueType = vtk::GetAPIType<ArrayT>;

    int numberOfComponents;
    vtkIdType numberOfTuples;
    std::string name;
    diy::load(this->BB, numberOfComponents);
    diy::load(this->BB, numberOfTuples);
    diy::load(this->BB, name);

    array->SetNumberOfComponents(numberOfComponents);
    array->SetNumberOfTuples(numberOfTuples);
    array->SetName(name.c_str());

    ValueType* data(nullptr);
    if (array->HasStandardMemoryLayout())
    {
      // get the void pointer, this is OK for standard memory layout
      data = static_cast<ValueType*>(array->GetVoidPointer(0));
    }
    else
    {
      // create a temporary array for loading
      data = new ValueType[numberOfComponents * numberOfTuples];
    }

    diy::load(this->BB, data, array->GetNumberOfValues());

    if (!array->HasStandardMemoryLayout())
    {
      // read the data into the non-standard array.
      auto range = vtk::DataArrayTupleRange(array);

      using TupleRef = typename decltype(range)::TupleReferenceType;
      using ComponentRef = typename decltype(range)::ComponentReferenceType;

      vtkIdType i(0);
      for (TupleRef tpl : range)
      {
        for (ComponentRef comp : tpl)
        {
          comp = data[i++];
        }
      }

      assert(i == numberOfComponents * numberOfTuples);
      delete[] data;
    }
  }

  diy::BinaryBuffer& BB;
};
} // anonymous namespace

static unsigned int vtkDIYUtilitiesCleanupCounter = 0;
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
static vtkMPIController* vtkDIYUtilitiesCleanupMPIController = nullptr;
#endif
vtkDIYUtilitiesCleanup::vtkDIYUtilitiesCleanup()
{
  ++vtkDIYUtilitiesCleanupCounter;
}

vtkDIYUtilitiesCleanup::~vtkDIYUtilitiesCleanup()
{
  if (--vtkDIYUtilitiesCleanupCounter == 0)
  {
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
    if (vtkDIYUtilitiesCleanupMPIController)
    {
      vtkLogF(TRACE, "Cleaning up MPI controller created for DIY filters.");
      vtkDIYUtilitiesCleanupMPIController->Finalize();
      vtkDIYUtilitiesCleanupMPIController->Delete();
      vtkDIYUtilitiesCleanupMPIController = nullptr;
    }
#endif
  }
}

//------------------------------------------------------------------------------
vtkDIYUtilities::vtkDIYUtilities() = default;

//------------------------------------------------------------------------------
vtkDIYUtilities::~vtkDIYUtilities() = default;

//------------------------------------------------------------------------------
void vtkDIYUtilities::InitializeEnvironmentForDIY()
{
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  int mpiOk;
  MPI_Initialized(&mpiOk);
  if (!mpiOk)
  {
    vtkLogF(TRACE,
      "Initializing MPI for DIY filters since process did not do so in an MPI enabled build.");
    assert(vtkDIYUtilitiesCleanupMPIController == nullptr);
    vtkDIYUtilitiesCleanupMPIController = vtkMPIController::New();

    static int argc = 0;
    static char** argv = { nullptr };
    vtkDIYUtilitiesCleanupMPIController->Initialize(&argc, &argv);
  }
#endif
}

//------------------------------------------------------------------------------
diy::mpi::communicator vtkDIYUtilities::GetCommunicator(vtkMultiProcessController* controller)
{
  vtkDIYUtilities::InitializeEnvironmentForDIY();

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkMPICommunicator* vtkcomm =
    vtkMPICommunicator::SafeDownCast(controller ? controller->GetCommunicator() : nullptr);
  return vtkcomm ? diy::mpi::communicator(*vtkcomm->GetMPIComm()->GetHandle())
                 : diy::mpi::communicator(MPI_COMM_SELF);
#else
  (void)controller;
  return diy::mpi::communicator();
#endif
}

//------------------------------------------------------------------------------
void vtkDIYUtilities::AllReduce(diy::mpi::communicator& comm, vtkBoundingBox& bbox)
{
  if (comm.size() > 1)
  {
    std::vector<double> local_minpoint(3), local_maxpoint(3);
    bbox.GetMinPoint(local_minpoint.data());
    bbox.GetMaxPoint(local_maxpoint.data());

    std::vector<double> global_minpoint(3), global_maxpoint(3);
    diy::mpi::all_reduce(comm, local_minpoint, global_minpoint, diy::mpi::minimum<float>());
    diy::mpi::all_reduce(comm, local_maxpoint, global_maxpoint, diy::mpi::maximum<float>());

    bbox.SetMinPoint(global_minpoint.data());
    bbox.SetMaxPoint(global_maxpoint.data());
  }
}

//------------------------------------------------------------------------------
void vtkDIYUtilities::Save(diy::BinaryBuffer& bb, vtkDataArray* array)
{
  if (!array)
  {
    diy::save(bb, static_cast<int>(VTK_VOID));
  }
  else
  {
    diy::save(bb, array->GetDataType());
    diy::save(bb, array->GetNumberOfComponents());
    diy::save(bb, array->GetNumberOfTuples());
    if (array->GetName())
    {
      diy::save(bb, std::string(array->GetName()));
    }
    else
    {
      diy::save(bb, std::string(""));
    }

    SaveArrayWorker worker(bb);
    if (!vtkArrayDispatch::Dispatch::Execute(array, worker))
    {
      worker(array);
    }
  }
}

//------------------------------------------------------------------------------
void vtkDIYUtilities::Save(diy::BinaryBuffer& bb, vtkStringArray* array)
{
  if (!array)
  {
    diy::save(bb, static_cast<int>(VTK_VOID));
  }
  else
  {
    diy::save(bb, static_cast<int>(VTK_STRING));
    diy::save(bb, array->GetNumberOfComponents());
    diy::save(bb, array->GetNumberOfTuples());
    if (array->GetName())
    {
      diy::save(bb, std::string(array->GetName()));
    }
    else
    {
      diy::save(bb, std::string(""));
    }

    for (vtkIdType id = 0; id < array->GetNumberOfValues(); ++id)
    {
      std::string& string = array->GetValue(id);
      diy::save(bb, string);
    }
  }
}

//------------------------------------------------------------------------------
void vtkDIYUtilities::Save(diy::BinaryBuffer& bb, vtkFieldData* fd)
{
  if (!fd)
  {
    diy::save(bb, static_cast<int>(0));
  }
  else
  {
    diy::save(bb, fd->GetNumberOfArrays());
    for (int id = 0; id < fd->GetNumberOfArrays(); ++id)
    {
      vtkAbstractArray* aa = fd->GetAbstractArray(id);
      if (auto da = vtkArrayDownCast<vtkDataArray>(aa))
      {
        diy::save(bb, static_cast<int>(0)); // vtkDataArray flag
        vtkDIYUtilities::Save(bb, da);
      }
      else if (auto sa = vtkArrayDownCast<vtkStringArray>(aa))
      {
        diy::save(bb, static_cast<int>(1)); // vtkStringArray flag
        vtkDIYUtilities::Save(bb, sa);
      }
      else
      {
        vtkLog(ERROR, "Cannot save array of type " << aa->GetClassName());
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkDIYUtilities::Save(diy::BinaryBuffer& bb, vtkDataSet* p)
{
  if (p)
  {
    diy::save(bb, p->GetDataObjectType());
    auto writer = vtkXMLDataObjectWriter::NewWriter(p->GetDataObjectType());
    if (writer)
    {
      writer->WriteToOutputStringOn();
      writer->SetCompressorTypeToLZ4();
      writer->SetEncodeAppendedData(false);
      writer->SetInputDataObject(p);
      writer->Write();
      diy::save(bb, writer->GetOutputString());
      writer->Delete();
    }
    else
    {
      vtkLogF(
        ERROR, "Cannot serialize `%s` yet. Aborting for debugging purposes.", p->GetClassName());
      abort();
    }
  }
  else
  {
    diy::save(bb, static_cast<int>(-1)); // can't be VTK_VOID since VTK_VOID == VTK_POLY_DATA.
  }
}

//------------------------------------------------------------------------------
void vtkDIYUtilities::Load(diy::BinaryBuffer& bb, vtkDataArray*& array)
{
  int type;
  diy::load(bb, type);
  if (type == VTK_VOID)
  {
    array = nullptr;
  }
  else
  {
    array = vtkArrayDownCast<vtkDataArray>(vtkAbstractArray::CreateArray(type));
    LoadArrayWorker worker(bb);

    if (!vtkArrayDispatch::Dispatch::Execute(array, worker))
    {
      worker(array);
    }
  }
}

//------------------------------------------------------------------------------
void vtkDIYUtilities::Load(diy::BinaryBuffer& bb, vtkStringArray*& array)
{
  int type;
  diy::load(bb, type);
  if (type == VTK_VOID)
  {
    array = nullptr;
  }
  else
  {
    array = vtkStringArray::New();

    int numberOfComponents;
    vtkIdType numberOfTuples;
    std::string name;

    diy::load(bb, numberOfComponents);
    diy::load(bb, numberOfTuples);
    diy::load(bb, name);

    array->SetNumberOfComponents(numberOfComponents);
    array->SetNumberOfTuples(numberOfTuples);
    array->SetName(name.c_str());

    vtkIdType numberOfValues = numberOfComponents * numberOfTuples;

    std::string string;
    for (vtkIdType id = 0; id < numberOfValues; ++id)
    {
      diy::load(bb, string);
      array->SetValue(id, string);
    }
  }
}

//------------------------------------------------------------------------------
void vtkDIYUtilities::Load(diy::BinaryBuffer& bb, vtkFieldData*& fd)
{
  int numberOfArrays;
  diy::load(bb, numberOfArrays);
  if (!numberOfArrays)
  {
    fd = nullptr;
  }
  else
  {
    fd = vtkFieldData::New();
    for (int id = 0; id < numberOfArrays; ++id)
    {
      int flag;
      diy::load(bb, flag);
      vtkAbstractArray* aa = nullptr;
      switch (flag)
      {
        case 0: // vtkDataArray flag
        {
          vtkDataArray* array = nullptr;
          vtkDIYUtilities::Load(bb, array);
          aa = array;
          break;
        }
        case 1: // vtkStringArray flag
        {
          vtkStringArray* array = nullptr;
          vtkDIYUtilities::Load(bb, array);
          aa = array;
          break;
        }
        default:
          vtkLog(ERROR, "Error while receiving array: wrong flag: " << flag << ".");
          break;
      }
      if (aa)
      {
        fd->AddArray(aa);
        aa->FastDelete();
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkDIYUtilities::Load(diy::BinaryBuffer& bb, vtkDataSet*& p)
{
  p = nullptr;
  int type;
  diy::load(bb, type);
  if (type == -1)
  {
    p = nullptr;
  }
  else
  {
    std::string data;
    diy::load(bb, data);

    vtkSmartPointer<vtkDataSet> ds;
    if (auto reader = vtkXMLGenericDataObjectReader::CreateReader(type, /*parallel*/ false))
    {
      reader->ReadFromInputStringOn();
      reader->SetInputString(data);
      reader->Update();
      ds = vtkDataSet::SafeDownCast(reader->GetOutputDataObject(0));
    }
    else
    {
      vtkLogF(ERROR, "Currrently type '%d' (%s) is not supported.", type,
        vtkDataObjectTypes::GetClassNameFromTypeId(type));
      // aborting for debugging purposes.
      abort();
    }

    ds->Register(nullptr);
    p = ds.GetPointer();
  }
}

//------------------------------------------------------------------------------
diy::ContinuousBounds vtkDIYUtilities::Convert(const vtkBoundingBox& bbox)
{
  if (bbox.IsValid())
  {
    diy::ContinuousBounds bds(3);
    bds.min[0] = static_cast<float>(bbox.GetMinPoint()[0]);
    bds.min[1] = static_cast<float>(bbox.GetMinPoint()[1]);
    bds.min[2] = static_cast<float>(bbox.GetMinPoint()[2]);
    bds.max[0] = static_cast<float>(bbox.GetMaxPoint()[0]);
    bds.max[1] = static_cast<float>(bbox.GetMaxPoint()[1]);
    bds.max[2] = static_cast<float>(bbox.GetMaxPoint()[2]);
    return bds;
  }
  return diy::ContinuousBounds(3);
}

//------------------------------------------------------------------------------
vtkBoundingBox vtkDIYUtilities::Convert(const diy::ContinuousBounds& bds)
{
  double bounds[6];
  bounds[0] = bds.min[0];
  bounds[1] = bds.max[0];
  bounds[2] = bds.min[1];
  bounds[3] = bds.max[1];
  bounds[4] = bds.min[2];
  bounds[5] = bds.max[2];
  vtkBoundingBox bbox;
  bbox.SetBounds(bounds);
  return bbox;
}

//------------------------------------------------------------------------------
void vtkDIYUtilities::Broadcast(
  diy::mpi::communicator& comm, std::vector<vtkBoundingBox>& boxes, int source)
{
  // FIXME: can't this be made more elegant?
  std::vector<double> raw_bounds;
  if (comm.rank() == source)
  {
    raw_bounds.resize(6 * boxes.size());
    for (size_t cc = 0; cc < boxes.size(); ++cc)
    {
      boxes[cc].GetBounds(&raw_bounds[6 * cc]);
    }
  }
  diy::mpi::broadcast(comm, raw_bounds, source);
  if (comm.rank() != source)
  {
    boxes.resize(raw_bounds.size() / 6);
    for (size_t cc = 0; cc < boxes.size(); ++cc)
    {
      boxes[cc].SetBounds(&raw_bounds[6 * cc]);
    }
  }
}

//------------------------------------------------------------------------------
std::vector<vtkSmartPointer<vtkPoints>> vtkDIYUtilities::ExtractPoints(
  const std::vector<vtkDataSet*>& datasets, bool use_cell_centers)
{
  vtkNew<vtkCellCenters> cellCenterFilter;
  cellCenterFilter->SetVertexCells(false);
  cellCenterFilter->SetCopyArrays(false);

  vtkNew<vtkRectilinearGridToPointSet> convertorRG;
  vtkNew<vtkImageDataToPointSet> convertorID;

  std::vector<vtkSmartPointer<vtkPoints>> all_points;
  for (auto ds : datasets)
  {
    if (use_cell_centers)
    {
      cellCenterFilter->SetInputDataObject(ds);
      cellCenterFilter->Update();
      ds = cellCenterFilter->GetOutput();
    }
    if (auto ps = vtkPointSet::SafeDownCast(ds))
    {
      all_points.emplace_back(ps->GetPoints());
    }
    else if (auto rg = vtkRectilinearGrid::SafeDownCast(ds))
    {
      convertorRG->SetInputDataObject(rg);
      convertorRG->Update();
      all_points.emplace_back(convertorRG->GetOutput()->GetPoints());
    }
    else if (auto id = vtkImageData::SafeDownCast(ds))
    {
      convertorID->SetInputDataObject(id);
      convertorID->Update();
      all_points.emplace_back(convertorID->GetOutput()->GetPoints());
    }
    else
    {
      // need a placeholder for dataset.
      all_points.emplace_back(nullptr);
    }
  }
  return all_points;
}

//------------------------------------------------------------------------------
vtkBoundingBox vtkDIYUtilities::GetLocalBounds(vtkDataObject* dobj)
{
  double bds[6];
  vtkMath::UninitializeBounds(bds);
  if (auto ds = vtkDataSet::SafeDownCast(dobj))
  {
    ds->GetBounds(bds);
  }
  else if (auto cd = vtkCompositeDataSet::SafeDownCast(dobj))
  {
    cd->GetBounds(bds);
  }
  return vtkBoundingBox(bds);
}

//------------------------------------------------------------------------------
void vtkDIYUtilities::Link(
  diy::Master& master, const diy::Assigner& assigner, const std::vector<std::set<int>>& linksMap)
{
  for (int localId = 0; localId < static_cast<int>(linksMap.size()); ++localId)
  {
    const auto& links = linksMap[localId];
    auto l = new diy::Link();
    for (const auto& nid : links)
    {
      l->add_neighbor(diy::BlockID(nid, assigner.rank(nid)));
    }
    master.replace_link(localId, l);
  }
}

//------------------------------------------------------------------------------
void vtkDIYUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
