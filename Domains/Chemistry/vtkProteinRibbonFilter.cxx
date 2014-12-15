/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProteinRibbonFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkProteinRibbonFilter.h"

#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkTubeFilter.h"
#include "vtkDoubleArray.h"
#include "vtkSplineFilter.h"
#include "vtkInformation.h"
#include "vtkStringArray.h"
#include "vtkCellArray.h"
#include "vtkObjectFactory.h"
#include "vtkVector.h"
#include "vtkMath.h"
#include "vtkCellData.h"
#include "vtkPolyDataNormals.h"
#include "vtkSphereSource.h"
#include "vtkPeriodicTable.h"
#include "vtkMolecule.h"
#include "vtkVectorOperators.h"
#include <map>

vtkStandardNewMacro(vtkProteinRibbonFilter)

namespace
{
// Shamelessly copied from vtkColorSeries vtkColor3ubFromHex3, should be added
// to the vtkColor code.
vtkColor3ub ToColor3ubFromHex3(vtkTypeUInt32 hex)
{
  unsigned char b = hex & 0xff;
  hex >>= 8;
  unsigned char g = hex & 0xff;
  hex >>= 8;
  unsigned char r = hex & 0xff;
  return vtkColor3ub(r, g, b);
}
// Again, should be added to the vtkColor code.
vtkColor3ub ToColor3ubFromColor3f(const vtkColor3f& color)
{
  return vtkColor3ub(static_cast<unsigned char>(color[0] * 255.0f),
                     static_cast<unsigned char>(color[1] * 255.0f),
                     static_cast<unsigned char>(color[2] * 255.0f));
}
} // End of anonymous namespace.

vtkProteinRibbonFilter::vtkProteinRibbonFilter()
{
  this->CoilWidth = 0.3f;
  this->HelixWidth = 1.3f;
  this->SphereResolution = 20;
  this->SubdivideFactor = 20;
  this->DrawSmallMoleculesAsSpheres = true;

  this->ElementColors["H"]  = ToColor3ubFromHex3(0xCCCCCC);
  this->ElementColors["C"]  = ToColor3ubFromHex3(0xAAAAAA);
  this->ElementColors["O"]  = ToColor3ubFromHex3(0xCC0000);
  this->ElementColors["N"]  = ToColor3ubFromHex3(0x0000CC);
  this->ElementColors["S"]  = ToColor3ubFromHex3(0xCCCC00);
  this->ElementColors["P"]  = ToColor3ubFromHex3(0x6622CC);
  this->ElementColors["F"]  = ToColor3ubFromHex3(0x00CC00);
  this->ElementColors["CL"] = ToColor3ubFromHex3(0x00CC00);
  this->ElementColors["BR"] = ToColor3ubFromHex3(0x882200);
  this->ElementColors["I"]  = ToColor3ubFromHex3(0x6600AA);
  this->ElementColors["FE"] = ToColor3ubFromHex3(0xCC6600);
  this->ElementColors["CA"] = ToColor3ubFromHex3(0xDDDDDD);
}

vtkProteinRibbonFilter::~vtkProteinRibbonFilter()
{
}

int vtkProteinRibbonFilter::FillInputPortInformation(int port,
                                                     vtkInformation *info)
{
  if(!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }

  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  return 1;
}

int vtkProteinRibbonFilter::RequestData(vtkInformation *,
                                        vtkInformationVector **inputVector,
                                        vtkInformationVector *outputVector)
{
  vtkPolyData *input = vtkPolyData::GetData(inputVector[0]);
  vtkPolyData *output = vtkPolyData::GetData(outputVector);

  vtkPointData* pointData = input->GetPointData();

  // Extract alpha-carbon backbone from input poly data
  vtkStringArray *atomTypes =
    vtkStringArray::SafeDownCast(pointData->GetAbstractArray("atom_types"));
  vtkIdTypeArray *atomType =
    vtkIdTypeArray::SafeDownCast(pointData->GetAbstractArray("atom_type"));

  if (!atomTypes || !atomType)
    {
    vtkErrorMacro(<< "Atom Type String & Ids Arrays Required");
    return 0;
    }

  // Extract secondary structures information from input poly data
  vtkIdTypeArray *resi =
    vtkIdTypeArray::SafeDownCast(pointData->GetAbstractArray("residue"));
  vtkUnsignedCharArray *chain =
    vtkUnsignedCharArray::SafeDownCast(pointData->GetAbstractArray("chain"));
  vtkUnsignedCharArray *atom_ss =
    vtkUnsignedCharArray::SafeDownCast(pointData->GetAbstractArray("secondary_structures"));
  vtkUnsignedCharArray *atom_ssbegin =
    vtkUnsignedCharArray::SafeDownCast(pointData->GetAbstractArray("secondary_structures_begin"));
  vtkUnsignedCharArray *atom_ssend =
    vtkUnsignedCharArray::SafeDownCast(pointData->GetAbstractArray("secondary_structures_end"));
  vtkUnsignedCharArray *ishetatm =
    vtkUnsignedCharArray::SafeDownCast(pointData->GetAbstractArray("ishetatm"));

  if (!resi || !chain || !atom_ss || !atom_ssbegin || !atom_ssend || !ishetatm)
    {
    vtkErrorMacro(<< "Atom Secondary Structures Arrays Required");
    return 0;
    }

  unsigned char currentChain = 0;
  unsigned char ss = 0;
  vtkIdType currentResi = 0;
  vtkVector3f currentCA(0.f);
  vtkVector3f prevCO(0.f);
  bool hasPrevCO = false;

  vtkNew<vtkPoints> strandPoints;
  vtkNew<vtkPolyData> strand;
  strand->Allocate();
  strand->SetPoints(strandPoints.GetPointer());

  vtkNew<vtkUnsignedCharArray> pointsColors;
  pointsColors->SetName("RGB");
  pointsColors->SetNumberOfComponents(3);

  // Initialize colors per point/atom
  std::vector<vtkColor3ub> atomsColors;
  this->SetColorByAtom(atomsColors, atomTypes);
  this->SetColorByStructure(atomsColors, atomTypes, atom_ss,
                            ToColor3ubFromHex3(0xFF0080),
                            ToColor3ubFromHex3(0xFFC800));

  std::vector<vtkColor3ub> colors;
  std::vector<std::pair<vtkVector3f, bool> > borderPoints[2];

   // Need this for radius / color lookups
  vtkNew<vtkPeriodicTable> pTab;

  for (int i = 0; i < input->GetNumberOfPoints(); i++)
    {
    vtkStdString type = atomTypes->GetValue(i);
    unsigned short atomicNum = static_cast<unsigned short>(atomType->GetValue(i) + 1);

    if (ishetatm->GetValue(i) && this->DrawSmallMoleculesAsSpheres)
      {
      if (type != "O")
        {
        CreateAtomAsSphere(strand.GetPointer(), pointsColors.GetPointer(),
                           input->GetPoint(i),
                           ToColor3ubFromColor3f(pTab->GetDefaultRGBTuple(atomicNum)),
                           pTab->GetVDWRadius(atomicNum), 1.f);
        }
      }
    else if (type == "CA")
      {
      // Create a ribbon between 2 CA atoms passing through each O atoms found in-between
      double *xyz = input->GetPoint(i);
      unsigned char atomChain = chain->GetValue(i);
      vtkIdType atomResi = resi->GetValue(i);

      if (currentChain != atomChain || currentResi + 1 != atomResi)
        {
        this->CreateThinStrip(strand.GetPointer(), pointsColors.GetPointer(),
          strandPoints.GetPointer(), borderPoints[0], borderPoints[1], colors);
        borderPoints[0].clear();
        borderPoints[1].clear();
        colors.clear();
        hasPrevCO = false;
        }
      currentCA.Set(xyz[0], xyz[1], xyz[2]);
      currentChain = atomChain;
      currentResi = atomResi;
      ss = atom_ss->GetValue(i);
      colors.push_back(atomsColors[i]);
      }
    else if (type == "O")
      {
      // Insert a new step in the next ribbon
      double *xyz = input->GetPoint(i);
      vtkVector3f p(xyz[0], xyz[1], xyz[2]);
      p = (p - currentCA).Normalized() * ((ss == 'c') ? this->CoilWidth : this->HelixWidth);
      if (hasPrevCO && p.Dot(prevCO) < 0)
        {
        p = p * -1.f;
        }
      hasPrevCO = true;
      prevCO = p;
      bool isSheet = (ss == 's');
      borderPoints[0].push_back(
        std::pair<vtkVector3f, bool>(currentCA - prevCO, isSheet));
      borderPoints[1].push_back(
        std::pair<vtkVector3f, bool>(currentCA + prevCO, isSheet));
      }
    }

  // Create the last ribbon strip if needed
  this->CreateThinStrip(strand.GetPointer(), pointsColors.GetPointer(),
                        strandPoints.GetPointer(), borderPoints[0],
                        borderPoints[1], colors);

  strand->GetPointData()->SetScalars(pointsColors.GetPointer());

  // Compute the model normals
  vtkNew<vtkPolyDataNormals> pdnormals;
  pdnormals->SetInputData(strand.GetPointer());
  pdnormals->SetFeatureAngle(150.0);
  pdnormals->Update();

  output->ShallowCopy(pdnormals->GetOutput());

  return 1;
}


void vtkProteinRibbonFilter::SetColorByAtom(std::vector<vtkColor3ub>& colors,
                                            vtkStringArray* atomTypes)
{
  vtkNew<vtkPeriodicTable> pTab;
  unsigned int len = atomTypes->GetNumberOfValues();
  colors.resize(len);
  for (unsigned int i = 0; i < len; i++)
    {
    if(this->ElementColors.find(atomTypes->GetValue(i)) != this->ElementColors.end())
      {
      colors[i] = this->ElementColors[atomTypes->GetValue(i)];
      }
    else
      {
      colors[i] = vtkColor3ub(0xFFFFFF);
      }
    }
}


void vtkProteinRibbonFilter::SetColorByStructure(std::vector<vtkColor3ub>& colors,
                                                 vtkStringArray* atomTypes,
                                                 vtkUnsignedCharArray* ss,
                                                 const vtkColor3ub& helixColor,
                                                 const vtkColor3ub& sheetColor)
{
  unsigned int len = atomTypes->GetNumberOfValues();
  colors.resize(len);
  for (unsigned int i = 0; i < len; i++)
    {
    if (ss->GetValue(i) == 's')
      {
      colors[i] = sheetColor;
      }
    else if (ss->GetValue(i) == 'h')
      {
      colors[i] = helixColor;
      }
    }
}


void vtkProteinRibbonFilter::CreateAtomAsSphere(vtkPolyData* poly,
                                                vtkUnsignedCharArray *pointsColors,
                                                double *pos,
                                                const vtkColor3ub& color,
                                                float radius, float scale)
{
  // Create the sphere source at the atom size & position
  vtkNew<vtkSphereSource> sphereSource;
  sphereSource->SetPhiResolution(this->SphereResolution);
  sphereSource->SetThetaResolution(this->SphereResolution);
  sphereSource->SetCenter(pos);
  sphereSource->SetRadius(radius * scale);
  sphereSource->Update();

  // Extract polydata from sphere
  vtkPolyData *sphere = sphereSource->GetOutput();
  vtkPoints *spherePoints = sphere->GetPoints();
  vtkCellArray *spherePolys = sphere->GetPolys();

  vtkPoints *points = poly->GetPoints();
  // Get offset for the new point IDs that will be added to points
  vtkIdType pointOffset = points->GetNumberOfPoints();
  // Total number of new points
  vtkIdType numPoints = spherePoints->GetNumberOfPoints();
  vtkIdType numCellPoints,  *cellPoints;
  // Add new points
  for (vtkIdType i = 0; i < numPoints; ++i)
    {
    points->InsertNextPoint(spherePoints->GetPoint(i));
    for (int j = 0; j < 3; ++j)
      {
      pointsColors->InsertNextValue(color[j]);
      }
    }

  // Add new cells (polygons) that represent the sphere
  spherePolys->InitTraversal();
  while (spherePolys->GetNextCell(numCellPoints, cellPoints) != 0)
    {
    vtkIdType *newCellPoints = new vtkIdType[numCellPoints];
    for (vtkIdType i = 0; i < numCellPoints; ++i)
      {
      // The new point ids should be offset by the pointOffset above
      newCellPoints[i] = cellPoints[i] + pointOffset;
      }
    poly->InsertNextCell(VTK_TRIANGLE_STRIP, numCellPoints, newCellPoints);
    delete [] newCellPoints;
    }
}


void vtkProteinRibbonFilter::CreateThinStrip(vtkPolyData* poly,
                                             vtkUnsignedCharArray *pointsColors,
                                             vtkPoints* p,
                                             std::vector<std::pair<vtkVector3f, bool> >& p1,
                                             std::vector<std::pair<vtkVector3f, bool> >& p2,
                                             std::vector<vtkColor3ub> &colors)
{
  if (p1.size() < 2 || p2.size() < 2)
    {
    return;
    }

  // Get offset for the new point IDs that will be added to points
  vtkIdType pointOffset = p->GetNumberOfPoints();

  // Subdivide (smooth) the 2 ribbon borders
  std::vector<vtkVector3f>* points1 = Subdivide(p1, this->SubdivideFactor);
  std::vector<vtkVector3f>* points2 = Subdivide(p2, this->SubdivideFactor);

  int len = static_cast<int>(points1->size());

  // Insert smoothed ribbon borders points into the polydata
  for (int i = 0; i < len; i++)
    {
    p->InsertNextPoint((*points1)[i].GetData());
    p->InsertNextPoint((*points2)[i].GetData());

    vtkColor3ub color = colors[static_cast<int>(floor(0.5f + i /
                                     static_cast<float>(this->SubdivideFactor)))];
    for (int k = 0; k < 2; ++k)
      {
      for (int ci = 0; ci < 3; ++ci)
        {
        pointsColors->InsertNextValue(color[ci]);
        }
      }
    }
  delete points1;
  delete points2;

  // Fill in between the 2 ribbons borders with triangle strips
  vtkIdType connectivity[4];
  for (int i = 0, offset = pointOffset; i < len - 1; i++, offset += 2)
    {
    for (int j = 0; j < 4; j++)
      {
      connectivity[j] = offset + j;
      }
    poly->InsertNextCell(VTK_TRIANGLE_STRIP, 4 , connectivity);
    }
}


std::vector<vtkVector3f>* vtkProteinRibbonFilter::Subdivide(std::vector<std::pair<vtkVector3f, bool> >& p,
                                                            int div)
{
  std::vector<vtkVector3f>* ret = new std::vector<vtkVector3f>;
  std::vector<vtkVector3f> points;

  // Smoothing test
  points.push_back(p[0].first);
  for (int i = 1, lim = static_cast<int>(p.size()) - 1; i < lim; i++)
    {
    vtkVector3f& p1 = p[i].first;
    vtkVector3f& p2 = p[i+1].first;
    if (p[i].second)
      {
      points.push_back((p1 + p2) * 0.5f);
      }
    else
      {
      points.push_back(p1);
      }
    }
  points.push_back(p[p.size() - 1].first);

  // Catmull-Rom subdivision
  for (int i = -1, size = static_cast<int>(points.size()); i <= size - 3; i++)
    {
    vtkVector3f& p0 = points[(i == -1) ? 0 : i];
    vtkVector3f& p1 = points[i+1];
    vtkVector3f& p2 = points[i+2];
    vtkVector3f& p3 = points[(i == size - 3) ? size - 1 : i + 3];
    vtkVector3f v0 = (p2 - p0) * 0.5f;
    vtkVector3f v1 = (p3 - p1) * 0.5f;
    for (int j = 0; j < div; j++)
      {
      double t = 1.0 / div * j;
      double t2 = t * t;
      double x = p1.GetX() + t * v0.GetX()
        + t2 * (-3 * p1.GetX() + 3 * p2.GetX() - 2 * v0.GetX() - v1.GetX())
        + t2 * t * (2 * p1.GetX() - 2 * p2.GetX() + v0.GetX() + v1.GetX());
      double y = p1.GetY() + t * v0.GetY()
        + t2 * (-3 * p1.GetY() + 3 * p2.GetY() - 2 * v0.GetY() - v1.GetY())
        + t2 * t * (2 * p1.GetY() - 2 * p2.GetY() + v0.GetY() + v1.GetY());
      double z = p1.GetZ() + t * v0.GetZ()
        + t2 * (-3 * p1.GetZ() + 3 * p2.GetZ() - 2 * v0.GetZ() - v1.GetZ())
        + t2 * t * (2 * p1.GetZ() - 2 * p2.GetZ() + v0.GetZ() + v1.GetZ());
      ret->push_back(vtkVector3f(x, y, z));
      }
    }
   ret->push_back(points[points.size() - 1]);
   return ret;
}


void vtkProteinRibbonFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
