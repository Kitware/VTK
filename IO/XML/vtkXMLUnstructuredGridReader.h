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
  void PrintSelf(ostream& os, vtkIndent indent);
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
  ~vtkXMLUnstructuredGridReader();

  const char* GetDataSetName();
  void GetOutputUpdateExtent(int& piece, int& numberOfPieces, int& ghostLevel);
  void SetupOutputTotals();
  void SetupPieces(int numPieces);
  void DestroyPieces();

  void SetupOutputData();
  int ReadPiece(vtkXMLDataElement* ePiece);
  void SetupNextPiece();
  int ReadPieceData();

  // Read a data array whose tuples correspond to cells.
  virtual int ReadArrayForCells(vtkXMLDataElement* da,
    vtkAbstractArray* outArray);

  // Get the number of cells in the given piece.  Valid after
  // UpdateInformation.
  virtual vtkIdType GetNumberOfCellsInPiece(int piece);

  virtual int FillOutputPortInformation(int, vtkInformation*);

  // The index of the cell in the output where the current piece
  // begins.
  vtkIdType StartCell;

  // The Cells element for each piece.
  vtkXMLDataElement** CellElements;
  vtkIdType* NumberOfCells;

  int CellsTimeStep;
  unsigned long CellsOffset;

private:
  vtkXMLUnstructuredGridReader(const vtkXMLUnstructuredGridReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXMLUnstructuredGridReader&) VTK_DELETE_FUNCTION;
};

#endif
