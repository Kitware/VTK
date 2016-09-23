/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFlashReaderInternal.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAMRFlashReaderInternal.h"

//-----------------------------------------------------------------------------
void vtkFlashReaderInternal::GetBlockAttribute(
    const char *atribute, int blockIdx, vtkDataSet *pDataSet )
{
 // this function must be called by GetBlock( ... )
 this->ReadMetaData();

 if ( atribute == NULL || blockIdx < 0  ||
      pDataSet == NULL || blockIdx >= this->NumberOfBlocks )
 {
//   vtkDebugMacro( "Data attribute name or vtkDataSet NULL, or " <<
//                  "invalid block index." << endl );
   return;
 }
 // remove the prefix ("mesh_blockandlevel/" or "mesh_blockandproc/") to get
 // the actual attribute name
 std::string  tempName = atribute;
 size_t          slashPos = tempName.find( "/" );
 std::string  attrName = tempName.substr ( slashPos + 1 );
 hid_t           dataIndx = H5Dopen
                            ( this->FileIndex, attrName.c_str() );

 if ( dataIndx < 0 )
 {
//   vtkErrorMacro( "Invalid attribute name." << endl );
   return;
 }

 hid_t    spaceIdx = H5Dget_space( dataIndx );
 hsize_t  dataDims[4]; // dataDims[0] == number of blocks
 hsize_t  numbDims = H5Sget_simple_extent_dims( spaceIdx, dataDims, NULL );

 if ( numbDims != 4 )
 {
//   vtkErrorMacro( "Error with reading the data dimensions." << endl );
   return;
 }

 int      numTupls = dataDims[1] * dataDims[2] * dataDims[3];
 hsize_t  startVec[5];
 hsize_t  stridVec[5];
 hsize_t  countVec[5];

 startVec[0] = blockIdx;
 startVec[1] = 0;
 startVec[2] = 0;
 startVec[3] = 0;

 stridVec[0] = 1;
 stridVec[1] = 1;
 stridVec[2] = 1;
 stridVec[3] = 1;

 countVec[0] = 1;
 countVec[1] = dataDims[1];
 countVec[2] = dataDims[2];
 countVec[3] = dataDims[3];

 // file space index
 hid_t      filSpace = H5Screate_simple( 4, dataDims, NULL );
 H5Sselect_hyperslab ( filSpace, H5S_SELECT_SET, startVec,
                       stridVec, countVec,       NULL );

 startVec[0] = 0;
 startVec[1] = 0;
 startVec[2] = 0;
 startVec[3] = 0;

 stridVec[0] = 1;
 stridVec[1] = 1;
 stridVec[2] = 1;
 stridVec[3] = 1;

 countVec[0] = 1;
 countVec[1] = dataDims[1];
 countVec[2] = dataDims[2];
 countVec[3] = dataDims[3];

 hid_t      memSpace = H5Screate_simple( 4, dataDims, NULL );
 H5Sselect_hyperslab ( memSpace, H5S_SELECT_SET, startVec,
                       stridVec, countVec,       NULL );

 vtkDoubleArray   * dataAray = vtkDoubleArray::New();
 dataAray->SetName( atribute );
 dataAray->SetNumberOfTuples( numTupls );
 double           * arrayPtr = static_cast < double * >
                               (  dataAray->GetPointer( 0 )  );

 int    i;
 hid_t  hRawType = H5Dget_type( dataIndx );
 hid_t  dataType = H5Tget_native_type( hRawType, H5T_DIR_ASCEND );

 if (  H5Tequal( dataType, H5T_NATIVE_DOUBLE )  )
 {
   H5Dread( dataIndx, dataType,    memSpace,
            filSpace, H5P_DEFAULT, arrayPtr );
 }
 else
 if (  H5Tequal( dataType, H5T_NATIVE_FLOAT )  )
 {
   float * dataFlts = new float [ numTupls ];
   H5Dread( dataIndx, dataType,    memSpace,
            filSpace, H5P_DEFAULT, dataFlts );
   for ( i = 0; i < numTupls; i ++ )
   {
     arrayPtr[i] = dataFlts[i];
   }
   delete [] dataFlts;
   dataFlts = NULL;
 }
 else
 if (  H5Tequal( dataType, H5T_NATIVE_INT )  )
 {
   int * dataInts = new int [ numTupls ];
   H5Dread( dataIndx, dataType,    memSpace,
            filSpace, H5P_DEFAULT, dataInts );
   for ( i = 0; i < numTupls; i ++ )
   {
     arrayPtr[i] = dataInts[i];
   }
   delete[] dataInts;
   dataInts = NULL;
 }
 else
 if (  H5Tequal( dataType, H5T_NATIVE_UINT )  )
 {
   unsigned int * unsgnInt = new unsigned int [ numTupls ];
   H5Dread( dataIndx, dataType,    memSpace,
            filSpace, H5P_DEFAULT, unsgnInt );
   for ( i = 0; i < numTupls; i ++ )
   {
     arrayPtr[i] = unsgnInt[i];
   }
   delete[] unsgnInt;
   unsgnInt = NULL;
 }
 else
 {
//   vtkErrorMacro( "Invalid data attribute type." << endl );
 }

 H5Sclose( filSpace );
 H5Sclose( memSpace );
 H5Sclose( spaceIdx );
 H5Tclose( dataType );
 H5Tclose( hRawType );
 H5Dclose( dataIndx );

 pDataSet->GetCellData()->AddArray ( dataAray );

 dataAray->Delete();
 dataAray = NULL;
 arrayPtr = NULL;

}

//-----------------------------------------------------------------------------
void vtkFlashReaderInternal::Init()
{
  this->FileName  = NULL;
  this->FileIndex = -1;
  this->MinBounds[0] =
  this->MinBounds[1] =
  this->MinBounds[2] = VTK_DOUBLE_MAX;
  this->MaxBounds[0] =
  this->MaxBounds[1] =
  this->MaxBounds[2] =-VTK_DOUBLE_MAX;

  this->NumberOfBlocks = 0;
  this->NumberOfLevels = 0;
  this->FileFormatVersion  =-1;
  this->NumberOfParticles  = 0;
  this->NumberOfLeafBlocks = 0;
  this->NumberOfDimensions = 0;
  this->NumberOfProcessors = 0;
  this->HaveProcessorsInfo = 0;
  this->BlockGridDimensions[0] = 1;
  this->BlockGridDimensions[1] = 1;
  this->BlockGridDimensions[2] = 1;
  this->BlockCellDimensions[0] = 1;
  this->BlockCellDimensions[1] = 1;
  this->BlockCellDimensions[2] = 1;
  this->NumberOfChildrenPerBlock  = 0;
  this->NumberOfNeighborsPerBlock = 0;

  this->Blocks.clear();
  this->LeafBlocks.clear();
  this->AttributeNames.clear();

  this->ParticleName = "";
  this->ParticleAttributeTypes.clear();
  this->ParticleAttributeNames.clear();
  this->ParticleAttributeNamesToIds.clear();
}

// ----------------------------------------------------------------------------
int vtkFlashReaderInternal::GetCycle()
{
  const bool bTmCycle = true;

  hid_t      fileIndx = H5Fopen( this->FileName, H5F_ACC_RDONLY, H5P_DEFAULT );
  if ( fileIndx < 0 )
  {
    return -VTK_INT_MAX;
  }

  this->ReadVersionInformation( fileIndx );
  this->ReadSimulationParameters( fileIndx, bTmCycle );
  H5Fclose( fileIndx );

  return this->SimulationParameters.NumberOfTimeSteps;
}

// ----------------------------------------------------------------------------
double vtkFlashReaderInternal::GetTime()
{
    const bool bTmCycle = true;

    hid_t      fileIndx = H5Fopen( this->FileName, H5F_ACC_RDONLY, H5P_DEFAULT);
    if ( fileIndx < 0 )
    {
      return -VTK_DOUBLE_MAX;
    }

    this->ReadVersionInformation( fileIndx );
    this->ReadSimulationParameters( fileIndx, bTmCycle );
    H5Fclose( fileIndx );

    return this->SimulationParameters.Time;
}

//-----------------------------------------------------------------------------
void vtkFlashReaderInternal::ReadMetaData()
{
  if ( this->FileIndex >= 0 )
  {
    return;
  }

  // file handle
  this->FileIndex = H5Fopen( this->FileName, H5F_ACC_RDONLY, H5P_DEFAULT );
  if ( this->FileIndex < 0 )
  {
    vtkGenericWarningMacro( "Failed to open file " << this->FileName <<
                            "." << endl );
    return;
  }

  // file format version
  this->ReadVersionInformation( this->FileIndex );
  if ( this->FileFormatVersion < FLASH_READER_FLASH3_FFV8 )
  {
    this->ReadParticleAttributes();       // FLASH2 version
  }
  else
  {
    this->ReadParticleAttributesFLASH3(); // FLASH3 version
  }

  // block structures
  this->ReadBlockStructures();
  if ( this->NumberOfParticles == 0 && this->NumberOfBlocks == 0 )
  {
    vtkGenericWarningMacro( "Invalid Flash file, without any " <<
                            "block/particle." << endl );
    return;
  }

  // obtain further information about blocks
  if ( this->NumberOfBlocks > 0 )
  {
    this->ReadBlockBounds();
    this->ReadRefinementLevels();
    this->ReadSimulationParameters( this->FileIndex );
    this->ReadDataAttributeNames();
    this->GetBlockMinMaxGlobalDivisionIds();
    this->ReadBlockTypes();
    this->ReadBlockCenters();
    this->ReadProcessorIds();
  }
}

//-----------------------------------------------------------------------------
void vtkFlashReaderInternal::ReadProcessorIds()
{
  hid_t rootIndx = H5Gopen( this->FileIndex, "/" );
  if ( rootIndx < 0 )
  {
    vtkGenericWarningMacro( "Failed to open the root group" << endl );
    return;
  }

  hsize_t numbObjs;
  herr_t  errorIdx = H5Gget_num_objs( rootIndx, &numbObjs );
  if ( errorIdx < 0 )
  {
    vtkGenericWarningMacro( "Failed to get the number of objects " <<
                            "in the root group" << endl );
    return;
  }

  hsize_t        objIndex;
  std::string sObjName = "processor number";
  char           namefromfile[17];
  for ( objIndex = 0; objIndex < numbObjs; objIndex ++ )
  {
    ssize_t objsize = H5Gget_objname_by_idx( rootIndx, objIndex, NULL, 0 );
    if ( objsize == 16 )
    {
      H5Gget_objname_by_idx( rootIndx, objIndex, namefromfile, 17 );
      std::string tempstr = namefromfile;
      if ( tempstr == sObjName ) // if this file contains processor numbers
      {
        this->HaveProcessorsInfo = 1;
      }
    }
  }
  H5Gclose( rootIndx );

  if ( this->HaveProcessorsInfo )
  {
    // Read the processor number description for the blocks
    hid_t procnumId = H5Dopen( this->FileIndex, "processor number" );
    if ( procnumId < 0 )
    {
      vtkGenericWarningMacro( "Processor Id information not found." << endl );
    }

    hid_t procnumSpaceId = H5Dget_space( procnumId );

    hsize_t procnum_dims[1];
    hsize_t procnum_ndims = H5Sget_simple_extent_dims
                            ( procnumSpaceId, procnum_dims, NULL );

    if (  static_cast<int> ( procnum_ndims   ) != 1 ||
          static_cast<int> ( procnum_dims[0] ) != this->NumberOfBlocks  )
    {
      vtkGenericWarningMacro( "Error with getting the number of " <<
                              "processor Ids." << endl );
    }

    hid_t procnum_raw_data_type = H5Dget_type( procnumId );
    hid_t procnum_data_type = H5Tget_native_type
                              ( procnum_raw_data_type, H5T_DIR_ASCEND );

    int * procnum_array = new int [ this->NumberOfBlocks ];
    H5Dread( procnumId, procnum_data_type, H5S_ALL,
             H5S_ALL, H5P_DEFAULT, procnum_array );

    int highProcessor = -1;
    for (int b = 0; b < this->NumberOfBlocks; b ++ )
    {
      int pnum = procnum_array[b];
      if ( pnum > highProcessor )
      {
        highProcessor = pnum;
        this->NumberOfProcessors ++;
      }
      this->Blocks[b].ProcessorId = pnum;
    }

    H5Tclose( procnum_data_type );
    H5Tclose( procnum_raw_data_type );
    H5Sclose( procnumSpaceId );
    H5Dclose( procnumId );

    delete[] procnum_array;
    procnum_array = NULL;
  }
  else
  {
    this->NumberOfProcessors = 1;
    for ( int b = 0; b < this->NumberOfBlocks; b ++ )
    {
      this->Blocks[b].ProcessorId = 0;
    }
  }
}

//-----------------------------------------------------------------------------
void vtkFlashReaderInternal::ReadDoubleScalars( hid_t fileIndx )
{
  // Should only be used for FLASH3 files

  if ( this->FileFormatVersion < FLASH_READER_FLASH3_FFV8 )
  {
    vtkGenericWarningMacro( "Error with the format version." << endl );
    return;
  }

  hid_t realScalarsId = H5Dopen( fileIndx, "real scalars" );
  //
  // Read the real scalars
  //
  if ( realScalarsId < 0 )
  {
    vtkGenericWarningMacro( "Real scalars not found in FLASH3." << endl );
    return;
  }

  hid_t spaceId = H5Dget_space( realScalarsId );
  if ( spaceId < 0 )
  {
    vtkGenericWarningMacro( "Failed to get the real scalars space." << endl );
    return;
  }

  hsize_t scalarDims[10];
  H5Sget_simple_extent_dims( spaceId, scalarDims, NULL );

  int nScalars = scalarDims[0];

  hid_t datatype = H5Tcreate
                   (  H5T_COMPOUND,  sizeof( FlashReaderDoubleScalar )  );

  hid_t string20 = H5Tcopy( H5T_C_S1 );
  H5Tset_size( string20, 20 );

  H5Tinsert(  datatype, "name",
              HOFFSET( FlashReaderDoubleScalar, Name  ), string20  );
  H5Tinsert(  datatype, "value",
              HOFFSET( FlashReaderDoubleScalar, Value ), H5T_NATIVE_DOUBLE  );

  FlashReaderDoubleScalar * rs = new FlashReaderDoubleScalar[ nScalars ];
  H5Dread(realScalarsId, datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, rs);

  for ( int i = 0; i < nScalars; i ++ )
  {
    if (  strncmp( rs[i].Name, "time", 4 ) == 0  )
    {
      this->SimulationParameters.Time = rs[i].Value;
    }
  }

  delete [] rs;
  rs = NULL;

  H5Tclose( string20 );
  H5Tclose( datatype );
  H5Sclose( spaceId  );
  H5Dclose( realScalarsId );
}

//-----------------------------------------------------------------------------
void vtkFlashReaderInternal::ReadIntegerScalars( hid_t fileIndx )
{
  // Should only be used for FLASH3 files

  if ( this->FileFormatVersion < FLASH_READER_FLASH3_FFV8 )
  {
    vtkGenericWarningMacro( "Error with the format version." << endl );
    return;
  }

  hid_t intScalarsId = H5Dopen( fileIndx, "integer scalars" );

  // Read the integer scalars
  if ( intScalarsId < 0 )
  {
    vtkGenericWarningMacro( "Integer scalars not found in FLASH3." << endl );
    return;
  }

  hid_t spaceId = H5Dget_space( intScalarsId );
  if ( spaceId < 0 )
  {
    vtkGenericWarningMacro( "Failed to get the integer scalars space." << endl );
    return;
  }

  hsize_t  scalarDims[1];
  H5Sget_simple_extent_dims( spaceId, scalarDims, NULL );
  int   nScalars = scalarDims[0];

  hid_t datatype = H5Tcreate
                   (  H5T_COMPOUND,  sizeof( FlashReaderIntegerScalar )  );

  hid_t string20 = H5Tcopy( H5T_C_S1 );
  H5Tset_size( string20, 20 );

  H5Tinsert(  datatype, "name",
              HOFFSET( FlashReaderIntegerScalar, Name  ), string20       );
  H5Tinsert(  datatype, "value",
              HOFFSET( FlashReaderIntegerScalar, Value ), H5T_NATIVE_INT );

  FlashReaderIntegerScalar * is = new FlashReaderIntegerScalar [ nScalars ];
  H5Dread( intScalarsId, datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, is );

  for ( int i = 0; i < nScalars; i ++ )
  {
    if (  strncmp( is[i].Name, "nxb", 3 ) == 0  )
    {
      this->SimulationParameters.NumberOfXDivisions = is[i].Value;
    }
    else
    if (  strncmp( is[i].Name, "nyb", 3 ) == 0  )
    {
      this->SimulationParameters.NumberOfYDivisions = is[i].Value;
    }
    else
    if (  strncmp( is[i].Name, "nzb", 3 ) == 0  )
    {
      this->SimulationParameters.NumberOfZDivisions = is[i].Value;
    }
    else
    if (  strncmp( is[i].Name, "globalnumblocks", 15 ) == 0  )
    {
      this->SimulationParameters.NumberOfBlocks = is[i].Value;
    }
    else
    if (  strncmp( is[i].Name, "nstep", 5 ) == 0  )
    {
      this->SimulationParameters.NumberOfTimeSteps = is[i].Value;
    }
  }

  delete [] is;
  is = NULL;

  H5Tclose( string20 );
  H5Tclose( datatype );
  H5Sclose( spaceId  );
  H5Dclose( intScalarsId );
}

//-----------------------------------------------------------------------------
void vtkFlashReaderInternal::ReadVersionInformation( hid_t fileIndx )
{
  // temporarily disable error reporting
  H5E_auto_t   old_errorfunc;
  void       * old_clientdata = NULL;
  H5Eget_auto( &old_errorfunc, &old_clientdata );
  H5Eset_auto( NULL, NULL );

  // If this is a FLASH3 Particles file, or a FLASH3 file with particles,
  // then it will have the "particle names" field.  If, in addition, it's a
  // file format version (FFV) 9 file, it can have "file format version" and
  // "sim info", so further checking is needed.  Further checking is also
  // needed for non-particle files.  So...further checking all around.

  int flash3_particles = 0;   //  Init to false
  hid_t h5_PN = H5Dopen( fileIndx, "particle names" );
  if ( h5_PN >= 0 )
  {
    flash3_particles = 1;
    H5Dclose( h5_PN );
  }

  // Read the file format version  (<= 7 means FLASH2)
  hid_t h5_FFV = H5Dopen( fileIndx, "file format version" );

  if ( h5_FFV < 0 )
  {
    hid_t h5_SI = H5Dopen( fileIndx, "sim info" );
    if ( h5_SI < 0 )
    {
      if ( flash3_particles == 1 )
      {
        this->FileFormatVersion = FLASH_READER_FLASH3_FFV8;
      }
      else
      {
        this->FileFormatVersion = 7;
      }
    }
    else
    {
      // Read the "sim info" components
      hid_t si_type = H5Tcreate(  H5T_COMPOUND,
                                  sizeof( FlashReaderSimulationInformation )  );
      H5Tinsert(  si_type, "file format version",
                  HOFFSET( FlashReaderSimulationInformation, FileFormatVersion ),
                  H5T_STD_I32LE  );
      H5Tinsert(  si_type, "setup call",
                  HOFFSET( FlashReaderSimulationInformation, SetupCall ),
                  H5T_STRING  );
      H5Tinsert(  si_type, "file creation time",
                  HOFFSET( FlashReaderSimulationInformation, FileCreationTime ),
                  H5T_STRING  );
      H5Tinsert(  si_type, "flash version",
                  HOFFSET( FlashReaderSimulationInformation, FlashVersion ),
                  H5T_STRING  );
      H5Tinsert(  si_type, "build date",
                  HOFFSET( FlashReaderSimulationInformation, BuildData ),
                  H5T_STRING  );
      H5Tinsert(  si_type, "build dir",
                  HOFFSET( FlashReaderSimulationInformation, BuildDirectory ),
                  H5T_STRING  );
      H5Tinsert(  si_type, "build machine",
                  HOFFSET( FlashReaderSimulationInformation, build_machine ),
                  H5T_STRING  );
      H5Tinsert(  si_type, "cflags",
                  HOFFSET(FlashReaderSimulationInformation, CFlags ),
                  H5T_STRING  );
      H5Tinsert(  si_type, "fflags",
                  HOFFSET( FlashReaderSimulationInformation, FFlags ),
                  H5T_STRING  );
      H5Tinsert(  si_type, "setup time stamp",
                  HOFFSET( FlashReaderSimulationInformation, SetupTimeStamp ),
                  H5T_STRING  );
      H5Tinsert(  si_type, "build time stamp",
                  HOFFSET( FlashReaderSimulationInformation, BuildTimeStamp ),
                  H5T_STRING  );

      H5Dread( h5_SI,   si_type,     H5S_ALL,
               H5S_ALL, H5P_DEFAULT, &this->SimulationInformation );

      H5Tclose( si_type );
      H5Dclose( h5_SI );

      // FileFormatVersion is readin as little-endian. On BE machines, we need to
      // ensure that it's swapped back to right order.
      // The following will have no effect on LE machines.
      vtkByteSwap::SwapLE(&this->SimulationInformation.FileFormatVersion);
      this->FileFormatVersion = this->SimulationInformation.FileFormatVersion;
    }

    // turn back on error reporting
    H5Eset_auto( old_errorfunc, old_clientdata );
    old_clientdata = NULL;
    return;
  }

  if ( flash3_particles == 1 )
  {
    this->FileFormatVersion = FLASH_READER_FLASH3_FFV8;
  }
  else
  {
    // FLASH 2 has file format version available in global attributes.
    H5Dread( h5_FFV,  H5T_NATIVE_INT, H5S_ALL,
             H5S_ALL, H5P_DEFAULT,    &this->FileFormatVersion );
  }

  H5Dclose( h5_FFV );

  // turn back on error reporting
  H5Eset_auto( old_errorfunc, old_clientdata );
  old_clientdata = NULL;
}

//-----------------------------------------------------------------------------
void vtkFlashReaderInternal::ReadSimulationParameters
  ( hid_t fileIndx, bool bTmCycle )
{
  if ( this->FileFormatVersion < FLASH_READER_FLASH3_FFV8 )
  {
    // Read the simulation parameters
    hid_t simparamsId = H5Dopen( fileIndx, "simulation parameters" );
    if ( simparamsId < 0 )
    {
      vtkGenericWarningMacro( "Simulation parameters unavailable." << endl );
    }

    hid_t sp_type = H5Tcreate(  H5T_COMPOUND,
                                sizeof( FlashReaderSimulationParameters )  );

    H5Tinsert(  sp_type, "total blocks",
                HOFFSET( FlashReaderSimulationParameters, NumberOfBlocks ),
                H5T_NATIVE_INT  );
    H5Tinsert(  sp_type, "time",
                HOFFSET( FlashReaderSimulationParameters, Time ),
                H5T_NATIVE_DOUBLE  );
    H5Tinsert(  sp_type, "timestep",
                HOFFSET( FlashReaderSimulationParameters, TimeStep ),
                H5T_NATIVE_DOUBLE  );
    H5Tinsert(  sp_type, "redshift",
                HOFFSET( FlashReaderSimulationParameters, RedShift ),
                H5T_NATIVE_DOUBLE  );
    H5Tinsert(  sp_type, "number of steps",
                HOFFSET( FlashReaderSimulationParameters, NumberOfTimeSteps ),
                H5T_NATIVE_INT  );
    H5Tinsert(  sp_type, "nxb",
                HOFFSET( FlashReaderSimulationParameters, NumberOfXDivisions ),
                H5T_NATIVE_INT  );
    H5Tinsert(  sp_type, "nyb",
                HOFFSET( FlashReaderSimulationParameters, NumberOfYDivisions ),
                H5T_NATIVE_INT  );
    H5Tinsert(  sp_type, "nzb",
                HOFFSET( FlashReaderSimulationParameters, NumberOfZDivisions ),
                H5T_NATIVE_INT  );

    H5Dread( simparamsId, sp_type,     H5S_ALL,
             H5S_ALL,     H5P_DEFAULT, &this->SimulationParameters );

    H5Tclose( sp_type );
    H5Dclose( simparamsId );
  }
  else
  {
    this->ReadIntegerScalars( fileIndx );
    this->ReadDoubleScalars ( fileIndx );
  }

  if ( bTmCycle )
  {
    return;
  }

  // Sanity check: size of the gid array better match number of blocks
  //               reported in the simulation parameters
  if ( this->SimulationParameters.NumberOfBlocks != this->NumberOfBlocks )
  {
    vtkGenericWarningMacro( "Inconsistency in the number of blocks." << endl );
    return;
  }

  if ( this->SimulationParameters.NumberOfXDivisions == 1 )
  {
    this->BlockGridDimensions[0] = 1;
    this->BlockCellDimensions[0] = 1;
  }
  else
  {
    this->BlockGridDimensions[0] =
    this->SimulationParameters.NumberOfXDivisions + 1;
    this->BlockCellDimensions[0] =
    this->SimulationParameters.NumberOfXDivisions;
  }

  if ( this->SimulationParameters.NumberOfYDivisions == 1 )
  {
    this->BlockGridDimensions[1] = 1;
    this->BlockCellDimensions[1] = 1;
  }
  else
  {
    this->BlockGridDimensions[1] =
    this->SimulationParameters.NumberOfYDivisions + 1;
    this->BlockCellDimensions[1] =
    this->SimulationParameters.NumberOfYDivisions;
  }

  if ( this->SimulationParameters.NumberOfZDivisions == 1 )
  {
    this->BlockGridDimensions[2] = 1;
    this->BlockCellDimensions[2] = 1;
  }
  else
  {
    this->BlockGridDimensions[2] =
    this->SimulationParameters.NumberOfZDivisions + 1;
    this->BlockCellDimensions[2] =
    this->SimulationParameters.NumberOfZDivisions;
  }
}

//-----------------------------------------------------------------------------
void vtkFlashReaderInternal::GetBlockMinMaxGlobalDivisionIds()
{
  double problemsize[3] = { this->MaxBounds[0] - this->MinBounds[0],
                            this->MaxBounds[1] - this->MinBounds[1],
                            this->MaxBounds[2] - this->MinBounds[2] };

  for ( int b = 0; b < this->NumberOfBlocks; b ++ )
  {
    Block & B = this->Blocks[b];

    for ( int d = 0; d < 3; d ++ )
    {
      if ( d < this->NumberOfDimensions )
      {
        double factor = problemsize[d] / ( B.MaxBounds[d] - B.MinBounds[d] );
        double start  = ( B.MinBounds[d] - this->MinBounds[d] ) / problemsize[d];

        double beg = this->BlockCellDimensions[d] * start * factor;
        double end = this->BlockCellDimensions[d] * start * factor +
                     this->BlockCellDimensions[d];
        this->Blocks[b].MinGlobalDivisionIds[d] = int( beg + 0.5 );
        this->Blocks[b].MaxGlobalDivisionIds[d] = int( end + 0.5 );
      }
      else
      {
        this->Blocks[b].MinGlobalDivisionIds[d] = 0;
        this->Blocks[b].MaxGlobalDivisionIds[d] = 0;
      }
    }
  }
}

//-----------------------------------------------------------------------------
void vtkFlashReaderInternal::ReadBlockTypes()
{
  // Read the node type description for the blocks
  hid_t nodetypeId = H5Dopen( this->FileIndex, "node type" );
  if ( nodetypeId < 0 )
  {
    vtkGenericWarningMacro( "Block types not found." << endl );
    return;
  }

  hid_t nodetypeSpaceId = H5Dget_space( nodetypeId );

  hsize_t nodetype_dims[1];
  hsize_t nodetype_ndims = H5Sget_simple_extent_dims
                           ( nodetypeSpaceId, nodetype_dims, NULL );

  if (  static_cast<int> ( nodetype_ndims   ) != 1 ||
        static_cast<int> ( nodetype_dims[0] ) != this->NumberOfBlocks  )
  {
    vtkGenericWarningMacro( "Inconsistency in the number of blocks." << endl );
    return;
  }

  hid_t nodetype_raw_data_type = H5Dget_type( nodetypeId );
  hid_t nodetype_data_type = H5Tget_native_type
                             ( nodetype_raw_data_type, H5T_DIR_ASCEND );

  int * nodetype_array = new int [ this->NumberOfBlocks ];
  H5Dread( nodetypeId, nodetype_data_type, H5S_ALL,
           H5S_ALL,    H5P_DEFAULT,        nodetype_array );

  this->NumberOfLeafBlocks = 0;
  for ( int b = 0; b < this->NumberOfBlocks; b ++ )
  {
    int ntype = nodetype_array[b];
    this->Blocks[b].Type = ntype;
    if ( ntype == FLASH_READER_LEAF_BLOCK )
    {
      this->NumberOfLeafBlocks ++;
      this->LeafBlocks.push_back( b );
    }
  }

  delete [] nodetype_array;
  nodetype_array = NULL;

  H5Tclose( nodetype_data_type );
  H5Tclose( nodetype_raw_data_type );
  H5Sclose( nodetypeSpaceId );
  H5Dclose( nodetypeId );
}

// ----------------------------------------------------------------------------
void vtkFlashReaderInternal::ReadBlockBounds()
{
  // Read the bounding box description for the blocks
  hid_t bboxId = H5Dopen( this->FileIndex, "bounding box" );
  if ( bboxId < 0 )
  {
    vtkGenericWarningMacro( "Blocks bounding info not found." << endl );
    return;
  }

  hid_t  bboxSpaceId = H5Dget_space( bboxId );
  hsize_t bbox_dims[3];
  hsize_t bbox_ndims = H5Sget_simple_extent_dims
                       ( bboxSpaceId, bbox_dims, NULL );

  if ( this->FileFormatVersion <= FLASH_READER_FLASH3_FFV8 )
  {
    if (  static_cast<int> ( bbox_ndims   ) != 3 ||
          static_cast<int> ( bbox_dims[0] ) != this->NumberOfBlocks ||
          static_cast<int> ( bbox_dims[1] ) != this->NumberOfDimensions ||
          static_cast<int> ( bbox_dims[2] ) != 2  )
    {
      vtkGenericWarningMacro( "Error with number of blocks " <<
                              "or number of dimensions." << endl );
      return;
    }

    double * bbox_array = new double [ this->NumberOfBlocks *
                                       this->NumberOfDimensions * 2 ];
    H5Dread( bboxId,  H5T_NATIVE_DOUBLE, H5S_ALL,
             H5S_ALL, H5P_DEFAULT,       bbox_array );

    this->MinBounds[0] = VTK_DOUBLE_MAX;
    this->MinBounds[1] = VTK_DOUBLE_MAX;
    this->MinBounds[2] = VTK_DOUBLE_MAX;
    this->MaxBounds[0] =-VTK_DOUBLE_MAX;
    this->MaxBounds[1] =-VTK_DOUBLE_MAX;
    this->MaxBounds[2] =-VTK_DOUBLE_MAX;

    for (int b=0; b<this->NumberOfBlocks; b++)
    {
      double * bbox_line = &bbox_array[ this->NumberOfDimensions * 2 * b ];
      for ( int d = 0; d < 3; d ++ )
      {
        if ( d + 1 <= this->NumberOfDimensions )
        {
          this->Blocks[b].MinBounds[d] = bbox_line[ d * 2 + 0 ];
          this->Blocks[b].MaxBounds[d] = bbox_line[ d * 2 + 1 ];
        }
        else
        {
          this->Blocks[b].MinBounds[d] = 0;
          this->Blocks[b].MaxBounds[d] = 0;
        }

        if ( this->Blocks[b].MinBounds[0] < this->MinBounds[0] )
        {
          this->MinBounds[0] = this->Blocks[b].MinBounds[0];
        }

        if ( this->Blocks[b].MinBounds[1] < this->MinBounds[1] )
        {
          this->MinBounds[1] = this->Blocks[b].MinBounds[1];
        }

        if ( this->Blocks[b].MinBounds[2] < this->MinBounds[2] )
        {
          this->MinBounds[2] = this->Blocks[b].MinBounds[2];
        }

        if ( this->Blocks[b].MaxBounds[0] > this->MaxBounds[0] )
        {
          this->MaxBounds[0] = this->Blocks[b].MaxBounds[0];
        }

        if ( this->Blocks[b].MaxBounds[1] > this->MaxBounds[1] )
        {
          this->MaxBounds[1] = this->Blocks[b].MaxBounds[1];
        }

        if ( this->Blocks[b].MaxBounds[2] > this->MaxBounds[2] )
        {
          this->MaxBounds[2] = this->Blocks[b].MaxBounds[2];
        }
      }

      bbox_line = NULL;
    }

    delete[] bbox_array;
    bbox_array = NULL;
  }
  else
  if ( this->FileFormatVersion == FLASH_READER_FLASH3_FFV9 )
  {
    if (  static_cast<int> ( bbox_ndims   ) != 3 ||
          static_cast<int> ( bbox_dims[0] ) != this->NumberOfBlocks  ||
          static_cast<int> ( bbox_dims[1] ) != FLASH_READER_MAX_DIMS ||
          static_cast<int> ( bbox_dims[2] ) != 2  )
    {
      vtkGenericWarningMacro( "Error with number of blocks." << endl );
      return;
    }

    double * bbox_array = new double [ this->NumberOfBlocks *
                                       FLASH_READER_MAX_DIMS * 2];
    H5Dread( bboxId,  H5T_NATIVE_DOUBLE, H5S_ALL,
             H5S_ALL, H5P_DEFAULT,       bbox_array );

    this->MinBounds[0] = VTK_DOUBLE_MAX;
    this->MinBounds[1] = VTK_DOUBLE_MAX;
    this->MinBounds[2] = VTK_DOUBLE_MAX;
    this->MaxBounds[0] =-VTK_DOUBLE_MAX;
    this->MaxBounds[1] =-VTK_DOUBLE_MAX;
    this->MaxBounds[2] =-VTK_DOUBLE_MAX;

    for ( int b = 0; b < this->NumberOfBlocks; b ++ )
    {
      double * bbox_line = &bbox_array[ FLASH_READER_MAX_DIMS * 2 * b ];

      for ( int d = 0; d < 3; d ++ )
      {
        this->Blocks[b].MinBounds[d] = bbox_line[ d * 2 + 0 ];
        this->Blocks[b].MaxBounds[d] = bbox_line[ d * 2 + 1 ];

        if ( this->Blocks[b].MinBounds[0] < this->MinBounds[0] )
        {
          this->MinBounds[0] = this->Blocks[b].MinBounds[0];
        }

        if ( this->Blocks[b].MinBounds[1] < this->MinBounds[1] )
        {
          this->MinBounds[1] = this->Blocks[b].MinBounds[1];
        }

        if ( this->Blocks[b].MinBounds[2] < this->MinBounds[2] )
        {
          this->MinBounds[2] = this->Blocks[b].MinBounds[2];
        }

        if ( this->Blocks[b].MaxBounds[0] > this->MaxBounds[0] )
        {
          this->MaxBounds[0] = this->Blocks[b].MaxBounds[0];
        }

        if ( this->Blocks[b].MaxBounds[1] > this->MaxBounds[1] )
        {
          this->MaxBounds[1] = this->Blocks[b].MaxBounds[1];
        }

        if ( this->Blocks[b].MaxBounds[2] > this->MaxBounds[2] )
        {
          this->MaxBounds[2] = this->Blocks[b].MaxBounds[2];
        }
      }

      bbox_line = NULL;
    }

    delete[] bbox_array;
    bbox_array = NULL;
  }

  H5Sclose(bboxSpaceId);
  H5Dclose(bboxId);
}

//-----------------------------------------------------------------------------
void vtkFlashReaderInternal::ReadBlockCenters()
{
  // Read the coordinates description for the blocks
  hid_t coordinatesId = H5Dopen( this->FileIndex, "coordinates" );
  if ( coordinatesId < 0 )
  {
    vtkGenericWarningMacro( "Block centers not found." << endl );
    return;
  }

  hid_t coordinatesSpaceId = H5Dget_space( coordinatesId );

  hsize_t coordinates_dims[2];
  hsize_t coordinates_ndims = H5Sget_simple_extent_dims
                              ( coordinatesSpaceId, coordinates_dims, NULL );

  if ( this->FileFormatVersion <= FLASH_READER_FLASH3_FFV8 )
  {
    if (  static_cast<int> ( coordinates_ndims   ) != 2 ||
          static_cast<int> ( coordinates_dims[0] ) != this->NumberOfBlocks ||
          static_cast<int> ( coordinates_dims[1] ) != this->NumberOfDimensions  )
    {
      vtkGenericWarningMacro( "Error with number of blocks or " <<
                              "number of dimensions." << endl );
      return;
    }

    double * coordinates_array = new double [ this->NumberOfBlocks *
                                              this->NumberOfDimensions ];
    H5Dread( coordinatesId, H5T_NATIVE_DOUBLE, H5S_ALL,
             H5S_ALL,       H5P_DEFAULT,       coordinates_array );

    for ( int b = 0; b < this->NumberOfBlocks; b ++ )
    {
      double * coords = &coordinates_array[ this->NumberOfDimensions * b ];

      if ( this->NumberOfDimensions == 1 )
      {
        this->Blocks[b].Center[0] = coords[0];
        this->Blocks[b].Center[1] = 0.0;
        this->Blocks[b].Center[2] = 0.0;
      }
      else
      if ( this->NumberOfDimensions == 2 )
      {
        this->Blocks[b].Center[0] = coords[0];
        this->Blocks[b].Center[1] = coords[1];
        this->Blocks[b].Center[2] = 0.0;
      }
      else
      if ( this->NumberOfDimensions == 3 )
      {
        this->Blocks[b].Center[0] = coords[0];
        this->Blocks[b].Center[1] = coords[1];
        this->Blocks[b].Center[2] = coords[2];
      }

      coords = NULL;
    }

    delete [] coordinates_array;
    coordinates_array = NULL;
  }
  else
  if ( this->FileFormatVersion == FLASH_READER_FLASH3_FFV9 )
  {
    if (  static_cast<int> ( coordinates_ndims   ) != 2 ||
          static_cast<int> ( coordinates_dims[0] ) != this->NumberOfBlocks ||
          static_cast<int> ( coordinates_dims[1] ) != FLASH_READER_MAX_DIMS  )
    {
      vtkGenericWarningMacro( "Error with number of blocks." << endl );
      return;
    }

    double * coordinates_array = new double [ this->NumberOfBlocks *
                                              FLASH_READER_MAX_DIMS ];
    H5Dread( coordinatesId, H5T_NATIVE_DOUBLE, H5S_ALL,
             H5S_ALL,       H5P_DEFAULT,       coordinates_array );

    for ( int b = 0; b < this->NumberOfBlocks; b ++ )
    {
      double * coords = &coordinates_array[ FLASH_READER_MAX_DIMS * b ];
      this->Blocks[b].Center[0] = coords[0];
      this->Blocks[b].Center[1] = coords[1];
      this->Blocks[b].Center[2] = coords[2];
      coords = NULL;
    }

    delete [] coordinates_array;
    coordinates_array = NULL;
  }

  H5Sclose( coordinatesSpaceId );
  H5Dclose( coordinatesId );
}

// ----------------------------------------------------------------------------
void vtkFlashReaderInternal::ReadBlockStructures()
{
  // temporarily disable error reporting
  H5E_auto_t  old_errorfunc;
  void      * old_clientdata = NULL;
  H5Eget_auto( &old_errorfunc, &old_clientdata );
  H5Eset_auto( NULL, NULL );

  // Read the "gid" block connectivity description
  hid_t gidId = H5Dopen( this->FileIndex, "gid" );

  // turn back on error reporting
  H5Eset_auto( old_errorfunc, old_clientdata );

  if ( gidId < 0 )
  {
    this->NumberOfBlocks = 0;
    old_clientdata = NULL;
    return;
  }

  hid_t gidSpaceId = H5Dget_space( gidId );

  hsize_t gid_dims[2];
  hsize_t gid_ndims =  H5Sget_simple_extent_dims( gidSpaceId, gid_dims, NULL );
  if ( gid_ndims != 2 )
  {
    vtkGenericWarningMacro( "Error with reading block connectivity." << endl );
    return;
  }

  this->NumberOfBlocks = gid_dims[0];
  switch ( gid_dims[1] )
  {
    case 5:
      this->NumberOfDimensions        = 1;
      this->NumberOfChildrenPerBlock  = 2;
      this->NumberOfNeighborsPerBlock = 2;
      break;

    case 9:
      this->NumberOfDimensions        = 2;
      this->NumberOfChildrenPerBlock  = 4;
      this->NumberOfNeighborsPerBlock = 4;
      break;

    case 15:
      this->NumberOfDimensions        = 3;
      this->NumberOfChildrenPerBlock  = 8;
      this->NumberOfNeighborsPerBlock = 6;
      break;

    default:
      vtkGenericWarningMacro( "Invalid block connectivity." << endl );
      break;
  }

  hid_t gid_raw_data_type = H5Dget_type( gidId );
  hid_t gid_data_type = H5Tget_native_type( gid_raw_data_type, H5T_DIR_ASCEND );

  int * gid_array = new int [ this->NumberOfBlocks * gid_dims[1] ];
  H5Dread( gidId,   gid_data_type, H5S_ALL,
           H5S_ALL, H5P_DEFAULT,   gid_array );

  // convert to an easier-to-grok format
  this->Blocks.resize( this->NumberOfBlocks );
  for ( int b = 0; b < this->NumberOfBlocks; b ++ )
  {
    int * gid_line = &gid_array[ gid_dims[1] * b ];
    int   pos = 0;
    int   n;

    this->Blocks[b].Index = b + 1;  // 1-origin IDs

    for ( n = 0; n < 6; n ++ )
    {
      this->Blocks[b].NeighborIds[n] = -32;
    }

    for ( n = 0; n < this->NumberOfNeighborsPerBlock; n ++ )
    {
      this->Blocks[b].NeighborIds[n] = gid_line[ pos ++ ];
    }

    this->Blocks[b].ParentId = gid_line[ pos ++ ];

    for ( n = 0; n < 8; n ++ )
    {
      this->Blocks[b].ChildrenIds[n] = -1;
    }

    for ( n = 0; n < this->NumberOfChildrenPerBlock; n ++ )
    {
      this->Blocks[b].ChildrenIds[n] = gid_line[ pos ++ ];
    }

    gid_line = NULL;
  }

  delete[] gid_array;
  gid_array = NULL;

  H5Tclose( gid_data_type );
  H5Tclose( gid_raw_data_type );
  H5Sclose( gidSpaceId );
  H5Dclose( gidId );
}

// ----------------------------------------------------------------------------
void vtkFlashReaderInternal::ReadRefinementLevels()
{
  // Read the bounding box description for the blocks
  hid_t refinementId = H5Dopen( this->FileIndex, "refine level" );
  if ( refinementId < 0 )
  {
    vtkGenericWarningMacro( "Refinement levels not found." << endl );
    return;
  }

  hid_t refinementSpaceId = H5Dget_space( refinementId );

  hsize_t refinement_dims[1];
  hsize_t refinement_ndims = H5Sget_simple_extent_dims
                             ( refinementSpaceId, refinement_dims, NULL );

  if (  static_cast<int> ( refinement_ndims   ) != 1 ||
        static_cast<int> ( refinement_dims[0] ) != this->NumberOfBlocks  )
  {
    vtkGenericWarningMacro( "Error with number of blocks" << endl );
    return;
  }

  hid_t refinement_raw_data_type = H5Dget_type( refinementId );
  hid_t refinement_data_type = H5Tget_native_type
                               ( refinement_raw_data_type, H5T_DIR_ASCEND );

  int * refinement_array = new int [ this->NumberOfBlocks ];
  H5Dread( refinementId, refinement_data_type, H5S_ALL,
           H5S_ALL,      H5P_DEFAULT,          refinement_array );

  for ( int b = 0; b < this->NumberOfBlocks; b ++ )
  {
    int level = refinement_array[b];
    this->Blocks[b].Level = level;
    if ( level > this->NumberOfLevels )
    {
      this->NumberOfLevels = level;
    }
  }

  delete[] refinement_array;
  refinement_array = NULL;

  H5Tclose( refinement_data_type );
  H5Tclose( refinement_raw_data_type );
  H5Sclose( refinementSpaceId );
  H5Dclose( refinementId );
}

//-----------------------------------------------------------------------------
void vtkFlashReaderInternal::ReadDataAttributeNames()
{
  hid_t unknownsId = H5Dopen( this->FileIndex, "unknown names" );
  if ( unknownsId < 0 )
  {
    vtkGenericWarningMacro( "Data attributes not found." << endl );
    return;
  }

  hid_t unkSpaceId = H5Dget_space( unknownsId );

  hsize_t unk_dims[2];
  hsize_t unk_ndims =  H5Sget_simple_extent_dims( unkSpaceId, unk_dims, NULL );
  if ( unk_ndims != 2 || unk_dims[1] != 1 )
  {
    vtkGenericWarningMacro( "Error with reading data attributes." << endl );
    return;
  }

  hid_t unk_raw_data_type = H5Dget_type( unknownsId );
  int length = (int)(H5Tget_size( unk_raw_data_type ));

  int nvars = unk_dims[0];
  char * unk_array = new char [ nvars * length ];

  H5Dread( unknownsId, unk_raw_data_type, H5S_ALL,
           H5S_ALL,    H5P_DEFAULT,       unk_array );

  this->AttributeNames.resize( nvars );
  char * tmpstring = new char [ length + 1 ];
  for ( int v = 0; v < nvars; v ++ )
  {
    for ( int c = 0; c < length; c ++ )
    {
      tmpstring[c] = unk_array[ v * length + c ];
    }
    tmpstring[ length ] = '\0';

    this->AttributeNames[v] = tmpstring;
  }

  delete [] unk_array;
  delete [] tmpstring;
  unk_array = NULL;
  tmpstring = NULL;

  H5Tclose( unk_raw_data_type );
  H5Sclose( unkSpaceId );
  H5Dclose( unknownsId );
}

// ----------------------------------------------------------------------------
void vtkFlashReaderInternal::ReadParticlesComponent
  ( hid_t dataIndx, const char * compName, double * dataBuff )
{
  if ( !compName || this->FileFormatVersion < FLASH_READER_FLASH3_FFV8 )
  {
    vtkGenericWarningMacro( "Invalid component name of particles or " <<
                            "non FLASH3_FFV8 file format." << endl );
    return;
  }

  hsize_t    spaceIdx = H5Dget_space( dataIndx ); // data space index
  hsize_t    thisSize = this->NumberOfParticles;
  hsize_t    spaceMem = H5Screate_simple( 1, &thisSize, NULL );
  int        attrIndx = this->ParticleAttributeNamesToIds[ compName ];

  hsize_t    theShift[2] = {0,static_cast<hsize_t>(attrIndx)};
  hsize_t    numReads[2] = {static_cast<hsize_t>(this->NumberOfParticles),1};
  H5Sselect_hyperslab ( spaceIdx, H5S_SELECT_SET, theShift,
                        NULL,     numReads,       NULL );
  H5Dread( dataIndx, H5T_NATIVE_DOUBLE, spaceMem,
           spaceIdx, H5P_DEFAULT,       dataBuff );

  H5Sclose( spaceIdx );
  H5Sclose( spaceMem );
}

// ----------------------------------------------------------------------------
void vtkFlashReaderInternal::ReadParticleAttributes()
{
  // temporarily disable error reporting
  H5E_auto_t  old_errorfunc;
  void      * old_clientdata = NULL;
  H5Eget_auto( &old_errorfunc, &old_clientdata );
  H5Eset_auto( NULL, NULL );

  // find the particle variable (if it exists)
  hid_t pointId;
  this->ParticleName = "particle tracers";
  pointId = H5Dopen( this->FileIndex, this->ParticleName.c_str() );
  if ( pointId < 0 )
  {
    this->ParticleName = "tracer particles";
    pointId = H5Dopen( this->FileIndex, this->ParticleName.c_str() );
  }

  // turn back on error reporting
  H5Eset_auto( old_errorfunc, old_clientdata );

  if ( pointId < 0 )
  {
    this->NumberOfParticles = 0;
    old_clientdata = NULL;
    return;
  }

  hid_t pointSpaceId = H5Dget_space( pointId );

  hsize_t p_dims[100];
  hsize_t p_ndims =  H5Sget_simple_extent_dims( pointSpaceId, p_dims, NULL );
  if ( p_ndims != 1 )
  {
    vtkGenericWarningMacro( "Error with number of data attributes." << endl );
  }

  this->NumberOfParticles = p_dims[0];

  hid_t point_raw_type = H5Dget_type( pointId );
  int numMembers = H5Tget_nmembers( point_raw_type );
  for ( int i = 0; i < numMembers; i ++ )
  {
    char  * member_name = H5Tget_member_name( point_raw_type, i );
    std::string nice_name = GetSeparatedParticleName( member_name );
    hid_t  member_raw_type = H5Tget_member_type( point_raw_type, i );
    hid_t  member_type = H5Tget_native_type( member_raw_type, H5T_DIR_ASCEND );
    int    index = (int)(this->ParticleAttributeTypes.size());

    if (  strcmp( member_name, "particle_x" )  &&
          strcmp( member_name, "particle_y" )  &&
          strcmp( member_name, "particle_z" )
       )
    {
      if (  H5Tequal( member_type, H5T_NATIVE_DOUBLE ) > 0  )
      {
        this->ParticleAttributeTypes.push_back( H5T_NATIVE_DOUBLE );
        this->ParticleAttributeNames.push_back( member_name );
        this->ParticleAttributeNamesToIds[ nice_name ] = index;
      }
      else
      if (  H5Tequal( member_type, H5T_NATIVE_INT ) > 0  )
      {
        this->ParticleAttributeTypes.push_back( H5T_NATIVE_INT );
        this->ParticleAttributeNames.push_back( member_name );
        this->ParticleAttributeNamesToIds[ nice_name ] = index;
      }
      else
      {
        vtkGenericWarningMacro( "Only DOUBLE and INT supported." << endl );
      }
    }

    // We read the particles before the grids.  Just in case we
    // don't have any grids, take a stab at the problem dimension
    // based purely on the existence of various data members.
    // This will be overwritten by the true grid topological
    // dimension if the grid exists.
    if (  strcmp( member_name, "particle_x" ) == 0 &&
          this->NumberOfDimensions < 1  )
    {
      this->NumberOfDimensions = 1;
    }

    if (  strcmp( member_name, "particle_y" ) == 0 &&
          this->NumberOfDimensions < 2  )
    {
      this->NumberOfDimensions = 2;
    }

    if (  strcmp( member_name, "particle_z" ) == 0 &&
          this->NumberOfDimensions < 3  )
    {
      this->NumberOfDimensions = 3;
    }

    member_name = NULL;

    H5Tclose( member_type );
    H5Tclose( member_raw_type );
  }

  H5Tclose( point_raw_type );
  H5Sclose( pointSpaceId );
  H5Dclose( pointId );
}

// ----------------------------------------------------------------------------
void vtkFlashReaderInternal::ReadParticleAttributesFLASH3()
{
  // Should only be used for FLASH3 files
  if ( this->FileFormatVersion < FLASH_READER_FLASH3_FFV8 )
  {
    return;
  }

  // temporarily disable error reporting
  H5E_auto_t  old_errorfunc;
  void      * old_clientdata = NULL;
  H5Eget_auto( &old_errorfunc, &old_clientdata );
  H5Eset_auto( NULL, NULL );

  hid_t pnameId = H5Dopen( this->FileIndex, "particle names" );

  // turn back on error reporting
  H5Eset_auto( old_errorfunc, old_clientdata );

  if ( pnameId < 0 )
  {
    this->NumberOfParticles = 0;
    old_clientdata = NULL;
    return;
  }

  hid_t pnamespace = H5Dget_space( pnameId );
  hsize_t dims[10];
  hsize_t ndims =  H5Sget_simple_extent_dims( pnamespace, dims, NULL );

  // particle names ndims should be 2, and if the second dim isn't 1,
  // need to come up with a way to handle it!
  if ( ndims != 2 || dims[1] != 1 )
  {
    if ( ndims != 2 )
    {
      vtkGenericWarningMacro( "FLASH3 expecting particle names ndims of 2, got "
                              << ndims << endl );
    }
    if ( dims[1] != 1 )
    {
      vtkGenericWarningMacro( "FLASH3 expecting particle names dims[1] of 1, got "
                              << dims[1] << endl );
    }
  }

  int numNames = dims[0];

  // create the right-size string, and a char array to read the data into
  hid_t string24 = H5Tcopy( H5T_C_S1 );
  H5Tset_size( string24, 24 );
  char * cnames = new char [ 24 * numNames ];
  H5Dread( pnameId, string24, H5S_ALL, H5S_ALL, H5P_DEFAULT, cnames );

  // Convert the single string to individual variable names.
  std::string  snames( cnames );
  delete[] cnames;
  cnames = NULL;

  for ( int i = 0; i < numNames; i ++ )
  {
    std::string name = snames.substr( i * 24, 24 );

    int sp = (int)(name.find_first_of(' '));
    if ( sp < 24 )
    {
      name = name.substr( 0, sp );
    }

    if (  name != "particle_x" &&
          name != "particle_y" &&
          name != "particle_z"
       )
    {
      std::string nice_name = GetSeparatedParticleName( name );
      this->ParticleAttributeTypes.push_back( H5T_NATIVE_DOUBLE );
      this->ParticleAttributeNames.push_back( name );
      this->ParticleAttributeNamesToIds[ nice_name ] = i;
    }

    // We read the particles before the grids.  Just in case we
    // don't have any grids, take a stab at the problem dimension
    // based purely on the existence of various data members.
    // This will be overwritten by the true grid topological
    // dimension if the grid exists.
    if ( name == "posx" && this->NumberOfDimensions < 1 )
    {
      this->NumberOfDimensions = 1;
    }

    if ( name == "posy" && this->NumberOfDimensions < 2 )
    {
      this->NumberOfDimensions = 2;
    }

    if ( name == "posz" && this->NumberOfDimensions < 3 )
    {
      this->NumberOfDimensions = 3;
    }
  }

  H5Tclose( string24 );
  H5Sclose( pnamespace );
  H5Dclose( pnameId );

  // Read particle dimensions and particle HDFVarName

  // temporarily disable error reporting
  H5Eget_auto( &old_errorfunc, &old_clientdata );
  H5Eset_auto( NULL, NULL );

  // find the particle variable (if it exists)
  hid_t pointId;
  this->ParticleName = "particle tracers";
  pointId = H5Dopen( this->FileIndex, this->ParticleName.c_str() );
  if ( pointId < 0 )
  {
    this->ParticleName = "tracer particles";
    pointId = H5Dopen( this->FileIndex, this->ParticleName.c_str() );
  }

  // turn back on error reporting
  H5Eset_auto( old_errorfunc, old_clientdata );

  // Doesn't exist?  No problem -- we just don't have any particles
  if ( pointId < 0 )
  {
    vtkGenericWarningMacro( "FLASH3 no tracer particles" << endl );
    this->NumberOfParticles = 0;
    old_clientdata = NULL;
    return;
  }

  hid_t pointSpaceId = H5Dget_space( pointId );

  hsize_t p_dims[10];
  hsize_t p_ndims =  H5Sget_simple_extent_dims( pointSpaceId, p_dims, NULL );
  if ( p_ndims != 2 )
  {
    vtkGenericWarningMacro( "FLASH3, expecting particle tracer ndims of 2, got"
                            << p_ndims << endl );
  }
  this->NumberOfParticles = p_dims[0];

  H5Sclose( pointSpaceId );
  H5Dclose( pointId );
}
