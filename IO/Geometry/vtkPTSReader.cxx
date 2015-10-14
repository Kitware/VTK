/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPTSReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPTSReader.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkCellArray.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"

vtkStandardNewMacro(vtkPTSReader);

//----------------------------------------------------------------------------
vtkPTSReader::vtkPTSReader() :
  FileName(NULL),
  OutputDataTypeIsDouble(false),
  LimitReadToBounds(false),
  LimitToMaxNumberOfPoints(false),
  MaxNumberOfPoints(1000000)
{
  this->SetNumberOfInputPorts(0);
  this->ReadBounds[0] = this->ReadBounds[2] = this->ReadBounds[4] = VTK_DOUBLE_MAX;
  this->ReadBounds[1] = this->ReadBounds[3] = this->ReadBounds[5] = VTK_DOUBLE_MIN;

}

//----------------------------------------------------------------------------
vtkPTSReader::~vtkPTSReader()
{
  if (this->FileName)
    {
    delete [] this->FileName;
    this->FileName = NULL;
    }
}

//-----------------------------------------------------------------------------
// vtkSetStringMacro except we clear some variables if we update the value
void vtkPTSReader::SetFileName(const char *filename)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting FileName to " << filename );
  if (this->FileName == NULL && filename == NULL)
    {
    return;
    }
  if (this->FileName && filename && !strcmp(this->FileName, filename))
    {
    return;
    }
  if (this->FileName)
    {
    delete [] this->FileName;
    }
  if (filename)
    {
    size_t n = strlen(filename) + 1;
    char *cp1 =  new char[n];
    const char *cp2 = (filename);
    this->FileName = cp1;
    do
      {
      *cp1++ = *cp2++;
      } while ( --n );
    }
   else
    {
    this->FileName = NULL;
    }
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkPTSReader::
RequestInformation(vtkInformation *vtkNotUsed(request),
                   vtkInformationVector **vtkNotUsed(inputVector),
                   vtkInformationVector *vtkNotUsed(outputVector))
{
  if (!this->FileName)
    {
    vtkErrorMacro("FileName has to be specified!");
    return 0;
    }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkPTSReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "File Name: "
     << (this->FileName ? this->FileName : "(none)") << "\n";
  os << indent << "OutputDataType = "
     << (this->OutputDataTypeIsDouble ? "double" : "float") << "\n";

  if (this->LimitReadToBounds)
    {
    os << indent << "LimitReadToBounds = true\n";
    os << indent << "ReadBounds = [" << this->ReadBounds[0] << ","<< this->ReadBounds[1] << ","
       << this->ReadBounds[2] << this->ReadBounds[3] << ","<< this->ReadBounds[4] << ","
       << this->ReadBounds[5]<< "]\n";
    }
  else
    {
    os << indent << "LimitReadToBounds = false\n";
    }

  if (this->LimitToMaxNumberOfPoints)
    {
    os << indent << "LimitToMaxNumberOfPoints = true\n";
    os << indent << "MaxNumberOfPoints" << MaxNumberOfPoints << "\n";
    }
  else
    {
    os << indent << "LimitToMaxNumberOfPoints = false\n";
    }
}

//-----------------------------------------------------------------------------
int vtkPTSReader::
RequestData(vtkInformation *vtkNotUsed(request),
            vtkInformationVector **vtkNotUsed(inputVector),
            vtkInformationVector *outputVector)
{
  // See if we can open in the file
  if (!this->FileName)
    {
    vtkErrorMacro(<<"FileName must be specified.");
    return 0;
    }

  // Open the new file.
  vtkDebugMacro(<< "Opening file " << this->FileName);
  ifstream file(this->FileName, ios::in | ios::binary);
  if (!file || file.fail())
    {
    vtkErrorMacro(<< "Could not open file " <<
    this->FileName);
    return 0;
    }

  // Determine the number of points to be read in which should be
  // a single int at the top of the file
  const unsigned int bufferSize = 2048;
  std::string buffer;
  char junk[bufferSize];
  vtkTypeInt32 numPts = -1, tempNumPts;
  for (numPts = -1; !file.eof();)
    {
    getline(file, buffer);
    // Scanf should match the interger part but not the string
    int numArgs = sscanf(buffer.c_str(), "%d%s", &tempNumPts, junk);
    if (numArgs == 1)
      {
      numPts = static_cast<vtkTypeInt32>(tempNumPts);
      break;
      }
    if (numArgs != -1)
      {
      // We have a file that doesn't have a number of points line
      // Instead we need to count the number of lines in the file
      // Remember we already read in the first line hence numPts starts
      // at 1
      for (numPts = 1; getline(file, buffer); ++numPts)
        {
        }
      file.clear();
      file.seekg(0);
      break;
      }
    }

  // Next determine the format the point info. Is it x y z,
  // x y z intensity or
  // or x y z intensity r g b?
  int numValuesPerLine;
  float intensity;
  double rgb[3], pt[3];

  if (numPts == -1)
    {
    vtkErrorMacro(<< "Could not process file " <<
                  this->FileName << " - Unknown Format");
    return 0;
    }
  else if (numPts == 0)
    {
    // Trivial case of no points - lets set it to 3
    numValuesPerLine = 3;
    }
  else
    {
    getline(file, buffer);
    numValuesPerLine = sscanf(buffer.c_str(), "%lf %lf %lf %f %lf %lf %lf",
                              pt, pt+1, pt+2,
                              &intensity, rgb, rgb+1, rgb+2);
    }

  if (!((numValuesPerLine == 3) ||
        (numValuesPerLine == 4) ||
        (numValuesPerLine == 7)))
    {
    // Unsupported line format!
      vtkErrorMacro(<< "Invalid Pts Format (point info has "
                    << numValuesPerLine << ") in the file:"
                    << this->FileName);
      return 0;
    }

  // Lets setup the VTK Arrays and Points
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the ouptut
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkNew<vtkPoints> newPts;
  if (this->OutputDataTypeIsDouble)
    {
    newPts->SetDataTypeToDouble();
    }
  else
    {
    newPts->SetDataTypeToFloat();
    }
  newPts->Allocate(numPts);

  vtkNew<vtkCellArray> newVerts;
  vtkNew<vtkUnsignedCharArray> colors;
  vtkNew<vtkFloatArray> intensities;
  output->SetPoints( newPts.GetPointer() );
  output->SetVerts( newVerts.GetPointer() );

  if (numValuesPerLine == 7)
    {
    colors->SetNumberOfComponents(3);
    colors->SetName("Color");
    colors->Allocate(numPts*3);
    output->GetPointData()->SetScalars( colors.GetPointer());
    }

  if (numValuesPerLine > 3)
    {
    intensities->SetName("Intensities");
    intensities->SetNumberOfComponents(1);
    intensities->Allocate(numPts);
    output->GetPointData()->AddArray( intensities.GetPointer());
    }

  this->UpdateProgress(0);
  if (this->GetAbortExecute())
    {
    this->UpdateProgress( 1.0 );
    return 1;
    }


  // setup the ReadBBox, IF we're limiting the read to specifed ReadBounds
  if (this->LimitReadToBounds)
    {
    this->ReadBBox.Reset();
    this->ReadBBox.SetMinPoint(this->ReadBounds[0], this->ReadBounds[2],
      this->ReadBounds[4]);
    this->ReadBBox.SetMaxPoint(this->ReadBounds[1], this->ReadBounds[3],
      this->ReadBounds[5]);
    // the ReadBBox is guaranteed to be "valid", regardless of the whether
    // ReadBounds is valid.  If any of the MonPoint values are greater than
    // the corresponding MaxPoint, the MinPoint component will be set to be
    // the same as the MaxPoint during the SetMaxPoint fn call.
    }

  if (numPts == 0)
    {
    // we are done
    return 1;
    }

  // If we are trying to limit the max number of points calculate the
  // onRatio - else set it to 1
  int onRatio = 1;
  if (this->LimitToMaxNumberOfPoints)
    {
    onRatio =  ceil(static_cast<double>(numPts) / this->MaxNumberOfPoints);
    }

  // Lets Process the points!  Remember that we have already loaded in
  // the first line of points in buffer
  vtkIdType *pids = new vtkIdType[numPts], pid;
  for (long i = 0; i < numPts; i++)
    {
    // Should we process this point?  Meaning that we skipped the appropriate number of points
    // based on the Max Number of points (onRatio) or the filtering by the read bounding box
    // OK to process based on Max Number of Points
    if ((i % onRatio) == 0)
      {
      sscanf(buffer.c_str(), "%lf %lf %lf %f %lf %lf %lf",
             pt, pt+1, pt+2,
             &intensity, rgb, rgb+1, rgb+2);
      // OK to process based on bounding box
      if ((!this->LimitReadToBounds) || this->ReadBBox.ContainsPoint(pt))
        {
        pid = newPts->InsertNextPoint(pt);
        //std::cerr << "Point " << i << " : " << pt[0] << " " << pt[1] << " " << pt[2] << "\n";
        pids[pid] = pid;
        if (numValuesPerLine > 3)
          {
          intensities->InsertNextValue(intensity);
          }
        if (numValuesPerLine == 7)
          {
          colors->InsertNextTuple(rgb);
          }
        }
      }
    if (file.eof())
      {
      break;
      }
    getline(file, buffer);
    }

  // Do we have to squeeze any of the arrays?
  if (newPts->GetNumberOfPoints() < numPts)
    {
    newPts->Squeeze();
    if (numValuesPerLine > 3)
      {
      intensities->Squeeze();
      }
    if (numValuesPerLine > 7)
      {
      colors->Squeeze();
      }
    }
  newVerts->InsertNextCell(newPts->GetNumberOfPoints(), pids);
  delete [] pids;
  return 1;
}
