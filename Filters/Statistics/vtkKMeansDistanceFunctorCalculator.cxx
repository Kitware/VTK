// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkKMeansDistanceFunctorCalculator.h"

#include "vtkDoubleArray.h"
#include "vtkFunctionParser.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

#include <iostream>
#include <sstream>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkKMeansDistanceFunctorCalculator);
vtkCxxSetObjectMacro(vtkKMeansDistanceFunctorCalculator, FunctionParser, vtkFunctionParser);

//------------------------------------------------------------------------------
vtkKMeansDistanceFunctorCalculator::vtkKMeansDistanceFunctorCalculator()
{
  this->FunctionParser = vtkFunctionParser::New();
  this->DistanceExpression = nullptr;
  this->TupleSize = -1;
}

//------------------------------------------------------------------------------
vtkKMeansDistanceFunctorCalculator::~vtkKMeansDistanceFunctorCalculator()
{
  this->SetFunctionParser(nullptr);
  this->SetDistanceExpression(nullptr);
}

//------------------------------------------------------------------------------
void vtkKMeansDistanceFunctorCalculator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FunctionParser: " << this->FunctionParser << "\n";
  os << indent << "DistanceExpression: "
     << (this->DistanceExpression && this->DistanceExpression[0] ? this->DistanceExpression
                                                                 : "nullptr")
     << "\n";
  os << indent << "TupleSize: " << this->TupleSize << "\n";
}

//------------------------------------------------------------------------------
void vtkKMeansDistanceFunctorCalculator::operator()(
  double& distance, vtkVariantArray* clusterCoord, vtkVariantArray* dataCoord)
{
  distance = 0.0;
  vtkIdType nv = clusterCoord->GetNumberOfValues();
  if (nv != dataCoord->GetNumberOfValues())
  {
    std::cout << "The dimensions of the cluster and data do not match.\n";
    distance = -1;
    return;
  }

  if (!this->DistanceExpression)
  {
    distance = -1;
    return;
  }

  this->FunctionParser->SetFunction(this->DistanceExpression);
  if (this->TupleSize != nv)
  { // Need to update the scalar variable names as well as values...
    this->FunctionParser->RemoveScalarVariables();
    for (vtkIdType i = 0; i < nv; ++i)
    {
      std::ostringstream xos;
      std::ostringstream yos;
      xos << "x" << i;
      yos << "y" << i;
      this->FunctionParser->SetScalarVariableValue(
        xos.str().c_str(), clusterCoord->GetValue(i).ToDouble());
      this->FunctionParser->SetScalarVariableValue(
        yos.str().c_str(), dataCoord->GetValue(i).ToDouble());
    }
  }
  else
  { // Use faster integer comparisons to set values...
    for (vtkIdType i = 0; i < nv; ++i)
    {
      this->FunctionParser->SetScalarVariableValue(2 * i, clusterCoord->GetValue(i).ToDouble());
      this->FunctionParser->SetScalarVariableValue(2 * i + 1, dataCoord->GetValue(i).ToDouble());
    }
  }
  distance = this->FunctionParser->GetScalarResult();
  /*
  std::cout << "f([";
  for ( vtkIdType i = 0; i < nv; ++ i )
    std::cout << " " << dataCoord->GetValue( i ).ToDouble();
  std::cout << " ],[";
  for ( vtkIdType i = 0; i < nv; ++ i )
    std::cout << " " << clusterCoord->GetValue( i ).ToDouble();
  std::cout << " ]) = " << distance << "\n";
  */
}
VTK_ABI_NAMESPACE_END
