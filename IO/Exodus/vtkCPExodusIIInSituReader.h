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

/**
 * @class   vtkCPExodusIIInSituReader
 * @brief   Read an Exodus II file into data structures
 * that map the raw arrays returned by the Exodus II library into a multi-block
 * data set containing vtkUnstructuredGridBase subclasses.
 *
 *
 * This class can be used to import Exodus II files into VTK without repacking
 * the data into the standard VTK memory layout, avoiding the cost of a deep
 * copy.
*/

#ifndef vtkCPExodusIIInSituReader_h
#define vtkCPExodusIIInSituReader_h

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
  void PrintSelf(ostream &os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the name of the Exodus file to read.
   */
  vtkSetStringMacro(FileName)
  vtkGetStringMacro(FileName)
  //@}

  //@{
  /**
   * Get/Set the current timestep to read as a zero-based index.
   */
  vtkGetMacro(CurrentTimeStep, int)
  vtkSetMacro(CurrentTimeStep, int)
  //@}

  //@{
  /**
   * Get the range of timesteps, represented as [0, numTimeSteps - 1]. Call
   * UpdateInformation first to set this without reading any timestep data.
   */
  vtkGetVector2Macro(TimeStepRange, int)
  //@}

  /**
   * Get the floating point tag associated with the timestep at 'step'.
   */
  double GetTimeStepValue(int step)
  {
    return TimeSteps.at(step);
  }

protected:
  vtkCPExodusIIInSituReader();
  ~vtkCPExodusIIInSituReader() override;

  int ProcessRequest(vtkInformation *request,
                     vtkInformationVector **inputVector,
                     vtkInformationVector *outputVector) override;
  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *) override;
  int RequestInformation(vtkInformation *, vtkInformationVector **,
                         vtkInformationVector *) override;

private:
  vtkCPExodusIIInSituReader(const vtkCPExodusIIInSituReader &) = delete;
  void operator=(const vtkCPExodusIIInSituReader &) = delete;

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

#endif //vtkCPExodusIIInSituReader_h
