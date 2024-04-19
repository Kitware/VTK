// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkOMFReader_h
#define vtkOMFReader_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkIOOMFModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArraySelection;
class vtkStringArray;

/**
 * @class   vtkOMFReader
 * @brief   Read Open Mining Format files
 *
 * vtkOMFReader reads OMF files. Details about the OMF format can be
 * found at https://omf.readthedocs.io/en/stable/index.html.
 * The reader outputs a vtkPartitionedDataSetCollection, where each
 * vtkPartitionedDataSet is one OMF element (point set, line set,
 * surface, or volume).
 */
class VTKIOOMF_EXPORT vtkOMFReader : public vtkDataObjectAlgorithm
{
public:
  vtkTypeMacro(vtkOMFReader, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct a new reader instance.
   */
  static vtkOMFReader* New();

  ///@{
  /**
   * Accessor for name of the OMF file to read
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  ///@{
  /**
   * Data element selection to determine which data elements in the OMF
   * file should be loaded. By default all elements' status are set to false/disabled,
   * so they will not be loaded.
   */
  bool GetDataElementArrayStatus(const char* name);
  void SetDataElementArrayStatus(const char* name, int status);
  int GetNumberOfDataElementArrays();
  const char* GetDataElementArrayName(int index);
  vtkDataArraySelection* GetDataElementArraySelection();
  ///@}

  /**
   * Overridden to take into account mtimes for vtkDataArraySelection instances.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Get/Set for determining to write out textures from OMF file
   */
  vtkSetMacro(WriteOutTextures, bool);
  vtkGetMacro(WriteOutTextures, bool);
  ///@}

  ///@{
  /**
   * Get/Set if scalar data is in column major order.
   * It should be in row major order but some software seems to
   * write out column major for volumes.
   */
  vtkSetMacro(ColumnMajorOrdering, bool);
  vtkGetMacro(ColumnMajorOrdering, bool);
  ///@}

protected:
  vtkOMFReader();
  ~vtkOMFReader() override;

  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int FillOutputPortInformation(int port, vtkInformation* info) override;

  char* FileName;
  bool WriteOutTextures = true;
  bool ColumnMajorOrdering = false;

private:
  vtkOMFReader(const vtkOMFReader&) = delete;
  void operator=(const vtkOMFReader&) = delete;

  struct ReaderImpl;
  ReaderImpl* Impl;
};

VTK_ABI_NAMESPACE_END
#endif
