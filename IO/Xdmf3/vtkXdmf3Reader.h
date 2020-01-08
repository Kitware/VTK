/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXdmf3Reader.h
  Language:  C++

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXdmf3Reader
 * @brief   Reads <tt>eXtensible Data Model and Format</tt> files
 *
 * vtkXdmf3Reader reads XDMF data files so that they can be visualized using
 * VTK. The output data produced by this reader depends on the number of grids
 * in the data file. If the data file has a single domain with a single grid,
 * then the output type is a vtkDataSet subclass of the appropriate type,
 * otherwise it's a vtkMultiBlockDataSet.
 *
 * @warning
 * Uses the XDMF API (http://www.xdmf.org)
 */

#ifndef vtkXdmf3Reader_h
#define vtkXdmf3Reader_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkIOXdmf3Module.h" // For export macro

class vtkXdmf3ArraySelection;
class vtkGraph;

class VTKIOXDMF3_EXPORT vtkXdmf3Reader : public vtkDataObjectAlgorithm
{
public:
  static vtkXdmf3Reader* New();
  vtkTypeMacro(vtkXdmf3Reader, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set tells the reader the name of a single top level xml file to read.
   */
  void SetFileName(const char* filename);

  //@{
  /**
   * Add and remove give the reader a list of top level xml files to read.
   * Whether the set is treated as a spatial or temporal collection depends
   * on FileSeriestAsTime.
   */
  virtual void AddFileName(const char* filename);
  virtual void RemoveAllFileNames();
  //@}

  //@{
  /**
   * When true (the default) the reader treats a series of files as a temporal
   * collection. When false it treats it as a spatial partition and uses
   * an optimized top level partitioning strategy.
   */
  vtkSetMacro(FileSeriesAsTime, bool);
  vtkGetMacro(FileSeriesAsTime, bool);
  //@}

  /**
   * Determine if the file can be read with this reader.
   */
  virtual int CanReadFile(const char* filename);

  /**
   * Get information about point-based arrays. As is typical with readers this
   * in only valid after the filename is set and UpdateInformation() has been
   * called.
   */
  int GetNumberOfPointArrays();

  /**
   * Returns the name of point array at the give index. Returns nullptr if index is
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

  //@{
  /**
   * Get information about cell-based arrays.  As is typical with readers this
   * in only valid after the filename is set and UpdateInformation() has been
   * called.
   */
  int GetNumberOfCellArrays();
  const char* GetCellArrayName(int index);
  void SetCellArrayStatus(const char* name, int status);
  int GetCellArrayStatus(const char* name);
  //@}

  //@{
  /**
   * Get information about unaligned arrays.  As is typical with readers this
   * in only valid after the filename is set and UpdateInformation() has been
   * called.
   */
  int GetNumberOfFieldArrays();
  const char* GetFieldArrayName(int index);
  void SetFieldArrayStatus(const char* name, int status);
  int GetFieldArrayStatus(const char* name);
  //@}

  //@{
  /**
   * Get/Set information about grids. As is typical with readers this is valid
   * only after the filename as been set and UpdateInformation() has been
   * called.
   */
  int GetNumberOfGrids();
  const char* GetGridName(int index);
  void SetGridStatus(const char* gridname, int status);
  int GetGridStatus(const char* gridname);
  //@}

  //@{
  /**
   * Get/Set information about sets. As is typical with readers this is valid
   * only after the filename as been set and UpdateInformation() has been
   * called. Note that sets with non-zero Ghost value are not treated as sets
   * that the user can select using this API.
   */
  int GetNumberOfSets();
  const char* GetSetName(int index);
  void SetSetStatus(const char* gridname, int status);
  int GetSetStatus(const char* gridname);
  //@}

  /**
   * These methods are provided to make it easier to use the Sets in ParaView.
   */
  int GetNumberOfSetArrays() { return this->GetNumberOfSets(); }
  const char* GetSetArrayName(int index) { return this->GetSetName(index); }
  int GetSetArrayStatus(const char* name) { return this->GetSetStatus(name); }

  /**
   * SIL describes organization of/relationships between classifications
   * eg. blocks/materials/hierarchies.
   */
  virtual vtkGraph* GetSIL();

  /**
   * Every time the SIL is updated a this will return a different value.
   */
  int GetSILUpdateStamp();

protected:
  vtkXdmf3Reader();
  ~vtkXdmf3Reader() override;

  const char* FileNameInternal;
  vtkSetStringMacro(FileNameInternal);

  // Overridden to announce that we make general DataObjects.
  int FillOutputPortInformation(int port, vtkInformation* info) override;

  // Overridden to handle RDO requests the way we need to
  vtkTypeBool ProcessRequest(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  // Overridden to create the correct vtkDataObject subclass for the file.
  virtual int RequestDataObjectInternal(vtkInformationVector*);

  // Overridden to announce temporal information and to participate in
  // structured extent splitting.
  virtual int RequestInformation(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  // Read the XDMF and HDF input files and fill in vtk data objects.
  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkXdmf3ArraySelection* GetFieldArraySelection();
  vtkXdmf3ArraySelection* GetCellArraySelection();
  vtkXdmf3ArraySelection* GetPointArraySelection();
  vtkXdmf3ArraySelection* GetGridsSelection();
  vtkXdmf3ArraySelection* GetSetsSelection();
  vtkXdmf3ArraySelection* FieldArraysCache;
  vtkXdmf3ArraySelection* CellArraysCache;
  vtkXdmf3ArraySelection* PointArraysCache;
  vtkXdmf3ArraySelection* GridsCache;
  vtkXdmf3ArraySelection* SetsCache;

private:
  vtkXdmf3Reader(const vtkXdmf3Reader&) = delete;
  void operator=(const vtkXdmf3Reader&) = delete;

  bool FileSeriesAsTime;

  class Internals;
  Internals* Internal;
};

#endif
