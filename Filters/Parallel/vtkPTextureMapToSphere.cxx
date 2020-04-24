/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPTextureMapToSphere.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPTextureMapToSphere.h"

#include "vtkCommunicator.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkTextureMapToSphere.h"

vtkStandardNewMacro(vtkPTextureMapToSphere);

// Create object with Center (0,0,0) and the PreventSeam ivar is set to true. The
// sphere center is automatically computed.
vtkPTextureMapToSphere::vtkPTextureMapToSphere()
{
  this->Controller = vtkMultiProcessController::GetGlobalController();
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
