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

#ifndef ITKMetaIO_METAOBJECT_H
#  define ITKMetaIO_METAOBJECT_H

#  include "metaUtils.h"
#  include "metaEvent.h"

#  include <string>

#  ifdef _MSC_VER
#    pragma warning(disable : 4251)
#  endif


#  if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE
{
#  endif

class METAIO_EXPORT MetaObject
{
  // PROTECTED
protected:
  typedef std::vector<MET_FieldRecordType *> FieldsContainerType;

  std::ifstream * m_ReadStream;
  std::ofstream * m_WriteStream;

  FieldsContainerType m_Fields;
  FieldsContainerType m_UserDefinedWriteFields;
  FieldsContainerType m_UserDefinedReadFields;
  FieldsContainerType m_AdditionalReadFields;

  std::string m_FileName;

  char m_Comment[255]{}; // "Comment = "       ""

  char m_ObjectTypeName[255]{};    // "ObjectType = "    defined by suffix
  char m_ObjectSubTypeName[255]{}; // "ObjectSubType = " defined by suffix

  int m_NDims; // "NDims = "         required

  double m_Offset[10]{};           // "Offset = "          0,0,0
  double m_TransformMatrix[100]{}; // "TransformMatrix = " 1,0,0,0,1,0,0,0,1
  double m_CenterOfRotation[10]{}; // "CenterOfRotation = "  0 0 0

  MET_OrientationEnumType m_AnatomicalOrientation[10]{};
  mutable char            m_OrientationAcronym[10]{};

  MET_DistanceUnitsEnumType m_DistanceUnits; // "DistanceUnits = mm"

  double m_ElementSpacing[10]{}; // "ElementSpacing = "   0,0,0

  float m_Color[4]{}; // "Color = "            1.0, 0.0, 0.0, 1.0

  char m_AcquisitionDate[255]{}; // "AcquisitionDate = "  "2007.03.21"

  int m_ID{}; // "ID = "               0

  int m_ParentID{}; // "ParentID = "         -1

  char m_Name[255]{}; // "Name = "             ""

  bool m_BinaryData{}; // "BinaryData = "      False

  bool m_BinaryDataByteOrderMSB{};

  std::streamoff m_CompressedDataSize{};
  // Used internally to set if the dataSize should be written
  bool m_WriteCompressedDataSize{};
  bool m_CompressedData{};
  int  m_CompressionLevel{};

  static void M_Destroy();

  virtual void
  M_SetupReadFields();

  virtual void
  M_SetupWriteFields();

  virtual bool
  M_Read();

  virtual bool
  M_Write();

  virtual void
  M_PrepareNewReadStream();

  MetaEvent * m_Event;
  // MET_FieldRecordType * M_GetFieldRecord(const char * _fieldName);
  // int   M_GetFieldRecordNumber(const char * _fieldName);

  unsigned int m_DoublePrecision;

  // PUBLIC
public:
  // Constructors & Destructor
  MetaObject();
  explicit MetaObject(const char * _fileName);
  explicit MetaObject(unsigned int dim);

  virtual ~MetaObject();

  void
  FileName(const char * _fileName);
  const char *
  FileName() const;

  virtual void
  CopyInfo(const MetaObject * _object);

  virtual bool
  Read(const char * _fileName = nullptr);

  bool
  ReadStream(int _nDims, std::ifstream * _stream);

  virtual bool
  Write(const char * _fileName = nullptr);

  virtual bool
  Append(const char * _headName = nullptr);

  // Common fields

  //    PrintMetaInfo()
  //       Writes image parameters to stdout
  virtual void
  PrintInfo() const;

  //    Comment(...)
  //       Optional Field
  //       Arbitrary string
  const char *
  Comment() const;
  void
  Comment(const char * _comment);

  const char *
  ObjectTypeName() const;
  void
  ObjectTypeName(const char * _objectTypeName);
  const char *
  ObjectSubTypeName() const;
  void
  ObjectSubTypeName(const char * _objectSubTypeName);

  //    NDims()
  //       REQUIRED Field
  //       Number of dimensions to the image
  int
  NDims() const;

  //    Offset(...)
  //       Optional Field
  //       Physical location (in millimeters and wrt machine coordinate
  //         system or the patient) of the first element in the image
  const double *
  Offset() const;
  double
  Offset(int _i) const;
  void
  Offset(const double * _position);
  void
  Offset(int _i, double _value);
  const double *
  Position() const;
  double
  Position(int _i) const;
  void
  Position(const double * _position);
  void
  Position(int _i, double _value);
  const double *
  Origin() const;
  double
  Origin(int _i) const;
  void
  Origin(const double * _position);
  void
  Origin(int _i, double _value);

  //    TransformMatrix(...)
  //       Optional Field
  //       Physical orientation of the object as an NDims x NDims matrix
  const double *
  TransformMatrix() const;
  double
  TransformMatrix(int _i, int _j) const;
  void
  TransformMatrix(const double * _orientation);
  void
  TransformMatrix(int _i, int _j, double _value);
  const double *
  Rotation() const;
  double
  Rotation(int _i, int _j) const;
  void
  Rotation(const double * _orientation);
  void
  Rotation(int _i, int _j, double _value);
  const double *
  Orientation() const;
  double
  Orientation(int _i, int _j) const;
  void
  Orientation(const double * _orientation);
  void
  Orientation(int _i, int _j, double _value);

  const double *
  CenterOfRotation() const;
  double
  CenterOfRotation(int _i) const;
  void
  CenterOfRotation(const double * _position);
  void
  CenterOfRotation(int _i, double _value);

  const char *
  DistanceUnitsName() const;
  MET_DistanceUnitsEnumType
  DistanceUnits() const;
  void
  DistanceUnits(MET_DistanceUnitsEnumType _distanceUnits);
  void
  DistanceUnits(const char * _distanceUnits);

  const char *
  AnatomicalOrientationAcronym() const;
  const MET_OrientationEnumType *
  AnatomicalOrientation() const;
  MET_OrientationEnumType
  AnatomicalOrientation(int _dim) const;
  void
  AnatomicalOrientation(const char * _ao);
  void
  AnatomicalOrientation(const MET_OrientationEnumType * _ao);
  void
  AnatomicalOrientation(int _dim, MET_OrientationEnumType _ao);
  void
  AnatomicalOrientation(int _dim, char _ao);


  //    ElementSpacing(...)
  //       Optional Field
  //       Physical Spacing (in same units as position)
  const double *
  ElementSpacing() const;
  double
  ElementSpacing(int _i) const;
  void
  ElementSpacing(const double * _elementSpacing);
  void
  ElementSpacing(const float * _elementSpacing);
  void
  ElementSpacing(int _i, double _value);

  //    Name(...)
  //       Optional Field
  //       Name of the current metaObject
  void
  Name(const char * _name);
  const char *
  Name() const;

  //    Color(...)
  //       Optional Field
  //       Color of the current metaObject
  const float *
  Color() const;
  void
  Color(float _r, float _g, float _b, float _a);
  void
  Color(const float * _color);

  //    ID(...)
  //       Optional Field
  //       ID number of the current metaObject
  void
  ID(int _id);
  int
  ID() const;

  //    ParentID(...)
  //       Optional Field
  //       ID number of the parent  metaObject
  void
  ParentID(int _parentId);
  int
  ParentID() const;

  //    AcquisitionDate(...)
  //       Optional Field
  //       YYYY.MM.DD is the recommended format
  void
  AcquisitionDate(const char * _acquisitionDate);
  const char *
  AcquisitionDate() const;

  //    BinaryData(...)
  //       Optional Field
  //       Data is binary or not
  void
  BinaryData(bool _binaryData);
  bool
  BinaryData() const;

  void
  BinaryDataByteOrderMSB(bool _elementByteOrderMSB);
  bool
  BinaryDataByteOrderMSB() const;


  void
  CompressedData(bool _compressedData);
  bool
  CompressedData() const;

  // Compression level 0-9. 0 = no compression.
  void
  CompressionLevel(int _compressionLevel);
  int
  CompressionLevel() const;

  virtual void
  Clear();

  void
  ClearFields();

  void
  ClearAdditionalFields();

  bool
  InitializeEssential(int _nDims);

  // User's field definitions
  bool
  AddUserField(const char *      _fieldName,
               MET_ValueEnumType _type,
               int               _length = 0,
               bool              _required = true,
               int               _dependsOn = -1);

  // find a field record in a field vector
  static MET_FieldRecordType *
  FindFieldRecord(FieldsContainerType & container, const char * fieldName)
  {
    FieldsContainerType::iterator it;
    for (it = container.begin(); it != container.end(); ++it)
    {
      if (strcmp((*it)->name, fieldName) == 0)
      {
        return (*it);
      }
    }
    return nullptr;
  }

  // Add a user's field
  template <class T>
  bool
  AddUserField(const char *      _fieldName,
               MET_ValueEnumType _type,
               int               _length,
               T *               _v,
               bool              _required = true,
               int               _dependsOn = -1)
  {
    // don't add the same field twice. In the unlikely event
    // a field of the same name gets added more than once,
    // over-write the existing FieldRecord
    bool                  duplicate(true);
    MET_FieldRecordType * mFw = MetaObject::FindFieldRecord(m_UserDefinedWriteFields, _fieldName);
    if (mFw == nullptr)
    {
      duplicate = false;
      mFw = new MET_FieldRecordType;
    }
    MET_InitWriteField(mFw, _fieldName, _type, static_cast<size_t>(_length), _v);
    if (!duplicate)
    {
      m_UserDefinedWriteFields.push_back(mFw);
    }

    duplicate = true;
    MET_FieldRecordType * mFr = MetaObject::FindFieldRecord(m_UserDefinedReadFields, _fieldName);
    if (mFr == nullptr)
    {
      duplicate = false;
      mFr = new MET_FieldRecordType;
    }

    MET_InitReadField(mFr, _fieldName, _type, _required, _dependsOn, static_cast<size_t>(_length));
    if (!duplicate)
    {
      m_UserDefinedReadFields.push_back(mFr);
    }
    return true;
  }

  // Clear UserFields
  void
  ClearUserFields();

  // Get the user field
  void *
  GetUserField(const char * _name);

  int
  GetNumberOfAdditionalReadFields();
  char *
  GetAdditionalReadFieldName(int i);
  char *
  GetAdditionalReadFieldValue(int i);
  int
  GetAdditionalReadFieldValueLength(int i);

  void
  SetEvent(MetaEvent * event)
  {
    m_Event = event;
  }

  // Set the double precision for writing
  void
  SetDoublePrecision(unsigned int precision)
  {
    m_DoublePrecision = precision;
  }
  unsigned int
  GetDoublePrecision() const
  {
    return m_DoublePrecision;
  }
};

#  if (METAIO_USE_NAMESPACE)
};
#  endif

#endif
