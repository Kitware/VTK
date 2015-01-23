//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================

#ifndef vtkDaxThreshold_h
#define vtkDaxThreshold_h

#include "vtkThreshold.h"
#include "vtkAcceleratorsDaxModule.h" //required for correct implementation

class VTKACCELERATORSDAX_EXPORT vtkDaxThreshold : public vtkThreshold
{
public:
  vtkTypeMacro(vtkDaxThreshold,vtkThreshold)
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkDaxThreshold* New();

protected:
  vtkDaxThreshold();
  ~vtkDaxThreshold();

  virtual int RequestData(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *);

private:
  vtkDaxThreshold(const vtkDaxThreshold&); // Not implemented
  void operator=(const vtkDaxThreshold&); // Not implemented
};

#endif // vtkDaxThreshold_h
