// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLUnstructuredGridReader
 * @brief   Read VTK XML UnstructuredGrid files.
 *
 * vtkXMLUnstructuredGridReader reads the VTK XML UnstructuredGrid
 * file format.  One unstructured grid file can be read to produce one
 * output.  Streaming is supported.  The standard extension for this
 * reader's file format is "vtu".  This reader is also used to read a
 * single piece of the parallel file format.
 *
 * The SideData and SideIds elements of a piece are read into the side data
 * and side numbering of the output when exactly one piece is read into it.
 * Side numberings of several pieces are not merged, so their side data is
 * skipped with a warning.
 *
 * @sa
 * vtkXMLPUnstructuredGridReader
 */

#ifndef vtkXMLUnstructuredGridReader_h
#define vtkXMLUnstructuredGridReader_h

#include "vtkIOXMLModule.h"   // For export macro
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO
#include "vtkXMLUnstructuredDataReader.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArraySelection;
class vtkUnstructuredGrid;
class vtkIdTypeArray;

class VTKIOXML_EXPORT VTK_MARSHALAUTO vtkXMLUnstructuredGridReader
  : public vtkXMLUnstructuredDataReader
{
public:
  vtkTypeMacro(vtkXMLUnstructuredGridReader, vtkXMLUnstructuredDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkXMLUnstructuredGridReader* New();

  ///@{
  /**
   * Get the reader's output.
   */
  vtkUnstructuredGrid* GetOutput();
  vtkUnstructuredGrid* GetOutput(int idx);
  ///@}

  ///@{
  /**
   * Which side data arrays to read, the way point and cell data arrays are
   * chosen. All are read by default.
   */
  vtkGetObjectMacro(SideDataArraySelection, vtkDataArraySelection);
  int GetNumberOfSideArrays();
  const char* GetSideArrayName(int index);
  int GetSideArrayStatus(const char* name);
  void SetSideArrayStatus(const char* name, int status);
  ///@}

  // For the specified port, copy the information this reader sets up in
  // SetupOutputInformation to outInfo
  void CopyOutputInformation(vtkInformation* outInfo, int port) override;

protected:
  vtkXMLUnstructuredGridReader();
  ~vtkXMLUnstructuredGridReader() override;

  const char* GetDataSetName() override;
  void GetOutputUpdateExtent(int& piece, int& numberOfPieces, int& ghostLevel) override;
  void SetupOutputTotals() override;
  void SetupPieces(int numPieces) override;
  void DestroyPieces() override;

  void SetupOutputData() override;
  int ReadPiece(vtkXMLDataElement* ePiece) override;
  void SetupNextPiece() override;
  int ReadPieceData() override;
  void ReadXMLData() override;
  void SetupOutputInformation(vtkInformation* outInfo) override;

  // Read the side numbering and side data of the piece read into the output.
  void ReadSides();

  // Read a data array whose tuples correspond to cells.
  int ReadArrayForCells(vtkXMLDataElement* da, vtkAbstractArray* outArray) override;

  // Get the number of cells in the given piece.  Valid after
  // UpdateInformation.
  vtkIdType GetNumberOfCellsInPiece(int piece) override;

  int FillOutputPortInformation(int, vtkInformation*) override;

  // The index of the cell in the output where the current piece
  // begins.
  vtkIdType StartCell;

  // The Cells element for each piece.
  vtkXMLDataElement** CellElements;
  vtkIdType* NumberOfCells;

  // The SideData and SideIds elements for each piece.
  vtkXMLDataElement** SideDataElements = nullptr;
  vtkXMLDataElement** SideIdsElements = nullptr;
  vtkDataArraySelection* SideDataArraySelection;

  int CellsTimeStep;
  unsigned long CellsOffset;

private:
  vtkXMLUnstructuredGridReader(const vtkXMLUnstructuredGridReader&) = delete;
  void operator=(const vtkXMLUnstructuredGridReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
