/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFastNumericConversion.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkFastNumericConversion - Enables fast conversion of floating point to fixed point
// .SECTION Description
// vtkFastNumericConversion uses a portable (assuming IEEE format) method for
// converting single and double precision floating point values to a fixed
// point representation. This allows fast integer floor operations on
// platforms, such as Intel X86, in which CPU floating point conversion
// algorithms are very slow. It is based on the techniques described in Chris
// Hecker's article, "Let's Get to the (Floating) Point", in Game Developer
// Magazine, Feb/Mar 1996, and the techniques described in Michael Herf's
// website, http://www.stereopsis.com/FPU.html.  The Hecker article can be
// found at http://www.d6.com/users/checker/pdfs/gdmfp.pdf.  Unfortunately,
// each of these techniques is incomplete, and doesn't convert properly, in a
// way that depends on how many bits are reserved for fixed point fractional
// use, due to failing to properly account for the default round-towards-even
// rounding mode of the X86. Thus, my implementation incorporates some
// rounding correction that undoes the rounding that the FPU performs during
// denormalization of the floating point value. Note that the rounding affect
// I'm talking about here is not the effect on the fistp instruction, but
// rather the effect that occurs during the denormalization of a value that
// occurs when adding it to a much larger value. The bits must be shifted to
// the right, and when a "1" bit falls off the edge, the rounding mode
// determines what happens next, in order to avoid completely "losing" the
// 1-bit. Furthermore, my implementation works on Linux, where the default
// precision mode is 64-bit extended precision.

// This class is contributed to VTK by Chris Volpe of Applied Research
// Associates, Inc.  (My employer requires me to say that -- CRV)

// This code assumes that the FPU is in round-to-nearest mode. It assumes, on
// Linux, that the default extended precision mode is in effect, and it
// assumes, on Windows, that the default double precision mode is in effect.

#ifndef vtkFastNumericConversion_h
#define vtkFastNumericConversion_h

#include "vtkCommonMathModule.h" // For export macro
#include "vtkObject.h"

// Use the bit-representation trick only on X86, and only when producing
// optimized code
#if defined(NDEBUG) && (defined i386 || defined _M_IX86)
#define VTK_USE_TRICK
#endif

// Linux puts the FPU in extended precision. Windows and FreeBSD keep it in
// double precision.  If other operating systems for i386 (Solaris?) behave
// like Linux, add them below.  Special care needs to be taken when dealing
// with extended precision mode because even though we are eventually writing
// out to a double-precision variable to capture the fixed-point or integer
// results, the extra bits maintained in the internal computations disrupt
// the bit-playing that we're doing here.
#if defined(__linux__)
#define VTK_EXT_PREC
#endif

//#define VTK_TEST_HACK_TO_EMULATE_LINUX_UNDER_WINDOWS
#ifdef VTK_TEST_HACK_TO_EMULATE_LINUX_UNDER_WINDOWS
#define VTK_EXT_PREC
#endif


class VTKCOMMONMATH_EXPORT vtkFastNumericConversion : public vtkObject
{
public:
  static vtkFastNumericConversion *New();
  vtkTypeMacro(vtkFastNumericConversion, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  int TestQuickFloor(double val)
    {
    return vtkFastNumericConversion::QuickFloor(val);
    }

  int TestSafeFloor(double val)
    {
    return vtkFastNumericConversion::SafeFloor(val);
    }

  int TestRound(double val)
    {
    return vtkFastNumericConversion::Round(val);
    }

  int TestConvertFixedPointIntPart(double val)
    {
    int frac;
    return ConvertFixedPoint(val, frac);
    }

  int TestConvertFixedPointFracPart(double val)
    {
    int frac;
    ConvertFixedPoint(val, frac);
    return frac;
    }

protected:
  //BTX
  // Description:
  // Internal use: multiply the denormalizer value by 1.5 to ensure that it
  // has a "1" bit, other than the implicit initial "1" bit, from which to
  // borrow when adding (flooring) a negative number, so that we don't borrow
  // from the implicit "1" bit, which would cause partial re-normalization,
  // resulting in a shift of our integer bits.
  static inline double BorrowBit() { return 1.5;};

  // Description:
  // Represent 2^30 as a double precision float. Use as a stepping
  // stone for computing 2^52 as a double, since we can't represent 2^52 as an
  // int before converting to double.
  static inline double two30()
    {
      return static_cast<double>(static_cast<unsigned long>(1) << 30);
    }

  // Description:
  // Represent 2^52 as a double precision float. This value is
  // significant because doubles have 52 bits of precision in the mantissa
  static inline double two52()
    {
      return (static_cast<unsigned long>(1) << (52-30)) * two30();
    }

  // Description:
  // Represent 2^51 as a double precision float. This value is significant
  // because doubles have 52 (explicit) bits of precision in the mantissa,
  // but we're going to pretend we only have 51 to play with when using safe
  // floor, since the default round-to-even on an X86 mucks with the LSB
  // during the denormalizing shift.
  static inline double two51()
    {
      return (static_cast<unsigned long>(1) << (51-30)) * two30();
    }

  // Description:
  // Represent 2^63 as a double precision float. We need this value to shift
  // unwanted fractional bits off the end of an extended precision value
  static inline double two63()
    {
      return (static_cast<unsigned long>(1) << (63-60)) * two30() * two30();
    }

  // Description:
  // Represent 2^62 as a double precision float. We need this value to shift
  // unwanted fractional bits off the end of an extended precision value. Use
  // when we're doing a SafeFloor.
  static inline double two62()
    {
      return (static_cast<unsigned long>(1) << (62-60)) * two30() * two30();
    }

  // Define number of bits of precision for various data types.
  // Note: INT_BITS is really 31, (rather than 32, since one of the bits is
  // just used for the two's-complement sign), but we say 30 because we don't
  // need to be able to handle 31-bit magnitudes correctly. I say that
  // because this is used for the QuickFloor code, and the SafeFloor code
  // retains an extra bit of fixed point precision which it shifts-out at the
  // end, thus reducing the magnitude of integers that it can handle. That's
  // an inherent limitation of using SafeFloor to prevent round-ups under any
  // circumstances, and there's no need to make QuickFloor handle a wider
  // range of numbers than SafeFloor.
#define INT_BITS 30
#define EXT_BITS 64
#define DBL_BITS 53

  // Description:
  // Small amount to use as a rounding tie-breaker to prevent
  // round-to-nearest-and-even mode from flooring-down odd numbered
  // integers. But number to nudge by depends on number of bits mantissa in
  // our floating point representation minus number of mantissa bits in the
  // range of signed ints we need to handle. In order to ensure that
  // flooring-down doesn't happen even for very large odd-integer values, the
  // number of bits used to represent the tie-breaker (i.e. to the right of
  // the binary-point), plus the number of bits needed to represent the
  // integer (to the left of the binary point), can not exceeds the number of
  // bits in the current precision mode. Thus, in selecting the tie-breaker
  // value, we select the largest number of bits to the right of the binary
  // point as possible while still maintaining that inequality. Thus, extended
  // precision mode allows a larger number of bits to the right of the binary
  // point.  This, in turn, implies a smaller value of the tie-breaker. And a
  // smaller tie-breaker will impose a tighter window on the range of values
  // that are erroneously rounded-up by a floor operation. Under double
  // precision, a QuickFloor of 0.9999998 (six 9's and an 8) correctly yields
  // 0. A value must be very close to 1.0, in fact, at least as close as
  // 0.9999999 (seven 9's)in order for the tie-breaker to bump it up to 1.
  // Under extended precision, an even smaller tie-breaker can be used. In
  // this mode, a QuickFloor of 0.9999999999 (ten 9's) correctly yields 0. A
  // QuickFloor of 0.99999999999 (eleven 9's) gets rounded up to 1. Since
  // these spurious round-ups occur only when the given value is virtually
  // indistinguishable from the next higher integer, the results should be
  // acceptable in most situations where performance is of the essence.
  // Make this public so that clients can account for the RoundingTieBreaker
  // if necessary
public:
#ifdef VTK_EXT_PREC
  // Compute (0.5 ^ (EXT_BITS-INT_BITS)) as a compile-time constant
  static inline double RoundingTieBreaker()
    {
      return 1.0 / (two30() * (static_cast<unsigned long>(1) << (EXT_BITS - INT_BITS - 30)));
    }
#else
  // Compute (0.5 ^ (DBL_BITS-INT_BITS)) as a compile-time constant
  static inline double RoundingTieBreaker()
    {
      return 1.0 / (static_cast<unsigned long>(1) << (DBL_BITS - INT_BITS));
    }
#endif

protected:
  // Description:
  // This is the magic floating point value which when added to any other
  // floating point value, causes the rounded integer portion of that
  // floating point value to appear in the least significant bits of the
  // mantissa, which is what we want.
  static inline double QuickFloorDenormalizer()
    {return two52() * BorrowBit(); };

  // Description:
  // This is the magic floating point value which when added to any other
  // floating point value, causes the rounded integer portion of that
  // floating point value to appear in the NEXT TO least significant bits of
  // the mantissa, which is what we want. This allows the CPU rounding mode
  // to muck with the LSB which we can then discard in SafeFloor
  static inline double SafeFloorDenormalizer()
    { return two51() * BorrowBit(); };

  // Description:
  // This value is added to and then subtracted from an extended precision
  // value in order to clear the fractional bits so that they do not
  // adversely affect the final double-precision result.
  static inline double QuickExtPrecTempDenormalizer()
    {return two63() * BorrowBit(); };

  // Description:
  // Just like QuickExtPrecTempDenormalizer(), but preserves one extra bit of
  // fixed point precision to guard against the CPU mucking with the LSB
  static inline double SafeExtPrecTempDenormalizer()
    {return two62() * BorrowBit(); };

  static inline double QuickRoundAdjust() {return 0.5;};
  static inline double SafeRoundAdjust() {return 0.25;};
  static inline int SafeFinalShift() {return 1;};


#ifdef VTK_WORDS_BIGENDIAN
  enum {exponent_pos = 0, mantissa_pos = 1};
#else
  enum {exponent_pos = 1, mantissa_pos = 0};
#endif
  //ETX

public:

  // Description:
  // Set the number of bits reserved for fractional precision that are
  // maintained as part of the flooring process. This number affects the
  // flooring arithmetic. It may be useful if the factional part is to be
  // used to index into a lookup table of some sort. However, if you are only
  // interested in knowing the fractional remainder after flooring, there
  // doesn't appear to be any advantage to using these bits, either in terms
  // of a lookup table, or by directly multiplying by some unit fraction,
  // over simply subtracting the floored value from the original value. Note
  // that since only 32 bits are used for the entire fixed point
  // representation, increasing the number of reserved fractional bits
  // reduces the range of integer values that can be floored to.
  void SetReservedFracBits(int bits)
    {
    // Add one to the requested number of fractional bits, to make
    // the conversion safe with respect to rounding mode. This is the
    // same as the difference between QuickFloor and SafeFloor.
    bits++;
    unsigned long mtime = this->GetMTime();
    this->SetinternalReservedFracBits(bits);
    if (mtime != this->GetMTime())
      {
      this->InternalRebuild();
      }
    };

  //BTX
  // Description:
  // Perform a quick flooring of the double-precision floating point
  // value. The result is sometimes incorrect, but in a way that makes it
  // acceptable for most uses. The naive way to implement floor(), given that
  // the x86 FPU does round() by default, is to define floor(x) as
  // round(x-.5).  This would work fine except for the fact that the x86 FPU
  // breaks rounding ties by selecting the even number. Thus, floor(4.0) =
  // round(3.5) = 4, but floor(3.0) = round(2.5) = 2. As a result,
  // subtracting .5 gives the wrong answer for odd integers. So, let's
  // subtract just a TEENSY bit less than .5, to swing the odd-integer
  // results up to their corect value. How teensy? Well, if it's too teensy,
  // it will be insignificant compared to 0.5, and will become equivalent to
  // 0.5.  And if it's not teensy enough, we'll overshoot, causing results
  // like floor(N-epsilon)==N, for some epsilon. Furthermore, the "too
  // teensy" problem is exacerbated when trying to floor larger numbers, due
  // to limitations of the representation's dynamic range. See the definition
  // of RoundingTieBreaker() for details.
  static int QuickFloor(const double &val);

  // Description:
  // Perform a SAFE flooring. Similar to QuickFloor, but modified to return
  // the correct result always. Use this when it absolutely positively needs
  // to be the correct answer all the time, and considering 0.9999999 as
  // being equal to 1.0 is simply not acceptable.  It works similarly to
  // QuickFloor, but it retains one extra bit of fixed point precision in the
  // conversion process, so that the problem with QuickFloor affects only an
  // unneeded bit, and then it ditches that bit from the resulting integer
  // with a right-shift. In other words, it rounds to the nearest one-half,
  // choosing the EVEN one-half (i.e. the integer) as a tie-breaker, and then
  // shifting off that half-integer bit. As a result of maintaining one extra
  // bit of fixed point precision in the intermediate calculation, the range
  // of integers supported is reduced by one bit. Plus, it takes a little
  // longer to execute, due to the final bit shift.
  static int SafeFloor(const double &val);

  // Description:
  // Round to nearest int.  This is pretty sweet in the default
  // round-to-nearest FPU mode, since it is generally immaterial how ties are
  // broken when rounding. I.e., either "2" or "3" are acceptable results for
  // "Round(2.5)", but only one of them (the one naively not chosen without
  // jumping through the hoops in QuickFloor and SafeFloor) is the acceptable
  // result for the analogous "Floor(3)". Therefore, we don't need to worry
  // at all about adding a teensy but not too teensy tie breaker, or shifting
  // off a half-integer bit. This makes it exceptionally fast.
  static int Round(const double &val);

  // Description:
  // Convert the value to a fixed point representation, returning the
  // integer portion as function value, and returning the fractional
  // part in the second parameter.
  inline int ConvertFixedPoint(const double &val, int &fracPart)
    {
      union { int i[2]; double d; } u;
#ifdef VTK_EXT_PREC
      u.d = (((val - fixRound)
              + this->epTempDenormalizer)
             - this->epTempDenormalizer)
        + this->fpDenormalizer;
#else // ! VTK_EXT_PREC
      u.d = (val - fixRound)
        + this->fpDenormalizer;
#endif // VTK_EXT_PREC
    fracPart = (u.i[mantissa_pos] & fracMask) >> 1;
    return u.i[mantissa_pos] >> this->internalReservedFracBits;
    }
  //ETX


protected:
  //BTX
  vtkFastNumericConversion();
  ~vtkFastNumericConversion() {}
  void InternalRebuild(void);

private:
  vtkSetMacro(internalReservedFracBits, int);
  vtkGetMacro(internalReservedFracBits, int);

#ifndef VTK_LEGACY_SILENT
  static int QuickFloorInline(const double &val);
  static int SafeFloorInline(const double &val);
  static int RoundInline(const double &val);
#endif

  int internalReservedFracBits;
  int fracMask;

  // Used when doing fixed point conversions with a certain number of bits
  // remaining for the fractional part, as opposed to the pure integer
  // flooring
  double fpDenormalizer;

  // Used when doing fixed point conversions in extended precision mode
  double epTempDenormalizer;

  // Adjustment for rounding based on the number of bits reserved for
  // fractional representation
  double fixRound;
  //ETX

  vtkFastNumericConversion(const vtkFastNumericConversion&); // Not implemented
  void operator=(const vtkFastNumericConversion&); // Not implemented
};

#ifndef VTK_LEGACY_SILENT
inline int vtkFastNumericConversion::QuickFloorInline(const double &val)
#else
inline int vtkFastNumericConversion::QuickFloor(const double &val)
#endif
{
#ifdef VTK_USE_TRICK
  union { int i[2]; double d; } u;
#ifdef VTK_EXT_PREC
  u.d = (((val - (QuickRoundAdjust() - RoundingTieBreaker()))
          // Push off those extended precision bits
          + QuickExtPrecTempDenormalizer())
         // Pull back the wanted bits into double range
         - QuickExtPrecTempDenormalizer())
    + QuickFloorDenormalizer();
#else // ! VTK_EXT_PREC
  u.d = (val - (QuickRoundAdjust() - RoundingTieBreaker()))
    + QuickFloorDenormalizer();
#endif // VTK_EXT_PREC
  return u.i[mantissa_pos];
#else // ! VTK_USE_TRICK
  return static_cast<int>(val);
#endif // VTK_USE_TRICK
}

#ifndef VTK_LEGACY_SILENT
inline int vtkFastNumericConversion::SafeFloorInline(const double &val)
#else
inline int vtkFastNumericConversion::SafeFloor(const double &val)
#endif
{
#ifdef VTK_USE_TRICK
  union { int i[2]; double d; } u;
#ifdef VTK_EXT_PREC
  u.d = (((val - SafeRoundAdjust())
          + SafeExtPrecTempDenormalizer())
         - SafeExtPrecTempDenormalizer())
    + SafeFloorDenormalizer();
#else // ! VTK_EXT_PREC
  u.d = (val - SafeRoundAdjust())
    + SafeFloorDenormalizer();
#endif // VTK_EXT_PREC
  return u.i[mantissa_pos] >> SafeFinalShift();
#else // ! VTK_USE_TRICK
  return static_cast<int>(val);
#endif // VTK_USE_TRICK
}

#ifndef VTK_LEGACY_SILENT
inline int vtkFastNumericConversion::RoundInline(const double &val)
#else
inline int vtkFastNumericConversion::Round(const double &val)
#endif
{
#ifdef VTK_USE_TRICK
  union { int i[2]; double d; } u;
#ifdef VTK_EXT_PREC
  u.d = ((val
          + QuickExtPrecTempDenormalizer())
         - QuickExtPrecTempDenormalizer())
    + QuickFloorDenormalizer();
#else // ! VTK_EXT_PREC
  u.d = val
    + QuickFloorDenormalizer();
#endif // VTK_EXT_PREC
return u.i[mantissa_pos];
#else // ! VTK_USE_TRICK
if (val>=0)
  {
  return static_cast<int>(val + 0.5);
  }
else
  {
  return static_cast<int>(val - 0.5);
  }
#endif // VTK_USE_TRICK
}

#endif
