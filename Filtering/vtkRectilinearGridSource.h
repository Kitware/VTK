/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRectilinearGridSource.h
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
// .NAME vtkRectilinearGridSource - Abstract class whose subclasses generates rectilinear grid data
// .SECTION Description
// vtkRectilinearGridSource is an abstract class whose subclasses generate
// rectilinear grid data.

// .SECTION See Also
// vtkRectilinearGridReader

#ifndef __vtkRectilinearGridSource_h
#define __vtkRectilinearGridSource_h

#include "vtkSource.h"
#include "vtkRectilinearGrid.h"

class VTK_FILTERING_EXPORT vtkRectilinearGridSource : public vtkSource
{
public:
  static vtkRectilinearGridSource *New();
  vtkTypeRevisionMacro(vtkRectilinearGridSource,vtkSource);

  // Description:
  // Get the output of this source.
  vtkRectilinearGrid *GetOutput();
  vtkRectilinearGrid *GetOutput(int idx)
    {return (vtkRectilinearGrid *) this->vtkSource::GetOutput(idx); };
  void SetOutput(vtkRectilinearGrid *output);

protected:
  vtkRectilinearGridSource();
  ~vtkRectilinearGridSource() {};

private:
  vtkRectilinearGridSource(const vtkRectilinearGridSource&);  // Not implemented.
  void operator=(const vtkRectilinearGridSource&);  // Not implemented.
};

#endif


