/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHDFReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHDFReader
 * @brief   VTKHDF format reader.
 *
 */

#ifndef vtkHDFReader_h
#define vtkHDFReader_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkIOHDFModule.h" // For export macro
#include <vector>           // For storing list of values

class vtkAbstractArray;
class vtkCallbackCommand;
class vtkCommand;
class vtkDataArraySelection;
class vtkDataSet;
class vtkDataSetAttributes;
class vtkImageData;
class vtkInformationVector;
class vtkInformation;
class vtkOverlappingAMR;
class vtkUnstructuredGrid;

/**
 * @class vtkHDFReader
 * @brief  Read VTK HDF files.
 *
 * Reads data saved using the VTK HDF format which supports all
 * vtkDataSet types (image data and unstructured grid are currently
 * implemented) and serial as well as parallel processing.
 *
 */
class VTKIOHDF_EXPORT vtkHDFReader : public vtkDataObjectAlgorithm
{
public:
  static vtkHDFReader* New();
  vtkTypeMacro(vtkHDFReader, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the name of the input file.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  /**
   * Test whether the file (type) with the given name can be read by this
   * reader. If the file has a newer version than the reader, we still say
   * we can read the file type and we fail later, when we try to read the file.
   * This enables clients (ParaView) to distinguish between failures when we
   * need to look for another reader and failures when we don't.
   */
  virtual int CanReadFile(VTK_FILEPATH const char* name);

  ///@{
  /**
   * Get the output as a vtkDataSet pointer.
   */
  vtkDataSet* GetOutputAsDataSet();
  vtkDataSet* GetOutputAsDataSet(int index);
  ///@}

  ///@{
  /**
   * Get the data array selection tables used to configure which data
   * arrays are loaded by the reader.
   */
  virtual vtkDataArraySelection* GetPointDataArraySelection();
  virtual vtkDataArraySelection* GetCellDataArraySelection();
  virtual vtkDataArraySelection* GetFieldDataArraySelection();
  ///@}

  ///@{
  /**
   * Get the number of point or cell arrays available in the input.
   */
  int GetNumberOfPointArrays();
  int GetNumberOfCellArrays();
  ///@}

  ///@{
  /**
   * Get the name of the point or cell array with the given index in
   * the input.
   */
  const char* GetPointArrayName(int index);
  const char* GetCellArrayName(int index);
  ///@}

  vtkSetMacro(MaximumLevelsToReadByDefaultForAMR, unsigned int);
  vtkGetMacro(MaximumLevelsToReadByDefaultForAMR, unsigned int);

protected:
  vtkHDFReader();
  ~vtkHDFReader() override;

  /**
   * How many attribute types we have. This returns 3: point, cell and field
   * attribute types.
   */
  constexpr static int GetNumberOfAttributeTypes() { return 3; }

  /**
   * Test if the reader can read a file with the given version number.
   */
  int CanReadFileVersion(int major, int minor);

  ///@{
  /**
   * Reads the 'data' requested in 'outInfo' (through extents or
   * pieces). Returns 1 if successful, 0 otherwise.
   */
  int Read(vtkInformation* outInfo, vtkImageData* data);
  int Read(vtkInformation* outInfo, vtkUnstructuredGrid* data);
  int Read(vtkInformation* outInfo, vtkOverlappingAMR* data);
  ///@}
  /**
   * Read 'pieceData' specified by 'filePiece' where
   * number of points, cells and connectivity ids
   * store those numbers for all pieces.
   */
  int Read(const std::vector<vtkIdType>& numberOfPoints,
    const std::vector<vtkIdType>& numberOfCells,
    const std::vector<vtkIdType>& numberOfConnectivityIds, int filePiece,
    vtkUnstructuredGrid* pieceData);
  /**
   * Read the field arrays from the file and add them to the dataset.
   */
  int AddFieldArrays(vtkDataObject* data);

  /**
   * Modify this object when an array selection is changed.
   */
  static void SelectionModifiedCallback(
    vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

  ///@{
  /**
   * Standard functions to specify the type, information and read the data from
   * the file.
   */
  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  ///@}

  /**
   * Print update number of pieces, piece number and ghost levels.
   */
  void PrintPieceInformation(vtkInformation* outInfo);

private:
  vtkHDFReader(const vtkHDFReader&) = delete;
  void operator=(const vtkHDFReader&) = delete;

protected:
  /**
   * The input file's name.
   */
  char* FileName;

  /**
   * The array selections.
   * in the same order as vtkDataObject::AttributeTypes: POINT, CELL, FIELD
   */
  vtkDataArraySelection* DataArraySelection[3];

  /**
   * The observer to modify this object when the array selections are
   * modified.
   */
  vtkCallbackCommand* SelectionObserver;
  ///@{
  /**
   * Image data topology and geometry.
   */
  int WholeExtent[6];
  double Origin[3];
  double Spacing[3];
  ///@}

  unsigned int MaximumLevelsToReadByDefaultForAMR = 0;

  class Implementation;
  Implementation* Impl;
};

#endif
