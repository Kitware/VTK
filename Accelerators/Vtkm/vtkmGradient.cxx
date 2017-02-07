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
#include "vtkmGradient.h"

#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

#include "vtkmlib/ArrayConverters.h"
#include "vtkmlib/DataSetConverters.h"
#include "vtkmlib/PolyDataConverter.h"
#include "vtkmlib/Storage.h"

#include "vtkmCellSetExplicit.h"
#include "vtkmCellSetSingleType.h"
#include "vtkmFilterPolicy.h"

#include <vtkm/filter/Gradient.h>
#include <vtkm/filter/PointAverage.h>

vtkStandardNewMacro(vtkmGradient)

namespace
{
struct GradientOutTypes
    : vtkm::ListTagBase<
                        vtkm::Vec<vtkm::Float32,3>,
                        vtkm::Vec<vtkm::Float64,3>,
                        vtkm::Vec< vtkm::Vec<vtkm::Float32,3>, 3>,
                        vtkm::Vec< vtkm::Vec<vtkm::Float64,3>, 3>
                        >
{
};

//------------------------------------------------------------------------------
class vtkmGradientOutputFilterPolicy
      : public vtkm::filter::PolicyBase<vtkmGradientOutputFilterPolicy>
  {
  public:
    typedef GradientOutTypes FieldTypeList;
    typedef tovtkm::TypeListTagVTMOut FieldStorageList;

    typedef tovtkm::CellListStructuredOutVTK StructuredCellSetList;
    typedef tovtkm::CellListUnstructuredOutVTK UnstructuredCellSetList;
    typedef tovtkm::CellListAllOutVTK AllCellSetList;

    typedef vtkm::TypeListTagFieldVec3 CoordinateTypeList;
    typedef tovtkm::PointListOutVTK CoordinateStorageList;

    typedef vtkm::filter::PolicyDefault::DeviceAdapterList DeviceAdapterList;
  };
}

//------------------------------------------------------------------------------
vtkmGradient::vtkmGradient()
{
}

//------------------------------------------------------------------------------
vtkmGradient::~vtkmGradient()
{
}

//------------------------------------------------------------------------------
void vtkmGradient::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int vtkmGradient::RequestData(vtkInformation* request,
                                 vtkInformationVector** inputVector,
                                 vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataSet* input =
      vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataSet* output =
      vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  output->ShallowCopy(input);

  // grab the input array to process to determine the field want to compute
  // the gradient for
  int association = this->GetInputArrayAssociation(0, inputVector);
  if(association != vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    vtkWarningMacro(<< "VTK-m Gradient currently only support point based fields.\n"
                    << "Falling back to vtkGradientFilter."
                    );
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }

  // convert the input dataset to a vtkm::cont::DataSet
  vtkm::cont::DataSet in = tovtkm::Convert(input);
  // convert the array over to vtkm
  vtkDataArray* inputArray = this->GetInputArrayToProcess(0, inputVector);
  vtkm::cont::Field field = tovtkm::Convert(inputArray, association);

  const bool dataSetValid =
      in.GetNumberOfCoordinateSystems() > 0 && in.GetNumberOfCellSets() > 0;
  const bool fieldValid =
      (field.GetAssociation() == vtkm::cont::Field::ASSOC_POINTS) &&
      (field.GetName() != std::string());

  const bool fieldIsVec = (inputArray->GetNumberOfComponents() == 3);
  const bool fieldIsScalar = inputArray->GetDataType() == VTK_FLOAT ||
                             inputArray->GetDataType() == VTK_DOUBLE;

  if(!(dataSetValid && fieldValid) || !fieldIsScalar)
  {
    vtkWarningMacro(<< "Unable convert dataset over to VTK-m.\n"
                    << "Falling back to vtkGradientFilter."
                    );
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }

  vtkmInputFilterPolicy policy;
  vtkm::filter::Gradient filter;
  filter.SetComputePointGradient( !this->FasterApproximation );


  if( fieldIsVec )
    { //this properties are only valid when processing a vec<3> field
    filter.SetComputeDivergence( this->ComputeDivergence != 0 );
    filter.SetComputeVorticity( this->ComputeVorticity != 0 );
    filter.SetComputeQCriterion( this->ComputeQCriterion != 0 );
    }

  if(this->ResultArrayName)
  {
    filter.SetOutputFieldName( this->ResultArrayName );
  }

  if(this->DivergenceArrayName)
  {
    filter.SetDivergenceName( this->DivergenceArrayName );
  }

  if(this->VorticityArrayName)
  {
    filter.SetVorticityName( this->VorticityArrayName );
  }

  if(this->QCriterionArrayName)
  {
    filter.SetQCriterionName( this->QCriterionArrayName );
  }
  else
  {
    filter.SetQCriterionName( "Q-criterion" );
  }

  vtkm::filter::ResultField result = filter.Execute(in, field, policy);
  if(!result.IsValid())
  {
    vtkWarningMacro(<< "VTK-m gradient computation failed for an unknown reason.\n"
                    << "Falling back to vtkGradientFilter."
                    );
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }

  vtkm::cont::DataSet const& resultData = result.GetDataSet();
  vtkDataArray* gradientArray = nullptr;
  vtkDataArray* divergenceArray = nullptr;
  vtkDataArray* vorticityArray = nullptr;
  vtkDataArray* qcriterionArray = nullptr;

  if(this->FasterApproximation)
  {
    //We need to convert this field back to a point field
    //Which means converting cell data back to point data
    vtkmGradientOutputFilterPolicy averagepolicy;
    vtkm::filter::PointAverage cellToPoint;
    cellToPoint.SetOutputFieldName(filter.GetOutputFieldName());
    vtkm::filter::ResultField toPointResult = cellToPoint.Execute(in,
                                                                  result.GetField(),
                                                                  averagepolicy);
    gradientArray = fromvtkm::Convert(toPointResult.GetField());

    if(this->ComputeDivergence && fieldIsVec)
      {
      vtkm::filter::ResultField dresult =
        cellToPoint.Execute(in, resultData.GetField(filter.GetDivergenceName()));
      divergenceArray = fromvtkm::Convert(dresult.GetField());
      }

    if(this->ComputeVorticity  && fieldIsVec)
      {
      vtkm::filter::ResultField vresult =
        cellToPoint.Execute(in, resultData.GetField(filter.GetVorticityName()));
      vorticityArray = fromvtkm::Convert(vresult.GetField());
      }

    if(this->ComputeQCriterion && fieldIsVec)
      {
      vtkm::filter::ResultField qresult =
        cellToPoint.Execute(in,resultData.GetField(filter.GetQCriterionName()));
      qcriterionArray = fromvtkm::Convert(qresult.GetField());
      }
  }
  else
  {
    gradientArray = fromvtkm::Convert(result.GetField());

    if(this->ComputeDivergence && fieldIsVec)
      {
      divergenceArray = fromvtkm::Convert(resultData.GetField(filter.GetDivergenceName()));
      }
    if(this->ComputeVorticity && fieldIsVec)
      {
      vorticityArray = fromvtkm::Convert(resultData.GetField(filter.GetVorticityName()));
      }
    if(this->ComputeQCriterion && fieldIsVec)
      {
      qcriterionArray = fromvtkm::Convert(resultData.GetField(filter.GetQCriterionName()));
      }
  }

  if(this->GetComputeGradient() && gradientArray)
  {
    output->GetPointData()->AddArray(gradientArray);
    gradientArray->FastDelete();
  }
  else if(gradientArray)
  { //gradient is the only array we unconditional convert so we have to handle
    //the use case the user doesn't want it on the output data
    gradientArray->Delete();
  }

  if(this->ComputeDivergence && divergenceArray)
  {
    output->GetPointData()->AddArray(divergenceArray);
    divergenceArray->FastDelete();
  }

  if(this->ComputeVorticity && vorticityArray)
  {
    output->GetPointData()->AddArray(vorticityArray);
    vorticityArray->FastDelete();
  }

  if(this->ComputeQCriterion && qcriterionArray)
  {
    output->GetPointData()->AddArray(qcriterionArray);
    qcriterionArray->FastDelete();
  }

  return 1;
}
