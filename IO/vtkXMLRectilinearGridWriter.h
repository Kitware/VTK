/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLRectilinearGridWriter.h
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
// .NAME vtkXMLRectilinearGridWriter - Write VTK XML RectilinearGrid files.
// .SECTION Description
// vtkXMLRectilinearGridWriter writes the VTK XML RectilinearGrid
// file format.  One rectilinear grid input can be written into one
// file in any number of streamed pieces.  The standard extension for
// this writer's file format is "vtr".  This writer is also used to
// write a single piece of the parallel file format.

// .SECTION See Also
// vtkXMLPRectilinearGridWriter

#ifndef __vtkXMLRectilinearGridWriter_h
#define __vtkXMLRectilinearGridWriter_h

#include "vtkXMLStructuredDataWriter.h"

class vtkRectilinearGrid;

class VTK_IO_EXPORT vtkXMLRectilinearGridWriter : public vtkXMLStructuredDataWriter
{
public:
  static vtkXMLRectilinearGridWriter* New();
  vtkTypeRevisionMacro(vtkXMLRectilinearGridWriter,vtkXMLStructuredDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Get/Set the writer's input.
  void SetInput(vtkRectilinearGrid* input);
  vtkRectilinearGrid* GetInput();
  
  // Description:
  // Get the default file extension for files written by this writer.
  const char* GetDefaultFileExtension();
  
protected:
  vtkXMLRectilinearGridWriter();
  ~vtkXMLRectilinearGridWriter();  
  
  void WriteAppendedMode(vtkIndent indent);
  void WriteAppendedPiece(int index, vtkIndent indent);
  void WriteAppendedPieceData(int index);
  void WriteInlinePiece(int index, vtkIndent indent);
  void GetInputExtent(int* extent);
  const char* GetDataSetName();
  vtkDataArray* CreateExactCoordinates(vtkDataArray* a, int xyz);
  
  // Coordinate array appended data positions.
  unsigned long** CoordinatePositions;
  
private:
  vtkXMLRectilinearGridWriter(const vtkXMLRectilinearGridWriter&);  // Not implemented.
  void operator=(const vtkXMLRectilinearGridWriter&);  // Not implemented.
};

#endif
