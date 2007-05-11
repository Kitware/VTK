/*=========================================================================

  Program:   MetaIO
  Module:    metaForm.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#include "metaTypes.h"

#ifndef ITKMetaIO_METAFORM_H
#define ITKMetaIO_METAFORM_H

#include "metaUtils.h"
#include "metaEvent.h"

#ifdef _MSC_VER
#pragma warning ( disable: 4251 )
#endif

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif

class METAIO_EXPORT MetaForm
  {
  public:

    MetaForm(void);
    MetaForm(const char * _fileName);

    virtual ~MetaForm(void);

    virtual void  PrintInfo(void) const;

    virtual void  CopyInfo(const MetaForm * _form);

    virtual void  Clear(void);

    void          ClearFields(void);

    bool  InitializeEssential();

    //
    //
    //
    const char  * FileName(void) const;
    void          FileName(const char *_fileName);


    //    Comment(...)
    //       Optional Field
    //       Arbitrary string
    const char  * Comment(void) const;
    void          Comment(const char * _comment);

    //     FormTypeName()
    //       The intended type: vector, co-vector, matrix....
    const char  * FormTypeName(void) const;
    void          FormTypeName(const char * _formTypeName);

    //    Name(...)
    //       Optional Field
    //       Name of the current metaForm
    const char  * Name(void) const;
    void          Name(const char *_Name);

    //
    //
    //
    bool          BinaryData(void) const;
    void          BinaryData(bool _binaryData);

    bool          BinaryDataByteOrderMSB(void) const;
    void          BinaryDataByteOrderMSB(bool _binaryDataByteOrderMSB);

    //
    //
    //
    bool          CompressedData(void) const;
    void          CompressedData(bool _compressedData);

    //
    // Get/Set the double precision for writing
    //
    unsigned int  DoublePrecision() const;
    unsigned int  GetDoublePrecision() const
                     { return this->DoublePrecision(); }

    void          DoublePrecision(unsigned int _doublePrecision);
    void          SetDoublePrecision(unsigned int _doublePrecision)
                     { this->DoublePrecision(_doublePrecision); }

    //
    //
    //
    MetaEvent *   Event(void);
    MetaEvent *   GetEvent(void)
                     { return Event(); }

    void          Event(MetaEvent * _event);
    void          SetEvent(MetaEvent * _event)
                     { Event(_event); }
    //
    //
    //
    void   ClearUserFields();

    void * GetUserField(const char* _name);

    bool   AddUserField(const char* _fieldName,
                        MET_ValueEnumType _type,
                        int _length=0,
                        bool _required=true,
                        int _dependsOn=-1);

    template <class TType>
    bool   AddUserField(const char* _fieldName,
                        MET_ValueEnumType _type,
                        int _length,
                        TType *_v,
                        bool _required=true,
                        int _dependsOn=-1 )
            {
            MET_FieldRecordType * mFw = new MET_FieldRecordType;
            MET_InitWriteField(mFw, _fieldName, _type, _length, _v);
            m_UserDefinedWriteFields.push_back(mFw);
      
            MET_FieldRecordType * mFr = new MET_FieldRecordType;
            MET_InitReadField(mFr, _fieldName, _type, _required,
                              _dependsOn, _length);
            m_UserDefinedReadFields.push_back(mFr);
      
            return true;
            }

    //
    //
    //
    virtual bool  CanRead(const char * _fileName=NULL) const;

    bool  Read(const char * _fileName=NULL);

    virtual bool  CanReadStream(METAIO_STREAM::ifstream * _stream) const;

    bool  ReadStream(METAIO_STREAM::ifstream * _stream);

    bool  Write(const char * _fileName=NULL);

    bool  WriteStream(METAIO_STREAM::ofstream * _stream);

  ////
  //
  // PROTECTED
  //
  ////
  protected:

    typedef METAIO_STL::vector<MET_FieldRecordType *> FieldsContainerType;

    METAIO_STREAM::ifstream *  m_ReadStream;
    METAIO_STREAM::ofstream *  m_WriteStream;

    char  m_FileName[255];

    char  m_Comment[255];      

    char  m_FormTypeName[255];

    char  m_Name[255];

    bool  m_BinaryData;
    bool  m_BinaryDataByteOrderMSB;

    bool  m_CompressedData;

    unsigned int  m_DoublePrecision;

    MetaEvent   * m_Event;

    FieldsContainerType m_Fields;
    FieldsContainerType m_UserDefinedWriteFields;
    FieldsContainerType m_UserDefinedReadFields;
    
    // protected functions

    virtual void M_Destroy(void);

    virtual void M_SetupReadFields(void);

    virtual void M_SetupWriteFields(void);

    virtual bool M_Read(void);

    virtual bool M_Write(void);

  };

#if (METAIO_USE_NAMESPACE)
};
#endif

#endif
