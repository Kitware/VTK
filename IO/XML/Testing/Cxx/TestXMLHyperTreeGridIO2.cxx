/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAMRReadWrite.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of vtkSimplePointsReader and vtkSimplePointsWriter
// .SECTION Description

#include "vtkCellData.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridAxisClip.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkNew.h"
#include "vtkRandomHyperTreeGridSource.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkVariant.h"
#include "vtkXMLHyperTreeGridReader.h"
#include "vtkXMLHyperTreeGridWriter.h"

#include <string>

namespace
{
bool AreHTSame(vtkCellData* cd1, vtkHyperTreeGridNonOrientedCursor* cursor1, vtkCellData* cd2,
  vtkHyperTreeGridNonOrientedCursor* cursor2)
{
  if (cursor1->GetGlobalNodeIndex() != cursor2->GetGlobalNodeIndex() ||
    cursor1->IsLeaf() != cursor2->IsLeaf())
  {
    return false;
  }

  vtkIdType id1 = cursor1->GetVertexId(), id2 = cursor2->GetVertexId();
  for (int i = 0; i < cd1->GetNumberOfArrays(); ++i)
  {
    vtkAbstractArray *array1 = cd1->GetAbstractArray(i), *array2 = cd2->GetAbstractArray(i);
    if (!array1->GetVariantValue(id1).IsEqual(array2->GetVariantValue(id2)))
    {
      return false;
    }
  }

  if (cursor1->IsLeaf())
  {
    return true;
  }

  for (int ichild = 0; ichild < cursor1->GetNumberOfChildren(); ++ichild)
  {
    cursor1->ToChild(ichild);
    cursor2->ToChild(ichild);
    AreHTSame(cd1, cursor1, cd2, cursor2);
    cursor1->ToParent();
    cursor2->ToParent();
  }
  return true;
}

bool AreHTGSame(vtkHyperTreeGrid* htg1, vtkHyperTreeGrid* htg2)
{
  if (!htg1 || !htg2 || htg1->GetBranchFactor() != htg2->GetBranchFactor() ||
    htg1->GetDimension() != htg2->GetDimension())
  {
    return false;
  }

  vtkCellData *cd1 = htg1->GetCellData(), *cd2 = htg2->GetCellData();
  if (cd1->GetNumberOfArrays() != cd2->GetNumberOfArrays())
  {
    return false;
  }

  vtkHyperTreeGrid::vtkHyperTreeGridIterator it1, it2;
  htg1->InitializeTreeIterator(it1);
  htg2->InitializeTreeIterator(it2);
  vtkIdType idx1 = 0, idx2 = 0;

  while (it1.GetNextTree(idx1) && it2.GetNextTree(idx2))
  {
    if (idx1 != idx2)
    {
      return false;
    }
    vtkNew<vtkHyperTreeGridNonOrientedCursor> cursor1, cursor2;
    htg1->InitializeNonOrientedCursor(cursor1, false);
    htg2->InitializeNonOrientedCursor(cursor2, false);
    if (!AreHTSame(cd1, cursor1, cd2, cursor2))
    {
      return false;
    }
  }
  return idx1 == idx2;
}
} // namespace

int TestXMLHyperTreeGridIO2(int argc, char* argv[])
{
  const char* tmpstr =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  std::string tdir = tmpstr ? tmpstr : std::string();
  delete[] tmpstr;

  std::string fname = tdir + std::string("/TestXMLHyperTreeGridIO2_Appendedv0.htg");

  vtkNew<vtkRandomHyperTreeGridSource> source;
  source->Update();

  vtkHyperTreeGrid* htgWrite = vtkHyperTreeGrid::SafeDownCast(source->GetOutputDataObject(0));

  std::cout << "Writing TestXMLHyperTreeGridIO2_Appendedv0.htg" << std::endl;
  vtkNew<vtkXMLHyperTreeGridWriter> writer;
  writer->SetFileName(fname.c_str());
  writer->SetDataModeToAppended();
  writer->SetInputData(htgWrite);
  writer->SetDataSetMajorVersion(0);
  writer->Write();

  std::cout << "Reading TestXMLHyperTreeGridIO2_Appendedv0.htg" << std::endl;
  vtkNew<vtkXMLHyperTreeGridReader> reader;
  reader->SetFileName(fname.c_str());
  reader->Update();

  vtkHyperTreeGrid* htgRead = vtkHyperTreeGrid::SafeDownCast(reader->GetOutputDataObject(0));

  if (!AreHTGSame(htgWrite, htgRead))
  {
    std::cerr << "Appended Write and Read version 0 failed" << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Writing TestXMLHyperTreeGridIO2_Appendedv1.htg" << std::endl;
  fname = tdir + std::string("/TestXMLHyperTreeGridIO2_Appendedv1.htg");
  writer->SetDataSetMajorVersion(1);
  writer->SetFileName(fname.c_str());
  writer->Write();

  std::cout << "Reading TestXMLHyperTreeGridIO2_Appendedv1.htg" << std::endl;
  reader->SetFileName(fname.c_str());
  reader->Update();
  htgRead = vtkHyperTreeGrid::SafeDownCast(reader->GetOutputDataObject(0));
  if (!AreHTGSame(htgWrite, htgRead))
  {
    std::cerr << "Appended Write and Read version 1 failed" << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Writing TestXMLHyperTreeGridIO2_Binaryv0.htg" << std::endl;
  fname = tdir + std::string("/TestXMLHyperTreeGridIO2_Binaryv0.htg");
  writer->SetDataSetMajorVersion(0);
  writer->SetFileName(fname.c_str());
  writer->SetDataModeToBinary();
  writer->Write();

  std::cout << "Reading TestXMLHyperTreeGridIO2_Binaryv0.htg" << std::endl;
  reader->SetFileName(fname.c_str());
  reader->Update();
  htgRead = vtkHyperTreeGrid::SafeDownCast(reader->GetOutputDataObject(0));
  if (!AreHTGSame(htgWrite, htgRead))
  {
    std::cerr << "Binary Write and Read version 0 failed" << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Writing TestXMLHyperTreeGridIO2_Binaryv1.htg" << std::endl;
  fname = tdir + std::string("/TestXMLHyperTreeGridIO2_Binaryv1.htg");
  writer->SetDataSetMajorVersion(1);
  writer->SetFileName(fname.c_str());
  writer->Write();

  std::cout << "Reading TestXMLHyperTreeGridIO2_Binaryv1.htg" << std::endl;
  reader->SetFileName(fname.c_str());
  reader->Update();
  htgRead = vtkHyperTreeGrid::SafeDownCast(reader->GetOutputDataObject(0));
  if (!AreHTGSame(htgWrite, htgRead))
  {
    std::cerr << "Binary Write and Read version 1 failed" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;

  // This part tests masking
  //
  // It is currently disabled because the behavior is questionable. More
  // discussion needs to be done to decide if we should either write masked subtrees
  // to disk or consider them dead forever, and save memory.
  //
  // Maybe an option to write or not masked subtree in the writer could be a good
  // way to address that issue
  /*
    vtkNew<vtkHyperTreeGridAxisClip> clip;
    clip->SetInputConnection(source->GetOutputPort(0));
    clip->Update();
    writer->SetInputData(clip->GetOutputDataObject(0));
    htgWrite = vtkHyperTreeGrid::SafeDownCast(clip->GetOutputDataObject(0));

    fname = tdir + std::string("/TestXMLHyperTreeGridIO2_MaskedAppendedv0.htg");
    writer->SetDataSetMajorVersion(0);
    writer->SetDataModeToAppended();
    writer->SetFileName(fname.c_str());
    writer->Write();
    reader->SetFileName(fname.c_str());
    reader->Update();
    htgRead = vtkHyperTreeGrid::SafeDownCast(reader->GetOutputDataObject(0));
    if (!AreHTGSame(htgWrite, htgRead))
    {
      std::cerr << "Masked Appended Write and Read version 0 failed" << std::endl;
      return EXIT_FAILURE;
    }

    fname = tdir + std::string("/TestXMLHyperTreeGridIO2_MaskedAppendedv1.htg");
    writer->SetDataSetMajorVersion(1);
    writer->SetFileName(fname.c_str());
    writer->Write();
    reader->SetFileName(fname.c_str());
    reader->Update();
    htgRead = vtkHyperTreeGrid::SafeDownCast(reader->GetOutputDataObject(0));
    if (!AreHTGSame(htgWrite, htgRead))
    {
      std::cerr << "Masked Appended Write and Read version 1 failed" << std::endl;
      return EXIT_FAILURE;
    }

    fname = tdir + std::string("/TestXMLHyperTreeGridIO2_MaskedBinaryv0.htg");
    writer->SetDataSetMajorVersion(0);
    writer->SetDataModeToBinary();
    writer->SetFileName(fname.c_str());
    writer->Write();
    reader->SetFileName(fname.c_str());
    reader->Update();
    htgRead = vtkHyperTreeGrid::SafeDownCast(reader->GetOutputDataObject(0));
    if (!AreHTGSame(htgWrite, htgRead))
    {
      std::cerr << "Masked Binary Write and Read version 0 failed" << std::endl;
      return EXIT_FAILURE;
    }

    fname = tdir + std::string("/TestXMLHyperTreeGridIO2_MaskedAppendedv1.htg");
    writer->SetDataSetMajorVersion(1);
    writer->SetFileName(fname.c_str());
    writer->Write();
    reader->SetFileName(fname.c_str());
    reader->Update();
    htgRead = vtkHyperTreeGrid::SafeDownCast(reader->GetOutputDataObject(0));
    if (!AreHTGSame(htgWrite, htgRead))
    {
      std::cerr << "Masked Binary Write and Read version 1 failed" << std::endl;
      return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;*/
}
