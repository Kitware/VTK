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

#ifndef vtkToDax_Contour_h
#define vtkToDax_Contour_h

#include "vtkDispatcher.h"
#include "vtkPolyData.h"

#include "vtkToDax/CellTypeToType.h"
#include "vtkToDax/CompactPointField.h"
#include "vtkToDax/DataSetTypeToType.h"
#include "vtkToDax/DataSetConverters.h"

#include "daxToVtk/CellTypeToType.h"
#include "daxToVtk/DataSetConverters.h"

#include <dax/cont/DispatcherGenerateInterpolatedCells.h>
#include <dax/cont/DispatcherMapCell.h>
#include <dax/worklet/MarchingCubes.h>
#include <dax/worklet/MarchingTetrahedra.h>

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
struct DoContour
{
  template<class InGridType,
           class OutGridType,
           typename ValueType,
           class Container1,
           class Adapter>
  int operator()(const InGridType &,
                 vtkDataSet *,
                 OutGridType &,
                 vtkPolyData *,
                 ValueType,
                 const dax::cont::ArrayHandle<ValueType,Container1,Adapter> &,
                 bool)
  {
    vtkGenericWarningMacro(
          << "Not calling Dax, GridType-CellType combination not supported");
    return 0;
  }
};
template<>
struct DoContour<1>
{
  template<class InGridType,
           class OutGridType,
           typename ValueType,
           class Container1,
           class Adapter>
  int operator()(
      const InGridType &inDaxGrid,
      vtkDataSet *inVTKGrid,
      OutGridType &outDaxGeom,
      vtkPolyData *outVTKGrid,
      ValueType isoValue,
      const dax::cont::ArrayHandle<ValueType,Container1,Adapter> &mcHandle,
      bool computeScalars)
  {
    dax::Scalar isoValueT(isoValue);

    dax::worklet::MarchingCubesCount countWorklet(isoValueT);
    dax::worklet::MarchingCubesGenerate generateWorklet(isoValueT);

    return this->DispatchWork(inDaxGrid,
                              inVTKGrid,
                              outDaxGeom,
                              outVTKGrid,
                              countWorklet,
                              generateWorklet,
                              mcHandle,
                              computeScalars);
  }

  template<class GridCellContainer,
           class GridPointContainer,
           class OutGridType,
           typename ValueType,
           class Container1,
           class Adapter>
  int operator()(
      const dax::cont::UnstructuredGrid<
        dax::CellTagTetrahedron,GridCellContainer,GridPointContainer,Adapter>
        &inDaxGrid,
      vtkDataSet *inVTKGrid,
      OutGridType &outDaxGeom,
      vtkPolyData *outVTKGrid,
      ValueType isoValue,
      const dax::cont::ArrayHandle<ValueType,Container1,Adapter> &mcHandle,
      bool computeScalars)
  {
    dax::Scalar isoValueT(isoValue);

    dax::worklet::MarchingTetrahedraCount countWorklet(isoValueT);
    dax::worklet::MarchingTetrahedraGenerate generateWorklet(isoValueT);

    return this->DispatchWork(inDaxGrid,
                              inVTKGrid,
                              outDaxGeom,
                              outVTKGrid,
                              countWorklet,
                              generateWorklet,
                              mcHandle,
                              computeScalars);
  }

  template<class InGridType,
           class OutGridType,
           typename ValueType,
           class Container1,
           class Adapter,
           class CountWorkletType,
           class GenerateWorkletType>
  int DispatchWork(
      const InGridType &inDaxGrid,
      vtkDataSet *inVTKGrid,
      OutGridType &outDaxGeom,
      vtkPolyData *outVTKGrid,
      CountWorkletType &countWorklet,
      GenerateWorkletType &generateWorklet,
      const dax::cont::ArrayHandle<ValueType,Container1,Adapter> &mcHandle,
      bool computeScalars)
  {
    int result=1;

    try
      {

      typedef dax::cont::DispatcherGenerateInterpolatedCells<
                  GenerateWorkletType,
                  dax::cont::ArrayHandle< dax::Id >,
                  Adapter >                             DispatchIC;

      typedef typename DispatchIC::CountHandleType CountHandleType;

      dax::cont::DispatcherMapCell<CountWorkletType,Adapter>
          dispatchCount( countWorklet );

      CountHandleType count;
      dispatchCount.Invoke(inDaxGrid, mcHandle, count);


      DispatchIC generateSurface(count, generateWorklet);
      generateSurface.SetRemoveDuplicatePoints(true);
      generateSurface.Invoke(inDaxGrid,outDaxGeom,mcHandle);

      // Convert output geometry to VTK.
      daxToVtk::dataSetConverter(outDaxGeom, outVTKGrid);

      // Interpolate arrays where possible.
      if (computeScalars)
        {
        vtkToDax::CompactPointField<DispatchIC> compact(generateSurface,
                                                        outVTKGrid);
        vtkDispatcher<vtkAbstractArray,int> compactDispatcher;
        compactDispatcher.Add<vtkFloatArray>(compact);
        compactDispatcher.Add<vtkDoubleArray>(compact);

        vtkPointData *pd = inVTKGrid->GetPointData();
        for (int arrayIndex = 0;
             arrayIndex < pd->GetNumberOfArrays();
             arrayIndex++)
          {
          vtkDataArray *array = pd->GetArray(arrayIndex);
          if (array == NULL) { continue; }

          compactDispatcher.Go(array);
          }

        // Pass information about attributes.
        for (int attributeType = 0;
             attributeType < vtkDataSetAttributes::NUM_ATTRIBUTES;
             attributeType++)
          {
          vtkDataArray *attribute = pd->GetAttribute(attributeType);
          if (attribute == NULL) { continue; }
          outVTKGrid->GetPointData()->SetActiveAttribute(attribute->GetName(),
                                                         attributeType);
          }
        } //computeScalars
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
  struct Contour
  {
    public:
    typedef FieldType_ FieldType;
    //we expect FieldType_ to be an dax::cont::ArrayHandle
    typedef typename FieldType::ValueType T;

    Contour(const FieldType& f, T value, bool computeScalars):
      Result(NULL),
      Field(f),
      Value(value),
      ComputeScalars(computeScalars),
      Name()
      {
      }

    void setOutputGrid(vtkPolyData* grid)
      {
      this->Result=grid;
      }

    void setFieldName(const char* name)
      {
      this->Name=std::string(name);
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
                 vtkToDax::vtkTopologyContainerTag<VTKCellType>,
                 vtkToDax::vtkPointsContainerTag > resultGrid;

      InputDataSetType inputDaxData = vtkToDax::dataSetConverter(&dataSet,
                                                     DataSetTypeToTypeStruct());

      vtkToDax::DoContour<DataSetTypeToTypeStruct::Valid> mc;
      int result = mc(inputDaxData,
                      &dataSet,
                      resultGrid,
                      this->Result,
                      this->Value,
                      this->Field,
                      this->ComputeScalars);

      return result;
      }
  private:
    vtkPolyData* Result;
    FieldType Field;
    T Value;
    bool ComputeScalars;
    std::string Name;

  };
}

#endif //vtkToDax_Contour_h
