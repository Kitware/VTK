/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVASPTessellationReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkVASPTessellationReader.h"

#include "vtkBoundingBox.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkDataSetAttributes.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMolecule.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointLocator.h"
#include "vtkPoints.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedShortArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

#include <vtksys/RegularExpression.hxx>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <set>
#include <sstream>
#include <vector>

vtkStandardNewMacro(vtkVASPTessellationReader)

typedef vtksys::RegularExpression RegEx;
typedef vtkStreamingDemandDrivenPipeline vtkSDDP;

namespace {

template <typename T>
bool parse(const std::string &str, T &result)
{
  if (!str.empty())
  {
    std::istringstream tmp(str);
    tmp >> result;
    return !tmp.fail();
  }
  return false;
}

template <typename T>
bool parseCommaSepList(const std::string &input, std::vector<T> &data)
{
  std::istringstream in(input);
  for (std::string valStr; std::getline(in, valStr, ',');)
  {
    T tmp;
    if (!parse(valStr, tmp))
    {
      return false;
    }
    data.push_back(tmp);
  }
  return true;
}

// This parses the voronoi points/faces list. The input is expected to be:
// [number of commaSepLists], ([commaSepList]) ([commaSepList]) ...
template <typename T>
bool parseVariableLists(const std::string &input,
                        std::vector<std::vector<T> > &data,
                        RegEx *parenExtract)
{
  std::istringstream in(input);
  size_t nLists;
  in >> nLists;
  if (in.fail())
  {
    return false;
  }

  data.resize(nLists);

  std::string parseBuffer = input;
  for (size_t i = 0; i < nLists; ++i)
  {
    if (!parenExtract->find(parseBuffer))
    {
      return false;
    }
    if (!parseCommaSepList(parenExtract->match(1), data[i]))
    {
      return false;
    }
    // Chop the current match off of the buffer to prepare for the next iter.
    parseBuffer = parseBuffer.substr(parenExtract->end());
  }

  return true;
}

} // end anon namespace

//------------------------------------------------------------------------------
void vtkVASPTessellationReader::PrintSelf(std::ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkVASPTessellationReader::vtkVASPTessellationReader()
  : FileName(NULL),
    TimeParser(new RegEx("^ *time *= *([0-9EeDd.+-]+) *$")), // time = (timeVal)
    LatticeParser(new RegEx("^ *Rx1 *= *([0-9EeDd.+-]+) *," // Rx1
                            " *Rx2 *= *([0-9EeDd.+-]+) *," // Rx2
                            " *Rx3 *= *([0-9EeDd.+-]+) *," // Rx3
                            " *Ry2 *= *([0-9EeDd.+-]+) *," // Ry2
                            " *Ry3 *= *([0-9EeDd.+-]+) *," // Ry3
                            " *Rz3 *= *([0-9EeDd.+-]+) *$" // Rz3
                            )),
    AtomCountParser(new RegEx("^ *Natoms *= *([0-9]+) *$")), // Natoms = (int)),
    AtomParser(new RegEx("^ *([0-9]+) *," // Atom index
                         " *\\(" // Open paren
                         " *([0-9EeDd.+-]+) *," // X coord
                         " *([0-9EeDd.+-]+) *," // Y coord
                         " *([0-9EeDd.+-]+)" // Z coord
                         " *\\) *," // Close paren
                         " *([0-9EeDd.+-]+) *$" // Radius
                         )),
    ParenExtract(new RegEx("\\(([^(]+)\\)")) // Extract contents of (...)
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(2);
}

//------------------------------------------------------------------------------
vtkVASPTessellationReader::~vtkVASPTessellationReader()
{
  this->SetFileName(NULL);
  delete this->TimeParser;
  delete this->LatticeParser;
  delete this->AtomCountParser;
  delete this->AtomParser;
  delete this->ParenExtract;
}

//------------------------------------------------------------------------------
int vtkVASPTessellationReader::RequestData(vtkInformation *,
                                           vtkInformationVector **,
                                           vtkInformationVector *outInfos)
{
  vtkInformation *outInfo0 = outInfos->GetInformationObject(0);
  vtkInformation *outInfo1 = outInfos->GetInformationObject(1);

  vtkMolecule *molecule = vtkMolecule::SafeDownCast(
        outInfo0->Get(vtkDataObject::DATA_OBJECT()));
  assert(molecule);

  vtkUnstructuredGrid *voronoi = vtkUnstructuredGrid::SafeDownCast(
        outInfo1->Get(vtkDataObject::DATA_OBJECT()));
  assert(voronoi);

  std::ifstream in(this->FileName);
  if (!in)
  {
    vtkErrorMacro("Could not open file for reading: "
                  << (this->FileName ? this->FileName : ""));
    return 1;
  }

  // Advance to the selected timestep:
  size_t stepIdx = this->SelectTimeStepIndex(outInfo0);
  double time = 0.;
  for (size_t i = 0; i <= stepIdx; ++i) // <= to read the "time=" line
  {
    if (!this->NextTimeStep(in, time))
    {
      vtkErrorMacro("Error -- attempting to read timestep #" << (stepIdx + 1)
                    << " but encountered a parsing error at timestep #"
                    << (i + 1) << ".");
      return 1;
    }
  }

  if (this->ReadTimeStep(in, molecule, voronoi))
  {
    molecule->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), time);
    voronoi->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), time);
  }
  else
  {
    molecule->Initialize();
    voronoi->Initialize();
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkVASPTessellationReader::RequestInformation(
    vtkInformation *, vtkInformationVector **,
    vtkInformationVector *outInfos)
{
  std::ifstream in(this->FileName);
  if (!in)
  {
    vtkErrorMacro("Could not open file for reading: "
                  << (this->FileName ? this->FileName : ""));
    return 1;
  }

  // Scan the file for timesteps:
  double time;
  std::vector<double> times;
  double timeRange[2] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN };
  while (this->NextTimeStep(in, time))
  {
    times.push_back(time);
    timeRange[0] = std::min(timeRange[0], time);
    timeRange[1] = std::max(timeRange[1], time);
  }

  if (!times.empty())
  {
    for (int port = 0; port < 2; ++port)
    {
      vtkInformation *outInfo = outInfos->GetInformationObject(port);
      outInfo->Set(vtkSDDP::TIME_RANGE(), timeRange, 2);
      outInfo->Set(vtkSDDP::TIME_STEPS(), &times[0],
          static_cast<int>(times.size()));
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkVASPTessellationReader::FillOutputPortInformation(int port,
                                                         vtkInformation *info)
{
  switch (port)
  {
    case 0:
      info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMolecule");
      break;
    case 1:
      info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkUnstructuredGrid");
      break;
    default:
      return 0;
  }

  return 1;
}

//------------------------------------------------------------------------------
bool vtkVASPTessellationReader::NextTimeStep(std::istream &in, double &time)
{
  std::string line;
  while (std::getline(in, line))
  {
    if (this->TimeParser->find(line))
    {
      // Parse timestamp:
      if (!parse(this->TimeParser->match(1), time))
      {
        vtkErrorMacro("Error parsing time information from line: " << line);
        return false;
      }
      return true;
    }
  }

  return false;
}

//------------------------------------------------------------------------------
size_t vtkVASPTessellationReader::SelectTimeStepIndex(vtkInformation *info)
{
  if (!info->Has(vtkSDDP::TIME_STEPS()) ||
      !info->Has(vtkSDDP::UPDATE_TIME_STEP()))
  {
    return 0;
  }

  double *times = info->Get(vtkSDDP::TIME_STEPS());
  int nTimes = info->Length(vtkSDDP::TIME_STEPS());
  double t = info->Get(vtkSDDP::UPDATE_TIME_STEP());

  double resultDiff = VTK_DOUBLE_MAX;
  size_t result = 0;
  for (int i = 0; i < nTimes; ++i)
  {
    double diff = std::fabs(times[i] - t);
    if (diff < resultDiff)
    {
      resultDiff = diff;
      result = static_cast<size_t>(i);
    }
  }

  return result;
}

//------------------------------------------------------------------------------
bool vtkVASPTessellationReader::ReadTimeStep(std::istream &in,
                                             vtkMolecule *molecule,
                                             vtkUnstructuredGrid *voronoi)
{
  // Assume the 'time = ...' line has already been read.
  std::string line;

  // Read the lattice info:
  if (!std::getline(in, line))
  {
    vtkErrorMacro("Unexpected EOF while reading lattice info.");
    return false;
  }
  if (!this->LatticeParser->find(line))
  {
    vtkErrorMacro("Error parsing lattice info from line: " << line);
    return false;
  }

  vtkVector3d latA(0.);
  vtkVector3d latB(0.);
  vtkVector3d latC(0.);
  vtkVector3d latO(0.);

  // Rxx
  if (!parse(this->LatticeParser->match(1), latA[0]))
  {
    vtkErrorMacro("Error parsing Rxx component '"
                  << this->LatticeParser->match(1) << "' from line: " << line);
    return false;
  }
  // Rxy
  if (!parse(this->LatticeParser->match(2), latB[0]))
  {
    vtkErrorMacro("Error parsing Rxy component '"
                  << this->LatticeParser->match(2) << "' from line: " << line);
    return false;
  }
  // Rxz
  if (!parse(this->LatticeParser->match(3), latC[0]))
  {
    vtkErrorMacro("Error parsing Rxz component '"
                  << this->LatticeParser->match(3) << "' from line: " << line);
    return false;
  }
  // Ryy
  if (!parse(this->LatticeParser->match(4), latB[1]))
  {
    vtkErrorMacro("Error parsing Ryy component '"
                  << this->LatticeParser->match(4) << "' from line: " << line);
    return false;
  }
  // Ryz
  if (!parse(this->LatticeParser->match(5), latC[1]))
  {
    vtkErrorMacro("Error parsing Ryz component '"
                  << this->LatticeParser->match(5) << "' from line: " << line);
    return false;
  }
  // Rzz
  if (!parse(this->LatticeParser->match(6), latC[2]))
  {
    vtkErrorMacro("Error parsing Rzz component '"
                  << this->LatticeParser->match(6) << "' from line: " << line);
    return false;
  }

  molecule->SetLattice(latA, latB, latC);
  molecule->SetLatticeOrigin(latO);

  // Number of atoms:
  if (!std::getline(in, line))
  {
    vtkErrorMacro("Unexpected EOF while parsing number of atoms.");
    return false;
  }
  if (!this->AtomCountParser->find(line))
  {
    vtkErrorMacro("Error parsing number of atoms from line: " << line);
    return false;
  }
  vtkIdType nAtoms;
  if (!parse(this->AtomCountParser->match(1), nAtoms))
  {
    vtkErrorMacro("Error parsing number atoms '"
                  << this->AtomCountParser->match(1) << "'' from line: "
                  << line);
    return false;
  }

  // Skip 'Atomic_Numbers =' line:
  if (!std::getline(in, line))
  {
    vtkErrorMacro("Unexpected EOF.");
    return false;
  }

  // Atomic numbers:
  std::vector<unsigned short> atomicNumbers;
  atomicNumbers.reserve(static_cast<size_t>(nAtoms));
  if (!std::getline(in, line))
  {
    vtkErrorMacro("Unexpected EOF while reading atomic number list.");
    return false;
  }
  if (!parseCommaSepList(line, atomicNumbers))
  {
    vtkErrorMacro("Error while parsing atomic number list: " << line);
    return false;
  }

  // Initialize the molecule with atoms, setting just the atomic number.
  // We'll add positions as we parse them later.
  if (static_cast<size_t>(nAtoms) != atomicNumbers.size())
  {
    vtkErrorMacro("Error: expected " << nAtoms << " atomic numbers, but only "
                  "parsed " << atomicNumbers.size());
    return false;
  }
  vtkVector3f pos(0.f);
  for (size_t i = 0; i < atomicNumbers.size(); ++i)
  {
    molecule->AppendAtom(atomicNumbers[i], pos);
  }

  // Now for the actual list of atoms/tessellations
  vtkNew<vtkFloatArray> radii;
  radii->SetNumberOfTuples(nAtoms);

  // Compute unit cell bounds to initialize point merging:
  vtkBoundingBox bbox;
  bbox.AddPoint(latO.GetData());
  bbox.AddPoint((latO + latA).GetData());
  bbox.AddPoint((latO + latB).GetData());
  bbox.AddPoint((latO + latC).GetData());
  bbox.AddPoint((latO + latA + latB).GetData());
  bbox.AddPoint((latO + latA + latC).GetData());
  bbox.AddPoint((latO + latB + latC).GetData());
  bbox.AddPoint((latO + latA + latB + latC).GetData());
  double bounds[6];
  bbox.GetBounds(bounds);

  // Merge the tessellation points using a locator:
  vtkNew<vtkPointLocator> locator;
  vtkNew<vtkPoints> tessPoints;
  tessPoints->SetDataTypeToFloat();
  voronoi->SetPoints(tessPoints.Get());
  voronoi->Allocate(nAtoms);

  // Cell attributes for the voronoi tesselation:
  vtkNew<vtkUnsignedShortArray> tessAtomicNumbers;
  tessAtomicNumbers->SetName("Atomic Numbers");
  tessAtomicNumbers->Allocate(nAtoms);
  vtkNew<vtkIdTypeArray> tessAtomIds;
  tessAtomIds->SetName("Atom Ids");
  tessAtomIds->Allocate(nAtoms);

  // Estimate 10 unique points per atom:
  locator->InitPointInsertion(tessPoints.Get(), bounds, nAtoms * 10);

  // Storage for parsing the tessellation points/faces info
  std::vector<vtkIdType> faceStream;
  std::vector<vtkIdType> pointIds;
  std::set<vtkIdType> uniquePointIds;
  // parse as doubles for locator API, but store as floats:
  std::vector<std::vector<double> > pointData;
  std::vector<std::vector<vtkIdType> > faceData;

  for (vtkIdType atomEntry = 0; atomEntry < nAtoms; ++atomEntry)
  {
    // Skip any blank lines:
    line.clear();
    while (line.empty() || line.find_first_not_of(" \t") == std::string::npos)
    {
      if (!std::getline(in, line))
      {
        vtkErrorMacro("Unexpected EOF while reading atom entry " << atomEntry);
        return false;
      }
    }

    if (!this->AtomParser->find(line))
    {
      vtkErrorMacro("Error parsing atom position/radius specification: "
                    << line);
      return false;
    }
    vtkIdType atomId;
    if (!parse(this->AtomParser->match(1), atomId))
    {
      vtkErrorMacro("Error parsing atomId '" << this->AtomParser->match(1)
                    << "' from line: " << line);
      return false;
    }
    if (!parse(this->AtomParser->match(2), pos[0]))
    {
      vtkErrorMacro("Error parsing x coordinate '" << this->AtomParser->match(2)
                    << "' from line: " << line);
      return false;
    }
    if (!parse(this->AtomParser->match(3), pos[1]))
    {
      vtkErrorMacro("Error parsing y coordinate '" << this->AtomParser->match(3)
                    << "' from line: " << line);
      return false;
    }
    if (!parse(this->AtomParser->match(4), pos[2]))
    {
      vtkErrorMacro("Error parsing z coordinate '" << this->AtomParser->match(4)
                    << "' from line: " << line);
      return false;
    }
    float radius;
    if (!parse(this->AtomParser->match(5), radius))
    {
      vtkErrorMacro("Error parsing radius '" << this->AtomParser->match(5)
                    << "' from line: " << line);
      return false;
    }

    if (atomId >= nAtoms)
    {
      vtkErrorMacro("Found entry for atom with id " << atomId << ", but "
                    "only " << nAtoms << " atoms exist.");
      return false;
    }
    vtkAtom atom = molecule->GetAtom(atomId);
    atom.SetPosition(pos);
    radii->SetTypedComponent(atomId, 0, radius);

    // Extract tessellation points:
    pointData.clear();
    if (!std::getline(in, line))
    {
      vtkErrorMacro("Unexpected EOF while reading voronoi points for atom "
                    << atomId);
      return false;
    }
    if (!parseVariableLists(line, pointData, this->ParenExtract))
    {
      vtkErrorMacro("Error while parsing voronoi point data for atom "
                    << atomId << ". Input: " << line);
      return false;
    }

    // Extract tessellation faces:
    faceData.clear();
    if (!std::getline(in, line))
    {
      vtkErrorMacro("Unexpected EOF while reading voronoi faces for atom "
                    << atomId);
      return false;
    }
    if (!parseVariableLists(line, faceData, this->ParenExtract))
    {
      vtkErrorMacro("Error while parsing voronoi face data for atom "
                    << atomId << ". Input: " << line);
      return false;
    }

    // Add points to locator:
    pointIds.resize(pointData.size());
    uniquePointIds.clear();
    for (size_t i = 0; i < pointData.size(); ++i)
    {
      const std::vector<double> &p = pointData[i];
      if (p.size() != 3)
      {
        vtkErrorMacro("Error: Tessellation point " << i << " for atom "
                      << atomId << " has " << p.size() << " components. "
                      "Expected a 3D coordinate.");
        return false;
      }
      locator->InsertUniquePoint(&p[0], pointIds[i]);
      uniquePointIds.insert(pointIds[i]);
    }

    // Create face stream:
    faceStream.clear();
    for (size_t faceId = 0; faceId < faceData.size(); ++faceId)
    {
      const std::vector<vtkIdType> &face = faceData[faceId];
      faceStream.push_back(face.size());
      for (std::vector<vtkIdType>::const_iterator it = face.begin(),
           itEnd = face.end(); it != itEnd; ++it)
      {
        // Convert the local point id into the dataset point id:
        vtkIdType datasetId = pointIds[*it];
        faceStream.push_back(datasetId);
      }
    }

    // Reuse pointIds to prepare a contiguous buffer of the unique pointId set
    pointIds.resize(uniquePointIds.size());
    std::copy(uniquePointIds.begin(), uniquePointIds.end(), pointIds.begin());

    // Add cell to tessellation dataset:
    voronoi->InsertNextCell(VTK_POLYHEDRON,
                            static_cast<vtkIdType>(pointIds.size()),
                            pointIds.empty() ? NULL : &pointIds[0],
                            faceData.size(),
                            faceStream.empty() ? NULL : &faceStream[0]);
    tessAtomicNumbers->InsertNextValue(atom.GetAtomicNumber());
    tessAtomIds->InsertNextValue(atom.GetId());
  }

  molecule->GetVertexData()->AddArray(radii.Get());
  voronoi->GetCellData()->SetScalars(tessAtomicNumbers.Get());
  voronoi->GetCellData()->AddArray(tessAtomIds.Get());

  return true;
}
