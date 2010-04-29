/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPolyhedronMeshReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLPolyhedronMeshReader - Read VTK XML UnstructuredGrid files.
// .SECTION Description
// vtkXMLPolyhedronMeshReader reads the VTK XML PolyhedronMesh file format. 
// One unstructured grid file can be read to produce one output. Each cell 
// of the unstructured grid is a polyhedron. Streaming is supported.  The 
// standard extension for this reader's file format is "vth".  This reader 
// is also used to read a single piece of the parallel file format.

// .SECTION See Also
// vtkXMLUnstructuredGridReader

#ifndef __vtkXMLPolyhedronMeshReader_h
#define __vtkXMLPolyhedronMeshReader_h

#include "vtkXMLUnstructuredGridReader.h"

class vtkUnstructuredGrid;

class VTK_IO_EXPORT vtkXMLPolyhedronMeshReader : public vtkXMLUnstructuredGridReader
{
public:
  vtkTypeMacro(vtkXMLPolyhedronMeshReader, vtkXMLUnstructuredGridReader);
  void PrintSelf(ostream& os, vtkIndent indent);  
  static vtkXMLPolyhedronMeshReader *New();
  
protected:
  vtkXMLPolyhedronMeshReader();
  ~vtkXMLPolyhedronMeshReader();
  
  const char* GetDataSetName();
  
  void SetupPieces(int numPieces);
  int ReadPiece(vtkXMLDataElement* ePiece);
  int ReadPieceData();
  
  // The Faces element for each piece.
  vtkXMLDataElement** FaceElements;
  vtkIdType* NumberOfFaces;

private:
  vtkXMLPolyhedronMeshReader(const vtkXMLPolyhedronMeshReader&);  // Not implemented.
  void operator=(const vtkXMLPolyhedronMeshReader&);  // Not implemented.
};

#endif
