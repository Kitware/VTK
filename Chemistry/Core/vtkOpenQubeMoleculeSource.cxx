/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenQubeMoleculeSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
#include "vtkOpenQubeMoleculeSource.h"

#include "vtkExecutive.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkOpenQubeElectronicData.h"
#include "vtkMolecule.h"

#include <openqube/basisset.h>
#include <openqube/basissetloader.h>
#include <openqube/molecule.h>

#include <vector>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenQubeMoleculeSource);

//----------------------------------------------------------------------------
vtkOpenQubeMoleculeSource::vtkOpenQubeMoleculeSource()
  : vtkDataReader(),
    FileName(NULL),
    CleanUpBasisSet(false)
{
}

//----------------------------------------------------------------------------
vtkOpenQubeMoleculeSource::~vtkOpenQubeMoleculeSource()
{
  this->SetFileName(NULL);
  if (this->CleanUpBasisSet && this->BasisSet != NULL)
    {
    delete this->BasisSet;
    this->BasisSet = NULL;
    }
}

//----------------------------------------------------------------------------
vtkMolecule *vtkOpenQubeMoleculeSource::GetOutput()
{
  return vtkMolecule::SafeDownCast(this->GetOutputDataObject(0));;
}

//----------------------------------------------------------------------------
void vtkOpenQubeMoleculeSource::SetOutput(vtkMolecule *output)
{
  this->GetExecutive()->SetOutputData(0, output);
}

//----------------------------------------------------------------------------
void vtkOpenQubeMoleculeSource::SetBasisSet(OpenQube::BasisSet *b)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this
                << "): setting BasisSet to " << b);
  if (this->BasisSet != b)
    {
    if (this->CleanUpBasisSet && this->BasisSet != NULL)
      {
      delete this->BasisSet;
      }
    this->BasisSet = b;
    this->CleanUpBasisSetOff();
    this->Modified();
    }
}

//----------------------------------------------------------------------------
int vtkOpenQubeMoleculeSource::RequestData(
  vtkInformation *,
  vtkInformationVector **,
  vtkInformationVector *outputVector)
{
  vtkMolecule *output = vtkMolecule::SafeDownCast
    (vtkDataObject::GetData(outputVector));

  if (!output)
    {
    vtkWarningMacro(<<"vtkOpenQubeMoleculeSource does not have a vtkMolecule "
                  "as output.");
    return 1;
    }

  // Obtain basis set
  OpenQube::BasisSet *basisSet = 0;
  if (this->BasisSet)
    {
    basisSet = this->BasisSet;
    }
  else
    {
    if (!this->FileName)
      {
      vtkWarningMacro(<<"No FileName or OpenQube::BasisSet specified.");
      return 1;
      }
    // We're creating the BasisSet, so we need to clean it up
    this->CleanUpBasisSetOn();
    // Huge padding, better safe than sorry.
    char basisName[strlen(this->FileName) + 256];
    OpenQube::BasisSetLoader::MatchBasisSet(this->FileName, basisName);
    if (!basisName[0])
      {
      vtkErrorMacro(<< "OpenQube cannot find matching basis set file for '"
                    << this->FileName << "'");
      return 1;
      }
    basisSet = OpenQube::BasisSetLoader::LoadBasisSet(basisName);
    this->BasisSet = basisSet;
    vtkDebugMacro(<<"Loaded basis set file: "<< basisName);
    }

  // Populate vtkMolecule
  const OpenQube::Molecule &oqmol = basisSet->moleculeRef();
  this->CopyOQMoleculeToVtkMolecule(&oqmol, output);

  // Add ElectronicData
  vtkNew<vtkOpenQubeElectronicData> oqed;
  oqed->SetBasisSet(basisSet);
  output->SetElectronicData(oqed.GetPointer());

  return 1;
}

//----------------------------------------------------------------------------
int vtkOpenQubeMoleculeSource::FillOutputPortInformation(int,
                                                         vtkInformation *info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMolecule");
  return 1;
}

//----------------------------------------------------------------------------
void vtkOpenQubeMoleculeSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "FileName: " << this->FileName << "\n";
}

//----------------------------------------------------------------------------
void vtkOpenQubeMoleculeSource::CopyOQMoleculeToVtkMolecule(
  const OpenQube::Molecule *oqmol, vtkMolecule *mol)
{
  mol->Initialize();
  // Copy atoms
  Eigen::Vector3d pos;
  for (size_t i = 0; i < oqmol->numAtoms(); ++i)
    {
    vtkAtom atom = mol->AppendAtom();
    pos = oqmol->atomPos(i);
    atom.SetPosition(vtkVector3d(pos.data()).Cast<float>().GetData());
    atom.SetAtomicNumber(oqmol->atomAtomicNumber(i));
    }

  // TODO copy bonds (OQ doesn't currently have bonds)
}
