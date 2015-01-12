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

#ifndef vtkDaxContourImpl_h
#define vtkDaxContourImpl_h

// Common code
#include "vtkDaxConfig.h"
#include "vtkDaxDetailCommon.h"

#include "vtkDispatcher.h"
#include "vtkDoubleDispatcher.h"
#include "vtkNew.h"

//fields we support
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"

//cell types we support
#include "vtkCellTypes.h"
#include "vtkGenericCell.h"
#include "vtkHexahedron.h"
#include "vtkTetra.h"
#include "vtkTriangle.h"
#include "vtkVoxel.h"

//datasets we support
#include "vtkDataObjectTypes.h"
#include "vtkImageData.h"
#include "vtkUniformGrid.h"
#include "vtkPolyData.h"

//helpers that convert vtk to dax
#include "vtkToDax/CellTypeToType.h"
#include "vtkToDax/Containers.h"
#include "vtkToDax/Contour.h"
#include "vtkToDax/DataSetTypeToType.h"
#include "vtkToDax/FieldTypeToType.h"
#include "vtkToDax/Portals.h"

namespace vtkDax {
namespace detail {
  struct ValidContourInput
  {
    typedef int ReturnType;
    vtkDataSet* Input;
    vtkCell* Cell;
    double IsoValue;
    bool ComputeScalars;

    vtkPolyData* Result;

    ValidContourInput(vtkDataSet* in, vtkPolyData* out,
                      vtkCell* cell, double isoValue,
                      bool computeScalars) :
      Input(in),
      Cell(cell),
      IsoValue(isoValue),
      ComputeScalars(computeScalars),
      Result(out) {  }

    template<typename LHS>
    int operator()(LHS &arrayField) const
      {
      //we can derive the type of the field at compile time, but not the
      //length
      if (arrayField.GetNumberOfComponents() == 1)
        {
        //first we extract the field type of the array
        //second we extract the number of components
        typedef typename vtkToDax::FieldTypeToType<LHS,1>::DaxValueType VT1;
        return dispatchOnFieldType<LHS,VT1>(arrayField);
        }
      return 0;
      }

    template<typename VTKArrayType, typename DaxValueType>
    int dispatchOnFieldType(VTKArrayType& vtkField) const
      {
      typedef vtkToDax::vtkArrayContainerTag<VTKArrayType> FieldTag;
      typedef dax::cont::ArrayHandle<DaxValueType,FieldTag> FieldHandle;

      typedef typename dax::cont::ArrayHandle
        <DaxValueType, FieldTag>::PortalConstControl PortalType;

      FieldHandle field = FieldHandle( PortalType(&vtkField,
                                            vtkField.GetNumberOfTuples() ) );
      vtkToDax::Contour<FieldHandle> contour(field,
                                             DaxValueType(this->IsoValue),
                                             this->ComputeScalars);
      contour.setFieldName(vtkField.GetName());
      contour.setOutputGrid(this->Result);

      // see if we have a valid data set type if so will perform the
      // marchingcubes if possible
      vtkDoubleDispatcher<vtkDataSet,vtkCell,int> dataDispatcher;
      dataDispatcher.Add<vtkImageData,vtkVoxel>(contour);
      dataDispatcher.Add<vtkUniformGrid,vtkVoxel>(contour);
      dataDispatcher.Add<vtkUnstructuredGrid,vtkHexahedron>(contour);
      dataDispatcher.Add<vtkUnstructuredGrid,vtkTetra>(contour);

      int validMC = dataDispatcher.Go(this->Input,this->Cell);
      return validMC;
      }
  private:
    void operator=(const ValidContourInput&);
  };
} //namespace detail


//------------------------------------------------------------------------------
int Contour(vtkDataSet* input, vtkPolyData *output,
            vtkDataArray* field, float isoValue, bool computeScalars)
{
  //we are doing a point threshold now verify we have suitable cells
  //Dax currently supports: hexs,lines,quads,tets,triangles,vertex,voxel,wedge
  //if something a cell that doesn't match that list we punt to the
  //VTK implementation.
  vtkDax::detail::CellTypeInDataSet cType = vtkDax::detail::cellType(input);

  //construct the object that holds all the state needed to do the MC
  vtkDax::detail::ValidContourInput validInput(input,output,cType.Cell,
                                               isoValue, computeScalars);


  //setup the dispatch to only allow float and int array to go to the next step
  vtkDispatcher<vtkAbstractArray,int> fieldDispatcher;
  fieldDispatcher.Add<vtkFloatArray>(validInput);
  fieldDispatcher.Add<vtkDoubleArray>(validInput);
  return fieldDispatcher.Go(field);
}

} //end vtkDax namespace
// VTK-HeaderTest-Exclude: vtkDaxContourImpl.h
#endif
