/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLStructuredGridReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLStructuredGridReader - Read VTK XML StructuredGrid files.
// .SECTION Description
// vtkXMLStructuredGridReader reads the VTK XML StructuredGrid file
// format.  One structured grid file can be read to produce one
// output.  Streaming is supported.  The standard extension for this
// reader's file format is "vts".  This reader is also used to read a
// single piece of the parallel file format.

// .SECTION See Also
// vtkXMLPStructuredGridReader

#ifndef __vtkXMLStructuredGridReader_h
#define __vtkXMLStructuredGridReader_h

#include "vtkXMLStructuredDataReader.h"

class vtkStructuredGrid;

class VTK_IO_EXPORT vtkXMLStructuredGridReader : public vtkXMLStructuredDataReader
{
public:
  vtkTypeMacro(vtkXMLStructuredGridReader,vtkXMLStructuredDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkXMLStructuredGridReader *New();
  
  // Description:
  // Get the reader's output.
  vtkStructuredGrid *GetOutput();
  vtkStructuredGrid *GetOutput(int idx);

protected:
  vtkXMLStructuredGridReader();
  ~vtkXMLStructuredGridReader();  
  
  const char* GetDataSetName();
  void SetOutputExtent(int* extent);
  
  void SetupPieces(int numPieces);
  void DestroyPieces();
  void SetupOutputData();
  
  int ReadPiece(vtkXMLDataElement* ePiece);
  int ReadPieceData();
  virtual int FillOutputPortInformation(int, vtkInformation*);
  
  // The elements representing the points for each piece.
  vtkXMLDataElement** PointElements;
  
private:
  vtkXMLStructuredGridReader(const vtkXMLStructuredGridReader&);  // Not implemented.
  void operator=(const vtkXMLStructuredGridReader&);  // Not implemented.
};

#endif
