// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkMultiProcessControllerHelper.h"

#include "vtkAppendCompositeDataLeaves.h"
#include "vtkAppendFilter.h"
#include "vtkAppendPartitionedDataSetCollection.h"
#include "vtkAppendPolyData.h"
#include "vtkCompositeDataSet.h"
#include "vtkGraph.h"
#include "vtkImageAppend.h"
#include "vtkImageData.h"
#include "vtkMolecule.h"
#include "vtkMoleculeAppend.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGridAppend.h"
#include "vtkTrivialProducer.h"
#include "vtkUnstructuredGrid.h"

#include <vector>

vtkStandardNewMacro(vtkMultiProcessControllerHelper);
//----------------------------------------------------------------------------
vtkMultiProcessControllerHelper::vtkMultiProcessControllerHelper() = default;

//----------------------------------------------------------------------------
vtkMultiProcessControllerHelper::~vtkMultiProcessControllerHelper() = default;

//----------------------------------------------------------------------------
int vtkMultiProcessControllerHelper::ReduceToAll(vtkMultiProcessController* controller,
  vtkMultiProcessStream& data,
  void (*operation)(vtkMultiProcessStream& A, vtkMultiProcessStream& B), int vtkNotUsed(tag))
{
  int numProcs = controller->GetNumberOfProcesses();
  if (numProcs <= 1)
  {
    return 1;
  }

  auto raw_data = data.GetRawData();
  data.Reset();

  std::vector<vtkIdType> counts(numProcs);
  const auto my_count = static_cast<vtkIdType>(raw_data.size());
  controller->AllGather(&my_count, &counts[0], 1);

  std::vector<vtkIdType> offsets(numProcs, 0);
  for (int cc = 1; cc < numProcs; ++cc)
  {
    offsets[cc] = offsets[cc - 1] + counts[cc - 1];
  }

  std::vector<unsigned char> buffer(offsets.back() + counts.back());

  controller->AllGatherV(&raw_data[0], &buffer[0], my_count, &counts[0], &offsets[0]);

  // now perform pair-wise reduction operation locally.
  data.SetRawData(&buffer[0], static_cast<unsigned int>(counts[0]));
  for (int cc = 1; cc < numProcs; ++cc)
  {
    vtkMultiProcessStream other;
    other.SetRawData(&buffer[offsets[cc]], static_cast<unsigned int>(counts[cc]));
    // operation produces result in the second argument.
    (*operation)(other, data);
  }
  return 1;
}

//-----------------------------------------------------------------------------
vtkDataObject* vtkMultiProcessControllerHelper::MergePieces(
  vtkDataObject** pieces, unsigned int num_pieces)
{
  if (num_pieces == 0)
  {
    return nullptr;
  }

  vtkDataObject* result = pieces[0]->NewInstance();

  std::vector<vtkSmartPointer<vtkDataObject>> piece_vector;
  piece_vector.resize(num_pieces);
  for (unsigned int cc = 0; cc < num_pieces; cc++)
  {
    piece_vector[cc] = pieces[cc];
  }

  if (vtkMultiProcessControllerHelper::MergePieces(piece_vector, result))
  {
    return result;
  }
  result->Delete();
  return nullptr;
}

//-----------------------------------------------------------------------------
bool vtkMultiProcessControllerHelper::MergePieces(
  std::vector<vtkSmartPointer<vtkDataObject>>& pieces, vtkDataObject* result)
{
  if (pieces.empty())
  {
    return false;
  }

  if (pieces.size() == 1)
  {
    result->ShallowCopy(pieces[0]);
    vtkImageData* id = vtkImageData::SafeDownCast(pieces[0]);
    if (id)
    {
      vtkStreamingDemandDrivenPipeline::SetWholeExtent(
        result->GetInformation(), static_cast<vtkImageData*>(pieces[0].GetPointer())->GetExtent());
    }
    return true;
  }

  // PolyData and Unstructured grid need different append filters.
  vtkAlgorithm* appender = nullptr;
  if (vtkPolyData::SafeDownCast(result))
  {
    appender = vtkAppendPolyData::New();
  }
  else if (vtkUnstructuredGrid::SafeDownCast(result))
  {
    appender = vtkAppendFilter::New();
  }
  else if (vtkImageData::SafeDownCast(result))
  {
    vtkImageAppend* ia = vtkImageAppend::New();
    ia->PreserveExtentsOn();
    appender = ia;
  }
  else if (vtkStructuredGrid::SafeDownCast(result))
  {
    appender = vtkStructuredGridAppend::New();
  }
  else if (vtkMolecule::SafeDownCast(result))
  {
    appender = vtkMoleculeAppend::New();
  }
  else if (vtkGraph::SafeDownCast(result))
  {
    vtkGenericWarningMacro("Support for vtkGraph has been depreciated.");
    return false;
  }
  else if (vtkPartitionedDataSetCollection::SafeDownCast(result))
  {
    vtkAppendPartitionedDataSetCollection* apdsc = vtkAppendPartitionedDataSetCollection::New();
    apdsc->AppendFieldDataOn();
    apdsc->SetAppendModeToAppendPartitions();
    appender = apdsc;
  }
  else if (vtkCompositeDataSet::SafeDownCast(result))
  {
    // this only supports composite datasets of polydata and unstructured grids.
    vtkAppendCompositeDataLeaves* cdl = vtkAppendCompositeDataLeaves::New();
    cdl->AppendFieldDataOn();
    appender = cdl;
  }
  else
  {
    vtkGenericWarningMacro(<< result->GetClassName() << " cannot be merged");
    result->ShallowCopy(pieces[0]);
    return false;
  }
  std::vector<vtkSmartPointer<vtkDataObject>>::iterator iter;
  for (iter = pieces.begin(); iter != pieces.end(); ++iter)
  {
    vtkDataSet* ds = vtkDataSet::SafeDownCast(iter->GetPointer());
    if (ds && ds->GetNumberOfPoints() == 0)
    {
      // skip empty pieces.
      continue;
    }
    vtkMolecule* mol = vtkMolecule::SafeDownCast(iter->GetPointer());
    if (mol && mol->GetNumberOfAtoms() == 0)
    {
      continue;
    }

    vtkNew<vtkTrivialProducer> tp;
    tp->SetOutput(iter->GetPointer());
    appender->AddInputConnection(0, tp->GetOutputPort());
  }
  // input connections may be 0, since we skip empty inputs in the loop above.
  if (appender->GetNumberOfInputConnections(0) > 0)
  {
    appender->Update();
    result->ShallowCopy(appender->GetOutputDataObject(0));
  }
  appender->Delete();
  return true;
}

//----------------------------------------------------------------------------
void vtkMultiProcessControllerHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
