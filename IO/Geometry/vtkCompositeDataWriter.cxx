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

#include "vtkAMRBox.h"
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
    *fp << "CHILD " << (child? child->GetDataObjectType() : -1) << "\n";
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
    *fp << "CHILD " << (child? child->GetDataObjectType() : -1) << "\n";
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
  unsigned int num_levels = oamr->GetNumberOfLevels();
  unsigned int total_datasets = 0;
  // we'll dump out all level information and then the individual blocks.
  *fp << "LEVELS " << num_levels;
  for (unsigned int cc=0; cc < num_levels; cc++)
    {
    *fp << " " << oamr->GetNumberOfDataSets(cc);
    total_datasets += oamr->GetNumberOfDataSets(cc);
    }
  *fp << "\n";

  // now dump the amr boxes and real data, if any.

  // Information about amrboxes can be "too much". So we compact it in
  // vtkDataArray subclasses to ensure that it can be written as binary data
  // with correct swapping, as needed.

  vtkNew<vtkDoubleArray> ddata;
  ddata->SetName("DoubleMetaData");
  // box.X0[3], box.DX[3]
  ddata->SetNumberOfComponents(6);
  ddata->SetNumberOfTuples(total_datasets);

  vtkNew<vtkIntArray> idata;
  // box.Dimension[1], box.ProcessId[1], box.GridDiscription[1],
  // box.LoCorner[3], box.HiCorner[3], box.RealExtent[6],
  idata->SetName("IntMetaData");
  idata->SetNumberOfComponents(15);
  idata->SetNumberOfTuples(total_datasets);

  unsigned int metadata_index=0;
  for (unsigned int level=0; level < num_levels; level++)
    {
    unsigned int num_datasets = oamr->GetNumberOfDataSets(level);
    for (unsigned int index=0; index < num_datasets; index++, metadata_index++)
      {
      vtkAMRBox box;
      vtkUniformGrid* dataset = oamr->GetDataSet(level, index, box);
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
      memcpy(ddata->GetPointer(6*metadata_index), box.GetDataSetOrigin(),
        3*sizeof(double));
      memcpy(ddata->GetPointer(6*metadata_index+3), box.GetGridSpacing(),
        3*sizeof(double));

      idata->SetValue(15*metadata_index + 0, box.GetDimensionality());
      idata->SetValue(15*metadata_index + 1, box.GetProcessId());
      idata->SetValue(15*metadata_index + 2, box.GetGridDescription());
      memcpy(idata->GetPointer(15*metadata_index + 3), box.GetLoCorner(), 3*sizeof(int));
      memcpy(idata->GetPointer(15*metadata_index + 6), box.GetHiCorner(), 3*sizeof(int));
      memcpy(idata->GetPointer(15*metadata_index + 9), box.GetRealExtent(), 6*sizeof(int));
      }
    }
  *fp << "METADATA\n";

  char format[1024];
  sprintf(format,"%s %%s %d\nLOOKUP_TABLE %s\n", "DoubleMetaData", 6, "default");
  this->WriteArray(fp, ddata->GetDataType(), ddata.GetPointer(), format, total_datasets, 6);
  
  sprintf(format,"%s %%s %d\nLOOKUP_TABLE %s\n", "IntMetaData", 6, "default");
  this->WriteArray(fp, idata->GetDataType(), idata.GetPointer(), format,
    total_datasets, 15);
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
