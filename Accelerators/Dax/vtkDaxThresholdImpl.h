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

#ifndef vtkDaxThresholdImpl_h
#define vtkDaxThresholdImpl_h

// Common code
#include "vtkDaxConfig.h"
#include "vtkDaxDetailCommon.h"

#include "vtkDispatcher.h"
#include "vtkDoubleDispatcher.h"
#include "vtkNew.h"

//cell types we support
#include "vtkCellTypes.h"
#include "vtkGenericCell.h"
#include "vtkHexahedron.h"
#include "vtkLine.h"
#include "vtkQuad.h"
#include "vtkTetra.h"
#include "vtkTriangle.h"
#include "vtkVertex.h"
#include "vtkVoxel.h"
#include "vtkWedge.h"

//fields we support
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkUnsignedCharArray.h"

//datasets we support
#include "vtkDataObjectTypes.h"
#include "vtkImageData.h"
#include "vtkStructuredGrid.h"
#include "vtkUniformGrid.h"
#include "vtkUnstructuredGrid.h"

//helpers that convert to and from Dax
#include "vtkToDax/CellTypeToType.h"
#include "vtkToDax/Containers.h"
#include "vtkToDax/DataSetTypeToType.h"
#include "vtkToDax/FieldTypeToType.h"
#include "vtkToDax/Portals.h"
#include "vtkToDax/Threshold.h"


namespace vtkDax{
namespace detail{

  struct ValidThresholdInput
  {
    typedef int ReturnType;
    vtkDataSet* Input;
    vtkCell* Cell;
    double Min;
    double Max;

    vtkUnstructuredGrid* Result;

    ValidThresholdInput(vtkDataSet* in, vtkUnstructuredGrid* out,
                        vtkCell* cell, double lower, double upper):
      Input(in),Cell(cell),Min(lower),Max(upper),Result(out){}

    template<typename LHS>
    int operator()(LHS &arrayField) const
      {
      //we can derive the type of the field at compile time, but not the
      //length
      switch(arrayField.GetNumberOfComponents())
        {
          case 1:
            //first we extract the field type of the array
            //second we extract the number of components
            typedef typename vtkToDax::FieldTypeToType<LHS,1>::DaxValueType VT1;
            return dispatchOnFieldType<LHS,VT1>(arrayField);
          case 2:
            typedef typename vtkToDax::FieldTypeToType<LHS,2>::DaxValueType VT2;
          return dispatchOnFieldType<LHS,VT2>(arrayField);
          case 3:
            typedef typename vtkToDax::FieldTypeToType<LHS,3>::DaxValueType VT3;
            return dispatchOnFieldType<LHS,VT3>(arrayField);
        default:
          //currently only support 1 to 3 components
          //we need to make dispatch on field data smarter in that it does
          //this automagically
          return 0;
        }


      }

    template<typename VTKArrayType, typename DaxValueType>
    int dispatchOnFieldType(VTKArrayType& vtkField) const
      {
      typedef vtkToDax::vtkArrayContainerTag<VTKArrayType> FieldTag;
      typedef dax::cont::ArrayHandle<DaxValueType,FieldTag> FieldHandle;
      typedef typename dax::cont::ArrayHandle<DaxValueType,
                      FieldTag>::PortalConstControl      PortalType;

      FieldHandle field = FieldHandle( PortalType(&vtkField,
                                            vtkField.GetNumberOfTuples() ) );
      vtkToDax::Threshold<FieldHandle> threshold(field,
                                                 DaxValueType(Min),
                                                 DaxValueType(Max));
      threshold.setFieldName(vtkField.GetName());
      threshold.setOutputGrid(this->Result);

      //see if we have a valid data set type
      //if so will perform the threshold if possible
      vtkDoubleDispatcher<vtkDataSet,vtkCell,int> dataDispatcher;
      dataDispatcher.Add<vtkImageData,vtkVoxel>(threshold);
      dataDispatcher.Add<vtkUniformGrid,vtkVoxel>(threshold);

      dataDispatcher.Add<vtkUnstructuredGrid,vtkHexahedron>(threshold);
      dataDispatcher.Add<vtkUnstructuredGrid,vtkLine>(threshold);
      dataDispatcher.Add<vtkUnstructuredGrid,vtkQuad>(threshold);
      dataDispatcher.Add<vtkUnstructuredGrid,vtkTetra>(threshold);
      dataDispatcher.Add<vtkUnstructuredGrid,vtkTriangle>(threshold);
      dataDispatcher.Add<vtkUnstructuredGrid,vtkVertex>(threshold);
      dataDispatcher.Add<vtkUnstructuredGrid,vtkWedge>(threshold);

      int validThreshold = dataDispatcher.Go(this->Input,this->Cell);
      return validThreshold;
      }
  private:
    void operator=(const ValidThresholdInput&);
  };
} //end detail namespace


//------------------------------------------------------------------------------
int Threshold(vtkDataSet* input, vtkUnstructuredGrid *output,
              vtkDataArray* field, double lower, double upper)
{
  //we are doing a point threshold now verify we have suitable cells
  //Dax currently supports: hexs,lines,quads,tets,triangles,vertex,voxel,wedge
  //if something a cell that doesn't match that list we punt to the
  //VTK implementation.
  vtkDax::detail::CellTypeInDataSet cType = vtkDax::detail::cellType(input);

  //construct the object that holds all the state needed to do the threshold
  vtkDax::detail::ValidThresholdInput validInput(input,output,cType.Cell,
                                                 lower,
                                                 upper);


  //setup the dispatch to only allow float and int array to go to the next step
  vtkDispatcher<vtkAbstractArray,int> fieldDispatcher;
  fieldDispatcher.Add<vtkFloatArray>(validInput);
  fieldDispatcher.Add<vtkDoubleArray>(validInput);
  fieldDispatcher.Add<vtkUnsignedCharArray>(validInput);
  fieldDispatcher.Add<vtkIntArray>(validInput);
  return fieldDispatcher.Go(field);
}

} //end vtkDax namespace
// VTK-HeaderTest-Exclude: vtkDaxThresholdImpl.h
#endif
