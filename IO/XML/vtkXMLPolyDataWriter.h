/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPolyDataWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLPolyDataWriter - Write VTK XML PolyData files.
// .SECTION Description
// vtkXMLPolyDataWriter writes the VTK XML PolyData file format.  One
// polygonal data input can be written into one file in any number of
// streamed pieces (if supported by the rest of the pipeline).  The
// standard extension for this writer's file format is "vtp".  This
// writer is also used to write a single piece of the parallel file
// format.

// .SECTION See Also
// vtkXMLPPolyDataWriter

#ifndef __vtkXMLPolyDataWriter_h
#define __vtkXMLPolyDataWriter_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLUnstructuredDataWriter.h"

class vtkPolyData;

class VTKIOXML_EXPORT vtkXMLPolyDataWriter : public vtkXMLUnstructuredDataWriter
{
public:
  vtkTypeMacro(vtkXMLPolyDataWriter,vtkXMLUnstructuredDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkXMLPolyDataWriter* New();

  //BTX
  // Description:
  // Get/Set the writer's input.
  vtkPolyData* GetInput();
  //ETX

  // Description:
  // Get the default file extension for files written by this writer.
  const char* GetDefaultFileExtension();

protected:
  vtkXMLPolyDataWriter();
  ~vtkXMLPolyDataWriter();

  // see algorithm for more info
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  const char* GetDataSetName();

  virtual void AllocatePositionArrays();
  virtual void DeletePositionArrays();

  void WriteInlinePieceAttributes();
  void WriteInlinePiece(vtkIndent indent);

  void WriteAppendedPieceAttributes(int index);
  void WriteAppendedPiece(int index, vtkIndent indent);
  void WriteAppendedPieceData(int index);

  virtual vtkIdType GetNumberOfInputCells();
  void CalculateSuperclassFraction(float* fractions);

  // Positions of attributes for each piece.
  unsigned long* NumberOfVertsPositions;
  unsigned long* NumberOfLinesPositions;
  unsigned long* NumberOfStripsPositions;
  unsigned long* NumberOfPolysPositions;

  OffsetsManagerArray *VertsOM;
  OffsetsManagerArray *LinesOM;
  OffsetsManagerArray *StripsOM;
  OffsetsManagerArray *PolysOM;

private:
  vtkXMLPolyDataWriter(const vtkXMLPolyDataWriter&);  // Not implemented.
  void operator=(const vtkXMLPolyDataWriter&);  // Not implemented.
};

#endif
