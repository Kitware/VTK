/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLUniformGridAMRWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLUniformGridAMRWriter.h"

#include "vtkAMRBox.h"
#include "vtkErrorCode.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMR.h"
#include "vtkSmartPointer.h"
#include "vtkUniformGrid.h"
#include "vtkXMLDataElement.h"

#include <assert.h>

vtkStandardNewMacro(vtkXMLUniformGridAMRWriter);
//----------------------------------------------------------------------------
vtkXMLUniformGridAMRWriter::vtkXMLUniformGridAMRWriter()
{
}

//----------------------------------------------------------------------------
vtkXMLUniformGridAMRWriter::~vtkXMLUniformGridAMRWriter()
{
}

//----------------------------------------------------------------------------
int vtkXMLUniformGridAMRWriter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUniformGridAMR");
  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLUniformGridAMRWriter::WriteComposite(vtkCompositeDataSet* compositeData,
    vtkXMLDataElement* parent, int &writerIdx)
{
  vtkUniformGridAMR* amr = vtkUniformGridAMR::SafeDownCast(compositeData);
  assert(amr != NULL);

  vtkOverlappingAMR* oamr = vtkOverlappingAMR::SafeDownCast(amr);

  // For vtkOverlappingAMR, we need to add additional meta-data to the XML.
  if (oamr)
    {
    const double *origin = oamr->GetOrigin();
    // I cannot decide what case to use. The other VTK-XML format used mixed
    // case for attributes, but the composite files are using all lower case
    // attributes. For consistency, I'm sticking with that.
    parent->SetVectorAttribute("origin", 3, origin);
    const char* gridDescription = "";
    switch (oamr->GetGridDescription())
      {
    case VTK_XY_PLANE:
      gridDescription = "XY";
      break;

    case VTK_YZ_PLANE:
      gridDescription = "YZ";
      break;

    case VTK_XZ_PLANE:
      gridDescription = "XZ";
      break;

    case VTK_XYZ_GRID:
    default:
      gridDescription = "XYZ";
      break;
      }
    parent->SetAttribute("grid_description", gridDescription);
    }

  unsigned int numLevels = amr->GetNumberOfLevels();

  // Iterate over each level.
  for (unsigned int level=0; level < numLevels; level++)
    {
    vtkSmartPointer<vtkXMLDataElement> block = vtkSmartPointer<vtkXMLDataElement>::New();
    block->SetName("Block");
    block->SetIntAttribute("level", level);
    if (oamr)
      {
      // save spacing for each level.
      double spacing[3];
      oamr->GetSpacing(level, spacing);
      block->SetVectorAttribute("spacing", 3, spacing);

      // we no longer save the refinement ratios since those can be deduced from
      // the spacing very easily.
      }

    unsigned int numDS = amr->GetNumberOfDataSets(level);
    for (unsigned int cc=0; cc < numDS; cc++)
      {
      vtkUniformGrid* ug = amr->GetDataSet(level, cc);

      vtkSmartPointer<vtkXMLDataElement> datasetXML =
        vtkSmartPointer<vtkXMLDataElement>::New();
      datasetXML->SetName("DataSet");
      datasetXML->SetIntAttribute("index", cc);
      if (oamr)
        {
        // AMRBox meta-data is available only for vtkOverlappingAMR. Also this
        // meta-data is expected to be consistent (and available) on all
        // processes so we don't have to worry about missing amr-box
        // information.
        const vtkAMRBox& amrBox = oamr->GetAMRBox(level, cc);

        int box_buffer[6];
        box_buffer[0] = amrBox.GetLoCorner()[0];
        box_buffer[1] = amrBox.GetHiCorner()[0];
        box_buffer[2] = amrBox.GetLoCorner()[1];
        box_buffer[3] = amrBox.GetHiCorner()[1];
        box_buffer[4] = amrBox.GetLoCorner()[2];
        box_buffer[5] = amrBox.GetHiCorner()[2];
        // Don't use vtkAMRBox.Serialize() since it writes the box in a different
        // order than we wrote the box in traditionally. The expected order is
        // (xLo, xHi, yLo, yHi, zLo, zHi).
        datasetXML->SetVectorAttribute("amr_box", 6, box_buffer);
        }

      vtkStdString fileName = this->CreatePieceFileName(writerIdx);
      if (fileName != "")
        {
        // if fileName is empty, it implies that no file is written out for this
        // node, so don't add a filename attribute for it.
        datasetXML->SetAttribute("file", fileName);
        }
      block->AddNestedElement(datasetXML);

      // if this->WriteNonCompositeData() returns 0, it doesn't meant it's an
      // error, it just means that it didn't write a file for the current node.
      this->WriteNonCompositeData(ug, datasetXML, writerIdx, fileName.c_str());

      if (this->GetErrorCode() != vtkErrorCode::NoError)
        {
        return 0;
        }
      }
    parent->AddNestedElement(block);
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLUniformGridAMRWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
