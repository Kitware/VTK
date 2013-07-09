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

#ifndef __vtkDaxMarchingCubesImpl_h
#define __vtkDaxMarchingCubesImpl_h

// Common code
#include "vtkDaxConfig.h"
#include "vtkDaxDetailCommon.h"

#include "vtkDispatcher.h"
#include "vtkDoubleDispatcher.h"
#include "vtkNew.h"

//fields we support
#include "vtkFloatArray.h"

//cell types we support
#include "vtkCellTypes.h"
#include "vtkGenericCell.h"
#include "vtkTriangle.h"
#include "vtkVoxel.h"

//datasets we support
#include "vtkDataObjectTypes.h"
#include "vtkImageData.h"
#include "vtkUniformGrid.h"
#include "vtkPolyData.h"

//helpers that convert vtk to dax
#include "vtkToDax/Portals.h"
#include "vtkToDax/Containers.h"
#include "vtkToDax/CellTypeToType.h"
#include "vtkToDax/DataSetTypeToType.h"
#include "vtkToDax/FieldTypeToType.h"
#include "vtkToDax/MarchingCubes.h"

namespace vtkDax {
namespace detail {
  struct ValidMarchingCubesInput
  {
    typedef int ReturnType;
    vtkDataSet* Input;
    vtkCell* Cell;
    double IsoValue;

    vtkPolyData* Result;

    ValidMarchingCubesInput(vtkDataSet* in, vtkPolyData* out,
                        vtkCell* cell, double isoValue):
      Input(in),Cell(cell),IsoValue(isoValue),Result(out){}

    template<typename LHS>
    int operator()(LHS &arrayField) const
      {
      //we can derive the type of the field at compile time, but not the
      //length
      if (arrayField.GetNumberOfComponents() == 1)
        {
        //first we extract the field type of the array
        //second we extract the number of components
        typedef typename vtkToDax::FieldTypeToType<LHS,1>::FieldType FT1;
        return dispatchOnFieldType<LHS,FT1>(arrayField);
        }
      return 0;
      }

    template<typename VTKArrayType, typename DaxFieldType>
    int dispatchOnFieldType(VTKArrayType& vtkField) const
      {
      typedef DaxFieldType FieldType;
      typedef vtkToDax::vtkArrayContainerTag<VTKArrayType> FieldTag;
      typedef dax::cont::ArrayHandle<FieldType,FieldTag> FieldHandle;

      typedef typename dax::cont::ArrayHandle
        <FieldType, FieldTag>::PortalConstControl PortalType;

      FieldHandle field = FieldHandle( PortalType(&vtkField,
                                            vtkField.GetNumberOfTuples() ) );
      vtkToDax::MarchingCubes<FieldHandle> marching(field,
                                                 FieldType(IsoValue));
      marching.setFieldName(vtkField.GetName());
      marching.setOutputGrid(this->Result);

      // see if we have a valid data set type if so will perform the
      // marchingcubes if possible
      vtkDoubleDispatcher<vtkDataSet,vtkCell,int> dataDispatcher;
      dataDispatcher.Add<vtkImageData,vtkVoxel>(marching);
      dataDispatcher.Add<vtkUniformGrid,vtkVoxel>(marching);

      int validMC = dataDispatcher.Go(this->Input,this->Cell);
      return validMC;
      }
  private:
    void operator=(const ValidMarchingCubesInput&);
  };
} //namespace detail


//------------------------------------------------------------------------------
int MarchingCubes(vtkDataSet* input, vtkPolyData *output,
              vtkDataArray* field, float isoValue)
{
  //we are doing a point threshold now verify we have suitable cells
  //Dax currently supports: hexs,lines,quads,tets,triangles,vertex,voxel,wedge
  //if something a cell that doesn't match that list we punt to the
  //VTK implementation.
  vtkDax::detail::CellTypeInDataSet cType = vtkDax::detail::cellType(input);

  //construct the object that holds all the state needed to do the MC
  vtkDax::detail::ValidMarchingCubesInput validInput(input,output,cType.Cell,
                                                     isoValue);


  //setup the dispatch to only allow float and int array to go to the next step
  vtkDispatcher<vtkAbstractArray,int> fieldDispatcher;
  fieldDispatcher.Add<vtkFloatArray>(validInput);
  return fieldDispatcher.Go(field);
}

} //end vtkDax namespace
// VTK-HeaderTest-Exclude: vtkDaxMarchingCubesImpl.h
#endif