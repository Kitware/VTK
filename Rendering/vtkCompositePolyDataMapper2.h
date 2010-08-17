/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositePolyDataMapper2.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCompositePolyDataMapper2 - mapper for composite dataset consisting
// of polygonal data.
// .SECTION Description
// vtkCompositePolyDataMapper2 is similar to vtkCompositePolyDataMapper except
// that instead of creating individual mapper for each block in the composite
// dataset, it iterates over the blocks internally. 

#ifndef __vtkCompositePolyDataMapper2_h
#define __vtkCompositePolyDataMapper2_h

#include "vtkPainterPolyDataMapper.h"

class VTK_RENDERING_EXPORT vtkCompositePolyDataMapper2 : public vtkPainterPolyDataMapper
{
public:
  static vtkCompositePolyDataMapper2* New();
  vtkTypeMacro(vtkCompositePolyDataMapper2, vtkPainterPolyDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkCompositePolyDataMapper2();
  ~vtkCompositePolyDataMapper2();

  // Description:
  // We need to override this method because the standard streaming
  // demand driven pipeline is not what we want - we are expecting
  // hierarchical data as input
  vtkExecutive* CreateDefaultExecutive();

  // Description:
  // Need to define the type of data handled by this mapper.
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // Description:
  // Need to loop over the hierarchy to compute bounds
  virtual void ComputeBounds();

  // Description:
  // Time stamp for computation of bounds.
  vtkTimeStamp BoundsMTime;

private:
  vtkCompositePolyDataMapper2(const vtkCompositePolyDataMapper2&); // Not implemented.
  void operator=(const vtkCompositePolyDataMapper2&); // Not implemented.
//ETX
};

#endif


