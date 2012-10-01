/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIVWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkIVWriter.h"

#include "vtkCellArray.h"
#include "vtkInformation.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPointData.h"

vtkStandardNewMacro(vtkIVWriter);

//----------------------------------------------------------------------------
void vtkIVWriter::WriteData()
{
  FILE *fp;

  // make sure the user specified a FileName
  if ( this->FileName == NULL)
    {
    vtkErrorMacro(<< "Please specify FileName to use");
    return;
    }

  // try opening the files
  fp = fopen(this->FileName,"w");
  if (!fp)
    {
    vtkErrorMacro(<< "unable to open OpenInventor file: " << this->FileName);
    return;
    }

  //
  //  Write header
  //
  vtkDebugMacro("Writing OpenInventor file");
  fprintf(fp,"#Inventor V2.0 ascii\n");
  fprintf(fp,"# OpenInventor file written by the visualization toolkit\n\n");
  this->WritePolyData(this->GetInput(), fp);
  if (fclose(fp))
    {
    vtkErrorMacro(<< this->FileName
                  << " did not close successfully. Check disk space.");
    }
}

//----------------------------------------------------------------------------
void vtkIVWriter::WritePolyData(vtkPolyData *pd, FILE *fp)
{
  vtkPoints *points;
  vtkIdType i;
  vtkCellArray *cells;
  vtkIdType npts = 0;
  vtkIdType *indx = 0;
  vtkUnsignedCharArray *colors=NULL;

  points = pd->GetPoints();

  // create colors for vertices
  vtkDataArray *scalars = pd->GetPointData()->GetScalars();

  if ( scalars )
    {
    vtkLookupTable *lut;
    if ( (lut=scalars->GetLookupTable()) == NULL )
      {
      lut = vtkLookupTable::New();
      lut->Build();
      }
    colors = lut->MapScalars(scalars,VTK_COLOR_MODE_DEFAULT,0);
    if ( ! scalars->GetLookupTable() )
      {
      lut->Delete();
      }
    }

  fprintf(fp,"Separator {\n");

  // Point data (coordinates)
  fprintf(fp,"\tCoordinate3 {\n");
  fprintf(fp,"\t\tpoint [\n");
  fprintf(fp,"\t\t\t");
  for (i=0; i<points->GetNumberOfPoints(); i++)
    {
    double xyz[3];
    points->GetPoint(i, xyz);
    fprintf(fp, "%g %g %g, ", xyz[0], xyz[1], xyz[2]);
    if (!((i+1)%2))
      {
      fprintf(fp, "\n\t\t\t");
      }
    }
  fprintf(fp, "\n\t\t]");
  fprintf(fp, "\t}\n");

  // Per vertex coloring
  fprintf(fp,"\tMaterialBinding {\n");
  fprintf(fp,"\t\tvalue PER_VERTEX_INDEXED\n");
  fprintf(fp,"\t}\n");

  // Colors, if any
  if (colors)
    {
    fprintf(fp,"\tMaterial {\n");
    fprintf(fp,"\t\tdiffuseColor [\n");
    fprintf(fp, "\t\t\t");
    for (i=0; i<colors->GetNumberOfTuples(); i++)
      {
      unsigned char *rgba;
      rgba = colors->GetPointer(4*i);
      fprintf(fp, "%g %g %g, ", rgba[0]/255.0f,
              rgba[1]/255.0f, rgba[2]/255.0f);
      if (!((i+1)%2))
        {
        fprintf(fp, "\n\t\t\t");
        }
      }
    fprintf(fp, "\n\t\t]\n");
    fprintf(fp,"\t}\n");
    colors->Delete();
    }


  // write out polys if any
  if (pd->GetNumberOfPolys() > 0)
    {
    fprintf(fp,"\tIndexedFaceSet {\n");
    fprintf(fp,"\t\tcoordIndex [\n");
    cells = pd->GetPolys();
    for (cells->InitTraversal(); cells->GetNextCell(npts,indx); )
      {
      fprintf(fp, "\t\t\t");
      for (i = 0; i < npts; i++)
        {
        // treating vtkIdType as int
        fprintf(fp,"%i, ", (int)indx[i]);
        }
      fprintf(fp,"-1,\n");
      }
    fprintf(fp,"\t\t]\n");
    fprintf(fp,"\t}\n");
    }

  // write out lines if any
  if (pd->GetNumberOfLines() > 0)
    {
    fprintf(fp,"\tIndexedLineSet {\n");
    fprintf(fp,"\t\tcoordIndex  [\n");

    cells = pd->GetLines();
    for (cells->InitTraversal(); cells->GetNextCell(npts,indx); )
      {
      fprintf(fp,"\t\t\t");
      for (i = 0; i < npts; i++)
        {
        // treating vtkIdType as int
        fprintf(fp,"%i, ", (int)indx[i]);
        }
      fprintf(fp,"-1,\n");
      }
    fprintf(fp,"\t\t]\n");
    fprintf(fp,"\t}\n");
    }

  // write out verts if any
  if (pd->GetNumberOfVerts() > 0)
    {
    fprintf(fp,"\tIndexdedPointSet {\n");
    fprintf(fp,"\t\tcoordIndex [");
    cells = pd->GetVerts();
    for (cells->InitTraversal(); cells->GetNextCell(npts,indx); )
      {
      fprintf(fp,"\t\t\t");
      for (i = 0; i < npts; i++)
        {
        // treating vtkIdType as int
        fprintf(fp,"%i, ", (int)indx[i]);
        }
      fprintf(fp,"-1,\n");
      }
    fprintf(fp,"\t\t]\n");
    fprintf(fp,"\t}\n");
    }


  // write out tstrips if any
  if (pd->GetNumberOfStrips() > 0)
    {

    fprintf(fp,"\tIndexedTriangleStripSet {\n");
    fprintf(fp,"\t\tcoordIndex [\n");
    cells = pd->GetStrips();
    for (cells->InitTraversal(); cells->GetNextCell(npts,indx); )
      {
      fprintf(fp,"\t\t\t");
      for (i = 0; i < npts; i++)
        {
        // treating vtkIdType as int
        fprintf(fp,"%i, ", (int)indx[i]);
        }
      fprintf(fp,"-1,\n");
      }
    fprintf(fp,"\t\t]\n");
    fprintf(fp,"\t}\n");
    }

  fprintf(fp,"}\n"); // close the  Shape

}


//----------------------------------------------------------------------------
void vtkIVWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
vtkPolyData* vtkIVWriter::GetInput()
{
  return vtkPolyData::SafeDownCast(this->Superclass::GetInput());
}

//----------------------------------------------------------------------------
vtkPolyData* vtkIVWriter::GetInput(int port)
{
  return vtkPolyData::SafeDownCast(this->Superclass::GetInput(port));
}

//----------------------------------------------------------------------------
int vtkIVWriter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  return 1;
}
