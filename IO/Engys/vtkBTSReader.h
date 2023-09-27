// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2019-2023 Engys Ltd.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkBTSReader
 * @brief   class to read bts files used by Engys software
 *
 * The class vtkBTSReader allows the user to read bts surface files,
 * which are more compressed than stl files.
 * The output of the reader is a `vtkPartitionedDataSet` where each
 * partition is a `vtkPolyData` representing one solid, with the name
 * of the solid being available as meta data.
 */

#ifndef vtkBTSReader_h
#define vtkBTSReader_h

#include "vtkIOEngysModule.h" // For export macro
#include "vtkPartitionedDataSetAlgorithm.h"
#include "vtkSmartPointer.h" // For vtkSmartPointer

VTK_ABI_NAMESPACE_BEGIN
class vtkPolyData;
class SolidNames;
class vtkResourceStream;

class VTKIOENGYS_EXPORT vtkBTSReader : public vtkPartitionedDataSetAlgorithm
{
public:
  static vtkBTSReader* New();
  vtkTypeMacro(vtkBTSReader, vtkPartitionedDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the filename (with path) for the bts file.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  ///@{
  /**
   * Set/Get the stream from which to read the .bts file.
   * If Stream is not nullptr, it will be used in priority from FileName.
   * The passed Stream must have already been opened.
   */
  virtual void SetStream(vtkResourceStream* stream);
  virtual vtkResourceStream* GetStream();
  ///@}

  ///@{
  /**
   * Get the registration name for display purposes,
   * which is the file name without path or extension
   */
  const char* GetRegistrationName();
  ///@}

protected:
  vtkBTSReader();
  ~vtkBTSReader() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  ///@{
  /**
   * Reads the bts file.  If an error occurred, false is returned; otherwise true.
   */
  bool Read(vtkPartitionedDataSet* output);
  ///@}

  ///@{
  /**
   * Reads the corresponding part of the bts file.
   * If an error occurred, false is returned; otherwise true.
   */
  bool ReadHeader();
  bool ReadSolidNames(int numberOfSolids, SolidNames& solidNames);
  bool ReadSolid(vtkPolyData* polyData);
  bool ReadPoints(vtkPolyData* polyData);
  bool ReadFaces(vtkPolyData* polyData);
  ///@}

  ///@{
  /**
   * Finds the size of the stream, in bytes
   */
  long Filesize();
  ///@}

  ///@{
  /**
   * Controls the progress of the algorithm, based on the number of bytes read from the file
   */
  void InitReadProgress(unsigned long fileSize);
  void UpdateReadProgress(size_t bytes);
  ///@}

  ///@{
  /**
   * Reads data from the stream, wrapped to also update the progress
   */
  uint32_t ReadUint32Value();
  bool ReadArray(void* ptr, size_t variableSize, size_t nVariables);
  ///@}

  char* FileName = nullptr;
  vtkSmartPointer<vtkResourceStream> Stream;

  size_t ReadBytes = 0;
  size_t FileSize = 1;
  int PreviousPercentProgress = 0;
  std::string RegistrationName;

  vtkBTSReader(const vtkBTSReader&) = delete;   // Not implemented.
  void operator=(const vtkBTSReader&) = delete; // Not implemented.
};

VTK_ABI_NAMESPACE_END
#endif
