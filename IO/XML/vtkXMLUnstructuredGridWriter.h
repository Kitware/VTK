/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLUnstructuredGridWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLUnstructuredGridWriter
 * @brief   Write VTK XML UnstructuredGrid files.
 *
 * vtkXMLUnstructuredGridWriter writes the VTK XML UnstructuredGrid
 * file format.  One unstructured grid input can be written into one
 * file in any number of streamed pieces (if supported by the rest of
 * the pipeline).  The standard extension for this writer's file
 * format is "vtu".  This writer is also used to write a single piece
 * of the parallel file format.
 *
 * @sa
 * vtkXMLPUnstructuredGridWriter
*/

#ifndef vtkXMLUnstructuredGridWriter_h
#define vtkXMLUnstructuredGridWriter_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLUnstructuredDataWriter.h"


class vtkUnstructuredGridBase;

class VTKIOXML_EXPORT vtkXMLUnstructuredGridWriter : public vtkXMLUnstructuredDataWriter
{
public:
  vtkTypeMacro(vtkXMLUnstructuredGridWriter,vtkXMLUnstructuredDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  static vtkXMLUnstructuredGridWriter* New();

  /**
   * Get/Set the writer's input.
   */
  vtkUnstructuredGridBase* GetInput();

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() VTK_OVERRIDE;

protected:
  vtkXMLUnstructuredGridWriter();
  ~vtkXMLUnstructuredGridWriter() VTK_OVERRIDE;

  // see algorithm for more info
  int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  void AllocatePositionArrays() VTK_OVERRIDE;
  void DeletePositionArrays() VTK_OVERRIDE;

  const char* GetDataSetName() VTK_OVERRIDE;

  void WriteInlinePieceAttributes() VTK_OVERRIDE;
  void WriteInlinePiece(vtkIndent indent) VTK_OVERRIDE;

  void WriteAppendedPieceAttributes(int index) VTK_OVERRIDE;
  void WriteAppendedPiece(int index, vtkIndent indent) VTK_OVERRIDE;
  void WriteAppendedPieceData(int index) VTK_OVERRIDE;

  vtkIdType GetNumberOfInputCells() VTK_OVERRIDE;
  void CalculateSuperclassFraction(float* fractions);

  // Positions of attributes for each piece.
  vtkTypeInt64* NumberOfCellsPositions;
  OffsetsManagerArray *CellsOM; //one per piece

private:
  vtkXMLUnstructuredGridWriter(const vtkXMLUnstructuredGridWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXMLUnstructuredGridWriter&) VTK_DELETE_FUNCTION;
};

#endif
