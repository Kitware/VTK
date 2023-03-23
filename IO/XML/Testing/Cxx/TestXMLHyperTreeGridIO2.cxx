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
#include "vtkLogger.h"
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
  vtkHyperTreeGridNonOrientedCursor* cursor2, unsigned int maxDepth)
{
  if (cursor1->GetLevel() > maxDepth)
  {
    return true;
  }
  if (cursor1->IsMasked() && cursor2->IsMasked() && cursor1->IsMasked() == cursor2->IsMasked())
  {
    return true;
  }
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
    AreHTSame(cd1, cursor1, cd2, cursor2, maxDepth);
    cursor1->ToParent();
    cursor2->ToParent();
  }
  return true;
}

bool AreHTGSame(vtkHyperTreeGrid* htg1, vtkHyperTreeGrid* htg2, unsigned int maxDepth = UINT_MAX)
{
  if (!htg1 || !htg2 || htg1->GetBranchFactor() != htg2->GetBranchFactor() ||
    htg1->GetDimension() != htg2->GetDimension() || htg2->GetNumberOfLevels() > maxDepth)
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
    if (!AreHTSame(cd1, cursor1, cd2, cursor2, maxDepth))
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

  vtkLog(INFO, "Writing TestXMLHyperTreeGridIO2_Appendedv0.htg");
  vtkNew<vtkXMLHyperTreeGridWriter> writer;
  writer->SetFileName(fname.c_str());
  writer->SetDataModeToAppended();
  writer->SetInputData(htgWrite);
  writer->SetDataSetMajorVersion(0);
  writer->Write();

  vtkLog(INFO, "Reading TestXMLHyperTreeGridIO2_Appendedv0.htg");
  vtkNew<vtkXMLHyperTreeGridReader> reader;
  reader->SetFileName(fname.c_str());
  reader->Update();

  vtkHyperTreeGrid* htgRead = vtkHyperTreeGrid::SafeDownCast(reader->GetOutputDataObject(0));

  if (!AreHTGSame(htgWrite, htgRead))
  {
    vtkLog(ERROR, "Appended Write and Read version 0 failed");
    return EXIT_FAILURE;
  }

  vtkLog(INFO, "Writing TestXMLHyperTreeGridIO2_Appendedv1.htg");
  fname = tdir + std::string("/TestXMLHyperTreeGridIO2_Appendedv1.htg");
  writer->SetDataSetMajorVersion(1);
  writer->SetFileName(fname.c_str());
  writer->Write();

  vtkLog(INFO, "Reading TestXMLHyperTreeGridIO2_Appendedv1.htg");
  reader->SetFileName(fname.c_str());
  reader->Update();
  htgRead = vtkHyperTreeGrid::SafeDownCast(reader->GetOutputDataObject(0));
  if (!AreHTGSame(htgWrite, htgRead))
  {
    vtkLog(ERROR, "Appended Write and Read version 1 failed");
    return EXIT_FAILURE;
  }

  vtkLog(INFO, "Writing TestXMLHyperTreeGridIO2_Appendedv2.htg");
  fname = tdir + std::string("/TestXMLHyperTreeGridIO2_Appendedv2.htg");
  writer->SetDataSetMajorVersion(2);
  writer->SetFileName(fname.c_str());
  writer->Write();

  vtkLog(INFO, "Reading TestXMLHyperTreeGridIO2_Appendedv2.htg");
  reader->SetFileName(fname.c_str());
  reader->Update();
  htgRead = vtkHyperTreeGrid::SafeDownCast(reader->GetOutputDataObject(0));
  if (!AreHTGSame(htgWrite, htgRead))
  {
    vtkLog(ERROR, "Appended Write and Read version 2 failed");
    return EXIT_FAILURE;
  }

  vtkLog(INFO, "Writing TestXMLHyperTreeGridIO2_Binaryv0.htg");
  fname = tdir + std::string("/TestXMLHyperTreeGridIO2_Binaryv0.htg");
  writer->SetDataSetMajorVersion(0);
  writer->SetFileName(fname.c_str());
  writer->SetDataModeToBinary();
  writer->Write();

  vtkLog(INFO, "Reading TestXMLHyperTreeGridIO2_Binaryv0.htg");
  reader->SetFileName(fname.c_str());
  reader->Update();
  htgRead = vtkHyperTreeGrid::SafeDownCast(reader->GetOutputDataObject(0));
  if (!AreHTGSame(htgWrite, htgRead))
  {
    vtkLog(ERROR, "Binary Write and Read version 0 failed");
    return EXIT_FAILURE;
  }

  vtkLog(INFO, "Writing TestXMLHyperTreeGridIO2_Binaryv1.htg");
  fname = tdir + std::string("/TestXMLHyperTreeGridIO2_Binaryv1.htg");
  writer->SetDataSetMajorVersion(1);
  writer->SetFileName(fname.c_str());
  writer->Write();

  vtkLog(INFO, "Reading TestXMLHyperTreeGridIO2_Binaryv1.htg");
  reader->SetFileName(fname.c_str());
  reader->Update();
  htgRead = vtkHyperTreeGrid::SafeDownCast(reader->GetOutputDataObject(0));
  if (!AreHTGSame(htgWrite, htgRead))
  {
    vtkLog(ERROR, "Binary Write and Read version 1 failed");
    return EXIT_FAILURE;
  }

  vtkLog(INFO, "Writing TestXMLHyperTreeGridIO2_Binaryv2.htg");
  fname = tdir + std::string("/TestXMLHyperTreeGridIO2_Binaryv2.htg");
  writer->SetDataSetMajorVersion(2);
  writer->SetFileName(fname.c_str());
  writer->Write();

  vtkLog(INFO, "Reading TestXMLHyperTreeGridIO2_Binaryv2.htg");
  reader->SetFileName(fname.c_str());
  reader->Update();
  htgRead = vtkHyperTreeGrid::SafeDownCast(reader->GetOutputDataObject(0));
  if (!AreHTGSame(htgWrite, htgRead))
  {
    vtkLog(ERROR, "Binary Write and Read version 2 failed");
    return EXIT_FAILURE;
  }

  // Testing with mask htg

  vtkNew<vtkHyperTreeGridAxisClip> clip;
  clip->SetInputConnection(source->GetOutputPort(0));
  double normal[3] = { 0.809, -0.42, 0.411 };
  clip->SetClipTypeToQuadric();
  clip->SetQuadricCoefficients(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, normal[0], normal[1], normal[2], 0.0);
  clip->SetInsideOut(true);

  clip->Update();
  writer->SetInputData(clip->GetOutputDataObject(0));
  htgWrite = vtkHyperTreeGrid::SafeDownCast(clip->GetOutputDataObject(0));

  vtkLog(INFO, "Writing TestXMLHyperTreeGridIO2_MaskedAppendedv0.htg");
  fname = tdir + std::string("/TestXMLHyperTreeGridIO2_MaskedAppendedv0.htg");
  writer->SetDataSetMajorVersion(0);
  writer->SetDataModeToAppended();
  writer->SetFileName(fname.c_str());
  writer->Write();

  vtkLog(INFO, "Reading TestXMLHyperTreeGridIO2_MaskedAppendedv0.htg");
  reader->SetFileName(fname.c_str());
  reader->Update();
  htgRead = vtkHyperTreeGrid::SafeDownCast(reader->GetOutputDataObject(0));

  if (!AreHTGSame(htgWrite, htgRead))
  {
    vtkLog(ERROR, "Masked Appended Write and Read version 0 failed");
    return EXIT_FAILURE;
  }

  vtkLog(INFO, "Writing TestXMLHyperTreeGridIO2_MaskedAppendedv1.htg");
  fname = tdir + std::string("/TestXMLHyperTreeGridIO2_MaskedAppendedv1.htg");
  writer->SetDataSetMajorVersion(1);
  writer->SetFileName(fname.c_str());
  writer->Write();

  vtkLog(INFO, "Reading TestXMLHyperTreeGridIO2_MaskedAppendedv1.htg");
  reader->SetFileName(fname.c_str());
  reader->Update();
  htgRead = vtkHyperTreeGrid::SafeDownCast(reader->GetOutputDataObject(0));

  if (!AreHTGSame(htgWrite, htgRead))
  {
    vtkLog(ERROR, "Masked Appended Write and Read version 1 failed");
    return EXIT_FAILURE;
  }

  vtkLog(INFO, "Writing TestXMLHyperTreeGridIO2_MaskedAppendedv2.htg");
  fname = tdir + std::string("/TestXMLHyperTreeGridIO2_MaskedAppendedv2.htg");
  writer->SetDataSetMajorVersion(2);
  writer->SetFileName(fname.c_str());
  writer->Write();

  vtkLog(INFO, "Reading TestXMLHyperTreeGridIO2_MaskedAppendedv2.htg");
  reader->SetFileName(fname.c_str());
  reader->Update();
  htgRead = vtkHyperTreeGrid::SafeDownCast(reader->GetOutputDataObject(0));

  if (!AreHTGSame(htgWrite, htgRead))
  {
    vtkLog(ERROR, "Masked Appended Write and Read version 2 failed");
    return EXIT_FAILURE;
  }

  vtkLog(INFO, "Writing TestXMLHyperTreeGridIO2_MaskedBinaryv0.htg");
  fname = tdir + std::string("/TestXMLHyperTreeGridIO2_MaskedBinaryv0.htg");
  writer->SetDataSetMajorVersion(0);
  writer->SetDataModeToBinary();
  writer->SetFileName(fname.c_str());
  writer->Write();

  vtkLog(INFO, "Reading TestXMLHyperTreeGridIO2_MaskedBinaryv0.htg");
  reader->SetFileName(fname.c_str());
  reader->Update();
  htgRead = vtkHyperTreeGrid::SafeDownCast(reader->GetOutputDataObject(0));

  if (!AreHTGSame(htgWrite, htgRead))
  {
    vtkLog(ERROR, "Masked Binary Write and Read version 0 failed");
    return EXIT_FAILURE;
  }

  vtkLog(INFO, "Writing TestXMLHyperTreeGridIO2_MaskedBinaryv1.htg");
  fname = tdir + std::string("/TestXMLHyperTreeGridIO2_MaskedBinaryv1.htg");
  writer->SetDataSetMajorVersion(1);
  writer->SetFileName(fname.c_str());
  writer->Write();

  vtkLog(INFO, "Reading TestXMLHyperTreeGridIO2_MaskedBinaryv1.htg");
  reader->SetFileName(fname.c_str());
  reader->Update();
  htgRead = vtkHyperTreeGrid::SafeDownCast(reader->GetOutputDataObject(0));

  if (!AreHTGSame(htgWrite, htgRead))
  {
    vtkLog(ERROR, "Masked Binary Write and Read version 1 failed");
    return EXIT_FAILURE;
  }

  vtkLog(INFO, "Writing TestXMLHyperTreeGridIO2_MaskedBinaryv2.htg");
  fname = tdir + std::string("/TestXMLHyperTreeGridIO2_MaskedBinaryv2.htg");
  writer->SetDataSetMajorVersion(2);
  writer->SetFileName(fname.c_str());
  writer->Write();

  vtkLog(INFO, "Reading TestXMLHyperTreeGridIO2_MaskedBinaryv2.htg");
  reader->SetFileName(fname.c_str());
  reader->Update();
  htgRead = vtkHyperTreeGrid::SafeDownCast(reader->GetOutputDataObject(0));

  if (!AreHTGSame(htgWrite, htgRead))
  {
    vtkLog(ERROR, "Masked Binary Write and Read version 2 failed");
    return EXIT_FAILURE;
  }

  // Testing depth limiter with mask
  unsigned int maxDepth = 3;
  reader->SetFixedLevel(maxDepth);

  vtkLog(INFO, "Reading TestXMLHyperTreeGridIO2_MaskedAppendedv1.htg with depth limiter");
  fname = tdir + std::string("/TestXMLHyperTreeGridIO2_MaskedAppendedv1.htg");
  reader->SetFileName(fname.c_str());
  reader->Update();
  htgRead = vtkHyperTreeGrid::SafeDownCast(reader->GetOutputDataObject(0));
  if (!AreHTGSame(htgWrite, htgRead, maxDepth))
  {
    vtkLog(ERROR, "Masked Appended Write and Read version 1 failed");
    return EXIT_FAILURE;
  }

  vtkLog(INFO, "Reading TestXMLHyperTreeGridIO2_MaskedAppendedv2.htg with depth limiter");
  fname = tdir + std::string("/TestXMLHyperTreeGridIO2_MaskedAppendedv2.htg");
  reader->SetFileName(fname.c_str());
  reader->Update();
  htgRead = vtkHyperTreeGrid::SafeDownCast(reader->GetOutputDataObject(0));
  if (!AreHTGSame(htgWrite, htgRead, maxDepth))
  {
    vtkLog(ERROR, "Masked Appended Write and Read version 2 failed");
    return EXIT_FAILURE;
  }

  vtkLog(INFO, "Reading TestXMLHyperTreeGridIO2_MaskedBinaryv1.htg with depth limiter");
  fname = tdir + std::string("/TestXMLHyperTreeGridIO2_MaskedBinaryv1.htg");
  reader->SetFileName(fname.c_str());
  reader->Update();
  htgRead = vtkHyperTreeGrid::SafeDownCast(reader->GetOutputDataObject(0));
  if (!AreHTGSame(htgWrite, htgRead, maxDepth))
  {
    vtkLog(ERROR, "Masked Binary Write and Read version 1 failed");
    return EXIT_FAILURE;
  }

  vtkLog(INFO, "Reading TestXMLHyperTreeGridIO2_MaskedBinaryv2.htg with depth limiter");
  fname = tdir + std::string("/TestXMLHyperTreeGridIO2_MaskedBinaryv2.htg");
  reader->SetFileName(fname.c_str());
  reader->Update();
  htgRead = vtkHyperTreeGrid::SafeDownCast(reader->GetOutputDataObject(0));
  if (!AreHTGSame(htgWrite, htgRead, maxDepth))
  {
    vtkLog(ERROR, "Masked Binary Write and Read version 2 failed");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
