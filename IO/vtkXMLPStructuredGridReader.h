/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPStructuredGridReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLPStructuredGridReader - Read PVTK XML StructuredGrid files.
// .SECTION Description
// vtkXMLPStructuredGridReader reads the PVTK XML StructuredGrid file
// format.  This reads the parallel format's summary file and then
// uses vtkXMLStructuredGridReader to read data from the individual
// StructuredGrid piece files.  Streaming is supported.  The standard
// extension for this reader's file format is "pvts".

// .SECTION See Also
// vtkXMLStructuredGridReader

#ifndef __vtkXMLPStructuredGridReader_h
#define __vtkXMLPStructuredGridReader_h

#include "vtkXMLPStructuredDataReader.h"

class vtkStructuredGrid;

class VTK_IO_EXPORT vtkXMLPStructuredGridReader : public vtkXMLPStructuredDataReader
{
public:
  vtkTypeMacro(vtkXMLPStructuredGridReader,vtkXMLPStructuredDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkXMLPStructuredGridReader *New();
  
  // Description:
  // Get the reader's output.
  vtkStructuredGrid *GetOutput();

  // Description:
  // Needed for ParaView
  vtkStructuredGrid* GetOutput(int idx);
  
protected:
  vtkXMLPStructuredGridReader();
  ~vtkXMLPStructuredGridReader();
  
  vtkStructuredGrid* GetPieceInput(int index);

  void SetupEmptyOutput();
  const char* GetDataSetName();
  void SetOutputExtent(int* extent);
  void GetPieceInputExtent(int index, int* extent);
  int ReadPrimaryElement(vtkXMLDataElement* ePrimary);
  void SetupOutputData();
  int ReadPieceData();
  vtkXMLDataReader* CreatePieceReader();  
  virtual int FillOutputPortInformation(int, vtkInformation*);
  
  // The PPoints element with point information.
  vtkXMLDataElement* PPointsElement;
  
private:
  vtkXMLPStructuredGridReader(const vtkXMLPStructuredGridReader&);  // Not implemented.
  void operator=(const vtkXMLPStructuredGridReader&);  // Not implemented.
};

#endif
