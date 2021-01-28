/*============================================================================
  MetaIO
  Copyright 2000-2010 Insight Software Consortium

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
#include "metaTypes.h"

#ifndef ITKMetaIO_METAFORM_H
#  define ITKMetaIO_METAFORM_H

#  include "metaUtils.h"
#  include "metaEvent.h"

#  ifdef _MSC_VER
#    pragma warning(disable : 4251)
#  endif

#  if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE
{
#  endif

class METAIO_EXPORT MetaForm
{
public:
  MetaForm();
  explicit MetaForm(const char * _fileName);

  virtual ~MetaForm();

  virtual void
  PrintInfo() const;

  virtual void
  CopyInfo(const MetaForm * _form);

  virtual void
  Clear();

  void
  ClearFields();

  static bool
  InitializeEssential();

  const char *
  FileName() const;
  void
  FileName(const char * _fileName);


  //    Comment(...)
  //       Optional Field
  //       Arbitrary string
  const char *
  Comment() const;
  void
  Comment(const char * _comment);

  //     FormTypeName()
  //       The intended type: vector, co-vector, matrix....
  const char *
  FormTypeName() const;
  void
  FormTypeName(const char * _formTypeName);

  //    Name(...)
  //       Optional Field
  //       Name of the current metaForm
  const char *
  Name() const;
  void
  Name(const char * _name);

  bool
  BinaryData() const;
  void
  BinaryData(bool _binaryData);

  bool
  BinaryDataByteOrderMSB() const;
  void
  BinaryDataByteOrderMSB(bool _elementByteOrderMSB);

  bool
  CompressedData() const;
  void
  CompressedData(bool _compressedData);

  // Get/Set the double precision for writing
  unsigned int
  DoublePrecision() const;
  unsigned int
  GetDoublePrecision() const
  {
    return this->DoublePrecision();
  }

  void
  DoublePrecision(unsigned int _doublePrecision);
  void
  SetDoublePrecision(unsigned int _doublePrecision)
  {
    this->DoublePrecision(_doublePrecision);
  }

  MetaEvent *
  Event();
  MetaEvent *
  GetEvent()
  {
    return Event();
  }

  void
  Event(MetaEvent * _event);
  void
  SetEvent(MetaEvent * _event)
  {
    Event(_event);
  }

  void
  ClearUserFields();

  void *
  GetUserField(const char * _name);

  template <class TType>
  bool
  AddUserField(const char *      _fieldName,
               MET_ValueEnumType _type,
               int               _length,
               TType *           _v,
               bool              _required = true,
               int               _dependsOn = -1)
  {
    auto * mFw = new MET_FieldRecordType;
    MET_InitWriteField(mFw, _fieldName, _type, static_cast<size_t>(_length), _v);
    m_UserDefinedWriteFields.push_back(mFw);

    auto * mFr = new MET_FieldRecordType;
    MET_InitReadField(mFr, _fieldName, _type, _required, _dependsOn, static_cast<size_t>(_length));
    m_UserDefinedReadFields.push_back(mFr);

    return true;
  }

  static bool
  CanRead(const char * _fileName = nullptr) ;

  bool
  Read(const char * _fileName = nullptr);

  static bool
  CanReadStream(std::ifstream * _stream) ;

  bool
  ReadStream(std::ifstream * _stream);

  bool
  Write(const char * _fileName = nullptr);

  bool
  WriteStream(std::ofstream * _stream);

  // PROTECTED
protected:
  typedef std::vector<MET_FieldRecordType *> FieldsContainerType;

  std::ifstream * m_ReadStream;
  std::ofstream * m_WriteStream;

  std::string m_FileName;

  char m_Comment[255]{};

  char m_FormTypeName[255]{};

  char m_Name[255]{};

  bool m_BinaryData{};
  bool m_BinaryDataByteOrderMSB{};

  bool m_CompressedData{};

  unsigned int m_DoublePrecision;

  MetaEvent * m_Event;

  FieldsContainerType m_Fields;
  FieldsContainerType m_UserDefinedWriteFields;
  FieldsContainerType m_UserDefinedReadFields;

  // protected functions

  static void
  M_Destroy();

  virtual void
  M_SetupReadFields();

  virtual void
  M_SetupWriteFields();

  virtual bool
  M_Read();

  virtual bool
  M_Write();
};

#  if (METAIO_USE_NAMESPACE)
};
#  endif

#endif
