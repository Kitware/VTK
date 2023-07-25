// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkEnSightWriter
 * @brief   write vtk unstructured grid data as an EnSight file
 *
 * vtkEnSightWriter is a source object that writes binary
 * unstructured grid data files in EnSight format. See EnSight Manual for
 * format details
 *
 * @warning
 * Binary files written on one system may not be readable on other systems.
 * Be sure to specify the endian-ness of the file when reading it into EnSight
 */

#ifndef vtkEnSightWriter_h
#define vtkEnSightWriter_h

#include "vtkIOParallelModule.h" // For export macro
#include "vtkWriter.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkUnstructuredGrid;

class VTKIOPARALLEL_EXPORT vtkEnSightWriter : public vtkWriter
{

public:
  vtkTypeMacro(vtkEnSightWriter, vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**

   */
  static vtkEnSightWriter* New();

  ///@{
  /**
   * Specify which process this writer is
   */
  vtkSetMacro(ProcessNumber, int);
  vtkGetMacro(ProcessNumber, int);
  ///@}

  ///@{
  /**
   * Specify path of EnSight data files to write.
   */
  vtkSetFilePathMacro(Path);
  vtkGetFilePathMacro(Path);
  ///@}

  ///@{
  /**
   * Specify base name of EnSight data files to write.
   */
  vtkSetStringMacro(BaseName);
  vtkGetStringMacro(BaseName);
  ///@}

  ///@{
  /**
   * Specify the path and base name of the output files.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  ///@{
  /**
   * Specify the Timestep that this data is for
   */
  vtkSetMacro(TimeStep, int);
  vtkGetMacro(TimeStep, int);
  ///@}

  ///@{
  /**
   * Specify the number of ghost levels to include in output files
   */
  vtkSetMacro(GhostLevel, int);
  vtkGetMacro(GhostLevel, int);
  ///@}

  ///@{
  /**
   * Specify whether the geometry changes each timestep
   * if false, geometry is only written at timestep 0
   */
  vtkSetMacro(TransientGeometry, bool);
  vtkGetMacro(TransientGeometry, bool);
  ///@}

  ///@{
  /**
   * set the number of block ID's
   */
  vtkSetMacro(NumberOfBlocks, int);
  vtkGetMacro(NumberOfBlocks, int);
  ///@}

  ///@{
  /**
   * Turn on/off writing node IDs (default: on).
   * If this is on, geometry files will contain node IDs for each part
   * (<code>node id given</code>), otherwise node IDs are omitted
   * (<code>node id off</code>).
   *
   * The EnSight node IDs correspond to VTK point IDs in the input dataset.
   */
  vtkBooleanMacro(WriteNodeIDs, bool);
  vtkSetMacro(WriteNodeIDs, bool);
  vtkGetMacro(WriteNodeIDs, bool);
  ///@}

  ///@{
  /**
   * Turn on/off writing element IDs (default: on).
   * If this is on, geometry files will contain element IDs for each part
   * (<code>element id given</code>), otherwise element IDs are omitted
   * (<code>element id off</code>).
   *
   * The EnSight element IDs correspond to VTK cell IDs in the input dataset.
   */
  vtkBooleanMacro(WriteElementIDs, bool);
  vtkSetMacro(WriteElementIDs, bool);
  vtkGetMacro(WriteElementIDs, bool);
  ///@}

  ///@{
  /**
   * set the array of Block ID's
   * this class keeps a reference to the array and will not delete it
   */
  virtual void SetBlockIDs(int* val) { BlockIDs = val; }
  virtual int* GetBlockIDs() { return BlockIDs; }
  ///@}

  ///@{
  /**
   * Specify the input data or filter.
   */
  virtual void SetInputData(vtkUnstructuredGrid* input);
  virtual vtkUnstructuredGrid* GetInput();
  ///@}

  ///@{
  /**
   * Writes the case file that EnSight is capable of reading
   * The other data files must be written before the case file
   * and the input must be one of the time steps
   * variables must be the same for all time steps or the case file will be
   * missing variables
   */
  virtual void WriteCaseFile(int TotalTimeSteps);
  virtual void WriteSOSCaseFile(int NumProcs);
  ///@}

protected:
  vtkEnSightWriter();
  ~vtkEnSightWriter() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  void WriteData() override; // method to allow this class to be instantiated and delegated to

  virtual void WriteStringToFile(const char* string, FILE* file);
  virtual void WriteTerminatedStringToFile(const char* string, FILE* file);
  virtual void WriteIntToFile(int i, FILE* file);
  virtual void WriteFloatToFile(float f, FILE* file);
  virtual void WriteElementTypeToFile(int ElementType, FILE* fd);

  virtual bool ShouldWriteGeometry();
  virtual void SanitizeFileName(char* name);
  virtual FILE* OpenFile(char* name);

  void ComputeNames();
  void DefaultNames();

  int GetExodusModelIndex(int* ElementArray, int NumberElements, int PartID);

  static int GetDestinationComponent(int srcComponent, int numComponents);

  char* Path;
  char* BaseName;
  char* FileName;
  int TimeStep;
  int GhostLevelMultiplier;
  int ProcessNumber;
  int NumberOfProcesses;
  int NumberOfBlocks;
  int* BlockIDs;
  bool TransientGeometry;
  int GhostLevel;
  bool WriteNodeIDs;
  bool WriteElementIDs;
  vtkUnstructuredGrid* TmpInput;

  vtkEnSightWriter(const vtkEnSightWriter&) = delete;
  void operator=(const vtkEnSightWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
