/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestXMLCInterface.c

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkXMLWriterC.h"

#define NPOINTS 8
#define NTIMESTEPS 8

int main()
{
  int i,j;
  vtkXMLWriterC* writer = vtkXMLWriterC_New();
  const char filename[] = "cube.vtu";
  float points[3*NPOINTS] = {0, 0, 0,
                       1, 0, 0,
                       1, 1, 0,
                       0, 1, 0,
                       0, 0, 1,
                       1, 0, 1,
                       1, 1, 1,
                       0, 1, 1 };
  vtkIdType cellarray[] = {8, 0, 1, 2, 3, 4, 5, 6, 7};
  float pointdata[NTIMESTEPS][NPOINTS];
  /* Give different values for the pointdata: */
  for(i=0;i<NTIMESTEPS;i++)
    {
    for(j=0; j<NPOINTS;j++)
      {
      pointdata[i][j] = (float)i;
      }
    }

  /* #define VTK_UNSTRUCTURED_GRID               4 */
  vtkXMLWriterC_SetDataObjectType(writer, 4);
  vtkXMLWriterC_SetFileName(writer, filename);
  /* #define VTK_FLOAT          10 */
  vtkXMLWriterC_SetPoints(writer, 10, points, NPOINTS);
  /* #define VTK_HEXAHEDRON    12 */
  vtkXMLWriterC_SetCellsWithType(writer, 12, 1, cellarray, 1+NPOINTS);

  /* for all timesteps: */
  vtkXMLWriterC_SetNumberOfTimeSteps(writer, NTIMESTEPS);
  vtkXMLWriterC_Start(writer);
  for(i=0; i<NTIMESTEPS; i++)
    {
    /* #define VTK_FLOAT          10 */
    vtkXMLWriterC_SetPointData(writer, "example data", 10, pointdata[i],
                               NPOINTS, 1, "SCALARS");
    vtkXMLWriterC_WriteNextTimeStep(writer, i);
    }
  vtkXMLWriterC_Stop(writer);
  vtkXMLWriterC_Delete(writer);

  return 0;
}
