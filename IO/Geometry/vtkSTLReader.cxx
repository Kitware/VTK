/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSTLReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSTLReader.h"

#include "vtkByteSwap.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkErrorCode.h"
#include "vtkFloatArray.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMergePoints.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <ctype.h>
#include <vtksys/SystemTools.hxx>

vtkStandardNewMacro(vtkSTLReader);

#define VTK_ASCII 0
#define VTK_BINARY 1

vtkCxxSetObjectMacro(vtkSTLReader,Locator,vtkIncrementalPointLocator);

// Construct object with merging set to true.
vtkSTLReader::vtkSTLReader()
{
  this->FileName = NULL;
  this->Merging = 1;
  this->ScalarTags = 0;
  this->Locator = NULL;

  this->SetNumberOfInputPorts(0);
}

vtkSTLReader::~vtkSTLReader()
{
  this->SetFileName(0);
  this->SetLocator(0);
}

// Overload standard modified time function. If locator is modified,
// then this object is modified as well.
unsigned long vtkSTLReader::GetMTime()
{
  unsigned long mTime1 = this->Superclass::GetMTime();
  unsigned long mTime2;

  if (this->Locator)
    {
    mTime2 = this->Locator->GetMTime();
    mTime1 = ( mTime1 > mTime2 ? mTime1 : mTime2 );
    }

  return mTime1;
}

int vtkSTLReader::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the ouptut
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  FILE *fp;
  vtkPoints *newPts, *mergedPts;
  vtkCellArray *newPolys, *mergedPolys;
  vtkFloatArray *newScalars=0, *mergedScalars=0;

  // All of the data in the first piece.
  if (outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) > 0)
    {
    return 0;
    }

  if (!this->FileName || (this->FileName && (0==strlen(this->FileName))))
    {
    vtkErrorMacro(<<"A FileName must be specified.");
    this->SetErrorCode( vtkErrorCode::NoFileNameError );
    return 0;
    }

  // Initialize
  //
  if ((fp = fopen(this->FileName, "r")) == NULL)
    {
    vtkErrorMacro(<< "File " << this->FileName << " not found");
    this->SetErrorCode( vtkErrorCode::CannotOpenFileError );
    return 0;
    }

  newPts = vtkPoints::New();
  newPolys = vtkCellArray::New();

  // Depending upon file type, read differently
  //
  if ( this->GetSTLFileType(this->FileName) == VTK_ASCII )
    {
    newPts->Allocate(5000,10000);
    newPolys->Allocate(10000,20000);
    if (this->ScalarTags)
      {
      newScalars = vtkFloatArray::New();
      newScalars->Allocate(5000,10000);
      }
    if ( this->ReadASCIISTL(fp,newPts,newPolys,newScalars) )
      {
      return 1;
      }
    }
  else
    {
    fclose(fp);
    fp = fopen(this->FileName, "rb");
    if ( this->ReadBinarySTL(fp,newPts,newPolys) )
      {
      return 1;
      }
    }

  vtkDebugMacro(<< "Read: "
                << newPts->GetNumberOfPoints() << " points, "
                << newPolys->GetNumberOfCells() << " triangles");

  fclose(fp);
  //
  // If merging is on, create hash table and merge points/triangles.
  //
  if ( this->Merging )
    {
    int i;
    vtkIdType *pts = 0;
    vtkIdType nodes[3];
    vtkIdType npts;
    double x[3];
    int nextCell=0;

    mergedPts = vtkPoints::New();
    mergedPts->Allocate(newPts->GetNumberOfPoints()/2);
    mergedPolys = vtkCellArray::New();
    mergedPolys->Allocate(newPolys->GetSize());
    if (newScalars)
      {
      mergedScalars = vtkFloatArray::New();
      mergedScalars->Allocate(newPolys->GetSize());
      }

    vtkSmartPointer<vtkIncrementalPointLocator> locator = this->Locator;
    if (this->Locator == NULL)
      {
      locator.TakeReference(this->NewDefaultLocator());
      }
    locator->InitPointInsertion (mergedPts, newPts->GetBounds());

    for (newPolys->InitTraversal(); newPolys->GetNextCell(npts,pts); )
      {
      for (i=0; i < 3; i++)
        {
        newPts->GetPoint(pts[i],x);
        locator->InsertUniquePoint(x, nodes[i]);
        }

      if ( nodes[0] != nodes[1] &&
           nodes[0] != nodes[2] &&
           nodes[1] != nodes[2] )
        {
        mergedPolys->InsertNextCell(3,nodes);
        if (newScalars)
          {
          mergedScalars->InsertNextValue(newScalars->GetValue(nextCell));
          }
        }
      nextCell++;
      }

    newPts->Delete();
    newPolys->Delete();
    if (newScalars)
      {
      newScalars->Delete();
      }

    vtkDebugMacro(<< "Merged to: "
    << mergedPts->GetNumberOfPoints() << " points, "
    << mergedPolys->GetNumberOfCells() << " triangles");
    }
  else
    {
    mergedPts = newPts;
    mergedPolys = newPolys;
    mergedScalars = newScalars;
    }
//
// Update ourselves
//
  output->SetPoints(mergedPts);
  mergedPts->Delete();

  output->SetPolys(mergedPolys);
  mergedPolys->Delete();

  if (mergedScalars)
    {
    mergedScalars->SetName("STLSolidLabeling");
    output->GetCellData()->SetScalars(mergedScalars);
    mergedScalars->Delete();
    }

  if (this->Locator)
    {
    this->Locator->Initialize(); //free storage
    }

  output->Squeeze();

  return 1;
}

int vtkSTLReader::ReadBinarySTL(FILE *fp, vtkPoints *newPts,
                                vtkCellArray *newPolys)
{
  int i, numTris;
  vtkIdType pts[3];
  unsigned long   ulint;
  unsigned short  ibuff2;
  char    header[81];
  typedef struct  {float  n[3], v1[3], v2[3], v3[3];} facet_t;
  facet_t facet;

  vtkDebugMacro(<< " Reading BINARY STL file");

  //  File is read to obtain raw information as well as bounding box
  //
  if (fread (header, 1, 80, fp) != 80)
    {
    vtkErrorMacro ("STLReader error reading file: " << this->FileName
                   << " Premature EOF while reading header.");
    fclose(fp);
    return 0;
    }
  if (fread (&ulint, 1, 4, fp) != 4)
    {
    vtkErrorMacro ("STLReader error reading file: " << this->FileName
                   << " Premature EOF while reading header.");
    fclose(fp);
    return 0;
    }
  vtkByteSwap::Swap4LE(&ulint);

  // Many .stl files contain bogus count.  Hence we will ignore and read
  //   until end of file.
  //
  if ( (numTris = (int) ulint) <= 0 )
    {
    vtkDebugMacro(<< "Bad binary count: attempting to correct ("
    << numTris << ")");
    }

  // Verify the numTris with the length of the file
  unsigned long ulFileLength = vtksys::SystemTools::FileLength(this->FileName);
  ulFileLength -= (80 + 4); // 80 byte - header, 4 byte - tringle count
  ulFileLength /= 50;       // 50 byte - twelve 32-bit-floating point numbers + 2 byte for attribute byte count

  if (numTris < static_cast<int>(ulFileLength))
    numTris = static_cast<int>(ulFileLength);

  // now we can allocate the memory we need for this STL file
  newPts->Allocate(numTris*3,10000);
  newPolys->Allocate(numTris,20000);

  for ( i=0; fread(&facet,48,1,fp) > 0; i++ )
    {
    if (fread(&ibuff2,2,1,fp) != 1) //read extra junk
      {
      vtkErrorMacro ("STLReader error reading file: " << this->FileName
                     << " Premature EOF while reading extra junk.");
      fclose(fp);
      return 0;
      }

    vtkByteSwap::Swap4LE (facet.n);
    vtkByteSwap::Swap4LE (facet.n+1);
    vtkByteSwap::Swap4LE (facet.n+2);

    vtkByteSwap::Swap4LE (facet.v1);
    vtkByteSwap::Swap4LE (facet.v1+1);
    vtkByteSwap::Swap4LE (facet.v1+2);
    pts[0] = newPts->InsertNextPoint(facet.v1);

    vtkByteSwap::Swap4LE (facet.v2);
    vtkByteSwap::Swap4LE (facet.v2+1);
    vtkByteSwap::Swap4LE (facet.v2+2);
    pts[1] = newPts->InsertNextPoint(facet.v2);

    vtkByteSwap::Swap4LE (facet.v3);
    vtkByteSwap::Swap4LE (facet.v3+1);
    vtkByteSwap::Swap4LE (facet.v3+2);
    pts[2] = newPts->InsertNextPoint(facet.v3);

    newPolys->InsertNextCell(3,pts);

    if ( (i % 5000) == 0 && i != 0 )
      {
      vtkDebugMacro(<< "triangle# " << i);
      this->UpdateProgress(static_cast<double>(i)/numTris);
      }
    }

  return 0;
}

int vtkSTLReader::ReadASCIISTL(FILE *fp, vtkPoints *newPts,
                               vtkCellArray *newPolys, vtkFloatArray *scalars)
{
  char line[256];
  float x[3];
  vtkIdType pts[3];
  int done;
  int currentSolid = 0;

  vtkDebugMacro(<< " Reading ASCII STL file");

  //  Ingest header and junk to get to first vertex
  //
  if (!fgets (line, 255, fp))
    {
    vtkErrorMacro ("STLReader error reading file: " << this->FileName
                   << " Premature EOF while reading header.");
    return 0;
    }

  done = (fscanf(fp,"%s %*s %f %f %f\n", line, x, x+1, x+2)==EOF);
  if ((strcmp(line, "COLOR") == 0) || (strcmp(line, "color") == 0))
    {
      done = (fscanf(fp,"%s %*s %f %f %f\n", line, x, x+1, x+2)==EOF);
    }

  //  Go into loop, reading  facet normal and vertices
  //
//  while (fscanf(fp,"%*s %*s %f %f %f\n", x, x+1, x+2)!=EOF)
  while (!done)
    {
//if (ctr>=253840) {
//    fprintf(stdout, "Reading record %d\n", ctr);
//}
//ctr += 7;
    if (!fgets (line, 255, fp))
      {
      vtkErrorMacro ("STLReader error reading file: " << this->FileName
                     << " Premature EOF while reading header.");
      return 0;
      }

    if (fscanf (fp, "%*s %f %f %f\n", x,x+1,x+2) != 3)
      {
      vtkErrorMacro ("STLReader error reading file: " << this->FileName
                     << " Premature EOF while reading point.");
      fclose(fp);
      return 0;
      }

    pts[0] = newPts->InsertNextPoint(x);
    if (fscanf (fp, "%*s %f %f %f\n", x,x+1,x+2) != 3)
      {
      vtkErrorMacro ("STLReader error reading file: " << this->FileName
                     << " Premature EOF while reading point.");
      fclose(fp);
      return 0;
      }

    pts[1] = newPts->InsertNextPoint(x);
    if (fscanf (fp, "%*s %f %f %f\n", x,x+1,x+2) != 3)
      {
      vtkErrorMacro ("STLReader error reading file: " << this->FileName
                     << " Premature EOF while reading point.");
      fclose(fp);
      return 0;
      }

    pts[2] = newPts->InsertNextPoint(x);
    if (!fgets (line, 255, fp)) // end loop
      {
      vtkErrorMacro ("STLReader error reading file: " << this->FileName
                     << " Premature EOF while reading end loop.");
      fclose(fp);
      return 0;
      }
    if (!fgets (line, 255, fp)) // end facet
      {
      vtkErrorMacro ("STLReader error reading file: " << this->FileName
                     << " Premature EOF while reading end facet.");
      fclose(fp);
      return 0;
      }

    newPolys->InsertNextCell(3,pts);
    if (scalars)
      {
      scalars->InsertNextValue(currentSolid);
      }

    if ( (newPolys->GetNumberOfCells() % 5000) == 0 )
      {
      vtkDebugMacro(<< "triangle# " << newPolys->GetNumberOfCells());
      this->UpdateProgress((newPolys->GetNumberOfCells()%50000)/50000.0);
      }
    done = (fscanf(fp,"%s", line)==EOF);
    if ((strcmp(line, "ENDSOLID") == 0) || (strcmp(line, "endsolid") == 0))
      {
      currentSolid++;
      if (!fgets(line, 255, fp))
        {
        vtkErrorMacro ("STLReader error reading file: " << this->FileName
                       << " Premature EOF while reading solid.");
        fclose(fp);
        return 0;
        }

      done = feof(fp);
      while ((strstr(line, "SOLID") == 0) && (strstr(line, "solid") == 0) && !done)
        {
        if (!fgets(line, 255, fp))
          {
          // if fgets() returns an error, it may be due to the fact that the EOF
          // is reached (BUG #13101) hence we test again.
          done = feof(fp);
          if (!done)
            {
            vtkErrorMacro ("STLReader error reading file: " << this->FileName
              << " Premature EOF while reading end solid.");
            fclose(fp);
            return 0;
            }
          }
        done = feof(fp);
        }

      done = (fscanf(fp,"%s", line)==EOF);
      if ((strstr(line, "COLOR") == 0) || (strstr(line, "color") == 0))
        {
          done = (fscanf(fp,"%f %f %f\n", x,x+1,x+2)==EOF);
          done = done || (fscanf(fp,"%s", line)==EOF);
        }
      }
    if (!done) {
    done = (fscanf(fp,"%*s %f %f %f\n", x, x+1, x+2)==EOF);
    }
    }
  //fprintf(stdout, "Maximum ctr val %d\n", ctr);
  return 0;
}

int vtkSTLReader::GetSTLFileType(const char *filename)
{
  int type;
  vtksys::SystemTools::FileTypeEnum ft =
    vtksys::SystemTools::DetectFileType(filename);
  switch(ft)
    {
  case vtksys::SystemTools::FileTypeBinary:
    type = VTK_BINARY;
    break;
  case vtksys::SystemTools::FileTypeText:
    type = VTK_ASCII;
    break;
  case vtksys::SystemTools::FileTypeUnknown:
    vtkWarningMacro( "File type not recognized attempting binary" );
    type = VTK_BINARY;
    break;
  default:
    vtkErrorMacro( "Case not handled" );
    type = VTK_BINARY; // should not happen
    }
  return type;
}

// Specify a spatial locator for merging points. By
// default an instance of vtkMergePoints is used.
vtkIncrementalPointLocator* vtkSTLReader::NewDefaultLocator()
{
  return vtkMergePoints::New();
}

void vtkSTLReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "File Name: "
     << (this->FileName ? this->FileName : "(none)") << "\n";

  os << indent << "Merging: " << (this->Merging ? "On\n" : "Off\n");
  os << indent << "ScalarTags: " << (this->ScalarTags ? "On\n" : "Off\n");
  os << indent << "Locator: ";
  if ( this->Locator )
    {
    this->Locator->PrintSelf(os<<endl, indent.GetNextIndent());
    }
  else
    {
    os << "(none)\n";
    }
}
