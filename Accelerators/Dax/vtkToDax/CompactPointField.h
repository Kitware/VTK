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
//  Copyright 2014 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================

#ifndef vtkToDax_CompactPointField_h
#define vtkToDax_CompactPointField_h

#include "vtkDataSet.h"
#include "vtkDispatcher.h"

#include "vtkToDax/Containers.h"
#include "vtkToDax/FieldTypeToType.h"

#include "daxToVtk/DataSetConverters.h"

namespace vtkToDax {

template<typename DispatcherType>
struct CompactPointField
{
  DispatcherType *Dispatcher;
  vtkDataSet *Output;

  CompactPointField(DispatcherType &dispatcher,
                    vtkDataSet *output)
    : Dispatcher(&dispatcher), Output(output) {  }

  template<typename InputVTKArrayType>
  int operator()(InputVTKArrayType &inputFieldVTKArray)
  {
    switch(inputFieldVTKArray.GetNumberOfComponents())
      {
      case 1: return this->DoCompact<1>(inputFieldVTKArray);
      case 2: return this->DoCompact<2>(inputFieldVTKArray);
      case 3: return this->DoCompact<3>(inputFieldVTKArray);
      case 4: return this->DoCompact<4>(inputFieldVTKArray);
      default:
        vtkGenericWarningMacro(
              << "Cannot compact point array " << inputFieldVTKArray.GetName()
              << " with " << inputFieldVTKArray.GetNumberOfComponents()
              << " components.");
        return 0;
      }
  }

  template<int NumComponents, typename InputVTKArrayType>
  int DoCompact(InputVTKArrayType &inputFieldVTKArray)
  {
    typedef vtkToDax::vtkArrayContainerTag<InputVTKArrayType> ContainerTag;
    typedef typename vtkToDax::FieldTypeToType<InputVTKArrayType,NumComponents>
        ::DaxValueType DaxValueType;
    typedef dax::cont::ArrayHandle<DaxValueType, ContainerTag>
        FieldHandleType;
    typedef typename FieldHandleType::PortalConstControl PortalType;

    if (inputFieldVTKArray.GetNumberOfComponents() != NumComponents)
      {
      // Generally this method is only called from operator(), and that
      // method is supposed to check that this does not happen.
      vtkGenericWarningMacro(
            << "vtkToDax::CompactPointField in unexpected function call.");
      return 0;
      }

    FieldHandleType daxInputField =
        FieldHandleType(PortalType(&inputFieldVTKArray,
                                   inputFieldVTKArray.GetNumberOfTuples()));

    FieldHandleType daxOutputField;

    this->Dispatcher->CompactPointField(daxInputField, daxOutputField);

    daxToVtk::addPointData(this->Output,
                           daxOutputField,
                           inputFieldVTKArray.GetName());

    return 1;
  }
};

} // namespace vtkToDax

#endif //vtkToDax_CompactPointField_h
