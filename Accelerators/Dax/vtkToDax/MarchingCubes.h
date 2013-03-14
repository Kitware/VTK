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

#ifndef vtkToDax_MarchingCubes_h
#define vtkToDax_MarchingCubes_h

#include <vtkPolyData.h>

#include "DataSetTypeToType.h"
#include "CellTypeToType.h"
#include "DataSetConverters.h"

#include <daxToVtk/CellTypeToType.h>
#include <daxToVtk/DataSetConverters.h>

#include <dax/cont/Scheduler.h>
#include <dax/cont/ScheduleGenerateTopology.h>
#include <dax/worklet/MarchingCubes.h>

namespace
{
template <typename T> struct MarchingCubesOuputType
{
  typedef dax::CellTagTriangle type;
};

}

namespace vtkToDax
{
  template<int B>
  struct DoMarchingCubes
  {
    template<class InGridType,
             class OutGridType,
             typename ValueType,
             typename TriangleValueType,
             class Container1,
             class Container2,
             class Adapter>
    int operator()(const InGridType &,
                   OutGridType &,
                   ValueType,
                   const dax::cont::ArrayHandle<ValueType,Container1,Adapter> &,
                   dax::cont::ArrayHandle<TriangleValueType,Container2,Adapter> &)
      {
      return 0;
      }
  };
  template<>
  struct DoMarchingCubes<1>
  {
    template<class InGridType,
             class OutGridType,
             typename ValueType,
             typename TriangleValueType,
             class Container1,
             class Container2,
             class Adapter>
    int operator()(
        const InGridType &inGrid,
        OutGridType &outGeom,
        ValueType isoValue,
        const dax::cont::ArrayHandle<ValueType,Container1,Adapter> &mcHandle,
        dax::cont::ArrayHandle<TriangleValueType,Container2,Adapter> &mcResult)
      {
      int result=1;

      dax::Scalar isoValueT(isoValue);

      try
        {
        typedef dax::cont::ScheduleGenerateTopology
          <dax::worklet::MarchingCubesTopology,Adapter> ScheduleGT;
        typedef typename ScheduleGT::ClassifyResultType  ClassifyResultType;

        // construct the scheduler that will execute all the worklets
        dax::cont::Scheduler<Adapter> scheduler;

        // construct the two worklets what will be used to do marching cubes
        dax::worklet::MarchingCubesClassify classifyWorklet(isoValueT);
        dax::worklet::MarchingCubesTopology generateWorklet(isoValueT);

        // run the first step
        ClassifyResultType classification; // array handle for the
                                           // first step
                                           // (classification)
        scheduler.Invoke(classifyWorklet,
                         inGrid,
                         mcHandle,
                         classification);

        // construct the topology generation worklet
        ScheduleGT generate(classification,generateWorklet);
        generate.SetRemoveDuplicatePoints(false);

        // run the second step
        scheduler.Invoke(generate,
                         inGrid,
                         outGeom,
                         mcHandle,
                         inGrid.GetPointCoordinates(),
                         mcResult);
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
  struct MarchingCubes
  {
    public:
    typedef FieldType_ FieldType;
    //we expect FieldType_ to be an dax::cont::ArrayHandle
    typedef typename FieldType::ValueType T;

    MarchingCubes(const FieldType& f, T value):
      Result(NULL),
      Field(f),
      Value(value),
      Name()
      {
      }

    void setOutputGrid(vtkPolyData* grid)
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

      //get the mapped output type of this operation(MarchingCubes)
      //todo make this a typedef on the MarchingCubes
      typedef typename MarchingCubesOuputType< typename VTKCellTypeStruct::DaxCellType >::type OutCellType;

      //get the input dataset type
      typedef typename DataSetTypeToTypeStruct::DaxDataSetType InputDataSetType;

      //construct the output grid type to use the vtk containers
      //as we know we are going back to vtk. In a more general framework
      //we would want a tag to say what the destination container tag types
      //are. We don't need the points container be
      typedef daxToVtk::CellTypeToType<OutCellType> VTKCellType;
      dax::cont::UnstructuredGrid<OutCellType,
                vtkToDax::vtkTopologyContainerTag<VTKCellType> > resultGrid;

      InputDataSetType inputDaxData = vtkToDax::dataSetConverter(&dataSet,
                                                    DataSetTypeToTypeStruct());


      //schedule marching cubes worklet generate step, saving
      //the coordinates into outputHandle. In our case we want the output
      //handle to point to a container whose storage is a vtkPoints class

      typedef dax::Tuple<dax::Vector3,3> TriCoordinatesType;
      dax::cont::ArrayHandle<TriCoordinatesType,
                             vtkToDax::vtkTrianglesContainerTag> outputHandle;

      vtkToDax::DoMarchingCubes<DataSetTypeToTypeStruct::Valid> mc;
      int result = mc(inputDaxData,
                      resultGrid,
                      this->Value,
                      this->Field,
                      outputHandle);
      if(result==1)
        {

        daxToVtk::dataSetConverter(resultGrid,
                                   outputHandle,
                                   this->Result);
        }

      return result;

      }
  private:
    vtkPolyData* Result;
    FieldType Field;
    T Value;
    std::string Name;

  };
}

#endif //vtkToDax_MarchingCubes_h
