/*=========================================================================

  Program:   ParaView
  Module:    H5RageAdaptor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "H5RageAdaptor.h"

#include "vtkCellArray.h"
#include "vtkDataArraySelection.h"
#include "vtkDirectory.h"
#include "vtkDoubleArray.h"
#include "vtkErrorCode.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"

#include <cctype>
#include <float.h>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <vtk_hdf5.h>

#if defined(_WIN32) && !defined(__CYGWIN__)
const static char* Slash = "\\";
#else
const static char* Slash = "/";
#endif

namespace
{
// mpi tag
const int mpiTag = 1758978;

void BroadcastString(vtkMultiProcessController* controller, std::string& str, int rank)
{
  unsigned long len = static_cast<unsigned long>(str.size()) + 1;
  controller->Broadcast(&len, 1, 0);
  if (len)
  {
    if (rank)
    {
      std::vector<char> tmp;
      tmp.resize(len);
      controller->Broadcast(&(tmp[0]), len, 0);
      str = &tmp[0];
    }
    else
    {
      const char* start = str.c_str();
      std::vector<char> tmp(start, start + len);
      controller->Broadcast(&tmp[0], len, 0);
    }
  }
}

void BroadcastStringVector(
  vtkMultiProcessController* controller, std::vector<std::string>& svec, int rank)
{
  unsigned long len = static_cast<unsigned long>(svec.size());
  controller->Broadcast(&len, 1, 0);
  if (rank)
  {
    svec.resize(len);
  }
  std::vector<std::string>::iterator it;
  for (it = svec.begin(); it != svec.end(); ++it)
  {
    BroadcastString(controller, *it, rank);
  }
}
};

///////////////////////////////////////////////////////////////////////////////
//
// Constructor and destructor
//
///////////////////////////////////////////////////////////////////////////////

H5RageAdaptor::H5RageAdaptor(vtkMultiProcessController* ctrl)
{
  this->Controller = ctrl;
  if (this->Controller)
  {
    this->Rank = this->Controller->GetLocalProcessId();
    this->TotalRank = this->Controller->GetNumberOfProcesses();
  }
  else
  {
    this->Rank = 0;
    this->TotalRank = 1;
  }

  this->NumberOfVariables = 0;
  this->NumberOfTimeSteps = 0;
  this->NumberOfDimensions = 3;
  this->TotalTuples = 0;
  this->UseFloat64 = false;

  for (int dim = 0; dim < this->NumberOfDimensions; dim++)
  {
    this->Dimension[dim] = 1;
    this->Origin[dim] = 0.0;
    this->Spacing[dim] = 1.0;
    this->WholeExtent[dim * 2] = 0;
    this->WholeExtent[dim * 2 + 1] = 0;
  }

  // Schedule for sending partitioned variables to processors
  // Only rank 0 reads the data, ghost overlap is 1
  this->NumberOfTuples = new int[this->TotalRank];
  this->ExtentSchedule = new int*[this->TotalRank];
  for (int rank = 0; rank < this->TotalRank; rank++)
  {
    this->ExtentSchedule[rank] = new int[6];
  }
}

H5RageAdaptor::~H5RageAdaptor()
{
  for (int rank = 0; rank < this->TotalRank; rank++)
  {
    delete[] this->ExtentSchedule[rank];
  }
  delete[] this->ExtentSchedule;
  delete[] this->NumberOfTuples;

  this->Controller = nullptr;
}

///////////////////////////////////////////////////////////////////////////////
//
// Remove whitespace from the beginning and end of a string
//
///////////////////////////////////////////////////////////////////////////////

std::string H5RageAdaptor::TrimString(const std::string& str)
{
  std::string whitespace = " \n\r\t\f\v";
  size_t start = str.find_first_not_of(whitespace);
  size_t end = str.find_last_not_of(whitespace);
  if (start == std::string::npos || end == std::string::npos)
  {
    // the whole line is whitespace
    return std::string("");
  }
  else
  {
    return str.substr(start, end - start + 1);
  }
}

///////////////////////////////////////////////////////////////////////////////
//
// Read descriptor file, collect meta data on proc 0, share with other procs
//
///////////////////////////////////////////////////////////////////////////////

int H5RageAdaptor::InitializeGlobal(const char* H5RageFileName)
{
  int success;
  if (this->Rank == 0)
  {
    success = CollectMetaData(H5RageFileName);
    this->Controller->Broadcast(&success, 1, 0);
  }
  else
  {
    this->Controller->Broadcast(&success, 1, 0);
  }

  if (!success)
  {
    return 0;
  }

  // Share with all processors sizes, timesteps, variables
  this->Controller->Broadcast(this->Dimension, this->NumberOfDimensions, 0);
  this->Controller->Broadcast(this->Origin, this->NumberOfDimensions, 0);
  this->Controller->Broadcast(this->Spacing, this->NumberOfDimensions, 0);
  this->Controller->Broadcast(&this->NumberOfVariables, 1, 0);
  this->Controller->Broadcast(&this->NumberOfTimeSteps, 1, 0);
  if (this->Rank > 0)
  {
    this->TimeSteps = new double[this->NumberOfTimeSteps];
  }
  this->Controller->Broadcast(this->TimeSteps, this->NumberOfTimeSteps, 0);
  BroadcastStringVector(this->Controller, this->VariableName, this->Rank);

  // Calculate WholeExtent
  for (int dim = 0; dim < this->NumberOfDimensions; dim++)
  {
    this->WholeExtent[dim * 2] = 0;
    this->WholeExtent[dim * 2 + 1] = this->Dimension[dim] - 1;
  }

  // Calculate SubExtent for each processor for schedule to send variables
  if (this->Rank == 0)
  {
    // Initialize schedule to whole extent
    for (int rank = 0; rank < this->TotalRank; rank++)
    {
      for (int ext = 0; ext < 6; ext++)
      {
        this->ExtentSchedule[rank][ext] = this->WholeExtent[ext];
      }
    }

    // Calculate which dimension to partition on in slabs
    int maxDim = 0;
    int useDim = 0;
    for (int dim = 0; dim < this->NumberOfDimensions; dim++)
    {
      if (this->Dimension[dim] > maxDim)
      {
        maxDim = this->Dimension[dim];
        useDim = dim;
      }
    }
    int perSlab = maxDim / this->TotalRank;
    int indx0 = useDim * 2;
    int indx1 = indx0 + 1;
    for (int rank = 0; rank < this->TotalRank; rank++)
    {
      this->ExtentSchedule[rank][indx0] = rank * perSlab;
      this->ExtentSchedule[rank][indx1] = this->ExtentSchedule[rank][indx0] + perSlab - 1;
    }
    this->ExtentSchedule[(this->TotalRank - 1)][indx1] = this->WholeExtent[indx1];

    // Enlarge the extents to allow for ghost levels
    for (int rank = 0; rank < this->TotalRank; rank++)
    {
      if (this->ExtentSchedule[rank][indx1] != this->WholeExtent[indx1])
      {
        this->ExtentSchedule[rank][indx1] += 1;
      }
    }

    // Share the extent schedule and number of tuples belonging to each processor
    for (int rank = 0; rank < this->TotalRank; rank++)
    {
      this->NumberOfTuples[rank] = 1;
      for (int dim = 0; dim < this->NumberOfDimensions; dim++)
      {
        int subDim =
          this->ExtentSchedule[rank][dim * 2 + 1] - this->ExtentSchedule[rank][dim * 2] + 1;
        if (subDim > 0)
        {
          this->NumberOfTuples[rank] *= subDim;
        }
      }
    }

    // Send to each processor
    for (int rank = 1; rank < this->TotalRank; rank++)
    {
      this->Controller->Send(this->NumberOfTuples, this->TotalRank, rank, mpiTag);
      this->Controller->Send(this->ExtentSchedule[rank], 6, rank, mpiTag);
    }

    // Set for rank 0
    for (int ext = 0; ext < 6; ext++)
    {
      this->SubExtent[ext] = this->ExtentSchedule[0][ext];
    }
  }
  else
  {
    // Receive on each processor
    this->Controller->Receive(this->NumberOfTuples, this->TotalRank, 0, mpiTag);
    this->Controller->Receive(this->SubExtent, 6, 0, mpiTag);
  }
  return 1;
}

///////////////////////////////////////////////////////////////////////////////
//
// Read the global descriptor file (name.h5rage) collecting hdf directory info,
// filenames, cycle numbers and variable names
// Read hdf file to collect sizes, origin and spacing
//
///////////////////////////////////////////////////////////////////////////////

int H5RageAdaptor::CollectMetaData(const char* H5RageFileName)
{
  // Parse descriptor file collecting hdf directory, base name, create list of files
  // One hdf file per variable and per cycle with dataset named "data"
  if (ParseH5RageFile(H5RageFileName) == 0)
  {
    return 0;
  }

  // Read the first file to get sizes and type
  std::string firstFileName = this->HdfFileName[0];
  hid_t file_id = H5Fopen(firstFileName.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
  if (file_id < 0)
  {
    vtkGenericWarningMacro(
      "Error loading file: " << firstFileName << ". Please ensure files are HDF5 and not HDF4.");
    return 0;
  }

  // Data is stored column major order so dimensions must be reversed
  hid_t dataset_id = H5Dopen(file_id, "data", hid_t(0));
  hid_t datatype_id = H5Dget_type(dataset_id);
  hid_t dataspace_id = H5Dget_space(dataset_id);
  size_t size = H5Tget_size(datatype_id);
  if (size == 8)
  {
    this->UseFloat64 = true;
  }

  // Read coordinates for physical extents and record which plane
  // Dimension of each coordinate array gives dimensions for render
  std::string coordName[3] = { "x", "y", "z" };
  hid_t memspace_id = hid_t(0);
  for (int dim = 0; dim < 3; dim++)
  {
    if (H5Lexists(file_id, coordName[dim].c_str(), H5P_DEFAULT))
    {
      dataset_id = H5Dopen(file_id, coordName[dim].c_str(), hid_t(0));
      dataspace_id = H5Dget_space(dataset_id);
      hsize_t ndims = H5Sget_simple_extent_ndims(dataspace_id);
      hsize_t* coordSize = new hsize_t[ndims];
      H5Sget_simple_extent_dims(dataspace_id, coordSize, nullptr);
      memspace_id = H5Screate_simple(ndims, coordSize, nullptr);
      float* coordinates = new float[coordSize[0]];
      H5Dread(dataset_id, H5T_NATIVE_FLOAT, memspace_id, dataspace_id, H5P_DEFAULT, coordinates);
      this->Origin[dim] = coordinates[0];
      this->Spacing[dim] = coordinates[1] - coordinates[0];
      this->Dimension[dim] = coordSize[0];
      delete[] coordinates;
      delete[] coordSize;
    }
  }

  this->TotalTuples = 1;
  for (int dim = 0; dim < 3; dim++)
  {
    this->TotalTuples *= this->Dimension[dim];
  }

  H5Tclose(datatype_id);
  H5Dclose(dataset_id);
  H5Sclose(dataspace_id);
  H5Sclose(memspace_id);
  H5Fclose(file_id);

  return 1;
}

///////////////////////////////////////////////////////////////////////////////
//
// Read the global descriptor file (name.h5rage)
//
// HDF_BASE_NAME base        (Required)
// HDF_DIRECTORY hdf0        (Defaults to "." if missing)
// HDF_DIRECTORY hdf1
// HDF_CYCLE_DIGITS number
//
///////////////////////////////////////////////////////////////////////////////

int H5RageAdaptor::ParseH5RageFile(const char* H5RageFileName)
{
  // Read the global descriptor file (name.hrage)
  std::string hdfRageFileName = H5RageFileName;
  std::ifstream ifStr(hdfRageFileName);
  if (!ifStr)
  {
    vtkGenericWarningMacro(
      "Could not open the global description .h5rage file: " << H5RageFileName);
    return 0;
  }

  // Get the directory name from the full path in GUI or use current directory
  std::string::size_type dirPos = hdfRageFileName.find_last_of(Slash);
  std::string dirName;
  if (dirPos == std::string::npos)
  {
    std::ostringstream tempStr;
    tempStr << "." << Slash;
    dirName = tempStr.str();
  }
  else
  {
    dirName = hdfRageFileName.substr(0, dirPos);
  }

  // Parse the h5rage input file
  std::string hdfBaseName;               // base name to use for data files
  std::vector<std::string> hdfDirectory; // directories holding data files
  int numCycleDigits = 6;                // number of digits used for cycle number, default is 6
  std::string::size_type pos = hdfRageFileName.rfind('.');
  std::string suffix = hdfRageFileName.substr(pos + 1);
  char inBuf[256];
  std::string rest;
  std::string keyword;

  while (ifStr.getline(inBuf, 256))
  {
    std::string localline(inBuf);
    localline = TrimString(localline);
    if (localline.length() > 0)
    {
      if (localline[0] != '#' && localline[0] != '!')
      {
        // Remove quotes from input
        localline.erase(std::remove(localline.begin(), localline.end(), '\"'), localline.end());
        localline.erase(std::remove(localline.begin(), localline.end(), '\''), localline.end());

        // check for comments in the middle of the line
        std::string::size_type poundPos = localline.find('#');
        if (poundPos != std::string::npos)
        {
          localline = localline.substr(0, poundPos);
        }

        std::string::size_type bangPos = localline.find('!');
        if (bangPos != std::string::npos)
        {
          localline = localline.substr(0, bangPos);
        }

        std::string::size_type keyPos = localline.find(' ');
        keyword = TrimString(localline.substr(0, keyPos));
        rest = TrimString(localline.substr(keyPos + 1));
        std::istringstream line(rest);

        if (keyword == "HDF_DIRECTORY")
        {
          line >> rest;
          if (rest[0] == '/')
          {
            // If a full path is given use it
            hdfDirectory.push_back(rest);
          }
          else
          {
            // If partial path append to the dir of the .h5rage file
            std::ostringstream tempStr;
            tempStr << dirName << Slash << rest;
            hdfDirectory.push_back(tempStr.str());
          }
        }

        if (keyword == "HDF_BASE_NAME")
        {
          line >> rest;
          std::ostringstream tempStr;
          tempStr << rest;
          hdfBaseName = tempStr.str();
        }

        if (keyword == "HDF_CYCLE_DIGITS")
        {
          line >> rest;
          std::ostringstream tempStr;
          tempStr << rest;

          // check that all characters are digits
          bool good = true;
          std::string cycleDigits = tempStr.str();
          for (std::string::const_iterator it = cycleDigits.begin(); it != cycleDigits.end(); it++)
          {
            if (!std::isdigit(*it))
            {
              vtkGenericWarningMacro(
                "Argument for HDF_CYCLE_DIGITS is not a number: \'" << cycleDigits << "\'");
              good = false;
              break;
            }
          }
          if (good)
          {
            numCycleDigits = std::stoi(cycleDigits);
          }
        }
      }
    }
  }
  if (hdfDirectory.empty())
  {
    hdfDirectory.push_back(dirName);
  }

  // Find all files starting with base name with -h appended
  auto directory = vtkSmartPointer<vtkDirectory>::New();
  uint64_t numFiles = 0;
  std::ostringstream baseNameStr;
  baseNameStr << hdfBaseName << "-h";

  std::map<std::string, std::vector<std::string>> varToFileMap;
  std::set<std::string> cycleSet;

  // Each hdf file is specific to a variable and a cycle
  int numTotalHDFFiles = 0;
  for (size_t dir = 0; dir < hdfDirectory.size(); dir++)
  {
    if (directory->Open(hdfDirectory[dir].c_str()) == false)
    {
      vtkGenericWarningMacro("HDF directory does not exist: " << hdfDirectory[dir]);
    }
    else
    {
      int numFound = 0;
      numFiles = directory->GetNumberOfFiles();
      for (unsigned int i = 0; i < numFiles; i++)
      {
        // Check for legal hdf name
        // check that the beginning of the filename is correct
        std::string fileStr = directory->GetFile(i);
        std::size_t found = fileStr.find(baseNameStr.str());
        if (found == std::string::npos || found > 0)
        {
          continue;
        }

        // check that there is no '.' in the filename
        // filename should not have file extension or be a hidden file
        std::size_t dotPos = fileStr.find('.');
        if (dotPos != std::string::npos)
        {
          continue;
        }

        // check if there is only one "-" in the filename
        std::size_t dashPos = fileStr.find('-');
        if (dashPos != std::string::npos)
        {
          std::string postDash = fileStr.substr(dashPos + 1);
          dashPos = postDash.find('-');
          if (dashPos != std::string::npos)
          {
            // more than one dash was found
            continue;
          }
        }

        // filename is basename + four digits, then variable name, then cycle number
        size_t base_length = baseNameStr.str().length();
        std::string varStr =
          fileStr.substr(base_length + 4, fileStr.length() - base_length - 4 - numCycleDigits);

        // check that the cycle is all digits
        std::string cycleStr = fileStr.substr(fileStr.length() - numCycleDigits, numCycleDigits);
        for (std::string::const_iterator it = cycleStr.begin(); it != cycleStr.end(); it++)
        {
          if (!std::isdigit(*it))
          {
            vtkGenericWarningMacro("Expected last six characters of filename to be digits, but "
                                   "found a non-digit chacter. Filename: \'"
              << fileStr << "\' cycleStr: \'" << cycleStr << "\'");
            return 0;
          }
        }

        cycleSet.insert(cycleStr);
        std::ostringstream tempStr2;
        tempStr2 << hdfDirectory[dir] << Slash << fileStr;
        varToFileMap[varStr].push_back(tempStr2.str());
        numFound++;
      }
      if (numFound == 0)
      {
        vtkGenericWarningMacro("HDF directory contains no valid HDF5 files: " << hdfDirectory[dir]);
      }
      numTotalHDFFiles += numFound;
    }
  }

  if (numTotalHDFFiles == 0)
  {
    vtkGenericWarningMacro("No valid HDF5 files were found over all HDF directories");
    return 0;
  }

  this->NumberOfVariables = static_cast<int>(varToFileMap.size());
  this->NumberOfTimeSteps = static_cast<int>(cycleSet.size());

  std::map<std::string, std::vector<std::string>>::iterator miter;
  std::vector<std::string>::iterator viter;
  for (miter = varToFileMap.begin(); miter != varToFileMap.end(); ++miter)
  {
    this->VariableName.push_back(miter->first);
    sort(miter->second.begin(), miter->second.end());
    if ((int)miter->second.size() == this->NumberOfTimeSteps)
    {
      for (viter = miter->second.begin(); viter != miter->second.end(); ++viter)
      {
        this->HdfFileName.push_back(*viter);
      }
    }
    else
    {
      vtkGenericWarningMacro("Missing cycle for var " + miter->first);
    }
  }

  // Move cycle set to double array of steps
  this->TimeSteps = nullptr;
  std::set<std::string>::iterator siter;
  if (this->NumberOfTimeSteps > 0)
  {
    this->TimeSteps = new double[this->NumberOfTimeSteps];
    int step = 0;
    for (siter = cycleSet.begin(); siter != cycleSet.end(); ++siter)
    {
      char* p;
      long cycle = std::strtol(siter->c_str(), &p, 10);
      this->TimeSteps[step] = (double)cycle;
      step++;
    }
  }
  return 1;
}

//------------------------------------------------------------------------------
// Load variable data using files associated with cycle and point selection
//------------------------------------------------------------------------------
void H5RageAdaptor::LoadVariableData(
  vtkImageData* output, int timeStep, vtkDataArraySelection* pointDataArraySelection)
{
  // Add FieldData array for cycle number
  vtkNew<vtkDoubleArray> cycleArray;
  cycleArray->SetName("CycleIndex");
  cycleArray->SetNumberOfComponents(1);
  cycleArray->SetNumberOfTuples(1);
  cycleArray->SetTuple1(0, this->TimeSteps[timeStep]);
  output->GetFieldData()->AddArray(cycleArray);

  bool firstScalar = true;
  for (int var = 0; var < this->NumberOfVariables; var++)
  {
    if (pointDataArraySelection->ArrayIsEnabled(this->VariableName[var].c_str()))
    {
      float* fData = nullptr;
      double* dData = nullptr;

      // Proc 0 reads the HDF file verifying and getting data
      // HDF data was written as column major and must be reversed
      if (this->Rank == 0)
      {
        int offset = var * this->NumberOfTimeSteps + timeStep;
        hid_t file_id = H5Fopen(this->HdfFileName[offset].c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
        hid_t dataset_id = H5Dopen(file_id, "data", hid_t(0));
        hid_t dataspace_id = H5Dget_space(dataset_id);
        hid_t datatype_id = H5Dget_type(dataset_id);
        hid_t ndims = H5Sget_simple_extent_ndims(dataspace_id);
        hsize_t* dims_out = new hsize_t[ndims];
        H5Sget_simple_extent_dims(dataspace_id, dims_out, nullptr);
        int* dimensions = new int[ndims];
        for (int dim = 0; dim < ndims; dim++)
        {
          dimensions[dim] = dims_out[dim];
        }

        hid_t memspace_id;
        if (this->UseFloat64)
        {
          dData = new double[this->TotalTuples];
          memspace_id = H5Screate_simple(ndims, dims_out, nullptr);
          H5Dread(dataset_id, H5T_NATIVE_DOUBLE, memspace_id, dataspace_id, H5P_DEFAULT, dData);
          ConvertHDFData(ndims, dimensions, dData);
        }
        else
        {
          fData = new float[this->TotalTuples];
          memspace_id = H5Screate_simple(ndims, dims_out, nullptr);
          H5Dread(dataset_id, H5T_NATIVE_FLOAT, memspace_id, dataspace_id, H5P_DEFAULT, fData);
          ConvertHDFData(ndims, dimensions, fData);
        }

        H5Tclose(datatype_id);
        H5Dclose(dataset_id);
        H5Sclose(memspace_id);
        H5Sclose(dataspace_id);
        H5Fclose(file_id);
        delete[] dimensions;
        delete[] dims_out;
      }

      // Share the hdf data read among processors according to schedule
      if (this->UseFloat64)
      {
        // First variable is stored in Scalars to eliminate error on Contour
        if (firstScalar)
        {
          firstScalar = false;
          output->GetPointData()->GetScalars()->SetName(pointDataArraySelection->GetArrayName(var));
        }
        vtkNew<vtkDoubleArray> data;
        data->SetName(pointDataArraySelection->GetArrayName(var));
        data->SetNumberOfComponents(1);
        data->SetNumberOfTuples(this->NumberOfTuples[this->Rank]);
        output->GetPointData()->AddArray(data);
        double* varData = data->GetPointer(0);

        if (this->Rank == 0)
        {
          // Store the rank 0 portion of the data
          int pos = 0;
          for (int k = this->ExtentSchedule[0][4]; k <= this->ExtentSchedule[0][5]; k++)
          {
            for (int j = this->ExtentSchedule[0][2]; j <= this->ExtentSchedule[0][3]; j++)
            {
              for (int i = this->ExtentSchedule[0][0]; i <= this->ExtentSchedule[0][1]; i++)
              {
                int indx =
                  (k * this->Dimension[0] * this->Dimension[1]) + (j * this->Dimension[0]) + i;
                varData[pos++] = dData[indx];
              }
            }
          }

          // Buffer for sending to other processors
          for (int rank = 1; rank < this->TotalRank; rank++)
          {
            // Allocate buffer to send and fill with correct range per processor
            double* buffer = new double[this->NumberOfTuples[this->Rank]];
            int pos2 = 0;
            for (int k = this->ExtentSchedule[rank][4]; k <= this->ExtentSchedule[rank][5]; k++)
            {
              for (int j = this->ExtentSchedule[rank][2]; j <= this->ExtentSchedule[rank][3]; j++)
              {
                for (int i = this->ExtentSchedule[rank][0]; i <= this->ExtentSchedule[rank][1]; i++)
                {
                  int indx =
                    (k * this->Dimension[0] * this->Dimension[1]) + (j * this->Dimension[0]) + i;
                  buffer[pos2++] = dData[indx];
                }
              }
            }
            this->Controller->Send(buffer, this->NumberOfTuples[rank], rank, mpiTag);
            delete[] buffer;
          }
        }
        else
        {
          this->Controller->Receive(varData, this->NumberOfTuples[this->Rank], 0, mpiTag);
        }
      }
      else
      {
        // First variable is stored in Scalars to eliminate error on Contour
        if (firstScalar)
        {
          firstScalar = false;
          output->GetPointData()->GetScalars()->SetName(pointDataArraySelection->GetArrayName(var));
        }
        vtkNew<vtkFloatArray> data;
        data->SetName(pointDataArraySelection->GetArrayName(var));
        data->SetNumberOfComponents(1);
        data->SetNumberOfTuples(this->NumberOfTuples[this->Rank]);
        output->GetPointData()->AddArray(data);
        float* varData = data->GetPointer(0);

        if (this->Rank == 0)
        {
          // Store the rank 0 portion of the data
          int pos2 = 0;
          for (int k = this->ExtentSchedule[0][4]; k <= this->ExtentSchedule[0][5]; k++)
          {
            for (int j = this->ExtentSchedule[0][2]; j <= this->ExtentSchedule[0][3]; j++)
            {
              for (int i = this->ExtentSchedule[0][0]; i <= this->ExtentSchedule[0][1]; i++)
              {
                int indx =
                  (k * this->Dimension[0] * this->Dimension[1]) + (j * this->Dimension[0]) + i;
                varData[pos2++] = fData[indx];
              }
            }
          }

          // Buffer for sending to other processors
          for (int rank = 1; rank < this->TotalRank; rank++)
          {
            // Allocate buffer to send and fill with correct range per processor
            float* buffer = new float[this->NumberOfTuples[rank]];
            int pos3 = 0;
            for (int k = this->ExtentSchedule[rank][4]; k <= this->ExtentSchedule[rank][5]; k++)
            {
              for (int j = this->ExtentSchedule[rank][2]; j <= this->ExtentSchedule[rank][3]; j++)
              {
                for (int i = this->ExtentSchedule[rank][0]; i <= this->ExtentSchedule[rank][1]; i++)
                {
                  int indx =
                    (k * this->Dimension[0] * this->Dimension[1]) + (j * this->Dimension[0]) + i;
                  buffer[pos3++] = fData[indx];
                }
              }
            }
            this->Controller->Send(buffer, this->NumberOfTuples[rank], rank, mpiTag);
            delete[] buffer;
          }
        }
        else
        {
          this->Controller->Receive(varData, this->NumberOfTuples[this->Rank], 0, mpiTag);
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
// Convert HDF data into standard row major ordering.
// Depending on whether the data is 3D or 2D, and which 2D slice, data along
// one axis needs to be flipped. This is a bug in Rage that did not get fixed
// for backward compatibility issues.
// Rage data 3D must be flipped on the second dim
// Rage data 2D must be flipped on first dim (will be y in y-z/y-x or z in z-x)
//------------------------------------------------------------------------------
template <class T>
void H5RageAdaptor::ConvertHDFData(int ndims, int* dimensions, T* hdfData)
{
  T* convertedData = new T[this->TotalTuples];

  if (ndims == 3)
  {
    int pos = 0;
    int planeSize = dimensions[1] * dimensions[2];
    int rowSize = dimensions[2];
    for (int k = 0; k < dimensions[0]; k++)
    {
      for (int j = (dimensions[1] - 1); j >= 0; j--)
      {
        for (int i = 0; i < dimensions[2]; i++)
        {
          int indx = (k * planeSize) + (j * rowSize) + i;
          convertedData[pos++] = hdfData[indx];
        }
      }
    }
  }
  else
  {
    int pos = 0;
    int rowSize = dimensions[1];
    for (int j = (dimensions[0] - 1); j >= 0; j--)
    {
      for (int i = 0; i < dimensions[1]; i++)
      {
        int indx = (j * rowSize) + i;
        convertedData[pos++] = hdfData[indx];
      }
    }
  }

  // Copy back to original
  for (int indx = 0; indx < this->TotalTuples; indx++)
  {
    hdfData[indx] = convertedData[indx];
  }
  delete[] convertedData;
}
