/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointSetSource.h
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
// .NAME vtkPointSetSource - abstract class whose subclasses generate point data
// .SECTION Description
// vtkPointSetSource is an abstract class whose subclasses generate pointdata.

#ifndef __vtkPointDataSource_h
#define __vtkPointDataSource_h

#include "vtkSource.h"
#include "vtkPointSet.h"

class VTK_FILTERING_EXPORT vtkPointSetSource : public vtkSource
{
public:
  static vtkPointSetSource *New();
  vtkTypeRevisionMacro(vtkPointSetSource,vtkSource);

  // Description:
  // Get the output of this source.
  vtkPointSet *GetOutput();
  vtkPointSet *GetOutput(int idx)
    {return (vtkPointSet *) this->vtkSource::GetOutput(idx); };
  void SetOutput(vtkPointSet *output);
  
protected:
  vtkPointSetSource();
  ~vtkPointSetSource() {};

private:
  vtkPointSetSource(const vtkPointSetSource&);  // Not implemented.
  void operator=(const vtkPointSetSource&);  // Not implemented.
};

#endif


