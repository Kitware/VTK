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


std::map<std::string, unsigned int> ElementColors;

vtkStandardNewMacro(vtkProteinRibbonFilter)

vtkProteinRibbonFilter::vtkProteinRibbonFilter()
{
  this->CoilWidth = 0.3f;
  this->HelixWidth = 1.3f;
  this->SphereResolution = 60;
  this->SubdivideFactor = 20;

  ElementColors["H"]  = 0xCCCCCC;
  ElementColors["C"]  = 0xAAAAAA;
  ElementColors["O"]  = 0xCC0000;
  ElementColors["N"]  = 0x0000CC;
  ElementColors["S"]  = 0xCCCC00;
  ElementColors["P"]  = 0x6622CC;
  ElementColors["F"]  = 0x00CC00;
  ElementColors["CL"] = 0x00CC00;
  ElementColors["BR"] = 0x882200;
  ElementColors["I"]  = 0x6600AA;
  ElementColors["FE"] = 0xCC6600;
  ElementColors["CA"] = 0xDDDDDD;
}

// Extract the R, G & B values from a compact unsigned int color
#define GETRVALUE(_c_) (((_c_) & 0xFF0000) >> 16)
#define GETGVALUE(_c_) (((_c_) & 0x00FF00) >> 8)
#define GETBVALUE(_c_) ((_c_) & 0x0000FF)
// Transform a RGB color from a 3 float array into a compact unsigned int
#define F2UICOLOR(_c_) ((unsigned int(_c_[0] * 255) << 16) | (unsigned int(_c_[1] * 255) << 8) | (unsigned int(_c_[2] * 255)))

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

  if (!atomTypes)
    {
    vtkErrorMacro(<< "Atom Type String Array Required");
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

  char currentChain = 0;
  char ss = 0;
  vtkIdType currentResi = 0;
  vtkVector3f currentCA;
  vtkVector3f prevCO;
  bool hasPrevCO = false;

  vtkNew<vtkPoints> strandPoints;
  vtkNew<vtkPolyData> strand;
  strand->Allocate();
  strand->SetPoints(strandPoints.GetPointer());

  vtkNew<vtkUnsignedCharArray> faceColors;
  faceColors->SetName("RGB");
  faceColors->SetNumberOfComponents(3);

  // Initialize colors per point/atom
  std::vector<unsigned int> atomsColors;
  SetColorByAtom(atomsColors, atomTypes);
  SetColorByStructure(atomsColors, atomTypes, atom_ss);

  std::vector<unsigned int> colors;
  std::vector<std::pair<vtkVector3f, bool> > borderPoints[2];

   // Need this for radius / color lookups
  vtkNew<vtkPeriodicTable> pTab;

  for (int i = 0; i < input->GetNumberOfPoints(); i++)
    {
    vtkStdString type = atomTypes->GetValue(i);
    if (ishetatm->GetValue(i))
      {
      if (type == "O")
        {
        continue;
        }

      unsigned short atomicNum = pTab->GetAtomicNumber(type);
      float col[4];
      pTab->GetDefaultRGBTuple(atomicNum, col);
      CreateAtomAsSphere(strand.GetPointer(), faceColors.GetPointer(),
        input->GetPoint(i), F2UICOLOR(col), pTab->GetVDWRadius(atomicNum), 1.f);
      }
    else if (type == "CA")
      {
      // Create a ribbon between 2 CA atoms passing through each O atoms found in-between
      double *xyz = input->GetPoint(i);
      char atomChain = chain->GetValue(i);
      vtkIdType atomResi = resi->GetValue(i);

      if (currentChain != atomChain || currentResi + 1 != atomResi)
        {
        this->CreateThinStrip(strand.GetPointer(), faceColors.GetPointer(),
          strandPoints.GetPointer(), borderPoints[0], borderPoints[1], colors);
        borderPoints[0].clear();
        borderPoints[1].clear();
        colors.clear();
        ss = 0;
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
  CreateThinStrip(strand.GetPointer(), faceColors.GetPointer(),
    strandPoints.GetPointer(), borderPoints[0], borderPoints[1], colors);

  strand->GetCellData()->SetScalars(faceColors.GetPointer());

  // Compute the model normals
  vtkNew<vtkPolyDataNormals> pdnormals;
  pdnormals->SetInputData(strand.GetPointer());
  pdnormals->SetFeatureAngle(60.0);
  pdnormals->Update();

  output->ShallowCopy(pdnormals->GetOutput());

  return 1;
}


void vtkProteinRibbonFilter::SetColorByAtom(std::vector<unsigned int>& colors, vtkStringArray* atomTypes)
{
  vtkNew<vtkPeriodicTable> pTab;
  unsigned int len = atomTypes->GetNumberOfValues();
  colors.resize(len);
  for (unsigned int i = 0; i < len; i++)
    {
    /*unsigned short atomicNum = pTab->GetAtomicNumber(atomTypes->GetValue(i));
    float col[4];
    pTab->GetDefaultRGBTuple(atomicNum, col);
    colors[i] = F2UICOLOR(col);*/
    colors[i] = ElementColors[atomTypes->GetValue(i)];
    }
}


void vtkProteinRibbonFilter::SetColorByStructure(std::vector<unsigned int>& colors,
                                                 vtkStringArray* atomTypes,
                                                 vtkUnsignedCharArray* ss,
                                                 unsigned int helixColor,
                                                 unsigned int sheetColor)
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
                                                vtkUnsignedCharArray *faceColors,
                                                double *pos,
                                                unsigned int color, float radius, float scale)
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
    faceColors->InsertNextTuple(vtkVector3f(GETRVALUE(color), GETGVALUE(color), GETBVALUE(color)).GetData());
    delete [] newCellPoints;
    }
}


void vtkProteinRibbonFilter::CreateThinStrip(vtkPolyData* poly,
                                             vtkUnsignedCharArray *faceColors,
                                             vtkPoints* p,
                                             std::vector<std::pair<vtkVector3f, bool> >& p1,
                                             std::vector<std::pair<vtkVector3f, bool> >& p2,
                                             std::vector<unsigned int> &colors)
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

  const int len = points1->size();

  // Insert smoothed ribbon borders points into the polydata
  for (int i = 0; i < len; i++)
    {
    p->InsertNextPoint((*points1)[i].GetData());
    p->InsertNextPoint((*points2)[i].GetData());
    }
  delete points1;
  delete points2;

  // Fill in between the 2 ribbons borders with a single triangle strip
  vtkIdType* connectivity = new vtkIdType[4 * (len - 1)];
  for (int i = 0, j = 0; i < len - 1; i++, j += 4)
    {
    connectivity[j + 0] = pointOffset + 2 * i;
    connectivity[j + 1] = pointOffset + 2 * i + 1;
    connectivity[j + 2] = pointOffset + 2 * i + 2;
    connectivity[j + 3] = pointOffset + 2 * i + 3;

    unsigned int color = colors[floor(0.5f + i / (float)this->SubdivideFactor)];
    vtkVector3f facecolor(GETRVALUE(color), GETGVALUE(color), GETBVALUE(color));
    faceColors->InsertNextTuple(facecolor.GetData());
    faceColors->InsertNextTuple(facecolor.GetData());
    faceColors->InsertNextTuple(facecolor.GetData());
    faceColors->InsertNextTuple(facecolor.GetData());
    }
  poly->InsertNextCell(VTK_TRIANGLE_STRIP, 4 * (len - 1), connectivity);
}


std::vector<vtkVector3f>* vtkProteinRibbonFilter::Subdivide(std::vector<std::pair<vtkVector3f, bool> >& p,
                                                            int div)
{
  std::vector<vtkVector3f>* ret = new std::vector<vtkVector3f>;
  std::vector<vtkVector3f> points;

   // Smoothing test
  points.push_back(p[0].first);
  for (int i = 1, lim = p.size() - 1; i < lim; i++)
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
  for (int i = -1, size = points.size(); i <= size - 3; i++)
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
