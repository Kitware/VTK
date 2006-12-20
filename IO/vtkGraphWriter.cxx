/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGraphWriter.h"

#include "vtkByteSwap.h"
#include "vtkCellArray.h"
#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"

#if!defined(_WIN32) || defined(__CYGWIN__)
# include <unistd.h> /* unlink */
#else
# include <io.h> /* unlink */
#endif

vtkCxxRevisionMacro(vtkGraphWriter, "1.1");
vtkStandardNewMacro(vtkGraphWriter);

void vtkGraphWriter::WriteData()
{
  ostream *fp;
  vtkGraph* const input = this->GetInput();

  vtkDebugMacro(<<"Writing vtk graph data...");

  if( !(fp=this->OpenVTKFile()) || !this->WriteHeader(fp) )
    {
    if(fp)
      {
      if(this->FileName)
        {
        vtkErrorMacro("Ran out of disk space; deleting file: "
                      << this->FileName);
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

  *fp << "DATASET GRAPH\n"; 

  if(input->GetDirected())
    {
    *fp << "DIRECTED\n";
    }
  else
    {
    *fp << "UNDIRECTED\n";
    }
  
  int error_occurred = 0;

  if(!error_occurred && !this->WriteDataSetData(fp, input))
    {
    error_occurred = 1;
    }
  if(!error_occurred && !this->WritePoints(fp, input->GetPoints()))
    {
    error_occurred = 1;
    }
  if(!error_occurred)
    {
    const vtkIdType arc_count = input->GetNumberOfArcs();
    *fp << "ARCS " << arc_count << "\n";
    for(int arc = 0; arc != arc_count; ++arc)
      {
      *fp << input->GetSourceNode(arc) << " " << input->GetTargetNode(arc) << "\n";
      }
    }
  if(!error_occurred && !this->WriteCellData(fp, input))
    {
    error_occurred = 1;
    }
  if(!error_occurred && !this->WritePointData(fp, input))
    {
    error_occurred = 1;
    }

  if(error_occurred)
    {
    if(this->FileName)
      {
      vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
      this->CloseVTKFile(fp);
      unlink(this->FileName);
      }
    else
      {
      vtkErrorMacro("Error writing data set to memory");
      this->CloseVTKFile(fp);
      }
    return;
    }
  this->CloseVTKFile(fp);
}

int vtkGraphWriter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
  return 1;
}

vtkGraph* vtkGraphWriter::GetInput()
{
  return vtkGraph::SafeDownCast(this->Superclass::GetInput());
}

vtkGraph* vtkGraphWriter::GetInput(int port)
{
  return vtkGraph::SafeDownCast(this->Superclass::GetInput(port));
}

void vtkGraphWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
