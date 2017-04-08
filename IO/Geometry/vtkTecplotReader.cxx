/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTecplotReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*****************************************************************************
*
* Copyright (c) 2000 - 2009, Lawrence Livermore National Security, LLC
* Produced at the Lawrence Livermore National Laboratory
* LLNL-CODE-400124
* All rights reserved.
*
* This file was adapted from the ASCII Tecplot reader of VisIt. For  details,
* see https://visit.llnl.gov/.  The full copyright notice is contained in the
* file COPYRIGHT located at the root of the VisIt distribution or at
* http://www.llnl.gov/visit/copyright.html.
*
*****************************************************************************/

#include "vtkTecplotReader.h"

#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkCallbackCommand.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkCompositeDataPipeline.h"

#include "vtkPoints.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnsignedCharArray.h"
#include "vtkDataArraySelection.h"

#include "vtk_zlib.h"
#include <vtksys/SystemTools.hxx>

#include <fstream>

#include <cctype> // for isspace(), isalnum()

vtkStandardNewMacro( vtkTecplotReader );


// ============================================================================
class FileStreamReader
{
public:
  FileStreamReader();
  ~FileStreamReader();

  bool open(const char* fileName);
  bool is_open()const {return Open;}
  bool eof()const {return Eof;}

  void rewind();
  void close();
  int get();
  bool operator !() const;

protected:
  bool Open;
  bool Eof;
  static const int BUFF_SIZE = 2048;
  char buff[BUFF_SIZE];
  int Pos;
  int BuffEnd;
  gzFile file;
  std::string FileName;

};

// ----------------------------------------------------------------------------
FileStreamReader::FileStreamReader()
  : Open(false),Eof(true),Pos(BUFF_SIZE),BuffEnd(BUFF_SIZE),FileName()
{

}

// ----------------------------------------------------------------------------
FileStreamReader::~FileStreamReader()
{
  this->close();
}

// ----------------------------------------------------------------------------
bool FileStreamReader::open( const char* fileName )
{
  if ( !this->Open )
  {
    this->FileName = std::string(fileName);
    //zlib handles both compressed and uncompressed file
    //we just have peak into the file and see if it has the magic
    //flags or not
    unsigned char magic[2];
    FILE *ff = fopen(fileName,"rb");
    size_t count = fread(magic,1,2,ff);
    fclose(ff);

    // only continue if fread succeeded
    if (count == 2)
    {
      const char* mode = (magic[0] == 0x1f && magic[1] == 0x8b) ? "rb" : "r";
      this->file = gzopen(fileName,mode);

      this->Eof = (this->file == 0);
      this->Open = (this->file != 0);
      this->Pos = BUFF_SIZE;
    }
  }
  return this->Open;
}
// ----------------------------------------------------------------------------
int FileStreamReader::get()
{
  if (!this->is_open() || this->eof() )
  {
    return this->eof();
  }

  //when reading uncompressed data, zlib will return if it hits
  //and eol character

  if (this->Pos >= this->BuffEnd)
  {
    this->Pos = 0;
    //read the first buffer
    this->BuffEnd = gzread(this->file,this->buff,this->BUFF_SIZE);
    //assign EOF to what gzread returned
    this->Eof = (this->BuffEnd <= 0);
    if (this->Eof)
    {
      return this->Eof;
    }
  }
  return this->buff[this->Pos++];
}

// ----------------------------------------------------------------------------
void FileStreamReader::rewind( )
{
  if ( this->Open )
  {
    //we don't want to use gzrewind as it rewinds to not the start of the
    //file, but to start of the data in the file, meaning we are past any
    //comments or headers.
    std::string fn = this->FileName;
    this->close();
    this->open(fn.c_str());
  }
}

// ----------------------------------------------------------------------------
void FileStreamReader::close()
{
  if ( this->Open )
  {
    this->Open = false;
    this->Eof = false;
    this->Pos = this->BUFF_SIZE;
    this->BuffEnd = this->BUFF_SIZE;
    this->FileName = std::string();

    gzclose(this->file);
  }
}

// ----------------------------------------------------------------------------
bool FileStreamReader::operator!() const
{
  return this->Eof;
}

// ==========================================================================//
class vtkTecplotReaderInternal
{
public:
  vtkTecplotReaderInternal()  { this->Init(); }
  ~vtkTecplotReaderInternal() { this->Init(); }

  int     XIdInList;
  int     YIdInList;
  int     ZIdInList;
  int     Completed;
  int     GeometryDim;
  int     TopologyDim;
  char    TheNextChar;
  bool    NextCharEOF;
  bool    NextCharEOL;
  bool    NextCharValid;
  bool    TokenIsString;
  bool    IsCompressed;
  FileStreamReader ASCIIStream;
  std::string TokenBackup;

public:
  void Init()
  {
    this->Completed =  0;
    this->XIdInList = -1;
    this->YIdInList = -1;
    this->ZIdInList = -1;

    this->TopologyDim   = 0;
    this->GeometryDim   = 1;
    this->TheNextChar   = '\0';
    this->TokenBackup   = "";
    this->NextCharEOF   = false;
    this->NextCharEOL   = false;
    this->NextCharValid = false;
    this->TokenIsString = false;
    this->IsCompressed  = false;
  }

  // This functions obtains the next token from the ASCII stream.
  // Note that it is assumed that the ASCII stream is ready and no
  // reading error occurs.
  std::string GetNextToken()
  {
    // this is where we take a one-token lookahead
    if ( !this->TokenBackup.empty() )
    {
      std::string  retval = this->TokenBackup;
      this->TokenBackup = "";
      return  retval;
    }

    // oops!  we hit EOF and someone still wants more.
    if ( this->NextCharEOF )
    {
      return "";
    }

    this->NextCharEOL   = false;
    this->TokenIsString = false;

    std::string  retval = "";
    if ( !this->NextCharValid )
    {
      this->TheNextChar   = this->ASCIIStream.get();
      this->NextCharValid = true;

      if ( !this->ASCIIStream )
      {
        this->NextCharEOF = true;
      }
    }

    //if the token is a comment token, skip the entire line
    if (!this->NextCharEOF && this->TheNextChar == '#')
    {
      while ( (!this->NextCharEOF) &&
                (this->TheNextChar != '\n') &&
                (this->TheNextChar != '\r') )
      {
        this->TheNextChar = this->ASCIIStream.get();
        if ( this->TheNextChar == '\n'||
             this->TheNextChar == '\r' )
        {
          this->NextCharEOL = true;
        }
      }
    }

    // skip inter-token whitespace
    while ( ! this->NextCharEOF &&
            ( this->TheNextChar == ' '  ||
              this->TheNextChar == '\n' ||
              this->TheNextChar == '\r' ||
              this->TheNextChar == '\t' ||
              this->TheNextChar == '='  ||
              this->TheNextChar == '('  ||
              this->TheNextChar == ')'  ||
              this->TheNextChar == ','
            )
          )
    {
      if ( this->TheNextChar == '\n' || this->TheNextChar == '\r' )
      {
        this->NextCharEOL = true;
      }

      this->TheNextChar = this->ASCIIStream.get();
      if ( !this->ASCIIStream )
      {
        this->NextCharEOF = true;
      }

      // Ignore blank lines since they don't return a token
      if ( this->NextCharEOL )
      {
        return this->GetNextToken();
      }
    }

    if ( this->TheNextChar == '\"' )
    {
      this->TokenIsString = true;
      this->TheNextChar = this->ASCIIStream.get();
      if ( !this->ASCIIStream )
      {
        this->NextCharEOF = true;
      }

      while ( !this->NextCharEOF && this->TheNextChar != '\"' )
      {
        retval += this->TheNextChar;
        this->TheNextChar = this->ASCIIStream.get();

        if ( !this->ASCIIStream )
        {
          this->NextCharEOF = true;
        }
      }

      this->TheNextChar = this->ASCIIStream.get();
      if ( !this->ASCIIStream )
      {
        this->NextCharEOF = true;
      }
    }
    else
    {
      // handle a normal token
      while ( ! this->NextCharEOF &&
              ( this->TheNextChar != ' '  &&
                this->TheNextChar != '\n' &&
                this->TheNextChar != '\r' &&
                this->TheNextChar != '\t' &&
                this->TheNextChar != '='  &&
                this->TheNextChar != '('  &&
                this->TheNextChar != ')'  &&
                this->TheNextChar != ','
              )
            )
      {
        if ( this->TheNextChar >= 'a' && this->TheNextChar <= 'z' )
        {
          this->TheNextChar += (  int( 'A' ) - int( 'a' )  );
        }

        retval += this->TheNextChar;
        this->TheNextChar = this->ASCIIStream.get();

        if ( !this->ASCIIStream )
        {
          this->NextCharEOF = true;
        }
      }
    }

    // skip whitespace to EOL
    while ( ! this->NextCharEOF &&
            ( this->TheNextChar == ' '  ||
              this->TheNextChar == '\n' ||
              this->TheNextChar == '\r' ||
              this->TheNextChar == '\t' ||
              this->TheNextChar == '='  ||
              this->TheNextChar == '('  ||
              this->TheNextChar == ')'  ||
              this->TheNextChar == ','
            )
          )
    {
      if ( this->TheNextChar == '\n' || this->TheNextChar == '\r' )
      {
        this->NextCharEOL = true;
      }

      this->TheNextChar = this->ASCIIStream.get();
      if ( !this->ASCIIStream )
      {
        this->NextCharEOF = true;
      }

      if ( this->NextCharEOL )
      {
        break;
      }
    }
    return retval;
  }

private:

  vtkTecplotReaderInternal( const vtkTecplotReaderInternal & ) VTK_DELETE_FUNCTION;
  void operator = ( const vtkTecplotReaderInternal & ) VTK_DELETE_FUNCTION;
};
// ==========================================================================//

// ----------------------------------------------------------------------------
//                         Supporting Functions (begin)
// ----------------------------------------------------------------------------

#ifndef MAX
#define MAX( a, b ) (  (a) > (b) ? (a) : (b)  )
#endif

// ----------------------------------------------------------------------------
static int GetCoord( const std::string & theToken )
{
  if ( theToken == "X" || theToken == "x" || theToken == "I" || theToken == "CoordinateX" )
  {
    return 0;
  }

  if ( theToken == "Y" || theToken == "y" || theToken == "J" || theToken == "CoordinateY" )
  {
    return 1;
  }

  if ( theToken == "Z" || theToken == "z" || theToken == "K" || theToken == "CoordinateZ" )
  {
    return 2;
  }

  return -1;
}

// ----------------------------------------------------------------------------
static int GuessCoord( const std::string & theToken )
{
  int  guessVal = GetCoord( theToken );

  if ( theToken.length() >= 3 )
  {
    // do match: "x[m]" or "x (m)", etc. don't match: "x velocity"
    if (  (  !isspace( theToken[1] ) && !isalnum( theToken[1] )  ) ||
          (   isspace( theToken[1]   && !isalnum( theToken[2] )  )   )
       )
    {
      guessVal = GetCoord(  theToken.substr( 0, 1 )  );
    }
  }

  return guessVal;
}

// ----------------------------------------------------------------------------
static std::string SimplifyWhitespace( const std::string & s )
{
  int headIndx = 0;
  int tailIndx = int( s.length() ) - 1;

  while (  headIndx < tailIndx && ( s[headIndx] == ' ' || s[headIndx] == '\t' )  )
  {
    headIndx ++;
  }

  while (  tailIndx > headIndx && ( s[tailIndx]  == ' ' || s[tailIndx]  == '\t' )  )
  {
    tailIndx --;
  }

  return s.substr( headIndx, tailIndx - headIndx + 1 );
}

// ----------------------------------------------------------------------------
//                         Supporting Functions ( end )
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
vtkTecplotReader::vtkTecplotReader()
{
  this->SelectionObserver  = vtkCallbackCommand::New();
  this->SelectionObserver->SetClientData( this );
  this->SelectionObserver->SetCallback
        ( &vtkTecplotReader::SelectionModifiedCallback );

  this->DataArraySelection = vtkDataArraySelection::New();
  this->DataArraySelection->AddObserver( vtkCommand::ModifiedEvent,
                                         this->SelectionObserver );

  this->FileName = NULL;
  this->Internal = new vtkTecplotReaderInternal;
  this->SetNumberOfInputPorts( 0 );

  this->Init();
}

// ----------------------------------------------------------------------------
vtkTecplotReader::~vtkTecplotReader()
{
  this->Init();

  delete [] this->FileName;

  delete this->Internal;
  this->Internal = NULL;

  this->DataArraySelection->RemoveAllArrays();
  this->DataArraySelection->RemoveObserver( this->SelectionObserver );
  this->DataArraySelection->Delete();
  this->DataArraySelection = NULL;

  this->SelectionObserver->SetClientData( NULL );
  this->SelectionObserver->SetCallback( NULL );
  this->SelectionObserver->Delete();
  this->SelectionObserver  = NULL;
}

// ----------------------------------------------------------------------------
void vtkTecplotReader::Init()
{
  // do NOT address this->FileName in this function !!!

  this->DataTitle         = "";
  this->NumberOfVariables = 0;
  this->CellBased.clear();
  this->ZoneNames.clear();
  this->Variables.clear();

  this->Internal->Init();
}

// ----------------------------------------------------------------------------
void vtkTecplotReader::SetFileName( const char * fileName )
{
  if (    fileName
       && strcmp( fileName, "" )
       && ( ( this->FileName == NULL ) || strcmp( fileName, this->FileName ) )
     )
  {
    delete [] this->FileName;
    this->FileName = new char[  strlen( fileName ) + 1  ];
    strcpy( this->FileName, fileName );
    this->FileName[ strlen( fileName ) ] = '\0';

    this->Modified();
    this->Internal->Completed = 0;
  }
}

//----------------------------------------------------------------------------
void vtkTecplotReader::SelectionModifiedCallback( vtkObject *, unsigned long,
                                                  void * tpReader, void * )
{
  static_cast< vtkTecplotReader * >( tpReader )->Modified();
}

// ----------------------------------------------------------------------------
int vtkTecplotReader::FillOutputPortInformation
  (  int vtkNotUsed( port ),  vtkInformation * info  )
{
  info->Set( vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet" );
  return 1;
}

// ----------------------------------------------------------------------------
int vtkTecplotReader::RequestInformation( vtkInformation * request,
                                          vtkInformationVector ** inputVector,
                                          vtkInformationVector  * outputVector )
{
  if(  !this->Superclass::RequestInformation
        ( request, inputVector, outputVector )
    )
  {
    return 0;
  }

  this->GetDataArraysList();

  return 1;
}

// ----------------------------------------------------------------------------
int vtkTecplotReader::RequestData( vtkInformation *        vtkNotUsed( request ),
                                   vtkInformationVector ** vtkNotUsed( inputVector ),
                                   vtkInformationVector *  outputVector )
{
  vtkInformation *       outInf = outputVector->GetInformationObject( 0 );
  vtkMultiBlockDataSet * output = vtkMultiBlockDataSet::SafeDownCast
                                  (  outInf->Get( vtkDataObject::DATA_OBJECT() )  );

  this->Internal->Completed = 0;
  this->ReadFile( output );
  outInf = NULL;
  output = NULL;

  return 1;
}

// ----------------------------------------------------------------------------
const char * vtkTecplotReader::GetDataTitle()
{
  return this->DataTitle.c_str();
}

// ----------------------------------------------------------------------------
int   vtkTecplotReader::GetNumberOfBlocks()
{
  return int( this->ZoneNames.size() );
}

// ----------------------------------------------------------------------------
const char * vtkTecplotReader::GetBlockName( int blockIdx )
{
  if (  blockIdx < 0  ||  blockIdx >= int( this->ZoneNames.size() )  )
  {
    return NULL;
  }

  return this->ZoneNames[ blockIdx ].c_str();
}

// ----------------------------------------------------------------------------
int  vtkTecplotReader::GetNumberOfDataAttributes()
{
  return
    this->NumberOfVariables - (   !(  !( this->Internal->XIdInList + 1 )  )   )
                            - (   !(  !( this->Internal->YIdInList + 1 )  )   )
                            - (   !(  !( this->Internal->ZIdInList + 1 )  )   );
}

// ----------------------------------------------------------------------------
const char * vtkTecplotReader::GetDataAttributeName( int attrIndx )
{
  if ( attrIndx < 0 && attrIndx >= this->GetNumberOfDataAttributes() )
  {
    return NULL;
  }

  return this->Variables[ attrIndx +
                          this->Variables.size() -
                          this->GetNumberOfDataAttributes()
                        ].c_str();
}

// ----------------------------------------------------------------------------
int   vtkTecplotReader::IsDataAttributeCellBased( int attrIndx )
{
  int  cellBasd  =-1;
  if ( attrIndx >= 0 && attrIndx < this->GetNumberOfDataAttributes() )
  {
    // the if-statement ensures that this->CellBased has been ready
    cellBasd = this->CellBased[ attrIndx +
                                this->CellBased.size() -
                                this->GetNumberOfDataAttributes()
                              ];
  }

  return cellBasd;
}

// ----------------------------------------------------------------------------
int   vtkTecplotReader::IsDataAttributeCellBased( const char * attrName )
{
  int  cellBasd = -1;
  int  varIndex = -1;

  if ( attrName )
  {
    for ( unsigned int i = 0; i < this->Variables.size(); i ++ )
    {
      if (  strcmp( this->Variables[i].c_str(), attrName ) == 0  )
      {
        varIndex = i;
        break;
      }
    }

    cellBasd = ( varIndex == -1 ) ? -1: this->CellBased[ varIndex ];
  }

  return cellBasd;
}

//-----------------------------------------------------------------------------
int vtkTecplotReader::GetNumberOfDataArrays()
{
  return this->DataArraySelection->GetNumberOfArrays();
}

//-----------------------------------------------------------------------------
const char* vtkTecplotReader::GetDataArrayName( int arrayIdx )
{
  return this->DataArraySelection->GetArrayName( arrayIdx );
}

//-----------------------------------------------------------------------------
int vtkTecplotReader::GetDataArrayStatus( const char * arayName )
{
  return this->DataArraySelection->ArrayIsEnabled( arayName );
}

//-----------------------------------------------------------------------------
void vtkTecplotReader::SetDataArrayStatus( const char * arayName, int bChecked )
{
  vtkDebugMacro( "Set cell array \"" << arayName
                 << "\" status to: " << bChecked );

  if( bChecked )
  {
    this->DataArraySelection->EnableArray( arayName );
  }
  else
  {
    this->DataArraySelection->DisableArray( arayName );
  }
}

// ----------------------------------------------------------------------------
void vtkTecplotReader::PrintSelf( ostream & os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );

  os << indent << "DataTitle: "          << this->DataTitle         << endl;
  os << indent << "Size of CellBased: "  << this->CellBased.size()  << endl;
  os << indent << "Size of ZoneNames: "  << this->ZoneNames.size()  << endl;
  os << indent << "Size of Variables: "  << this->Variables.size()  << endl;
  os << indent << "NumberOfVariables: "  << this->NumberOfVariables << endl;
}

// ----------------------------------------------------------------------------
void vtkTecplotReader::GetArraysFromPointPackingZone
  ( int numNodes, vtkPoints * theNodes, vtkPointData * nodeData )
{
  // NOTE: The Tecplot ASCII file format mandates that cell data of any zone be
  // stored in block-packing mode (VARLOCATION, pp. 158, Tecplot 360 Data Format
  // Guide 2009). Thus we do not need to consider any cell data in this function.

  if ( !theNodes || !nodeData ||
    !this->Internal->ASCIIStream.is_open()
     )
  {
    vtkErrorMacro( << "File not open, errors with reading, or NULL vtkPoints /"
                   << "vtkPointData." );
    return;
  }

  int     n,  v;
  int     zArrayId;        // indexing zoneData
  int     cordBase;        // offset of a 3D-coordinate triple in cordsPtr
  int     isXcoord;
  int     isYcoord;
  int     isZcoord;
  int   * anyCoord; // is any coordinate?
  int   * coordIdx; // index of the coordinate array, just in case
  int   * selected; // is a selected data array?
  float * arrayPtr = NULL;
  float   theValue;
  vtkFloatArray * theArray = NULL;
  std::vector< float * > pointers;
  std::vector< vtkFloatArray * > zoneData;

  pointers.clear();
  zoneData.clear();

  // geoemtry: 3D point coordinates (note that this array must be initialized
  // since only 2D coordinates might be provided by a Tecplot file)
  theNodes->SetNumberOfPoints( numNodes );
  float * cordsPtr = static_cast< float * > (  theNodes->GetVoidPointer( 0 )  );
  memset( cordsPtr, 0, sizeof( float ) * 3 * numNodes );

  // three arrays used to determine the role of each variable (including
  // the coordinate arrays)
  anyCoord = new int[ this->NumberOfVariables ];
  coordIdx = new int[ this->NumberOfVariables ];
  selected = new int[ this->NumberOfVariables ];

  // allocate arrays only if necessary to load the zone data
  for ( v = 0; v < this->NumberOfVariables; v ++ )
  {
    isXcoord    = int(  !( v - this->Internal->XIdInList )  );
    isYcoord    = int(  !( v - this->Internal->YIdInList )  );
    isZcoord    = int(  !( v - this->Internal->ZIdInList )  );
    anyCoord[v] = isXcoord + isYcoord + isZcoord;
    coordIdx[v] = isYcoord + ( isZcoord << 1 );
    selected[v] = this->DataArraySelection
                      ->ArrayIsEnabled( this->Variables[v].c_str() );

    if ( anyCoord[v] + selected[v] )
    {
      theArray = vtkFloatArray::New();
      theArray->SetNumberOfTuples( numNodes );
      theArray->SetName( this->Variables[v].c_str() );
      zoneData.push_back( theArray );
      arrayPtr = static_cast< float * > (  theArray->GetVoidPointer( 0 )  );
      pointers.push_back( arrayPtr );
      arrayPtr = NULL;
      theArray = NULL;
    }
  }

  // load the zone data (number of tuples <= number of points / nodes)
  for ( n = 0; n < numNodes; n ++ )
  {
    cordBase = ( n << 1 ) + n;

    zArrayId = 0;
    for ( v = 0; v < this->NumberOfVariables; v ++ )
    {
      // obtain a value that is either a coordinate or a selected attribute
      if ( anyCoord[v] || selected[v] )
      {
        theValue = atof( this->Internal->GetNextToken().c_str() );
        pointers[ zArrayId ++ ][n] = theValue;

        // collect the coordinate
        if ( anyCoord[v] )
        {
          cordsPtr[ cordBase + coordIdx[v] ] = theValue;
        }
      }
      else
      {
        // a value of an un-selected data array
        this->Internal->GetNextToken();
      }
    }
  }
  cordsPtr = NULL;

  // attach the node-based data attributes to the grid
  zArrayId = 0;
  for ( v = 0; v < this->NumberOfVariables; v ++ )
  {
    if ( !anyCoord[v] && selected[v] )
    {
      nodeData->AddArray( zoneData[zArrayId] );
    }

    zArrayId += int(    !(  !( anyCoord[v] + selected[v] )  )    );
  }

  pointers.clear();

  //remove all the float arrays from vector so they won't leak
  for ( unsigned int i=0; i < zoneData.size(); ++i)
  {
    vtkFloatArray *fa = zoneData.at(i);
    if ( fa )
    {
      fa->FastDelete();
    }
  }
  zoneData.clear();

  delete [] anyCoord;
  delete [] coordIdx;
  delete [] selected;
  anyCoord = NULL;
  coordIdx = NULL;
  selected = NULL;
}

// ----------------------------------------------------------------------------
void vtkTecplotReader::GetArraysFromBlockPackingZone( int numNodes, int numCells,
     vtkPoints * theNodes, vtkPointData * nodeData, vtkCellData * cellData )
{
  // NOTE: The Tecplot ASCII file format states that a block-packing zone may
  // contain point data or cell data (VARLOCATION, pp. 158, Tecplot 360 Data
  // Format Guide 2009). Thus we need to consider both cases in this function.

  if ( !theNodes || !nodeData || !cellData ||
      !this->Internal->ASCIIStream.is_open()
     )
  {
    vtkErrorMacro( << "File not open, errors with reading, or NULL vtkPoints /"
                   << "vtkPointData / vtkCellData." );
    return;
  }

  int     v;
  int     zArrayId;        // indexing zoneData
  int     arraySiz;
  int     isXcoord;
  int     isYcoord;
  int     isZcoord;
  int   * anyCoord; // is any coordinate?
  int   * selected; // is a selected data attribute?
  float * arrayPtr = NULL;
  vtkFloatArray * theArray = NULL;
  std::vector< vtkFloatArray * > zoneData;
  vtkDataSetAttributes * attribut[2] = { nodeData, cellData };

  zoneData.clear();

  // geoemtry: 3D point coordinates (note that this array must be initialized
  // since only 2D coordinates might be provided by a Tecplot file)
  theNodes->SetNumberOfPoints( numNodes );
  float * cordsPtr = static_cast< float * > (  theNodes->GetVoidPointer( 0 )  );
  memset( cordsPtr, 0, sizeof( float ) * 3 * numNodes );

  // two arrays used to determine the role of each variable (including
  // the coordinate arrays)
  anyCoord = new int[ this->NumberOfVariables ];
  selected = new int[ this->NumberOfVariables ];

  for ( v = 0; v < this->NumberOfVariables; v ++ )
  {
    // check if this variable refers to a coordinate array
    isXcoord = int(  !( v - this->Internal->XIdInList )  );
    isYcoord = int(  !( v - this->Internal->YIdInList )  );
    isZcoord = int(  !( v - this->Internal->ZIdInList )  );
    anyCoord[v] = isXcoord + isYcoord + isZcoord;

    // in case of a data attribute, is it selected by the user?
    selected[v] = this->DataArraySelection
                      ->ArrayIsEnabled( this->Variables[v].c_str() );

    // obtain the size of the block
    arraySiz = ( this->CellBased[v] ? numCells : numNodes );

    if ( anyCoord[v] || selected[v] )
    {
      // parse the block to extract either coordinates or data attribute
      // values

      // extract the variable array throughout a block
      theArray = vtkFloatArray::New();
      theArray->SetNumberOfTuples( arraySiz );
      theArray->SetName( this->Variables[v].c_str() );
      zoneData.push_back( theArray );

      arrayPtr = static_cast< float * > (  theArray->GetVoidPointer( 0 )  );
      for (int i = 0; i < arraySiz; i ++ )
      {
        arrayPtr[i] = atof( this->Internal->GetNextToken().c_str() );
      }
      theArray = NULL;

      // three special arrays are 'combined' to fill the 3D coord array
      if ( anyCoord[v] )
      {
        float * coordPtr = cordsPtr + isYcoord + ( isZcoord << 1 );
        for (int i = 0; i < arraySiz; i ++, coordPtr += 3 )
        {
          *coordPtr = arrayPtr[i];
        }
        coordPtr = NULL;
      }

      arrayPtr = NULL;
    }
    else
    {
      // this block contains an un-selected data attribute and we
      // need to read but ignore the values
      for (int i = 0; i < arraySiz; i ++ )
      {
        this->Internal->GetNextToken();
      }
    }
  }
  cordsPtr = NULL;

  // attach the dataset attributes (node-based and cell-based) to the grid
  // NOTE: zoneData[] and this->Variables (and this->CellBased) may differ
  // in the number of the maintained arrays
  zArrayId = 0;
  for ( v = 0; v < this->NumberOfVariables; v ++ )
  {
    if ( !anyCoord[v] && selected[v] )
    {
      attribut[ this->CellBased[v] ]->AddArray( zoneData[zArrayId] );
    }

    zArrayId += int(    !(  !( anyCoord[v] + selected[v] )  )    );
  }

  //remove all the float arrays from vector so they won't leak
  for ( unsigned int i=0; i < zoneData.size(); ++i)
  {
    vtkFloatArray *fa = zoneData.at(i);
    if ( fa )
    {
      fa->FastDelete();
    }
  }
  zoneData.clear();
  attribut[0] = attribut[1] = NULL;

  delete [] anyCoord;
  delete [] selected;
  anyCoord = NULL;
  selected = NULL;
}

// ----------------------------------------------------------------------------
void vtkTecplotReader::GetStructuredGridFromBlockPackingZone
  ( int iDimSize, int jDimSize, int kDimSize,
    int zoneIndx, const char * zoneName, vtkMultiBlockDataSet * multZone )
{
  if ( !zoneName || !multZone )
  {
    vtkErrorMacro( "Zone name un-specified or NULL vtkMultiBlockDataSet." );
    return;
  }

  // determine the topological dimension
  if ( jDimSize == 1 && kDimSize == 1 )
  {
    this->Internal->TopologyDim = MAX( this->Internal->TopologyDim, 1 );
  }
  else
  if ( kDimSize == 1 )
  {
    this->Internal->TopologyDim = MAX( this->Internal->TopologyDim, 2 );
  }
  else
  {
    this->Internal->TopologyDim = MAX( this->Internal->TopologyDim, 3 );
  }

  // number of points, number of cells, and dimensionality
  int numNodes    = iDimSize * jDimSize * kDimSize;
  int numCells    = (  ( iDimSize <= 1 ) ? 1 : ( iDimSize - 1 )  ) *
                    (  ( jDimSize <= 1 ) ? 1 : ( jDimSize - 1 )  ) *
                    (  ( kDimSize <= 1 ) ? 1 : ( kDimSize - 1 )  );
  int gridDims[3] = { iDimSize, jDimSize, kDimSize };

  // Create vtkPoints and vtkStructuredGrid and associate them
  vtkPoints *         pntCords = vtkPoints::New();
  vtkStructuredGrid * strcGrid = vtkStructuredGrid::New();
  this->GetArraysFromBlockPackingZone( numNodes, numCells,
        pntCords, strcGrid->GetPointData(), strcGrid->GetCellData() );
  strcGrid->SetDimensions( gridDims );
  strcGrid->SetPoints( pntCords );
  pntCords->Delete();
  pntCords = NULL;

  if (    (    this->Internal->TopologyDim == 2
            || this->Internal->TopologyDim == 3
          )
       || (    this->Internal->TopologyDim == 0
            && this->Internal->GeometryDim >  1
          )
     )
  {
    multZone->SetBlock( zoneIndx, strcGrid );
    multZone->GetMetaData( zoneIndx )
            ->Set( vtkCompositeDataSet::NAME(), zoneName );
  }
  strcGrid->Delete();
  strcGrid = NULL;
}

// ----------------------------------------------------------------------------
void vtkTecplotReader::GetStructuredGridFromPointPackingZone
  ( int iDimSize, int jDimSize, int kDimSize,
    int zoneIndx, const char * zoneName, vtkMultiBlockDataSet * multZone )
{
  if ( !zoneName || !multZone )
  {
    vtkErrorMacro( "Zone name un-specified or NULL vtkMultiBlockDataSet." );
    return;
  }

  if ( jDimSize == 1 && kDimSize == 1 )
  {
    this->Internal->TopologyDim = MAX( this->Internal->TopologyDim, 1 );
  }
  else if ( kDimSize == 1 )
  {
    this->Internal->TopologyDim = MAX( this->Internal->TopologyDim, 2 );
  }
  else
  {
    this->Internal->TopologyDim = MAX( this->Internal->TopologyDim, 3 );
  }

  // number of points, number of cells, and dimensionality
  int numNodes    = iDimSize * jDimSize * kDimSize;
  int gridDims[3] = { iDimSize, jDimSize, kDimSize };

  // Create vtkPoints and vtkStructuredGrid and associate them
  vtkPoints *         pntCords = vtkPoints::New();
  vtkStructuredGrid * strcGrid = vtkStructuredGrid::New();
  this->GetArraysFromPointPackingZone
        ( numNodes, pntCords, strcGrid->GetPointData() );
  strcGrid->SetDimensions( gridDims );
  strcGrid->SetPoints( pntCords );
  pntCords->Delete();
  pntCords = NULL;

  if (    (    this->Internal->TopologyDim == 2
            || this->Internal->TopologyDim == 3
          )
       || (    this->Internal->TopologyDim == 0
            && this->Internal->GeometryDim >  1
          )
     )
  {
    multZone->SetBlock( zoneIndx, strcGrid );
    multZone->GetMetaData( zoneIndx )
            ->Set( vtkCompositeDataSet::NAME(), zoneName );
  }
  strcGrid->Delete();
  strcGrid = NULL;
}

// ----------------------------------------------------------------------------
void vtkTecplotReader::GetUnstructuredGridFromBlockPackingZone
  ( int numNodes, int numCells, const char * cellType,
    int zoneIndx, const char * zoneName, vtkMultiBlockDataSet * multZone )
{
  if ( !cellType || !zoneName || !multZone )
  {
    vtkErrorMacro( << "Zone name / cell type un-specified, or NULL "
                   << "vtkMultiBlockDataSet object." );
    return;
  }

  vtkPoints *           gridPnts = vtkPoints::New();
  vtkUnstructuredGrid * unstruct = vtkUnstructuredGrid::New();
  this->GetArraysFromBlockPackingZone( numNodes, numCells,
        gridPnts, unstruct->GetPointData(), unstruct->GetCellData() );
  this->GetUnstructuredGridCells( numCells, cellType, unstruct );
  unstruct->SetPoints( gridPnts );
  gridPnts->Delete();
  gridPnts = NULL;

  if (    (    this->Internal->TopologyDim == 2
            || this->Internal->TopologyDim == 3
          )
       || (    this->Internal->TopologyDim == 0
            && this->Internal->GeometryDim >  1
          )
     )
  {
    multZone->SetBlock( zoneIndx, unstruct );
    multZone->GetMetaData( zoneIndx )
            ->Set( vtkCompositeDataSet::NAME(), zoneName );
  }
  unstruct->Delete();
  unstruct = NULL;
}

// ----------------------------------------------------------------------------
void vtkTecplotReader::GetUnstructuredGridFromPointPackingZone
  ( int numNodes, int numCells, const char * cellType,
    int zoneIndx, const char * zoneName, vtkMultiBlockDataSet * multZone )
{
  if ( !cellType || !zoneName || !multZone )
  {
    vtkErrorMacro( << "Zone name / cell type un-specified, or NULL "
                   << "vtkMultiBlockDataSet object." );
    return;
  }

  vtkPoints *           gridPnts = vtkPoints::New();
  vtkUnstructuredGrid * unstruct = vtkUnstructuredGrid::New();
  this->GetArraysFromPointPackingZone
        ( numNodes, gridPnts, unstruct->GetPointData() );
  this->GetUnstructuredGridCells( numCells, cellType, unstruct );
  unstruct->SetPoints( gridPnts );
  gridPnts->Delete();
  gridPnts = NULL;

  if (    (    this->Internal->TopologyDim == 2
            || this->Internal->TopologyDim == 3
          )
       || (    this->Internal->TopologyDim == 0
            && this->Internal->GeometryDim >  1
          )
     )
  {
    multZone->SetBlock( zoneIndx, unstruct );
    multZone->GetMetaData( zoneIndx )
            ->Set( vtkCompositeDataSet::NAME(), zoneName );
  }
  unstruct->Delete();
  unstruct = NULL;
}

// ----------------------------------------------------------------------------
void vtkTecplotReader::GetUnstructuredGridCells( int numberCells,
  const char * cellTypeStr, vtkUnstructuredGrid * unstrctGrid )
{
  if ( !cellTypeStr || !unstrctGrid )
  {
    vtkErrorMacro( << "Cell type (connectivity type) unspecified or NULL "
                   << "vtkUnstructuredGrid object." );
    return;
  }

  // determine the number of points per cell and the cell type
  int numCellPnts = -1;
  int theCellType = -1;

  if (  strcmp( cellTypeStr, "BRICK" ) == 0  )
  {
    numCellPnts = 8;
    theCellType = VTK_HEXAHEDRON;
    this->Internal->TopologyDim = MAX( this->Internal->TopologyDim, 3 );
  }
  else if (  strcmp( cellTypeStr, "TRIANGLE" ) == 0  )
  {
    numCellPnts = 3;
    theCellType = VTK_TRIANGLE;
    this->Internal->TopologyDim = MAX( this->Internal->TopologyDim, 2 );
  }
  else if (  strcmp( cellTypeStr, "QUADRILATERAL" ) == 0  )
  {
    numCellPnts = 4;
    theCellType = VTK_QUAD;
    this->Internal->TopologyDim = MAX( this->Internal->TopologyDim, 2 );
  }
  else if ( strcmp( cellTypeStr, "TETRAHEDRON" ) == 0  )
  {
    numCellPnts = 4;
    theCellType = VTK_TETRA;
    this->Internal->TopologyDim = MAX( this->Internal->TopologyDim, 3 );
  }
  else if (  strcmp( cellTypeStr, "POINT" ) == 0 || strcmp( cellTypeStr, "" ) == 0  )
  {
    numCellPnts = 1;
    theCellType = VTK_VERTEX;
    this->Internal->TopologyDim = MAX( this->Internal->TopologyDim, 0 );
  }
  else
  {
    vtkErrorMacro( << this->FileName << ": Unknown cell type for a zone." );
    return;
  }

  // the storage of each cell begins with the number of points per cell,
  // followed by a list of point ids representing the cell
  vtkIdTypeArray * cellInfoList = vtkIdTypeArray::New();
  cellInfoList->SetNumberOfValues(  ( numCellPnts + 1 ) * numberCells  );
  vtkIdType *      cellInforPtr = cellInfoList->GetPointer( 0 );

  // type of each cell
  vtkUnsignedCharArray * cellTypeList = vtkUnsignedCharArray::New();
  cellTypeList->SetNumberOfValues( numberCells );
  unsigned char *        cellTypesPtr = cellTypeList->GetPointer( 0 );

  // location of each cell in support of fast random (non-sequential) access
  vtkIdTypeArray * cellLocArray = vtkIdTypeArray::New();
  cellLocArray->SetNumberOfValues( numberCells );
  vtkIdType *      cellLocatPtr = cellLocArray->GetPointer( 0 );

  // fill the three arrays
  int locateOffset = 0;
  for ( int c = 0; c < numberCells; c ++ )
  {
    *cellTypesPtr ++ = theCellType;
    *cellInforPtr ++ = numCellPnts;

    // 1-origin connectivity array
    for ( int j = 0; j < numCellPnts; j ++ )
    {
      *cellInforPtr ++ = (   theCellType == VTK_VERTEX
                           ? c
                           : atoi( this->Internal->GetNextToken().c_str() ) - 1
                         );
    }

    *cellLocatPtr ++ = locateOffset;
    locateOffset    += numCellPnts + 1;
  }
  cellInforPtr = NULL;
  cellTypesPtr = NULL;
  cellLocatPtr = NULL;

  // create a cell array object to accept the cell info
  vtkCellArray * theCellArray = vtkCellArray::New();
  theCellArray->SetCells( numberCells, cellInfoList );
  cellInfoList->Delete();
  cellInfoList = NULL;

  // create a vtkUnstructuredGrid object and attach the 3 arrays (types, locations,
  // and cells) to it for export.
  unstrctGrid->SetCells( cellTypeList, cellLocArray, theCellArray );
  theCellArray->Delete();
  cellTypeList->Delete();
  cellLocArray->Delete();
  theCellArray = NULL;
  cellTypeList = NULL;
  cellLocArray = NULL;
}

// ----------------------------------------------------------------------------
void vtkTecplotReader::GetDataArraysList()
{
  if (    ( this->Internal->Completed == 1 )
       || ( this->DataArraySelection->GetNumberOfArrays() > 0 )
       || ( this->FileName == NULL ) || (  strcmp( this->FileName, "" ) == 0  )
     )
  {
    return;
  }

  #define READ_UNTIL_TITLE_OR_VARIABLES !this->Internal->NextCharEOF &&\
                                        theTpToken != "TITLE"        &&\
                                        theTpToken != "VARIABLES"
  int             i;
  int             tpTokenLen = 0;
  int             guessedXid = -1;
  int             guessedYid = -1;
  int             guessedZid = -1;
  bool            tokenReady = false;
  std::string  noSpaceTok = "";

  this->Variables.clear();
  this->NumberOfVariables = 0;

  this->Internal->Init();
  this->Internal->ASCIIStream.open( this->FileName );
  std::string theTpToken = this->Internal->GetNextToken();

  while ( !this->Internal->NextCharEOF )
  {
    tokenReady = false;

    if ( theTpToken == "" )
    {
      // whitespace: do nothing
    }
    else if ( theTpToken == "TITLE" )
    {
      this->Internal->GetNextToken();
    }
    else if ( theTpToken == "VARIABLES" )
    {
      theTpToken = this->Internal->GetNextToken();

      while ( this->Internal->TokenIsString )
      {
        tpTokenLen = int( theTpToken.length() );
        for ( i = 0; i < tpTokenLen; i ++ )
        {
          if ( theTpToken[i] == '(' )
          {
            theTpToken[i] = '[';
          }
          else if ( theTpToken[i] == ')' )
          {
            theTpToken[i] = ']';
          }
          else if ( theTpToken[i] == '/' )
          {
            theTpToken[i] = '_';
          }
        }

        noSpaceTok = SimplifyWhitespace( theTpToken );

        switch (  GetCoord( noSpaceTok )  )
        {
          case 0:  this->Internal->XIdInList = this->NumberOfVariables; break;
          case 1:  this->Internal->YIdInList = this->NumberOfVariables; break;
          case 2:  this->Internal->ZIdInList = this->NumberOfVariables; break;
          default: break;
        }

        switch (  GuessCoord( noSpaceTok )  )
        {
          case 0:  guessedXid = this->NumberOfVariables; break;
          case 1:  guessedYid = this->NumberOfVariables; break;
          case 2:  guessedZid = this->NumberOfVariables; break;
          default: break;
        }

        this->Variables.push_back( theTpToken );
        this->NumberOfVariables ++;
        theTpToken = this->Internal->GetNextToken();
      }

      if ( this->NumberOfVariables == 0 )
      {
        while ( true )
        {
          noSpaceTok = SimplifyWhitespace( theTpToken );

          switch (  GetCoord( noSpaceTok )  )
          {
            case 0:  this->Internal->XIdInList = this->NumberOfVariables; break;
            case 1:  this->Internal->YIdInList = this->NumberOfVariables; break;
            case 2:  this->Internal->ZIdInList = this->NumberOfVariables; break;
            default: break;
          }

          switch (  GuessCoord( noSpaceTok )  )
          {
            case 0:  guessedXid = this->NumberOfVariables; break;
            case 1:  guessedYid = this->NumberOfVariables; break;
            case 2:  guessedZid = this->NumberOfVariables; break;
            default: break;
          }

          this->Variables.push_back( theTpToken );
          this->NumberOfVariables ++;

          if ( this->Internal->NextCharEOL )
          {
            break;
          }
          theTpToken = this->Internal->GetNextToken();
        }
      }

      // in case there is not an exact match for coordinate axis vars
      this->Internal->XIdInList = ( this->Internal->XIdInList < 0 )
                                  ? guessedXid
                                  : this->Internal->XIdInList;
      this->Internal->YIdInList = ( this->Internal->YIdInList < 0 )
                                  ? guessedYid
                                  : this->Internal->YIdInList;
      this->Internal->ZIdInList = ( this->Internal->ZIdInList < 0 )
                                  ? guessedZid
                                  : this->Internal->ZIdInList;

      break;
    }
    else
    {
      do
      {
        theTpToken = this->Internal->GetNextToken();
      } while ( READ_UNTIL_TITLE_OR_VARIABLES );

      tokenReady = true;
    }

    if ( !tokenReady )
    {
      theTpToken = this->Internal->GetNextToken();
    }

  }

  this->Internal->ASCIIStream.rewind();

  // register the data arrays
  for ( i = 0; i < this->GetNumberOfDataAttributes(); i ++ )
  {
    // all data arrays are selected here by default
    this->DataArraySelection->EnableArray(  this->GetDataAttributeName( i )  );
  }
}

// ----------------------------------------------------------------------------
void vtkTecplotReader::ReadFile( vtkMultiBlockDataSet * multZone )
{
  if (    ( this->Internal->Completed == 1 )
       || ( this->FileName == NULL ) || (  strcmp( this->FileName, "" ) == 0  )
     )
  {
    return;
  }

  if ( multZone == NULL )
  {
    vtkErrorMacro( "vtkMultiBlockDataSet multZone NULL!" );
    return;
  }

  #define READ_UNTIL_LINE_END !this->Internal->NextCharEOF &&\
                              tok != "TITLE"               &&\
                              tok != "VARIABLES"           &&\
                              tok != "ZONE"                &&\
                              tok != "GEOMETRY"            &&\
                              tok != "TEXT"                &&\
                              tok != "DATASETAUXDATA"
  int  zoneIndex  = 0;
  bool firstToken = true;
  bool tokenReady = false;

  this->Init();
  this->Internal->ASCIIStream.open( this->FileName );
  std::string tok = this->Internal->GetNextToken();

  while ( !this->Internal->NextCharEOF )
  {
    tokenReady = false;
    if ( tok == "" )
    {
      // whitespace: do nothing
    }
    else if ( tok == "TITLE" )
    {
      this->DataTitle = this->Internal->GetNextToken();
    }
    else if ( tok == "GEOMETRY" )
    {
      // unsupported
      tok = this->Internal->GetNextToken();
      while ( READ_UNTIL_LINE_END )
      {
        // skipping token
        tok = this->Internal->GetNextToken();
      }
      tokenReady = true;
    }
    else if ( tok == "TEXT" )
    {
      // unsupported
      tok = this->Internal->GetNextToken();
      while ( READ_UNTIL_LINE_END )
      {
        // Skipping token
        tok = this->Internal->GetNextToken();
      }
      tokenReady = true;
    }
    else if ( tok == "VARIABLES" )
    {
      int guessedXindex = -1;
      int guessedYindex = -1;
      int guessedZindex = -1;

      // variable lists
      tok = this->Internal->GetNextToken();
      while ( this->Internal->TokenIsString )
      {
        int tokLen = int( tok.length() );
        for ( int i = 0; i < tokLen; i ++ )
        {
          if ( tok[i] == '(' )
          {
            tok[i] = '[';
          }
          else
          if ( tok[i] == ')' )
          {
            tok[i] = ']';
          }
          else
          if ( tok[i] == '/' )
          {
            tok[i] = '_';
          }
        }

        std::string   tok_nw = SimplifyWhitespace( tok );

        switch (  GetCoord( tok_nw )  )
        {
          case 0:  this->Internal->XIdInList = this->NumberOfVariables; break;
          case 1:  this->Internal->YIdInList = this->NumberOfVariables; break;
          case 2:  this->Internal->ZIdInList = this->NumberOfVariables; break;
          default: break;
        }

        switch (  GuessCoord( tok_nw )  )
        {
          case 0:  guessedXindex = this->NumberOfVariables; break;
          case 1:  guessedYindex = this->NumberOfVariables; break;
          case 2:  guessedZindex = this->NumberOfVariables; break;
          default: break;
        }

        this->Variables.push_back( tok );
        this->NumberOfVariables ++;
        tok = this->Internal->GetNextToken();
      }

      if ( this->NumberOfVariables == 0 )
      {
        while ( true )
        {
          std::string      tok_nw = SimplifyWhitespace( tok );

          switch (  GetCoord( tok_nw )  )
          {
            case 0:  this->Internal->XIdInList = this->NumberOfVariables; break;
            case 1:  this->Internal->YIdInList = this->NumberOfVariables; break;
            case 2:  this->Internal->ZIdInList = this->NumberOfVariables; break;
            default: break;
          }

          switch (  GuessCoord( tok_nw )  )
          {
            case 0:  guessedXindex = this->NumberOfVariables; break;
            case 1:  guessedYindex = this->NumberOfVariables; break;
            case 2:  guessedZindex = this->NumberOfVariables; break;
            default: break;
          }

          this->Variables.push_back( tok );
          this->NumberOfVariables ++;

          if ( this->Internal->NextCharEOL )
          {
            tok = this->Internal->GetNextToken();
            break;
          }
          else
          {
            tok = this->Internal->GetNextToken();
          }
        }
      }

      // Default the centering to nodal
      this->CellBased.clear();
      this->CellBased.resize( this->NumberOfVariables, 0 );

      // If we didn't find an exact match for coordinate axis vars, guess
      if ( this->Internal->XIdInList < 0 )
      {
        this->Internal->XIdInList = guessedXindex;
      }
      if ( this->Internal->YIdInList < 0 )
      {
        this->Internal->YIdInList = guessedYindex;
      }
      if ( this->Internal->ZIdInList < 0 )
      {
        this->Internal->ZIdInList = guessedZindex;
      }

      // Based on how many spatial coords we got, guess the spatial dim
      if ( this->Internal->XIdInList >= 0 )
      {
        this->Internal->GeometryDim = 1;
        if ( this->Internal->YIdInList >= 0 )
        {
          this->Internal->GeometryDim = 2;
          if ( this->Internal->ZIdInList >= 0 )
          {
            this->Internal->GeometryDim = 3;
          }
        }
      }

      tokenReady = true;
    }
    else if ( tok == "ZONE" )
    {
      int      numI = 1;
      int      numJ = 1;
      int      numK = 1;
      int      numNodes    = 0;
      int      numElements = 0;
      char     untitledZoneName[40];
      snprintf( untitledZoneName, sizeof(untitledZoneName), "zone%05d", zoneIndex );

      std::string format    = "";
      std::string elemType  = "";
      std::string ZoneName = untitledZoneName;

      tok = this->Internal->GetNextToken();
      while (  !( tok != "T"  &&
                  tok != "I"  &&
                  tok != "J"  &&
                  tok != "K"  &&
                  tok != "N"  &&
                  tok != "E"  &&
                  tok != "ET" &&
                  tok != "F"  &&
                  tok != "D"  &&
                  tok != "DT" &&
                  tok != "STRANDID"     &&
                  tok != "SOLUTIONTIME" &&
                  tok != "DATAPACKING"  &&
                  tok != "VARLOCATION"
                )
            )
      {
        if ( tok == "T" )
        {
          ZoneName = this->Internal->GetNextToken();
          if ( !this->Internal->TokenIsString )
          {
            vtkErrorMacro( << this->FileName << ": Zone titles MUST be "
                           << "quoted." );
            return;
          }
        }
        else if ( tok == "I" )
        {
          numI = atoi( this->Internal->GetNextToken().c_str() );
        }
        else if ( tok == "J" )
        {
          numJ = atoi( this->Internal->GetNextToken().c_str() );
        }
        else if ( tok == "K" )
        {
          numK = atoi( this->Internal->GetNextToken().c_str() );
        }
        else if ( tok == "N" )
        {
          numNodes = atoi( this->Internal->GetNextToken().c_str() );
        }
        else if ( tok == "E" )
        {
          numElements = atoi( this->Internal->GetNextToken().c_str() );
        }
        else if ( tok == "ET" )
        {
          elemType = this->Internal->GetNextToken();
        }
        else if ( tok == "F" || tok == "DATAPACKING" )
        {
          format = this->Internal->GetNextToken();
        }
        else if ( tok == "VARLOCATION" )
        {
          std::string  centering;
          this->CellBased.clear();
          this->CellBased.resize( this->NumberOfVariables, 0 );

          //read token to ascertain VARLOCATION syntax usage
          std::string var_format_type = this->Internal->GetNextToken();

          //if each variable will have data type specified explicitly (as is handled in old Tecplot reader),
          //else a range is specified for CELLCENTERED only, with NODAL values assumed implicitly
          if ( var_format_type == "NODAL" || var_format_type == "CELLCENTERED" )
          {
            if ( var_format_type == "CELLCENTERED" )
            {
              this->CellBased[0] = 1;
            }
            for ( int i = 1; i < this->NumberOfVariables; i ++ )
            {
              centering = this->Internal->GetNextToken();
              if ( centering == "CELLCENTERED" )
              {
                this->CellBased[i] = 1;
              }
            }
          }
          else
          {
            do
            {
              //remove left square bracket, if it exists
              size_t brack_pos = var_format_type.find("[");
              if ( brack_pos != std::string::npos )
                var_format_type.erase(brack_pos, brack_pos+1);

              //remove right square bracket, if it exists
              brack_pos = var_format_type.find("]");
              if ( brack_pos != std::string::npos )
                var_format_type.erase(brack_pos,brack_pos+1);

              //if a range is defined, then split again, convert to int and set to cell data
              //else if a single value is defined, then just set the flag directly
              if ( var_format_type.find("-") != std::string::npos )
              {
                std::vector<std::string> var_range;
                vtksys::SystemTools::Split(var_format_type.c_str(), var_range, '-');

                int cell_start = atoi(var_range[0].c_str()) - 1;
                int cell_end = atoi(var_range[1].c_str());
                for ( int i = cell_start; i != cell_end; ++i )
                {
                  this->CellBased[i] = 1;
                }
              }
              else
              {
                int index = atoi(var_format_type.c_str())-1;
                this->CellBased[index] = 1;
              }

              //get next value
              var_format_type = this->Internal->GetNextToken();

              //continue until the CELLCENTERED keyword is found
            } while ( var_format_type != "CELLCENTERED" );
          }
        }
        else if ( tok == "DT" )
        {
          for ( int i = 0; i < this->NumberOfVariables; i ++ )
          {
            this->Internal->GetNextToken();
          }
        }
        else if ( tok == "D" )
        {
          vtkWarningMacro( << this->FileName << "; Tecplot zone record parameter "
                         << "'D' is currently unsupported." );
          this->Internal->GetNextToken();
        }
        else if ( tok == "STRANDID" )
        {
          vtkWarningMacro( << this->FileName << "; Tecplot zone record parameter "
                         << "'STRANDID' is currently unsupported." );
          this->Internal->GetNextToken();
        }
        else if ( tok == "SOLUTIONTIME" )
        {
          vtkWarningMacro( << this->FileName << "; Tecplot zone record parameter "
                         << "'SOLUTIONTIME' is currently unsupported." );
          this->Internal->GetNextToken();
        }
        tok = this->Internal->GetNextToken();
      }

      this->Internal->TokenBackup = tok;

      this->ZoneNames.push_back( ZoneName );

      if ( format == "FEBLOCK" )
      {
        this->GetUnstructuredGridFromBlockPackingZone( numNodes, numElements,
              elemType.c_str(), zoneIndex, ZoneName.c_str(),  multZone );
      }
      else if ( format == "FEPOINT" )
      {
        this->GetUnstructuredGridFromPointPackingZone( numNodes, numElements,
              elemType.c_str(), zoneIndex, ZoneName.c_str(),  multZone );
      }
      else if ( format == "BLOCK" )
      {
        this->GetStructuredGridFromBlockPackingZone
              ( numI, numJ, numK, zoneIndex, ZoneName.c_str(),  multZone );
      }
      else if ( format == "POINT" )
      {
        this->GetStructuredGridFromPointPackingZone
              ( numI, numJ, numK, zoneIndex, ZoneName.c_str(),  multZone );
      }
      else if ( format == "" )
      {
        // No format given; we will assume we got a POINT format
        this->GetStructuredGridFromPointPackingZone
              ( numI, numJ, numK, zoneIndex, ZoneName.c_str(),  multZone );
      }
      else
      {
        // UNKNOWN FORMAT
        vtkErrorMacro( << this->FileName << ": The format " << format.c_str()
                       << " found in the file is unknown." );
        return;
      }

      zoneIndex ++;
    }
    else if( tok == "DATASETAUXDATA" )
    {
      int   tokIndex       = 0;
      bool  haveVectorExpr = false;
      tok = this->Internal->GetNextToken();

      while ( READ_UNTIL_LINE_END )
      {
        if( tokIndex == 0 )
        {
          haveVectorExpr = ( tok == "VECTOR" );
        }
        else if( tokIndex == 1 )
        {
          if( haveVectorExpr )
          {
            // Remove spaces
            std::string::size_type pos = tok.find( " " );
            while( pos != std::string::npos )
            {
              tok.replace( pos, 1, "" );
              pos = tok.find( " " );
            }

            // Look for '('
            pos = tok.find( "(" );
            if( pos != std::string::npos )
            {
              std::string  exprName(  tok.substr( 0, pos )  );
              std::string  exprDef (  tok.substr( pos, tok.size() - pos )  );

              exprDef.replace( 0, 1, "{" );

              // Replace ')' with '}'
              pos = exprDef.find( ")" );
              if( pos != std::string::npos )
              {
                exprDef.replace( pos, 1, "}" );
                vtkDebugMacro( "Expr name = " << exprName.c_str()
                               << ", Expr def = " << exprDef.c_str() );
              }
            }
          }
        }

        // Skipping token
        tok = this->Internal->GetNextToken();
        tokIndex ++;
      }

      tokenReady = true;
    }
    else if ( firstToken && this->Internal->TokenIsString )
    {
      // Robust: assume it's a title
      this->DataTitle = tok;
    }
    else
    {
      // UNKNOWN RECORD TYPE
      vtkErrorMacro( << this->FileName << ": The record type " << tok.c_str()
                     << " found in the file is unknown." );
      return;
    }

    firstToken = false;
    if ( !tokenReady )
    {
      tok = this->Internal->GetNextToken();
    }
  }
  this->Internal->ASCIIStream.close();

  if ( this->Internal->TopologyDim > this->Internal->GeometryDim )
  {
    this->Internal->TopologyDim = this->Internal->GeometryDim;
  }

  this->Internal->Completed = 1;
}
