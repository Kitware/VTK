/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPolyhedronMeshWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLPolyhedronMeshWriter - Write VTK XML PolyhedronMesh files.
// .SECTION Description
// vtkXMLPolyhedronMeshWriter writes the VTK XML PolyhedronMesh
// file format. It extracts from its input polyhedron mesh (formulated as 
// an unstructured grid) all VTK_POLYHEDRON cells and write them into a 
// file with standard extension "vth".
// One polyhedron mesh input can be written into one file in any number 
// of streamed pieces (if supported by the rest of the pipeline).  The 
// standard extension for this writer's file format is "vth".  This writer 
// is also used to write a single piece of the parallel file format.

// .SECTION See Also
// vtkXMLUnstructuredGridWriter

#ifndef __vtkXMLPolyhedronMeshWriter_h
#define __vtkXMLPolyhedronMeshWriter_h

#include "vtkXMLUnstructuredGridWriter.h"

class vtkUnstructuredGrid;
class vtkCellArray;
class vtkIdTypeArray;

class VTK_IO_EXPORT vtkXMLPolyhedronMeshWriter : public vtkXMLUnstructuredGridWriter
{
public:
  vtkTypeMacro(vtkXMLPolyhedronMeshWriter, vtkXMLUnstructuredGridWriter);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkXMLPolyhedronMeshWriter* New();

  //BTX
  // Description:
  // Get/Set the writer's input.
  vtkUnstructuredGrid* GetInput();  
  //ETX
  
  // Description:
  // Get the default file extension for files written by this writer.
  const char* GetDefaultFileExtension();

  // See the vtkAlgorithm for a desciption of what these do
  int ProcessRequest(vtkInformation*,
                     vtkInformationVector**,
                     vtkInformationVector*);
  
protected:
  vtkXMLPolyhedronMeshWriter();
  ~vtkXMLPolyhedronMeshWriter();  
  
  // see algorithm for more info
  virtual const char* GetDataSetName();


  virtual void WriteInlinePieceAttributes();
  virtual void WriteInlinePiece(vtkIndent indent);
  virtual void CalculateSuperclassFraction(float* fractions);
  
  vtkIdType GetNumberOfInputFaces();
  vtkIdType GetSizeOfFaceConnectivityArray();
  void WriteFacesInline(const char* name, 
                        vtkUnstructuredGrid* input, vtkIndent indent);
  void CalculateFaceFractions(float* fractions);
  
  void ConstructArrays(vtkIdTypeArray * connectivityArray, 
                        vtkIdTypeArray * offsetArray);
  
private:
  vtkXMLPolyhedronMeshWriter(const vtkXMLPolyhedronMeshWriter&);  // Not implemented.
  void operator=(const vtkXMLPolyhedronMeshWriter&);  // Not implemented.
};

#endif
