/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiGroupDataGroupIdScalars.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiGroupDataGroupIdScalars - generate scalars from groups
// .SECTION Description
// vtkMultiGroupDataGroupIdScalars is a filter to that generates scalars 
// using multi-group data group information. For example, it will assign
// an vtkUnsignedCharArray named GroupIdScalars and of value 0 to all 
// datasets in group 0.

#ifndef __vtkMultiGroupDataGroupIdScalars_h
#define __vtkMultiGroupDataGroupIdScalars_h

#include "vtkMultiGroupDataSetAlgorithm.h"

class VTK_GRAPHICS_EXPORT vtkMultiGroupDataGroupIdScalars : public vtkMultiGroupDataSetAlgorithm 
{
public:
  vtkTypeRevisionMacro(vtkMultiGroupDataGroupIdScalars,vtkMultiGroupDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with PointIds and CellIds on; and ids being generated
  // as scalars.
  static vtkMultiGroupDataGroupIdScalars *New();

protected:
  vtkMultiGroupDataGroupIdScalars();
  ~vtkMultiGroupDataGroupIdScalars();

  int RequestData(vtkInformation *, 
                  vtkInformationVector **, 
                  vtkInformationVector *);

private:
  vtkMultiGroupDataGroupIdScalars(const vtkMultiGroupDataGroupIdScalars&);  // Not implemented.
  void operator=(const vtkMultiGroupDataGroupIdScalars&);  // Not implemented.
};

#endif


