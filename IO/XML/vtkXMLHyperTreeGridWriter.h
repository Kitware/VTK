/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLHyperTreeGridWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLHyperTreeGridWriter
 * @brief   Write VTK XML HyperTreeGrid files.
 *
 * vtkXMLHyperTreeGridWriter writes the VTK XML HyperTreeGrid file
 * format.  One HyperTreeGrid input can be written into one file in
 * any number of streamed pieces.  The standard extension for this
 * writer's file format is "vhg".  This writer is also used to write a
 * single piece of the parallel file format.
 *
 * @sa
 * vtkXMLPHyperTreeGridWriter
*/

#ifndef vtkXMLHyperTreeGridWriter_h
#define vtkXMLHyperTreeGridWriter_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLWriter.h"

#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeCursor.h"
#include "vtkStdString.h"

class VTKIOXML_EXPORT vtkXMLHyperTreeGridWriter : public vtkXMLWriter
{
public:
  vtkTypeMacro(vtkXMLHyperTreeGridWriter,vtkXMLWriter);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  static vtkXMLHyperTreeGridWriter* New();

  /**
   * Get/Set the writer's input.
   */
  vtkHyperTreeGrid* GetInput();

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() VTK_OVERRIDE;

protected:
  vtkXMLHyperTreeGridWriter();
  ~vtkXMLHyperTreeGridWriter() VTK_OVERRIDE;

  const char* GetDataSetName() VTK_OVERRIDE;

  // specify that we require HyperTreeGrid input
  int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  // The most important method, make the XML file for my input.
  int WriteData() VTK_OVERRIDE;

  // <HyperTreeGrid ...
  int StartPrimElement(vtkIndent);

  // ... dim, size, origin>
  void WritePrimaryElementAttributes(ostream &, vtkIndent) VTK_OVERRIDE;

  // Grid coordinates (if origin and scale are not specified)
  void WriteGridCoordinates(vtkIndent);

  // Tree Structure
  int WriteDescriptor(vtkIndent);

  // Used by WriteDescriptor to make an array from the Tree structure recursively
  void BuildDescriptor(void *, int, vtkStdString*);

  // Writes PointData and CellData attribute data.
  int WriteAttributeData(vtkIndent);

  // </HyperTreeGrid>
  int FinishPrimElement(vtkIndent);

private:
  vtkXMLHyperTreeGridWriter(const vtkXMLHyperTreeGridWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXMLHyperTreeGridWriter&) VTK_DELETE_FUNCTION;
};

#endif
