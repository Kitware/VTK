/*=========================================================================

  Program:   Visualization Toolkit
  Module:    H5RageAdaptor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef H5RageAdaptor_h
#define H5RageAdaptor_h

#include "vtkDataArraySelection.h"
#include "vtkIOH5RageModule.h" // For export macro

#include <string> // for std::string
#include <vector> // for std::vector

class vtkDataArraySelection;
class vtkImageData;
class vtkMultiProcessController;

class VTKIOH5RAGE_EXPORT H5RageAdaptor
{
public:
  H5RageAdaptor(vtkMultiProcessController* ctrl);
  ~H5RageAdaptor();

  int InitializeGlobal(const char* DescFile);
  void LoadVariableData(vtkImageData* data, int timeStep, vtkDataArraySelection* cellSelection);
  template <class T>
  void ConvertHDFData(int ndims, int* dims_out, T* hdfData);

  int GetNumberOfTimeSteps() { return this->NumberOfTimeSteps; }
  double GetTimeStep(int step) { return this->TimeSteps[step]; }

  int GetNumberOfVariables() { return (int)this->VariableName.size(); }
  const char* GetVariableName(int indx) { return this->VariableName[indx].c_str(); }

  int GetWholeExtent(int ext) { return this->WholeExtent[ext]; }
  int GetSubExtent(int ext) { return this->SubExtent[ext]; }
  int GetDimension(int dim) { return this->Dimension[dim]; }
  double GetOrigin(int dim) { return this->Origin[dim]; }
  double GetSpacing(int dim) { return this->Spacing[dim]; }

protected:
  // Collect the metadata
  int CollectMetaData(const char* H5RageFileName);
  int ParseH5RageFile(const char* H5RageFileName);
  std::string TrimString(const std::string& str);

  // Used in parallel reader and load balancing
  vtkMultiProcessController* Controller;
  int Rank;
  int TotalRank;

  // Time series of hdf files
  std::vector<std::string> HdfFileName; // all hdf files

  // Time step information retrieved from hdf filenames
  int NumberOfTimeSteps;
  double* TimeSteps;

  // Geometry information for sharing data with other processors
  int** ExtentSchedule;
  int* NumberOfTuples;

  int WholeExtent[6]; // Size of image
  int SubExtent[6];   // Size of image this processor
  int Dimension[3];   // Dimension of image
  double Origin[3];   // Physical origin
  double Spacing[3];  // Physical spacing

  int NumberOfDimensions;
  int TotalTuples;
  bool UseFloat64;

  // Variable information retrieved from hdf filenames
  int NumberOfVariables;
  std::vector<std::string> VariableName;
};

#endif

// VTK-HeaderTest-Exclude: H5RageAdaptor.h
