/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLUnstructuredGridReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
 * @sa
 * vtkXMLPUnstructuredGridReader
*/

#ifndef vtkXMLUnstructuredGridReader_h
#define vtkXMLUnstructuredGridReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLUnstructuredDataReader.h"

class vtkUnstructuredGrid;
class vtkIdTypeArray;

class VTKIOXML_EXPORT vtkXMLUnstructuredGridReader : public vtkXMLUnstructuredDataReader
{
public:
  vtkTypeMacro(vtkXMLUnstructuredGridReader,vtkXMLUnstructuredDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkXMLUnstructuredGridReader *New();

  //@{
  /**
   * Get the reader's output.
   */
  vtkUnstructuredGrid *GetOutput();
  vtkUnstructuredGrid *GetOutput(int idx);
  //@}

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

  // Read a data array whose tuples correspond to cells.
  int ReadArrayForCells(vtkXMLDataElement* da,
    vtkAbstractArray* outArray) override;

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

  int CellsTimeStep;
  unsigned long CellsOffset;

private:
  vtkXMLUnstructuredGridReader(const vtkXMLUnstructuredGridReader&) = delete;
  void operator=(const vtkXMLUnstructuredGridReader&) = delete;
};

#endif
