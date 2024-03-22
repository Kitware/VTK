// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCompositeCellGridReader
 * @brief   Read a cell-grid file.
 *
 * vtkCompositeCellGridReader is a concrete subclass of
 * vtkReaderAlgorithm that reads data into multiple vtkCellGrid instances.
 *
 * @sa
 * vtkReaderAlgorithm
 */

#ifndef vtkCompositeCellGridReader_h
#define vtkCompositeCellGridReader_h

#include "vtkIOCellGridModule.h" // For export macro
#include "vtkReaderAlgorithm.h"
#include "vtkSmartPointer.h" // For SmartPointer

#include <string> // For std::string
#include <vector> // For std::vector

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArraySelection;
class vtkFieldData;
class vtkCellGridDocumentLoader;
class vtkImageData;
class vtkStringArray;

class VTKIOCELLGRID_EXPORT vtkCompositeCellGridReader : public vtkReaderAlgorithm
{
public:
  static vtkCompositeCellGridReader* New();
  vtkTypeMacro(vtkCompositeCellGridReader, vtkReaderAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the name of the file from which to read data.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  vtkDataArraySelection* GetCellTypeSelection();
  vtkDataArraySelection* GetCellAttributeSelection();

  /// Override GetMTime so we can indicate we are modified when
  /// the cell-type or cell-attribute array-selections are modified.
  vtkMTimeType GetMTime() override;

protected:
  vtkCompositeCellGridReader();
  ~vtkCompositeCellGridReader() override;

  int FillOutputPortInformation(int port, vtkInformation* info) override;

  int ReadMetaData(vtkInformation* metadata) override;
  int ReadMesh(int, int, int, int, vtkDataObject*) override;
  int ReadPoints(int, int, int, int, vtkDataObject*) override;
  int ReadArrays(int, int, int, int, vtkDataObject*) override;

  bool UpdateMetadata();

  struct FileGroup
  {
    std::vector<std::string> Files;
  };

  char* FileName = nullptr;
  FileGroup Groups; // For now, the reader only supports a single group of files.
  vtkNew<vtkDataArraySelection> CellTypeSelection;
  vtkNew<vtkDataArraySelection> CellAttributeSelection;
  struct MetadataGuard;      // Used to double-buffer Groups, CellTypeSelection, and
                             // CellAttributeSelection.
  vtkTimeStamp MetadataTime; // The last time metadata was read from disk

private:
  vtkCompositeCellGridReader(const vtkCompositeCellGridReader&) = delete;
  void operator=(const vtkCompositeCellGridReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
