/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetSource.h
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
// .NAME vtkDataSetSource - abstract class whose subclasses generate datasets
// .SECTION Description
// vtkDataSetSource is an abstract class whose subclasses generate datasets.

#ifndef __vtkDataSetSource_h
#define __vtkDataSetSource_h

#include "vtkSource.h"
#include "vtkDataSet.h"

class VTK_FILTERING_EXPORT vtkDataSetSource : public vtkSource
{
public:
  static vtkDataSetSource *New();
  vtkTypeRevisionMacro(vtkDataSetSource,vtkSource);

  // Description:
  // Get the output of this source.
  vtkDataSet *GetOutput();
  vtkDataSet *GetOutput(int idx)
    {return (vtkDataSet *) this->vtkSource::GetOutput(idx); };
  void SetOutput(vtkDataSet *);
  
protected:  
  vtkDataSetSource();
  ~vtkDataSetSource() {};

private:
  vtkDataSetSource(const vtkDataSetSource&);  // Not implemented.
  void operator=(const vtkDataSetSource&);  // Not implemented.
};

#endif


