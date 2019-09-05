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

#ifndef vtkmTags_h
#define vtkmTags_h

#include "vtkCellArray.h"
#include "vtkPoints.h"
#include "vtkAOSDataArrayTemplate.h"
#include "vtkSOADataArrayTemplate.h"

#include "vtkmConfig.h" //required for general vtkm setup

namespace tovtkm {

//this tag is used to construct points coordinates
struct VTKM_ALWAYS_EXPORT vtkCellArrayContainerTag
{
};

}

#endif
// VTK-HeaderTest-Exclude: vtkmTags.h
