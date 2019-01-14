/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLRectilinearGridReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLRectilinearGridReader
 * @brief   Read VTK XML RectilinearGrid files.
 *
 * vtkXMLRectilinearGridReader reads the VTK XML RectilinearGrid file
 * format.  One rectilinear grid file can be read to produce one
 * output.  Streaming is supported.  The standard extension for this
 * reader's file format is "vtr".  This reader is also used to read a
 * single piece of the parallel file format.
 *
 * @sa
 * vtkXMLPRectilinearGridReader
*/

#ifndef vtkXMLRectilinearGridReader_h
#define vtkXMLRectilinearGridReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLStructuredDataReader.h"

class vtkRectilinearGrid;

class VTKIOXML_EXPORT vtkXMLRectilinearGridReader : public vtkXMLStructuredDataReader
{
public:
  vtkTypeMacro(vtkXMLRectilinearGridReader,vtkXMLStructuredDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkXMLRectilinearGridReader *New();

  //@{
  /**
   * Get the reader's output.
   */
  vtkRectilinearGrid *GetOutput();
  vtkRectilinearGrid *GetOutput(int idx);
  //@}

protected:
  vtkXMLRectilinearGridReader();
  ~vtkXMLRectilinearGridReader() override;

  const char* GetDataSetName() override;
  void SetOutputExtent(int* extent) override;

  void SetupPieces(int numPieces) override;
  void DestroyPieces() override;
  void SetupOutputData() override;
  int ReadPiece(vtkXMLDataElement* ePiece) override;
  int ReadPieceData() override;
  int ReadSubCoordinates(int* inBounds, int* outBounds, int* subBounds,
                         vtkXMLDataElement* da, vtkDataArray* array);
  int FillOutputPortInformation(int, vtkInformation*) override;

  // The elements representing the coordinate arrays for each piece.
  vtkXMLDataElement** CoordinateElements;

private:
  vtkXMLRectilinearGridReader(const vtkXMLRectilinearGridReader&) = delete;
  void operator=(const vtkXMLRectilinearGridReader&) = delete;
};

#endif
