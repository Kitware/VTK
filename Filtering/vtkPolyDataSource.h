/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataSource.h
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
// .NAME vtkPolyDataSource - abstract class whose subclasses generate polygonal data
// .SECTION Description
// vtkPolyDataSource is an abstract class whose subclasses generate polygonal
// data.

// .SECTION See Also
// vtkPolyDataReader vtkAxes vtkBYUReader vtkConeSource vtkCubeSource
// vtkCursor3D vtkCyberReader vtkCylinderSource vtkDiskSource vtkLineSource
// vtkMCubesReader vtkOutlineSource vtkPlaneSource vtkPointSource vtkSTLReader
// vtkSphereSource vtkTextSource vtkUGFacetReader vtkVectorText

#ifndef __vtkPolyDataSource_h
#define __vtkPolyDataSource_h

#include "vtkSource.h"
#include "vtkPolyData.h"

class VTK_FILTERING_EXPORT vtkPolyDataSource : public vtkSource
{
public:
  vtkTypeRevisionMacro(vtkPolyDataSource,vtkSource);

  // Description:
  // Get the output of this source.
  vtkPolyData *GetOutput();
  vtkPolyData *GetOutput(int idx)
    {return (vtkPolyData *) this->vtkSource::GetOutput(idx); };
  void SetOutput(vtkPolyData *output);

protected:
  vtkPolyDataSource();
  ~vtkPolyDataSource() {};
  
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
  vtkPolyDataSource(const vtkPolyDataSource&);  // Not implemented.
  void operator=(const vtkPolyDataSource&);  // Not implemented.
};

#endif





