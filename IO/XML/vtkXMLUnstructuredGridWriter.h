/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLUnstructuredGridWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLUnstructuredGridWriter - Write VTK XML UnstructuredGrid files.
// .SECTION Description
// vtkXMLUnstructuredGridWriter writes the VTK XML UnstructuredGrid
// file format.  One unstructured grid input can be written into one
// file in any number of streamed pieces (if supported by the rest of
// the pipeline).  The standard extension for this writer's file
// format is "vtu".  This writer is also used to write a single piece
// of the parallel file format.

// .SECTION See Also
// vtkXMLPUnstructuredGridWriter

#ifndef vtkXMLUnstructuredGridWriter_h
#define vtkXMLUnstructuredGridWriter_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLUnstructuredDataWriter.h"


class vtkUnstructuredGridBase;

class VTKIOXML_EXPORT vtkXMLUnstructuredGridWriter : public vtkXMLUnstructuredDataWriter
{
public:
  vtkTypeMacro(vtkXMLUnstructuredGridWriter,vtkXMLUnstructuredDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkXMLUnstructuredGridWriter* New();

  //BTX
  // Description:
  // Get/Set the writer's input.
  vtkUnstructuredGridBase* GetInput();
  //ETX

  // Description:
  // Get the default file extension for files written by this writer.
  const char* GetDefaultFileExtension();

protected:
  vtkXMLUnstructuredGridWriter();
  ~vtkXMLUnstructuredGridWriter();

  // see algorithm for more info
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  virtual void AllocatePositionArrays();
  virtual void DeletePositionArrays();

  const char* GetDataSetName();

  void WriteInlinePieceAttributes();
  void WriteInlinePiece(vtkIndent indent);

  void WriteAppendedPieceAttributes(int index);
  void WriteAppendedPiece(int index, vtkIndent indent);
  void WriteAppendedPieceData(int index);

  virtual vtkIdType GetNumberOfInputCells();
  void CalculateSuperclassFraction(float* fractions);

  // Positions of attributes for each piece.
  vtkTypeInt64* NumberOfCellsPositions;
  OffsetsManagerArray *CellsOM; //one per piece

private:
  vtkXMLUnstructuredGridWriter(const vtkXMLUnstructuredGridWriter&);  // Not implemented.
  void operator=(const vtkXMLUnstructuredGridWriter&);  // Not implemented.
};

#endif
