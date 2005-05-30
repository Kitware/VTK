/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLCInterface.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __vtkXMLCInterface_h
#define __vtkXMLCInterface_h

#include <stdio.h>  /* for size_t */

#ifdef __cplusplus
extern "C" {
#endif /*cplusplus*/
  
void vtkXML_Initialize();
void vtkXML_SetFileName(const char* filename);
void vtkXML_SetPoints(int datatype, float* array, size_t size);
void vtkXML_SetPointData(int datatype, float* array, size_t size);
void vtkXML_SetCellArray(int datatype, int* array, int ncells, size_t size);
void vtkXML_Write();
void vtkXML_WriteNextTime(double t);
void vtkXML_SetNumberOfTimeSteps(int n);
void vtkXML_Start();
void vtkXML_Stop();

#ifdef __cplusplus
};
#endif /*cplusplus*/

#endif
