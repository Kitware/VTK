/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPUnstructuredGridReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLPUnstructuredGridReader - Read PVTK XML UnstructuredGrid files.
// .SECTION Description
// vtkXMLPUnstructuredGridReader reads the PVTK XML UnstructuredGrid
// file format.  This reads the parallel format's summary file and
// then uses vtkXMLUnstructuredGridReader to read data from the
// individual UnstructuredGrid piece files.  Streaming is supported.
// The standard extension for this reader's file format is "pvtu".

// .SECTION See Also
// vtkXMLUnstructuredGridReader

#ifndef vtkXMLPUnstructuredGridReader_h
#define vtkXMLPUnstructuredGridReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLPUnstructuredDataReader.h"

class vtkUnstructuredGrid;

class VTKIOXML_EXPORT vtkXMLPUnstructuredGridReader : public vtkXMLPUnstructuredDataReader
{
public:
  vtkTypeMacro(vtkXMLPUnstructuredGridReader,vtkXMLPUnstructuredDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkXMLPUnstructuredGridReader *New();

  // Description:
  // Get the reader's output.
  vtkUnstructuredGrid *GetOutput();
  vtkUnstructuredGrid *GetOutput(int idx);

protected:
  vtkXMLPUnstructuredGridReader();
  ~vtkXMLPUnstructuredGridReader();

  const char* GetDataSetName();
  void GetOutputUpdateExtent(int& piece, int& numberOfPieces, int& ghostLevel);
  void SetupOutputTotals();

  void SetupOutputData();
  void SetupNextPiece();
  int ReadPieceData();

  void CopyArrayForCells(vtkDataArray* inArray, vtkDataArray* outArray);
  vtkXMLDataReader* CreatePieceReader();
  virtual int FillOutputPortInformation(int, vtkInformation*);

  // The index of the cell in the output where the current piece
  // begins.
  vtkIdType StartCell;

private:
  vtkXMLPUnstructuredGridReader(const vtkXMLPUnstructuredGridReader&);  // Not implemented.
  void operator=(const vtkXMLPUnstructuredGridReader&);  // Not implemented.
};

#endif
