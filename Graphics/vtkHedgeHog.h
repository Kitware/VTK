/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHedgeHog.h
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
// .NAME vtkHedgeHog - create oriented lines from vector data
// .SECTION Description
// vtkHedgeHog creates oriented lines from the input data set. Line
// length is controlled by vector (or normal) magnitude times scale
// factor. If VectorMode is UseNormal, normals determine the orientation
// of the lines. Lines are colored by scalar data, if available.

#ifndef __vtkHedgeHog_h
#define __vtkHedgeHog_h

#include "vtkDataSetToPolyDataFilter.h"

#define VTK_USE_VECTOR 0
#define VTK_USE_NORMAL 1

class VTK_GRAPHICS_EXPORT vtkHedgeHog : public vtkDataSetToPolyDataFilter
{
public:
  static vtkHedgeHog *New();
  vtkTypeRevisionMacro(vtkHedgeHog,vtkDataSetToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set scale factor to control size of oriented lines.
  vtkSetMacro(ScaleFactor,float);
  vtkGetMacro(ScaleFactor,float);

  // Description:
  // Specify whether to use vector or normal to perform vector operations.
  vtkSetMacro(VectorMode,int);
  vtkGetMacro(VectorMode,int);
  void SetVectorModeToUseVector() {this->SetVectorMode(VTK_USE_VECTOR);};
  void SetVectorModeToUseNormal() {this->SetVectorMode(VTK_USE_NORMAL);};
  const char *GetVectorModeAsString();

protected:
  vtkHedgeHog();
  ~vtkHedgeHog() {};

  void Execute();
  float ScaleFactor;
  int VectorMode; // Orient/scale via normal or via vector data

private:
  vtkHedgeHog(const vtkHedgeHog&);  // Not implemented.
  void operator=(const vtkHedgeHog&);  // Not implemented.
};

// Description:
// Return the vector mode as a character string.
inline const char *vtkHedgeHog::GetVectorModeAsString(void)
{
  if ( this->VectorMode == VTK_USE_VECTOR) 
    {
    return "UseVector";
    }
  else if ( this->VectorMode == VTK_USE_NORMAL) 
    {
    return "UseNormal";
    }
  else 
    {
    return "Unknown";
    }
}
#endif


