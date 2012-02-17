/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenQubeElectronicData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenQubeElectronicData.h"

#include "vtkDataSetCollection.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"

#include <openqube/basisset.h>
#include <openqube/cube.h>

// Internal class to store queue/qube information along with the image
class OQEDImageData : public vtkImageData
{
public:
  vtkTypeMacro(OQEDImageData, vtkImageData);
  static OQEDImageData * New();

  vtkSetMacro(OrbitalNumber, vtkIdType);
  vtkGetMacro(OrbitalNumber, vtkIdType);

  vtkSetMacro(ImageType, OpenQube::Cube::Type);
  vtkGetMacro(ImageType, OpenQube::Cube::Type);

  vtkSetMacro(MetaSpacing, float);
  vtkGetMacro(MetaSpacing, float);

  vtkSetMacro(MetaPadding, float);
  vtkGetMacro(MetaPadding, float);

  virtual void DeepCopy(vtkDataObject *src)
    {
    this->Superclass::DeepCopy(src);
    OQEDImageData *o = OQEDImageData::SafeDownCast(src);
    if (!o)
      {
      // Just return without copying metadata
      return;
      }
    this->OrbitalNumber = o->OrbitalNumber;
    this->ImageType = o->ImageType;
    this->MetaSpacing = o->MetaSpacing;
    this->MetaPadding = o->MetaPadding;
    }

protected:
  OQEDImageData() {};
  ~OQEDImageData() {};

  vtkIdType OrbitalNumber;
  OpenQube::Cube::Type ImageType;

  float MetaSpacing;
  float MetaPadding;

private:
  OQEDImageData(const OQEDImageData&);  // Not implemented
  void operator=(const OQEDImageData&); // Not implemented
};
vtkStandardNewMacro(OQEDImageData);

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenQubeElectronicData);

//----------------------------------------------------------------------------
vtkOpenQubeElectronicData::vtkOpenQubeElectronicData()
  : BasisSet(NULL), Spacing(0.1)
{
  this->Padding = 2.0;
}

//----------------------------------------------------------------------------
vtkOpenQubeElectronicData::~vtkOpenQubeElectronicData()
{
}

//----------------------------------------------------------------------------
void vtkOpenQubeElectronicData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "BasisSet: @" << this->BasisSet << "\n";

  // Dump Images
  os << indent << "Images: @" << this->Images << "\n";
  vtkCollectionSimpleIterator cookie;
  this->Images->InitTraversal(cookie);
  while (vtkDataSet *dataset = this->Images->GetNextDataSet(cookie))
    {
    vtkImageData *data = vtkImageData::SafeDownCast(dataset);
    // If this is somehow not a vtkImageData, print a warning and skip it
    if (!data)
      {
      vtkWarningMacro(<<"vtkDataSet in this->Images is not a vtkImageData"
                      " object. This should not happen...");
      continue;
      }

    OQEDImageData * oqeddata = OQEDImageData::SafeDownCast(dataset);
    // Identify the type of image
    if (oqeddata)
      {
      switch (oqeddata->GetImageType())
        {
        case OpenQube::Cube::MO:
          os << indent << "this->Images has molecular orbital #"
             << oqeddata->GetOrbitalNumber() << " imagedata @"
             << oqeddata << ":\n";
          break;
        case OpenQube::Cube::ElectronDensity:
          os << indent << "this->Images has electron density imagedata "
                "@" << oqeddata << ":\n";
          break;
        case OpenQube::Cube::VdW:
          os << indent << "this->Images has van der Waals imagedata"
                " @" << oqeddata << ":\n";
          break;
        case OpenQube::Cube::ESP:
          os << indent << "this->Images has electrostatic potential imagedata"
                " @" << oqeddata << ":\n";
          break;
        case OpenQube::Cube::FromFile:
          os << indent << "this->Images has file-loaded imagedata"
                " @" << oqeddata << ":\n";
          break;
        default:
        case OpenQube::Cube::None:
          os << indent << "this->Images has imagedata from an unknown source"
                " @" << oqeddata << ":\n";
          break;
        }
      }
    else // oqeddata null
      {
      os << indent << "this->Images has imagedata that was externally "
            "generated @" << data << ":\n";
      }
    data->PrintSelf(os, indent.GetNextIndent());
    }
}

//----------------------------------------------------------------------------
vtkIdType vtkOpenQubeElectronicData::GetNumberOfMOs()
{
  if (!this->BasisSet || !this->BasisSet->isValid())
    {
    return 0;
    }

  return this->BasisSet->numMOs();
}

//----------------------------------------------------------------------------
unsigned int vtkOpenQubeElectronicData::GetNumberOfElectrons()
{
  if (!this->BasisSet || !this->BasisSet->isValid())
    {
    return 0;
    }

  return this->BasisSet->numElectrons();
}

//----------------------------------------------------------------------------
vtkImageData * vtkOpenQubeElectronicData::GetMO(vtkIdType orbitalNumber)
{
  vtkDebugMacro(<<"Searching for MO " << orbitalNumber);
  // First check if there is an existing image for this orbital
  vtkCollectionSimpleIterator cookie;
  this->Images->InitTraversal(cookie);
  while (vtkDataSet *dataset = this->Images->GetNextDataSet(cookie))
    {
    OQEDImageData * data = OQEDImageData::SafeDownCast(dataset);
    if (!data)
      {
      continue;
      }

    if (data->GetImageType() != OpenQube::Cube::MO)
      {
      continue;
      }

    if (data->GetOrbitalNumber() != orbitalNumber)
      {
      continue;
      }

    if (data->GetMetaSpacing() != this->Spacing ||
        data->GetMetaPadding() != this->Padding)
      {
      continue;
      }

    vtkDebugMacro(<<"Found MO " << orbitalNumber);
    return static_cast<vtkImageData*>(data);
    }

  // If there is no existing image, calculate it
  vtkDebugMacro(<<"MO " << orbitalNumber << " not found. Calculating...");
  return this->CalculateMO(orbitalNumber);
}

//----------------------------------------------------------------------------
vtkImageData * vtkOpenQubeElectronicData::GetElectronDensity()
{
  // First check if there is an existing image for this orbital
  vtkCollectionSimpleIterator cookie;
  this->Images->InitTraversal(cookie);
  while (vtkDataSet *dataset = this->Images->GetNextDataSet(cookie))
    {
    OQEDImageData * data = OQEDImageData::SafeDownCast(dataset);
    if (!data)
      {
      continue;
      }

    if (data->GetImageType() != OpenQube::Cube::ElectronDensity)
      {
      continue;
      }

    if (data->GetMetaSpacing() != this->Spacing ||
        data->GetMetaPadding() != this->Padding)
      {
      continue;
      }

    return static_cast<vtkImageData*>(data);
    }

  // If there is no existing image, calculate it
  return this->CalculateElectronDensity();
}

//----------------------------------------------------------------------------
void vtkOpenQubeElectronicData::DeepCopy(vtkDataObject *obj)
{
  vtkOpenQubeElectronicData *oqed =
      vtkOpenQubeElectronicData::SafeDownCast(obj);
  if (!oqed)
    {
    vtkErrorMacro("Can only deep copy from vtkOpenQubeElectronicData "
                  "or subclass.");
    return;
    }

  // Call superclass
  this->Superclass::DeepCopy(oqed);

  // Copy all images over by hand to get OQEDImageData metadata right
  vtkCollectionSimpleIterator cookie;
  oqed->Images->InitTraversal(cookie);
  while (vtkDataSet *dataset = oqed->Images->GetNextDataSet(cookie))
    {
    // Only copy valid OQEDImageData objects
    if (OQEDImageData *oqedImage = OQEDImageData::SafeDownCast(dataset))
      {
      vtkNew<OQEDImageData> thisImage;
      thisImage->DeepCopy(oqedImage);
      this->Images->AddItem(thisImage.GetPointer());
      }
    }

  // Copy other ivars
  if (oqed->BasisSet)
    {
    this->BasisSet = oqed->BasisSet->clone();
    }
  this->Spacing = oqed->Spacing;
}

//----------------------------------------------------------------------------
vtkImageData * vtkOpenQubeElectronicData::CalculateMO(vtkIdType orbitalNumber)
{
  vtkDebugMacro(<<"Calculating MO " << orbitalNumber);
  if (!this->BasisSet)
    {
    vtkWarningMacro(<<"No OpenQube::BasisSet set.");
    return 0;
    }
  else if (!this->BasisSet->isValid())
    {
    vtkWarningMacro(<<"Invalid OpenQube::BasisSet set.");
    return 0;
    }

  // Create and calculate cube
  OpenQube::Cube *cube = new OpenQube::Cube();
  cube->setLimits(this->BasisSet->moleculeRef(), this->Spacing,
                  this->Padding);

  vtkDebugMacro(<<"Calculating OpenQube::Cube for MO " << orbitalNumber);
  if (!this->BasisSet->blockingCalculateCubeMO( cube, orbitalNumber ))
    {
    vtkWarningMacro(<< "Unable to calculateMO for orbital " << orbitalNumber
                    << " in OpenQube.");
    return 0;
    }

  // Create image and set metadata
  vtkNew<OQEDImageData> image;
  image->SetMetaSpacing(this->Spacing);
  image->SetMetaPadding(this->Padding);
  vtkDebugMacro(<<"Converting OpenQube::Cube to vtkImageData for MO "
                << orbitalNumber);

  // Copy cube --> image
  this->FillImageDataFromQube( cube, image.GetPointer() );
  image->SetOrbitalNumber(orbitalNumber);

  vtkDebugMacro(<<"Adding vtkImageData to this->Images for MO "
                << orbitalNumber);
  this->Images->AddItem(image.GetPointer());

  return image.GetPointer();
}

//----------------------------------------------------------------------------
vtkImageData * vtkOpenQubeElectronicData::CalculateElectronDensity()
{
  vtkDebugMacro(<<"Calculating electron density...");
  if (!this->BasisSet)
    {
    vtkErrorMacro(<<"No OpenQube::BasisSet set.");
    return 0;
    }
  else if (!this->BasisSet->isValid())
    {
    vtkErrorMacro(<<"Invalid OpenQube::BasisSet set.");
    return 0;
    }

  // Create and calculate cube
  OpenQube::Cube *cube = new OpenQube::Cube();
  cube->setLimits(this->BasisSet->moleculeRef(), this->Spacing,
                  this->Padding);

  vtkDebugMacro(<<"Calculating OpenQube::Cube...");
  if (!this->BasisSet->blockingCalculateCubeDensity(cube))
    {
    vtkWarningMacro(<<"Unable to calculate density in OpenQube.");
    return 0;
    }

  // Create image and set metadata
  vtkNew<OQEDImageData> image;
  image->SetMetaSpacing(this->Spacing);
  image->SetMetaPadding(this->Padding);
  vtkDebugMacro(<<"Converting OpenQube::Cube to vtkImageData.");

  // Copy cube --> image
  this->FillImageDataFromQube( cube, image.GetPointer() );

  vtkDebugMacro(<<"Adding vtkImageData to this->Images");
  this->Images->AddItem(image.GetPointer());

  return image.GetPointer();
}

//----------------------------------------------------------------------------
void vtkOpenQubeElectronicData::FillImageDataFromQube(OpenQube::Cube *qube,
                                                      vtkImageData *image)
{
  Eigen::Vector3i dim = qube->dimensions();
  Eigen::Vector3d min = qube->min();
  Eigen::Vector3d max = qube->max();
  Eigen::Vector3d spacing = qube->spacing();

  vtkDebugMacro(<< "Converting OpenQube::Cube to vtkImageData:"
                << "\n\tDimensions: "
                << dim[0] << "  " << dim[1] << " " << dim[2]
                << "\n\tMinimum: "
                << min[0] << "  " << min[1] << " " << min[2]
                << "\n\tMaximum: "
                << max[0] << "  " << max[1] << " " << max[2]
                << "\n\tSpacing: "
                << spacing[0] << "  " << spacing[1] << " " << spacing[2]);

  if (OQEDImageData *oqedImage = OQEDImageData::SafeDownCast(image))
    {
    vtkDebugMacro("Setting cube type to " << qube->cubeType());
    oqedImage->SetImageType(qube->cubeType());
    }
  else
    {
    vtkWarningMacro(
      <<"Cannot cast input image to internal OQEDImageData type.");
    }

  image->SetNumberOfScalarComponents(1);
  image->SetScalarTypeToDouble();
  image->SetExtent(0, dim[0] - 1, 0, dim[1] - 1, 0, dim[2] - 1);
  image->SetOrigin(min.data());
  image->SetSpacing(spacing.data());
  image->AllocateScalars();
  image->Update();

  double *dataPtr = static_cast<double *>(image->GetScalarPointer());
  std::vector<double> *qubeVec = qube->data();

  const size_t qubeSize = qubeVec->size();

  if (qubeSize != static_cast<size_t>(dim[0]) * dim[1] * dim[2])
    {
    vtkWarningMacro(<< "Size of qube (" << qubeSize << ") does not equal"
                    << " product of dimensions (" << dim[0]*dim[1]*dim[2]
                    << "). Image may not be accurate.");
    }

  long int qubeInd = -1; // Incremented to 0 on first use.
  for (int i = 0; i < dim[0]; ++i)
    {
    for (int j = 0; j < dim[1]; ++j)
      {
      for (int k = 0; k < dim[2]; ++k)
        {
        dataPtr[(k * dim[1] + j) * dim[0] + i] = (*qubeVec)[++qubeInd];
        }
      }
    }

  vtkDebugMacro(<< "Copied " << qubeSize
                << " (actual: " << qubeInd + 1
                << ") points from qube to vtkImageData.");
  image->Update();
}
