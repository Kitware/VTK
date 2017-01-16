/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHoudiniPolyDataWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHoudiniPolyDataWriter
 * @brief   write vtk polygonal data to Houdini file.
 *
 *
 * vtkHoudiniPolyDataWriter is a source object that writes VTK polygonal data
 * files in ASCII Houdini format (see
 * http://www.sidefx.com/docs/houdini15.0/io/formats/geo).
*/

#ifndef vtkHoudiniPolyDataWriter_h
#define vtkHoudiniPolyDataWriter_h

#include "vtkIOGeometryModule.h" // For export macro
#include "vtkWriter.h"

class vtkPolyData;

class VTKIOGEOMETRY_EXPORT vtkHoudiniPolyDataWriter : public vtkWriter
{
public:
  static vtkHoudiniPolyDataWriter* New();
  vtkTypeMacro(vtkHoudiniPolyDataWriter, vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Specifies the delimited text file to be loaded.
   */
  vtkGetStringMacro(FileName);
  vtkSetStringMacro(FileName);
  //@}

protected:
  vtkHoudiniPolyDataWriter();
  ~vtkHoudiniPolyDataWriter() VTK_OVERRIDE;

  void WriteData() VTK_OVERRIDE;

  int FillInputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;

  char* FileName;

private:
  vtkHoudiniPolyDataWriter(const vtkHoudiniPolyDataWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkHoudiniPolyDataWriter&) VTK_DELETE_FUNCTION;

};

#endif
