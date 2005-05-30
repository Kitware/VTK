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

#include "vtkXMLCInterface.h"

#define NPOINTS 8
#define NTIMESTEPS 8

int main()
{
  int i,j;
  const char filename[] = "cube.vtu";
  float points[3*NPOINTS] = {0, 0, 0, 
                       1, 0, 0, 
                       1, 1, 0,
                       0, 1, 0, 
                       0, 0, 1, 
                       1, 0, 1,
                       1, 1, 1, 
                       0, 1, 1 };
  int cellarray[] = {8, 0, 1, 2, 3, 4, 5, 6, 7};
  float pointdata[NPOINTS][NTIMESTEPS];
  /* Give different values for the pointdata: */
  for(i=0;i<NTIMESTEPS;i++)
    {
    float *pointdata_tmp = pointdata[i];
    for(j=0; j<NPOINTS;j++)
      {
      pointdata_tmp[j] = (float)i;
      }
    }


  vtkXML_Initialize();
  vtkXML_SetFileName( filename );
  vtkXML_SetPoints   (10, points,    NPOINTS);
  vtkXML_SetCellArray(cellarray, 1, 9); /*1 cell and length is ncells+size(cell) */

  /* for all timesteps: */
  vtkXML_SetNumberOfTimeSteps(NTIMESTEPS);
  vtkXML_Start();
  for(i=0; i<NTIMESTEPS; i++)
    {
    /* #define VTK_FLOAT          10 */
    vtkXML_SetPointData(10, pointdata[i], NPOINTS, 1);
    vtkXML_WriteNextTime(i);
    }
  vtkXML_Stop();

  return 1;
}

