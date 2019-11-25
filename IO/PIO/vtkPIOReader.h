/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPIOReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPIOReader
 * @brief   class for reading PIO (Parallel Input Output) data files
 *
 * This class reads in dump files generated from xRage, a LANL physics code.
 * The PIO (Parallel Input Output) library is used to create the dump files.
 *
 * @sa
 * vtkMultiBlockReader
 */

#ifndef vtkPIOReader_h
#define vtkPIOReader_h

#include "vtkIOPIOModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

class vtkCallbackCommand;
class vtkDataArraySelection;
class vtkFloatArray;
class vtkInformation;
class vtkMultiBlockDataSet;
class vtkMultiProcessController;
class vtkStdString;

class PIOAdaptor;
class PIO_DATA;

class VTKIOPIO_EXPORT vtkPIOReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkPIOReader* New();
  vtkTypeMacro(vtkPIOReader, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Specify file name of PIO data file to read.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Specify the timestep to be loaded
   */
  vtkSetMacro(CurrentTimeStep, int);
  vtkGetMacro(CurrentTimeStep, int);
  //@}

  //@{
  /**
   * Get the reader's output
   */
  vtkMultiBlockDataSet* GetOutput();
  vtkMultiBlockDataSet* GetOutput(int index);
  //@}

  //@{
  /**
   * The following methods allow selective reading of solutions fields.
   * By default, ALL data fields on the nodes are read, but this can
   * be modified.
   */
  int GetNumberOfCellArrays();
  const char* GetCellArrayName(int index);
  int GetCellArrayStatus(const char* name);
  void SetCellArrayStatus(const char* name, int status);
  void DisableAllCellArrays();
  void EnableAllCellArrays();
  //@}

protected:
  vtkPIOReader();
  ~vtkPIOReader() override;

  char* FileName; // First field part file giving path

  int Rank;      // Number of this processor
  int TotalRank; // Number of processors

  PIOAdaptor* pioAdaptor; // Adapts data format to VTK

  int NumberOfVariables; // Number of variables to display

  int NumberOfTimeSteps; // Temporal domain
  double* TimeSteps;     // Times available for request
  int CurrentTimeStep;   // Time currently displayed
  int LastTimeStep;      // Last time displayed

  // Controls initializing and querrying MPI
  vtkMultiProcessController* MPIController;

  // Selected field of interest
  vtkDataArraySelection* CellDataArraySelection;

  // Observer to modify this object when array selections are modified
  vtkCallbackCommand* SelectionObserver;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(
    vtkInformation*, vtkInformationVector** inVector, vtkInformationVector*) override;

  static void SelectionCallback(
    vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);
  static void EventCallback(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

private:
  vtkPIOReader(const vtkPIOReader&) = delete;
  void operator=(const vtkPIOReader&) = delete;
};

#endif
