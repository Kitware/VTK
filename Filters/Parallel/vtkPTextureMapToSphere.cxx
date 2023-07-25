// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPTextureMapToSphere.h"

#include "vtkCommunicator.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkTextureMapToSphere.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPTextureMapToSphere);
vtkCxxSetObjectMacro(vtkPTextureMapToSphere, Controller, vtkMultiProcessController);

// Create object with Center (0,0,0) and the PreventSeam ivar is set to true. The
// sphere center is automatically computed.
vtkPTextureMapToSphere::vtkPTextureMapToSphere()
{
  this->Controller = nullptr;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

vtkPTextureMapToSphere::~vtkPTextureMapToSphere()
{
  this->SetController(nullptr);
}

void vtkPTextureMapToSphere::ComputeCenter(vtkDataSet* dataSet)
{
  if (this->AutomaticSphereGeneration && this->Controller->GetNumberOfProcesses() > 1)
  {
    vtkIdType numberOfPoints = dataSet->GetNumberOfPoints();
    double out[4] = { static_cast<double>(numberOfPoints), 0.0, 0.0, 0.0 }, x[3], in[4];

    for (vtkIdType id = 0; id < numberOfPoints; ++id)
    {
      dataSet->GetPoint(id, x);
      out[1] += x[0];
      out[2] += x[1];
      out[3] += x[2];
    }

    this->Controller->AllReduce(out, in, 4, vtkCommunicator::SUM_OP);

    if (!in[0])
    {
      vtkErrorMacro(<< "No points");
    }

    this->Center[0] = in[1] / in[0];
    this->Center[1] = in[2] / in[0];
    this->Center[2] = in[3] / in[0];
  }
  else
  {
    this->Superclass::ComputeCenter(dataSet);
  }
}

void vtkPTextureMapToSphere::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->Controller)
  {
    os << indent << "Controller:\n";
    this->Controller->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Controller: (none)" << endl;
  }
}
VTK_ABI_NAMESPACE_END
