/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTRUCHASReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkTRUCHASReader
 * @brief   read GE TRUCHAS format HDF5 files
 *
 * vtkTRUCHASReader is a source object that reads TRUCHAS simulation
 * data from HDF5 files.
*/

#ifndef vtkTRUCHASReader_h
#define vtkTRUCHASReader_h

#include "vtkIOTRUCHASModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

class vtkDataArraySelection;

class VTKIOTRUCHAS_EXPORT vtkTRUCHASReader
  : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkTRUCHASReader *New();
  vtkTypeMacro(vtkTRUCHASReader,vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Specify file name of vtk data file to read.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  /**
   * A simple, non-exhaustive check to see if a file is a valid truchas file.
   */
  static int CanReadFile(const char *filename);

  //@{
  /**
   * Get/Set information about blocks. As is typical with readers this is valid
   * only after the filename as been set and UpdateInformation() has been
   * called.
   */
  int GetNumberOfBlockArrays();
  const char* GetBlockArrayName(int index);
  void SetBlockArrayStatus(const char* gridname, int status);
  int GetBlockArrayStatus(const char* gridname);
  //@}

  /**
   * Get information about point-based arrays. As is typical with readers this
   * in only valid after the filename is set and UpdateInformation() has been
   * called.
   */
  int GetNumberOfPointArrays();

  /**
   * Returns the name of point array at the give index. Returns NULL if index is
   * invalid.
   */
  const char* GetPointArrayName(int index);

  //@{
  /**
   * Get/Set the point array status.
   */
  int GetPointArrayStatus(const char* name);
  void SetPointArrayStatus(const char* name, int status);
  //@}

  /**
   * Get information about cell-based arrays. As is typical with readers this
   * in only valid after the filename is set and UpdateInformation() has been
   * called.
   */
  int GetNumberOfCellArrays();

  /**
   * Returns the name of cell array at the give index. Returns NULL if index is
   * invalid.
   */
  const char* GetCellArrayName(int index);

  //@{
  /**
   * Get/Set the cell array status.
   */
  int GetCellArrayStatus(const char* name);
  void SetCellArrayStatus(const char* name, int status);
  //@}

protected:
  vtkTRUCHASReader();
  ~vtkTRUCHASReader();

  /**
   * Overridden to announce timesteps we can produce
   */
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *) VTK_OVERRIDE;
  /**
   * Overridden to read the file and parse into an output
   */
  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *) VTK_OVERRIDE;

  char *FileName;

  class Internal;
  Internal * Internals;
  friend class Internal;

  vtkDataArraySelection* BlockChoices;
  vtkDataArraySelection* PointArrayChoices;
  vtkDataArraySelection* CellArrayChoices;

private:
  vtkTRUCHASReader(const vtkTRUCHASReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTRUCHASReader&) VTK_DELETE_FUNCTION;

};

#endif
