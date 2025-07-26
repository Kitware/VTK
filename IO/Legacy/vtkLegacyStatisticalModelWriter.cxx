// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkLegacyStatisticalModelWriter.h"

#include "vtkBase64Utilities.h"
#include "vtkErrorCode.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkStatisticalModel.h"
#include "vtkTable.h"
#include "vtkTableWriter.h"

#include <algorithm>
#include <iterator>
#include <vector>

#if !defined(_WIN32) || defined(__CYGWIN__)
#include <unistd.h> /* unlink */
#else
#include <io.h> /* unlink */
#endif

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkLegacyStatisticalModelWriter);

void vtkLegacyStatisticalModelWriter::WriteData()
{
  ostream* fp;
  vtkStatisticalModel* input = vtkStatisticalModel::SafeDownCast(this->GetInput());

  vtkDebugMacro(<< "Writing vtk statistical model data...");

  if (!(fp = this->OpenVTKFile()) || !this->WriteHeader(fp))
  {
    if (fp)
    {
      vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
      this->CloseVTKFile(fp);
      unlink(this->FileName);
    }
    return;
  }

  // Figure out how many table groups we will write:
  int numberOfTableTypes = 0;
  for (int ttype = vtkStatisticalModel::Learned; ttype <= vtkStatisticalModel::Derived; ++ttype)
  {
    if (input->GetNumberOfTables(ttype) > 0)
    {
      ++numberOfTableTypes;
    }
  }

  std::string encodedData;
  vtkNew<vtkTableWriter> tableWriter;
  tableWriter->WriteToOutputStringOn();
  // TODO: Turn this on so table values are exact:
  //       It currently causes issues for the reader.
  // tableWriter->SetFileTypeToBinary();

  // Write stuff specific to statistical models.
  //
  // We use "DATASET" here to prevent vtkDataObjectReader from attempting
  // to read cell-grids, even though vtkStatisticalModel does not inherit vtkDataSet.
  *fp << "DATASET STATISTICAL_MODEL " << numberOfTableTypes << "\n";

  // Write out the algorithm parameters (encoded):
  const char* param = input->GetAlgorithmParameters();
  if (!param || !param[0])
  {
    *fp << "ALGORITHM_PARAMETERS 0 0\n";
  }
  else
  {
    unsigned long paramLen = static_cast<unsigned long>(strlen(param));
    encodedData.resize(paramLen + paramLen / 2);
    auto encodedLen = vtkBase64Utilities::Encode(reinterpret_cast<const unsigned char*>(param),
      paramLen, reinterpret_cast<unsigned char*>(encodedData.data()), /*mark_end*/ 0);
    encodedData.resize(encodedLen);
    *fp << "ALGORITHM_PARAMETERS " << encodedLen << " " << paramLen << "\n";
    *fp << encodedData << "\n";
  }

  // Now write out the non-empty table groups:
  for (int tableType = vtkStatisticalModel::Learned; tableType <= vtkStatisticalModel::Derived;
       ++tableType)
  {
    int numTables = input->GetNumberOfTables(tableType);
    if (numTables == 0)
    {
      continue;
    }
    *fp << "MODEL_TABLES " << vtkStatisticalModel::GetTableTypeName(tableType) << " " << numTables
        << "\n";
    for (int tt = 0; tt < numTables; ++tt)
    {
      std::string tableName = input->GetTableName(tableType, tt);
      vtkIdType bareNameLen = static_cast<vtkIdType>(tableName.size());
      encodedData.resize(bareNameLen + bareNameLen / 2);
      auto encodedNameLen =
        vtkBase64Utilities::Encode(reinterpret_cast<const unsigned char*>(tableName.c_str()),
          static_cast<unsigned long>(bareNameLen),
          reinterpret_cast<unsigned char*>(encodedData.data()), /*mark_end*/ 0);
      encodedData.resize(encodedNameLen);
      *fp << "NAME " << encodedNameLen << " " << bareNameLen << "\n";
      *fp << encodedData << "\n";

      auto* table = input->GetTable(tableType, tt);
      if (table)
      {
        tableWriter->SetInputDataObject(0, table);
        tableWriter->Update();
        const auto* tableData = tableWriter->GetBinaryOutputString();
        vtkIdType bareTableSize = tableWriter->GetOutputStringLength();
        encodedData.resize(bareTableSize + bareTableSize / 2);
        auto encodedLen =
          vtkBase64Utilities::Encode(tableData, static_cast<unsigned long>(bareTableSize),
            reinterpret_cast<unsigned char*>(encodedData.data()), /*mark_end*/ 1);
        encodedData.resize(encodedLen);
        *fp << "MODEL_TABLE " << encodedLen << " " << bareTableSize << "\n";
        *fp << encodedData << "\n";
      }
      else
      {
        // Write a record for a null (but reserved) table.
        *fp << "MODEL_TABLE 0 0\n";
      }
    }
  }
  // *fp << "\n";
  this->CloseVTKFile(fp);
}

int vtkLegacyStatisticalModelWriter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkStatisticalModel");
  return 1;
}

vtkStatisticalModel* vtkLegacyStatisticalModelWriter::GetInput()
{
  return vtkStatisticalModel::SafeDownCast(this->Superclass::GetInput());
}

vtkStatisticalModel* vtkLegacyStatisticalModelWriter::GetInput(int port)
{
  return vtkStatisticalModel::SafeDownCast(this->Superclass::GetInput(port));
}

void vtkLegacyStatisticalModelWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
