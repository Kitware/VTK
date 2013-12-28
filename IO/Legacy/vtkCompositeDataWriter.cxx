/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeDataWriter.h"

#include "vtkDoubleArray.h"
#include "vtkGenericDataObjectWriter.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkInformation.h"
#include "vtkIntArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkNew.h"
#include "vtkNonOverlappingAMR.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMR.h"
#include "vtkUniformGrid.h"
#include "vtkAMRBox.h"
#include "vtkAMRInformation.h"

#if !defined(_WIN32) || defined(__CYGWIN__)
# include <unistd.h> /* unlink */
#else
# include <io.h> /* unlink */
#endif

vtkStandardNewMacro(vtkCompositeDataWriter);
//----------------------------------------------------------------------------
vtkCompositeDataWriter::vtkCompositeDataWriter()
{
}

//----------------------------------------------------------------------------
vtkCompositeDataWriter::~vtkCompositeDataWriter()
{
}

//----------------------------------------------------------------------------
vtkCompositeDataSet* vtkCompositeDataWriter::GetInput()
{
  return this->GetInput(0);
}

//----------------------------------------------------------------------------
vtkCompositeDataSet* vtkCompositeDataWriter::GetInput(int port)
{
  return vtkCompositeDataSet::SafeDownCast(this->GetInputDataObject(port, 0));
}

//----------------------------------------------------------------------------
int vtkCompositeDataWriter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkCompositeDataWriter::WriteData()
{
  ostream *fp;
  vtkCompositeDataSet *input = this->GetInput();

  vtkDebugMacro(<<"Writing vtk composite data...");
  if ( !(fp=this->OpenVTKFile()) || !this->WriteHeader(fp) )
    {
    if (fp)
      {
      if(this->FileName)
        {
        vtkErrorMacro(
          "Ran out of disk space; deleting file: " << this->FileName);
        this->CloseVTKFile(fp);
        unlink(this->FileName);
        }
      else
        {
        this->CloseVTKFile(fp);
        vtkErrorMacro("Could not read memory header. ");
        }
      }
    return;
    }

  vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::SafeDownCast(input);
  vtkHierarchicalBoxDataSet* hb =
    vtkHierarchicalBoxDataSet::SafeDownCast(input);
  vtkOverlappingAMR* oamr = vtkOverlappingAMR::SafeDownCast(input);
  vtkNonOverlappingAMR* noamr = vtkNonOverlappingAMR::SafeDownCast(input);
  vtkMultiPieceDataSet* mp = vtkMultiPieceDataSet::SafeDownCast(input);
  if (mb)
    {
    *fp << "DATASET MULTIBLOCK\n";
    if (!this->WriteCompositeData(fp, mb))
      {
      vtkErrorMacro("Error writing multiblock dataset.");
      }
    }
  else if (hb)
    {
    *fp << "DATASET HIERARCHICAL_BOX\n";
    if (!this->WriteCompositeData(fp, hb))
      {
      vtkErrorMacro("Error writing hierarchical-box dataset.");
      }
    }
  else if (oamr)
    {
    *fp << "DATASET OVERLAPPING_AMR\n";
    if (!this->WriteCompositeData(fp, oamr))
      {
      vtkErrorMacro("Error writing overlapping amr dataset.");
      }
    }
  else if (noamr)
    {
    *fp << "DATASET NON_OVERLAPPING_AMR\n";
    if (!this->WriteCompositeData(fp, noamr))
      {
      vtkErrorMacro("Error writing non-overlapping amr dataset.");
      }
    }
  else if (mp)
    {
    *fp << "DATASET MULTIPIECE\n";
    if (!this->WriteCompositeData(fp, mp))
      {
      vtkErrorMacro("Error writing multi-piece dataset.");
      }
    }
  else
    {
    vtkErrorMacro("Unsupported input type: " << input->GetClassName());
    }

  this->CloseVTKFile(fp);
}

//----------------------------------------------------------------------------
bool vtkCompositeDataWriter::WriteCompositeData(ostream* fp,
  vtkMultiBlockDataSet* mb)
{
  *fp << "CHILDREN " << mb->GetNumberOfBlocks() << "\n";
  for (unsigned int cc=0; cc < mb->GetNumberOfBlocks(); cc++)
    {
    vtkDataObject* child = mb->GetBlock(cc);
    *fp << "CHILD " << (child? child->GetDataObjectType() : -1);
    // add name if present.
    if (mb->HasMetaData(cc) &&
      mb->GetMetaData(cc)->Has(vtkCompositeDataSet::NAME()))
      {
      *fp << " [" << mb->GetMetaData(cc)->Get(vtkCompositeDataSet::NAME())
          << "]";
      }
    *fp << "\n";
    if (child)
      {
      if (!this->WriteBlock(fp, child))
        {
        return false;
        }
      }
    *fp << "ENDCHILD\n";
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkCompositeDataWriter::WriteCompositeData(ostream* fp,
  vtkMultiPieceDataSet* mp)
{
  *fp << "CHILDREN " << mp->GetNumberOfPieces() << "\n";
  for (unsigned int cc=0; cc < mp->GetNumberOfPieces(); cc++)
    {
    vtkDataObject* child = mp->GetPieceAsDataObject(cc);
    *fp << "CHILD " << (child? child->GetDataObjectType() : -1);
    // add name if present.
    if (mp->HasMetaData(cc) &&
      mp->GetMetaData(cc)->Has(vtkCompositeDataSet::NAME()))
      {
      *fp << " [" << mp->GetMetaData(cc)->Get(vtkCompositeDataSet::NAME())
          << "]";
      }
    *fp << "\n";

    if (child)
      {
      if (!this->WriteBlock(fp, child))
        {
        return false;
        }
      }
    *fp << "ENDCHILD\n";
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkCompositeDataWriter::WriteCompositeData(ostream* fp,
  vtkHierarchicalBoxDataSet* hb)
{
  (void)fp;
  (void)hb;
  vtkErrorMacro("This isn't supported yet.");
  return false;
}

//----------------------------------------------------------------------------
bool vtkCompositeDataWriter::WriteCompositeData(
  ostream* fp, vtkOverlappingAMR* oamr)
{
  vtkAMRInformation* amrInfo = oamr->GetAMRInfo();

  *fp << "GRID_DESCRIPTION " << amrInfo->GetGridDescription() << "\n";

  const double* origin = oamr->GetOrigin();
  *fp << "ORIGIN " << origin[0] << " " << origin[1] << " " << origin[2] << "\n";

  unsigned int num_levels = oamr->GetNumberOfLevels();
  // we'll dump out all level information and then the individual blocks.
  *fp << "LEVELS " << num_levels << "\n";
  for (unsigned int level=0; level < num_levels; level++)
    {
    // <num datasets> <spacing x> <spacing y> <spacing z>
    double spacing[3];
    amrInfo->GetSpacing(level, spacing);

    *fp << oamr->GetNumberOfDataSets(level)
        << " " << spacing[0]
        << " " << spacing[1]
        << " " << spacing[2]
        << "\n";
    }

  // now dump the amr boxes, if any.
  // Information about amrboxes can be "too much". So we compact it in
  // vtkDataArray subclasses to ensure that it can be written as binary data
  // with correct swapping, as needed.
  vtkNew<vtkIntArray> idata;
  // box.LoCorner[3], box.HiCorner[3]
  idata->SetName("IntMetaData");
  idata->SetNumberOfComponents(6);
  idata->SetNumberOfTuples(amrInfo->GetTotalNumberOfBlocks());
  unsigned int metadata_index=0;
  for (unsigned int level=0; level < num_levels; level++)
    {
    unsigned int num_datasets = oamr->GetNumberOfDataSets(level);
    for (unsigned int index=0; index < num_datasets; index++, metadata_index++)
      {
      const vtkAMRBox& box = oamr->GetAMRBox(level,index);
      int tuple[6];
      box.Serialize(tuple);
      idata->SetTupleValue(metadata_index, tuple);
      }
    }
  *fp << "AMRBOXES "
      << idata->GetNumberOfTuples() << " " << idata->GetNumberOfComponents() << "\n";
  this->WriteArray(fp, idata->GetDataType(), idata.GetPointer(),
    "", idata->GetNumberOfTuples(), idata->GetNumberOfComponents());

  // now dump the real data, if any.
  metadata_index=0;
  for (unsigned int level=0; level < num_levels; level++)
    {
    unsigned int num_datasets = oamr->GetNumberOfDataSets(level);
    for (unsigned int index=0; index < num_datasets; index++, metadata_index++)
      {
      vtkUniformGrid* dataset = oamr->GetDataSet(level, index);
      if (dataset)
        {
        *fp << "CHILD " << level << " " << index << "\n";
        // since we cannot write vtkUniformGrid's, we create a vtkImageData and
        // write it.
        vtkNew<vtkImageData> image;
        image->ShallowCopy(dataset);
        if (!this->WriteBlock(fp, image.GetPointer()))
          {
          return false;
          }
        *fp << "ENDCHILD\n";
        }
      }
    }
  return true;
}

//----------------------------------------------------------------------------
bool vtkCompositeDataWriter::WriteCompositeData(ostream* fp,
  vtkNonOverlappingAMR* hb)
{
  (void)fp;
  (void)hb;
  vtkErrorMacro("This isn't supported yet.");
  return false;
}

//----------------------------------------------------------------------------
bool vtkCompositeDataWriter::WriteBlock(ostream* fp, vtkDataObject* block)
{
  bool success = false;
  vtkGenericDataObjectWriter* writer = vtkGenericDataObjectWriter::New();
  writer->WriteToOutputStringOn();
  writer->SetFileType(this->FileType);
  writer->SetInputData(block);
  if (writer->Write())
    {
    fp->write(
      reinterpret_cast<const char*>(writer->GetBinaryOutputString()),
      writer->GetOutputStringLength());
    success = true;
    }
  writer->Delete();
  return success;
}

//----------------------------------------------------------------------------
void vtkCompositeDataWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
