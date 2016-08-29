/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVASPAnimationReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkVASPAnimationReader.h"

#include "vtkDataSetAttributes.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMolecule.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkVectorOperators.h"

#include <vtksys/RegularExpression.hxx>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <sstream>
#include <vector>

typedef vtksys::RegularExpression RegEx;

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

bool approxEqual(const vtkVector3d &a, const vtkVector3d &b, float tol)
{
  return (a - b).SquaredNorm() < tol;
}

} // end anon namespace

vtkStandardNewMacro(vtkVASPAnimationReader)

//------------------------------------------------------------------------------
void vtkVASPAnimationReader::PrintSelf(std::ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkVASPAnimationReader::vtkVASPAnimationReader()
  : FileName(NULL),
    TimeParser(new RegEx("^ *time *= *([0-9EeDd.+-]+) *$")), // time = (timeVal)
    SegmentParser(new RegEx("^ *([0-9EeDd.+-]+) +" // Set of 6 floats
                            "([0-9EeDd.+-]+) +"
                            "([0-9EeDd.+-]+) +"
                            "([0-9EeDd.+-]+) +"
                            "([0-9EeDd.+-]+) +"
                            "([0-9EeDd.+-]+) *$")),
    AtomCountParser(new RegEx("^ *([0-9]+) *$")), // Just a single integer
    AtomParser(new RegEx("^ *[0-9]+ +" // Atom index
                         "([0-9]+) +" // Atomic number
                         "[A-Za-z]+ +" // Element symbol
                         "([0-9EeDd.+-]+) +" // X coordinate
                         "([0-9EeDd.+-]+) +" // Y coordinate
                         "([0-9EeDd.+-]+) +" // Z coordinate
                         "([0-9EeDd.+-]+) +" // Radius
                         "([0-9EeDd.+-]+) *$")) // Kinetic energy
{
  this->SetNumberOfInputPorts(0);
}

//------------------------------------------------------------------------------
vtkVASPAnimationReader::~vtkVASPAnimationReader()
{
  this->SetFileName(NULL);
  delete this->TimeParser;
  delete this->SegmentParser;
  delete this->AtomCountParser;
  delete this->AtomParser;
}

//------------------------------------------------------------------------------
int vtkVASPAnimationReader::RequestData(vtkInformation *,
                                        vtkInformationVector **,
                                        vtkInformationVector *outInfos)
{
  vtkInformation *outInfo = outInfos->GetInformationObject(0);

  vtkMolecule *output = vtkMolecule::SafeDownCast(
        outInfo->Get(vtkDataObject::DATA_OBJECT()));
  assert(output);

  std::ifstream in(this->FileName);
  if (!in)
    {
    vtkErrorMacro("Could not open file for reading: "
                  << (this->FileName ? this->FileName : ""));
    return 1;
    }

  // Advance to the selected timestep:
  size_t stepIdx = this->SelectTimeStepIndex(outInfo);
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

  if (this->ReadMolecule(in, output))
    {
    output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), time);
    }
  else
    {
    output->Initialize();
    }

  return 1;
}

//------------------------------------------------------------------------------
int vtkVASPAnimationReader::RequestInformation(vtkInformation *,
                                               vtkInformationVector **,
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
    vtkInformation *outInfo = outInfos->GetInformationObject(0);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &times[0],
                 static_cast<int>(times.size()));
    }

  return 1;
}

//------------------------------------------------------------------------------
bool vtkVASPAnimationReader::NextTimeStep(std::istream &in, double &time)
{
  std::string line;
  while (std::getline(in, line))
    {
    if (this->TimeParser->find(line))
      {
      // Parse timestamp:
      if (!parse(this->TimeParser->match(1), time))
        {
        vtkErrorMacro("Error parsing time information from line: " << line );
        return false;
        }
      return true;
      }
    }

  return false;
}

//------------------------------------------------------------------------------
size_t vtkVASPAnimationReader::SelectTimeStepIndex(vtkInformation *info)
{
  if (!info->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()) ||
      !info->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
    {
    return 0;
    }

  double *times = info->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  int nTimes = info->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  double t = info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

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
bool vtkVASPAnimationReader::ReadMolecule(std::istream &in,
                                          vtkMolecule *molecule)
{
  std::string line;

  bool latticeFound;
  vtkVector3d lattice[3];
  vtkVector3d origin;
  if (!this->DetermineLatticeVectors(in, lattice, origin, latticeFound))
    { // parse error. DetermineLatticeVectors prints the error message for us.
    return false;
    }

  // If lattice determination fails, it's not fatal -- it's likely that the
  // code for determining the vectors missed a corner case. We should still
  // ship out the atomic data.
  if (latticeFound)
    {
    molecule->SetLattice(lattice[0], lattice[1], lattice[2]);
    molecule->SetLatticeOrigin(origin);
    }

  // Next line should be the number of atoms in the molecule:
  if (!std::getline(in, line))
    {
    vtkErrorMacro("Unexpected EOF while parsing atom count.");
    return false;
    }
  if (!this->AtomCountParser->find(line))
    {
    vtkErrorMacro("Error parsing atom count from line: " << line);
    return false;
    }
  vtkIdType numAtoms;
  if (!parse(this->AtomCountParser->match(1), numAtoms))
    {
    vtkErrorMacro("Error parsing atom count as integer: "
                  << this->AtomCountParser->match(1));
    return false;
    }

  // Create some attribute arrays to store the radii and energy.
  vtkNew<vtkFloatArray> radii;
  radii->SetName("radii");
  radii->SetNumberOfTuples(numAtoms);

  vtkNew<vtkFloatArray> kineticEnergies;
  kineticEnergies->SetName("kinetic_energy");
  kineticEnergies->SetNumberOfTuples(numAtoms);

  // Atoms are next:
  for (vtkIdType atomIdx = 0; atomIdx < numAtoms; ++atomIdx)
    {
    if (!std::getline(in, line))
      {
      vtkErrorMacro("Unexpected EOF while parsing atom at index " << atomIdx);
      return false;
      }
    if (!this->AtomParser->find(line))
      {
      vtkErrorMacro("Malformed atom specification: " << line);
      return false;
      }

    unsigned short atomicNumber;
    if (!parse(this->AtomParser->match(1), atomicNumber))
      {
      vtkErrorMacro("Error parsing atomic number '"
                    << this->AtomParser->match(1) << "' from line: " << line);
      return false;
      }

    vtkVector3f position;
    if (!parse(this->AtomParser->match(2), position[0]))
      {
      vtkErrorMacro("Error parsing x coordinate '"
                    << this->AtomParser->match(2) << "' from line: " << line);
      return false;
      }
    if (!parse(this->AtomParser->match(3), position[1]))
      {
      vtkErrorMacro("Error parsing y coordinate '"
                    << this->AtomParser->match(3) << "' from line: " << line);
      return false;
      }
    if (!parse(this->AtomParser->match(4), position[2]))
      {
      vtkErrorMacro("Error parsing z coordinate '"
                    << this->AtomParser->match(4) << "' from line: " << line);
      return false;
      }

    float radius;
    if (!parse(this->AtomParser->match(5), radius))
      {
      vtkErrorMacro("Error parsing radius '"
                    << this->AtomParser->match(5) << "' from line: " << line);
      return false;
      }

    float kineticEnergy;
    if (!parse(this->AtomParser->match(6), kineticEnergy))
      {
      vtkErrorMacro("Error parsing kinetic energy '"
                    << this->AtomParser->match(6) << "' from line: " << line);
      return false;
      }

    molecule->AppendAtom(atomicNumber, position);
    radii->SetTypedComponent(atomIdx, 0, radius);
    kineticEnergies->SetTypedComponent(atomIdx, 0, kineticEnergy);
    }

  vtkDataSetAttributes *atomData = molecule->GetVertexData();
  atomData->AddArray(radii.Get());
  atomData->AddArray(kineticEnergies.Get());

  return true;
}

bool vtkVASPAnimationReader::DetermineLatticeVectors(std::istream &in,
                                                     vtkVector3d lattice[3],
                                                     vtkVector3d &origin,
                                                     bool &latticeFound)
{
  latticeFound = false;
  const float tol = 1e-5; // Tolerance for vector comparisons
  std::string line;

  // Read segment info from input:
  typedef vtkVector3d Segment[2];
  Segment segments[12];
  for (size_t segmentIdx = 0; segmentIdx < 12; ++segmentIdx)
    {
    if (!std::getline(in, line))
      {
      vtkErrorMacro("Unexpected EOF while parsing boundary segments.");
      return false;
      }
    if (!this->SegmentParser->find(line))
      {
      vtkErrorMacro("Malformed boundary segment specification: " << line);
      return false;
      }
    for (int pointIdx = 0; pointIdx < 2; ++pointIdx)
      {
      for (int compIdx = 0; compIdx < 3; ++compIdx)
        {
        int matchIdx = pointIdx * 3 + compIdx + 1; // for regex match lookup
        if (!parse(this->SegmentParser->match(matchIdx),
                   segments[segmentIdx][pointIdx][compIdx]))
          {
          vtkErrorMacro("Error parsing boundary specification: Value is not a "
                        "floating point number: "
                        << this->SegmentParser->match(matchIdx));
          return false;
          }
        }
      }
    }

  // Choose an origin. Select the most negative point considering x, y, then z:
  origin = segments[0][0];
  for (size_t segmentIdx = 0; segmentIdx < 12; ++segmentIdx)
    {
    for (size_t pointIdx = 0; pointIdx < 2; ++pointIdx)
      {
      const vtkVector3d &candidateOrigin = segments[segmentIdx][pointIdx];
      for (int compIdx = 0; compIdx < 3; ++compIdx)
        {
        if (candidateOrigin[compIdx] < origin[compIdx] - tol)
          { // definitely less than, this is the new origin
          origin = candidateOrigin;
          break;
          }
        else if (candidateOrigin[compIdx] > origin[compIdx] + tol)
          { // definitely greater than, move to the next point
          break;
          }
        else
          { // approximately equal, check next component
          continue;
          }
        }
      }
    }

  // Find the segments containing the origin point. There should be 3.
  size_t latticeIdx = 0;
  for (size_t segmentIdx = 0; segmentIdx < 12 && latticeIdx < 3; ++segmentIdx)
    {
    for (size_t pointIdx = 0; pointIdx < 2 && latticeIdx < 3; ++pointIdx)
      {
      const vtkVector3d &candidateOrigin = segments[segmentIdx][pointIdx];
      const vtkVector3d &endpoint = segments[segmentIdx][pointIdx + 1 % 2];

      if (approxEqual(origin, candidateOrigin, tol))
        {
        // Compute the lattice vector:
        lattice[latticeIdx++] = endpoint - candidateOrigin;
        }
      }
    }

  if (latticeIdx < 3)
    {
    vtkWarningMacro("Error determining lattice vectors from boundary segments. "
                    "Only " << latticeIdx << " segments contain the selected "
                    "origin point (" << origin[0] << " " << origin[1] << " "
                    << origin[2] << ").");
    // return true -- parsing succeeded. latticeFound is false to indicate the
    // error.
    return true;
    }

  // Finally, sort the lattice vectors to the usual convention:
  // a: largest x component
  // b: in x-y plane
  // c: largest z component

  // Set a to the vector with the greatest x:
  for (size_t i = 1; i < 2; ++i)
    {
    if (lattice[i][0] > lattice[0][0])
      {
      std::swap(lattice[i], lattice[0]);
      }
    }

  // Sort the last two by just comparing the z values:
  if (lattice[1][2] > lattice[2][2])
    {
    std::swap(lattice[1], lattice[2]);
    }

  latticeFound = true;
  return true;
}
