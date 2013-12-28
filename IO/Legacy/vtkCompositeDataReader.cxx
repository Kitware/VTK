/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeDataReader.h"

#include "vtkAMRBox.h"
#include "vtkAMRInformation.h"
#include "vtkDataObjectTypes.h"
#include "vtkDoubleArray.h"
#include "vtkGenericDataObjectReader.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkNonOverlappingAMR.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMR.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUniformGrid.h"

#include <vtksys/RegularExpression.hxx>
#include <vtksys/SystemTools.hxx>
#include <vtksys/ios/sstream>

#include <vector>

vtkStandardNewMacro(vtkCompositeDataReader);
//----------------------------------------------------------------------------
vtkCompositeDataReader::vtkCompositeDataReader()
{
}

//----------------------------------------------------------------------------
vtkCompositeDataReader::~vtkCompositeDataReader()
{
}

//----------------------------------------------------------------------------
vtkCompositeDataSet* vtkCompositeDataReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkCompositeDataSet* vtkCompositeDataReader::GetOutput(int idx)
{
  return vtkCompositeDataSet::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
void vtkCompositeDataReader::SetOutput(vtkCompositeDataSet *output)
{
  this->GetExecutive()->SetOutputData(0, output);
}

//----------------------------------------------------------------------------
int vtkCompositeDataReader::RequestUpdateExtent(
  vtkInformation *, vtkInformationVector **, vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  int piece, numPieces, ghostLevel;

  piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ghostLevel = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  // make sure piece is valid
  if (piece < 0 || piece >= numPieces)
    {
    return 1;
    }

  if (ghostLevel < 0)
    {
    return 1;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkCompositeDataReader::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkCompositeDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkCompositeDataReader::ProcessRequest(vtkInformation* request,
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // generate the data
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
    {
    return this->RequestDataObject(request, inputVector, outputVector);
    }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkCompositeDataReader::RequestDataObject(vtkInformation *,
  vtkInformationVector **, vtkInformationVector *outputVector)
{
  int output_type = this->ReadOutputType();
  if (output_type < 0)
    {
    vtkErrorMacro("Failed to read data-type.");
    return 0;
    }

  vtkDataObject* output = vtkDataObject::GetData(outputVector, 0);
  if (!output ||
    !output->IsA(vtkDataObjectTypes::GetClassNameFromTypeId(output_type)))
    {
    output = vtkDataObjectTypes::NewDataObject(output_type);
    outputVector->GetInformationObject(0)->Set(
      vtkDataObject::DATA_OBJECT(),
      output);
    output->FastDelete();
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkCompositeDataReader::ReadOutputType()
{
  char line[256];
  if (!this->OpenVTKFile() || !this->ReadHeader())
    {
    return -1;
    }

  // Determine dataset type
  //
  if (!this->ReadString(line))
    {
    vtkDebugMacro(<< "Premature EOF reading dataset keyword");
    return -1;
    }

  if (strncmp(this->LowerCase(line),"dataset",static_cast<unsigned long>(7)) == 0)
    {
    // See iftype is recognized.
    //
    if (!this->ReadString(line))
      {
      vtkDebugMacro(<< "Premature EOF reading type");
      this->CloseVTKFile ();
      return -1;
      }
    this->CloseVTKFile();

    if (strncmp(this->LowerCase(line), "multiblock", strlen("multiblock")) == 0)
      {
      return VTK_MULTIBLOCK_DATA_SET;
      }
    if (strncmp(this->LowerCase(line), "multipiece", strlen("multipiece")) == 0)
      {
      return VTK_MULTIPIECE_DATA_SET;
      }
    if (strncmp(this->LowerCase(line), "overlapping_amr", strlen("overlapping_amr")) == 0)
      {
      return VTK_OVERLAPPING_AMR;
      }
    if (strncmp(this->LowerCase(line), "non_overlapping_amr", strlen("non_overlapping_amr")) == 0)
      {
      return VTK_NON_OVERLAPPING_AMR;
      }
    if (strncmp(this->LowerCase(line), "hierarchical_box",
        strlen("hierarchical_box")) == 0)
      {
      return VTK_HIERARCHICAL_BOX_DATA_SET;
      }
    }

  return -1;
}

//----------------------------------------------------------------------------
int vtkCompositeDataReader::RequestData(vtkInformation *, vtkInformationVector **,
  vtkInformationVector *outputVector)
{
  if (!(this->OpenVTKFile()) || !this->ReadHeader())
    {
    return 0;
    }

  vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::GetData(outputVector, 0);
  vtkMultiPieceDataSet* mp = vtkMultiPieceDataSet::GetData(outputVector, 0);
  vtkHierarchicalBoxDataSet* hb = vtkHierarchicalBoxDataSet::GetData(outputVector, 0);
  vtkOverlappingAMR* oamr = vtkOverlappingAMR::GetData(outputVector, 0);
  vtkNonOverlappingAMR* noamr = vtkNonOverlappingAMR::GetData(outputVector, 0);

  // Read the data-type description line which was already read in
  // RequestDataObject() so we just skip it here without any additional
  // validation.
  char line[256];
  if (!this->ReadString(line) || !this->ReadString(line))
    {
    vtkErrorMacro(<<"Data file ends prematurely!");
    this->CloseVTKFile ();
    return 0;
    }

  if (mb)
    {
    this->ReadCompositeData(mb);
    }
  else if (mp)
    {
    this->ReadCompositeData(mp);
    }
  else if (hb)
    {
    this->ReadCompositeData(hb);
    }
  else if (oamr)
    {
    this->ReadCompositeData(oamr);
    }
  else if (noamr)
    {
    this->ReadCompositeData(noamr);
    }

  return 1;
}

//----------------------------------------------------------------------------
bool vtkCompositeDataReader::ReadCompositeData(vtkMultiBlockDataSet* mb)
{
  char line[256];
  if (!this->ReadString(line))
    {
    vtkErrorMacro("Failed to read block-count");
    return false;
    }

  if (strncmp(this->LowerCase(line), "children", strlen("children")) != 0)
    {
    vtkErrorMacro("Failed to read CHILDREN.");
    return false;
    }

  unsigned int num_blocks = 0;
  if (!this->Read(&num_blocks))
    {
    vtkErrorMacro("Failed to read number of blocks");
    return false;
    }

  mb->SetNumberOfBlocks(num_blocks);
  for (unsigned int cc=0; cc < num_blocks; cc++)
    {
    if (!this->ReadString(line))
      {
      vtkErrorMacro("Failed to read 'CHILD <type>' line");
      return false;
      }

    int type;
    if (!this->Read(&type))
      {
      vtkErrorMacro("Failed to read child type.");
      return false;
      }
    // eat up the "\n" and other whitespace at the end of CHILD <type>.
    this->ReadLine(line);
    // if "line" has text enclosed in [] then that's the composite name.
    vtksys::RegularExpression regEx("\\s*\\[(.*)\\]");
    if (regEx.find(line))
      {
      std::string name = regEx.match(1);
      mb->GetMetaData(cc)->Set(vtkCompositeDataSet::NAME(), name.c_str());
      }

    if (type != -1)
      {
      vtkDataObject* child = this->ReadChild();
      if (!child)
        {
        vtkErrorMacro("Failed to read child.");
        return false;
        }
      mb->SetBlock(cc, child);
      child->FastDelete();
      }
    else
      {
      // eat up the ENDCHILD marker.
      this->ReadString(line);
      }
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkCompositeDataReader::ReadCompositeData(vtkHierarchicalBoxDataSet* hb)
{
  (void)hb;
  vtkErrorMacro("This isn't supported yet.");
  return false;
}

//----------------------------------------------------------------------------
bool vtkCompositeDataReader::ReadCompositeData(vtkOverlappingAMR* oamr)
{
  char line[256];

  // read GRID_DESCRIPTION.
  int description;
  if (!this->ReadString(line) ||
    strncmp(this->LowerCase(line), "grid_description", strlen("grid_description")) != 0 ||
    !this->Read(&description))
    {
    vtkErrorMacro("Failed to read GRID_DESCRIPTION (or its value).");
    return false;
    }

  // read ORIGIN
  double origin[3];
  if (!this->ReadString(line) ||
    strncmp(this->LowerCase(line), "origin", strlen("origin")) != 0 ||
    !this->Read(&origin[0]) || !this->Read(&origin[1]) ||
    !this->Read(&origin[2]))
    {
    vtkErrorMacro("Failed to read ORIGIN (or its value).");
    return false;
    }

  // read LEVELS.
  int num_levels;
  if (!this->ReadString(line) ||
    strncmp(this->LowerCase(line), "levels", strlen("levels")) != 0 ||
    !this->Read(&num_levels))
    {
    vtkErrorMacro("Failed to read LEVELS (or its value).");
    return false;
    }

  std::vector<int> blocksPerLevel;
  blocksPerLevel.resize(num_levels);

  std::vector<double> spacing;
  spacing.resize(num_levels * 3);

  int total_blocks = 0;
  for (int cc=0; cc < num_levels; cc++)
    {
    if (!this->Read(&blocksPerLevel[cc]))
      {
      vtkErrorMacro("Failed to read number of datasets for level " << cc);
      return false;
      }
    if (!this->Read(&spacing[3*cc +0]) || !this->Read(&spacing[3*cc +1]) ||
      !this->Read(&spacing[3*cc+2]))
      {
      vtkErrorMacro("Failed to read spacing for level " << cc);
      return false;
      }
    total_blocks += blocksPerLevel[cc];
    }

  // initialize the AMR.
  oamr->Initialize(num_levels, &blocksPerLevel[0]);
  oamr->SetGridDescription(description);
  oamr->SetOrigin(origin);
  for (int cc=0; cc < num_levels; cc++)
    {
    oamr->GetAMRInfo()->SetSpacing(cc, &spacing[3*cc]);
    }

  //read in the amr boxes0
  if (!this->ReadString(line))
    {
    vtkErrorMacro("Failed to read AMRBOXES' line");
    }
  else
    {
    if (strncmp(this->LowerCase(line), "amrboxes", strlen("amrboxes")) != 0)
      {
      vtkErrorMacro("Failed to read AMRBOXES' line");
      }
    else
      {
      // now read the amrbox information.
      int num_tuples, num_components;
      if (!this->Read(&num_tuples) || !this->Read(&num_components))
        {
        vtkErrorMacro("Failed to read values for AMRBOXES.");
        return false;
        }

      vtkSmartPointer<vtkIntArray> idata;
      idata.TakeReference(vtkIntArray::SafeDownCast(
                            this->ReadArray("int", num_tuples, num_components)));
      if (!idata || idata->GetNumberOfComponents() != 6 ||
          idata->GetNumberOfTuples() != static_cast<vtkIdType>(oamr->GetTotalNumberOfBlocks()))
        {
        vtkErrorMacro("Failed to read meta-data");
        return false;
        }

      unsigned int metadata_index = 0;
      for (int level=0; level < num_levels; level++)
        {
        unsigned int num_datasets = oamr->GetNumberOfDataSets(level);
        for (unsigned int index=0; index < num_datasets; index++, metadata_index++)
          {
          int tuple[6];
          idata->GetTupleValue(metadata_index, tuple);

          vtkAMRBox box;
          box.SetDimensions(&tuple[0], &tuple[3], description);
          oamr->SetAMRBox(level, index, box);
          }
        }
      }
    }


  //read in the actual data


  for (int cc=0; cc < total_blocks; cc++)
    {
    if (!this->ReadString(line))
      {
      // we may reach end of file sooner than total_blocks since not all blocks
      // may be present in the data.
      break;
      }

    if (strncmp(this->LowerCase(line), "child", strlen("child")) == 0)
      {
      unsigned int level=0, index=0;
      if (!this->Read(&level) || !this->Read(&index))
        {
        vtkErrorMacro("Failed to read level and index information");
        return false;
        }
      this->ReadLine(line);
      vtkDataObject* child = this->ReadChild();
      if (!child)
        {
        vtkErrorMacro("Failed to read dataset at " << level << ", " << index);
        return false;
        }
      if (child->IsA("vtkImageData"))
        {
        vtkUniformGrid* grid = vtkUniformGrid::New();
        grid->ShallowCopy(child);
        oamr->SetDataSet(level, index, grid);
        grid->FastDelete();
        child->Delete();
        }
      else
        {
        vtkErrorMacro("vtkImageData expected at " << level << ", " << index);
        child->Delete();
        return false;
        }
      }
    else
      {
      vtkErrorMacro("Failed to read 'CHILD' line");
      return false;
      }
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkCompositeDataReader::ReadCompositeData(vtkNonOverlappingAMR* hb)
{
  (void)hb;
  vtkErrorMacro("This isn't supported yet.");
  return false;
}

//----------------------------------------------------------------------------
bool vtkCompositeDataReader::ReadCompositeData(vtkMultiPieceDataSet* mp)
{
  char line[256];
  if (!this->ReadString(line))
    {
    vtkErrorMacro("Failed to read block-count");
    return false;
    }

  if (strncmp(this->LowerCase(line), "children", strlen("children")) != 0)
    {
    vtkErrorMacro("Failed to read CHILDREN.");
    return false;
    }

  unsigned int num_pieces = 0;
  if (!this->Read(&num_pieces))
    {
    vtkErrorMacro("Failed to read number of pieces.");
    return false;
    }

  mp->SetNumberOfPieces(num_pieces);
  for (unsigned int cc=0; cc < num_pieces; cc++)
    {
    if (!this->ReadString(line))
      {
      vtkErrorMacro("Failed to read 'CHILD <type>' line");
      return false;
      }

    int type;
    if (!this->Read(&type))
      {
      vtkErrorMacro("Failed to read child type.");
      return false;
      }
    // eat up the "\n" and other whitespace at the end of CHILD <type>.
    this->ReadLine(line);
    // if "line" has text enclosed in [] then that's the composite name.
    vtksys::RegularExpression regEx("\\s*\\[(.*)\\]");
    if (regEx.find(line))
      {
      std::string name = regEx.match(1);
      mp->GetMetaData(cc)->Set(vtkCompositeDataSet::NAME(), name.c_str());
      }

    if (type != -1)
      {
      vtkDataObject* child = this->ReadChild();
      if (!child)
        {
        vtkErrorMacro("Failed to read child.");
        return false;
        }
      mp->SetPiece(cc, child);
      child->FastDelete();
      }
    else
      {
      // eat up the ENDCHILD marker.
      this->ReadString(line);
      }
    }

  return true;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkCompositeDataReader::ReadChild()
{
  // This is tricky. Simplistically speaking, we need to read the string for the
  // child and then pass it to a vtkGenericDataObjectReader and get it read.
  // Identifying where the child ends can be tricky since the child itself may
  // be a composite-dataset. Also, we need to avoid copy memory if we are
  // already reading from a string.

  // We can optimize this further when reading in from a string buffer to avoid
  // copying. But we'll do that later.

  unsigned int child_stack_depth = 1;

  vtksys_ios::ostringstream child_data;
  char line[512];
  while (child_stack_depth > 0)
    // read until ENDCHILD (passing over any nested CHILD-ENDCHILD correctly).
    {
    bool new_line = true;
    while (true)
      // read a full line until "\n". This maybe longer than 512 and hence this
      // extra loop.
      {
      this->IS->get(line, 512);
      if (this->IS->fail())
        {
        if (this->IS->eof())
          {
          vtkErrorMacro("Premature EOF.");
          return NULL;
          }
        else
          {
          // else it's possible that we read in an empty line. istream still marks
          // that as fail().
          this->IS->clear();
          }
        }

      if (new_line)
        {
        // these comparisons need to happen only when a new line is
        // started, hence this "if".
        if (strncmp(line, "ENDCHILD", strlen("ENDCHILD")) == 0)
          {
          child_stack_depth--;
          }
        else if (strncmp(line, "CHILD", strlen("CHILD")) == 0)
          {
          // should not match CHILDREN :(
          if (strncmp(line, "CHILDREN", strlen("CHILDREN")) != 0)
            {
            child_stack_depth++;
            }
          }
        }

      if (child_stack_depth > 0)
        {
        // except the last ENDCHILD, all the read content is to passed to the
        // child-reader.
        child_data.write(line, this->IS->gcount());
        }
      new_line = false;
      if (this->IS->peek() == '\n')
        {
        this->IS->ignore(VTK_INT_MAX, '\n');
        // end of line reached.
        child_data << '\n';
        break;
        }
      } // while (true);
    } // while (child_stack_depth > 0);

  vtkGenericDataObjectReader* reader = vtkGenericDataObjectReader::New();
  reader->SetBinaryInputString(child_data.str().c_str(),
    static_cast<int>(child_data.str().size()));
  reader->ReadFromInputStringOn();
  reader->Update();

  vtkDataObject* child = reader->GetOutput(0);
  if (child)
    {
    child->Register(this);
    }
  reader->Delete();
  return child;
}

//----------------------------------------------------------------------------
void vtkCompositeDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
