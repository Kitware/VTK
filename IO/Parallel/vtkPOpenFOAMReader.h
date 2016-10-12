/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPOpenFOAMReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

class vtkDataArraySelection;
class vtkMultiProcessController;

class VTKIOPARALLEL_EXPORT vtkPOpenFOAMReader : public vtkOpenFOAMReader
{
public:

  enum caseType { DECOMPOSED_CASE = 0, RECONSTRUCTED_CASE = 1 };

  static vtkPOpenFOAMReader *New();
  vtkTypeMacro(vtkPOpenFOAMReader, vtkOpenFOAMReader);

  void PrintSelf(ostream &os, vtkIndent indent);

  //@{
  /**
   * Set and get case type. 0 = decomposed case, 1 = reconstructed case.
   */
  void SetCaseType(const int t);
  vtkGetMacro(CaseType, caseType);
  //@}
  //@{
  /**
   * Set and get the controller.
   */
  virtual void SetController(vtkMultiProcessController *);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

protected:
  vtkPOpenFOAMReader();
  ~vtkPOpenFOAMReader();

  int RequestInformation(vtkInformation *, vtkInformationVector **,
    vtkInformationVector *);
  int RequestData(vtkInformation *, vtkInformationVector **,
    vtkInformationVector *);

private:
  vtkMultiProcessController *Controller;
  caseType CaseType;
  vtkMTimeType MTimeOld;
  int NumProcesses;
  int ProcessId;

  vtkPOpenFOAMReader(const vtkPOpenFOAMReader &) VTK_DELETE_FUNCTION;
  void operator=(const vtkPOpenFOAMReader &) VTK_DELETE_FUNCTION;

  void GatherMetaData();
  void BroadcastStatus(int &);
  void Broadcast(vtkStringArray *);
  void AllGather(vtkStringArray *);
  void AllGather(vtkDataArraySelection *);
};

#endif
