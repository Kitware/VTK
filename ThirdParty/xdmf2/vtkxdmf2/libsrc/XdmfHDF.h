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
/*     Copyright @ 2002 US Army Research Laboratory                */
/*     All Rights Reserved                                         */
/*     See Copyright.txt or http://www.arl.hpc.mil/ice for details */
/*                                                                 */
/*     This software is distributed WITHOUT ANY WARRANTY; without  */
/*     even the implied warranty of MERCHANTABILITY or FITNESS     */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice   */
/*     for more information.                                       */
/*                                                                 */
/*******************************************************************/
#ifndef __XdmfHDF_h
#define __XdmfHDF_h

#include "XdmfHeavyData.h"

#define XDMF_H5_DIRECTORY  H5G_GROUP
#define XDMF_H5_DATASET    H5G_DATASET
#define XDMF_H5_UNKNOWN    H5G_UNKNOWN
#define XDMF_H5_OTHER    0xFF

class XdmfArray;

//! Class for Accessing HDF5 Data
/*!
This is a convenience object for reading and writing
HDF5 Files. Use this to remain XDMF compliant.
Datasets in HDF5 are specified by :
  Domain:Filename:Pathname
where
  Domain = NDGM | FILE | CORE | GASS
    if Domain is not specified,
    FILE is assumed
  Filename = UNIX style Pathname of HDF5 file
  Pathname = HDF5 Pathname inside HDF5 File

XdmfHDF confines HDF5 to using only HDF5 Groups and
HDF5 Datasets. HDF5 Attributes (Name=Value pairs) are
not used (that function is served by XML). HDF5 Groups
are treated like "Directories" on a UNIX filesystem.
HDF5 Datasets are treated like "Files" on a UNIX
Filesystem.

Example of Createing an HDF5 File :
\code
        XdmfHDF         *H5 = new XdmfHDF();
        XdmfArray       *MyData = new XdmfArray();
        XdmfConstString DataSetNameConst;

        MyData->SetNumberType(XDMF_FLOAT32_TYPE);
        MyData->SetNumberOfElements(100);
        MyData->Generate(0, 99);
        DataSetNameConst = "FILE:TestFile.h5:/TestDataSets/Values1";
        H5->CopyType( MyData );
        H5->CopyShape( MyData );
        H5->Open( DataSetName, "rw" );
        H5->Write( MyData );
        H5->Close();

\endcode

This would create an HDF5 file with one Group (TestDataSets) and one Dataset in
that Group (Values1). The Dataset would be 100 32 bit floating point values
ranging from 0-99.
*/


class XDMF_EXPORT XdmfHDF : public XdmfHeavyData {

public:
  XdmfHDF();
  ~XdmfHDF();

  XdmfConstString GetClassName() { return ( "XdmfHDF" ) ; };

//! Set Compression Level to 0 - 9 . Level <= 0 is Off
/*!
        Compression level refers to the next dataset that is
        created. Once a dataset is created, the compression
        level does not change.

        Compression Levels 1-9 are progressively slower but
        result in much smaller HDF5 files. Compression uses
        the libz compression and "CHUNKS" the HDF5 file
        in the major dimension.
*/
  XdmfSetValueMacro(Compression, XdmfInt32);
//! Get Compression Level
  XdmfGetValueMacro(Compression, XdmfInt32);
//! Use Serial File Interface even if Parallel is Available
  XdmfSetValueMacro(UseSerialFile, XdmfInt32);
//! Get Value of Use Serial
  XdmfGetValueMacro(UseSerialFile, XdmfInt32);
//! Set the current internal HDF "Group" for creation
  XdmfInt32 SetCwdName( XdmfConstString Directory );
//! Get the current internal HDF "Group"
  XdmfGetValueMacro(CwdName, XdmfString );
//! Go to another HDF5 "Group"
  XdmfInt32 Cd( XdmfConstString Directory = "/"  ) {
    return( this->SetCwdName( Directory ) );
    };
//! Create an HDF5 Gourp
  XdmfInt32 Mkdir( XdmfString Name );
//! Get the number of members in the current HDF5 Group
  XdmfGetValueMacro( NumberOfChildren, XdmfInt64);
//! Get the HDF5 Library Version as Major.Minor.Release
  XdmfConstString GetHDFVersion(void);
//! Get the n'th child in the current group
  XdmfConstString GetChild( XdmfInt64 Index ) {
    if ( Index >= this->NumberOfChildren ) {
      return( "" );
    }
    return( this->Child[ Index ] );
    };

//! Internal Call to set the name of the next child in the list
  void SetNextChild( XdmfConstString Name );

//! Internal HDF5 info
  XdmfInt32 Info( hid_t Group, XdmfConstString Name );

//! Get The Type of the Child : Directory, Dataset, ot Other
  XdmfInt32 GetChildType( XdmfInt64 Index ) {
    switch( this->Info( this->Cwd, this->GetChild( Index ) ) ) {
      case H5G_GROUP :
        return ( XDMF_H5_DIRECTORY );
      case H5G_DATASET :
        return ( XDMF_H5_DATASET );
      case XDMF_FAIL :
        return( XDMF_H5_UNKNOWN );
      default :
        break;
      }
  return( XDMF_H5_OTHER );
  };
//! Get The Type of the Child as a String 
  XdmfConstString GetChildTypeAsString( XdmfInt64 Index ) {
    switch( this->GetChildType( Index ) ) {
      case XDMF_H5_DIRECTORY :
        return("XDMF_H5_DIRECTORY");
      case XDMF_H5_DATASET :
        return("XDMF_H5_DATASET");
      case XDMF_H5_UNKNOWN :
        return("XDMF_H5_UNKNOWN");
      }  
  return("XDMF_H5_OTHER");
  };
//! Create a new dataset in the current Group
  XdmfInt32 CreateDataset( XdmfConstString Path = NULL );

//! Open an existing Dataset in a currently open HDF5 file
  XdmfInt32 OpenDataset();
//! Open an HDF5 file and OpenDataset = DataSetName
/*!
    \verbatim
    Access is one of :
        "rw" | "wr" - Open for reading and writing. Create if necessary.
        "r+"        - Open for reading and writing.
        "w"         - Open for Writing. Create if necessary. This will truncate the file.
        "w+"        - Open for Writing. This will truncate the file.
        "r"         - Open for Read Only.
    \endverbatim
*/
  virtual XdmfInt32 DoOpen( 
    XdmfConstString DataSetName,
    XdmfConstString Access );
/*!
Read the curently open dataset into and Array.
*/
  virtual XdmfArray* DoRead( XdmfArray *Array );
/*!
Write to the curently open dataset from and Array.
*/
  virtual XdmfInt32 DoWrite( XdmfArray *Array );

//! Close the HDF5  File
  virtual XdmfInt32 DoClose();

protected:
  hid_t    File;
  hid_t    Cwd;
  hid_t    Dataset;
  hid_t    CreatePlist;
  hid_t    AccessPlist;

  char    CwdName[XDMF_MAX_STRING_LENGTH];
  XdmfInt32  Compression;
  XdmfInt32  UseSerialFile;
  XdmfInt64  NumberOfChildren;
  XdmfString  Child[1024];
};

/*
extern XdmfArray *CreateArrayFromType( XdmfType *Type,
  XdmfInt64 NumberOfElements = 10 );
*/
extern XDMF_EXPORT XdmfArray *CopyArray( XdmfArray *Source, XdmfArray *Target = NULL );

#endif // __XdmfHDF_h
