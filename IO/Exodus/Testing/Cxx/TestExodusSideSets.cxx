#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkExodusIIReader.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkTestUtilities.h"

int TestExodusSideSets(int argc, char* argv[])
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

  rdr->GenerateGlobalNodeIdArrayOn();
  rdr->GenerateGlobalElementIdArrayOn();
  rdr->ExodusModelMetadataOn();
  rdr->UpdateInformation();

  for(int i=0;i<rdr->GetNumberOfObjects(vtkExodusIIReader::ELEM_BLOCK);i++)
    {
    rdr->SetObjectStatus(vtkExodusIIReader::ELEM_BLOCK, i, 0);
    }

  for(int i=0;i<rdr->GetNumberOfObjects(vtkExodusIIReader::SIDE_SET);i++)
    {
    rdr->SetObjectStatus(vtkExodusIIReader::SIDE_SET, i, 1);
    }

  rdr->Update();

  vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::SafeDownCast(rdr->GetOutput());
  vtkCellData* cd = vtkDataSet::SafeDownCast(
    vtkMultiBlockDataSet::SafeDownCast(mb->GetBlock(4))->GetBlock(0))->GetCellData();

  if(cd == NULL)
    {
    cerr << "Can't find proper data set\n";
    return 1;
    }

  vtkIdTypeArray* sourceelementids = vtkIdTypeArray::SafeDownCast(
    cd->GetArray(vtkExodusIIReader::GetSideSetSourceElementIdArrayName()));

  vtkIntArray* sourceelementsides = vtkIntArray::SafeDownCast(
    cd->GetArray(vtkExodusIIReader::GetSideSetSourceElementSideArrayName()));

  if(!sourceelementsides || !sourceelementids)
    {
    cerr << "Can't find proper cell data arrays\n";
    return 1;
    }
  else
    {
    if(sourceelementids->GetNumberOfTuples() != 5)
      {
      cerr << "Wrong number of cell array tuples\n";
      return 1;
      }
    // correct values
    vtkIdType ids[] = {0, 0, 0, 1, 1};
    int sides[] = {2, 3, 4, 1, 0};

    for(vtkIdType i=0;i<sourceelementids->GetNumberOfTuples();i++)
      {
      if(sourceelementids->GetValue(i) != ids[i])
        {
        cerr << "Source element id is wrong\n";
        return 1;
        }
      if(sourceelementsides->GetValue(i) != sides[i])
        {
        cerr << "Source element side is wrong\n";
        return 1;
        }
      }
    }

  return 0;
}
