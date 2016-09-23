#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkExodusIIReader.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkTestUtilities.h"

int TestExodusAttributes(int argc, char* argv[])
{
  char* fname = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/edgeFaceElem.exii");
  if (!fname)
  {
    cout << "Could not obtain filename for test data.\n";
    return 1;
  }

  vtkNew<vtkExodusIIReader> rdr;
  if (!rdr->CanReadFile(fname))
  {
    cout << "Cannot read \"" << fname << "\"\n";
    return 1;
  }
  rdr->SetFileName(fname);
  delete[] fname;

  rdr->UpdateInformation();
  rdr->SetObjectAttributeStatus(vtkExodusIIReader::ELEM_BLOCK,0,"SPAGHETTI",1);
  rdr->SetObjectAttributeStatus(vtkExodusIIReader::ELEM_BLOCK,0,"WESTERN",1);
  rdr->Update();
  vtkCellData* cd =
    vtkDataSet::SafeDownCast(
      vtkMultiBlockDataSet::SafeDownCast(
        vtkMultiBlockDataSet::SafeDownCast(
          rdr->GetOutputDataObject(0))
        ->GetBlock(0))
      ->GetBlock(0))
    ->GetCellData();
  if (!cd)
  {
    cout << "Could not obtain cell data\n";
    return 1;
  }
  int na = cd->GetNumberOfArrays();
  for (int i = 0; i < na; ++i)
  {
    vtkDataArray* arr = cd->GetArray(i);
    cout << "Cell array " << i << " \"" << arr->GetName() << "\"\n";
    for (int j = 0; j <= arr->GetMaxId(); ++j)
    {
      cout << " " << arr->GetTuple1(j) << "\n";
    }
  }
  vtkDataArray* spaghetti = cd->GetArray("SPAGHETTI");
  vtkDataArray* western = cd->GetArray("WESTERN");
  if (
    !spaghetti || !western ||
    spaghetti->GetNumberOfTuples() != 2 || western->GetNumberOfTuples() != 2)
  {
    cout << "Attribute arrays not read or are wrong length.\n";
    return 1;
  }
  if (spaghetti->GetTuple1(0) != 127. || spaghetti->GetTuple1(1) != 137)
  {
    cout << "Bad spaghetti\n";
    return 1;
  }
  if (western->GetTuple1(0) != 101. || western->GetTuple1(1) != 139)
  {
    cout << "Wrong western\n";
    return 1;
  }
  return 0;
}
