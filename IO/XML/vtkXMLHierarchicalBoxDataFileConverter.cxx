/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLHierarchicalBoxDataFileConverter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLHierarchicalBoxDataFileConverter.h"

#include "vtkBoundingBox.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLDataParser.h"
#include "vtkXMLImageDataReader.h"
#include "vtkStructuredData.h"
#include "vtkExecutive.h"
#include "vtkInformation.h"

#include <string>
#include <set>
#include <map>
#include <vector>
#include <cassert>

vtkStandardNewMacro(vtkXMLHierarchicalBoxDataFileConverter);
//----------------------------------------------------------------------------
vtkXMLHierarchicalBoxDataFileConverter::vtkXMLHierarchicalBoxDataFileConverter()
{
  this->InputFileName = 0;
  this->OutputFileName = 0;
  this->FilePath = 0;
}

//----------------------------------------------------------------------------
vtkXMLHierarchicalBoxDataFileConverter::~vtkXMLHierarchicalBoxDataFileConverter()
{
  this->SetInputFileName(NULL);
  this->SetOutputFileName(NULL);
  this->SetFilePath(NULL);
}

//----------------------------------------------------------------------------
bool vtkXMLHierarchicalBoxDataFileConverter::Convert()
{
  if (!this->InputFileName)
    {
    vtkErrorMacro("Missing InputFileName.");
    return false;
    }

  if (!this->OutputFileName)
    {
    vtkErrorMacro("Missing OutputFileName.");
    return false;
    }

  vtkSmartPointer<vtkXMLDataElement> dom;
  dom.TakeReference(this->ParseXML(this->InputFileName));
  if (!dom)
    {
    return false;
    }

  // Ensure this file we can convert.
  if (dom->GetName() == NULL ||
    strcmp(dom->GetName(), "VTKFile") != 0 ||
    dom->GetAttribute("type") == NULL ||
    strcmp(dom->GetAttribute("type"), "vtkHierarchicalBoxDataSet") != 0 ||
    dom->GetAttribute("version") == NULL ||
    strcmp(dom->GetAttribute("version"), "1.0") != 0)
    {
    vtkErrorMacro("Cannot convert the input file: " << this->InputFileName);
    return false;
    }

  dom->SetAttribute("version", "1.1");
  dom->SetAttribute("type", "vtkOverlappingAMR");

  // locate primary element.
  vtkXMLDataElement* ePrimary =
    dom->FindNestedElementWithName("vtkHierarchicalBoxDataSet");
  if (!ePrimary)
    {
    vtkErrorMacro("Failed to locate primary element.");
    return false;
    }

  ePrimary->SetName("vtkOverlappingAMR");

  // Find the path to this file in case the internal files are
  // specified as relative paths.
  std::string filePath = this->InputFileName;
  std::string::size_type pos = filePath.find_last_of("/\\");
  if (pos != filePath.npos)
    {
    filePath = filePath.substr(0, pos);
    }
  else
    {
    filePath = "";
    }
  this->SetFilePath(filePath.c_str());

  // We need origin for level 0, and spacing for all levels.
  double origin[3];
  double *spacing = NULL;

  int gridDescription = this->GetOriginAndSpacing(
    ePrimary, origin, spacing);
  if (gridDescription < VTK_XY_PLANE || gridDescription > VTK_XYZ_GRID)
    {
    delete [] spacing;
    vtkErrorMacro("Failed to determine origin/spacing/grid description.");
    return false;
    }

//  cout << "Origin: " << origin[0] <<", " << origin[1] << ", " << origin[2]
//    << endl;
//  cout << "Spacing: " << spacing[0] << ", " << spacing[1] << ", " << spacing[2]
//    << endl;

  const char* grid_description = "XYZ";
  switch (gridDescription)
    {
  case VTK_XY_PLANE:
    grid_description = "XY";
    break;
  case VTK_XZ_PLANE:
    grid_description = "XZ";
    break;
  case VTK_YZ_PLANE:
    grid_description = "YZ";
    break;
    }

  ePrimary->SetAttribute("grid_description", grid_description);
  ePrimary->SetVectorAttribute("origin", 3, origin);

  // Now iterate over all "<Block>" elements and update them.
  for (int cc=0; cc < ePrimary->GetNumberOfNestedElements(); cc++)
    {
    int level = 0;
    vtkXMLDataElement* child = ePrimary->GetNestedElement(cc);
    if (child && child->GetName() &&
      strcmp(child->GetName(), "Block") == 0 &&
      child->GetScalarAttribute("level", level) &&
      level >= 0)
      {
      }
    else
      {
      continue;
      }
    child->SetVectorAttribute("spacing", 3, &spacing[3*level]);
    child->RemoveAttribute("refinement_ratio");
    }
  delete [] spacing;

  // now save the xml out.
  dom->PrintXML(this->OutputFileName);
  return true;
}

//----------------------------------------------------------------------------
vtkXMLDataElement* vtkXMLHierarchicalBoxDataFileConverter::ParseXML(
  const char* fname)
{
  assert(fname);

  vtkNew<vtkXMLDataParser> parser;
  parser->SetFileName(fname);
  if (!parser->Parse())
    {
    vtkErrorMacro("Failed to parse input XML: " << fname);
    return NULL;
    }

  vtkXMLDataElement* element = parser->GetRootElement();
  element->Register(this);
  return element;
}

//----------------------------------------------------------------------------
int vtkXMLHierarchicalBoxDataFileConverter::GetOriginAndSpacing(
  vtkXMLDataElement* ePrimary, double origin[3], double* &spacing)
{
  // Build list of filenames for all levels.
  std::map<int, std::set<std::string> > filenames;

  for (int cc=0; cc < ePrimary->GetNumberOfNestedElements(); cc++)
    {
    int level = 0;

    vtkXMLDataElement* child = ePrimary->GetNestedElement(cc);
    if (child && child->GetName() &&
      strcmp(child->GetName(), "Block") == 0 &&
      child->GetScalarAttribute("level", level) &&
      level >= 0)
      {
      }
    else
      {
      continue;
      }

    for (int kk=0; kk < child->GetNumberOfNestedElements(); kk++)
      {
      vtkXMLDataElement* dsElement = child->GetNestedElement(cc);
      if (dsElement && dsElement->GetName() &&
        strcmp(dsElement->GetName(), "DataSet") == 0 &&
        dsElement->GetAttribute("file") != 0)
        {
        std::string file = dsElement->GetAttribute("file");
        if (file.c_str()[0] != '/' && file.c_str()[1] != ':')
          {
          std::string prefix = this->FilePath;
          if (prefix.length())
            {
            prefix += "/";
            }
          file = prefix + file;
          }
        filenames[level].insert(file);
        }
      }
    }

  vtkBoundingBox bbox;
  int gridDescription = VTK_UNCHANGED;
  spacing = new double[3* filenames.size() + 1];
  memset(spacing, 0, (3*filenames.size() + 1)*sizeof(double));

  // Now read all the datasets at level 0.
  for (std::set<std::string>::iterator iter = filenames[0].begin();
    iter != filenames[0].end(); ++iter)
    {
    vtkNew<vtkXMLImageDataReader> imageReader;
    imageReader->SetFileName((*iter).c_str());
    imageReader->Update();

    vtkImageData* image = imageReader->GetOutput();
    if (image && vtkMath::AreBoundsInitialized(image->GetBounds()))
      {
      if (!bbox.IsValid())
        {
        gridDescription = vtkStructuredData::GetDataDescription(
          image->GetDimensions());
        }
      bbox.AddBounds(image->GetBounds());
      }
    }

  if (bbox.IsValid())
    {
    bbox.GetMinPoint(origin[0], origin[1], origin[2]);
    }

  // Read 1 dataset from each level to get information about spacing.
  for (std::map<int, std::set<std::string> >::iterator iter =
    filenames.begin(); iter != filenames.end(); ++iter)
    {
    if (iter->second.size() == 0)
      {
      continue;
      }

    std::string filename = (*iter->second.begin());
    vtkNew<vtkXMLImageDataReader> imageReader;
    imageReader->SetFileName(filename.c_str());
    imageReader->UpdateInformation();
    vtkInformation* outInfo =
      imageReader->GetExecutive()->GetOutputInformation(0);
    if (outInfo->Has(vtkDataObject::SPACING()))
      {
      assert(outInfo->Length(vtkDataObject::SPACING()) == 3);
      outInfo->Get(vtkDataObject::SPACING(), &spacing[3*iter->first]);
      }
    }

  return gridDescription;
}

//----------------------------------------------------------------------------
void vtkXMLHierarchicalBoxDataFileConverter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "InputFileName: "
    << (this->InputFileName? this->InputFileName : "(none)") << endl;
  os << indent << "OutputFileName: "
    << (this->OutputFileName?  this->OutputFileName : "(none)")<< endl;
}
