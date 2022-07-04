/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPartitionedDataSetCollectionConvertors.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkConvertToMultiBlockDataSet.h"
#include "vtkConvertToPartitionedDataSetCollection.h"
#include "vtkDataAssembly.h"
#include "vtkExodusIIReader.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"

#include <vector>
#define VERIFY(x)                                                                                  \
  do                                                                                               \
  {                                                                                                \
    if (!(x))                                                                                      \
    {                                                                                              \
      vtkLogF(ERROR, "Check Failed: '%s'", #x);                                                    \
      return EXIT_FAILURE;                                                                         \
    }                                                                                              \
  } while (false)

int TestPartitionedDataSetCollectionConvertors(int argc, char* argv[])
{
  vtkNew<vtkExodusIIReader> reader;
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/can.ex2");
  reader->SetFileName(fname);
  delete[] fname;

  reader->UpdateInformation();
  std::vector<int> obj_types{
    vtkExodusIIReader::EDGE_BLOCK,
    vtkExodusIIReader::FACE_BLOCK,
    vtkExodusIIReader::ELEM_BLOCK,
    vtkExodusIIReader::NODE_SET,
    vtkExodusIIReader::EDGE_SET,
    vtkExodusIIReader::FACE_SET,
    vtkExodusIIReader::SIDE_SET,
    vtkExodusIIReader::ELEM_SET,
    vtkExodusIIReader::NODE_MAP,
    vtkExodusIIReader::EDGE_MAP,
    vtkExodusIIReader::FACE_MAP,
    vtkExodusIIReader::ELEM_MAP,
  };

  for (const auto type : obj_types)
  {
    for (int cc = 0; cc < reader->GetNumberOfObjects(type); ++cc)
    {
      reader->SetObjectStatus(type, cc, 1);
    }
  }
  reader->Update();

  //-------------------------------------------------------------
  // Test vtkMultiBlockDataSet to vtkPartitionedDataSetCollection.
  //-------------------------------------------------------------
  vtkNew<vtkConvertToPartitionedDataSetCollection> m2p;
  m2p->SetInputDataObject(reader->GetOutputDataObject(0));
  m2p->Update();

  auto ptc = vtkPartitionedDataSetCollection::SafeDownCast(m2p->GetOutputDataObject(0));
  VERIFY(ptc->GetNumberOfPartitionedDataSets() == 5);
  // ptc->Print(cout);
  auto assembly = ptc->GetDataAssembly();
  VERIFY(assembly != nullptr);
  vtkLogF(INFO, "Assembly XML:\n%s", assembly->SerializeToXML(vtkIndent(2)).c_str());

  auto ids = assembly->SelectNodes({ "//*[@label='Element Blocks']" });
  VERIFY(ids.size() == 1);
  VERIFY(assembly->GetDataSetIndices(ids.front()) == std::vector<unsigned int>({ 0, 1 }));

  ids = assembly->SelectNodes({ "//*[@label='Side Sets']" });
  VERIFY(ids.size() == 1);
  VERIFY(assembly->GetDataSetIndices(ids.front()) == std::vector<unsigned int>({ 2 }));

  ids = assembly->SelectNodes({ "//*[@label='Node Sets']" });
  VERIFY(ids.size() == 1);
  VERIFY(assembly->GetDataSetIndices(ids.front()) == std::vector<unsigned int>({ 3, 4 }));

  //-------------------------------------------------------------
  // Test vtkPartitionedDataSetCollection to vtkMultiBlockDataSet.
  // Note, the output vtkMultiBlockDataSet is not same as the original
  // vtkMultiBlockDataSet by design.
  //-------------------------------------------------------------
  vtkNew<vtkConvertToMultiBlockDataSet> p2m;
  p2m->SetInputConnection(m2p->GetOutputPort());
  p2m->Update();

  auto mb = p2m->GetOutput();
  // mb->Print(cout);
  VERIFY(mb != nullptr);
  VERIFY(mb->GetNumberOfBlocks() == 5);
  VERIFY(vtkMultiPieceDataSet::SafeDownCast(mb->GetBlock(0))->GetNumberOfPieces() == 1);
  VERIFY(vtkMultiPieceDataSet::SafeDownCast(mb->GetBlock(1))->GetNumberOfPieces() == 1);
  VERIFY(vtkMultiPieceDataSet::SafeDownCast(mb->GetBlock(2))->GetNumberOfPieces() == 1);
  VERIFY(vtkMultiPieceDataSet::SafeDownCast(mb->GetBlock(3))->GetNumberOfPieces() == 1);
  VERIFY(vtkMultiPieceDataSet::SafeDownCast(mb->GetBlock(4))->GetNumberOfPieces() == 1);

  return EXIT_SUCCESS;
}
