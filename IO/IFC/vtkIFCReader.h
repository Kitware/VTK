// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkIFCReader
 * @brief   Reads an IFC file.
 *
 *
 * @sa
 * vtkPartitionedDataSetAlgorithm
 */

#ifndef vtkIFCReader_h
#define vtkIFCReader_h

#include "vtkIOIFCModule.h" // For export macro
#include "vtkPartitionedDataSetCollectionAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkPartitionedDataSet;

/**
 * @class   vtkIFCReader
 * @brief   Reads an IFC file
 *
 */
class VTKIOIFC_EXPORT vtkIFCReader : public vtkPartitionedDataSetCollectionAlgorithm
{
public:
  static vtkIFCReader* New();
  vtkTypeMacro(vtkIFCReader, vtkPartitionedDataSetCollectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the name of the file
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  ///@{
  /**
   * Set/Get the number of threads used to process the IFC file
   * Default is 8.
   */
  vtkSetMacro(NumberOfThreads, int);
  vtkGetMacro(NumberOfThreads, int);
  ///@}

  /**
   * Returns true if it can read the IFC file, false otherwise
   */
  virtual int CanReadFile(VTK_FILEPATH const char* name);

protected:
  vtkIFCReader();
  ~vtkIFCReader() override;

  char* FileName = nullptr;
  int NumberOfThreads;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkIFCReader(const vtkIFCReader&) = delete;
  void operator=(const vtkIFCReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
