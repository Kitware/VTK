/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPOPReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include <ctype.h>
#include <math.h>
#include "vtkPOPReader.h"
#include "vtkMath.h"
#include "vtkExtentTranslator.h"
#include "vtkFloatArray.h"
#include "vtkImageReader.h"
#include "vtkImageWrapPad.h"
#include "vtkObjectFactory.h"

//-------------------------------------------------------------------------
vtkPOPReader* vtkPOPReader::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPOPReader");
  if(ret)
    {
    return (vtkPOPReader*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPOPReader;
}

//----------------------------------------------------------------------------
vtkPOPReader::vtkPOPReader()
{
  this->Radius = 60000.0;
  
  this->Dimensions[0] = 3600;
  this->Dimensions[1] = 2400;
  
  this->GridFileName = NULL;
  this->FileName = NULL;
  
  this->NumberOfArrays = 0;
  this->MaximumNumberOfArrays = 0;
  this->ArrayNames = NULL;
  this->ArrayFileNames = NULL;
  this->ArrayOffsets = NULL;
  this->ArrayFileDimensionality = 3;

  this->UFlowFileName = NULL;
  this->UFlowFileOffset = 0;
  this->VFlowFileName = NULL;
  this->VFlowFileOffset = 0;

  this->DepthValues = vtkFloatArray::New();
} 

//----------------------------------------------------------------------------
vtkPOPReader::~vtkPOPReader()
{ 
  this->SetFileName(NULL);
  this->SetGridFileName(NULL);
  this->DeleteArrays();
  
  this->DepthValues->Delete();
  this->DepthValues = NULL;
}


//----------------------------------------------------------------------------
void vtkPOPReader::DeleteArrays()
{
  int i;

  for (i = 0; i < this->NumberOfArrays; ++i)
    {
    if (this->ArrayNames && this->ArrayNames[i])
      {
      delete [] this->ArrayNames[i];
      this->ArrayNames[i] = NULL;
      }
    if (this->ArrayFileNames && this->ArrayFileNames[i])
      {
      delete [] this->ArrayFileNames[i];
      this->ArrayFileNames[i] = NULL;
      }
    }
  if (this->ArrayNames)
    {
    delete [] this->ArrayNames;
    this->ArrayNames = NULL;
    }
  if (this->ArrayFileNames)
    {
    delete [] this->ArrayFileNames;
    this->ArrayFileNames = NULL;
    }
  if (this->ArrayOffsets)
    {
    delete [] this->ArrayOffsets;
    this->ArrayOffsets = NULL;
    }

  this->NumberOfArrays = 0;
  this->MaximumNumberOfArrays = 0;
}

//----------------------------------------------------------------------------
void vtkPOPReader::AddArray(char *arrayName, char *fileName, unsigned long offset)
{
  if (this->NumberOfArrays == this->MaximumNumberOfArrays)
    {
    int idx;
    char **tmp1, **tmp2;
    unsigned long *tmp3;

    this->MaximumNumberOfArrays += 20;
    tmp1 = new char*[this->MaximumNumberOfArrays];
    tmp2 = new char*[this->MaximumNumberOfArrays];
    tmp3 = new unsigned long[this->MaximumNumberOfArrays];
    for (idx = 0; idx < this->NumberOfArrays; ++idx)
      {
      tmp1[idx] = this->ArrayNames[idx];
      tmp2[idx] = this->ArrayFileNames[idx];
      tmp3[idx] = this->ArrayOffsets[idx];
      }
    delete [] this->ArrayNames;
    this->ArrayNames = tmp1;
    delete [] this->ArrayFileNames;
    this->ArrayFileNames = tmp2;
    delete [] this->ArrayOffsets;
    this->ArrayOffsets = tmp3;
    }
  
  this->ArrayNames[this->NumberOfArrays] = new char[strlen(arrayName)+1];
  strcpy(this->ArrayNames[this->NumberOfArrays], arrayName);

  this->ArrayFileNames[this->NumberOfArrays] = new char[strlen(fileName)+1];
  strcpy(this->ArrayFileNames[this->NumberOfArrays], fileName);

  this->ArrayOffsets[this->NumberOfArrays] = offset;

  ++this->NumberOfArrays;
}


//----------------------------------------------------------------------------
void vtkPOPReader::ExecuteInformation()
{
  int xDim, yDim, zDim;
  
  this->ReadInformationFile();  
  
  xDim = this->Dimensions[0]+1;
  yDim = this->Dimensions[1];
  zDim = this->DepthValues->GetNumberOfTuples();
  
  this->GetOutput()->SetWholeExtent(0, xDim-1, 0, yDim-1, 0, zDim-1);
}

//----------------------------------------------------------------------------
void vtkPOPReader::Execute()
{
  vtkStructuredGrid *output;
  vtkPoints *points;
  vtkImageData *image;
  vtkDataArray *array;
  int ext[6];
  int i;

  output = this->GetOutput();
  
  cout << "Reading POP file\n";
  cout.flush();

  // Set up the extent of the grid image.
  ext[0] = ext[2] = ext[4] = 0;
  ext[1] = this->Dimensions[0]-1;
  ext[3] = this->Dimensions[1]-1;
  ext[5] = 1;
  
  vtkImageReader *reader = vtkImageReader::New();
  reader->SetFileDimensionality(3);
  reader->SetDataExtent(ext);
  reader->SetFileName(this->GridFileName);
  reader->SetDataByteOrderToBigEndian();
  reader->SetNumberOfScalarComponents(1);
  reader->SetDataScalarTypeToDouble();
  reader->SetHeaderSize(0);
  vtkImageWrapPad *wrap = vtkImageWrapPad::New();
  wrap->SetInput(reader->GetOutput());
  ++ext[1];
  wrap->SetOutputWholeExtent(ext);

  image = wrap->GetOutput();
  output->GetUpdateExtent(ext);
  output->SetExtent(ext);
  ext[4] = 0;
  ext[5] = 1;
  image->SetUpdateExtent(ext);
  image->Update();
  
  // Create the grid points from the grid image.
  points = this->ReadPoints(image);
  output->SetPoints(points);
  points->Delete();
  points = NULL;
  
  // Now read in the arrays.
  // Set up the extent of the grid image.
  ext[0] = ext[2] = ext[4] = 0;
  ext[1] = this->Dimensions[0]-1;
  ext[3] = this->Dimensions[1]-1;
  ext[5] = this->DepthValues->GetNumberOfTuples()-1;
  reader->SetDataExtent(ext);
  reader->SetDataScalarTypeToFloat();
  reader->SetFileDimensionality(this->ArrayFileDimensionality);
  ++ext[1];
  wrap->SetOutputWholeExtent(ext);
  for (i = 0; i < this->NumberOfArrays; ++i)
    {
    if (this->ArrayFileNames[i] && this->ArrayNames[i])
      {
      if (this->ArrayFileDimensionality == 3)
        {
        reader->SetFileName(this->ArrayFileNames[i]);
        }
      else if (this->ArrayFileDimensionality == 2)
        {
        reader->SetFilePattern("%s.%02d");
        reader->SetFilePrefix(this->ArrayFileNames[i]);
        }
      else
        {
        vtkErrorMacro("FileDimensionality can only be 2 or 3.");
        reader->Delete();
        wrap->Delete();
        return;
        }
      reader->SetHeaderSize(this->ArrayOffsets[i] * 4 
			    * this->Dimensions[0] * this->Dimensions[1]);
      // Just in case.
      //reader->SetHeaderSize(0);
      output->GetUpdateExtent(ext);
      image = wrap->GetOutput();
      image->SetUpdateExtent(ext);
      image->Update();
      array = image->GetPointData()->GetScalars()->GetData();
      array->SetName(this->ArrayNames[i]);
      
      output->GetPointData()->AddArray(array);
      image->ReleaseData();
      }
    }
  reader->Delete();
  reader = NULL;
  wrap->Delete();
  wrap = NULL;

  // If there is flow defined.
  this->ReadFlow();
}


//----------------------------------------------------------------------------
vtkPoints *vtkPOPReader::GeneratePoints()
{
  /*
  vtkPoints *points;
  vtkImageData *temp;
  double x, y, z, radius;
  double theta, phi;
  int i, j;
  int *wholeExt;
  int xDim, yDim;
  int *ext;  
  int id;
  
  //temp = this->GetInput();
  wholeExt = temp->GetWholeExtent();
  if (wholeExt[0] != 0 || wholeExt[2] != 0 || wholeExt[4] != 0)
    {
    vtkErrorMacro("Expecting whole extent to start at 0.");
    return NULL;
    }
  xDim = wholeExt[1]+1;
  yDim = wholeExt[3]+1;  
  ext = temp->GetExtent();
  
  points = vtkPoints::New();
  points->Allocate(xDim*yDim);
  id = 0;
  radius = 20000.0;
  for (j = ext[2]; j <= ext[3]; ++j)
    {
    phi = (double)j * vtkMath::Pi() / (double)(yDim);
    for (i = ext[0]; i <= ext[1]; ++i)
      {
      theta = (double)i * 2.0 * vtkMath::Pi() / (double)(xDim);
      y = cos(phi)*radius;
      x = sin(theta)*sin(phi)*radius;
      z = cos(theta)*sin(phi)*radius;
      points->SetPoint(id, x, y, z);
      ++id;
      }
    }
  
  return points;
  */
  return NULL;
}




//----------------------------------------------------------------------------
vtkPoints *vtkPOPReader::ReadPoints(vtkImageData *image)
{
  vtkPoints *points;
  double x, y, z, depth, radius;
  double theta, phi;
  int i, j, k;
  int id, num;
  // The only different between these two is the z extent.
  // We should probably ditch ext and user update extent to make things simpler.
  int *updateExt = this->GetOutput()->GetUpdateExtent();
  int *ext = image->GetExtent();
  
  points = vtkPoints::New();
  num = (ext[1]-ext[0]+1)*(ext[3]-ext[2]+1)*(updateExt[5]-updateExt[4]+1);
  points->Allocate(num);
  points->SetNumberOfPoints(num);
  
  id = 0;
  for (k = updateExt[4]; k <= updateExt[5]; ++k)
    {
    depth = this->DepthValues->GetValue(k);
    radius = this->Radius - depth;
    for (j = ext[2]; j <= ext[3]; ++j)
      {
      for (i = ext[0]; i <= ext[1]; ++i)
	{      
	phi = (double)(image->GetScalarComponentAsFloat(i, j, 0, 0));
	theta = (double)(image->GetScalarComponentAsFloat(i, j, 1, 0));
	phi += vtkMath::Pi()/2.0;
	y = -cos(phi)*radius;
	x = sin(theta)*sin(phi)*radius;
	z = cos(theta)*sin(phi)*radius;
	points->SetPoint(id, x, y, z);
	++id;
	}
      }
    }
  
  return points;
}


//==================== Stuff for reading the pop file ========================



//----------------------------------------------------------------------------
void vtkPOPReader::ReadInformationFile()
{
  ifstream *file;
  int i, num;
  float tempf;
  char str[256];

  this->DeleteArrays();
  this->DepthValues->Reset();
  this->SetUFlowFileName(NULL);
  this->SetVFlowFileName(NULL);
  this->UFlowFileOffset = this->VFlowFileOffset = 0;
  file = new ifstream(this->FileName, ios::in);
  
  while (1)
    {
    // Read Key
    *file >> str;
    if (file->fail())
      {
      file->close();
      delete file;
      return;
      }
    
    if (strcmp(str, "Dimensions") == 0)
      {
      *file >> num;
      this->Dimensions[0] = num;
      *file >> num;
      this->Dimensions[1] = num;
      }

    else if (strcmp(str, "GridFileName") == 0)
      {
      *file >> str;
      this->SetGridName(str);
      }

    else if (strcmp(str, "NumberOfArrays") == 0)
      {
      char str2[256];
      unsigned long offset;
      *file >> num;
      for (i = 0; i < num; ++i)
        {
        *file >> str;
        *file >> str2;
        *file >> offset;
        if (file->fail())
          {
          vtkErrorMacro("Error reading array name " << i);    
          delete file;
          return;
          }
        this->AddArrayName(str, str2, offset);
        }
      }

    else if (strcmp(str, "ArrayFileDimensionality") == 0)
      {
      *file >> num;
      if (file->fail())
        {
        vtkErrorMacro("Error reading array name " << i);    
        delete file;
        return;
        }
      this->ArrayFileDimensionality = num;
      }

    else if (strcmp(str, "Flow") == 0)
      {
      char *tmp = NULL;
      char str2[256];
      unsigned long offset;
      *file >> num;
      for (i = 0; i < num; ++i)
        {
        *file >> str;
        *file >> str2;
        *file >> offset;

        if (file->fail())
          {
          vtkErrorMacro("Error reading flow component " << i);    
          delete file;
          return;
          }
  
        if (strcmp(str,"u") == 0)
          {
          if (str2[0] != '/' && str2[1] != ':')
            {
            tmp = this->MakeFileName(str2);
            this->SetUFlowFileName(tmp);
            delete [] tmp;
            }
          else
            {
            this->SetUFlowFileName(str2);
            }
          this->UFlowFileOffset = offset;
          }
        else if (strcmp(str,"v") == 0)
          {
          if (str2[0] != '/' && str2[1] != ':')
            {
            tmp = this->MakeFileName(str2);
            this->SetVFlowFileName(tmp);
            delete [] tmp;
            }
          else
            {
            this->SetVFlowFileName(str2);
            }
          this->VFlowFileOffset = offset;
          }
        }
      }

    else if (strcmp(str, "NumberOfDepthValues") == 0)
      {
      *file >> num;
      for (i = 0; i < num; ++i)
        {
        *file >> str;
        if (file->fail())
          {
          vtkErrorMacro("Error reading depth value " << i);    
          delete file;
          return;
          }
        tempf = atof(str);
        this->DepthValues->InsertNextValue(tempf);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkPOPReader::SetGridName(char *name)
{
  if (name[0] == '/' || name[1] == ':')
    {
    this->SetGridFileName(name);
    return;
    }
  
  char *tmp;
  
  tmp = this->MakeFileName(name);
  this->SetGridFileName(tmp);
  delete [] tmp;
}

//----------------------------------------------------------------------------
// This will append the full path onto the file name. (using the grid path/
void vtkPOPReader::AddArrayName(char *name, char *fileName, unsigned long offset)
{
  char *tmp;

  if (fileName[0] == '/' || fileName[1] == ':')
    {
    this->AddArray(name, fileName, offset);
    return;
    }
  
  tmp = this->MakeFileName(fileName);
  this->AddArray(name, tmp, offset);
  delete [] tmp;
}

//----------------------------------------------------------------------------
int vtkPOPReader::IsFileName(char *name)
{
  while (name && *name)
    {
    if (*name == '/')
      {
      return 1;
      }
    ++name;
    }
  
  return 0;  
}

//----------------------------------------------------------------------------
char *vtkPOPReader::MakeFileName(char *name)
{
  char *fileName;
  char *tmp1;
  char *tmp2;
  char *start;
  
  if (name == NULL)
    {
    vtkErrorMacro("No name.");
    return NULL;
    }
  
  if (this->FileName == NULL)
    {
    fileName = new char[strlen(name) + 1];
    strcpy(fileName, name);
    return fileName;
    }
  
  fileName = new char[strlen(this->FileName) + strlen(name) + 1];
  tmp1 = this->FileName;
  tmp2 = fileName;
  start = fileName;
  while (tmp1 && *tmp1)
    {
    *tmp2 = *tmp1;
    if (*tmp1 == '/')
      {
      start = tmp2+1;
      }
    ++tmp1;
    ++tmp2;
    }
  
  strcpy(start, name);
  
  return fileName;
}

  

//----------------------------------------------------------------------------
void vtkPOPReader::ReadFlow()
{
  vtkStructuredGrid *output;
  vtkDataArray *array;
  vtkImageData *uImage, *vImage, *fImage;
  int updateExt[6], wholeExt[6], ext[6];
  int idx, u, v, w;
  float *pf, *pu, *pv;
  float v0, v2, w0, u0, u2;
  float du, dv, dw;
  int uvInc0, uvInc1, uvInc2;
  int vMin, vMax, wMax;
  vtkPoints *pts;
  float *pp;
  int pfInc1, pfInc2;
  float nv[3], axisW[3], axisV[3], axisU[3];
  float tmp;

  if (this->UFlowFileName == NULL || this->VFlowFileName == NULL)
    {
    return;
    }

  output = this->GetOutput();
  pts = output->GetPoints();

  ext[0] = ext[2] = ext[4] = 0;
  ext[1] = this->Dimensions[0]-1;
  ext[3] = this->Dimensions[1]-1;
  ext[5] = this->DepthValues->GetNumberOfTuples()-1;

  vtkImageReader *reader = vtkImageReader::New();
  reader->SetFileDimensionality(this->ArrayFileDimensionality);
  reader->SetDataExtent(ext);
  reader->SetDataByteOrderToBigEndian();
  reader->SetNumberOfScalarComponents(1);
  reader->SetDataScalarTypeToFloat();
  reader->SetHeaderSize(0);
  vtkImageWrapPad *wrap = vtkImageWrapPad::New();
  wrap->SetInput(reader->GetOutput());
  // To complete the last row (shared with the first row).
  ++ext[1];
  // We will need ghost cells. Poles are discontinuities.
  // U is cyclical
  --ext[0];
  ++ext[1];
  wrap->SetOutputWholeExtent(ext);
  
  // Figure out what extent we need for the request.
  wrap->GetOutputWholeExtent(wholeExt);
  output->GetUpdateExtent(updateExt);
  if (wholeExt[5] != updateExt[5])
    {
    vtkErrorMacro("Requested extent does not have bottom slice required for correct completion of the flow vectors.");
    }

  output->GetUpdateExtent(ext);
  for (idx = 0; idx < 3; ++idx)
    {
    --ext[idx*2];
    ++ext[idx*2+1];
    if (ext[idx*2] < wholeExt[idx*2])
      {
      ext[idx*2] = wholeExt[idx*2];
      }
    if (ext[idx*2+1] > wholeExt[idx*2+1])
      {
      ext[idx*2+1] = wholeExt[idx*2+1];
      }
    }

  uImage = vtkImageData::New();
  // Set up the reader with the appropriate filename.
      if (this->ArrayFileDimensionality == 3)
        {
        reader->SetFileName(this->UFlowFileName);
        }
      else if (this->ArrayFileDimensionality == 2)
        {
        reader->SetFilePattern("%s.%02d");
        reader->SetFilePrefix(this->UFlowFileName);
        }
      else
        {
        vtkErrorMacro("FileDimensionality can only be 2 or 3.");
        reader->Delete();
        wrap->Delete();
        return;
        }
  reader->SetHeaderSize(this->UFlowFileOffset * 4 
                  * this->Dimensions[0] * this->Dimensions[1]);
  wrap->SetOutput(uImage);
  uImage->SetUpdateExtent(ext);
  uImage->Update();

  vImage = vtkImageData::New();
  // Set up the reader with the appropriate filename.
      if (this->ArrayFileDimensionality == 3)
        {
        reader->SetFileName(this->VFlowFileName);
        }
      else if (this->ArrayFileDimensionality == 2)
        {
        reader->SetFilePattern("%s.%02d");
        reader->SetFilePrefix(this->VFlowFileName);
        }
      else
        {
        vtkErrorMacro("FileDimensionality can only be 2 or 3.");
        reader->Delete();
        wrap->Delete();
        return;
        }
  reader->SetHeaderSize(this->VFlowFileOffset * 4 
                  * this->Dimensions[0] * this->Dimensions[1]);
  wrap->SetOutput(vImage);
  vImage->SetUpdateExtent(ext);
  vImage->Update();

  uvInc0 = 1;
  uvInc1 = ext[1]-ext[0]+1;
  uvInc2 = uvInc1 * (ext[3]-ext[2]+1);
  vMin = ext[2];
  vMax = ext[3];
  wMax = ext[5];

  reader->Delete();
  reader = NULL;
  wrap->Delete();
  wrap = NULL;

  fImage = vtkImageData::New();
  fImage->SetExtent(updateExt);
  fImage->SetNumberOfScalarComponents(3);
  fImage->SetScalarType(VTK_FLOAT);
  fImage->AllocateScalars();

  pfInc1 = 3*(updateExt[1]-updateExt[0]+1);
  pfInc2 = (updateExt[1]-updateExt[0]+1)*(updateExt[3]-updateExt[2]+1)*3;

  // Central differences is good, but I do not like it for the
  // z/propagation direction (alternation).  Normal difference
  // produces a shift.  As a start, I will use it any way.
  // I could always average the top and bottom versions...

  // Now do the computation from bottom to top.
  // Since dw is uniform across a level, forget about traversing the points
  // back to front.  Just do the first slice always.
  pp = pts->GetPoint(0);  
  for (v = updateExt[2]; v <= updateExt[3]; ++v)
    {
    for (u = updateExt[0]; u <= updateExt[1]; ++u)
      {
      // Loose a little efficiency here, but ...
      pf = (float*)fImage->GetScalarPointer(u, v, updateExt[5]);
      pu = (float*)uImage->GetScalarPointer(u, v, updateExt[5]);
      pv = (float*)vImage->GetScalarPointer(u, v, updateExt[5]);
      // Find the coordinate transform (and deltas as a side effect).
      // Except for dw, these are constant up a column.
      // W is just the noramlize vector 0->p.
      axisW[0] = pp[0];
      axisW[1] = pp[1];
      axisW[2] = pp[2];
      vtkMath::Normalize(axisW);
      // Ignore curvature of earth surface. Handle boundaries.
      if (v == updateExt[2])
        {
        axisV[0] = pp[0] - pp[pfInc1];
        axisV[1] = pp[1] - pp[1+pfInc1];
        axisV[2] = pp[2] - pp[2+pfInc1];
        }
      else
        {
        axisV[0] = pp[-pfInc1] - pp[0];
        axisV[1] = pp[1-pfInc1] - pp[1];
        axisV[2] = pp[2-pfInc1] - pp[2];
        }
      dv = vtkMath::Normalize(axisV);
      // Last U axis.
      if (u == updateExt[0])
        {
        axisU[0] =  pp[3] - pp[0];
        axisU[1] =  pp[4] - pp[1];
        axisU[2] =  pp[5] - pp[2];
        }
      else
        {
        axisU[0] =  pp[0] - pp[-3];
        axisU[1] =  pp[1] - pp[-2];
        axisU[2] =  pp[2] - pp[-1];
        }
      du = vtkMath::Normalize(axisU);

      // Since the points are not used in the inner most loop,
      // move to the next point here.      
      pp += 3;     

      // Now sum the w flow up the column.
      w0 = 0.0;
      for (w = updateExt[5]; w >= updateExt[4]; --w)
        {
        // Find dw (easy because we have the depth values in an array).
        if (w == 0)
          {
          if (this->DepthValues->GetNumberOfTuples() <= 1)
            {
            dw = 0.0;
            }
          else
            {
            dw = this->DepthValues->GetValue(1) - this->DepthValues->GetValue(0);
            }
          }
        else
          {
          dw = this->DepthValues->GetValue(w) - this->DepthValues->GetValue(w-1);
          }

        // Compute w componenet of flow ...
        // Find the five important values for this point (have w0 already).
        // U is circular so does not have boundary checks.
        v0 = v2 = 0.0;
        u0 = pu[-uvInc0];
        u2 = pu[uvInc0];
        if (v < vMax)
          {
          v0 = pv[uvInc1];
          }
        if (v > vMin)
          {
          v2 = pv[-uvInc1];
          }

        // Now fill the vector for this point (uvw coords).
        pf[0] =  *pu;  // i.e. u1
        pf[1] =  *pv;  // i.e. v1
        tmp = 0.5 * (((u0-u2)*dv*dw + (v0-v2)*du*dw)/(du*dv));
        pf[2] = w0 + tmp;
        //pf[2] = 0.0;

        // Before we transform and loose w1, 
        // save it for summing in the next iteration.
        w0 = pf[2];

        // Now compute the vector in the new coordinate system.
        nv[0] = axisU[0]*pf[0] + axisV[0]*pf[1] + axisW[0]*pf[2];
        nv[1] = axisU[1]*pf[0] + axisV[1]*pf[1] + axisW[1]*pf[2];
        nv[2] = axisU[2]*pf[0] + axisV[2]*pf[1] + axisW[2]*pf[2];
        pf[0] = nv[0];
        pf[1] = nv[1];
        pf[2] = nv[2];

        // Move up the column to the next point.
        pf -= pfInc2;
        pu -= uvInc2;
        pv -= uvInc2;
        }
      }
    }
    
  // Delete 
  uImage->Delete();
  vImage->Delete();

  array = fImage->GetPointData()->GetScalars()->GetData();
  array->SetName("Flow");
      
  output->GetPointData()->AddArray(array);
  fImage->ReleaseData();
  fImage->Delete();
}






  
//----------------------------------------------------------------------------
void vtkPOPReader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredGridSource::PrintSelf(os,indent);

}

