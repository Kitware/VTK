/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredGridSource.h
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
// .NAME vtkStructuredGridSource - Abstract class whose subclasses generates structured grid data
// .SECTION Description
// vtkStructuredGridSource is an abstract class whose subclasses generate
// structured grid data.

// .SECTION See Also
// vtkStructuredGridReader vtkPLOT3DReader

#ifndef __vtkStructuredGridSource_h
#define __vtkStructuredGridSource_h

#include "vtkSource.h"
#include "vtkStructuredGrid.h"

class VTK_FILTERING_EXPORT vtkStructuredGridSource : public vtkSource
{
public:
  vtkTypeRevisionMacro(vtkStructuredGridSource,vtkSource);

  // Description:
  // Get the output of this source.
  vtkStructuredGrid *GetOutput();
  vtkStructuredGrid *GetOutput(int idx)
    {return (vtkStructuredGrid *) this->vtkSource::GetOutput(idx); };
  void SetOutput(vtkStructuredGrid *output);  

protected:
  vtkStructuredGridSource();
  ~vtkStructuredGridSource() {};

private:
  vtkStructuredGridSource(const vtkStructuredGridSource&);  // Not implemented.
  void operator=(const vtkStructuredGridSource&);  // Not implemented.
};

#endif


