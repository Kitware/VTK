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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
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
  ~vtkXMLUnstructuredGridReader() VTK_OVERRIDE;

  const char* GetDataSetName() VTK_OVERRIDE;
  void GetOutputUpdateExtent(int& piece, int& numberOfPieces, int& ghostLevel) VTK_OVERRIDE;
  void SetupOutputTotals() VTK_OVERRIDE;
  void SetupPieces(int numPieces) VTK_OVERRIDE;
  void DestroyPieces() VTK_OVERRIDE;

  void SetupOutputData() VTK_OVERRIDE;
  int ReadPiece(vtkXMLDataElement* ePiece) VTK_OVERRIDE;
  void SetupNextPiece() VTK_OVERRIDE;
  int ReadPieceData() VTK_OVERRIDE;

  // Read a data array whose tuples correspond to cells.
  int ReadArrayForCells(vtkXMLDataElement* da,
    vtkAbstractArray* outArray) VTK_OVERRIDE;

  // Get the number of cells in the given piece.  Valid after
  // UpdateInformation.
  vtkIdType GetNumberOfCellsInPiece(int piece) VTK_OVERRIDE;

  int FillOutputPortInformation(int, vtkInformation*) VTK_OVERRIDE;

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
