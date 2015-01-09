/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoJSONWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkGeoJSONWriter.h"

#include "vtkCellArray.h"
#include "vtkInformation.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"

#include <vtksys/ios/sstream>

#if _MSC_VER
#define snprintf _snprintf
#endif

vtkStandardNewMacro(vtkGeoJSONWriter);

#define VTK_GJWRITER_MAXPOINTS 32000

class vtkGeoJSONWriter::Internals
{
public:
  Internals()
  {
    this->MaxBufferSize = 128;
    this->Buffer = new char[this->MaxBufferSize];
    this->Top = this->Buffer;
  };
  ~Internals()
  {
    delete[] this->Buffer;
  }
  inline size_t GetSize()
  {
    return this->Top-this->Buffer;
  }
  void Clear()
  {
    this->Top = this->Buffer;
  }
  inline void Grow()
  {
    this->MaxBufferSize*=2;
    //cerr << "GROW " << this->MaxBufferSize << endl;
    char *biggerBuffer = new char[this->MaxBufferSize];
    size_t curSize = this->Top-this->Buffer;
    memcpy(biggerBuffer, this->Buffer, curSize);
    delete[] this->Buffer;
    this->Buffer = biggerBuffer;
    this->Top = this->Buffer+curSize;
  }
  inline void append(const char *newcontent)
  {
    while (this->Top+strlen(newcontent)>=this->Buffer+this->MaxBufferSize)
      {
      this->Grow();
      }
    int nchars = sprintf(this->Top, "%s", newcontent);
    this->Top+=nchars;
  }
  inline void append(const double newcontent)
  {
    snprintf(this->NumBuffer, 64, "%g", newcontent);
    while (this->Top+strlen(NumBuffer)>=this->Buffer+this->MaxBufferSize)
      {
      this->Grow();
      }
     int nchars = sprintf(this->Top, "%s", this->NumBuffer);
     this->Top+=nchars;
  }
  char *Buffer;
  char *Top;
  size_t MaxBufferSize;
  char NumBuffer[64];
};

 //------------------------------------------------------------------------------
 vtkGeoJSONWriter::vtkGeoJSONWriter()
 {
   this->FileName = NULL;
   this->OutputString = NULL;
   this->SetNumberOfOutputPorts(0);
   this->WriteToOutputString = false;
   this->ScalarFormat = 2;
   this->LookupTable = NULL;
   this->WriterHelper = new vtkGeoJSONWriter::Internals();
 }

 //------------------------------------------------------------------------------
 vtkGeoJSONWriter::~vtkGeoJSONWriter()
 {
   this->SetFileName(NULL);
   delete[] this->OutputString;
   this->SetLookupTable(NULL);
   delete this->WriterHelper;
 }

 //------------------------------------------------------------------------------
 void vtkGeoJSONWriter::PrintSelf(ostream & os, vtkIndent indent)
 {
   this->Superclass::PrintSelf(os, indent);
   os << indent << "FileName: "
      << (this->FileName?this->FileName:"NONE") << endl;
   os << indent << "WriteToOutputString: "
      << (this->WriteToOutputString?"True":"False") << endl;
   os << indent << "ScalarFormat: " << this->ScalarFormat << endl;
 }

 //------------------------------------------------------------------------------
 int vtkGeoJSONWriter::FillInputPortInformation(int port, vtkInformation *info)
 {
   if (port == 0)
     {
     info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
     }
   return 1;
 }

 //------------------------------------------------------------------------------
 ostream *vtkGeoJSONWriter::OpenFile()
 {
   vtkDebugMacro(<<"Opening file\n");

   ostream *fptr;

   if (!this->WriteToOutputString)
     {
     if (!this->FileName)
       {
       vtkErrorMacro(<< "No FileName specified! Can't write!");
       return NULL;
       }

     fptr = new ofstream(this->FileName, ios::out);
     }
   else
     {
     // Get rid of any old output string.
     if (this->OutputString)
       {
       delete [] this->OutputString;
       this->OutputString = NULL;
       this->OutputStringLength = 0;
       }
     fptr = new vtksys_ios::ostringstream;
     }

   if (fptr->fail())
     {
     vtkErrorMacro(<< "Unable to open file: "<< this->FileName);
     delete fptr;
     return NULL;
     }

   return fptr;
 }

 //------------------------------------------------------------------------------
 void vtkGeoJSONWriter::CloseFile(ostream *fp)
 {
   vtkDebugMacro(<<"Closing file\n");

   if ( fp != NULL )
     {
     if (this->WriteToOutputString)
       {
       vtksys_ios::ostringstream *ostr =
         static_cast<vtksys_ios::ostringstream*>(fp);

       delete [] this->OutputString;
       this->OutputStringLength = static_cast<int>(ostr->str().size());
       //+1's account for null terminator
       this->OutputString = new char[ostr->str().size()+1];
       memcpy(this->OutputString, ostr->str().c_str(),
         this->OutputStringLength+1);
       }

     delete fp;
     }
 }

 //------------------------------------------------------------------------------
 void vtkGeoJSONWriter::ConditionalComma(
   vtkIdType cnt, vtkIdType limit)
 {
   if (cnt+1 != limit)
     {
     this->WriterHelper->append(",");
     }
 }

 //------------------------------------------------------------------------------
 void vtkGeoJSONWriter::WriteScalar(
   vtkDataArray *da, vtkIdType ptId)
 {
   if (this->ScalarFormat == 0)
   {
     return;
   }
   if (da)
   {
     double b = da->GetTuple1(ptId);
     if (this->ScalarFormat == 1)
     {
       vtkLookupTable *lut = this->GetLookupTable();
       if (!lut)
       {
         lut = vtkLookupTable::New();
         lut->SetNumberOfColors(256);
         lut->SetHueRange(0.0,0.667);
         lut->SetRange(da->GetRange());
         lut->Build();
         this->SetLookupTable(lut);
         lut->Delete();
       }
       unsigned char *color = lut->MapValue(b);
       this->WriterHelper->append(",");
       this->WriterHelper->append((double)color[0]/255.0);
       this->WriterHelper->append(",");
       this->WriterHelper->append((double)color[1]/255.0);
       this->WriterHelper->append(",");
       this->WriterHelper->append((double)color[2]/255.0);
     }
     else
     {
       if (vtkMath::IsNan(b))
         {
         this->WriterHelper->append(",null");
         }
       else
         {
         this->WriterHelper->append(",");
         this->WriterHelper->append(b);
         }
     }
   }
 }

 //------------------------------------------------------------------------------
 void vtkGeoJSONWriter::WriteData()
 {
   ostream *fp;
   vtkPolyData *input = vtkPolyData::SafeDownCast(this->GetInput());

   vtkDebugMacro(<<"Writing vtk polygonal data to geojson file...");
   fp=this->OpenFile();
   if ( !fp )
     {
     return;
     }

   this->WriterHelper->append("{\n");
   this->WriterHelper->append("\"type\": \"Feature\",\n");
   vtkDataArray *da = input->GetPointData()->GetScalars();
   if (!da)
     {
     da = input->GetPointData()->GetArray(0);
     }
   if (da)
     {
     switch (this->ScalarFormat)
       {
       case 0:
         this->WriterHelper->append("\"properties\": {\"ScalarFormat\": \"none\"},\n");
         break;
       case 1:
         this->WriterHelper->append("\"properties\": {\"ScalarFormat\": \"rgb\"},\n");
         break;
       case 2:
         double rng[2];
         da->GetRange(rng);
         this->WriterHelper->append("\"properties\": {\"ScalarFormat\": \"values\", \"ScalarRange\": [");
         this->WriterHelper->append(rng[0]);
         this->WriterHelper->append(",");
         this->WriterHelper->append(rng[1]);
         this->WriterHelper->append("] },\n");
         break;
       }
     }
   else
     {
     this->WriterHelper->append("\"properties\": {\"ScalarFormat\": \"none\"},\n");
     }
   this->WriterHelper->append("\"geometry\":\n");
   this->WriterHelper->append("{\n");
   this->WriterHelper->append("\"type\": \"GeometryCollection\",\n");
   this->WriterHelper->append("\"geometries\":\n");
   this->WriterHelper->append("[\n");

   vtkIdType cellLoc = 0;
   vtkIdType *cellPts = NULL;
   vtkIdType cellSize = 0;
   vtkIdType numlines, numpolys;
   numlines = input->GetLines()->GetNumberOfCells();
   numpolys = input->GetPolys()->GetNumberOfCells();

   //VERTS
   vtkCellArray *ca;
   ca = input->GetVerts();
   if (ca && ca->GetNumberOfCells())
     {
     bool done = false;
     vtkIdType inCell = 0;
     vtkIdType ptCnt = 0;
     do //loop to break into sections with < VTK_GJWRITER_MAXPOINTS points
       {
       this->WriterHelper->append("{\n");
       this->WriterHelper->append("\"type\": \"MultiPoint\",\n");
       this->WriterHelper->append("\"coordinates\":\n");
       this->WriterHelper->append("[\n");
       for (; inCell < ca->GetNumberOfCells() && ptCnt < VTK_GJWRITER_MAXPOINTS; inCell++)
         {
         ca->GetCell(cellLoc, cellSize, cellPts);
         cellLoc += cellSize+1;
         ptCnt += cellSize;
         vtkIdType inPt;
         for (inPt = 0; inPt < cellSize; inPt++)
           {
           double coords[3];
           input->GetPoint(cellPts[inPt], coords);
           this->WriterHelper->append("[");
           for (int i=0; i<3; i++)
             {
             if (vtkMath::IsNan(coords[i]))
               {
               this->WriterHelper->append("null");
               }
             else
               {
               this->WriterHelper->append(coords[i]);
               }
             if (i!=2)
               {
               this->WriterHelper->append(",");
               }
             }
           this->WriteScalar(da, cellPts[inPt]);
           this->WriterHelper->append("]");
           this->ConditionalComma(inPt, cellSize);
           }
         if (ptCnt<VTK_GJWRITER_MAXPOINTS)
           {
           this->ConditionalComma(inCell, ca->GetNumberOfCells());
           }
         this->WriterHelper->append("\n");
         }
       this->WriterHelper->append("]"); //coordinates for this cell array
       if (inCell < ca->GetNumberOfCells())
         {
         ptCnt = 0;
         this->WriterHelper->append(",\n");
         }
       else
         {
         if (numlines || numpolys)
           {
           this->WriterHelper->append(",");
           }
         done = true;
         }
       } while (!done);
     }

   //lines
   ca = input->GetLines();
   if (ca && ca->GetNumberOfCells())
     {
     bool done = false;
     vtkIdType inCell = 0;
     vtkIdType ptCnt = 0;
     do //loop to break into sections with < VTK_GJWRITER_MAXPOINTS points
       {
       this->WriterHelper->append("{\n");
       this->WriterHelper->append("\"type\": \"MultiLineString\",\n");
       this->WriterHelper->append("\"coordinates\":\n");
       this->WriterHelper->append("[\n");
       for (; inCell < ca->GetNumberOfCells() && ptCnt < VTK_GJWRITER_MAXPOINTS; inCell++)
         {
         this->WriterHelper->append("[ "); //one cell
         ca->GetCell(cellLoc, cellSize, cellPts);
         cellLoc += cellSize+1;
         ptCnt += cellSize;
         vtkIdType inPt;
         for (inPt = 0; inPt < cellSize; inPt++)
           {
           double coords[3];
           input->GetPoint(cellPts[inPt], coords);
           this->WriterHelper->append("[");
           for (int i =0; i<3; i++)
             {
             if (vtkMath::IsNan(coords[i]))
               {
               this->WriterHelper->append("null");
               }
             else
               {
               this->WriterHelper->append(coords[i]);
               }
             if (i!=2)
               {
               this->WriterHelper->append(",");
               }
             }
           this->WriteScalar(da, cellPts[inPt]);
           this->WriterHelper->append("]");
           this->ConditionalComma(inPt, cellSize);
           }
         this->WriterHelper->append("]");//one cell
         if (ptCnt<VTK_GJWRITER_MAXPOINTS)
           {
           this->ConditionalComma(inCell, ca->GetNumberOfCells());
           }
         this->WriterHelper->append("\n");
         }
       this->WriterHelper->append("]"); //coordinates for this cell array
       this->WriterHelper->append("\n");
       this->WriterHelper->append("}\n"); //this cell array
       if (inCell < ca->GetNumberOfCells())
         {
         ptCnt = 0;
         this->WriterHelper->append(",\n");
         }
       else
         {
         if (numpolys)
           {
           this->WriterHelper->append(",");
           }
         done = true;
         }
       } while (!done);
     }
   //polygons
   ca = input->GetPolys();
   if (ca && ca->GetNumberOfCells())
     {
     bool done = false;
     vtkIdType inCell = 0;
     vtkIdType ptCnt = 0;
     do //loop to break into sections with < VTK_GJWRITER_MAXPOINTS points
       {
       this->WriterHelper->append("{\n");
       this->WriterHelper->append("\"type\": \"MultiPolygon\",\n");
       this->WriterHelper->append("\"coordinates\":\n");
       this->WriterHelper->append("[\n");
       for (; inCell < ca->GetNumberOfCells() && ptCnt < VTK_GJWRITER_MAXPOINTS; inCell++)
         {
         this->WriterHelper->append("[[ "); //one cell
         ca->GetCell(cellLoc, cellSize, cellPts);
         cellLoc += cellSize+1;
         ptCnt += cellSize;
         vtkIdType inPt;
         for (inPt = 0; inPt < cellSize; inPt++)
           {
           double coords[3];
           input->GetPoint(cellPts[inPt], coords);
           this->WriterHelper->append("[");
           for (int i =0; i<3; i++)
             {
             if (vtkMath::IsNan(coords[i]))
               {
               this->WriterHelper->append("null");
               }
             else
               {
               this->WriterHelper->append(coords[i]);
               }
             if (i!=2)
               {
               this->WriterHelper->append(",");
               }
             }
           this->WriteScalar(da, cellPts[inPt]);
           this->WriterHelper->append("]");
           this->ConditionalComma(inPt, cellSize);
           }
         this->WriterHelper->append(" ]]");//one cell
         if (ptCnt<VTK_GJWRITER_MAXPOINTS)
           {
           this->ConditionalComma(inCell, ca->GetNumberOfCells());
           }
         this->WriterHelper->append("\n");
         }
       this->WriterHelper->append("]"); //coordinates for this cell array
       this->WriterHelper->append("\n");
       this->WriterHelper->append("}\n"); //this cell array
       if (inCell < ca->GetNumberOfCells())
         {
         ptCnt = 0;
         this->WriterHelper->append(",\n");
         }
       else
         {
         done = true;
         }
       } while (!done);
     }

   this->WriterHelper->append("]\n");//feature.geometry.GeometryCollection.geometries
   this->WriterHelper->append("}\n");//feature.geometry
   this->WriterHelper->append("}\n");//feature

   fp->write(this->WriterHelper->Buffer, this->WriterHelper->GetSize());
   this->WriterHelper->Clear();

   fp->flush();
   if (fp->fail())
     {
     vtkErrorMacro("Problem writing result check disk space.");
     delete fp;
     fp = NULL;
     }

   this->CloseFile(fp);
}

//------------------------------------------------------------------------------
char *vtkGeoJSONWriter::RegisterAndGetOutputString()
{
  char *tmp = this->OutputString;

  this->OutputString = NULL;
  this->OutputStringLength = 0;

  return tmp;
}

//------------------------------------------------------------------------------
vtkStdString vtkGeoJSONWriter::GetOutputStdString()
{
  return vtkStdString(this->OutputString, this->OutputStringLength);
}

//------------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkGeoJSONWriter, LookupTable, vtkLookupTable)
