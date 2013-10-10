/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCPExodusIIInSituReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkCPExodusIIInSituReader - Read an Exodus II file into data structures
// that map the raw arrays returned by the Exodus II library into a multi-block
// data set containing vtkUnstructuredGridBase subclasses.
//
// .SECTION Description
// This class can be used to import Exodus II files into VTK without repacking
// the data into the standard VTK memory layout, avoiding the cost of a deep
// copy.

#ifndef __vtkCPExodusIIInSituReader_h
#define __vtkCPExodusIIInSituReader_h

#include "vtkIOExodusModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkNew.h" // For vtkNew
#include <string> // For std::string
#include <vector> // For std::vector

class vtkDataArrayCollection;
class vtkPointData;
class vtkPoints;

class VTKIOEXODUS_EXPORT vtkCPExodusIIInSituReader :
    public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkCPExodusIIInSituReader *New();
  vtkTypeMacro(vtkCPExodusIIInSituReader, vtkMultiBlockDataSetAlgorithm)
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Get/Set the name of the Exodus file to read.
  vtkSetStringMacro(FileName)
  vtkGetStringMacro(FileName)

  // Description:
  // Get/Set the current timestep to read as a zero-based index.
  vtkGetMacro(CurrentTimeStep, int)
  vtkSetMacro(CurrentTimeStep, int)

  // Description:
  // Get the range of timesteps, represented as [0, numTimeSteps - 1]. Call
  // UpdateInformation first to set this without reading any timestep data.
  vtkGetVector2Macro(TimeStepRange, int)

  // Description:
  // Get the floating point tag associated with the timestep at 'step'.
  double GetTimeStepValue(int step)
  {
    return TimeSteps.at(step);
  }

protected:
  vtkCPExodusIIInSituReader();
  ~vtkCPExodusIIInSituReader();

  int ProcessRequest(vtkInformation *request,
                     vtkInformationVector **inputVector,
                     vtkInformationVector *outputVector);
  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *);
  int RequestInformation(vtkInformation *, vtkInformationVector **,
                         vtkInformationVector *);

private:
  vtkCPExodusIIInSituReader(const vtkCPExodusIIInSituReader &); // Not implemented.
  void operator=(const vtkCPExodusIIInSituReader &);   // Not implemented.

  bool ExOpen();
  char *FileName;
  int FileId;

  bool ExGetMetaData();
  int NumberOfDimensions;
  int NumberOfNodes;
  int NumberOfElementBlocks;
  std::vector<std::string> NodalVariableNames;
  std::vector<std::string> ElementVariableNames;
  std::vector<int> ElementBlockIds;
  std::vector<double> TimeSteps;
  int TimeStepRange[2];

  bool ExGetCoords();
  vtkNew<vtkPoints> Points;

  bool ExGetNodalVars();
  vtkNew<vtkPointData> PointData;

  bool ExGetElemBlocks();
  vtkNew<vtkMultiBlockDataSet> ElementBlocks;

  void ExClose();

  int CurrentTimeStep;
};

#endif //__vtkCPExodusIIInSituReader_h
