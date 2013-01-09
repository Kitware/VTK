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

#ifndef VTKDAXTHRESHOLD_H
#define VTKDAXTHRESHOLD_H

#include <vtkThreshold.h>
#include "vtkAcceleratorsDaxModule.h"

class VTKACCELERATORSDAX_EXPORT vtkDaxThreshold : public vtkThreshold
{
public:
  vtkTypeMacro(vtkDaxThreshold,vtkThreshold)
  static vtkDaxThreshold* New();

protected:
  vtkDaxThreshold();
  ~vtkDaxThreshold();

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
  vtkDaxThreshold(const vtkDaxThreshold&);
  void operator=(const vtkDaxThreshold&);
};

#endif // VTKDAXTHRESHOLD_H
