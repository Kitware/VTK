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
 * format. The standard extension for this writer's file format is "htg".
*/

#ifndef vtkXMLHyperTreeGridWriter_h
#define vtkXMLHyperTreeGridWriter_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLWriter.h"

class vtkHyperTreeGrid;
class vtkHyperTreeGridNonOrientedCursor;

class VTKIOXML_EXPORT vtkXMLHyperTreeGridWriter : public vtkXMLWriter
{
public:
  vtkTypeMacro(vtkXMLHyperTreeGridWriter,vtkXMLWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkXMLHyperTreeGridWriter* New();

  //@{
  /**
   * Get/Set the number of pieces used to stream the table through the
   * pipeline while writing to the file.
   */
  vtkSetMacro(NumberOfPieces, int);
  vtkGetMacro(NumberOfPieces, int);
  //@}

  //@{
  /**
   * Get/Set the piece to write to the file.  If this is
   * negative or equal to the NumberOfPieces, all pieces will be written.
   */
  vtkSetMacro(WritePiece, int);
  vtkGetMacro(WritePiece, int);
  //@}

  /**
   * Get/Set the writer's input.
   */
  vtkHyperTreeGrid* GetInput();

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() override;

protected:
  vtkXMLHyperTreeGridWriter();
  ~vtkXMLHyperTreeGridWriter() override;

  const char* GetDataSetName() override;

  // specify that we require HyperTreeGrid input
  int FillInputPortInformation(int port, vtkInformation* info) override;

  // The most important method, make the XML file for my input.
  int WriteData() override;

  // <HyperTreeGrid ...
  int StartPrimaryElement(vtkIndent);

  // ... dim, size, origin>
  void WritePrimaryElementAttributes(ostream &, vtkIndent) override;

  // Grid coordinates and mask
  int WriteGrid(vtkIndent);

  // Tree Descriptor and  PointData
  int WriteTrees(vtkIndent);

  // </HyperTreeGrid>
  int FinishPrimaryElement(vtkIndent);

  /**
   * Number of pieces used for streaming.
   */
  int NumberOfPieces;

  /**
   * Which piece to write, if not all.
   */
  int WritePiece;

private:
  vtkXMLHyperTreeGridWriter(const vtkXMLHyperTreeGridWriter&) = delete;
  void operator=(const vtkXMLHyperTreeGridWriter&) = delete;
};

#endif
