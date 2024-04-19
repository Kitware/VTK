// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPOpenFOAMReader
 * @brief   reads a decomposed dataset in OpenFOAM format
 *
 * vtkPOpenFOAMReader creates a multiblock dataset. It reads
 * parallel-decomposed mesh information and time dependent data.  The
 * polyMesh folders contain mesh information. The time folders contain
 * transient data for the cells. Each folder can contain any number of
 * data files.
 *
 * @par Thanks:
 * This class was developed by Takuya Oshima at Niigata University,
 * Japan (oshima@eng.niigata-u.ac.jp).
 */

#ifndef vtkPOpenFOAMReader_h
#define vtkPOpenFOAMReader_h

#include "vtkIOParallelModule.h" // For export macro
#include "vtkOpenFOAMReader.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArraySelection;
class vtkMultiProcessController;

class VTKIOPARALLEL_EXPORT vtkPOpenFOAMReader : public vtkOpenFOAMReader
{
public:
  enum caseType
  {
    DECOMPOSED_CASE = 0,
    RECONSTRUCTED_CASE = 1
  };

  static vtkPOpenFOAMReader* New();
  vtkTypeMacro(vtkPOpenFOAMReader, vtkOpenFOAMReader);

  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set and get case type. 0 = decomposed case, 1 = reconstructed case.
   */
  void SetCaseType(int t);
  vtkGetMacro(CaseType, caseType);
  ///@}
  ///@{
  /**
   * Set and get the controller.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

protected:
  vtkPOpenFOAMReader();
  ~vtkPOpenFOAMReader() override;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkMultiProcessController* Controller;
  caseType CaseType;
  vtkMTimeType MTimeOld;
  int NumProcesses;
  int ProcessId;

  vtkPOpenFOAMReader(const vtkPOpenFOAMReader&) = delete;
  void operator=(const vtkPOpenFOAMReader&) = delete;

  void GatherMetaData();
  void Broadcast(vtkStringArray*);
  void AllGather(vtkStringArray*);
  void AllGather(vtkDataArraySelection*);
};

VTK_ABI_NAMESPACE_END
#endif
