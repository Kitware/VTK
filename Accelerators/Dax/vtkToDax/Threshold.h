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

#ifndef vtkToDax_Threshold_h
#define vtkToDax_Threshold_h

#include "DataSetTypeToType.h"
#include "CellTypeToType.h"
#include "DataSetConverters.h"

#include "daxToVtk/CellTypeToType.h"
#include "daxToVtk/DataSetConverters.h"

#include <dax/cont/Scheduler.h>
#include <dax/cont/GenerateTopology.h>
#include <dax/worklet/Threshold.h>

namespace
{
template <typename T> struct ThresholdOuputType
{
  typedef T type;
};
template <> struct ThresholdOuputType< dax::CellTagVoxel >
{
  typedef dax::CellTagHexahedron type;
};
}

namespace vtkToDax
{
  template<int B>
  struct DoThreshold
  {
    template<class InGridType,
             class OutGridType,
             typename ValueType,
             class Container1,
             class Container2,
             class Adapter>
    int operator()(const InGridType &,
                   OutGridType &,
                   ValueType,
                   ValueType,
                   const dax::cont::ArrayHandle<ValueType,Container1,Adapter> &,
                   dax::cont::ArrayHandle<ValueType,Container2,Adapter> &)
      {
      std::cout << "Not calling DAX, GridType and CellType combination not supported" << std::endl;
      return 0;
      }
  };
  template<>
  struct DoThreshold<1>
  {
    template<class InGridType,
             class OutGridType,
             typename ValueType,
             class Container1,
             class Container2,
             class Adapter>
    int operator()(
        const InGridType &inGrid,
        OutGridType &outGeom,
        ValueType thresholdMin,
        ValueType thresholdMax,
        const dax::cont::ArrayHandle<ValueType,Container1,Adapter> &thresholdHandle,
        dax::cont::ArrayHandle<ValueType,Container2,Adapter> &thresholdResult)
      {
      int result=1;
      try
        {
        //we don't want to use the custom container, so specify the default
        //container for the classification storage.
        typedef dax::cont::GenerateTopology<
                          dax::worklet::ThresholdTopology> ScheduleGT;
        typedef dax::worklet::ThresholdClassify<ValueType> ThresholdClassifyType;

        dax::cont::Scheduler<Adapter> scheduler;

        typedef typename ScheduleGT::ClassifyResultType ClassifyResultType;
        ClassifyResultType classification;

        scheduler.Invoke(ThresholdClassifyType(thresholdMin,thresholdMax),
                 inGrid, thresholdHandle, classification);

        ScheduleGT resolveTopology(classification);
        //remove classification resource from execution for more space
        scheduler.Invoke(resolveTopology,inGrid,outGeom);
        resolveTopology.CompactPointField(thresholdHandle,thresholdResult);
      }
      catch(dax::cont::ErrorControlOutOfMemory error)
        {
        std::cerr << "Ran out of memory trying to use the GPU" << std::endl;
        std::cerr << error.GetMessage() << std::endl;
        result = 0;
        }
      catch(dax::cont::ErrorExecution error)
        {
        std::cerr << "Got ErrorExecution from Dax." << std::endl;
        std::cerr << error.GetMessage() << std::endl;
        result = 0;
        }
      return result;
      }
  };

  template<typename FieldType_>
  struct Threshold
  {
    public:
    typedef FieldType_ FieldType;
    //we expect FieldType_ to be an dax::cont::ArrayHandle
    typedef typename FieldType::ValueType T;

    Threshold(const FieldType& f, T min, T max):
      Result(NULL),
      Field(f),
      Min(min),
      Max(max),
      Name()
      {
      }

    void setOutputGrid(vtkUnstructuredGrid* grid)
      {
      Result=grid;
      }

    void setFieldName(const char* name)
      {
      Name=std::string(name);
      }

    template<typename LHS, typename RHS>
    int operator()(LHS &dataSet, const RHS&) const
      {
      typedef CellTypeToType<RHS> VTKCellTypeStruct;
      typedef DataSetTypeToType<CellTypeToType<RHS>,LHS> DataSetTypeToTypeStruct;

      //get the mapped output type of this operation(threshold)
      //todo make this a typedef on the threshold
      typedef typename ThresholdOuputType< typename VTKCellTypeStruct::DaxCellType >::type OutCellType;

      //get the input dataset type
      typedef typename DataSetTypeToTypeStruct::DaxDataSetType InputDataSetType;

      //construct the output grid type to use the vtk containers
      //as we know we are going back to vtk. In a more general framework
      //we would want a tag to say what the destination container tag types
      //are
      typedef daxToVtk::CellTypeToType<OutCellType> VTKCellType;
      dax::cont::UnstructuredGrid<OutCellType,
                vtkToDax::vtkTopologyContainerTag<VTKCellType>,
                vtkToDax::vtkPointsContainerTag> resultGrid;

      //get from the Field the proper handle type
      FieldType outputHandle;

      InputDataSetType inputDaxData = vtkToDax::dataSetConverter(&dataSet,
                                                                 DataSetTypeToTypeStruct());

      vtkToDax::DoThreshold<DataSetTypeToTypeStruct::Valid> threshold;
      int result = threshold(inputDaxData,
                       resultGrid,
                       this->Min,
                       this->Max,
                       this->Field,
                       outputHandle);

      if(result==1 && resultGrid.GetNumberOfCells() > 0)
        {
        //if we converted correctly, copy the data back to VTK
        //remembering to add back in the output array to the generated
        //unstructured grid
        daxToVtk::addPointData(this->Result,outputHandle,this->Name);
        daxToVtk::dataSetConverter(resultGrid,this->Result);
        }

      return result;

      }
  private:
    vtkUnstructuredGrid* Result;
    FieldType Field;
    T Min;
    T Max;
    std::string Name;

  };
}

#endif //vtkToDax_Threshold_h
