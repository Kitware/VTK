//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2019 Sandia Corporation.
//  Copyright 2019 UT-Battelle, LLC.
//  Copyright 2019 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#define vtkmDataArray_cxx

#include "vtkmDataArray.h"

template class VTKACCELERATORSVTKM_EXPORT vtkmDataArray<char>;
template class VTKACCELERATORSVTKM_EXPORT vtkmDataArray<double>;
template class VTKACCELERATORSVTKM_EXPORT vtkmDataArray<float>;
template class VTKACCELERATORSVTKM_EXPORT vtkmDataArray<int>;
template class VTKACCELERATORSVTKM_EXPORT vtkmDataArray<long>;
template class VTKACCELERATORSVTKM_EXPORT vtkmDataArray<long long>;
template class VTKACCELERATORSVTKM_EXPORT vtkmDataArray<short>;
template class VTKACCELERATORSVTKM_EXPORT vtkmDataArray<signed char>;
template class VTKACCELERATORSVTKM_EXPORT vtkmDataArray<unsigned char>;
template class VTKACCELERATORSVTKM_EXPORT vtkmDataArray<unsigned int>;
template class VTKACCELERATORSVTKM_EXPORT vtkmDataArray<unsigned long>;
template class VTKACCELERATORSVTKM_EXPORT vtkmDataArray<unsigned long long>;
template class VTKACCELERATORSVTKM_EXPORT vtkmDataArray<unsigned short>;
