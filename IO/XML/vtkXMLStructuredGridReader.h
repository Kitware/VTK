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
  void PrintSelf(ostream& os, vtkIndent indent) override;
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
  ~vtkXMLStructuredGridReader() override;

  const char* GetDataSetName() override;
  void SetOutputExtent(int* extent) override;

  void SetupPieces(int numPieces) override;
  void DestroyPieces() override;
  void SetupOutputData() override;

  int ReadPiece(vtkXMLDataElement* ePiece) override;
  int ReadPieceData() override;
  int FillOutputPortInformation(int, vtkInformation*) override;

  // The elements representing the points for each piece.
  vtkXMLDataElement** PointElements;

private:
  vtkXMLStructuredGridReader(const vtkXMLStructuredGridReader&) = delete;
  void operator=(const vtkXMLStructuredGridReader&) = delete;
};

#endif
