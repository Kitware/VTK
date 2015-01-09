/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : Id  */
/*  Date : $Date$ */
/*  Version : $Revision$ */
/*                                                                 */
/*  Author:                                                        */
/*     Jerry A. Clarke                                             */
/*     clarke@arl.army.mil                                         */
/*     US Army Research Laboratory                                 */
/*     Aberdeen Proving Ground, MD                                 */
/*                                                                 */
/*     All Rights Reserved                                         */
/*     See Copyright.txt or http://www.arl.hpc.mil/ice for details */
/*                                                                 */
/*     This software is distributed WITHOUT ANY WARRANTY; without  */
/*     even the implied warranty of MERCHANTABILITY or FITNESS     */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice   */
/*     for more information.                                       */
/*                                                                 */
/*******************************************************************/
#include "XdmfHDF.h"
#include "XdmfDsmBuffer.h"
#include "XdmfH5Driver.h"

#include <cstring>

#ifdef WIN32
#define XDMF_HDF5_SIZE_T        ssize_t
#else
#define XDMF_HDF5_SIZE_T        hsize_t
#endif

namespace xdmf2
{

XdmfHDF::XdmfHDF() {

  H5dont_atexit();

  // We Know Nothing is Open
  this->File = H5I_BADID;
  this->Cwd = H5I_BADID;
  this->Dataset = H5I_BADID;
  this->CreatePlist = H5P_DEFAULT;
  this->AccessPlist = H5P_DEFAULT;

  // Defaults
  this->NumberOfChildren = 0;
  this->Compression = 0;
  this->UseSerialFile = 0;
  // We may have been compiled with Parallel IO support, but be run only on a single
  // machine without mpiexec. Disable parallel if just one process.
#if H5_HAVE_PARALLEL && ((H5_VERS_MAJOR>1)||((H5_VERS_MAJOR==1)&&(H5_VERS_MINOR>=6)))
  int valid, nprocs=0;
  MPI_Initialized(&valid);
  if (valid) 
  {
    MPI_Comm_size(MPI_COMM_WORLD,&nprocs);
  }
  if (nprocs<=1) 
  {
    this->UseSerialFile = 1;
  }
#endif

  this->DsmBuffer = 0;
  strcpy( this->CwdName, "" );
}

XdmfHDF::~XdmfHDF() {
  XdmfInt32 i;

  this->Close();
  for( i = 0 ; i < this->NumberOfChildren ; i++ ){
    delete this->Child[ i ];
  }
}

//
// Set the next child in the list
//
void
XdmfHDF::SetNextChild( XdmfConstString Name)
{

  this->Child[ this->NumberOfChildren ] = new char[ strlen( Name ) + 2 ];
  strcpy( this->Child[ this->NumberOfChildren ], Name );
  this->NumberOfChildren++;
}

XdmfConstString
XdmfHDF::GetHDFVersion(void){
  static char VersionBuf[80];
  ostrstream  Version(VersionBuf,80);
  unsigned major, minor, release;
  XdmfConstString toReturn;

  if(H5get_libversion(&major, &minor, &release) >= 0 ){
    Version << major << "." << minor << "." << release << ends;
  } else {
    Version << "-1.0" << ends;
  } 

  toReturn = (XdmfConstString)Version.str();
  Version.rdbuf()->freeze(0);
  return(toReturn);
}
//
// Get Directory Name from Dataset
//
XdmfString
GetDirectoryName( XdmfConstString Name )
{
  static char Directory[XDMF_MAX_STRING_LENGTH];
  XdmfString slash;

  strcpy( Directory, Name );
  slash = strrchr( Directory, '/' );
  if( slash == NULL ){
    // No Slash Present
    strcpy( Directory, ".");  
  } else if( slash == &Directory[0] ) {
    // Root
    strcpy( Directory, "/" );
  } else {
    *slash = '\0';
  }

  return( Directory );
}

#if (H5_VERS_MAJOR>1)||((H5_VERS_MAJOR==1)&&(H5_VERS_MINOR>=8))
#else
/*
* One often needs to temporarily disable automatic error reporting when
* trying something that's likely or expected to fail.  For instance, to
* determine if an object exists one can call H5Gget_objinfo() which will fail if
* the object doesn't exist.  The code to try can be nested between calls to
* H5Eget_auto() and H5Eset_auto(), but it's easier just to use this macro
* like:
*      H5E_BEGIN_TRY {
*          ...stuff here that's likely to fail...
*      } H5E_END_TRY;
*
* Warning: don't break, return, or longjmp() from the body of the loop or
*          the error reporting won't be properly restored!
*
* This does not work on Sun because of extern "C"
*/
#undef H5E_BEGIN_TRY
extern "C" { typedef herr_t (*H5E_saved_efunc_type)(void*); }
#define H5E_BEGIN_TRY {                                                       \
  H5E_saved_efunc_type H5E_saved_efunc;                                     \
  void *H5E_saved_edata;                                                    \
  H5Eget_auto (&H5E_saved_efunc, &H5E_saved_edata);                         \
  H5Eset_auto (NULL, NULL);


/*Need to redefine H5E_END_TRY also, to allow this to compile with hdf version
*1.7.40. The 1.7.40 version of H5E_END_TRY is not compatible with version
*1.6.2, and the above edited version of H5E_BEGIN_TRY came from version 1.6.2.
*The following version of H5E_END_TRY comes from 1.6.2 with no edits.*/
#undef H5E_END_TRY
#define H5E_END_TRY                                                           \
  H5Eset_auto (H5E_saved_efunc, H5E_saved_edata);                           \
}
#endif
//
// Get Type of Object
//
XdmfInt32
XdmfHDF::Info( hid_t Group, XdmfConstString Name )
{
  H5G_stat_t StatusBuffer;
  herr_t Status;

  H5E_BEGIN_TRY {
    Status = H5Gget_objinfo(Group,
      Name,
      0,
      &StatusBuffer );
  } H5E_END_TRY;

  if( ( Status >= 0 ) )
  {
    if ( StatusBuffer.type == H5G_GROUP || StatusBuffer.type == H5G_DATASET )
    {
      return( StatusBuffer.type );
    }
  }

  return( H5G_UNKNOWN );
}

//
// Used by Giterate()
//
#ifdef __cplusplus
extern "C" {
#endif
  herr_t
    XdmfHDFList( hid_t group, XdmfConstString name, void *me )
  {
    XdmfHDF *ThisClassPtr = ( XdmfHDF *)me;

    switch ( ThisClassPtr->Info( group, name ) ) {
  case H5G_GROUP :
    break;
  case H5G_DATASET :
    break;
  default :
    return(0);
    }
    ThisClassPtr->SetNextChild( name );
    return(0);
  }
#ifdef __cplusplus
}
#endif

//
// Set the Current Working Directory
//
XdmfInt32
XdmfHDF::SetCwdName( XdmfConstString Directory )
{
  hid_t    NewDirectory;
  XdmfConstString  NewDirectoryName = Directory;
  XdmfInt32  i, Type;

  Type = this->Info( this->Cwd, Directory );
  if ( Type != H5G_GROUP ) {
    NewDirectoryName = GetDirectoryName( Directory );
    Type = this->Info( this->Cwd, NewDirectoryName );
    if ( Type != H5G_GROUP ) {
      return( XDMF_FAIL );
    }
  }

  if( NewDirectoryName[0] == '/' ){
    strcpy( this->CwdName, NewDirectoryName );
  } else {
    if( NewDirectoryName[ strlen( NewDirectoryName ) - 1 ] != '/' ) {
      strcat( this->CwdName, "/");
    }
    strcat( this->CwdName, NewDirectoryName);
  }
  for( i = 0 ; i < this->NumberOfChildren ; i++ ){
    delete this->Child[ i ];
  }
  this->NumberOfChildren = 0;
  H5Giterate( this->Cwd,
    NewDirectoryName,
    NULL,
    XdmfHDFList,
    this );
#if (H5_VERS_MAJOR>1)||((H5_VERS_MAJOR==1)&&(H5_VERS_MINOR>=8))
  NewDirectory = H5Gopen2( this->Cwd, NewDirectoryName, H5P_DEFAULT );
#else
  NewDirectory = H5Gopen( this->Cwd, NewDirectoryName );
#endif
  H5Gclose( this->Cwd );
  this->Cwd = NewDirectory;

  return( XDMF_SUCCESS );
}


XdmfInt32
XdmfHDF::Mkdir( XdmfString Name )
{
  hid_t    NewDirectory = -1;

  XdmfDebug( " Checking for Existance of HDF Directory " << Name );
  H5E_BEGIN_TRY {
#if (!H5_USE_16_API && ((H5_VERS_MAJOR>1)||((H5_VERS_MAJOR==1)&&(H5_VERS_MINOR>=8))))
    NewDirectory = H5Gopen( this->Cwd, Name, H5P_DEFAULT );
#else
    NewDirectory = H5Gopen( this->Cwd, Name );
#endif
  } H5E_END_TRY;
  if ( NewDirectory < 0 ) {
    XdmfDebug( " Creating HDF Directory " << Name );
#if (!H5_USE_16_API && ((H5_VERS_MAJOR>1)||((H5_VERS_MAJOR==1)&&(H5_VERS_MINOR>=8))))
    H5Gcreate(this->Cwd, Name , 0, H5P_DEFAULT, H5P_DEFAULT);
#else
    H5Gcreate(this->Cwd, Name , 0);
#endif
  } else {
    XdmfDebug(Name << " Already exists");
  }

  // Re-Scan Children
  return( this->SetCwdName( this->CwdName ) );
}

XdmfInt32
XdmfHDF::DoClose() {

  XdmfDebug("Closing");
  H5E_BEGIN_TRY {
    if ( this->CreatePlist != H5P_DEFAULT ){
      XdmfDebug("Closing Create Plist");
      H5Pclose( this->CreatePlist );
      this->CreatePlist = H5P_DEFAULT;
    }
    if ( this->AccessPlist != H5P_DEFAULT ){
      XdmfDebug("Closing Access Plist");
      H5Pclose( this->AccessPlist );
      this->AccessPlist = H5P_DEFAULT;
    }
    if ( this->Cwd != H5I_BADID ){
      XdmfDebug("Closing Current Group");
      H5Gclose(this->Cwd);
      this->Cwd = H5I_BADID;
    }
    if ( this->Dataset != H5I_BADID ){
      XdmfDebug("Closing Dataset");
      H5Dclose(this->Dataset);
      this->Dataset = H5I_BADID;
    }

    if (this->File != H5I_BADID ) {
      XdmfDebug("Closing File");
      H5Fclose(this->File);  
      this->File = H5I_BADID;
    }
  } H5E_END_TRY;

  return( XDMF_SUCCESS );
}

XdmfInt32
XdmfHDF::CreateDataset( XdmfConstString path ) {

  XdmfString  Pathname, Slash;
  hid_t    Directory;

  if( path ) {
    XdmfConstString lastcolon;

    XdmfDebug("CreateDataset Creating  " << path);
    // Skip Colons
    lastcolon = strrchr( path, ':' );
    if( lastcolon != NULL ){
      path = lastcolon;
      path++;
    }
    XdmfDebug("Setting Path to " << path);
    this->SetPath( path );
  }else {
    XdmfDebug("CreateDataset passed NULL path");
  }
  XdmfDebug( "Creating HDF Dataset " <<
    this->Path << "  Rank = " << this->GetRank() );

  // Check That Directory Exists
  Pathname = strdup( this->Path );
  Slash = strrchr( Pathname, '/' );
  if( Slash != NULL ){
    XdmfString  TmpSlash;


    *Slash = '\0';
    TmpSlash = Pathname;

    // This is no necessarily an error
    H5E_BEGIN_TRY {
#if (!H5_USE_16_API && ((H5_VERS_MAJOR>1)||((H5_VERS_MAJOR==1)&&(H5_VERS_MINOR>=8))))
      Directory = H5Gopen( this->Cwd, Pathname, H5P_DEFAULT );
#else
      Directory = H5Gopen( this->Cwd, Pathname );
#endif
    } H5E_END_TRY;
    if( Directory < 0 ){
      // Create All Subdirectories
      *Slash = '/';
      if( Directory > 0  ) {
        H5Gclose( Directory );
      }
      XdmfDebug("Creating Subdirectories ...");
      if( *TmpSlash == '/' ){
        // Skip Leading Slash
        TmpSlash++;
      }
      while( TmpSlash <= Slash ){
        if( *TmpSlash == '/' ){
          *TmpSlash = '\0';
          H5E_BEGIN_TRY {
#if (!H5_USE_16_API && ((H5_VERS_MAJOR>1)||((H5_VERS_MAJOR==1)&&(H5_VERS_MINOR>=8))))
            Directory = H5Gopen( this->Cwd, Pathname, H5P_DEFAULT );
#else
            Directory = H5Gopen( this->Cwd, Pathname );
#endif
          } H5E_END_TRY;
          if( Directory < 0 ){
            XdmfDebug("Creating Directory" << Pathname );
#if (!H5_USE_16_API && ((H5_VERS_MAJOR>1)||((H5_VERS_MAJOR==1)&&(H5_VERS_MINOR>=8))))
            Directory = H5Gcreate( this->Cwd, Pathname, 0, H5P_DEFAULT, H5P_DEFAULT);
#else
            Directory = H5Gcreate( this->Cwd, Pathname, 0);
#endif
            if( Directory < 0 ){
              XdmfErrorMessage("Can't Create " << Pathname );
              return( XDMF_FAIL ); 
            }
            H5Gclose( Directory );
          } else {
            XdmfDebug(Pathname << " Already Exists");
            H5Gclose( Directory );
          }
          *TmpSlash = '/';
        }
        TmpSlash++;
      }
    } else {
      H5Gclose( Directory );
    }
  }

  free( Pathname );
  XdmfDebug("Checking for existance of " << this->Path );
  if ( this->Dataset != H5I_BADID ){
    XdmfDebug("Closing Dataset");
    H5Dclose(this->Dataset);
    this->Dataset=H5I_BADID;
    }
  H5E_BEGIN_TRY {
#if (!H5_USE_16_API && ((H5_VERS_MAJOR>1)||((H5_VERS_MAJOR==1)&&(H5_VERS_MINOR>=8))))
    this->Dataset = H5Dopen( this->Cwd, this->Path, H5P_DEFAULT );
#else
    this->Dataset = H5Dopen( this->Cwd, this->Path );
#endif
  } H5E_END_TRY;
  if( this->Dataset < 0 ) {
    if(this->Compression <= 0){
      XdmfDebug("Creating New Contiguous Dataset");
#if (!H5_USE_16_API && ((H5_VERS_MAJOR>1)||((H5_VERS_MAJOR==1)&&(H5_VERS_MINOR>=8))))
      this->Dataset = H5Dcreate(this->Cwd,
        this->Path,
        this->GetDataType(),
        this->GetDataSpace(),
        H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
#else
      this->Dataset = H5Dcreate(this->Cwd,
        this->Path,
        this->GetDataType(),
        this->GetDataSpace(),
        H5P_DEFAULT);
#endif
    }else{
      hid_t  Prop;
      hsize_t ChunkDims[XDMF_MAX_DIMENSION];
      XdmfInt64 DataDims[XDMF_MAX_DIMENSION];
      int    compression;
      int    i, NDims, NCDims;

      XdmfDebug("Creating New Compressed Dataset");
      NDims = this->GetShape(DataDims);
      if (NDims == 1) {
        // Special Case
        NCDims = 1;
        if(DataDims[0] > 10000) {
          ChunkDims[0] = 100; 
        } else {
          ChunkDims[0] = 1000; 
        }
      } else {
        NCDims = NDims;
        ChunkDims[0] = 1;
        for(i=1 ; i < NDims ; i++) {
          ChunkDims[i] = DataDims[i];
        }
      }
      Prop = H5Pcreate(H5P_DATASET_CREATE);
      H5Pset_chunk(Prop, NCDims, ChunkDims);
      compression = MIN(this->Compression, 9);
      XdmfDebug("Compression Level = " << compression);
      H5Pset_deflate(Prop, compression);
#if (!H5_USE_16_API && ((H5_VERS_MAJOR>1)||((H5_VERS_MAJOR==1)&&(H5_VERS_MINOR>=8))))
      this->Dataset = H5Dcreate(this->Cwd,
        this->Path,
        this->GetDataType(),
        this->GetDataSpace(),
        Prop, H5P_DEFAULT, H5P_DEFAULT);
#else
      this->Dataset = H5Dcreate(this->Cwd,
        this->Path,
        this->GetDataType(),
        this->GetDataSpace(),
        Prop);
#endif
    }
  } else {
    XdmfDebug("Dataset Exists");
    this->CopyType( H5Dget_type( this->Dataset ) );
    this->CopyShape( H5Dget_space( this->Dataset ) );
  }
  if( this->Dataset < 0 ){
    return( XDMF_FAIL );
  }
  return( XDMF_SUCCESS );
}


XdmfInt32
XdmfHDF::OpenDataset() {


  if( this->Dataset > 0 ) {
    // There is a currently open Dataset
    H5Dclose(this->Dataset);
  }

#if (!H5_USE_16_API && ((H5_VERS_MAJOR>1)||((H5_VERS_MAJOR==1)&&(H5_VERS_MINOR>=8))))
  this->Dataset = H5Dopen(this->Cwd, this->Path, H5P_DEFAULT);
#else
  this->Dataset = H5Dopen(this->Cwd, this->Path);
#endif
  if( this->Dataset < 0 ){
    XdmfErrorMessage("Cannot find dataset " << this->Cwd <<
      "/" << this->Path );
    return( XDMF_FAIL );  
  }
  this->CopyType( H5Dget_type( this->Dataset ) );
  this->CopyShape( H5Dget_space( this->Dataset ) );

  return( XDMF_SUCCESS );
}

XdmfArray *
XdmfHDF::DoRead( XdmfArray *Array ) {

  herr_t      status;
  XDMF_HDF5_SIZE_T  src_npts, dest_npts;

  if ( Array == NULL ){
    Array = new XdmfArray;
    Array->CopyType( this->GetDataType() );
    if( this->GetNumberOfElements() == this->GetSelectionSize() ) {
      Array->CopyShape( this->GetDataSpace() );
    } else {
      Array->SetNumberOfElements( this->GetSelectionSize() );
    }
  }

  if( Array->GetDataPointer() == NULL ){
    XdmfErrorMessage("Memory Object Array has no data storage");
    return( NULL );
  }

  src_npts = H5Sget_select_npoints( this->GetDataSpace() );
  dest_npts = H5Sget_select_npoints( Array->GetDataSpace() );
  if( src_npts != dest_npts ) {
    XdmfErrorMessage("Source and Target Spaces specify different sizes");
    XdmfErrorMessage("Source = " << XDMF_64BIT_CAST(src_npts) << " items");
    XdmfErrorMessage("Target = " << XDMF_64BIT_CAST(dest_npts) << " items");
    return( NULL );
  } else {
    XdmfDebug("Reading " << XDMF_64BIT_CAST(src_npts) << " items");
  }

  status = H5Dread( this->Dataset,
    Array->GetDataType(),
    Array->GetDataSpace(),
    this->GetDataSpace(),
    H5P_DEFAULT,
    Array->GetDataPointer() );

  if ( status < 0 ) {
    return( NULL );
  }
  return( Array );
}
//----------------------------------------------------------------------------
XdmfInt32
XdmfHDF::DoWrite( XdmfArray *Array ) {

  herr_t    status;
  XDMF_HDF5_SIZE_T src_npts, dest_npts;

  if ( Array == NULL ) {
    XdmfErrorMessage("No Array to Write");
    return(XDMF_FAIL);
  }
  if( Array->GetDataPointer() == NULL ) {
    XdmfErrorMessage("Memory Object Array has no data storage");
    return(XDMF_FAIL);
  }
  if( this->Dataset == H5I_BADID ) {
    XdmfDebug("Attempt Create");
    this->CopyType( Array );
    this->CopyShape( Array );
    if( this->CreateDataset() != XDMF_SUCCESS ) {
      XdmfErrorMessage("Unable to Create Dataset");
      return(XDMF_FAIL);
    }
  }

  src_npts = H5Sget_select_npoints( this->GetDataSpace() );
  dest_npts = H5Sget_select_npoints( Array->GetDataSpace() );
  if( src_npts != dest_npts ) {
    XdmfErrorMessage("Source and Target Spaces specify different sizes for path: " << this->Path);
    XdmfErrorMessage("Source = " << XDMF_64BIT_CAST(src_npts) << " items");
    XdmfErrorMessage("Target = " << XDMF_64BIT_CAST(dest_npts) << " items");
    return(XDMF_FAIL);
  } else {
    XdmfDebug("Writing " << XDMF_64BIT_CAST(src_npts) << " items to " << Array->GetHeavyDataSetName());
  }

  status = H5Dwrite( this->Dataset,
    Array->GetDataType(),
    Array->GetDataSpace(),
    this->GetDataSpace(),
    H5P_DEFAULT,
    Array->GetDataPointer() );

  if ( status < 0 ) {
    return(XDMF_FAIL);
  }
  return(XDMF_SUCCESS);
}


XdmfInt32
XdmfHDF::DoOpen( XdmfConstString DataSetName, XdmfConstString access ) {

  // XdmfString Domain, *File, *Path;
  XdmfString lastcolon;
  XdmfString firstcolon;
  XdmfInt32  status, flags = H5F_ACC_RDWR;
  XdmfInt32  AllowCreate = 0;
  ostrstream FullFileName;

  if( DataSetName != NULL ) {
    XdmfString NewName = NULL;
    NewName = strdup( DataSetName );
    // Get Parts from Fulll Name
    //   Start from the back
    lastcolon = strrchr( NewName, ':' );
    firstcolon = strchr( NewName, ':' );

    if( ( lastcolon == NULL ) && ( firstcolon == NULL ) ){
      // No : in name so "name" is a Dataset
      XdmfDebug("No Colons in HDF Filename");
      strcpy(this->Path, NewName );
    } else if( lastcolon != firstcolon ) {
      // Two :'s 
      // This is a full name
      *lastcolon = '\0';
      lastcolon++;
      strcpy(this->Path, lastcolon );
      *firstcolon = '\0';
      firstcolon++;
      // strcpy(this->FileName, firstcolon );
      this->SetFileName(firstcolon);
      strcpy(this->Domain, NewName);
      XdmfDebug("Two Colons -  Full HDF Filename Domain : " <<
        this->Domain << " File " <<
        this->FileName);
    } else {
      // One :
      //  
      *firstcolon = '\0';
      firstcolon++;
      if ( ( STRCASECMP( NewName, "FILE" ) == 0 ) ||
        ( STRCASECMP( NewName, "GASS" ) == 0 ) ||
        ( STRCASECMP( NewName, "CORE" ) == 0 ) ||
        ( STRCASECMP( NewName, "DUMMY" ) == 0 ) ) {
          // Domain::File
          strcpy( this->Domain, NewName);
          // strcpy( this->FileName, firstcolon );
          this->SetFileName(firstcolon);
          XdmfDebug("Two Colons -  Domain : " <<
            this->Domain << " File " <<
            this->FileName);
      } else {
        // File:Path
        // strcpy( this->FileName, NewName);
        this->SetFileName(NewName);
        strcpy( this->Path, firstcolon );
        XdmfDebug("Two Colons -  File : " <<
          this->FileName << " Path " <<
          this->Path);
      }

    }
    if( NewName ) free( NewName );
  }

  if ( access != NULL ){
    strcpy( this->Access, access );
  }

  if( STRCASECMP( this->Access, "RW" ) == 0 ) {
    // Read, Write, Create
    flags = H5F_ACC_RDWR;
    AllowCreate = 1;
  } else if( STRCASECMP( this->Access, "WR" ) == 0 ) {
    // Read, Write, Create
    flags = H5F_ACC_RDWR;
    AllowCreate = 1;
  } else if( STRCASECMP( this->Access, "R+" ) == 0 ) {
    // Read, Write
    flags = H5F_ACC_RDWR;
  } else if( STRCASECMP( this->Access, "W+" ) == 0 ) {
    // Read, Write
    flags = H5F_ACC_RDWR | H5F_ACC_TRUNC;
  } else if( STRCASECMP( this->Access, "W" ) == 0 ) {
    // Read, Write, Create
    flags = H5F_ACC_RDWR | H5F_ACC_TRUNC;
    AllowCreate = 1;
  } else if( STRCASECMP( this->Access, "R" ) == 0 ) {
    // Read
    flags = H5F_ACC_RDONLY;
  } else {
    flags = H5F_ACC_RDONLY;
  }


    XdmfDebug("Using Domain " << this->Domain );
    if( STRCASECMP( this->Domain, "CORE" ) == 0 ) {
      XdmfDebug("Using CORE Interface");  
      if( this->AccessPlist != H5P_DEFAULT ) {
        H5Pclose( this->AccessPlist );
      }
      this->AccessPlist = H5Pcreate( H5P_FILE_ACCESS );
      //      H5Pset_fapl_core( this->AccessPlist, 1000000, 1 );
      H5Pset_fapl_core( this->AccessPlist, 1000000, 0 );
    } else if( STRCASECMP( this->Domain, "DSM" ) == 0 ) {
      XdmfDebug("Using DSM Interface");  
      if(!this->DsmBuffer){
        XdmfErrorMessage("Cannot Open a DSM HDF5 File Until DsmBuffer has been set");
        return(XDMF_FAIL);
      }
      H5FD_dsm_init();
      this->AccessPlist = H5Pcreate( H5P_FILE_ACCESS );
      XdmfDebug("DsmBuffer = " << this->DsmBuffer);
      H5Pset_fapl_dsm( this->AccessPlist, H5FD_DSM_INCREMENT, this->DsmBuffer);
    } else if( STRCASECMP( this->Domain, "NDGM" ) == 0 ) {
      XdmfErrorMessage("NDGM Interface is unavailable");
      return( XDMF_FAIL );  
    } else if( STRCASECMP( this->Domain, "GASS" ) == 0 ) {
    } else {
      // Check for Parallel HDF5 ... MPI must already be initialized
#if H5_HAVE_PARALLEL && ((H5_VERS_MAJOR>1)||((H5_VERS_MAJOR==1)&&(H5_VERS_MINOR>=6)))
      if((!this->UseSerialFile) && (STRCASECMP( this->Domain, "SERIAL" ) != 0 )) {
        XdmfDebug("Using Parallel File Interface, Path = " << this->GetWorkingDirectory() );

        this->AccessPlist = H5Pcreate( H5P_FILE_ACCESS );
        H5Pset_fapl_mpio(this->AccessPlist, MPI_COMM_WORLD, MPI_INFO_NULL);
      }else{
        XdmfDebug("Using Serial File Interface (Specified in DOMAIN), Path = " << this->GetWorkingDirectory() );
      }

#else
      XdmfDebug("Using Serial File Interface (Parallel Not Available), Path = " << this->GetWorkingDirectory() );
#endif
      if( ( strlen( this->GetWorkingDirectory() ) > 0 ) && 
        ( this->FileName[0] != '/' ) ){
          FullFileName << this->GetWorkingDirectory() << "/";
      }
    }

  FullFileName << this->FileName << ends;

  // printf("Opening %s flags 0x%X\n", this->FileName, flags );
  // Turn of Errors if Creation is Allowed
    if( AllowCreate ) {
      H5E_BEGIN_TRY {
        this->File = H5Fopen(FullFileName.str(), flags, this->AccessPlist);
      } H5E_END_TRY;
    } else {
      this->File = H5Fopen(FullFileName.str(), flags, this->AccessPlist);
    }
XdmfDebug("this->File = " << this->File);
  FullFileName.rdbuf()->freeze(0);
  if( this->File < 0 ){
    XdmfDebug("Open failed, Checking for Create");
    if( AllowCreate ) {
      // File Doesn't Exist
      // So Create it and Return
      if( STRCASECMP( this->Domain, "CORE" ) == 0 ) {
        XdmfDebug("Using CORE Interface");  
        if( this->AccessPlist != H5P_DEFAULT ) {
          H5Pclose( this->AccessPlist );
        }
        this->AccessPlist = H5Pcreate( H5P_FILE_ACCESS );
        //      H5Pset_fapl_core( this->AccessPlist, 1000000, 1);
        H5Pset_fapl_core( this->AccessPlist, 1000000, 0);
      } else if( STRCASECMP( this->Domain, "DSM" ) == 0 ) {
        if(!this->DsmBuffer){
          XdmfErrorMessage("Cannot Open a DSM HDF5 File Until DsmBuffer has been set");
          return(XDMF_FAIL);
        }
        H5FD_dsm_init();
        this->AccessPlist = H5Pcreate( H5P_FILE_ACCESS );
        H5Pset_fapl_dsm( this->AccessPlist, H5FD_DSM_INCREMENT, this->DsmBuffer);
      } else if( STRCASECMP( this->Domain, "NDGM" ) == 0 ) {
        XdmfErrorMessage("NDGM interface is unavailable");
        return( XDMF_FAIL );
      } else if( STRCASECMP( this->Domain, "FILE" ) == 0 ) {
      }
      this->File = H5Fcreate(FullFileName.str(),
        H5F_ACC_TRUNC,
        this->CreatePlist,
        this->AccessPlist);
      FullFileName.rdbuf()->freeze(0);
      if( this->File < 0 ){
        XdmfErrorMessage( "Cannot create " << this->GetFileName() );
        return( XDMF_FAIL );  
      }
    } else {
      XdmfErrorMessage( "Cannot open " << this->GetFileName() << " / " << FullFileName.str() );
      FullFileName.rdbuf()->freeze(0);
      return( XDMF_FAIL );  
    }
  }
#if (!H5_USE_16_API && ((H5_VERS_MAJOR>1)||((H5_VERS_MAJOR==1)&&(H5_VERS_MINOR>=8))))
    this->Cwd = H5Gopen(this->File, "/", H5P_DEFAULT);
#else
    this->Cwd = H5Gopen(this->File, "/");
#endif
    XdmfDebug("File Open at /");

  status = XDMF_SUCCESS;
  if( this->Path[0] != '\0' ){
    XdmfInt32  Type;

    Type = Info( this->Cwd, this->Path );
    switch( Type ) {
    case H5G_GROUP :
      XdmfDebug("Attempt Cd to Path " << this->Path );
      status = this->SetCwdName( this->Path );
      break;
    case H5G_DATASET :
      XdmfDebug("Attempt OpenDataset of Path " << this->Path );
      status = this->OpenDataset();
      break;
    default :
      XdmfDebug( "H5 Data " <<
        this->Path <<
        " does not exist");
      status = XDMF_FAIL;
      if(AllowCreate){
        XdmfDebug("Attempt to Create Dataset : " << this->Path);
        status = this->CreateDataset();
      }
      break;

    }

  }

  return( status );
}

XdmfArray *CopyArray( XdmfArray *Source, XdmfArray *Target ) {

  XdmfString  H5Name;
  XdmfHDF    Hdf;
  XdmfArray  *NewArray = NULL;
  ostrstream str;

  if( Target == NULL ){
    NewArray = Target = new XdmfArray( Source->GetNumberType() );
    Target->SetNumberOfElements( Source->GetSelectionSize() );
  }
  // Build a Unique Name
  H5Name = GetUnique( "CORE:XdmfJunk" );
  str << H5Name;
  str << ".h5:/TempData" << ends;
  Hdf.CopyType( Source );
  if( Source->GetSelectionSize() == Source->GetNumberOfElements() ) {
    Hdf.CopyShape( Source );
  } else {
    XdmfInt64  Dimensions[2];

    Dimensions[0] = Source->GetSelectionSize();
    Hdf.SetShape( 1, Dimensions );
  }
Hdf.Open( str.str(), "rw" );
  if( Hdf.CreateDataset( str.str()) != XDMF_SUCCESS ){
    XdmfErrorMessage("Can't Create Temp Dataset " << str.str());
    str.rdbuf()->freeze(0);
    if( NewArray ){
      delete NewArray;
    }
    Hdf.Close();
    return( NULL );
  }
  str.rdbuf()->freeze(0);
  if( Hdf.Write( Source ) == XDMF_FAIL){
    XdmfErrorMessage("Can't Write Temp Dataset");
    if( NewArray ){
      delete NewArray;
    }
  Hdf.Close();
    return( NULL );
  }
  if( Hdf.Read( Target ) == NULL ){
    XdmfErrorMessage("Can't Read Temp Dataset");
    if( NewArray ){
      delete NewArray;
    }
  Hdf.Close();
    return( NULL );
  }
Hdf.Close();
  return( Target );
}
/*
XdmfArray *
CreateArrayFromType( XdmfType *Type, XdmfInt64 NumberOfElements ) {

XdmfArray    *array;
XdmfCompoundArray  *carray;
XdmfInt32         type;


type = HDF5TypeToXdmfType( Type->GetType() );  
switch ( type ) {
case XDMF_INT8_TYPE :
array = new XdmfInt8Array;
break;
case XDMF_INT32_TYPE :
array = new XdmfInt32Array;
break;
case XDMF_INT64_TYPE :
array = new XdmfInt64Array;
break;
case XDMF_FLOAT32_TYPE :
array = new XdmfFloat32Array;
break;
case XDMF_FLOAT64_TYPE :
array = new XdmfFloat64Array;
break;
case XDMF_COMPOUND_TYPE :
carray = new XdmfCompoundArray;
carray->SetPrecision( Type->GetSize() );  
array = (XdmfArray *)carray;
break;
default :
XdmfErrorMessage( " Unknown Datatype " );
return( NULL );
}
array->SetNumberOfElements(NumberOfElements);
return( array );
}
*/

}
