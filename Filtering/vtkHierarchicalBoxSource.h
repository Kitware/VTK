/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalBoxSource.h
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
// .NAME vtkHierarchicalBoxSource - abstract class whose subclasses generate hierarchical box data
// .SECTION Description
// vtkHierarchicalBoxSource is an abstract class whose subclasses generate 
// hierarchical box data.

#ifndef __vtkHierarchicalBoxSource_h
#define __vtkHierarchicalBoxSource_h

#include "vtkSource.h"

class vtkHierarchicalBoxDataSet;

class VTK_FILTERING_EXPORT vtkHierarchicalBoxSource : public vtkSource
{
public:
  vtkTypeRevisionMacro(vtkHierarchicalBoxSource,vtkSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output of this source.
  vtkHierarchicalBoxDataSet *GetOutput();
  vtkHierarchicalBoxDataSet *GetOutput(int idx);
  void SetOutput(vtkHierarchicalBoxDataSet *output);

protected:
  vtkHierarchicalBoxSource();
  ~vtkHierarchicalBoxSource() {};
  
  // Update extent of vtkHierarchicalBoxDataSet  is specified in pieces.  
  // Since all DataObjects should be able to set UpdateExent as pieces,
  // just copy output->UpdateExtent  all Inputs.
  void ComputeInputUpdateExtents(vtkDataObject *output);
  
private:
  vtkHierarchicalBoxSource(const vtkHierarchicalBoxSource&);  // Not implemented.
  void operator=(const vtkHierarchicalBoxSource&);  // Not implemented.
};

#endif





