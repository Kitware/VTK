/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataMapper.h
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
// .NAME vtkPolyDataMapper - map vtkPolyData to graphics primitives
// .SECTION Description
// vtkPolyDataMapper is a class that maps polygonal data (i.e., vtkPolyData)
// to graphics primitives. vtkPolyDataMapper serves as a superclass for
// device-specific poly data mappers, that actually do the mapping to the
// rendering/graphics hardware/software.
// It is now possible to set a memory limit for the pipeline in the mapper.
// If the total estimated memory usage of the pipeline is larger than
// this limit, the mapper will divide the data into pieces and render
// each in a for loop.

#ifndef __vtkPolyDataMapper_h
#define __vtkPolyDataMapper_h

#include "vtkMapper.h"
#include "vtkPolyData.h"
#include "vtkRenderer.h"

class VTK_RENDERING_EXPORT vtkPolyDataMapper : public vtkMapper 
{
public:
  static vtkPolyDataMapper *New();
  vtkTypeRevisionMacro(vtkPolyDataMapper,vtkMapper);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Implemented by sub classes. Actual rendering is done here.
  virtual void RenderPiece(vtkRenderer *ren, vtkActor *act) = 0;

  // Description:
  // This calls RenderPiece (in a for loop is streaming is necessary).
  virtual void Render(vtkRenderer *ren, vtkActor *act);

  // Description:
  // Specify the input data to map.
  void SetInput(vtkPolyData *in);
  vtkPolyData *GetInput();
  
  // Description:
  // Update that sets the update piece first.
  void Update();

  // Description:
  // If you want only a part of the data, specify by setting the piece.
  vtkSetMacro(Piece, int);
  vtkGetMacro(Piece, int);
  vtkSetMacro(NumberOfPieces, int);
  vtkGetMacro(NumberOfPieces, int);
  vtkSetMacro(NumberOfSubPieces, int);
  vtkGetMacro(NumberOfSubPieces, int);

  // Description:
  // Set the number of ghost cells to return.
  vtkSetMacro(GhostLevel, int);
  vtkGetMacro(GhostLevel, int);

  // Description:
  // Return bounding box (array of six floats) of data expressed as
  // (xmin,xmax, ymin,ymax, zmin,zmax).
  virtual float *GetBounds();
  virtual void GetBounds(float bounds[6]) 
    {this->vtkMapper::GetBounds(bounds);};
  
  // Description:
  // Make a shallow copy of this mapper.
  void ShallowCopy(vtkAbstractMapper *m);

protected:  
  vtkPolyDataMapper();
  ~vtkPolyDataMapper() {};

  int Piece;
  int NumberOfPieces;
  int NumberOfSubPieces;
  int GhostLevel;
private:
  vtkPolyDataMapper(const vtkPolyDataMapper&);  // Not implemented.
  void operator=(const vtkPolyDataMapper&);  // Not implemented.
};

#endif
