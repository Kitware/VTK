#include <climits>

// ----------------------------------------------------------------------

// First we have several helper functions that will determine what
// type we're actually dealing with.  With any luck the compiler will
// inline these so they have very little overhead.

inline bool 
IsSigned64Bit(int VariantType)
{
#if defined(VTK_TYPE_USE_LONG_LONG) && defined(VTK_TYPE_USE___INT64)
  return ((VariantType == VTK_LONG_LONG) ||
          (VariantType == VTK___INT64) ||
          (VariantType == VTK_TYPE_INT64));
#elsif defined(VTK_TYPE_USE_LONG_LONG)
  return ((VariantType == VTK_LONG_LONG) ||
          (VariantType == VTK_TYPE_INT64));
#elsif defined(VTK_TYPE_USE___INT64)
  return ((VariantType == VTK___INT64) ||
          (VariantType == VTK_TYPE_INT64));
#else
  return (VariantType == VTK_TYPE_INT64);
#endif
}
    
inline bool 
IsSigned(int VariantType)
{
#if (CHAR_MIN == SCHAR_MIN && CHAR_MAX == SCHAR_MAX)
// the char type is signed on this compiler
  return ((VariantType == VTK_CHAR) ||
          (VariantType == VTK_SIGNED_CHAR) ||
          (VariantType == VTK_SHORT) ||
          (VariantType == VTK_INT) ||
          (VariantType == VTK_LONG) ||
          (VariantType == VTK_ID_TYPE) ||
          IsSigned64Bit(VariantType));
#else 
  // char is unsigned
  return ((VariantType == VTK_SIGNED_CHAR) ||
          (VariantType == VTK_SHORT) ||
          (VariantType == VTK_INT) ||
          (VariantType == VTK_LONG) ||
          (VariantType == VTK_ID_TYPE) ||
          IsSigned64Bit(VariantType));
#endif
}
  
// ----------------------------------------------------------------------

inline bool
IsFloatingPoint(int VariantType)
{
  return ((VariantType == VTK_FLOAT) ||
          (VariantType == VTK_DOUBLE));
}

// ----------------------------------------------------------------------

inline bool
CompareSignedUnsignedEqual(const vtkVariant &SignedVariant,
                           const vtkVariant &UnsignedVariant)
{
  // If the signed value is less than zero then they cannot possibly
  // be equal.
  vtkTypeInt64 A = SignedVariant.ToTypeInt64();
  return (A >= 0) && (A == UnsignedVariant.ToTypeInt64());
}

// ----------------------------------------------------------------------

inline bool
CompareSignedUnsignedLessThan(const vtkVariant &SignedVariant,
                              const vtkVariant &UnsignedVariant)
{
  vtkTypeInt64 A = SignedVariant.ToTypeInt64();
  return ((A < 0) || 
          (static_cast<vtkTypeUInt64>(A) < UnsignedVariant.ToTypeUInt64()));
}

// ----------------------------------------------------------------------

inline bool
CompareUnsignedSignedLessThan(const vtkVariant &UnsignedVariant,
                              const vtkVariant &SignedVariant)
{
  vtkTypeInt64 B = SignedVariant.ToTypeInt64();
  return ((B > 0) && 
          (UnsignedVariant.ToTypeUInt64() < static_cast<vtkTypeUInt64>(B)));
}

// ----------------------------------------------------------------------

inline bool
CompareSignedLessThan(const vtkVariant &A,
                      const vtkVariant &B)
{
  return (A.ToTypeInt64() < B.ToTypeInt64());
}

// ----------------------------------------------------------------------

inline bool
CompareUnsignedLessThan(const vtkVariant &A,
                        const vtkVariant &B)
{
  return (A.ToTypeUInt64() < B.ToTypeUInt64());
}

// ----------------------------------------------------------------------

inline bool
vtkVariant::operator==(const vtkVariant &other) const
{
  // First test: NULL values are always equal to one another and
  // unequal to anything else.
  if (! (this->Valid && other.Valid))
    {
    return (!(this->Valid || other.Valid));
    }
  
  // Second test: VTK objects can only be compared with other VTK
  // objects.
  if ((this->Type == VTK_OBJECT) || (other.Type == VTK_OBJECT))
    {
    return ((this->Type == VTK_OBJECT) &&
            (other.Type == VTK_OBJECT) &&
            (this->Data.VTKObject == other.Data.VTKObject));
    }

  // Third test: the STRING type dominates all else.  If either item
  // is a string then they must both be compared as strings.
  if ((this->Type == VTK_STRING) ||
      (other.Type == VTK_STRING))
    {
    return (this->ToString() == other.ToString());
    }

  // Fourth test: the Unicode STRING type dominates all else.  If either item
  // is a unicode string then they must both be compared as strings.
  if ((this->Type == VTK_UNICODE_STRING) ||
      (other.Type == VTK_UNICODE_STRING))
    {
    return (this->ToUnicodeString() == other.ToUnicodeString());
    }


  // Fifth: floating point dominates integer types.
  if (IsFloatingPoint(this->Type) || IsFloatingPoint(other.Type))
    {
    return (this->ToDouble() == other.ToDouble());
    }

  // Sixth: we must be comparing integers.  

  // 6A: catch signed/unsigned comparison.  If the signed object is
  // less than zero then they cannot be equal.
  bool thisSigned = IsSigned(this->Type);
  bool otherSigned = IsSigned(other.Type);

  if (thisSigned ^ otherSigned)
    {
    if (thisSigned)
      {
      return CompareSignedUnsignedEqual(*this, other);
      }
    else
      {
      return CompareSignedUnsignedEqual(other, *this);
      }
    }
  else // 6B: both are signed or both are unsigned.  In either event
       // all we have to do is check whether the bit patterns are
       // equal.
    {
    return (this->ToTypeInt64() == other.ToTypeInt64());
    }
}
    
// ----------------------------------------------------------------------

inline bool
vtkVariant::operator<(const vtkVariant &other) const
{
  // First test: a NULL value is less than anything except another
  // NULL value.  unequal to anything else.
  if (! (this->Valid && other.Valid))
    {
    return ((!this->Valid) && (other.Valid));
    }
  
  // Second test: VTK objects can only be compared with other VTK
  // objects.
  if ((this->Type == VTK_OBJECT) || (other.Type == VTK_OBJECT))
    {
    return ((this->Type == VTK_OBJECT) &&
            (other.Type == VTK_OBJECT) &&
            (this->Data.VTKObject < other.Data.VTKObject));
    }

  // Third test: the STRING type dominates all else.  If either item
  // is a string then they must both be compared as strings.
  if ((this->Type == VTK_STRING) ||
      (other.Type == VTK_STRING))
    {
    return (this->ToString() < other.ToString());
    }

  // Fourth test: the Unicode STRING type dominates all else.  If either item
  // is a unicode string then they must both be compared as strings.
  if ((this->Type == VTK_UNICODE_STRING) ||
      (other.Type == VTK_UNICODE_STRING))
    {
    return (this->ToUnicodeString() < other.ToUnicodeString());
    }

  // Fourth: floating point dominates integer types.
  if (IsFloatingPoint(this->Type) || IsFloatingPoint(other.Type))
    {
    return (this->ToDouble() < other.ToDouble());
    }

  // Fifth: we must be comparing integers.  

  // 5A: catch signed/unsigned comparison.  If the signed object is
  // less than zero then they cannot be equal.
  bool thisSigned = IsSigned(this->Type);
  bool otherSigned = IsSigned(other.Type);

  if (thisSigned ^ otherSigned)
    {
    if (thisSigned)
      {
      return CompareSignedUnsignedLessThan(*this, other);
      }
    else
      {
      return CompareUnsignedSignedLessThan(*this, other);
      }
    }
  else if (thisSigned)
    {
    return CompareSignedLessThan(*this, other);
    }
  else
    {
    return CompareUnsignedLessThan(*this, other);
    }
}
 
// ----------------------------------------------------------------------

// Below this point are operators defined in terms of other operators.
// Again, this may sacrifice some speed, but reduces the chance of
// inconsistent behavior.


// ----------------------------------------------------------------------

inline bool
vtkVariant::operator!=(const vtkVariant &other) const
{
  return ! (this->operator==(other));
}

inline bool
vtkVariant::operator>(const vtkVariant &other) const
{
  return (!(this->operator==(other) ||
            this->operator<(other)));
}

inline bool
vtkVariant::operator<=(const vtkVariant &other) const
{
  return (this->operator==(other) ||
          this->operator<(other));
}

inline bool
vtkVariant::operator>=(const vtkVariant &other) const
{
  return (!this->operator<(other));
}

