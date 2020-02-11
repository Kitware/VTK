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

#include "vtkBoundingBox.h"
#include "vtkCellCenters.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkImageData.h"
#include "vtkImageDataToPointSet.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkRectilinearGrid.h"
#include "vtkRectilinearGridToPointSet.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLDataObjectWriter.h"
#include "vtkXMLImageDataReader.h"
#include "vtkXMLUnstructuredGridReader.h"

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPI.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#endif

#include <cassert>

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

//----------------------------------------------------------------------------
vtkDIYUtilities::vtkDIYUtilities() {}

//----------------------------------------------------------------------------
vtkDIYUtilities::~vtkDIYUtilities() {}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void vtkDIYUtilities::AllReduce(diy::mpi::communicator& comm, vtkBoundingBox& bbox)
{
  if (comm.size() > 1)
  {
    std::vector<double> local_minpoint(3), local_maxpoint(3);
    bbox.GetMinPoint(&local_minpoint[0]);
    bbox.GetMaxPoint(&local_maxpoint[0]);

    std::vector<double> global_minpoint(3), global_maxpoint(3);
    diy::mpi::all_reduce(comm, local_minpoint, global_minpoint, diy::mpi::minimum<float>());
    diy::mpi::all_reduce(comm, local_maxpoint, global_maxpoint, diy::mpi::maximum<float>());

    bbox.SetMinPoint(&global_minpoint[0]);
    bbox.SetMaxPoint(&global_maxpoint[0]);
  }
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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
    switch (type)
    {
      case VTK_UNSTRUCTURED_GRID:
      {
        vtkNew<vtkXMLUnstructuredGridReader> reader;
        reader->ReadFromInputStringOn();
        reader->SetInputString(data);
        reader->Update();
        ds = vtkDataSet::SafeDownCast(reader->GetOutputDataObject(0));
      }
      break;

      case VTK_IMAGE_DATA:
      {
        vtkNew<vtkXMLImageDataReader> reader;
        reader->ReadFromInputStringOn();
        reader->SetInputString(data);
        reader->Update();
        ds = vtkDataSet::SafeDownCast(reader->GetOutputDataObject(0));
      }
      break;
      default:
        // aborting for debugging purposes.
        abort();
    }

    ds->Register(nullptr);
    p = ds.GetPointer();
  }
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
std::vector<vtkDataSet*> vtkDIYUtilities::GetDataSets(vtkDataObject* input)
{
  std::vector<vtkDataSet*> datasets;
  if (auto cd = vtkCompositeDataSet::SafeDownCast(input))
  {
    auto iter = cd->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      if (auto ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject()))
      {
        datasets.push_back(ds);
      }
    }
    iter->Delete();
  }
  else if (auto ds = vtkDataSet::SafeDownCast(input))
  {
    datasets.push_back(ds);
  }

  return datasets;
}

//----------------------------------------------------------------------------
std::vector<vtkSmartPointer<vtkPoints> > vtkDIYUtilities::ExtractPoints(
  const std::vector<vtkDataSet*>& datasets, bool use_cell_centers)
{
  vtkNew<vtkCellCenters> cellCenterFilter;
  cellCenterFilter->SetVertexCells(false);
  cellCenterFilter->SetCopyArrays(false);

  vtkNew<vtkRectilinearGridToPointSet> convertorRG;
  vtkNew<vtkImageDataToPointSet> convertorID;

  std::vector<vtkSmartPointer<vtkPoints> > all_points;
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
      all_points.push_back(ps->GetPoints());
    }
    else if (auto rg = vtkRectilinearGrid::SafeDownCast(ds))
    {
      convertorRG->SetInputDataObject(rg);
      convertorRG->Update();
      all_points.push_back(convertorRG->GetOutput()->GetPoints());
    }
    else if (auto id = vtkImageData::SafeDownCast(ds))
    {
      convertorID->SetInputDataObject(id);
      convertorID->Update();
      all_points.push_back(convertorID->GetOutput()->GetPoints());
    }
    else
    {
      // need a placeholder for dataset.
      all_points.push_back(nullptr);
    }
  }
  return all_points;
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void vtkDIYUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
