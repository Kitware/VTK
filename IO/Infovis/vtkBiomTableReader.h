// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkBiomTableReader
 * @brief   read vtkTable from a .biom input file
 *
 * vtkBiomTableReader is a source object that reads ASCII biom data files.
 * The output of this reader is a single vtkTable data object.
 * @sa
 * vtkTable vtkTableReader vtkDataReader
 */

#ifndef vtkBiomTableReader_h
#define vtkBiomTableReader_h

#include "vtkIOInfovisModule.h" // For export macro
#include "vtkTableReader.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkTable;
class vtkVariant;

class VTKIOINFOVIS_EXPORT vtkBiomTableReader : public vtkTableReader
{
public:
  static vtkBiomTableReader* New();
  vtkTypeMacro(vtkBiomTableReader, vtkTableReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the output of this reader.
   */
  vtkTable* GetOutput();
  vtkTable* GetOutput(int idx);
  void SetOutput(vtkTable* output);
  ///@}

  /**
   * Actual reading happens here
   */
  int ReadMeshSimple(VTK_FILEPATH const std::string& fname, vtkDataObject* output) override;

protected:
  vtkBiomTableReader();
  ~vtkBiomTableReader() override;

  int FillOutputPortInformation(int, vtkInformation*) override;
  void ParseShape();
  void ParseDataType();
  void ParseSparseness();
  void InitializeData();
  void FillData(vtkVariant v);
  void ParseSparseData();
  void ParseDenseData();
  void InsertValue(int row, int col, const std::string& value);
  void ParseId();
  void ParseColumns();
  void ParseRows();

private:
  std::string FileContents;
  int NumberOfRows;
  int NumberOfColumns;
  int DataType;
  bool Sparse;
  vtkBiomTableReader(const vtkBiomTableReader&) = delete;
  void operator=(const vtkBiomTableReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
