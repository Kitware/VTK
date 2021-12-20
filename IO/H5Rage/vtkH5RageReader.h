/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkH5RageReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkH5RageReader
 * @brief   class for reading Rage HDF data files
 *
 * This class reads in hdf files generated from xRage, a LANL physics code.
 * The files are per variable and per cycle, hdf dataset is named Data.
 */

#ifndef vtkH5RageReader_h
#define vtkH5RageReader_h

#include "vtkIOH5RageModule.h" // For export macro
#include "vtkImageAlgorithm.h"

class vtkCallbackCommand;
class vtkDataArraySelection;
class vtkMultiProcessController;

class H5RageAdaptor;

class VTKIOH5RAGE_EXPORT vtkH5RageReader : public vtkImageAlgorithm
{
public:
  static vtkH5RageReader* New();
  vtkTypeMacro(vtkH5RageReader, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify file name of H5Rage data file to read.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  ///@{
  /**
   * Specify the timestep to be loaded
   */
  vtkSetMacro(CurrentTimeStep, int);
  vtkGetMacro(CurrentTimeStep, int);
  ///@}

  ///@{
  /**
   * Get the reader's output
   */
  vtkImageData* GetOutput();
  vtkImageData* GetOutput(int index);
  ///@}

  ///@{
  /**
   * The following methods allow selective reading of solutions fields.
   * By default, ALL data fields on the nodes are read, but this can
   * be modified.
   */
  int GetNumberOfPointArrays();
  const char* GetPointArrayName(int index);
  int GetPointArrayStatus(const char* name);
  void SetPointArrayStatus(const char* name, int status);
  void DisableAllPointArrays();
  void EnableAllPointArrays();
  ///@}

protected:
  vtkH5RageReader();
  ~vtkH5RageReader() override;

  char* FileName; // First field part file giving path

  int Rank;      // Number of this processor
  int TotalRank; // Number of processors

  H5RageAdaptor* H5rageAdaptor;

  int WholeExtent[6]; // Size of image
  int SubExtent[6];   // Size of image this processor
  int Dimension[3];   // Dimension of image
  double Origin[3];   // Physical origin
  double Spacing[3];  // Physical spacing

  int NumberOfTimeSteps;
  double* TimeSteps;   // Times available for request
  int CurrentTimeStep; // Time currently displayed

  // Controls initializing and querying MPI
  void SetController(vtkMultiProcessController*);
  vtkMultiProcessController* Controller;

  // Selected field of interest
  vtkDataArraySelection* PointDataArraySelection;

  // Observer to modify this object when array selections are modified
  vtkCallbackCommand* SelectionObserver;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(
    vtkInformation*, vtkInformationVector** inVector, vtkInformationVector*) override;

  static void SelectionCallback(
    vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);
  static void EventCallback(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

private:
  vtkH5RageReader(const vtkH5RageReader&) = delete;
  void operator=(const vtkH5RageReader&) = delete;
};

#endif
