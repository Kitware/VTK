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
/**
 * @class   vtkXMLStructuredGridReader
 * @brief   Read VTK XML StructuredGrid files.
 *
 * vtkXMLStructuredGridReader reads the VTK XML StructuredGrid file
 * format.  One structured grid file can be read to produce one
 * output.  Streaming is supported.  The standard extension for this
 * reader's file format is "vts".  This reader is also used to read a
 * single piece of the parallel file format.
 *
 * @sa
 * vtkXMLPStructuredGridReader
*/

#ifndef vtkXMLStructuredGridReader_h
#define vtkXMLStructuredGridReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLStructuredDataReader.h"

class vtkStructuredGrid;

class VTKIOXML_EXPORT vtkXMLStructuredGridReader : public vtkXMLStructuredDataReader
{
public:
  vtkTypeMacro(vtkXMLStructuredGridReader,vtkXMLStructuredDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkXMLStructuredGridReader *New();

  //@{
  /**
   * Get the reader's output.
   */
  vtkStructuredGrid *GetOutput();
  vtkStructuredGrid *GetOutput(int idx);
  //@}

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
  vtkXMLStructuredGridReader(const vtkXMLStructuredGridReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXMLStructuredGridReader&) VTK_DELETE_FUNCTION;
};

#endif
