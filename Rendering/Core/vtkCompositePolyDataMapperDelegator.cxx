/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositePolyDataMapperDelegator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCompositePolyDataMapperDelegator.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkAbstractObjectFactoryNewMacro(vtkCompositePolyDataMapperDelegator);

//------------------------------------------------------------------------------
vtkCompositePolyDataMapperDelegator::vtkCompositePolyDataMapperDelegator() = default;

//------------------------------------------------------------------------------
vtkCompositePolyDataMapperDelegator::~vtkCompositePolyDataMapperDelegator() = default;

//------------------------------------------------------------------------------
void vtkCompositePolyDataMapperDelegator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Delegate: " << this->Delegate << "\n";
  if (this->Delegate)
  {
    this->Delegate->PrintSelf(os, indent.GetNextIndent());
  }
}

//-----------------------------------------------------------------------------
void vtkCompositePolyDataMapperDelegator::ShallowCopy(vtkCompositePolyDataMapper* polydataMapper)
{
  if (this->Delegate)
  {
    // bypass vtkPolyDataMapper::ShallowCopy because it copies input connection.
    this->Delegate->vtkMapper::ShallowCopy(polydataMapper);
    this->Delegate->SetSelection(polydataMapper->GetSelection());
    // remaining properties related to vtkPolyDataMapper
    this->Delegate->SetVBOShiftScaleMethod(polydataMapper->GetVBOShiftScaleMethod());
    this->Delegate->SetSeamlessU(polydataMapper->GetSeamlessU());
    this->Delegate->SetSeamlessV(polydataMapper->GetSeamlessV());
  }
  else
  {
    vtkErrorMacro(<< "Delegate is not initialized! \n"
                  << "Possible cause: An object factory override for the abstract class "
                  << "vtkCompositePolyDataMapperDelegator was not correctly implemented.");
    std::terminate();
  }
}

VTK_ABI_NAMESPACE_END
