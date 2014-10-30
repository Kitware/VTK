/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLSDynaReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

// NOTE TO DEVELOPERS: ========================================================
//
// This is a really big reader.
// It uses several classes defined in Utilities/LSDyna:
// - LSDynaFamily:
//    A class to abstract away I/O from families of output files.
//    This performs the actual reads and writes plus any required byte swapping.
//    Also contains a subclass, LSDynaFamilyAdaptLevel, used to store
//    file+offset
//    information for each mesh adaptation's state info.
// - LSDynaMetaData:
//    A class to hold metadata about a particular file (such as time steps,
//    the start of state information for each time step, the number of
//    adaptive remeshes, and the large collection of constants that determine
//    the available attributes). It contains an LSDynaFamily instance.

//It also uses a helper vtk class
// - vtkLSDynaSummaryParser:
//    A class to parse XML summary files containing part names and their IDs.
//    This class is used by vtkLSDynaReader::ReadInputDeckXML().

// This class is preceded by some file-static constants and utility routines.

#include "vtkLSDynaReader.h"
#include "vtkLSDynaSummaryParser.h"
#include "vtkLSDynaPartCollection.h"
#include "LSDynaFamily.h"
#include "LSDynaMetaData.h"

#include "vtksys/SystemTools.hxx"

#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <cassert>

#include "vtkCellType.h"
#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkFloatArray.h"
#include "vtkPoints.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"


vtkStandardNewMacro(vtkLSDynaReader);

// Names of vtkDataArrays provided with grid:
#define LS_ARRAYNAME_DEATH              "Death"
#define LS_ARRAYNAME_USERID             "UserID"
#define LS_ARRAYNAME_SPECIES_BLNK       "SpeciesXX"
#define LS_ARRAYNAME_SPECIES_FMT        "Species%02d"
#define LS_ARRAYNAME_SPECIES_01         "Species01"
#define LS_ARRAYNAME_SPECIES_02         "Species02"
#define LS_ARRAYNAME_SPECIES_03         "Species03"
#define LS_ARRAYNAME_SPECIES_04         "Species04"
#define LS_ARRAYNAME_SPECIES_05         "Species05"
#define LS_ARRAYNAME_SPECIES_06         "Species06"
#define LS_ARRAYNAME_SPECIES_07         "Species07"
#define LS_ARRAYNAME_SPECIES_08         "Species08"
#define LS_ARRAYNAME_SPECIES_09         "Species09"
#define LS_ARRAYNAME_SPECIES_10         "Species10"
#define LS_ARRAYNAME_TEMPERATURE        "Temperature"
#define LS_ARRAYNAME_DEFLECTION         "Deflection"
#define LS_ARRAYNAME_VELOCITY           "Velocity"
#define LS_ARRAYNAME_ACCELERATION       "Acceleration"
#define LS_ARRAYNAME_PRESSURE           "Pressure"
#define LS_ARRAYNAME_VORTICITY          "Vorticity"
#define LS_ARRAYNAME_RESULTANTVORTICITY "ResVorticity"
#define LS_ARRAYNAME_ENSTROPHY          "Enstrophy"
#define LS_ARRAYNAME_HELICITY           "Helicity"
#define LS_ARRAYNAME_STREAMFUNCTION     "StreamFunc"
#define LS_ARRAYNAME_ENTHALPY           "Enthalpy"
#define LS_ARRAYNAME_DENSITY            "Density"
#define LS_ARRAYNAME_TURBULENTKE        "TurbulentKE"
#define LS_ARRAYNAME_DISSIPATION        "Dissipation"
#define LS_ARRAYNAME_EDDYVISCOSITY      "EddyVisc"
#define LS_ARRAYNAME_RADIUSOFINFLUENCE  "InfluenceRadius"
#define LS_ARRAYNAME_NUMNEIGHBORS       "NumberOfNeighbors"
#define LS_ARRAYNAME_SEGMENTID          "SegmentID"
#define LS_ARRAYNAME_STRAIN             "Strain"
#define LS_ARRAYNAME_STRESS             "Stress"
#define LS_ARRAYNAME_EPSTRAIN           "EffPlastStrn"
#define LS_ARRAYNAME_INTEGRATIONPOINT   "IntPtData"
#define LS_ARRAYNAME_RESULTANTS         "Resultants"
#define LS_ARRAYNAME_ELEMENTMISC        "ElementMisc"
#define LS_ARRAYNAME_INTERNALENERGY     "InternalEnergy"
#define LS_ARRAYNAME_AXIALFORCE         "AxialForce"
#define LS_ARRAYNAME_SHEARRESULTANT     "ShearResultant"
#define LS_ARRAYNAME_BENDINGRESULTANT   "BendingResultant"
#define LS_ARRAYNAME_TORSIONRESULTANT   "TorsionResultant"
#define LS_ARRAYNAME_NORMALRESULTANT    "NormalResultant"
#define LS_ARRAYNAME_AXIALSTRAIN        "AxialStrain"
#define LS_ARRAYNAME_AXIALSTRESS        "AxialStress"
#define LS_ARRAYNAME_SHEARSTRAIN        "ShearStrain"
#define LS_ARRAYNAME_SHEARSTRESS        "ShearStress"
#define LS_ARRAYNAME_PLASTICSTRAIN      "PlasticStrain"
#define LS_ARRAYNAME_THICKNESS          "Thickness"
#define LS_ARRAYNAME_MASS               "Mass"

// Possible material  options
#define LS_MDLOPT_NONE 0
#define LS_MDLOPT_POINT 1
#define LS_MDLOPT_CELL 2

#ifdef VTK_LSDYNA_DBG_MULTIBLOCK
static void vtkDebugMultiBlockStructure( vtkIndent indent, vtkMultiGroupDataSet* mbds );
#endif // VTK_LSDYNA_DBG_MULTIBLOCK

namespace
{
static const char* vtkLSDynaCellTypes[] =
{
  "Point",
  "Beam",
  "Shell",
  "Thick Shell",
  "Solid",
  "Rigid Body",
  "Road Surface"
};

static void vtkLSGetLine( ifstream& deck, std::string& line )
{
#if !defined(_WIN32) && !defined(WIN32) && !defined(_MSC_VER) && !defined(__BORLANDC__)
  // One line implementation for everyone but Windows (MSVC6 and BCC32 are the troublemakers):
  std::getline( deck, line, '\n' );
#else
  // Feed Windows its food cut up into little pieces
  int linechar;
  line = "";
  while ( deck.good() )
    {
    linechar = deck.get();
    if ( linechar == '\r' || linechar == '\n' )
      return;
    line += linechar;
    }
#endif
}

// Read in lines until one that's
// - not empty, and
// - not a comment
// is encountered. Return with that text stored in \a line.
// If an error or EOF is hit, return 0. Otherwise, return 1.
static int vtkLSNextSignificantLine( ifstream& deck, std::string& line )
{
  while ( deck.good() )
    {
    vtkLSGetLine( deck, line );
    if ( ! line.empty() && line[0] != '$' )
      {
      return 1;
      }
    }
  return 0;
}

static void vtkLSTrimWhitespace( std::string& line )
{
  std::string::size_type llen = line.length();
  while ( llen &&
    ( line[llen - 1] == ' ' ||
      line[llen - 1] == '\t' ||
      line[llen - 1] == '\r' ||
      line[llen - 1] == '\n' ) )
    {
    --llen;
    }

  std::string::size_type nameStart = 0;
  while ( nameStart < llen &&
    ( line[nameStart] == ' ' ||
      line[nameStart] == '\t' ) )
    {
    ++nameStart;
    }

  line = line.substr( nameStart, llen - nameStart );
}

static void vtkLSDowncaseFirstWord( std::string& downcased, const std::string& line )
{
  std::string::size_type i;
  std::string::value_type chr;
  int leadingSpace = 0;
  downcased = "";
  for ( i = 0; i < line.length(); ++i )
    {
    chr = tolower( line[i] );
    if ( chr == ' ' || chr == '\t' )
      {
      if ( leadingSpace )
        { // We've trimmed leading whitespace already, so we're done with the word.
        return;
        }
      }
    else
      {
      leadingSpace = 1;
      if ( chr == ',' )
        { // We're at a separator (other than whitespace). No need to continue.
        return;
        }
      }
    downcased += chr;
    }
}

void vtkLSSplitString( std::string& input, std::vector<std::string>& splits, const char* separators )
{
  std::string::size_type posBeg = 0;
  std::string::size_type posEnd;
  do {
    posEnd = input.find_first_of( separators, posBeg );
    if ( posEnd > posBeg )
      {
      // don't include empty entries in splits.
      // NOTE: This means ",comp,1, ,3" with separators ", " yields "comp","1","3", not "","comp","1","","","3".
      splits.push_back( input.substr( posBeg, posEnd - posBeg ) );
      }
    posBeg = input.find_first_not_of( separators, posEnd );
  } while ( posBeg != std::string::npos );
}

template<int hostBitSize, int fileBitSize, int cellLength> struct Converter
  {
  //general use case that the host
  //bit size and file bit size are the same
  vtkIdType* convert(vtkIdType* buff, const vtkIdType&)
    {
    return buff;
    }
  };

template<int cellLength> struct Converter<8,4,cellLength>
  {
  //specilization of 64bit machine and 32bit file
  //so we have to copy each item individually
  vtkIdType* convert(int* buff, const vtkIdType& size)
    {
    for(vtkIdType i=0;i<size;++i)
      {
      this->Conn[i]=static_cast<vtkIdType>(buff[i]);
      }
    return Conn;
    }
  vtkIdType Conn[cellLength];
  };

template<int cellLength> struct Converter<4,8,cellLength>
  {
  //specilization for reading 64 bit files on a 32 bit machine
  //which means reading the bottom half of the long long
  vtkIdType* convert(int* buff, const vtkIdType& size)
    {
    vtkIdType idx=0;
    for(vtkIdType i=0;i<size;i+=2,++idx)
      {
      this->Conn[idx]=static_cast<vtkIdType>(buff[i]);
      }
    return Conn;
    }
  vtkIdType Conn[cellLength];
  };

template<int type,int wordSize,int cellLength>
  struct FillBlock
  {
  Converter<sizeof(vtkIdType),wordSize,cellLength> BC;

  template<typename T>
  FillBlock(T* buff, vtkLSDynaPartCollection *parts,LSDynaMetaData *p,
    const vtkIdType& numWordsPerCell, const int& cellType)
    {
    //determine the relationship between the file bit size and
    //the host machine bit size. This allows us to read 64 bit files on a
    //32 bit machine. The Converter allows us to easily convert 32bit
    //arrays to 64bit arrays
    const int numWordsPerIdType (p->Fam.GetWordSize() / sizeof(T));
    const vtkIdType numFileWordsPerCell(numWordsPerCell * numWordsPerIdType);
    const vtkIdType offsetToMatId(numWordsPerIdType *(numWordsPerCell-1));
    vtkIdType *conn;

    vtkIdType nc=0,j=0,matlId=0;
    vtkIdType numCellsToSkip=0, numCellsToSkipEnd=0, chunkSize=0;


    //get from the part the read information for this lsdyna block type
    parts->GetPartReadInfo(type,nc,numCellsToSkip,numCellsToSkipEnd);

    p->Fam.SkipWords(numFileWordsPerCell * numCellsToSkip ); //skip to the right start id

    //buffer the amount in small chunks so we don't create a massive buffer
    vtkIdType numChunks = p->Fam.InitPartialChunkBuffering(nc,numWordsPerCell);
    for(vtkIdType i=0; i < numChunks; ++i)
      {
      chunkSize = p->Fam.GetNextChunk( LSDynaFamily::Int);
      buff = p->Fam.GetBufferAs<T>();
      for (j=0; j<chunkSize;j+=numWordsPerCell)
        {
        conn = BC.convert(buff,offsetToMatId);
        buff+=offsetToMatId;
        matlId = static_cast<vtkIdType>(*buff);
        buff+=numWordsPerIdType;
        parts->InsertCell(type,matlId,cellType,cellLength,conn);
        }
      }
    p->Fam.SkipWords(numFileWordsPerCell * numCellsToSkipEnd);
    }
  };

template<int wordSize,int cellLength>
  struct FillBlock<LSDynaMetaData::SOLID,wordSize,cellLength>
  {
  Converter<sizeof(vtkIdType),wordSize,cellLength> BC;

  template<typename T>
  FillBlock(T* buff, vtkLSDynaPartCollection *parts,LSDynaMetaData *p,
    const vtkIdType& numWordsPerCell, const int& vtkNotUsed(cellType) )
    {
    //determine the relationship between the file bit size and
    //the host machine bit size. This allows us to read 64 bit files on a
    //32 bit machine. The Converter allows us to easily convert 32bit
    //arrays to 64bit arrays
    const int numWordsPerIdType (p->Fam.GetWordSize() / sizeof(T));
    const vtkIdType numFileWordsPerCell(numWordsPerCell * numWordsPerIdType);
    const vtkIdType offsetToMatId(numWordsPerIdType * cellLength);
    vtkIdType *conn;

    //This is a read solids template specialization since it has a special use
    //case for cell length based on the connectivity mapping
    vtkIdType nc=0,j=0,matlId=0;
    vtkIdType numCellsToSkip=0, numCellsToSkipEnd=0, chunkSize=0;

    //get from the part the read information for this lsdyna block type
    parts->GetPartReadInfo(LSDynaMetaData::SOLID,nc,numCellsToSkip,numCellsToSkipEnd);

    p->Fam.SkipWords(numFileWordsPerCell * numCellsToSkip); //skip to the right start id

    //buffer the amount in small chunks so we don't create a massive buffer
    vtkIdType numChunks = p->Fam.InitPartialChunkBuffering(nc,numWordsPerCell);
    vtkIdType npts = 0;
    int ctype = 0;
    for(vtkIdType i=0; i < numChunks; ++i)
      {
      chunkSize = p->Fam.GetNextChunk( LSDynaFamily::Int);
      buff = p->Fam.GetBufferAs<T>();
      for (j=0; j<chunkSize;j+=numWordsPerCell)
        {
        conn = BC.convert(buff,offsetToMatId);
        buff+=offsetToMatId;
        matlId = static_cast<vtkIdType>(*buff);
        buff+=numWordsPerIdType;

        //Detect repeated connectivity entries to determine element type
        if (conn[3] == conn[7])
          {
          ctype = VTK_TETRA;
          npts = 4;
          }
        else if (conn[4] == conn[7])
          {
          ctype = VTK_PYRAMID;
          npts = 5;
          }
        else if (conn[5] == conn[7])
          {
          ctype = VTK_WEDGE;
          npts = 6;
          }
        else
          {
          ctype = VTK_HEXAHEDRON;
          npts = 8;
          }

        //push this cell back into the unstructured grid for this part(if the part is active)
        parts->InsertCell(LSDynaMetaData::SOLID,matlId,ctype,npts,conn);
        }
      }
    p->Fam.SkipWords(numFileWordsPerCell * numCellsToSkipEnd);
    }
  };

template<int wordSize,int cellLength>
  struct FillBlock<LSDynaMetaData::SHELL,wordSize,cellLength>
  {
  Converter<sizeof(vtkIdType),wordSize,cellLength> BC;

  template<typename T>
  FillBlock(T* buff, vtkLSDynaPartCollection *parts,LSDynaMetaData *p,
    const vtkIdType& numWordsPerCell, const int& cellType)
    {
    //determine the relationship between the file bit size and
    //the host machine bit size. This allows us to read 64 bit files on a
    //32 bit machine. The Converter allows us to easily convert 32bit
    //arrays to 64bit arrays
    const int numWordsPerIdType (p->Fam.GetWordSize() / sizeof(T));
    const vtkIdType numFileWordsPerCell(numWordsPerCell * numWordsPerIdType);
    const vtkIdType offsetToMatId(numWordsPerIdType * cellLength);
    vtkIdType *conn;

    //This is a read RIGID_BODY and SHELL template specialization since it
    //has a weird weaving of cell types
    bool haveRigidMaterials = (p->Dict["MATTYP"] != 0) &&
                              p->RigidMaterials.size();

    vtkIdType nc=0, j=0,matlId=0;
    vtkIdType numCellsToSkip=0, numCellsToSkipEnd=0, chunkSize=0;

    //get from the part the read information for this lsdyna block type
    parts->GetPartReadInfo(LSDynaMetaData::SHELL,nc,numCellsToSkip,numCellsToSkipEnd);

    p->Fam.SkipWords(numFileWordsPerCell * numCellsToSkip); //skip to the right start id

    //buffer the amount in small chunks so we don't create a massive buffer
    vtkIdType numChunks = p->Fam.InitPartialChunkBuffering(nc,numWordsPerCell);
    int pType = 0;
    for(vtkIdType i=0; i < numChunks; ++i)
      {
      chunkSize = p->Fam.GetNextChunk( LSDynaFamily::Int);
      buff = p->Fam.GetBufferAs<T>();
      for (j=0; j<chunkSize;j+=numWordsPerCell)
        {
        conn = BC.convert(buff,offsetToMatId);
        buff+=offsetToMatId;
        matlId = static_cast<vtkIdType>(*buff);
        buff+=numWordsPerIdType;

        if ( haveRigidMaterials &&
          p->RigidMaterials.find( matlId ) == p->RigidMaterials.end())
          {
          pType = LSDynaMetaData::RIGID_BODY;
          }
        else
          {
          pType = LSDynaMetaData::SHELL;
          }
        parts->InsertCell(pType,matlId,cellType,cellLength,conn);
        }
      }
    p->Fam.SkipWords(numFileWordsPerCell * numCellsToSkipEnd);
    }
  };

template<int wordSize,int cellLength>
  struct FillBlock<LSDynaMetaData::ROAD_SURFACE,wordSize,cellLength>
  {
  template<typename T>
  FillBlock(T*, vtkLSDynaPartCollection *parts,LSDynaMetaData *p,
    const vtkIdType&, const int& cellType)
    {
    //This is a ROAD_SURFACE specialization
    //has a weird weaving of cell types
    vtkIdType nc=0,segId=0,segSz=0;
    vtkIdType numCellsToSkip=0, numCellsToSkipEnd=0;

    //get from the part the read information for this lsdyna block type
    parts->GetPartReadInfo(LSDynaMetaData::SHELL,nc,numCellsToSkip,numCellsToSkipEnd);

    //the road surface format is horrible for parallel reading.
    //we don't know the number of cells in each road surface.
    //only the total number of cells. So we have to do some fun stuff to correctly skip
    //this is unoptimized since I don't have any road surface data
    vtkIdType currentCell=0;
    vtkIdType conn[4];
    for (vtkIdType i=0; i<p->Dict["NSURF"]; ++i)
      {
      p->Fam.BufferChunk( LSDynaFamily::Int, 2 );
      segId = p->Fam.GetNextWordAsInt();
      segSz = p->Fam.GetNextWordAsInt();
      p->Fam.BufferChunk( LSDynaFamily::Int, 4*segSz );
      for (vtkIdType t=0; t<segSz; ++t, ++currentCell)
        {
        if(currentCell >= numCellsToSkip)
          {
          for (int j=0; j<4; ++j )
            {
            conn[j] = p->Fam.GetNextWordAsInt() - 1;
            }
          parts->InsertCell(LSDynaMetaData::ROAD_SURFACE,segId,cellType,4,conn);
          }
        }
      }
    }
  };

}
// =================================================== Start of public interface
vtkLSDynaReader::vtkLSDynaReader()
{
  this->P = new LSDynaMetaData;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->TimeStepRange[0] = 0;
  this->TimeStepRange[1] = 0;
  this->DeformedMesh = 1;
  this->RemoveDeletedCells = 1;
  this->DeletedCellsAsGhostArray = 0;
  this->InputDeck = 0;
  this->Parts = NULL;
}

vtkLSDynaReader::~vtkLSDynaReader()
{
  this->ResetPartsCache();
  this->SetInputDeck(0);
  delete this->P;
  this->P = 0;
}

void vtkLSDynaReader::PrintSelf( ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );

  os << indent << "Title: \"" << this->GetTitle() << "\"" << endl;
  os << indent << "InputDeck: " << (this->InputDeck ? this->InputDeck : "(null)") << endl;
  os << indent << "DeformedMesh: " << (this->DeformedMesh ? "On" : "Off") << endl;
  os << indent << "RemoveDeletedCells: " << (this->RemoveDeletedCells ? "On" : "Off") << endl;
  os << indent << "TimeStepRange: " << this->TimeStepRange[0] << ", " << this->TimeStepRange[1] << endl;

  if (this->P)
    {
    os << indent << "PrivateData: " << this->P << endl;
    }
  else
    {
    os << indent << "PrivateData: (none)" << endl;
    }
  os << indent << "Show Deleted Cells as Ghost Cells: "<<
        (this->DeletedCellsAsGhostArray ? "On" : "Off") << endl;

  os << indent << "Dimensionality: " << this->GetDimensionality() << endl;
  os << indent << "Nodes: " << this->GetNumberOfNodes() << endl;
  os << indent << "Cells: " << this->GetNumberOfCells() << endl;

  os << indent << "PointArrays: ";
  for ( int i=0; i<this->GetNumberOfPointArrays(); ++i )
    {
    os << this->GetPointArrayName( i ) << " ";
    }
  os << endl;

  os << "CellArrays: " << endl;
  for ( int ct = 0; ct < LSDynaMetaData::NUM_CELL_TYPES; ++ct )
    {
    os << vtkLSDynaCellTypes[ct] << ":" << endl;
    for ( int i = 0; i < this->GetNumberOfCellArrays( ct ); ++i )
      {
      os << this->GetCellArrayName( ct, i ) << " ";
      }
    os << endl;
    }
  os << endl;

  os << indent << "Time Steps: " << this->GetNumberOfTimeSteps() << endl;
  for ( int j=0; j<this->GetNumberOfTimeSteps(); ++j )
    {
    os.precision(5);
    os.width(12);
    os << this->GetTimeValue(j) ;
    if ( (j+1) % 8 == 0 && j != this->GetNumberOfTimeSteps()-1 )
      {
      os << endl << indent;
      }
    else
      {
      os << " ";
      }
    }
  os << endl;
}

void vtkLSDynaReader::Dump( ostream& os )
{
  vtkIndent indent;
  os << indent << "Title: \"" << this->GetTitle() << "\"" << endl
     << indent << "DeformedMesh: " << (this->DeformedMesh ? "On" : "Off") << endl
     << indent << "RemoveDeletedCells: " << (this->RemoveDeletedCells ? "On" : "Off") << endl
     << indent << "TimeStepRange: " << this->TimeStepRange[0] << ", " << this->TimeStepRange[1] << endl
     << indent << "PrivateData: " << this->P << endl
     << indent << "Dimensionality: " << this->GetDimensionality() << endl
     << indent << "Nodes: " << this->GetNumberOfNodes() << endl
     << indent << "Cells: " << this->GetNumberOfCells() << endl
     << indent << "PointArrays:    ";
  for ( int i=0; i<this->GetNumberOfPointArrays(); ++i )
    {
    os << this->GetPointArrayName( i ) << " ";
    }
  os << endl
     << "CellArrays:" << endl;
  for ( int ct = 0; ct < LSDynaMetaData::NUM_CELL_TYPES; ++ct )
    {
    os << vtkLSDynaCellTypes[ct] << ":" << endl;
    for ( int i = 0; i < this->GetNumberOfCellArrays( ct ); ++i )
      {
      os << this->GetCellArrayName( ct, i ) << " ";
      }
    os << endl;
    }
  os << endl;

  os << indent << "Time Steps:       " << this->GetNumberOfTimeSteps() << endl;
  for ( int j=0; j<this->GetNumberOfTimeSteps(); ++j )
    {
    os.precision(5);
    os.width(12);
    os << this->GetTimeValue(j) ;
    if ( (j+1) % 8 == 0 && j != this->GetNumberOfTimeSteps()-1 )
      {
      os << endl << indent;
      }
    else
      {
      os << " ";
      }
    }
  os << endl;
}

void vtkLSDynaReader::DebugDump()
{
  this->Dump( cout );
}

int vtkLSDynaReader::CanReadFile( const char* fname )
{
  if ( ! fname )
    return 0;

  std::string dbDir = vtksys::SystemTools::GetFilenamePath( fname );
  std::string dbName = vtksys::SystemTools::GetFilenameName( fname );
  std::string dbExt;
  std::string::size_type dot;
  LSDynaMetaData* p = new LSDynaMetaData;
  int result = 0;

  // GetFilenameExtension doesn't look for the rightmost "." ... do it ourselves.
  dot = dbName.rfind( '.' );
  if ( dot != std::string::npos )
    {
    dbExt = dbName.substr( dot );
    }
  else
    {
    dbExt = "";
    }

  p->Fam.SetDatabaseDirectory( dbDir );

  if ( dbExt == ".k" || dbExt == ".lsdyna" )
    {
    p->Fam.SetDatabaseBaseName( "/d3plot" );
    }
  else
    {
    struct stat st;
    if ( stat( fname, &st ) == 0 )
      {
      dbName.insert( 0, "/" );
      p->Fam.SetDatabaseBaseName( dbName.c_str() );
      }
    else
      {
      p->Fam.SetDatabaseBaseName( "/d3plot" );
      }
    }
  // If the time step is set before RequestInformation is called, we must
  // read the header information immediately in order to determine whether
  // the timestep that's been passed is valid. If it's not, we ignore it.
  if ( ! p->FileIsValid )
    {
    if ( p->Fam.GetDatabaseDirectory().empty() )
      {
      result = -1;
      }
    else
      {
      if ( p->Fam.GetDatabaseBaseName().empty() )
        {
        p->Fam.SetDatabaseBaseName( "/d3plot" ); // not a bad assumption.
        }
      p->Fam.ScanDatabaseDirectory();
      if ( p->Fam.GetNumberOfFiles() < 1 )
        {
        result = -1;
        }
      else
        {
        if ( p->Fam.DetermineStorageModel() != 0 )
          result = 0;
        else
          result = 1;
        }
      }
    }
  delete p;

  return result > 0; // -1 and 0 are both problems, 1 indicates success.
}

void vtkLSDynaReader::SetDatabaseDirectory( const char* f )
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting DatabaseDirectory to " << f );
  if ( ! f )
    {
    if ( ! this->P->Fam.GetDatabaseDirectory().empty() )
      { // no string => no database directory
      this->P->Reset();
      this->SetInputDeck( 0 );
      this->ResetPartsCache();
      this->Modified();
      }
    return;
    }
  if ( strcmp(this->P->Fam.GetDatabaseDirectory().c_str(), f) )
    {
    this->P->Reset();
    this->SetInputDeck( 0 );
    this->P->Fam.SetDatabaseDirectory( std::string(f) );
    this->ResetPartsCache();
    this->Modified();
    }
}

const char* vtkLSDynaReader::GetDatabaseDirectory()
{
  return this->P->Fam.GetDatabaseDirectory().c_str();
}

int vtkLSDynaReader::IsDatabaseValid()
{
  return this->P->FileIsValid;
}

void vtkLSDynaReader::SetFileName( const char* f )
{
  std::string dbDir = vtksys::SystemTools::GetFilenamePath( f );
  std::string dbName = vtksys::SystemTools::GetFilenameName( f );
  std::string dbExt;
  std::string::size_type dot;

  // GetFilenameExtension doesn't look for the rightmost "." ... do it ourselves.
  dot = dbName.rfind( '.' );
  if ( dot != std::string::npos )
    {
    dbExt = dbName.substr( dot );
    }
  else
    {
    dbExt = "";
    }

  this->SetDatabaseDirectory( dbDir.c_str() );

  if ( dbExt == ".k" || dbExt == ".lsdyna" )
    {
    this->SetInputDeck( f );
    this->P->Fam.SetDatabaseBaseName( "/d3plot" );
    }
  else
    {
    struct stat st;
    if ( stat( f, &st ) == 0 )
      {
      dbName.insert( 0, "/" );
      this->P->Fam.SetDatabaseBaseName( dbName.c_str() );
      }
    else
      {
      this->P->Fam.SetDatabaseBaseName( "/d3plot" );
      }
    }
}

const char* vtkLSDynaReader::GetFileName()
{
  // This is completely thread UNsafe. But what to do?
  static std::string filenameSurrogate;
  filenameSurrogate = this->P->Fam.GetDatabaseDirectory() + "/d3plot";
  return filenameSurrogate.c_str();
}

char* vtkLSDynaReader::GetTitle()
{
  return this->P->Title;
}

int vtkLSDynaReader::GetDimensionality()
{
  return this->P->Dimensionality;
}

void vtkLSDynaReader::SetTimeStep( vtkIdType t )
{
  LSDynaMetaData* p = this->P;
  if ( p->CurrentState == t )
    {
    return;
    }

  // If the time step is set before RequestInformation is called, we must
  // read the header information immediately in order to determine whether
  // the timestep that's been passed is valid. If it's not, we ignore it.
  if ( ! p->FileIsValid )
    {
    if ( p->Fam.GetDatabaseDirectory().empty() )
      {
      vtkErrorMacro( "You haven't set the LS-Dyna database directory!" );
      return;
      }

    p->Fam.SetDatabaseBaseName( "/d3plot" ); // force this for now.
    p->Fam.ScanDatabaseDirectory();
    if ( p->Fam.GetNumberOfFiles() < 1 )
      {
      p->FileIsValid = 0;
      return;
      }
    p->Fam.DetermineStorageModel();
    p->MaxFileLength = p->FileSizeFactor*512*512*p->Fam.GetWordSize();
    p->FileIsValid = 1;

    // OK, now we have a list of files. Next, determine the length of the
    // state vector (#bytes of data stored per time step):
    this->ReadHeaderInformation( 0 );

    // Finally, we can loop through and determine where all the state
    // vectors start for each time step.
    this->ScanDatabaseTimeSteps();
    }

  // Now, make sure we update the dictionary to contain information
  // relevant to the adaptation level that matches the requested timestep.
  if ( t >= 0 && t < (int) p->TimeValues.size() )
    {
    if ( p->Fam.GetCurrentAdaptLevel() != p->Fam.TimeAdaptLevel( t ) )
      {
      if ( this->ReadHeaderInformation( p->Fam.TimeAdaptLevel( t ) ) == 0 )
        {
        // unable to read the header information for the adaptation level corresponding
        // to the requested time step
        return;
        }
      }
    }

  p->CurrentState = t;
  this->Modified();
}

vtkIdType vtkLSDynaReader::GetTimeStep()
{
  return this->P->CurrentState;
}

vtkIdType vtkLSDynaReader::GetNumberOfTimeSteps()
{
  return (vtkIdType) this->P->TimeValues.size();
}

double vtkLSDynaReader::GetTimeValue( vtkIdType s )
{
  if ( s < 0 || s >= (vtkIdType) this->P->TimeValues.size() )
    {
    return -1.0;
    }

  return this->P->TimeValues[s];
}

vtkIdType vtkLSDynaReader::GetNumberOfNodes()
{
  return this->P->NumberOfNodes;
}

vtkIdType vtkLSDynaReader::GetNumberOfCells()
{
  vtkIdType tmp=0;
  for ( int c=0; c<LSDynaMetaData::NUM_CELL_TYPES; ++c )
    {
    tmp += this->P->NumberOfCells[c];
    }
  return tmp;
}

vtkIdType vtkLSDynaReader::GetNumberOfSolidCells()
{
  return this->P->NumberOfCells[LSDynaMetaData::SOLID];
}

vtkIdType vtkLSDynaReader::GetNumberOfThickShellCells()
{
  return this->P->NumberOfCells[LSDynaMetaData::THICK_SHELL];
}

vtkIdType vtkLSDynaReader::GetNumberOfShellCells()
{
  return this->P->NumberOfCells[LSDynaMetaData::SHELL];
}

vtkIdType vtkLSDynaReader::GetNumberOfRigidBodyCells()
{
  return this->P->NumberOfCells[LSDynaMetaData::RIGID_BODY];
}

vtkIdType vtkLSDynaReader::GetNumberOfRoadSurfaceCells()
{
  return this->P->NumberOfCells[LSDynaMetaData::ROAD_SURFACE];
}

vtkIdType vtkLSDynaReader::GetNumberOfBeamCells()
{
  return this->P->NumberOfCells[LSDynaMetaData::BEAM];
}

vtkIdType vtkLSDynaReader::GetNumberOfParticleCells()
{
  return this->P->NumberOfCells[LSDynaMetaData::PARTICLE];
}

vtkIdType vtkLSDynaReader::GetNumberOfContinuumCells()
{
  vtkIdType tmp=0;
  for ( int c=LSDynaMetaData::PARTICLE+1; c<LSDynaMetaData::NUM_CELL_TYPES; ++c )
    {
    tmp += this->P->NumberOfCells[c];
    }
  return tmp;
}

// =================================== Point array queries
int vtkLSDynaReader::GetNumberOfPointArrays()
{
  return (int) this->P->PointArrayNames.size();
}

const char* vtkLSDynaReader::GetPointArrayName( int a )
{
  if ( a < 0 || a >= (int) this->P->PointArrayNames.size() )
    return 0;

  return this->P->PointArrayNames[a].c_str();
}

int vtkLSDynaReader::GetPointArrayStatus( int a )
{
  if ( a < 0 || a >= (int) this->P->PointArrayStatus.size() )
    return 0;

  return this->P->PointArrayStatus[a];
}

void vtkLSDynaReader::SetPointArrayStatus( int a, int stat )
{
  if ( a < 0 || a >= (int) this->P->PointArrayStatus.size() )
    {
    vtkWarningMacro( "Cannot set status of non-existent point array " << a );
    return;
    }

  if ( stat == this->P->PointArrayStatus[a] )
    return;

  this->P->PointArrayStatus[a] = stat;
  this->ResetPartsCache();
  this->Modified();
}

int vtkLSDynaReader::GetNumberOfComponentsInPointArray( int a )
{
  if ( a < 0 || a >= (int) this->P->PointArrayStatus.size() )
    return 0;

  return this->P->PointArrayComponents[a];
}

// =================================== Cell array queries
int vtkLSDynaReader::GetNumberOfCellArrays( int ct )
{
  return (int) this->P->CellArrayNames[ct].size();
}

const char* vtkLSDynaReader::GetCellArrayName( int ct, int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayNames[ct].size() )
    return 0;

  return this->P->CellArrayNames[ct][a].c_str();
}

int vtkLSDynaReader::GetCellArrayStatus( int ct, int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[ct].size() )
    return 0;

  return this->P->CellArrayStatus[ct][a];
}

int vtkLSDynaReader::GetNumberOfComponentsInCellArray( int ct, int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[ct].size() )
    return 0;

  return this->P->CellArrayComponents[ct][a];
}

void vtkLSDynaReader::SetCellArrayStatus( int ct, int a, int stat )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[ct].size() )
    {
    vtkWarningMacro( "Cannot set status of non-existent point array " << a );
    return;
    }

  if ( stat == this->P->CellArrayStatus[ct][a] )
    return;

  this->P->CellArrayStatus[ct][a] = stat;
  this->ResetPartsCache();
  this->Modified();
}

// =================================== Cell array queries
int vtkLSDynaReader::GetNumberOfSolidArrays()
{
  return (int) this->P->CellArrayNames[LSDynaMetaData::SOLID].size();
}

const char* vtkLSDynaReader::GetSolidArrayName( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayNames[LSDynaMetaData::SOLID].size() )
    return 0;

  return this->P->CellArrayNames[LSDynaMetaData::SOLID][a].c_str();
}

int vtkLSDynaReader::GetSolidArrayStatus( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[LSDynaMetaData::SOLID].size() )
    return 0;

  return this->P->CellArrayStatus[LSDynaMetaData::SOLID][a];
}

int vtkLSDynaReader::GetNumberOfComponentsInSolidArray( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[LSDynaMetaData::SOLID].size() )
    return 0;

  return this->P->CellArrayComponents[LSDynaMetaData::SOLID][a];
}

void vtkLSDynaReader::SetSolidArrayStatus( int a, int stat )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[LSDynaMetaData::SOLID].size() )
    {
    vtkWarningMacro( "Cannot set status of non-existent point array " << a );
    return;
    }

  if ( stat == this->P->CellArrayStatus[LSDynaMetaData::SOLID][a] )
    return;

  this->P->CellArrayStatus[LSDynaMetaData::SOLID][a] = stat;
  this->ResetPartsCache();
  this->Modified();
}

// =================================== Cell array queries
int vtkLSDynaReader::GetNumberOfThickShellArrays()
{
  return (int) this->P->CellArrayNames[LSDynaMetaData::THICK_SHELL].size();
}

const char* vtkLSDynaReader::GetThickShellArrayName( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayNames[LSDynaMetaData::THICK_SHELL].size() )
    return 0;

  return this->P->CellArrayNames[LSDynaMetaData::THICK_SHELL][a].c_str();
}

int vtkLSDynaReader::GetThickShellArrayStatus( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[LSDynaMetaData::THICK_SHELL].size() )
    return 0;

  return this->P->CellArrayStatus[LSDynaMetaData::THICK_SHELL][a];
}

int vtkLSDynaReader::GetNumberOfComponentsInThickShellArray( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[LSDynaMetaData::THICK_SHELL].size() )
    return 0;

  return this->P->CellArrayComponents[LSDynaMetaData::THICK_SHELL][a];
}

void vtkLSDynaReader::SetThickShellArrayStatus( int a, int stat )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[LSDynaMetaData::THICK_SHELL].size() )
    {
    vtkWarningMacro( "Cannot set status of non-existent point array " << a );
    return;
    }

  if ( stat == this->P->CellArrayStatus[LSDynaMetaData::THICK_SHELL][a] )
    return;

  this->P->CellArrayStatus[LSDynaMetaData::THICK_SHELL][a] = stat;
  this->ResetPartsCache();
  this->Modified();
}

// =================================== Cell array queries
int vtkLSDynaReader::GetNumberOfShellArrays()
{
  return (int) this->P->CellArrayNames[LSDynaMetaData::SHELL].size();
}

const char* vtkLSDynaReader::GetShellArrayName( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayNames[LSDynaMetaData::SHELL].size() )
    return 0;

  return this->P->CellArrayNames[LSDynaMetaData::SHELL][a].c_str();
}

int vtkLSDynaReader::GetShellArrayStatus( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[LSDynaMetaData::SHELL].size() )
    return 0;

  return this->P->CellArrayStatus[LSDynaMetaData::SHELL][a];
}

int vtkLSDynaReader::GetNumberOfComponentsInShellArray( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[LSDynaMetaData::SHELL].size() )
    return 0;

  return this->P->CellArrayComponents[LSDynaMetaData::SHELL][a];
}

void vtkLSDynaReader::SetShellArrayStatus( int a, int stat )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[LSDynaMetaData::SHELL].size() )
    {
    vtkWarningMacro( "Cannot set status of non-existent point array " << a );
    return;
    }

  if ( stat == this->P->CellArrayStatus[LSDynaMetaData::SHELL][a] )
    return;

  this->P->CellArrayStatus[LSDynaMetaData::SHELL][a] = stat;
  this->ResetPartsCache();
  this->Modified();
}

// =================================== Cell array queries
int vtkLSDynaReader::GetNumberOfRigidBodyArrays()
{
  return (int) this->P->CellArrayNames[LSDynaMetaData::RIGID_BODY].size();
}

const char* vtkLSDynaReader::GetRigidBodyArrayName( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayNames[LSDynaMetaData::RIGID_BODY].size() )
    return 0;

  return this->P->CellArrayNames[LSDynaMetaData::RIGID_BODY][a].c_str();
}

int vtkLSDynaReader::GetRigidBodyArrayStatus( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[LSDynaMetaData::RIGID_BODY].size() )
    return 0;

  return this->P->CellArrayStatus[LSDynaMetaData::RIGID_BODY][a];
}

int vtkLSDynaReader::GetNumberOfComponentsInRigidBodyArray( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[LSDynaMetaData::RIGID_BODY].size() )
    return 0;

  return this->P->CellArrayComponents[LSDynaMetaData::RIGID_BODY][a];
}

void vtkLSDynaReader::SetRigidBodyArrayStatus( int a, int stat )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[LSDynaMetaData::RIGID_BODY].size() )
    {
    vtkWarningMacro( "Cannot set status of non-existent point array " << a );
    return;
    }

  if ( stat == this->P->CellArrayStatus[LSDynaMetaData::RIGID_BODY][a] )
    return;

  this->P->CellArrayStatus[LSDynaMetaData::RIGID_BODY][a] = stat;
  this->ResetPartsCache();
  this->Modified();
}

// =================================== Cell array queries
int vtkLSDynaReader::GetNumberOfRoadSurfaceArrays()
{
  return (int) this->P->CellArrayNames[LSDynaMetaData::ROAD_SURFACE].size();
}

const char* vtkLSDynaReader::GetRoadSurfaceArrayName( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayNames[LSDynaMetaData::ROAD_SURFACE].size() )
    return 0;

  return this->P->CellArrayNames[LSDynaMetaData::ROAD_SURFACE][a].c_str();
}

int vtkLSDynaReader::GetRoadSurfaceArrayStatus( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[LSDynaMetaData::ROAD_SURFACE].size() )
    return 0;

  return this->P->CellArrayStatus[LSDynaMetaData::ROAD_SURFACE][a];
}

int vtkLSDynaReader::GetNumberOfComponentsInRoadSurfaceArray( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[LSDynaMetaData::ROAD_SURFACE].size() )
    return 0;

  return this->P->CellArrayComponents[LSDynaMetaData::ROAD_SURFACE][a];
}

void vtkLSDynaReader::SetRoadSurfaceArrayStatus( int a, int stat )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[LSDynaMetaData::ROAD_SURFACE].size() )
    {
    vtkWarningMacro( "Cannot set status of non-existent point array " << a );
    return;
    }

  if ( stat == this->P->CellArrayStatus[LSDynaMetaData::ROAD_SURFACE][a] )
    return;

  this->P->CellArrayStatus[LSDynaMetaData::ROAD_SURFACE][a] = stat;
  this->ResetPartsCache();
  this->Modified();
}

// =================================== Cell array queries
int vtkLSDynaReader::GetNumberOfBeamArrays()
{
  return (int) this->P->CellArrayNames[LSDynaMetaData::BEAM].size();
}

const char* vtkLSDynaReader::GetBeamArrayName( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayNames[LSDynaMetaData::BEAM].size() )
    return 0;

  return this->P->CellArrayNames[LSDynaMetaData::BEAM][a].c_str();
}

int vtkLSDynaReader::GetBeamArrayStatus( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[LSDynaMetaData::BEAM].size() )
    return 0;

  return this->P->CellArrayStatus[LSDynaMetaData::BEAM][a];
}

int vtkLSDynaReader::GetNumberOfComponentsInBeamArray( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[LSDynaMetaData::BEAM].size() )
    return 0;

  return this->P->CellArrayComponents[LSDynaMetaData::BEAM][a];
}

void vtkLSDynaReader::SetBeamArrayStatus( int a, int stat )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[LSDynaMetaData::BEAM].size() )
    {
    vtkWarningMacro( "Cannot set status of non-existent point array " << a );
    return;
    }

  if ( stat == this->P->CellArrayStatus[LSDynaMetaData::BEAM][a] )
    return;

  this->P->CellArrayStatus[LSDynaMetaData::BEAM][a] = stat;
  this->ResetPartsCache();
  this->Modified();
}

// =================================== Cell array queries
int vtkLSDynaReader::GetNumberOfParticleArrays()
{
  return (int) this->P->CellArrayNames[LSDynaMetaData::PARTICLE].size();
}

const char* vtkLSDynaReader::GetParticleArrayName( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayNames[LSDynaMetaData::PARTICLE].size() )
    return 0;

  return this->P->CellArrayNames[LSDynaMetaData::PARTICLE][a].c_str();
}

int vtkLSDynaReader::GetParticleArrayStatus( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[LSDynaMetaData::PARTICLE].size() )
    return 0;

  return this->P->CellArrayStatus[LSDynaMetaData::PARTICLE][a];
}

int vtkLSDynaReader::GetNumberOfComponentsInParticleArray( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[LSDynaMetaData::PARTICLE].size() )
    return 0;

  return this->P->CellArrayComponents[LSDynaMetaData::PARTICLE][a];
}

void vtkLSDynaReader::SetParticleArrayStatus( int a, int stat )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[LSDynaMetaData::PARTICLE].size() )
    {
    vtkWarningMacro( "Cannot set status of non-existent point array " << a );
    return;
    }

  if ( stat == this->P->CellArrayStatus[LSDynaMetaData::PARTICLE][a] )
    return;

  this->P->CellArrayStatus[LSDynaMetaData::PARTICLE][a] = stat;
  this->ResetPartsCache();
  this->Modified();
}

// =================================== Part queries
int vtkLSDynaReader::GetNumberOfPartArrays()
{
  return (int) this->P->PartNames.size();
}

const char* vtkLSDynaReader::GetPartArrayName( int a )
{
  if ( a < 0 || a >= (int) this->P->PartNames.size() )
    return 0;

  return this->P->PartNames[a].c_str();
}

int vtkLSDynaReader::GetPartArrayStatus( int a )
{
  if ( a < 0 || a >= (int) this->P->PartStatus.size() )
    return 0;

  return this->P->PartStatus[a];
}

void vtkLSDynaReader::SetPartArrayStatus( int a, int stat )
{
  if ( a < 0 || a >= (int) this->P->PartStatus.size() )
    {
    vtkWarningMacro( "Cannot set status of non-existent point array " << a );
    return;
    }

  if ( stat == this->P->PartStatus[a] )
    return;

  this->P->PartStatus[a] = stat;
  this->ResetPartsCache();
  this->Modified();
}

// ===================================
void vtkLSDynaReader::ResetPartsCache()
{
  if(this->Parts)
    {
    this->Parts->Delete();
    this->Parts=NULL;
    }
}

// =================================== Read the control word header for the current adaptation level
int vtkLSDynaReader::ReadHeaderInformation( int curAdapt )
{
  LSDynaMetaData* p = this->P;

  // =================================== Control Word Section
  p->Fam.SkipToWord( LSDynaFamily::ControlSection, curAdapt /*timestep*/, 0 );
  p->Fam.BufferChunk( LSDynaFamily::Char, 10 );
  memcpy( p->Title, p->Fam.GetNextWordAsChars(), 40*sizeof(char) );
  p->Title[40] = '\0';

  p->Fam.SkipToWord( LSDynaFamily::ControlSection, curAdapt /*timestep*/, 13 );
  p->Fam.BufferChunk( LSDynaFamily::Int, 1 );
  p->Dict["Code"] = p->Fam.GetNextWordAsInt();
  p->Fam.BufferChunk( LSDynaFamily::Float, 1 );
  p->Dict["Version"] = vtkIdType(p->Fam.GetNextWordAsFloat());
  p->Fam.BufferChunk( LSDynaFamily::Int, 49 );
  p->Dict[    "NDIM"] = p->Fam.GetNextWordAsInt();
  p->Dict[   "NUMNP"] = p->Fam.GetNextWordAsInt();
  p->Dict[   "ICODE"] = p->Fam.GetNextWordAsInt();
  p->Dict[   "NGLBV"] = p->Fam.GetNextWordAsInt();
  p->Dict[      "IT"] = p->Fam.GetNextWordAsInt();
  p->Dict[      "IU"] = p->Fam.GetNextWordAsInt();
  p->Dict[      "IV"] = p->Fam.GetNextWordAsInt();
  p->Dict[      "IA"] = p->Fam.GetNextWordAsInt();
  p->Dict[    "NEL8"] = p->Fam.GetNextWordAsInt();
  p->Dict[ "NUMMAT8"] = p->Fam.GetNextWordAsInt();
  p->Fam.GetNextWordAsInt(); // BLANK
  p->Fam.GetNextWordAsInt(); // BLANK
  p->Dict[    "NV3D"] = p->Fam.GetNextWordAsInt();
  p->Dict[    "NEL2"] = p->Fam.GetNextWordAsInt();
  p->Dict[ "NUMMAT2"] = p->Fam.GetNextWordAsInt();
  p->Dict[    "NV1D"] = p->Fam.GetNextWordAsInt();
  p->Dict[    "NEL4"] = p->Fam.GetNextWordAsInt();
  p->Dict[ "NUMMAT4"] = p->Fam.GetNextWordAsInt();
  p->Dict[    "NV2D"] = p->Fam.GetNextWordAsInt();
  p->Dict[   "NEIPH"] = p->Fam.GetNextWordAsInt();
  p->Dict[   "NEIPS"] = p->Fam.GetNextWordAsInt();
  p->Dict[  "MAXINT"] = p->Fam.GetNextWordAsInt();
  // do MDLOPT here?
  p->Dict[   "NMSPH"] = p->Fam.GetNextWordAsInt();
  p->Dict[  "EDLOPT"] = p->Dict["NMSPH"]; // EDLOPT is not standard
  p->Dict[  "NGPSPH"] = p->Fam.GetNextWordAsInt();
  p->Dict[   "NARBS"] = p->Fam.GetNextWordAsInt();
  p->Dict[    "NELT"] = p->Fam.GetNextWordAsInt();
  p->Dict[ "NUMMATT"] = p->Fam.GetNextWordAsInt();
  p->Dict[   "NV3DT"] = p->Fam.GetNextWordAsInt();
  p->Dict["IOSHL(1)"] = p->Fam.GetNextWordAsInt() == 1000 ? 1 : 0;
  p->Dict["IOSHL(2)"] = p->Fam.GetNextWordAsInt() == 1000 ? 1 : 0;
  p->Dict["IOSHL(3)"] = p->Fam.GetNextWordAsInt() == 1000 ? 1 : 0;
  p->Dict["IOSHL(4)"] = p->Fam.GetNextWordAsInt() == 1000 ? 1 : 0;
  p->Dict[ "IALEMAT"] = p->Fam.GetNextWordAsInt();
  p->Dict[  "NCFDV1"] = p->Fam.GetNextWordAsInt();
  p->Dict[  "NCFDV2"] = p->Fam.GetNextWordAsInt();
  p->Dict[  "NADAPT"] = p->Fam.GetNextWordAsInt();
  p->Fam.GetNextWordAsInt(); // BLANK

  // Compute the derived values in this->P
  // =========================================== Control Word Section Processing
  int itmp;
  vtkIdType iddtmp;
  char ctmp[128]; // temp space for generating keywords (i.e. isphfg) and attribute names (i.e., StressIntPt3)

  // --- Initialize some values
  p->ReadRigidRoadMvmt = 0;
  p->PreStateSize = 64*p->Fam.GetWordSize();
  p->StateSize = p->Fam.GetWordSize(); // Account for "time word"
  p->Dimensionality = p->Dict["NDIM"];
  switch (p->Dimensionality)
    {
  case 2:
  case 3:
    p->Dict["MATTYP"] = 0;
    p->ConnectivityUnpacked = 0;
    break;
  case 7:
    p->ReadRigidRoadMvmt = 1;
  case 5:
    p->Dict["MATTYP"] = 1;
    p->ConnectivityUnpacked = 1;
    p->Dimensionality = 3;
    break;
  case 4:
    p->ConnectivityUnpacked = 1;
    p->Dict["MATTYP"] = 0;
    p->Dimensionality = 3;
    break;
  default:
    vtkErrorMacro("Unknown Dimensionality " << p->Dimensionality << " encountered" );
    p->FileIsValid = 0;
    return 0;
    }

  // FIXME Are these marks valid since we are marking the word past the end of the chunk?
  p->Fam.MarkSectionStart( curAdapt, LSDynaFamily::StaticSection );
  p->Fam.MarkSectionStart( curAdapt, LSDynaFamily::MaterialTypeData );
  if ( p->Dict["MATTYP"] != 0 )
    {
    p->Fam.BufferChunk( LSDynaFamily::Int, 2 );
    p->Dict["NUMRBE"] = p->Fam.GetNextWordAsInt();
    p->Dict["NUMMAT"] = p->Fam.GetNextWordAsInt();
    }
  else
    {
    p->Dict["NUMRBE"] = 0;
    p->Dict["NUMMAT"] = 0;
    }
  p->NumberOfNodes = p->Dict["NUMNP"];

  p->NumberOfCells[LSDynaMetaData::RIGID_BODY] = p->Dict["NUMRBE"];
  p->NumberOfCells[LSDynaMetaData::SOLID] = p->Dict["NEL8"];
  p->NumberOfCells[LSDynaMetaData::THICK_SHELL] = p->Dict["NELT"];
  p->NumberOfCells[LSDynaMetaData::SHELL] = p->Dict["NEL4"];
  p->NumberOfCells[LSDynaMetaData::BEAM] = p->Dict["NEL2"];
  p->NumberOfCells[LSDynaMetaData::PARTICLE] = p->Dict["NMSPH"];

  p->StateSize += p->Dict["NGLBV"]*p->Fam.GetWordSize();

  if ( p->Dict["IT"] != 0 )
    {
    p->AddPointArray( LS_ARRAYNAME_TEMPERATURE, 1, 1 );
    p->StateSize += p->NumberOfNodes * p->Fam.GetWordSize();
    }
  if ( p->Dict["IU"] != 0 )
    {
    p->AddPointArray( LS_ARRAYNAME_DEFLECTION, p->Dimensionality, 1 );
    p->StateSize += p->NumberOfNodes * p->Dimensionality * p->Fam.GetWordSize();
    }
  if ( p->Dict["IV"] != 0 )
    {
    p->AddPointArray( LS_ARRAYNAME_VELOCITY, p->Dimensionality, 1 );
    p->StateSize += p->NumberOfNodes * p->Dimensionality * p->Fam.GetWordSize();
    }
  if ( p->Dict["IA"] != 0 )
    {
    p->AddPointArray( LS_ARRAYNAME_ACCELERATION, p->Dimensionality, 1 );
    p->StateSize += p->NumberOfNodes * p->Dimensionality * p->Fam.GetWordSize();
    }
  p->Dict["cfdPressure"] = 0;
  p->Dict["cfdVort"] = 0;
  p->Dict["cfdXVort"] = 0;
  p->Dict["cfdYVort"] = 0;
  p->Dict["cfdZVort"] = 0;
  p->Dict["cfdRVort"] = 0;
  p->Dict["cfdEnstrophy"] = 0;
  p->Dict["cfdHelicity"] = 0;
  p->Dict["cfdStream"] = 0;
  p->Dict["cfdEnthalpy"] = 0;
  p->Dict["cfdDensity"] = 0;
  p->Dict["cfdTurbKE"] = 0;
  p->Dict["cfdDiss"] = 0;
  p->Dict["cfdEddyVisc"] = 0;
  itmp = p->Dict["NCFDV1"];
  if ( itmp & 2 )
    {
    p->AddPointArray( LS_ARRAYNAME_PRESSURE, 1, 1 );
    p->StateSize += p->NumberOfNodes * p->Fam.GetWordSize();
    p->Dict["cfdPressure"] = 1;
    }
  if ( (itmp & 28) == 28 )
    {
    p->AddPointArray( LS_ARRAYNAME_VORTICITY, 3, 1 );
    p->StateSize += p->NumberOfNodes * 3 * p->Fam.GetWordSize();
    p->Dict["cfdVort"] = 1;
    p->Dict["cfdXVort"] = 1;
    p->Dict["cfdYVort"] = 1;
    p->Dict["cfdZVort"] = 1;
    }
  else
    { // OK, we don't have all the vector components... maybe we have some of them?
    if ( itmp & 4 )
      {
      p->AddPointArray( LS_ARRAYNAME_VORTICITY "_X", 1, 1 );
      p->StateSize += p->NumberOfNodes * p->Fam.GetWordSize();
      p->Dict["cfdXVort"] = 1;
      }
    if ( itmp & 8 )
      {
      p->AddPointArray( LS_ARRAYNAME_VORTICITY "_Y", 1, 1 );
      p->StateSize += p->NumberOfNodes * p->Fam.GetWordSize();
      p->Dict["cfdYVort"] = 1;
      }
    if ( itmp & 16 )
      {
      p->AddPointArray( LS_ARRAYNAME_VORTICITY "_Z", 1, 1 );
      p->StateSize += p->NumberOfNodes * p->Fam.GetWordSize();
      p->Dict["cfdZVort"] = 1;
      }
    }
  if ( itmp & 32 )
    {
    p->AddPointArray( LS_ARRAYNAME_RESULTANTVORTICITY, 1, 1 );
    p->StateSize += p->NumberOfNodes * p->Fam.GetWordSize();
    p->Dict["cfdRVort"] = 1;
  if ( itmp & 64 )
    {
    p->AddPointArray( LS_ARRAYNAME_ENSTROPHY, 1, 1 );
    p->StateSize += p->NumberOfNodes * p->Fam.GetWordSize();
    p->Dict["cfdEnstrophy"] = 1;
    }
  if ( itmp & 128 )
    {
    p->AddPointArray( LS_ARRAYNAME_HELICITY, 1, 1 );
    p->StateSize += p->NumberOfNodes * p->Fam.GetWordSize();
    p->Dict["cfdHelicity"] = 1;
    }
  if ( itmp & 256 )
    {
    p->AddPointArray( LS_ARRAYNAME_STREAMFUNCTION, 1, 1 );
    p->StateSize += p->NumberOfNodes * p->Fam.GetWordSize();
    p->Dict["cfdStream"] = 1;
    }
  if ( itmp & 512 )
    {
    p->AddPointArray( LS_ARRAYNAME_ENTHALPY, 1, 1 );
    p->StateSize += p->NumberOfNodes * p->Fam.GetWordSize();
    p->Dict["cfdEnthalpy"] = 1;
    }
  if ( itmp & 1024 )
    {
    p->AddPointArray( LS_ARRAYNAME_DENSITY, 1, 1 );
    p->StateSize += p->NumberOfNodes * p->Fam.GetWordSize();
    p->Dict["cfdDensity"] = 1;
    }
  if ( itmp & 2048 )
    {
    p->AddPointArray( LS_ARRAYNAME_TURBULENTKE, 1, 1 );
    p->StateSize += p->NumberOfNodes * p->Fam.GetWordSize();
    p->Dict["cfdTurbKE"] = 1;
    }
  if ( itmp & 4096 )
    {
    p->AddPointArray( LS_ARRAYNAME_DISSIPATION, 1, 1 );
    p->StateSize += p->NumberOfNodes * p->Fam.GetWordSize();
    p->Dict["cfdDiss"] = 1;
    }
  if ( itmp & 1040384 )
    {
    p->AddPointArray( LS_ARRAYNAME_EDDYVISCOSITY, 1, 1 );
    p->StateSize += p->NumberOfNodes * p->Fam.GetWordSize();
    p->Dict["cfdEddyVisc"] = 1;
    }
    }

  char sname[]=LS_ARRAYNAME_SPECIES_BLNK;
  iddtmp = p->Dict["NCFDV2"];
  for ( itmp=1; itmp<11; ++itmp )
    {
    if ( iddtmp & (vtkIdType)(1<<itmp) )
      {
      sprintf( sname, LS_ARRAYNAME_SPECIES_FMT, itmp );
      p->AddPointArray( sname, 1, 1 );
      p->StateSize += p->NumberOfNodes * p->Fam.GetWordSize();
      sprintf( sname, "cfdSpec%02d", itmp );
      p->Dict[ sname ] = 1;
      }
    else
      {
      sprintf( sname, "cfdSpec%02d", itmp );
      p->Dict[ sname ] = 0;
      }
    }

  // Solid element state size   FIXME: 7 + NEIPH should really be NV3D (in case things change)
  p->StateSize += (7+p->Dict["NEIPH"])*p->NumberOfCells[LSDynaMetaData::SOLID]*p->Fam.GetWordSize();
  // Thick shell state size
  p->StateSize += p->Dict["NV3DT"]*p->NumberOfCells[LSDynaMetaData::THICK_SHELL]*p->Fam.GetWordSize();
  // (Thin) shell state size (we remove rigid body shell element state below)
  p->StateSize += p->Dict["NV2D"]*p->NumberOfCells[LSDynaMetaData::SHELL]*p->Fam.GetWordSize();
  // Beam state size
  p->StateSize += p->Dict["NV1D"]*p->NumberOfCells[LSDynaMetaData::BEAM]*p->Fam.GetWordSize();

  // ================================================ Static Information Section
  // This is marked above so we can read NUMRBE in time to do StateSize calculations
  // ================================================ Material Type Data Section
  // This is marked above so we can read NUMRBE in time to do StateSize calculations
  if ( p->Dict["MATTYP"] != 0 )
    {
    // If there are rigid body elements, they won't have state data and we must
    // reduce the state size
    p->StateSize -= p->Dict["NV2D"] * p->NumberOfCells[ LSDynaMetaData::RIGID_BODY ];

    p->Fam.BufferChunk( LSDynaFamily::Int, p->Dict["NUMMAT"] );
    for ( itmp = 0; itmp < p->Dict["NUMMAT"]; ++itmp )
      {
      p->RigidMaterials.insert( p->Fam.GetNextWordAsInt() );
      }
    p->PreStateSize += (2 + p->Dict["NUMMAT"])*p->Fam.GetWordSize();
    }

  //process the deletion array
  //save the position we currently have as it is the offset to jump to
  //when reading deletion
  p->ElementDeletionOffset = p->StateSize/p->Fam.GetWordSize();

  int mdlopt;
  int intpts2;
  mdlopt = p->Dict["MAXINT"];
  if ( mdlopt >= 0 && mdlopt <= 10000)
    {
    intpts2 = mdlopt;
    mdlopt = LS_MDLOPT_NONE;
    }
  else if ( mdlopt < -10000 )
    {
    intpts2 = -mdlopt -10000;
    mdlopt = LS_MDLOPT_CELL;

    // WARNING: This needs NumberOfCells[LSDynaMetaData::RIGID_BODY] set, which relies on NUMRBE
    p->StateSize += this->GetNumberOfContinuumCells() * p->Fam.GetWordSize();
    }
  else if ( mdlopt > 10000)
    {
    intpts2 = mdlopt -10000;
    mdlopt = LS_MDLOPT_CELL;

    // WARNING: This needs NumberOfCells[LSDynaMetaData::RIGID_BODY] set, which relies on NUMRBE
    p->StateSize += this->GetNumberOfContinuumCells() * p->Fam.GetWordSize();
    }
  else
    {
    intpts2 = -mdlopt;
    mdlopt = LS_MDLOPT_POINT;
    //p->AddPointArray( LS_ARRAYNAME_DEATH, 1, 1 );
    p->StateSize += this->GetNumberOfNodes() * p->Fam.GetWordSize();
    }
  p->Dict["MDLOPT"] = mdlopt;
  p->Dict["_MAXINT_"] = intpts2;
  if ( p->Dict["NV2D"] > 0 )
    {
    if ( p->Dict["NV2D"]-(p->Dict["_MAXINT_"]*(6*p->Dict["IOSHL(1)"]+p->Dict["IOSHL(2)"]+p->Dict["NEIPS"])+8*p->Dict["IOSHL(3)"]+4*p->Dict["IOSHL(4)"]) > 1 )
      {
      p->Dict["ISTRN"] = 1;
      }
    else
      {
      p->Dict["ISTRN"] = 0;
      }
    }
  else if ( p->Dict["NELT"] > 0 )
    {
    if ( p->Dict["NV3D"] - p->Dict["_MAXINT_"]*(6*p->Dict["IOSHL(1)"]+p->Dict["IOSHL(2)"]+p->Dict["NEIPS"]) > 1 )
      {
      p->Dict["ISTRN"] = 1;
      }
    else
      {
      p->Dict["ISTRN"] = 0;
      }
    }
  else
    {
    p->Dict["ISTRN"] = 0;
    }

// OK, we are done processing the header (control) section.


  p->SPHStateOffset = p->StateSize/p->Fam.GetWordSize();
  // ============================================ Fluid Material ID Data Section
  // IALEMAT offset
  p->Fam.MarkSectionStart( curAdapt, LSDynaFamily::FluidMaterialIdData );
  p->PreStateSize += p->Dict["IALEMAT"];
  p->Fam.BufferChunk( LSDynaFamily::Int, p->Dict["IALEMAT"] );
  for ( itmp = 0; itmp < p->Dict["IALEMAT"]; ++itmp )
    {
    p->FluidMaterials.insert( p->Fam.GetNextWordAsInt() );
    }
  //p->Fam.SkipToWord( LSDynaFamily::FluidMaterialIdData, curAdapt, p->Dict["IALEMAT"] );

  // =======================  Smooth Particle Hydrodynamics Element Data Section
  // Only when NMSPH > 0
  p->Fam.MarkSectionStart( curAdapt, LSDynaFamily::SPHElementData );
  if ( p->NumberOfCells[LSDynaMetaData::PARTICLE] > 0 )
    {
    p->Fam.BufferChunk( LSDynaFamily::Int, 1 );
    vtkIdType sphAttributes = p->Fam.GetNextWordAsInt();
    p->Dict["isphfg(1)"] = sphAttributes;
    if ( sphAttributes >= 9 )
      {
      p->Fam.BufferChunk( LSDynaFamily::Int, sphAttributes - 1 ); // should be 9 or 10
      // Dyna docs call statePerParticle "NUM_SPH_DATA":
      int statePerParticle = 1; // start at 1 because we always have material ID of particle.
      for ( itmp = 2; itmp <= sphAttributes; ++itmp )
        {
        int numComponents = p->Fam.GetNextWordAsInt();
        sprintf( ctmp, "isphfg(%d)", itmp );
        p->Dict[ ctmp ] = numComponents;
        statePerParticle += numComponents;
        }
      p->Dict["NUM_SPH_DATA"] = statePerParticle;
      p->StateSize += statePerParticle * p->NumberOfCells[LSDynaMetaData::PARTICLE] * p->Fam.GetWordSize();
      }
    else
      {
      p->FileIsValid = 0;
      return 0;
      }
    p->Fam.SkipToWord( LSDynaFamily::SPHElementData, curAdapt, p->Dict["isphfg(1)"] );
    p->PreStateSize += p->Dict["isphfg(1)"]*p->Fam.GetWordSize();
    }

  // ===================================================== Geometry Data Section
  p->Fam.MarkSectionStart( curAdapt, LSDynaFamily::GeometryData );
  iddtmp = this->GetNumberOfNodes()*p->Dimensionality*p->Fam.GetWordSize(); // Size of nodes on disk
  iddtmp += p->NumberOfCells[LSDynaMetaData::SOLID]*9*p->Fam.GetWordSize(); // Size of hexes on disk
  iddtmp += p->NumberOfCells[LSDynaMetaData::THICK_SHELL]*9*p->Fam.GetWordSize(); // Size of thick shells on disk
  iddtmp += p->NumberOfCells[LSDynaMetaData::SHELL]*5*p->Fam.GetWordSize(); // Size of quads on disk
  iddtmp += p->NumberOfCells[LSDynaMetaData::BEAM]*6*p->Fam.GetWordSize(); // Size of beams on disk
  p->PreStateSize += iddtmp;
  p->Fam.SkipToWord( LSDynaFamily::GeometryData, curAdapt, iddtmp/p->Fam.GetWordSize() ); // Skip to end of geometry

  // =========== User Material, Node, And Element Identification Numbers Section
  p->Fam.MarkSectionStart( curAdapt, LSDynaFamily::UserIdData );
  if ( p->Dict["NARBS"] != 0 )
    {
    p->Fam.BufferChunk( LSDynaFamily::Int, 10 );
    p->PreStateSize += 10*p->Fam.GetWordSize();
    p->Dict["NSORT"] = p->Fam.GetNextWordAsInt();
    p->Dict["NSRH"] = p->Fam.GetNextWordAsInt();
    p->Dict["NSRB"] = p->Fam.GetNextWordAsInt();
    p->Dict["NSRS"] = p->Fam.GetNextWordAsInt();
    p->Dict["NSRT"] = p->Fam.GetNextWordAsInt();
    p->Dict["NSORTD"] = p->Fam.GetNextWordAsInt();
    p->Dict["NSRHD"] = p->Fam.GetNextWordAsInt();
    p->Dict["NSRBD"] = p->Fam.GetNextWordAsInt();
    p->Dict["NSRSD"] = p->Fam.GetNextWordAsInt();
    p->Dict["NSRTD"] = p->Fam.GetNextWordAsInt();
    //iddtmp = (this->GetNumberOfNodes() + this->GetNumberOfContinuumCells())*p->Fam.GetWordSize();
    if ( p->Dict["NSORT"] < 0 )
      {
      p->Fam.BufferChunk( LSDynaFamily::Int, 6 );
      p->PreStateSize += 6*p->Fam.GetWordSize();
      p->Dict["NSRMA"] = p->Fam.GetNextWordAsInt();
      p->Dict["NSRMU"] = p->Fam.GetNextWordAsInt();
      p->Dict["NSRMP"] = p->Fam.GetNextWordAsInt();
      p->Dict["NSRTM"] = p->Fam.GetNextWordAsInt();
      p->Dict["NUMRBS"] = p->Fam.GetNextWordAsInt();
      p->Dict["NMMAT"] = p->Fam.GetNextWordAsInt();
      iddtmp += 3*p->Dict["NMMAT"]*p->Fam.GetWordSize();
      }
    // FIXME!!! Why is NARBS larger than 10+NUMNP+NEL8+NEL2+NEL4+NELT?
    // Obviously, NARBS is definitive, but what are the extra numbers at the end?
    //iddtmp += (p->Dict["NARBS"]*p->Fam.GetWordSize() - iddtmp - 10*p->Fam.GetWordSize() - (p->Dict["NSORT"]<0 ? 6*p->Fam.GetWordSize() : 0));
    //p->PreStateSize += iddtmp;
    p->PreStateSize += p->Dict["NARBS"] * p->Fam.GetWordSize();
    // should just skip forward iddtmp bytes here, but no easy way to do that with the fam
    p->Fam.SkipToWord( LSDynaFamily::UserIdData, curAdapt, p->Dict["NARBS"] );
    }
  else
    {
    p->Dict["NSORT"] = 0;
    }
  // Break from convention and read in actual array values (as opposed to just summary information)
  // about the material IDs. This is required because the reader must present part names after
  // RequestInformation is called and that cannot be done without a list of material IDs.
  this->ReadUserMaterialIds();

  // ======================================= Adapted Element Parent List Section
  p->Fam.MarkSectionStart( curAdapt, LSDynaFamily::AdaptedParentData );
  p->Fam.SkipToWord( LSDynaFamily::AdaptedParentData, curAdapt, 2*p->Dict["NADAPT"] );
  iddtmp = 2*p->Dict["NADAPT"]*p->Fam.GetWordSize();
  p->PreStateSize += iddtmp;

  // ============== Smooth Particle Hydrodynamics Node And Material List Section
  p->Fam.MarkSectionStart( curAdapt, LSDynaFamily::SPHNodeData );
  iddtmp = 2*p->NumberOfCells[LSDynaMetaData::PARTICLE]*p->Fam.GetWordSize();
  p->PreStateSize += iddtmp;
  p->Fam.SkipToWord( LSDynaFamily::SPHNodeData, curAdapt, 2*p->NumberOfCells[LSDynaMetaData::PARTICLE] );

  // =========================================== Rigid Road Surface Data Section
  p->Fam.MarkSectionStart( curAdapt, LSDynaFamily::RigidSurfaceData );
  if ( p->Dict["NDIM"] > 5 )
    {
    p->Fam.BufferChunk( LSDynaFamily::Int, 4 );
    p->PreStateSize += 4*p->Fam.GetWordSize();
    p->Dict["NNODE"]  = p->Fam.GetNextWordAsInt();
    p->Dict["NSEG"]   = p->Fam.GetNextWordAsInt();
    p->Dict["NSURF"]  = p->Fam.GetNextWordAsInt();
    p->Dict["MOTION"] = p->Fam.GetNextWordAsInt();
    iddtmp = 4*p->Dict["NNODE"]*p->Fam.GetWordSize();
    p->PreStateSize += iddtmp;
    p->Fam.SkipWords( 4*p->Dict["NNODE"] );
    p->NumberOfCells[LSDynaMetaData::ROAD_SURFACE] = p->Dict["NSEG"];

    for ( itmp = 0; itmp < p->Dict["NSURF"]; ++itmp )
      {
      p->Fam.BufferChunk( LSDynaFamily::Int, 2 );
      p->Fam.GetNextWordAsInt(); // Skip SURFID
      iddtmp = p->Fam.GetNextWordAsInt(); // Read SURFNSEG[SURFID]
      p->RigidSurfaceSegmentSizes.push_back( iddtmp );
      p->PreStateSize += (2 + 4*iddtmp)*p->Fam.GetWordSize();
      p->Fam.SkipWords( 4*iddtmp );
      }

    if ( p->Dict["NSEG"] > 0 )
      {
      p->AddCellArray( LSDynaMetaData::ROAD_SURFACE, LS_ARRAYNAME_SEGMENTID, 1, 1 );
      //FIXME: p->AddRoadPointArray( LSDynaMetaData::ROAD_SURFACE, LS_ARRAYNAME_USERID, 1, 1 );
      }

    if ( p->Dict["MOTION"] )
      {
      p->StateSize += 6*p->Dict["NSURF"]*p->Fam.GetWordSize();
      }
    }

  //if ( curAdapt == 0 )
    {
    p->Fam.MarkSectionStart( curAdapt, LSDynaFamily::EndOfStaticSection );
    p->Fam.MarkSectionStart( curAdapt, LSDynaFamily::TimeStepSection );
    }
  p->Fam.SetStateSize( p->StateSize / p->Fam.GetWordSize() );


  // ==========================================================================
  // Now that we've read the header, create a list of the cell arrays for
  // each output mesh.

  // Currently, the LS-Dyna reader only gives users a few knobs to control which cell variables are loaded.
  // It is a difficult problem since many attributes only occur on some element types, there are many
  // dyna flags that conditionally omit results, and some quantities are repeated over differing numbers
  // of points for different types of cells.
  // Given the complexity, we punt by defining some knobs for "types" of data and define related fields.
  // In a perfect world, finer-grained control would be available.
  //
  // As an example: if there are any
  // - 3-D cells, OR
  // - non-rigid 2-D cells with IOSHL(1)==1, OR
  // - beam cells with NV1D > 6, OR
  // - SPH cells with isphfg(4)==6
  // then there will be stresses present

  // Every cell always has a material type
  // FIXME: Is this true? Rigid bodies may be an exception, in which
  // case we need to check that the number of cells in the other 5 meshes sum to >0

  if ( p->Dict["NARBS"] )
    {
    p->AddPointArray( LS_ARRAYNAME_USERID, 1, 1 );
    }

  if ( p->NumberOfCells[ LSDynaMetaData::PARTICLE ] )
    {
    //p->AddCellArray( LSDynaMetaData::PARTICLE, LS_ARRAYNAME_MATERIAL, 1, 1 );
    //p->AddCellArray( LSDynaMetaData::PARTICLE, LS_ARRAYNAME_DEATH, 1, 1 );
    if ( p->Dict["isphfg(2)"] == 1 )
      {
      p->AddCellArray( LSDynaMetaData::PARTICLE, LS_ARRAYNAME_RADIUSOFINFLUENCE, 1, 1 );
      }
    if ( p->Dict["isphfg(3)"] == 1 )
      {
      p->AddCellArray( LSDynaMetaData::PARTICLE, LS_ARRAYNAME_PRESSURE, 1, 1 );
      }
    if ( p->Dict["isphfg(4)"] == 6 )
      {
      p->AddCellArray( LSDynaMetaData::PARTICLE, LS_ARRAYNAME_STRESS, 6, 1 );
      }
    if ( p->Dict["isphfg(5)"] == 1 )
      {
      p->AddCellArray( LSDynaMetaData::PARTICLE, LS_ARRAYNAME_EPSTRAIN, 1, 1 );
      }
    if ( p->Dict["isphfg(6)"] == 1 )
      {
      p->AddCellArray( LSDynaMetaData::PARTICLE, LS_ARRAYNAME_DENSITY, 1, 1 );
      }
    if ( p->Dict["isphfg(7)"] == 1 )
      {
      p->AddCellArray( LSDynaMetaData::PARTICLE, LS_ARRAYNAME_INTERNALENERGY, 1, 1 );
      }
    if ( p->Dict["isphfg(8)"] == 1 )
      {
      p->AddCellArray( LSDynaMetaData::PARTICLE, LS_ARRAYNAME_NUMNEIGHBORS, 1, 1 );
      }
    if ( p->Dict["isphfg(9)"] == 6 )
      {
      p->AddCellArray( LSDynaMetaData::PARTICLE, LS_ARRAYNAME_STRAIN, 6, 1 );
      }
    if ( p->Dict["isphfg(10)"] == 1 )
      {
      p->AddCellArray( LSDynaMetaData::PARTICLE, LS_ARRAYNAME_MASS, 1, 1 );
      }
    }

  if ( p->NumberOfCells[ LSDynaMetaData::BEAM ] )
    {
//    p->AddCellArray( LSDynaMetaData::BEAM, LS_ARRAYNAME_MATERIAL, 1, 1 );
//    if ( p->Dict["MDLOPT"] == LS_MDLOPT_CELL )
//      {
//      p->AddCellArray( LSDynaMetaData::BEAM, LS_ARRAYNAME_DEATH, 1, 1 );
//      }

    if ( p->Dict["NARBS"] != 0 )
      {
      p->AddCellArray( LSDynaMetaData::BEAM, LS_ARRAYNAME_USERID, 1, 1 );
      }
    if ( p->Dict["NV1D"] >= 6 )
      {
      p->AddCellArray( LSDynaMetaData::BEAM, LS_ARRAYNAME_AXIALFORCE, 1, 1 );
      p->AddCellArray( LSDynaMetaData::BEAM, LS_ARRAYNAME_SHEARRESULTANT, 2, 1 );
      p->AddCellArray( LSDynaMetaData::BEAM, LS_ARRAYNAME_BENDINGRESULTANT, 2, 1 );
      p->AddCellArray( LSDynaMetaData::BEAM, LS_ARRAYNAME_TORSIONRESULTANT, 1, 1 );
      }
    if ( p->Dict["NV1D"] > 6 )
      {
      p->AddCellArray( LSDynaMetaData::BEAM, LS_ARRAYNAME_SHEARSTRESS, 2, 1 );
      p->AddCellArray( LSDynaMetaData::BEAM, LS_ARRAYNAME_AXIALSTRESS, 1, 1 );
      p->AddCellArray( LSDynaMetaData::BEAM, LS_ARRAYNAME_AXIALSTRAIN, 1, 1 );
      p->AddCellArray( LSDynaMetaData::BEAM, LS_ARRAYNAME_PLASTICSTRAIN, 1, 1 );
      }
    }

  if ( p->NumberOfCells[ LSDynaMetaData::SHELL ] )
    {
//    p->AddCellArray( LSDynaMetaData::SHELL, LS_ARRAYNAME_MATERIAL, 1, 1 );
//    if ( p->Dict["MDLOPT"] == LS_MDLOPT_CELL )
//      {
//      p->AddCellArray( LSDynaMetaData::SHELL, LS_ARRAYNAME_DEATH, 1, 1 );
//      }
    if ( p->Dict["NARBS"] != 0 )
      {
      p->AddCellArray( LSDynaMetaData::SHELL, LS_ARRAYNAME_USERID, 1, 1 );
      }
    if ( p->Dict["IOSHL(1)"] )
      {
      if ( p->Dict["_MAXINT_"] >= 3 )
        {
        p->AddCellArray( LSDynaMetaData::SHELL, LS_ARRAYNAME_STRESS, 6, 1 );
        p->AddCellArray( LSDynaMetaData::SHELL, LS_ARRAYNAME_STRESS "InnerSurf", 6, 1 );
        p->AddCellArray( LSDynaMetaData::SHELL, LS_ARRAYNAME_STRESS "OuterSurf", 6, 1 );
        }
      for ( itmp = 3; itmp < p->Dict["_MAXINT_"]; ++itmp )
        {
        sprintf( ctmp, "%sIntPt%d", LS_ARRAYNAME_STRESS, itmp + 1 );
        p->AddCellArray( LSDynaMetaData::SHELL, ctmp, 6, 1 );
        }
      }
    if ( p->Dict["IOSHL(2)"] )
      {
      if ( p->Dict["_MAXINT_"] >= 3 )
        {
        p->AddCellArray( LSDynaMetaData::SHELL, LS_ARRAYNAME_EPSTRAIN, 1, 1 );
        p->AddCellArray( LSDynaMetaData::SHELL, LS_ARRAYNAME_EPSTRAIN "InnerSurf", 1, 1 );
        p->AddCellArray( LSDynaMetaData::SHELL, LS_ARRAYNAME_EPSTRAIN "OuterSurf", 1, 1 );
        }
      for ( itmp = 3; itmp < p->Dict["_MAXINT_"]; ++itmp )
        {
        sprintf( ctmp, "%sIntPt%d", LS_ARRAYNAME_EPSTRAIN, itmp + 1 );
        p->AddCellArray( LSDynaMetaData::SHELL, ctmp, 1, 1 );
        }
      }
    if ( p->Dict["IOSHL(3)"] )
      {
      p->AddCellArray( LSDynaMetaData::SHELL, LS_ARRAYNAME_NORMALRESULTANT, 3, 1 );
      p->AddCellArray( LSDynaMetaData::SHELL, LS_ARRAYNAME_SHEARRESULTANT, 2, 1 );
      p->AddCellArray( LSDynaMetaData::SHELL, LS_ARRAYNAME_BENDINGRESULTANT, 3, 1 );
      }
    if ( p->Dict["IOSHL(4)"] )
      {
      p->AddCellArray( LSDynaMetaData::SHELL, LS_ARRAYNAME_THICKNESS, 1, 1 );
      p->AddCellArray( LSDynaMetaData::SHELL, LS_ARRAYNAME_ELEMENTMISC, 2, 1 );
      }
    if ( p->Dict["NEIPS"] )
      {
      int neips = p->Dict["NEIPS"];
      if ( p->Dict["_MAXINT_"] >= 3 )
        {
        p->AddCellArray( LSDynaMetaData::SHELL, LS_ARRAYNAME_INTEGRATIONPOINT, neips, 1 );
        p->AddCellArray( LSDynaMetaData::SHELL, LS_ARRAYNAME_INTEGRATIONPOINT, neips, 1 );
        p->AddCellArray( LSDynaMetaData::SHELL, LS_ARRAYNAME_INTEGRATIONPOINT "InnerSurf", neips, 1 );
        p->AddCellArray( LSDynaMetaData::SHELL, LS_ARRAYNAME_INTEGRATIONPOINT "OuterSurf", neips, 1 );
        }
      for ( itmp = 3; itmp < p->Dict["_MAXINT_"]; ++itmp )
        {
        sprintf( ctmp, "%sIntPt%d", LS_ARRAYNAME_INTEGRATIONPOINT, itmp + 1 );
        p->AddCellArray( LSDynaMetaData::SHELL, ctmp, 6, 1 );
        }
      }
    if ( p->Dict["ISTRN"] )
      {
        p->AddCellArray( LSDynaMetaData::SHELL, LS_ARRAYNAME_STRAIN "InnerSurf", 6, 1 );
        p->AddCellArray( LSDynaMetaData::SHELL, LS_ARRAYNAME_STRAIN "OuterSurf", 6, 1 );
      }
    if ( ! p->Dict["ISTRN"] || (p->Dict["ISTRN"] && p->Dict["NV2D"] >= 45) )
      {
        p->AddCellArray( LSDynaMetaData::SHELL, LS_ARRAYNAME_INTERNALENERGY, 1, 1 );
      }
    }

  if ( p->NumberOfCells[ LSDynaMetaData::THICK_SHELL ] )
    {
//    p->AddCellArray( LSDynaMetaData::THICK_SHELL, LS_ARRAYNAME_MATERIAL, 1, 1 );
//    if ( p->Dict["MDLOPT"] == LS_MDLOPT_CELL )
//      {
//      p->AddCellArray( LSDynaMetaData::THICK_SHELL, LS_ARRAYNAME_DEATH, 1, 1 );
//      }
    if ( p->Dict["NARBS"] != 0 )
      {
      p->AddCellArray( LSDynaMetaData::THICK_SHELL, LS_ARRAYNAME_USERID, 1, 1 );
      }
    if ( p->Dict["IOSHL(1)"] )
      {
      if ( p->Dict["_MAXINT_"] >= 3 )
        {
        p->AddCellArray( LSDynaMetaData::THICK_SHELL, LS_ARRAYNAME_STRESS, 6, 1 );
        p->AddCellArray( LSDynaMetaData::THICK_SHELL, LS_ARRAYNAME_STRESS "InnerSurf", 6, 1 );
        p->AddCellArray( LSDynaMetaData::THICK_SHELL, LS_ARRAYNAME_STRESS "OuterSurf", 6, 1 );
        }
      for ( itmp = 3; itmp < p->Dict["_MAXINT_"]; ++itmp )
        {
        sprintf( ctmp, "%sIntPt%d", LS_ARRAYNAME_STRESS, itmp + 1 );
        p->AddCellArray( LSDynaMetaData::THICK_SHELL, ctmp, 6, 1 );
        }
      }
    if ( p->Dict["IOSHL(2)"] )
      {
      if ( p->Dict["_MAXINT_"] >= 3 )
        {
        p->AddCellArray( LSDynaMetaData::THICK_SHELL, LS_ARRAYNAME_EPSTRAIN, 1, 1 );
        p->AddCellArray( LSDynaMetaData::THICK_SHELL, LS_ARRAYNAME_EPSTRAIN "InnerSurf", 1, 1 );
        p->AddCellArray( LSDynaMetaData::THICK_SHELL, LS_ARRAYNAME_EPSTRAIN "OuterSurf", 1, 1 );
        }
      for ( itmp = 3; itmp < p->Dict["_MAXINT_"]; ++itmp )
        {
        sprintf( ctmp, "%sIntPt%d", LS_ARRAYNAME_EPSTRAIN, itmp + 1 );
        p->AddCellArray( LSDynaMetaData::THICK_SHELL, ctmp, 1, 1 );
        }
      }
    if ( p->Dict["NEIPS"] )
      {
      int neips = p->Dict["NEIPS"];
      if ( p->Dict["_MAXINT_"] >= 3 )
        {
        p->AddCellArray( LSDynaMetaData::THICK_SHELL, LS_ARRAYNAME_INTEGRATIONPOINT, neips, 1 );
        p->AddCellArray( LSDynaMetaData::THICK_SHELL, LS_ARRAYNAME_INTEGRATIONPOINT, neips, 1 );
        p->AddCellArray( LSDynaMetaData::THICK_SHELL, LS_ARRAYNAME_INTEGRATIONPOINT "InnerSurf", neips, 1 );
        p->AddCellArray( LSDynaMetaData::THICK_SHELL, LS_ARRAYNAME_INTEGRATIONPOINT "OuterSurf", neips, 1 );
        }
      for ( itmp = 3; itmp < p->Dict["_MAXINT_"]; ++itmp )
        {
        sprintf( ctmp, "%sIntPt%d", LS_ARRAYNAME_INTEGRATIONPOINT, itmp + 1 );
        p->AddCellArray( LSDynaMetaData::THICK_SHELL, ctmp, 6, 1 );
        }
      }
    if ( p->Dict["ISTRN"] )
      {
        p->AddCellArray( LSDynaMetaData::THICK_SHELL, LS_ARRAYNAME_STRAIN "InnerSurf", 6, 1 );
        p->AddCellArray( LSDynaMetaData::THICK_SHELL, LS_ARRAYNAME_STRAIN "OuterSurf", 6, 1 );
      }
    }

  if ( p->NumberOfCells[ LSDynaMetaData::SOLID ] )
    {
//    p->AddCellArray( LSDynaMetaData::SOLID, LS_ARRAYNAME_MATERIAL, 1, 1 );
//    if ( p->Dict["MDLOPT"] == LS_MDLOPT_CELL )
//      {
//      p->AddCellArray( LSDynaMetaData::SOLID, LS_ARRAYNAME_DEATH, 1, 1 );
//      }
    if ( p->Dict["NARBS"] != 0 )
      {
      p->AddCellArray( LSDynaMetaData::SOLID, LS_ARRAYNAME_USERID, 1, 1 );
      }
    p->AddCellArray( LSDynaMetaData::SOLID, LS_ARRAYNAME_STRESS, 6, 1 );
    p->AddCellArray( LSDynaMetaData::SOLID, LS_ARRAYNAME_EPSTRAIN, 1, 1 );
    if ( p->Dict["ISTRN"] )
      {
      p->AddCellArray( LSDynaMetaData::SOLID, LS_ARRAYNAME_STRAIN, 6, 1 );
      }
    if ( p->Dict["NEIPH"] > 0 )
      {
      p->AddCellArray( LSDynaMetaData::SOLID, LS_ARRAYNAME_INTEGRATIONPOINT, p->Dict["NEIPH"], 1 );
      }
    }

  // Only try reading the keyword file if we don't have part names.
  if ( curAdapt == 0 && p->PartNames.size() == 0 )
    {
    this->ResetPartInfo();

    int result = this->ReadInputDeck();

    if (result == 0)
      {
      //we failed to read the input deck so we are going to read the first binary file for part names
      this->ReadPartTitlesFromRootFile();
      }
    }

  return -1;
}

int vtkLSDynaReader::ScanDatabaseTimeSteps()
{
  LSDynaMetaData* p = this->P;

  // ======================================================= State Data Sections
  // The 2 lines below are now in ReadHeaderInformation:
  // p->Fam.MarkSectionStart( curAdapt, LSDynaFamily::TimeStepSection );
  // p->Fam.SetStateSize( p->StateSize / p->Fam.GetWordSize() );
  // It may be useful to call
  // p->JumpToMark( LSDynaFamily::TimeStepSection );
  // here.
  if ( p->Fam.GetStateSize() <= 0 )
    {
    vtkErrorMacro( "Database has bad state size (" << p->Fam.GetStateSize() << ")." );
    return 1;
    }

  // Discover the number of states and record the time value for each.
  int ntimesteps = 0;
  double time;
  int itmp = 1;
  int lastAdapt = 0;
  do {
    if ( p->Fam.BufferChunk( LSDynaFamily::Float, 1 ) == 0 )
      {
      time = p->Fam.GetNextWordAsFloat();
      if ( time != LSDynaFamily::EOFMarker )
        {
        p->Fam.MarkTimeStep();
        p->TimeValues.push_back( time );
        //fprintf( stderr, "%d %f\n", (int) p->TimeValues.size() - 1, time ); fflush(stderr);
        if ( p->Fam.SkipToWord( LSDynaFamily::TimeStepSection, ntimesteps++, p->Fam.GetStateSize() ) )
          {
          itmp = 0;
          }
        }
      else
        {
        if ( p->Fam.AdvanceFile() )
          {
          itmp = 0;
          }
        else
          {
          if ( ntimesteps == 0 )
            {
            // First time step was an EOF marker... move the marker to the
            // beginning of the first real time step...
            p->Fam.MarkSectionStart( lastAdapt, LSDynaFamily::TimeStepSection );
            }
          }
        int nextAdapt = p->Fam.GetCurrentAdaptLevel();
        if ( nextAdapt != lastAdapt )
          {
          // Read the next static header section... state size has changed.
          p->Fam.MarkSectionStart( nextAdapt, LSDynaFamily::ControlSection );
          this->ReadHeaderInformation( nextAdapt );
          //p->Fam.SkipToWord( LSDynaFamily::EndOfStaticSection, nextAdapt, 0 );
          lastAdapt = nextAdapt;
          }
        }
      }
    else
      {
      itmp = 0;
      }
  } while (itmp);

  this->TimeStepRange[0] = 0;
  this->TimeStepRange[1] = ntimesteps ? ntimesteps - 1 : 0;

  return -1;
}

// =================================== Provide information about the database to the pipeline
int vtkLSDynaReader::RequestInformation( vtkInformation* vtkNotUsed(request),
                                         vtkInformationVector** vtkNotUsed(iinfo),
                                         vtkInformationVector* oinfo )
{
  LSDynaMetaData* p = this->P;
  // If the time step is set before RequestInformation is called, we must
  // read the header information immediately in order to determine whether
  // the timestep that's been passed is valid. If it's not, we ignore it.
  if ( ! p->FileIsValid )
    {
    if ( p->Fam.GetDatabaseDirectory().empty() )
      {
      // fail silently for CanReadFile()'s sake.
      //vtkErrorMacro( "You haven't set the LS-Dyna database directory!" );
      return 1;
      }

    if ( p->Fam.GetDatabaseBaseName().empty() )
      {
      p->Fam.SetDatabaseBaseName( "/d3plot" ); // not a bad assumption.
      }
    p->Fam.ScanDatabaseDirectory();
    if ( p->Fam.GetNumberOfFiles() < 1 )
      {
      p->FileIsValid = 0;
      return 1;
      }
    p->Fam.DetermineStorageModel();
    p->MaxFileLength = p->FileSizeFactor*512*512*p->Fam.GetWordSize();
    p->FileIsValid = 1;

    // OK, now we have a list of files. Next, determine the length of the
    // state vector (#bytes of data stored per time step):
    this->ReadHeaderInformation( 0 );

    // Finally, we can loop through and determine where all the state
    // vectors start for each time step.
    // This will call ReadHeaderInformation when it encounters any
    // mesh adaptations (so that it can get the new state vector size).
    this->ScanDatabaseTimeSteps();
    }

  if ( p->TimeValues.size() == 0 )
    {
    vtkErrorMacro( "No valid time steps in the LS-Dyna database" );
    return 0;
    }

  // Clamp timestep to be valid here.
  if ( p->CurrentState < 0 )
    {
    p->CurrentState = 0;
    }
  else if ( p->CurrentState >= (int) p->TimeValues.size() )
    {
    p->CurrentState = p->TimeValues.size() - 1;
    }

  int newAdaptLevel = p->Fam.TimeAdaptLevel( p->CurrentState );
  if ( p->Fam.GetCurrentAdaptLevel() !=  newAdaptLevel )
    {
    // Requested time step has a different mesh adaptation than current one.
    // Update the header information so that queries like GetNumberOfCells() work properly.
    int result;
    result = this->ReadHeaderInformation( newAdaptLevel );
    if ( result >= 0 )
      {
      this->ResetPartsCache();
      return result;
      }
    }

  // Every output object has all the time steps.
  vtkInformation* outInfo = oinfo->GetInformationObject(0);
  outInfo->Set( vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &p->TimeValues[0], (int)p->TimeValues.size() );
  double timeRange[2];
  timeRange[0] = p->TimeValues[0];
  timeRange[1] = p->TimeValues[p->TimeValues.size() - 1];
  outInfo->Set( vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2 );

  return 1;
}

//----------------------------------------------------------------------------
int vtkLSDynaReader::ReadTopology()
{
  bool readTopology=false;
  if(!this->Parts)
    {
    readTopology=true;
    this->Parts = vtkLSDynaPartCollection::New();
    this->Parts->InitCollection(this->P,NULL,NULL);
    }
  if(!readTopology)
    {
    return 0;
    }

  if( this->ReadPartSizes())
    {
    vtkErrorMacro( "Could not read cell sizes." );
    return 1;
    }

  if ( this->ReadConnectivityAndMaterial() )
    {
    vtkErrorMacro( "Could not read connectivity." );
    return 1;
    }

  this->Parts->FinalizeTopology();

  if(this->ReadNodes())
    {
    vtkErrorMacro("Could not read static node values.");
    return 1;
    }


  // we need to read the user ids after we have read the topology
  // so we know how many cells are in each part
  if ( this->ReadUserIds() )
    {
    vtkErrorMacro( "Could not read user node/element IDs." );
    return 1;
    }

  return 0;
}

// ============================================================= Section parsing
int vtkLSDynaReader::ReadNodes()
{
  LSDynaMetaData* p = this->P;

  // Skip reading coordinates if we are deflecting the mesh... they would be replaced anyway.
  // The only exception is if the deflected coordinates are not included in the LS-Dyna output
  // (i.e., when IU is 0).
  // Note that in any event we still have to read the rigid road coordinates.
  // If the mesh is deformed each state will have the points so see ReadState
  if ( ! this->DeformedMesh || ! p->Dict["IU"] )
    {
    p->Fam.SkipToWord( LSDynaFamily::GeometryData, p->Fam.GetCurrentAdaptLevel(), 0 );
    this->Parts->ReadPointProperty(p->NumberOfNodes,p->Dimensionality,NULL,false,true,false);
    }

  if ( p->ReadRigidRoadMvmt )
    {
    vtkIdType nnode = p->Dict["NNODE"];
    p->Fam.SkipToWord( LSDynaFamily::RigidSurfaceData, p->Fam.GetCurrentAdaptLevel(), 4 + nnode );
    this->Parts->ReadPointProperty(nnode,3,NULL,false,false,true);
    }

  return 0;
}

//-----------------------------------------------------------------------------
int vtkLSDynaReader::ReadUserIds()
{
  // Below here is code that runs when user node or element numbers are present
  int arbitraryMaterials = this->P->Dict["NSORT"] < 0 ? 1 : 0;
  vtkIdType isz = this->GetNumberOfNodes();
  if ( arbitraryMaterials )
    {
    this->P->Fam.SkipToWord( LSDynaFamily::UserIdData,
                             this->P->Fam.GetCurrentAdaptLevel(), 16 );
    }
  else
    {
    this->P->Fam.SkipToWord( LSDynaFamily::UserIdData,
                             this->P->Fam.GetCurrentAdaptLevel(), 10 );
    }

  // Node numbers
  bool nodeIdStatus = this->GetPointArrayStatus( LS_ARRAYNAME_USERID ) == 1;
  if(nodeIdStatus)
    {
    this->Parts->ReadPointUserIds(isz,LS_ARRAYNAME_USERID);
    }

  // FIXME: This won't work if Rigid Body and Shell elements are interleaved (which I now believe they are)
  this->Parts->ReadCellUserIds(LSDynaMetaData::BEAM,
              this->GetCellArrayStatus(LSDynaMetaData::BEAM, LS_ARRAYNAME_USERID));
  this->Parts->ReadCellUserIds(LSDynaMetaData::SHELL,
              this->GetCellArrayStatus(LSDynaMetaData::SHELL, LS_ARRAYNAME_USERID));
  this->Parts->ReadCellUserIds(LSDynaMetaData::THICK_SHELL,
              this->GetCellArrayStatus(LSDynaMetaData::THICK_SHELL, LS_ARRAYNAME_USERID));
  this->Parts->ReadCellUserIds(LSDynaMetaData::SOLID,
              this->GetCellArrayStatus(LSDynaMetaData::SOLID, LS_ARRAYNAME_USERID));
  this->Parts->ReadCellUserIds(LSDynaMetaData::RIGID_BODY,
              this->GetCellArrayStatus(LSDynaMetaData::RIGID_BODY, LS_ARRAYNAME_USERID));
  return 0;
}

//-----------------------------------------------------------------------------
int vtkLSDynaReader::ReadDeletion()
{
  enum LSDynaMetaData::LSDYNA_TYPES validCellTypes[4] = {
        LSDynaMetaData::SOLID,
        LSDynaMetaData::THICK_SHELL,
        LSDynaMetaData::SHELL,
        LSDynaMetaData::BEAM};

  if(!this->RemoveDeletedCells)
    {
    //this functions doesn't have to lead the reader to a certain
    //position in the files
    this->Parts->DisbleDeadCells();
    return 0;
    }

  LSDynaMetaData* p = this->P;
  vtkUnsignedCharArray* death;
  switch ( p->Dict["MDLOPT"] )
    {
  case LS_MDLOPT_POINT:
    vtkErrorMacro("We currently only support cell death");
    break;
  case LS_MDLOPT_CELL:
    for(int i=0; i < 4; ++i)
      {
      const LSDynaMetaData::LSDYNA_TYPES type = validCellTypes[i];
      vtkIdType numCells,numSkipStart,numSkipEnd;
      this->Parts->GetPartReadInfo(type,numCells,numSkipStart,numSkipEnd);

      death = vtkUnsignedCharArray::New();
      death->SetName( LS_ARRAYNAME_DEATH );
      death->SetNumberOfComponents( 1 );
      death->SetNumberOfTuples(numCells);

      p->Fam.SkipWords(numSkipStart);
      this->ReadDeletionArray(death,0,1);
      p->Fam.SkipWords(numSkipEnd);
      this->Parts->SetCellDeadFlags(type,death,this->DeletedCellsAsGhostArray);
      death->Delete();
      }

    //we are now at the position to read the SPH deletion info from the sph state info
    if(p->NumberOfCells[LSDynaMetaData::PARTICLE]>0)
      {
      const LSDynaMetaData::LSDYNA_TYPES type = LSDynaMetaData::PARTICLE;
      vtkIdType numCells,numSkipStart,numSkipEnd;
      this->Parts->GetPartReadInfo(type,numCells,numSkipStart,numSkipEnd);

      death = vtkUnsignedCharArray::New();
      death->SetName( LS_ARRAYNAME_DEATH );
      death->SetNumberOfComponents( 1 );
      death->SetNumberOfTuples(numCells);

      p->Fam.SkipWords(numSkipStart);
      //we are really reading the material id as the death flag
      //and each particle has twenty words of info, so we have to skip 19
      //since luckily material id is first
      this->ReadDeletionArray(death,0,20);
      p->Fam.SkipWords(numSkipEnd);
      this->Parts->SetCellDeadFlags(type,death,this->DeletedCellsAsGhostArray);
      death->Delete();
      }
    break;
  case LS_MDLOPT_NONE:
  default:
    // do nothing.
    break;
    }
  return 0;
}

//-----------------------------------------------------------------------------
void vtkLSDynaReader::ReadDeletionArray(vtkUnsignedCharArray* arr, const int& pos, const int& size)
{
  //setup to do a block read, way faster than converting each
  //float/double individually
  LSDynaMetaData *p = this->P;
  vtkIdType startId = 0;
  vtkIdType numChunks = p->Fam.InitPartialChunkBuffering(arr->GetNumberOfTuples(),size);
  if(p->Fam.GetWordSize() == 8)
    {
    for(vtkIdType i=0;i<numChunks;++i)
      {
      vtkIdType chunkSize = p->Fam.GetNextChunk(LSDynaFamily::Float );
      vtkIdType numCellsInChunk = chunkSize/size;
      double *dbuf = p->Fam.GetBufferAs<double>();
      this->FillDeletionArray(dbuf,arr,startId,numCellsInChunk,pos,size);
      startId+=numCellsInChunk;
      }
    }
  else
    {
    for(vtkIdType i=0;i<numChunks;++i)
      {
      vtkIdType chunkSize = p->Fam.GetNextChunk(LSDynaFamily::Float );
      vtkIdType numCellsInChunk = chunkSize/size;
      float *fbuf = p->Fam.GetBufferAs<float>();
      this->FillDeletionArray(fbuf,arr,startId,numCellsInChunk,pos,size);
      startId+=numCellsInChunk;
      }
    }
}

//-----------------------------------------------------------------------------
int vtkLSDynaReader::ReadState( vtkIdType step )
{
  //remember C style return so zero is pass
  if(this->ReadNodeStateInfo(step))
    {
    vtkErrorMacro( "Problem reading state point information." );
    return 1;
    }
  if(this->ReadCellStateInfo( step ))
    {
    vtkErrorMacro( "Problem reading state cell information." );
    return 1;
    }
  if(this->ReadDeletion())
    {
    vtkErrorMacro( "Problem reading state deletion information." );
    return 1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
int vtkLSDynaReader::ReadNodeStateInfo( vtkIdType step )
{
  LSDynaMetaData* p = this->P;

  // Skip global variables for now
  p->Fam.SkipToWord( LSDynaFamily::TimeStepSection, step, 1 + p->Dict["NGLBV"] );

  // Read nodal data ===========================================================
  std::vector<std::string> names;
  std::vector<int> cmps;
  // Important: push_back in the order these are interleaved on disk
  // Note that temperature and deflection are swapped relative to the order they
  // are specified in the header section.
  const char * aNames[] = {
    LS_ARRAYNAME_DEFLECTION,
    LS_ARRAYNAME_TEMPERATURE,
    LS_ARRAYNAME_VELOCITY,
    LS_ARRAYNAME_ACCELERATION,
    LS_ARRAYNAME_PRESSURE,
    LS_ARRAYNAME_VORTICITY "_X",
    LS_ARRAYNAME_VORTICITY "_Y",
    LS_ARRAYNAME_VORTICITY "_Z",
    LS_ARRAYNAME_RESULTANTVORTICITY,
    LS_ARRAYNAME_ENSTROPHY,
    LS_ARRAYNAME_HELICITY,
    LS_ARRAYNAME_STREAMFUNCTION,
    LS_ARRAYNAME_ENTHALPY,
    LS_ARRAYNAME_DENSITY,
    LS_ARRAYNAME_TURBULENTKE,
    LS_ARRAYNAME_DISSIPATION,
    LS_ARRAYNAME_EDDYVISCOSITY,
    LS_ARRAYNAME_SPECIES_01,
    LS_ARRAYNAME_SPECIES_02,
    LS_ARRAYNAME_SPECIES_03,
    LS_ARRAYNAME_SPECIES_04,
    LS_ARRAYNAME_SPECIES_05,
    LS_ARRAYNAME_SPECIES_06,
    LS_ARRAYNAME_SPECIES_07,
    LS_ARRAYNAME_SPECIES_08,
    LS_ARRAYNAME_SPECIES_09,
    LS_ARRAYNAME_SPECIES_10
  };

  const char* aDictNames[] = {
    "IU",
    "IT",
    "IV",
    "IA",
    "cfdPressure",
    "cfdXVort",
    "cfdYVort",
    "cfdZVort",
    "cfdRVort",
    "cfdEnstrophy",
    "cfdHelicity",
    "cfdStream",
    "cfdEnthalpy",
    "cfdDensity",
    "cfdTurbKE",
    "cfdDiss",
    "cfdEddyVisc",
    "cfdSpec01",
    "cfdSpec02",
    "cfdSpec03",
    "cfdSpec04",
    "cfdSpec05",
    "cfdSpec06",
    "cfdSpec07",
    "cfdSpec08",
    "cfdSpec09",
    "cfdSpec10"
  };
  int aComponents[] = {
    -1, 1, -1, -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
  };
  int vppt = 0; // values per point
  int allVortPresent = p->Dict["cfdXVort"] && p->Dict["cfdYVort"] && p->Dict["cfdZVort"];

  for ( int nvnum = 0; nvnum < (int) (sizeof(aComponents)/sizeof(aComponents[0])); ++nvnum )
    {
    if ( p->Dict[ aDictNames[nvnum] ] )
      {
      if ( allVortPresent && ! strncmp( LS_ARRAYNAME_VORTICITY, aNames[ nvnum ], sizeof(LS_ARRAYNAME_VORTICITY) ) )
        {
        // turn the vorticity components from individual scalars into one vector (with a hack)
        if ( nvnum < 7 )
          continue;
        aComponents[ nvnum ] = 3;
        aNames[ nvnum ] = LS_ARRAYNAME_VORTICITY;
        }
      names.push_back(aNames[ nvnum ]);
      cmps.push_back( aComponents[ nvnum ] == -1 ? p->Dimensionality : aComponents[ nvnum ] );
      vppt += cmps.back();
      }
    }

  if ( vppt != 0 )
    {
    for(size_t i=0; i < cmps.size(); i++)
      {
      //special case if the user has said they want a deformed mesh
      //we have to read in the deflection array
      bool valid = this->GetPointArrayStatus( names[i].c_str() ) != 0;
      bool isDeflectionArray = this->DeformedMesh &&
                               strcmp(names[i].c_str(), LS_ARRAYNAME_DEFLECTION)==0;
      this->Parts->ReadPointProperty(p->NumberOfNodes,cmps[i],names[i].c_str(),
                                     valid,isDeflectionArray);
      }
    //clear the buffer as it will be very large and not needed
    p->Fam.ClearBuffer();
    }
  return 0;
}

//-----------------------------------------------------------------------------
int vtkLSDynaReader::ReadCellStateInfo( vtkIdType vtkNotUsed(step) )
{

  LSDynaMetaData* p = this->P;
  int itmp;
  char ctmp[128];

#define VTK_LS_CELLARRAY(cond,celltype,arrayname,numComps)\
  if ( cond && this->GetCellArrayStatus( celltype, arrayname ) ) \
    { \
    this->Parts->AddProperty(celltype,arrayname,startPos,numComps); \
    } \
  startPos+=numComps;

  // Solid element data========================================================
  int startPos=0; //used to keep track of the startpos between calls to VTK_LS_CELLARRAY

  VTK_LS_CELLARRAY(1,LSDynaMetaData::SOLID,LS_ARRAYNAME_STRESS,6);
  VTK_LS_CELLARRAY(1,LSDynaMetaData::SOLID,LS_ARRAYNAME_EPSTRAIN,1);

  //From the documentation if ISTRN is 1 and we have 6 or more values in NEIPH
  //the last 6 are the strain
  //quote "If ISTRN=1, and NEIPH>=6, the last 6 additional values are the six
  //strain components"
  vtkIdType neiph = p->Dict["NEIPH"], istrn = p->Dict["ISTRN"];
  if(istrn == 1 && neiph >=6)
    {
    VTK_LS_CELLARRAY(neiph > 6,LSDynaMetaData::SOLID,LS_ARRAYNAME_INTEGRATIONPOINT,neiph-6);
    VTK_LS_CELLARRAY(p->Dict["ISTRN"] == 1,LSDynaMetaData::SOLID,LS_ARRAYNAME_STRAIN,6);
    }
  else
    {
    VTK_LS_CELLARRAY(p->Dict["NEIPH"] > 0,LSDynaMetaData::SOLID,LS_ARRAYNAME_INTEGRATIONPOINT,p->Dict["NEIPH"]);
    }

  this->ReadCellProperties(LSDynaMetaData::SOLID, p->Dict["NV3D"]);

  // Thick Shell element data==================================================
  startPos=0;
  // Mid-surface data
  VTK_LS_CELLARRAY(p->Dict["IOSHL(1)"] != 0,LSDynaMetaData::THICK_SHELL,LS_ARRAYNAME_STRESS,6);
  VTK_LS_CELLARRAY(p->Dict["IOSHL(2)"] != 0,LSDynaMetaData::THICK_SHELL,LS_ARRAYNAME_EPSTRAIN,1);
  VTK_LS_CELLARRAY(p->Dict["NEIPS"] > 0,LSDynaMetaData::THICK_SHELL,LS_ARRAYNAME_INTEGRATIONPOINT,p->Dict["NEIPS"]);

  // Inner surface data
  VTK_LS_CELLARRAY(p->Dict["IOSHL(1)"] != 0,LSDynaMetaData::THICK_SHELL,LS_ARRAYNAME_STRESS "InnerSurf",6);
  VTK_LS_CELLARRAY(p->Dict["IOSHL(2)"] != 0,LSDynaMetaData::THICK_SHELL,LS_ARRAYNAME_EPSTRAIN "InnerSurf",1);
  VTK_LS_CELLARRAY(p->Dict["NEIPS"] > 0,LSDynaMetaData::THICK_SHELL,LS_ARRAYNAME_INTEGRATIONPOINT "InnerSurf",p->Dict["NEIPS"]);

  // Outer surface data
  VTK_LS_CELLARRAY(p->Dict["IOSHL(1)"] != 0,LSDynaMetaData::THICK_SHELL,LS_ARRAYNAME_STRESS "OuterSurf",6);
  VTK_LS_CELLARRAY(p->Dict["IOSHL(2)"] != 0,LSDynaMetaData::THICK_SHELL,LS_ARRAYNAME_EPSTRAIN "OuterSurf",1);
  VTK_LS_CELLARRAY(p->Dict["NEIPS"] > 0,LSDynaMetaData::THICK_SHELL,LS_ARRAYNAME_INTEGRATIONPOINT "OuterSurf",p->Dict["NEIPS"]);

  if(p->Dict["NV3DT"] > 21)
    {
    //in some use case the ISTRN is incorrectly calculated because the d3plot
    //is unclear if the flag needs to be computed separately for
    //NV2D and NV3DT

    VTK_LS_CELLARRAY(p->Dict["ISTRN"],LSDynaMetaData::THICK_SHELL,LS_ARRAYNAME_STRAIN "InnerSurf",6);
    VTK_LS_CELLARRAY(p->Dict["ISTRN"],LSDynaMetaData::THICK_SHELL,LS_ARRAYNAME_STRAIN "OuterSurf",6);

    // If _MAXINT_ > 3, there will be additional fields. They are other
    // integration point values. There are (_MAXINT_ - 3) extra
    // integration points, each of which has a stress (6 vals),
    // an effective plastic strain (1 val), and extra integration
    // point values (NEIPS vals).
    for ( itmp = 3; itmp < p->Dict["_MAXINT_"]; ++itmp )
      {
      sprintf( ctmp, "%sIntPt%d", LS_ARRAYNAME_STRESS, itmp + 1 );
      VTK_LS_CELLARRAY(p->Dict["IOSHL(1)"] != 0,LSDynaMetaData::THICK_SHELL,ctmp,6);

      sprintf( ctmp, "%sIntPt%d", LS_ARRAYNAME_EPSTRAIN, itmp + 1 );
      VTK_LS_CELLARRAY(p->Dict["IOSHL(2)"] != 0,LSDynaMetaData::THICK_SHELL,ctmp,1);

      sprintf( ctmp, "%sIntPt%d", LS_ARRAYNAME_INTEGRATIONPOINT, itmp + 1 );
      VTK_LS_CELLARRAY(p->Dict["NEIPS"] > 0,LSDynaMetaData::THICK_SHELL,ctmp,p->Dict["NEIPS"]);
      }
    }

  this->ReadCellProperties(LSDynaMetaData::THICK_SHELL, p->Dict["NV3DT"]);


  // Beam element data=========================================================
  startPos=0;
  VTK_LS_CELLARRAY(1,LSDynaMetaData::BEAM,LS_ARRAYNAME_AXIALFORCE,1);
  VTK_LS_CELLARRAY(1,LSDynaMetaData::BEAM,LS_ARRAYNAME_SHEARRESULTANT,2);
  VTK_LS_CELLARRAY(1,LSDynaMetaData::BEAM,LS_ARRAYNAME_BENDINGRESULTANT,2);
  VTK_LS_CELLARRAY(1,LSDynaMetaData::BEAM,LS_ARRAYNAME_TORSIONRESULTANT,2);

  VTK_LS_CELLARRAY(p->Dict["NV1D"] > 6,LSDynaMetaData::BEAM,LS_ARRAYNAME_SHEARSTRESS,2);
  VTK_LS_CELLARRAY(p->Dict["NV1D"] > 6,LSDynaMetaData::BEAM,LS_ARRAYNAME_AXIALSTRESS,1);
  VTK_LS_CELLARRAY(p->Dict["NV1D"] > 6,LSDynaMetaData::BEAM,LS_ARRAYNAME_AXIALSTRAIN,1);
  VTK_LS_CELLARRAY(p->Dict["NV1D"] > 6,LSDynaMetaData::BEAM,LS_ARRAYNAME_PLASTICSTRAIN,1);
  this->ReadCellProperties(LSDynaMetaData::BEAM, p->Dict["NV1D"]);


  // Shell element data========================================================
  startPos=0;

  // Mid-surface data
  VTK_LS_CELLARRAY(p->Dict["IOSHL(1)"] != 0,LSDynaMetaData::SHELL,LS_ARRAYNAME_STRESS,6);
  VTK_LS_CELLARRAY(p->Dict["IOSHL(2)"] != 0,LSDynaMetaData::SHELL,LS_ARRAYNAME_EPSTRAIN,1);
  VTK_LS_CELLARRAY(p->Dict["NEIPS"] > 0,LSDynaMetaData::SHELL,LS_ARRAYNAME_INTEGRATIONPOINT,p->Dict["NEIPS"]);

  // Inner surface data
  VTK_LS_CELLARRAY(p->Dict["IOSHL(1)"] != 0,LSDynaMetaData::SHELL,LS_ARRAYNAME_STRESS "InnerSurf",6);
  VTK_LS_CELLARRAY(p->Dict["IOSHL(2)"] != 0,LSDynaMetaData::SHELL,LS_ARRAYNAME_EPSTRAIN "InnerSurf",1);
  VTK_LS_CELLARRAY(p->Dict["NEIPS"] > 0,LSDynaMetaData::SHELL,LS_ARRAYNAME_INTEGRATIONPOINT "InnerSurf",p->Dict["NEIPS"]);

  // Outer surface data
  VTK_LS_CELLARRAY(p->Dict["IOSHL(1)"] != 0,LSDynaMetaData::SHELL,LS_ARRAYNAME_STRESS "OuterSurf",6);
  VTK_LS_CELLARRAY(p->Dict["IOSHL(2)"] != 0,LSDynaMetaData::SHELL,LS_ARRAYNAME_EPSTRAIN "OuterSurf",1);
  VTK_LS_CELLARRAY(p->Dict["NEIPS"] > 0,LSDynaMetaData::SHELL,LS_ARRAYNAME_INTEGRATIONPOINT "OuterSurf",p->Dict["NEIPS"]);

  // If _MAXINT_ > 3, there will be additional fields. They are other
  // integration point values. There are (_MAXINT_ - 3) extra
  // integration points, each of which has a stress (6 vals),
  // an effective plastic strain (1 val), and extra integration
  // point values (NEIPS vals).
  for ( itmp = 3; itmp < p->Dict["_MAXINT_"]; ++itmp )
    {
    sprintf( ctmp, "%sIntPt%d", LS_ARRAYNAME_STRESS, itmp + 1 );
    VTK_LS_CELLARRAY(p->Dict["IOSHL(1)"] != 0,LSDynaMetaData::SHELL,ctmp,6);

    sprintf( ctmp, "%sIntPt%d", LS_ARRAYNAME_EPSTRAIN, itmp + 1 );
    VTK_LS_CELLARRAY(p->Dict["IOSHL(2)"] != 0,LSDynaMetaData::SHELL,ctmp,1);

    sprintf( ctmp, "%sIntPt%d", LS_ARRAYNAME_INTEGRATIONPOINT, itmp + 1 );
    VTK_LS_CELLARRAY(p->Dict["NEIPS"] > 0,LSDynaMetaData::SHELL,ctmp,p->Dict["NEIPS"]);
    }

  VTK_LS_CELLARRAY(p->Dict["IOSHL(3)"],LSDynaMetaData::SHELL,LS_ARRAYNAME_BENDINGRESULTANT,3); // Bending Mx, My, Mxy
  VTK_LS_CELLARRAY(p->Dict["IOSHL(3)"],LSDynaMetaData::SHELL,LS_ARRAYNAME_SHEARRESULTANT,2); // Shear Qx, Qy
  VTK_LS_CELLARRAY(p->Dict["IOSHL(3)"],LSDynaMetaData::SHELL,LS_ARRAYNAME_NORMALRESULTANT,3); // Normal Nx, Ny, Nxy

  VTK_LS_CELLARRAY(p->Dict["IOSHL(4)"],LSDynaMetaData::SHELL,LS_ARRAYNAME_THICKNESS,1);
  VTK_LS_CELLARRAY(p->Dict["IOSHL(4)"],LSDynaMetaData::SHELL,LS_ARRAYNAME_ELEMENTMISC,2);

  VTK_LS_CELLARRAY(p->Dict["ISTRN"],LSDynaMetaData::SHELL,LS_ARRAYNAME_STRAIN "InnerSurf",6);
  VTK_LS_CELLARRAY(p->Dict["ISTRN"],LSDynaMetaData::SHELL,LS_ARRAYNAME_STRAIN "OuterSurf",6);

  //we use a temp boolean so that we have less of a chance of causing a bug.
  //if you just insert the or conditions into the macro it becomes a || b && c
  //when you really want (a ||b) && c
  bool valid =(! p->Dict["ISTRN"] || (p->Dict["ISTRN"] && p->Dict["NV2D"] >= 45));
  VTK_LS_CELLARRAY(valid,LSDynaMetaData::SHELL,LS_ARRAYNAME_INTERNALENERGY,1);

  this->ReadCellProperties(LSDynaMetaData::SHELL, p->Dict["NV2D"]);

#undef VTK_LS_CELLARRAY
  return 0;
}

//-----------------------------------------------------------------------------
void vtkLSDynaReader::ReadCellProperties(const int& type,const int& numTuples)
{
  LSDynaMetaData::LSDYNA_TYPES t =
    static_cast<LSDynaMetaData::LSDYNA_TYPES>(type);
  vtkIdType numCells,numSkipStart,numSkipEnd;
  this->Parts->GetPartReadInfo(type,numCells,numSkipStart,numSkipEnd);

  this->P->Fam.SkipWords(numSkipStart * numTuples);
  vtkIdType numChunks = this->P->Fam.InitPartialChunkBuffering(numCells,numTuples);
  vtkIdType startId = 0;
  if(this->P->Fam.GetWordSize() == 8 && numCells > 0)
    {
    for(vtkIdType i=0; i < numChunks; ++i)
      {
      //we need offsets!
      vtkIdType chunkSize = this->P->Fam.GetNextChunk( LSDynaFamily::Float);
      vtkIdType numCellsInChunk = chunkSize/numTuples;
      double *dbuf = this->P->Fam.GetBufferAs<double>();
      this->Parts->FillCellProperties(dbuf,t,startId,numCellsInChunk,numTuples);
      startId += numCellsInChunk;
      }
    }
  else if (numCells > 0)
    {
    for(vtkIdType i=0; i < numChunks; ++i)
      {
      vtkIdType chunkSize = this->P->Fam.GetNextChunk( LSDynaFamily::Float);
      vtkIdType numCellsInChunk = chunkSize/numTuples;
      float *fbuf = this->P->Fam.GetBufferAs<float>();
      this->Parts->FillCellProperties(fbuf,t,startId,numCellsInChunk,numTuples);
      startId += numCellsInChunk;
      }
    }
  this->P->Fam.SkipWords(numSkipEnd * numTuples);

  //clear the buffer as it will be very large and not needed
  this->P->Fam.ClearBuffer();
}


int vtkLSDynaReader::ReadSPHState( vtkIdType vtkNotUsed(step) )
{
  LSDynaMetaData* p = this->P;

  //Make sure we are at the start of the SPH state data
  p->Fam.SkipToWord(LSDynaFamily::TimeStepSection, p->CurrentState,0);
  p->Fam.SkipWords(p->SPHStateOffset);

#define VTK_LS_SPHARRAY(cond,celltype,arrayname,numComps)\
  if ( cond && this->GetCellArrayStatus( celltype, arrayname ) ) \
    { \
    this->Parts->AddProperty(celltype,arrayname,startPos,numComps); \
    } \
  startPos+=numComps;

  // Smooth Particle ========================================================

  // currently have a bug when reading SPH properties disabling for now
  int startPos=0; //used to keep track of the startpos between calls to VTK_LS_CELLARRAY
  VTK_LS_SPHARRAY(               false,LSDynaMetaData::PARTICLE,LS_ARRAYNAME_DEATH,1); //always keep death off
  VTK_LS_SPHARRAY(p->Dict["isphfg(2)"],LSDynaMetaData::PARTICLE,LS_ARRAYNAME_RADIUSOFINFLUENCE,1);
  VTK_LS_SPHARRAY(p->Dict["isphfg(3)"],LSDynaMetaData::PARTICLE,LS_ARRAYNAME_PRESSURE,1);
  VTK_LS_SPHARRAY(p->Dict["isphfg(4)"],LSDynaMetaData::PARTICLE,LS_ARRAYNAME_STRESS,6);
  VTK_LS_SPHARRAY(p->Dict["isphfg(5)"],LSDynaMetaData::PARTICLE,LS_ARRAYNAME_EPSTRAIN,1);
  VTK_LS_SPHARRAY(p->Dict["isphfg(6)"],LSDynaMetaData::PARTICLE,LS_ARRAYNAME_DENSITY,1);
  VTK_LS_SPHARRAY(p->Dict["isphfg(7)"],LSDynaMetaData::PARTICLE,LS_ARRAYNAME_INTERNALENERGY,1);
  VTK_LS_SPHARRAY(p->Dict["isphfg(8)"],LSDynaMetaData::PARTICLE,LS_ARRAYNAME_NUMNEIGHBORS,1);
  VTK_LS_SPHARRAY(p->Dict["isphfg(9)"],LSDynaMetaData::PARTICLE,LS_ARRAYNAME_STRAIN,6);
  VTK_LS_SPHARRAY(p->Dict["isphfg(10)"],LSDynaMetaData::PARTICLE,LS_ARRAYNAME_MASS,1);

//  std::cout << "NUM_SPH_DATA: " << p->Dict["NUM_SPH_DATA"] << "start Pos is " << startPos << std::endl;
  this->ReadCellProperties(LSDynaMetaData::PARTICLE,p->Dict["NUM_SPH_DATA"]);


#undef VTK_LS_SPHARRAY
  return 0;
}

int vtkLSDynaReader::ReadUserMaterialIds()
{
  LSDynaMetaData* p = this->P;
  vtkIdType m, numMats;

  p->MaterialsOrdered.clear();
  p->MaterialsUnordered.clear();
  p->MaterialsLookup.clear();
  // Does the file contain arbitrary material IDs?

  if ( (p->Dict["NARBS"] > 0) && (p->Dict["NSORT"] < 0))
    { // Yes, it does. Read them.


    // Skip over arbitrary node and element IDs:
    vtkIdType skipIds = p->Dict["NUMNP"] + p->Dict["NEL8"] + p->Dict["NEL2"] + p->Dict["NEL4"] + p->Dict["NELT"];
    p->Fam.SkipToWord( LSDynaFamily::UserIdData, p->Fam.GetCurrentAdaptLevel(), 16 + skipIds );

    //in some cases the number of materials in NMAT is incorrect since we are loading
    //SPH materials.
    numMats = p->Dict["NMMAT"];

    // Read in material ID lists:
    p->Fam.BufferChunk( LSDynaFamily::Int, numMats*3 );
    for ( m=0; m<numMats; ++m )
      {
      p->MaterialsOrdered.push_back( p->Fam.GetNextWordAsInt() );
      }
    for ( m=0; m<numMats; ++m )
      {
      p->MaterialsUnordered.push_back( p->Fam.GetNextWordAsInt() );
      }
    for ( m=0; m<numMats; ++m )
      {
      p->MaterialsLookup.push_back( p->Fam.GetNextWordAsInt() );
      }

    }
  else
    {
    numMats = p->Dict["NUMMAT8"] + p->Dict["NUMMATT"] + p->Dict["NUMMAT4"] + p->Dict["NUMMAT2"] + p->Dict["NGPSPH"];
    // No, it doesn't. Fabricate a list of sequential IDs
    // construct the (trivial) material lookup tables
    for ( m = 1; m <= numMats; ++m )
      {
      p->MaterialsOrdered.push_back( m );
      p->MaterialsUnordered.push_back( m );
      p->MaterialsLookup.push_back( m );
      }
    }
  return 0;
}

int vtkLSDynaReader::ReadPartTitlesFromRootFile()
{
  /*
  The extra data is written at the end of the following files:
  d3plot, d3part and intfor files, and the header and part titles are written directly after the
  EOF (= -999999.0) marker.
  Value Length Description
  -------------------------------

  NTYPE 1 entity type = 90001
  NUMPROP 1 number of parts

  For NUMPROP parts:
  IDP 1 part id
  PTITLE 18 Part title (72 characters)

  For NUMPROP parts:
  NTYPE 1 entity type = 90000
  HEAD 18 Header title (72 characters)
  */

  LSDynaMetaData* p = this->P;
  if ( p->PreStateSize <= 0 )
    {
    vtkErrorMacro( "Database has bad pre state size(" << p->PreStateSize << ")." );
    return 1;
    }

 //when called this method is at the right spot to read the part names
  vtkIdType currentFileLoc = p->Fam.GetCurrentFWord();
  vtkIdType currentAdaptLevel = p->Fam.GetCurrentAdaptLevel();

  p->Fam.BufferChunk( LSDynaFamily::Float, 1 );
  double eofM = p->Fam.GetNextWordAsFloat();
  if(eofM !=LSDynaFamily::EOFMarker)
    {
    //we failed to find a marker stop on the part names
    p->Fam.SkipToWord(LSDynaFamily::ControlSection,currentAdaptLevel,currentFileLoc);
    return 1;
    }

  //make sure that the root files has room left for the amount of data we are going to request
  //if it doesn't we know it can't have part names
  vtkIdType numParts = p->PartIds.size();
  vtkIdType partTitlesByteSize = p->Fam.GetWordSize() * (2 + numParts); //NType + NUMPRop + (header part ids)
  partTitlesByteSize += (numParts * 72); //names are constant at 72 bytes each independent of word size

  vtkIdType fileSize = p->Fam.GetFileSize(0);
  if ( fileSize < partTitlesByteSize + p->Fam.GetCurrentFWord())
    {
    //this root file doesn't part names
    p->Fam.SkipToWord(LSDynaFamily::ControlSection,currentAdaptLevel,currentFileLoc);
    return 1;
    }

  //we can now safely read the part titles
  p->Fam.SkipWords(2); //skip types and num of parts
  vtkIdType nameWordSize = 72 / p->Fam.GetWordSize();
  for(vtkIdType i=0; i < numParts; ++i)
    {
    p->Fam.BufferChunk(LSDynaFamily::Int, 1);
    p->Fam.GetNextWordAsInt(); //vtkIdType partId

    p->Fam.BufferChunk( LSDynaFamily::Char, nameWordSize);
    std::string name(p->Fam.GetNextWordAsChars(),72);
    if(name.size() > 0 && name[0]!=' ')
      {
      //strip the name to the subset that
      size_t found = name.find_last_not_of(' ');
      if(found != std::string::npos)
        {
        name = name.substr(0,found+1);
        }
      //get the right part id
      p->PartNames[i] = name;
      }
    }
  p->Fam.SkipToWord(LSDynaFamily::ControlSection,currentAdaptLevel,currentFileLoc);
  return 0;
}

void vtkLSDynaReader::ResetPartInfo()
{
  LSDynaMetaData* p = this->P;
  p->PartNames.clear();
  p->PartIds.clear();
  p->PartMaterials.clear();
  p->PartStatus.clear();

  // Create simple part names as place holders
  int mat = 1, realMat = 1;
  int i;
  int N;
  char partLabel[64];
  int arbitraryMaterials = p->Dict["NMMAT"];

#define VTK_LSDYNA_PARTLABEL(dict,fmt) \
  N = p->Dict[dict]; \
  for ( i = 0; i < N; ++i, ++mat ) \
    { \
    if(arbitraryMaterials) \
    { \
      if(mat < static_cast<int>(p->MaterialsOrdered.size())) \
        { \
        realMat = p->MaterialsOrdered[mat - 1]; \
        } \
      else \
        { \
        realMat = mat; \
        } \
      sprintf( partLabel, fmt " (Matl%d)", mat, realMat ); \
    } \
    else{ \
      realMat = mat; \
      sprintf( partLabel, fmt, mat );  \
      } \
    p->PartNames.push_back( partLabel ); \
    p->PartIds.push_back( realMat ); \
    p->PartMaterials.push_back( mat ); \
    p->PartStatus.push_back( 1 ); \
    }

  VTK_LSDYNA_PARTLABEL("NUMMAT8","Part%d"); // was "PartSolid%d
  VTK_LSDYNA_PARTLABEL("NUMMATT","Part%d"); // was "PartThickShell%d
  VTK_LSDYNA_PARTLABEL("NUMMAT4","Part%d"); // was "PartShell%d
  VTK_LSDYNA_PARTLABEL("NUMMAT2","Part%d"); // was "PartBeam%d
  VTK_LSDYNA_PARTLABEL("NGPSPH", "Part%d"); // was "PartParticle%d
  VTK_LSDYNA_PARTLABEL("NSURF",  "Part%d"); // was "PartRoadSurface%d
  VTK_LSDYNA_PARTLABEL("NUMMAT", "Part%d"); // was "PartRigidBody%d

#undef VTK_LSDYNA_PARTLABEL
}

int vtkLSDynaReader::ReadInputDeck()
{
  if ( ! this->InputDeck )
    {
    // nothing more we can do
    return 0;
    }

  ifstream deck( this->InputDeck, ios::in );
  if ( ! deck.good() )
    {
    return 0;
    }

  std::string header;
  vtkLSGetLine( deck, header );
  deck.seekg( 0, ios::beg );
  int retval;
  if ( vtksys::SystemTools::StringStartsWith( header.c_str(), "<?xml" ) )
    {
    retval = this->ReadInputDeckXML( deck );
    }
  else
    {
    retval = this->ReadInputDeckKeywords( deck );
    }

  return retval;
}

int vtkLSDynaReader::ReadInputDeckXML( ifstream& deck )
{
  vtkLSDynaSummaryParser* parser = vtkLSDynaSummaryParser::New();
  parser->MetaData = this->P;
  parser->SetStream( &deck );
  // We must be able to parse the file and end up with 1 part per material ID
  if ( ! parser->Parse() || this->P->GetTotalMaterialCount() != (int)this->P->PartNames.size() )
    {
    // We had a problem identifying a part, give up and start over by reseting the parts
    this->ResetPartInfo();
    }
  parser->Delete();

  return 0;
}

int vtkLSDynaReader::ReadInputDeckKeywords( ifstream& deck )
{
  int success = 1;
  std::map<std::string,int> parameters;
  std::string line;
  std::string lineLowercase;
  std::string partName;
  int partMaterial;
  int partId;
  int curPart = 0;

  while ( deck.good() && vtkLSNextSignificantLine( deck, line ) && curPart < (int)this->P->PartNames.size() )
    {
    if ( line[0] == '*' )
      {
      vtkLSDowncaseFirstWord( lineLowercase, line.substr( 1 ) );
      if ( vtksys::SystemTools::StringStartsWith( lineLowercase.c_str(), "part" ) )
        {
        // found a part
        // ... read the next non-comment line as the part name
        if ( vtkLSNextSignificantLine( deck, line ) )
          {
          // Get rid of leading and trailing newlines, whitespace, etc.
          vtkLSTrimWhitespace( line );
          partName = line;
          }
        else
          {
          partName = "";
          }
        // ... read the next non-comment line as the part id or a reference to it.
        if ( vtkLSNextSignificantLine( deck, line ) )
          {
          std::vector<std::string> splits;
          vtkLSSplitString( line, splits, "& ,\t\n\r" );
          if ( line[0] == '&' )
            {
            // found a reference. look it up.
            partId = splits.size() ? parameters[splits[0]] : -1;
            }
          else
            {
            if ( splits.size() < 1 || sscanf( splits[0].c_str(), "%d", &partId ) <= 0 )
              {
              partId = -1;
              }
            }
          if ( splits.size() < 3 )
            {
            partMaterial = -1;
            }
          else
            {
            if ( splits[2][0] == '&' )
              {
              partMaterial = parameters[splits[2]];
              }
            else
              {
              if ( sscanf( splits[2].c_str(), "%d", &partMaterial ) <= 0 )
                {
                partMaterial = -1;
                }
              }
            }
          } // read the part id or reference
        else
          {
          partId = -1;
          partMaterial = -1;
          }
        // Comment on next line: partId values need not be consecutive. FIXME: ... or even positive?
        if ( ! partName.empty() && partId >= 0 )
          {
          this->P->PartNames[curPart] = partName;
          this->P->PartIds[curPart] = partId;
          this->P->PartMaterials[curPart] = partMaterial;
          this->P->PartStatus[curPart] = 1;
          fprintf( stderr, "%2d: Part: \"%s\" Id: %d\n", curPart, partName.c_str(), partId );
          ++curPart;
          }
        else
          {
          success = 0;
          }
        }
      else if ( vtksys::SystemTools::StringStartsWith( lineLowercase.c_str(), "parameter" ) )
        {
        // found a reference
        // ... read the next non-comment line to decode the reference
        if ( vtkLSNextSignificantLine( deck, line ) )
          {
          std::string paramName;
          int paramIntVal;
          // Look for "^[IiRr]\s*(\w+)\s+([\w\.-]+)" and set parameters[\2]=\1
          if ( line[0] == 'I' || line[0] == 'i' )
            { // We found an integer parameter. Those are the only ones we care about.
            line = line.substr( 1 );
            std::string::size_type paramStart = line.find_first_not_of( " \t," );
            if ( paramStart == std::string::npos )
              { // ignore a bad parameter line
              continue;
              }
            std::string::size_type paramEnd = line.find_first_of( " \t,", paramStart );
            if ( paramEnd == std::string::npos )
              { // found the parameter name, but no value after it
              continue;
              }
            paramName = line.substr( paramStart, paramEnd - paramStart );
            if ( sscanf( line.substr( paramEnd + 1 ).c_str(), "%d", &paramIntVal ) <= 0 )
              { // unable to read id
              continue;
              }
            parameters[ paramName ] = paramIntVal;
            }
          }
        else
          {
          // no valid line after "*parameter" keyword. Silently ignore it.
          }
        } // "parameter line"
      } // line starts with "*"
    } // while ( deck.good() )

  if ( success )
    {
    // Save a summary file if possible. The user can open the summary file next
    // time and not be forced to parse the entire input deck to get part IDs.
    std::string deckDir = vtksys::SystemTools::GetFilenamePath( this->InputDeck );
    std::string deckName = vtksys::SystemTools::GetFilenameName( this->InputDeck );
    std::string deckExt;
    std::string::size_type dot;
    std::string xmlSummary;

    // GetFilenameExtension doesn't look for the rightmost "." ... do it ourselves.
    dot = deckName.rfind( '.' );
    if ( dot != std::string::npos )
      {
      deckExt = deckName.substr( dot );
      deckName = deckName.substr( 0, dot );
      }
    else
      {
      deckExt = "";
      }
#ifndef WIN32
    xmlSummary = deckDir + "/" + deckName + ".lsdyna";
#else
    xmlSummary = deckDir + "\\" + deckName + ".lsdyna";
#endif // WIN32
    // As long as we don't kill the input deck, write the summary XML:
    if ( xmlSummary != this->InputDeck )
      {
      this->WriteInputDeckSummary( xmlSummary.c_str() );
      }
    }
  else
    {
    // We had a problem identifying a part, give up and reset part info
    this->ResetPartInfo();
    }

  return ! success;
}

int vtkLSDynaReader::WriteInputDeckSummary( const char* fname )
{
  ofstream xmlSummary( fname, ios::out | ios::trunc );
  if ( ! xmlSummary.good() )
    {
    return 1;
    }

  xmlSummary
    << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl
    << "<lsdyna>" << endl;

  std::string dbDir = this->P->Fam.GetDatabaseDirectory();
  std::string dbName = this->P->Fam.GetDatabaseBaseName();
  if ( this->IsDatabaseValid() && ! dbDir.empty() && ! dbName.empty() )
    {
#ifndef WIN32
    if ( dbDir[0] == '/' )
#else
    if ( dbDir[0] == '\\' )
#endif // WIN32
      {
      // OK, we have an absolute path, so it should be safe to write it out.
      xmlSummary
        << "  <database path=\"" << dbDir.c_str()
        << "\" name=\"" << dbName.c_str() << "\"/>" << endl;
      }
    }

  for ( unsigned p = 0; p < this->P->PartNames.size(); ++p )
    {
    xmlSummary
      << "  <part id=\"" << this->P->PartIds[p]
      << "\" material_id=\"" << this->P->PartMaterials[p]
      << "\" status=\"" << this->P->PartStatus[p]
      << "\"><name>" << this->P->PartNames[p].c_str()
      << "</name></part>" << endl;
    }

  xmlSummary
    << "</lsdyna>" << endl;

  return 0;
}

// ================================================== OK Already! Read the file!
int vtkLSDynaReader::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(iinfo),
  vtkInformationVector* oinfo )
{
  LSDynaMetaData* p = this->P;

  if ( ! p->FileIsValid )
    {
    // This should have been set in RequestInformation()
    return 0;
    }
  p->Fam.ClearBuffer();
  p->Fam.OpenFileHandles();

  vtkMultiBlockDataSet* mbds = 0;
  vtkInformation* oi = oinfo->GetInformationObject(0);
  if ( ! oi )
    {
    return 0;
    }

  if ( oi->Has( vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP() ) )
    {
    // Only return single time steps for now.
    double requestedTimeStep = oi->Get( vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    int timeStepLen = oi->Length( vtkStreamingDemandDrivenPipeline::TIME_STEPS() );
    double* timeSteps = oi->Get( vtkStreamingDemandDrivenPipeline::TIME_STEPS() );

    int cnt = 0;
    while ( cnt < timeStepLen - 1 && timeSteps[cnt] < requestedTimeStep )
      {
      ++cnt;
      }
    this->SetTimeStep( cnt );

    oi->Set( vtkDataObject::DATA_TIME_STEP(), p->TimeValues[ p->CurrentState ] );
    }

  mbds = vtkMultiBlockDataSet::SafeDownCast( oi->Get(vtkDataObject::DATA_OBJECT()) );
  if ( ! mbds )
    {
    return 0;
    }
  this->UpdateProgress( 0.01 );

  if ( p->Dict["MATTYP"] )
    {
    // Do something with material type data
    }
  this->UpdateProgress( 0.05 );

  if ( p->Dict["IALEMAT"] )
    {
    // Do something with fluid material ID data
    }
  this->UpdateProgress( 0.10 );

  if ( p->Dict["NMSPH"] )
    {
    // Do something with smooth partical hydrodynamics element data
    }
  this->UpdateProgress( 0.15 );

  //Read in the topology information for caching
  this->ReadTopology();

  // Adapted element parent list
  // This isn't even implemented by LS-Dyna yet

  // Smooth Particle Hydrodynamics Node and Material List are handled in ReadConnectivityAndMaterial()

  // Start of state data ===================
  // I. Node and Cell State
  this->UpdateProgress( 0.6 );
  if ( this->ReadState( p->CurrentState ) )
    {
    vtkErrorMacro( "Problem reading state data for time step " << p->CurrentState );
    return 1;
    }

  // III. SPH Node State
  this->UpdateProgress( 0.7 );
  if ( this->GetNumberOfParticleCells() )
    {
    if ( this->ReadSPHState( p->CurrentState ) )
      {
      vtkErrorMacro( "Problem reading smooth particle hydrodynamics state." );
      return 1;
      }
    }

  this->UpdateProgress( 0.8 );
  //add all the parts as child blocks to the output
  int size = this->Parts->GetNumberOfParts();
  for(int i=0; i < size;++i)
    {
    if (this->Parts->IsActivePart(i))
      {
      vtkUnstructuredGrid *ug = this->Parts->GetGridForPart(i);
      mbds->SetBlock(i,ug);
      mbds->GetMetaData(i)->Set(vtkCompositeDataSet::NAME(),
        this->P->PartNames[i].c_str());
      }
    else
      {
      mbds->SetBlock(i,NULL);
      }
    }

  this->P->Fam.ClearBuffer();
  this->UpdateProgress( 1.0 );
  return 1;
}

//-----------------------------------------------------------------------------
template<typename T>
void vtkLSDynaReader::FillDeletionArray(T *buffer, vtkUnsignedCharArray* arr,
  const vtkIdType& start, const vtkIdType& numCells,
  const int& deathPos, const int& cellSize)
{
  unsigned char val;
  for ( vtkIdType i=0; i<numCells; ++i )
    {
    //Quote from LSDyna Manual:
    //"each value is set to the element material number or =0,
    //if the element is deleted"
    val = (buffer[deathPos] == 0.0) ? 1 : 0;
    buffer+=cellSize;
    arr->SetTuple1(start+i, val);
    }
}

//-----------------------------------------------------------------------------
template <int wordSize,typename T>
int vtkLSDynaReader::FillTopology(T *buff)
{
  //the passed in buffer is null, and only used to specialze the method
  //as pure method specialization isn't support by some compilers
  //READ PARTICLES
  this->P->Fam.SkipToWord(LSDynaFamily::SPHNodeData,
                          this->P->Fam.GetCurrentAdaptLevel(), 0 );
  FillBlock<LSDynaMetaData::PARTICLE,wordSize,1>(buff,this->Parts,this->P,2,
                                      VTK_VERTEX);

  //READ SOLIDS
  this->P->Fam.SkipToWord(LSDynaFamily::GeometryData,
                          this->P->Fam.GetCurrentAdaptLevel(),
                          this->P->NumberOfNodes*this->P->Dimensionality );

  //other than buff, these parameters are changed by the template specialization
  //as SOLID is a unique case
  FillBlock<LSDynaMetaData::SOLID,wordSize,8>(buff,this->Parts,this->P,9,
                                    VTK_HEXAHEDRON);

  //READ THICK_SHELL
  FillBlock<LSDynaMetaData::THICK_SHELL,wordSize,8>(buff,this->Parts,this->P,9,
                                          VTK_QUADRATIC_QUAD);

  //READ BEAM
  FillBlock<LSDynaMetaData::BEAM,wordSize,2>(buff,this->Parts,this->P,6,VTK_LINE);

  //READ SHELL and RIGID_BODY
  //uses a specialization to weave SHELL and RIGID BODY cells together
  FillBlock<LSDynaMetaData::SHELL,wordSize,4>(buff,this->Parts,this->P,5,VTK_QUAD);

  //Read Road Surface
  if ( this->P->ReadRigidRoadMvmt )
    {
    this->P->Fam.SkipToWord( LSDynaFamily::RigidSurfaceData,
                             this->P->Fam.GetCurrentAdaptLevel(),
                             4 + 4*this->P->Dict["NNODE"] );
    FillBlock<LSDynaMetaData::ROAD_SURFACE,wordSize,4>(buff,this->Parts,this->P,5,
                                            VTK_QUAD);
    }
  return 0;
}

//-----------------------------------------------------------------------------
int vtkLSDynaReader::ReadConnectivityAndMaterial()
{
  LSDynaMetaData* p = this->P;
  if ( p->ConnectivityUnpacked == 0 )
    {
    // FIXME
    vtkErrorMacro( "Packed connectivity isn't supported yet." );
    return 1;
    }

  this->Parts->InitCellInsertion();
  if(p->Fam.GetWordSize() == 8)
    {
    vtkIdType *buf=NULL;
    return this->FillTopology<8>(buf);
    }
  else
    {
    int *buf=NULL;
    return this->FillTopology<4>(buf);
    }
}

//-----------------------------------------------------------------------------
template<typename T, int blockType, vtkIdType numWordsPerCell, vtkIdType cellLength>
void vtkLSDynaReader::ReadBlockCellSizes()
{
  //determine the relationship between the file bit size and
  //the host machine bit size. This allows us to read 64 bit files on a
  //32 bit machine
  const int numWordsPerIdType (this->P->Fam.GetWordSize() / sizeof(T));

  vtkIdType nc=0, t=0,j=0,matlId=0;
  vtkIdType numCellsToSkip=0, numCellsToSkipEnd=0, chunkSize=0;
  const T fileNumWordsPerCell(numWordsPerCell * numWordsPerIdType);
  const T offsetToMatId(numWordsPerIdType * (numWordsPerCell-1));
  T* buff = NULL;

  //get from the part the read information for this lsdyna block type
  this->Parts->GetPartReadInfo(blockType,nc,numCellsToSkip,numCellsToSkipEnd);

  this->P->Fam.SkipWords(fileNumWordsPerCell * numCellsToSkip ); //skip to the right start id

  //buffer the amount in small chunks so we don't create a massive buffer
  vtkIdType numChunks = this->P->Fam.InitPartialChunkBuffering(nc,numWordsPerCell);
  for(vtkIdType i=0; i < numChunks; ++i)
    {
    chunkSize = this->P->Fam.GetNextChunk( LSDynaFamily::Int);
    buff = this->P->Fam.GetBufferAs<T>();

    for (j=0; j<chunkSize;j+=numWordsPerCell)
      {
      buff+=offsetToMatId;
      matlId = static_cast<vtkIdType>(*buff);
      buff+=numWordsPerIdType;
      this->Parts->RegisterCellIndexToPart(blockType,matlId,t++,cellLength);
      }
    }
  this->P->Fam.SkipWords(fileNumWordsPerCell * numCellsToSkipEnd);
}

//-----------------------------------------------------------------------------
template <typename T>
int vtkLSDynaReader::FillPartSizes()
{
  //READ PARTICLES
  this->P->Fam.SkipToWord(LSDynaFamily::SPHNodeData,
                          this->P->Fam.GetCurrentAdaptLevel(), 0 );
  this->ReadBlockCellSizes<T,LSDynaMetaData::PARTICLE,2,1>();

  //READ SOLIDS
  this->P->Fam.SkipToWord(LSDynaFamily::GeometryData,
                          this->P->Fam.GetCurrentAdaptLevel(),
                          this->P->NumberOfNodes*this->P->Dimensionality );

  this->ReadBlockCellSizes<T,LSDynaMetaData::SOLID,9,8>();

  //READ THICK_SHELL
  this->ReadBlockCellSizes<T,LSDynaMetaData::THICK_SHELL,9,8>();

  //READ BEAM
  this->ReadBlockCellSizes<T,LSDynaMetaData::BEAM,6,2>();

  //READ SHELL and RIGID_BODY
  //uses a specialization to weave SHELL and RIGID BODY cells together
  this->ReadBlockCellSizes<T,LSDynaMetaData::SHELL,5,4>();

  //Read Road Surface
  if ( this->P->ReadRigidRoadMvmt )
    {
    this->P->Fam.SkipToWord( LSDynaFamily::RigidSurfaceData,
                             this->P->Fam.GetCurrentAdaptLevel(),
                             4 + 4*this->P->Dict["NNODE"] );
    this->ReadBlockCellSizes<T,LSDynaMetaData::ROAD_SURFACE,5,4>();
    }

  //now that all the registering is done tell the collection
  //it can allocate the necessary space for each part
  this->Parts->AllocateParts();
  return 0;
}

//-----------------------------------------------------------------------------
int vtkLSDynaReader::ReadPartSizes()
{
  LSDynaMetaData* p = this->P;
  if ( p->ConnectivityUnpacked == 0 )
    {
    // FIXME
    vtkErrorMacro( "Packed connectivity isn't supported yet." );
    return 1;
    }

  if(p->Fam.GetWordSize() == 8)
    {
    return this->FillPartSizes<vtkIdType>();
    }
  else
    {
    return this->FillPartSizes<int>();
    }
}

//-----------------------------------------------------------------------------
void vtkLSDynaReader::SetDeformedMesh(int deformed)
{
  if (this->DeformedMesh != deformed)
    {
    this->DeformedMesh = deformed;
    this->ResetPartsCache();
    this->Modified();
    }
}
