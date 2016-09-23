/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLHyperOctreeWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLHyperOctreeWriter
 * @brief   Write VTK XML HyperOctree files.
 *
 * vtkXMLHyperOctreeWriter writes the VTK XML HyperOctree file
 * format.  One HyperOctree input can be written into one file in
 * any number of streamed pieces.  The standard extension for this
 * writer's file format is "vto".  This writer is also used to write a
 * single piece of the parallel file format.
 *
 * @sa
 * vtkXMLPHyperOctreeWriter
*/

#ifndef vtkXMLHyperOctreeWriter_h
#define vtkXMLHyperOctreeWriter_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLWriter.h"

class vtkHyperOctree;
class vtkHyperOctreeCursor;
class vtkIntArray;

class VTKIOXML_EXPORT vtkXMLHyperOctreeWriter : public vtkXMLWriter
{
public:
  vtkTypeMacro(vtkXMLHyperOctreeWriter,vtkXMLWriter);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkXMLHyperOctreeWriter* New();

  /**
   * Get/Set the writer's input.
   */
  vtkHyperOctree* GetInput();

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension();

protected:
  vtkXMLHyperOctreeWriter();
  ~vtkXMLHyperOctreeWriter();

  const char* GetDataSetName();

  // specify that we require HyperOctree input
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // The most important method, make the XML file for my input.
  int WriteData();

  // <HyperOctree ...
  int StartPrimElement(vtkIndent);

  // ... dim, size, origin>
  void WritePrimaryElementAttributes(ostream &, vtkIndent);

  // Tree Structure
  int WriteTopology(vtkIndent);

  // Used by WriteTopology to make and array from the Tree structure recursively
  void SerializeTopology(vtkHyperOctreeCursor *, int);

  // Writes PointData and CellData attribute data.
  int WriteAttributeData(vtkIndent);

  // </HyperOctree>
  int FinishPrimElement(vtkIndent);

  // For appended mode placekeeping
  vtkIntArray *TopologyArray;
  unsigned long TopoOffset;
  OffsetsManagerGroup * TopologyOM;
  OffsetsManagerGroup * PointDataOM;
  OffsetsManagerGroup * CellDataOM;

private:
  vtkXMLHyperOctreeWriter(const vtkXMLHyperOctreeWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXMLHyperOctreeWriter&) VTK_DELETE_FUNCTION;
};

#endif
