/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPolyDataWriter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
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

#include "vtkXMLUnstructuredDataWriter.h"

class vtkPolyData;

class VTK_IO_EXPORT vtkXMLPolyDataWriter : public vtkXMLUnstructuredDataWriter
{
public:
  vtkTypeRevisionMacro(vtkXMLPolyDataWriter,vtkXMLUnstructuredDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkXMLPolyDataWriter* New();
  
  // Description:
  // Get/Set the writer's input.
  void SetInput(vtkPolyData* input);
  vtkPolyData* GetInput();
  
  // Description:
  // Get the default file extension for files written by this writer.
  const char* GetDefaultFileExtension();
  
protected:
  vtkXMLPolyDataWriter();
  ~vtkXMLPolyDataWriter();  

  const char* GetDataSetName();
  void SetInputUpdateExtent(int piece, int numPieces, int ghostLevel);
  
  void WriteInlinePieceAttributes();
  void WriteInlinePiece(vtkIndent indent);
  
  void WriteAppendedMode(vtkIndent indent);
  void WriteAppendedPieceAttributes(int index);
  void WriteAppendedPiece(int index, vtkIndent indent);
  void WriteAppendedPieceData(int index);

  // Positions of attributes for each piece.
  unsigned long* NumberOfVertsPositions;
  unsigned long* NumberOfLinesPositions;
  unsigned long* NumberOfStripsPositions;
  unsigned long* NumberOfPolysPositions;
  unsigned long** VertsPositions;
  unsigned long** LinesPositions;
  unsigned long** StripsPositions;
  unsigned long** PolysPositions;
private:
  vtkXMLPolyDataWriter(const vtkXMLPolyDataWriter&);  // Not implemented.
  void operator=(const vtkXMLPolyDataWriter&);  // Not implemented.
};

#endif
