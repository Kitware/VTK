/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPOPReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPOPReader - read POP data files
// .SECTION Description
// vtkPOPReader Just converts from images to a structured grid for now.


#ifndef __vtkPOPReader_h
#define __vtkPOPReader_h

#include "vtkStructuredGridAlgorithm.h"

class vtkFloatArray;
class vtkImageData;
class vtkPoints;

class VTK_PARALLEL_EXPORT vtkPOPReader : public vtkStructuredGridAlgorithm 
{
public:
  static vtkPOPReader *New();
  vtkTypeMacro(vtkPOPReader,vtkStructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // This is the longitude and latitude dimensions of the structured grid.
  vtkGetVector2Macro(Dimensions, int);  
    
  // Description:
  // This file contains the latitude and longitude of the grid.  
  // It must be double with no header.
  vtkGetStringMacro(GridFileName);

  // Description:
  // These files contains the u and v components of the flow.
  vtkGetStringMacro(UFlowFileName);
  vtkGetStringMacro(VFlowFileName);
  
  // Description:
  // This file contains information about all the files.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  
  // Description:
  // Radius of the earth.
  vtkSetMacro(Radius, double);
  vtkGetMacro(Radius, double);
  
  // Description:
  // Because the data can be so large, here is an option to clip
  // while reading.
  vtkSetVector6Macro(ClipExtent, int);
  vtkGetVector6Macro(ClipExtent, int);

  // Description:
  // Set the number of ghost levels to include in the data
  vtkSetMacro(NumberOfGhostLevels, int);
  vtkGetMacro(NumberOfGhostLevels, int);

protected:
  vtkPOPReader();
  ~vtkPOPReader();

  virtual int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  
  void ReadInformationFile();
  vtkPoints *ReadPoints(vtkImageData *image, vtkInformation *outInfo);
  void ReadFlow(vtkStructuredGrid *output, vtkInformation *outInfo);
  // NOT USED
  vtkPoints *GeneratePoints();
  
  char *FileName;
  
  int Dimensions[2];
  vtkSetStringMacro(GridFileName);
  void SetGridName(char *name);
  char *GridFileName;

  double Radius;
  vtkFloatArray *DepthValues;
  int NumberOfGhostLevels;

  void DeleteArrays();
  void AddArray(char *arrayName, char *fileName, unsigned long offset);
  void AddArrayName(char *arrayName, char *fileName, unsigned long offset);
  int NumberOfArrays;
  int MaximumNumberOfArrays;
  char **ArrayNames;
  char **ArrayFileNames;  
  unsigned long *ArrayOffsets;
  int ArrayFileDimensionality;

  char *UFlowFileName;
  vtkSetStringMacro(UFlowFileName);
  unsigned long UFlowFileOffset;
  char *VFlowFileName;
  vtkSetStringMacro(VFlowFileName);
  unsigned long VFlowFileOffset;  

  int IsFileName(char *name);
  char *MakeFileName(char *name);

  int ClipExtent[6];

  vtkPOPReader(const vtkPOPReader&);   // Not implemented
  void operator=(const vtkPOPReader&); // Not implemented
};

#endif
