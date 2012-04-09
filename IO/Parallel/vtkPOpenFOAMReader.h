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
// .NAME vtkPOpenFOAMReader - reads a decomposed dataset in OpenFOAM format
// .SECTION Description
// vtkPOpenFOAMReader creates a multiblock dataset. It reads
// parallel-decomposed mesh information and time dependent data.  The
// polyMesh folders contain mesh information. The time folders contain
// transient data for the cells. Each folder can contain any number of
// data files.

// .SECTION Thanks
// This class was developed by Takuya Oshima at Niigata University,
// Japan (oshima@eng.niigata-u.ac.jp).

#ifndef __vtkPOpenFOAMReader_h
#define __vtkPOpenFOAMReader_h

#include "vtkOpenFOAMReader.h"

class vtkDataArraySelection;
class vtkMultiProcessController;

class VTK_PARALLEL_EXPORT vtkPOpenFOAMReader : public vtkOpenFOAMReader
{
public:
  //BTX
  enum caseType { DECOMPOSED_CASE = 0, RECONSTRUCTED_CASE = 1 };
  //ETX
  static vtkPOpenFOAMReader *New();
  vtkTypeMacro(vtkPOpenFOAMReader, vtkOpenFOAMReader);

  void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Set and get case type. 0 = decomposed case, 1 = reconstructed case.
  void SetCaseType(const int t);
  vtkGetMacro(CaseType, caseType);
  // Description:
  // Set and get the controller.
  virtual void SetController(vtkMultiProcessController *);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

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
  unsigned long MTimeOld;
  int MaximumNumberOfPieces;
  int NumProcesses;
  int ProcessId;

  vtkPOpenFOAMReader(const vtkPOpenFOAMReader &); // Not implemented.
  void operator=(const vtkPOpenFOAMReader &); // Not implemented.

  void GatherMetaData();
  void BroadcastStatus(int &);
  void Broadcast(vtkStringArray *);
  void AllGather(vtkStringArray *);
  void AllGather(vtkDataArraySelection *);
};

#endif
