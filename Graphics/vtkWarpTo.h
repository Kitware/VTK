/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWarpTo.h
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
// .NAME vtkWarpTo - deform geometry by warping towards a point
// .SECTION Description
// vtkWarpTo is a filter that modifies point coordinates by moving the
// points towards a user specified position.

#ifndef __vtkWarpTo_h
#define __vtkWarpTo_h

#include "vtkPointSetToPointSetFilter.h"

class VTK_GRAPHICS_EXPORT vtkWarpTo : public vtkPointSetToPointSetFilter
{
public:
  static vtkWarpTo *New();
  vtkTypeRevisionMacro(vtkWarpTo,vtkPointSetToPointSetFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the value to scale displacement.
  vtkSetMacro(ScaleFactor,float);
  vtkGetMacro(ScaleFactor,float);

  // Description:
  // Set/Get the position to warp towards.
  vtkGetVectorMacro(Position,float,3);
  vtkSetVector3Macro(Position,float);

  // Description:
  // Set/Get the Absolute ivar. Turning Absolute on causes scale factor
  // of the new position to be one unit away from Position.
  vtkSetMacro(Absolute,int);
  vtkGetMacro(Absolute,int);
  vtkBooleanMacro(Absolute,int);
  
protected:
  vtkWarpTo(); 
  ~vtkWarpTo() {};

  void Execute();
  float ScaleFactor;
  float Position[3];
  int   Absolute;
private:
  vtkWarpTo(const vtkWarpTo&);  // Not implemented.
  void operator=(const vtkWarpTo&);  // Not implemented.
};

#endif
