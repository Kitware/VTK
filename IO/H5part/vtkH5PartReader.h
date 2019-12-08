/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkH5PartReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Copyright (C) CSCS - Swiss National Supercomputing Centre.
  You may use modify and and distribute this code freely providing
  1) This copyright notice appears on all copies of source code
  2) An acknowledgment appears with any substantial usage of the code
  3) If this code is contributed to any other open source project, it
  must not be reformatted such that the indentation, bracketing or
  overall style is modified significantly.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

=========================================================================*/
/**
 * @class vtkH5PartReader
 * @brief Read H5Part particle files
 *
 * vtkH5PartReader reads compatible with H5Part : documented here
 * http://amas.web.psi.ch/docs/H5Part-doc/h5part.html
 *
 * @note Thanks to John Bidiscombe of
 * CSCS - Swiss National Supercomputing Centre for creating and contributing
 * the original implementation of this class.
 */

#ifndef vtkH5PartReader_h
#define vtkH5PartReader_h

#include "vtkIOH5partModule.h" // for export macro
#include "vtkPolyDataAlgorithm.h"

#include <string> // for string
#include <vector> // for vector

class vtkDataArraySelection;
struct H5PartFile;
class VTKIOH5PART_EXPORT vtkH5PartReader : public vtkPolyDataAlgorithm
{
public:
  static vtkH5PartReader* New();
  vtkTypeMacro(vtkH5PartReader, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Specify file name.
   */
  void SetFileName(char* filename);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Set/Get the array that will be used for the X coordinates
   */
  vtkGetStringMacro(Xarray);
  vtkSetStringMacro(Xarray);
  //@}

  //@{
  /**
   * Set/Get the array that will be used for the Y coordinates
   */
  vtkGetStringMacro(Yarray);
  vtkSetStringMacro(Yarray);
  //@}

  //@{
  /**
   * Set/Get the array that will be used for the Z coordinates
   */
  vtkGetStringMacro(Zarray);
  vtkSetStringMacro(Zarray);
  //@}

  //@{
  /**
   * When set (default no), the reader will generate a vertex cell
   * for each point/particle read. When using the points directly
   * this is unnecessary and time can be saved by omitting cell generation
   * vtkPointSpriteMapper does not require them.
   * When using ParaView, cell generation is recommended, without them
   * many filter operations are unavailable
   */
  vtkSetMacro(GenerateVertexCells, int);
  vtkGetMacro(GenerateVertexCells, int);
  vtkBooleanMacro(GenerateVertexCells, int);
  //@}

  //@{
  /**
   * When this option is set, scalar fields with names which form a pattern
   * of the form scalar_0, scalar_1, scalar_2 will be combined into a single
   * vector field with N components
   */
  vtkSetMacro(CombineVectorComponents, int);
  vtkGetMacro(CombineVectorComponents, int);
  vtkBooleanMacro(CombineVectorComponents, int);
  //@}

  //@{
  /**
   * Normally, a request for data at time t=x, where x is either before the start of
   * time for the data, or after the end, will result in the first or last
   * timestep of data to be retrieved (time is clamped to max/min values).
   * Forsome applications/animations, it may be desirable to not display data
   * for invalid times. When MaskOutOfTimeRangeOutput is set to ON, the reader
   * will return an empty dataset for out of range requests. This helps
   * avoid corruption of animations.
   */
  vtkSetMacro(MaskOutOfTimeRangeOutput, int);
  vtkGetMacro(MaskOutOfTimeRangeOutput, int);
  vtkBooleanMacro(MaskOutOfTimeRangeOutput, int);
  //@}

  //@{
  /**
   * An H5Part file may contain multiple arrays
   * a GUI (eg Paraview) can provide a mechanism for selecting which data arrays
   * are to be read from the file. The PointArray variables and members can
   * be used to query the names and number of arrays available
   * and set the status (on/off) for each array, thereby controlling which
   * should be read from the file. Paraview queries these point arrays after
   * the (update) information part of the pipeline has been updated, and before the
   * (update) data part is updated.
   */
  int GetNumberOfPointArrays();
  const char* GetPointArrayName(int index);
  int GetPointArrayStatus(const char* name);
  void SetPointArrayStatus(const char* name, int status);
  void DisableAll();
  void EnableAll();
  void Disable(const char* name);
  void Enable(const char* name);

  int GetNumberOfPointArrayStatusArrays() { return GetNumberOfPointArrays(); }
  const char* GetPointArrayStatusArrayName(int index) { return GetPointArrayName(index); }
  int GetPointArrayStatusArrayStatus(const char* name) { return GetPointArrayStatus(name); }
  void SetPointArrayStatusArrayStatus(const char* name, int status)
  {
    this->SetPointArrayStatus(name, status);
  }

  //@}

  //@{
  int GetNumberOfCoordinateArrays() { return GetNumberOfPointArrays(); }
  const char* GetCoordinateArrayName(int index) { return GetPointArrayName(index); }
  int GetCoordinateArrayStatus(const char* name);
  void SetCoordinateArrayStatus(const char* name, int status);
  //@}

protected:
  vtkH5PartReader();
  ~vtkH5PartReader() override;
  //
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int OpenFile();
  void CloseFile();
  //  void  CopyIntoCoords(int offset, vtkDataArray *source, vtkDataArray *dest);
  // returns 0 if no, returns 1,2,3,45 etc for the first, second...
  // example : if CombineVectorComponents is true, then
  // velocity_0 returns 1, velocity_1 returns 2 etc
  // if CombineVectorComponents is false, then
  // velocity_0 returns 0, velocity_1 returns 0 etc
  int IndexOfVectorComponent(const char* name);

  std::string NameOfVectorComponent(const char* name);

  //
  // Internal Variables
  //
  char* FileName;
  int NumberOfTimeSteps;
  int TimeStep;
  int ActualTimeStep;
  double TimeStepTolerance;
  int CombineVectorComponents;
  int GenerateVertexCells;
  H5PartFile* H5FileId;
  vtkTimeStamp FileModifiedTime;
  vtkTimeStamp FileOpenedTime;
  int MaskOutOfTimeRangeOutput;
  int TimeOutOfRange;
  //
  char* Xarray;
  char* Yarray;
  char* Zarray;

  std::vector<double> TimeStepValues;

  // To allow paraview gui to enable/disable scalar reading
  vtkDataArraySelection* PointDataArraySelection;

private:
  vtkH5PartReader(const vtkH5PartReader&) = delete;
  void operator=(const vtkH5PartReader&) = delete;
};

#endif
