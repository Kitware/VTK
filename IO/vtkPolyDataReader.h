/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataReader.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPolyDataReader - read vtk polygonal data file
// .SECTION Description
// vtkPolyDataReader is a source object that reads ASCII or binary 
// polygonal data files in vtk format (see text for format details).
// The output of this reader is a single vtkPolyData data object.
// The superclass of this class, vtkDataReader, provides many methods for
// controlling the reading of the data file, see vtkDataReader for more
// information.
// .SECTION Caveats
// Binary files written on one system may not be readable on other systems.
// .SECTION See Also
// vtkPolyData vtkDataReader

#ifndef __vtkPolyDataReader_h
#define __vtkPolyDataReader_h

#include "vtkDataReader.h"

class vtkPolyData;

class VTK_IO_EXPORT vtkPolyDataReader : public vtkDataReader
{
public:
  static vtkPolyDataReader *New();
  vtkTypeRevisionMacro(vtkPolyDataReader,vtkDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output of this reader.
  vtkPolyData *GetOutput();
  vtkPolyData *GetOutput(int idx)
    {return (vtkPolyData *) this->vtkSource::GetOutput(idx); };
  void SetOutput(vtkPolyData *output);

protected:
  vtkPolyDataReader();
  ~vtkPolyDataReader();

  void Execute();

  // Update extent of PolyData is specified in pieces.  
  // Since all DataObjects should be able to set UpdateExent as pieces,
  // just copy output->UpdateExtent  all Inputs.
  void ComputeInputUpdateExtents(vtkDataObject *output);
  
  // Used by streaming: The extent of the output being processed
  // by the execute method. Set in the ComputeInputUpdateExtents method.
  int ExecutePiece;
  int ExecuteNumberOfPieces;
  int ExecuteGhostLevel;
private:
  vtkPolyDataReader(const vtkPolyDataReader&);  // Not implemented.
  void operator=(const vtkPolyDataReader&);  // Not implemented.
};

#endif


