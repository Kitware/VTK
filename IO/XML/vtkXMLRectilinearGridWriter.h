/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLRectilinearGridWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLRectilinearGridWriter
 * @brief   Write VTK XML RectilinearGrid files.
 *
 * vtkXMLRectilinearGridWriter writes the VTK XML RectilinearGrid
 * file format.  One rectilinear grid input can be written into one
 * file in any number of streamed pieces.  The standard extension for
 * this writer's file format is "vtr".  This writer is also used to
 * write a single piece of the parallel file format.
 *
 * @sa
 * vtkXMLPRectilinearGridWriter
*/

#ifndef vtkXMLRectilinearGridWriter_h
#define vtkXMLRectilinearGridWriter_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLStructuredDataWriter.h"

class vtkRectilinearGrid;

class VTKIOXML_EXPORT vtkXMLRectilinearGridWriter : public vtkXMLStructuredDataWriter
{
public:
  static vtkXMLRectilinearGridWriter* New();
  vtkTypeMacro(vtkXMLRectilinearGridWriter,vtkXMLStructuredDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Get/Set the writer's input.
   */
  vtkRectilinearGrid* GetInput();

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension();

protected:
  vtkXMLRectilinearGridWriter();
  ~vtkXMLRectilinearGridWriter();

  // see algorithm for more info
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  int WriteAppendedMode(vtkIndent indent);
  void WriteAppendedPiece(int index, vtkIndent indent);
  void WriteAppendedPieceData(int index);
  void WriteInlinePiece(vtkIndent indent);
  void GetInputExtent(int* extent);
  const char* GetDataSetName();
  void CalculateSuperclassFraction(float* fractions);

  // Coordinate array appended data positions.
  OffsetsManagerArray *CoordinateOM;

  virtual void AllocatePositionArrays();
  virtual void DeletePositionArrays();

private:
  vtkXMLRectilinearGridWriter(const vtkXMLRectilinearGridWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXMLRectilinearGridWriter&) VTK_DELETE_FUNCTION;
};

#endif
