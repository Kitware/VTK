/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProgrammableElectronicData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkProgrammableElectronicData.h"

#include "vtkDataSetCollection.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"

#include <vector>

// PIMPL'd std::vector
class StdVectorOfImageDataPointers :
    public std::vector<vtkSmartPointer<vtkImageData> > {};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkProgrammableElectronicData);
vtkCxxSetObjectMacro(vtkProgrammableElectronicData, ElectronDensity, vtkImageData);

//----------------------------------------------------------------------------
vtkProgrammableElectronicData::vtkProgrammableElectronicData()
  : NumberOfElectrons(0), MOs(new StdVectorOfImageDataPointers),
    ElectronDensity(NULL)
{
}

//----------------------------------------------------------------------------
vtkProgrammableElectronicData::~vtkProgrammableElectronicData()
{
  delete this->MOs;
  this->MOs = NULL;

  this->SetElectronDensity(NULL);
}

//----------------------------------------------------------------------------
void vtkProgrammableElectronicData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "NumberOfElectrons: " << this->NumberOfElectrons << "\n";

  os << indent << "MOs: (std::vector<vtkImageData*>) @" << this->MOs << "\n";
  os << indent.GetNextIndent() << "size: " << this->MOs->size() << "\n";
  for (size_t i = 0; i < this->MOs->size(); ++i)
    {
    vtkImageData *current = this->MOs->at(i).GetPointer();
    os << indent.GetNextIndent() << "MO #" << i+1 << " @" << current << "\n";
    if (current)
      current->PrintSelf(os, indent.GetNextIndent().GetNextIndent());
    }

  os << indent << "ElectronDensity: @" << this->ElectronDensity << "\n";
  if (this->ElectronDensity)
    this->ElectronDensity->PrintSelf(os,
                                     indent.GetNextIndent().GetNextIndent());

  os << indent << "Padding: " << this->Padding << "\n";
}

//----------------------------------------------------------------------------
vtkIdType vtkProgrammableElectronicData::GetNumberOfMOs()
{
  return static_cast<vtkIdType>(this->MOs->size());
}

//----------------------------------------------------------------------------
void vtkProgrammableElectronicData::SetNumberOfMOs(vtkIdType size)
{
  if (size == static_cast<vtkIdType>(this->MOs->size()))
    {
    return;
    }

  vtkDebugMacro(<<"Resizing MO vector from " << this->MOs->size() << " to "
                << size << ".");
  this->MOs->resize(size);

  this->Modified();

  return;
}

//----------------------------------------------------------------------------
vtkImageData * vtkProgrammableElectronicData::GetMO(vtkIdType orbitalNumber)
{
  if (orbitalNumber <= 0)
    {
    vtkWarningMacro(<< "Request for invalid orbital number "<<orbitalNumber);
    return NULL;
    }
  if (orbitalNumber > static_cast<vtkIdType>(this->MOs->size()))
    {
    vtkWarningMacro(<< "Request for orbital number " << orbitalNumber
                    << ", which exceeds the number of MOs ("
                    << this->MOs->size() << ")");
    return NULL;
    }

  vtkImageData *result = this->MOs->at(orbitalNumber - 1).GetPointer();

  vtkDebugMacro(<< "Returning '" << result << "' for MO '"
                << orbitalNumber << "'");
  return result;
}

//----------------------------------------------------------------------------
void vtkProgrammableElectronicData::SetMO(vtkIdType orbitalNumber,
                                          vtkImageData *data)
{
  if (orbitalNumber <= 0)
    {
    vtkErrorMacro("Cannot set invalid orbital number "<<orbitalNumber);
    return;
    }
  if (orbitalNumber > static_cast<vtkIdType>(this->MOs->size()))
    {
    this->SetNumberOfMOs(orbitalNumber);
    }

  vtkImageData *previous = this->MOs->at(orbitalNumber - 1).GetPointer();
  if (data == previous)
    return;

  vtkDebugMacro(<<"Changing MO " << orbitalNumber << " from @" << previous
                << " to @" << data << ".");

  this->MOs->at(orbitalNumber - 1) = data;

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkProgrammableElectronicData::DeepCopy(vtkDataObject *obj)
{
  vtkProgrammableElectronicData *source =
      vtkProgrammableElectronicData::SafeDownCast(obj);
  if (!source)
    {
    vtkErrorMacro("Can only deep copy from vtkProgrammableElectronicData "
                  "or subclass.");
    return;
    }

  // Call superclass
  this->Superclass::DeepCopy(source);

  this->NumberOfElectrons = source->NumberOfElectrons;

  // Grow vector if needed
  this->SetNumberOfMOs(source->GetNumberOfMOs());

  for (size_t i = 0; i < source->MOs->size(); ++i)
    {
    vtkImageData *current = source->MOs->at(i).GetPointer();
    if (current)
      {
      vtkNew<vtkImageData> newImage;
      newImage->DeepCopy(current);
      this->SetMO(static_cast<vtkIdType>(i), newImage.GetPointer());
      }
    }

  if (source->ElectronDensity)
    {
    vtkNew<vtkImageData> newImage;
    newImage->DeepCopy(source->ElectronDensity);
    this->SetElectronDensity(newImage.GetPointer());
    }

}
