"""Test vtkVariant support in VTK-Python

The following vtkVariant features have to be tested:
- Construction from various types
- Automatic arg conversion for methods with vtkVariant args
- Access of various types
- Operators < <= == > >=
- Use of vtkVariant as a dict key

The following features are not supported
- The ToNumeric method

Created on May 12, 2010 by David Gobbi

"""

import sys
from vtkmodules.util.vtkVariant import (
    vtkVariantEqual,
    vtkVariantExtract,
    vtkVariantLessThan,
    vtkVariantStrictEquality,
    vtkVariantStrictWeakOrder,
    vtkVariantStrictWeakOrderKey,
)
from vtkmodules.vtkCommonCore import (
    VTK_DOUBLE,
    VTK_INT,
    VTK_OBJECT,
    VTK_STRING,
    vtkArray,
    vtkIntArray,
    vtkObject,
    vtkVariant,
    vtkVariantArray,
)
from vtkmodules.test import Testing

cedilla = 'Fran\xe7ois'


class TestVariant(Testing.vtkTest):
    def testDefaultConstructor(self):
        """Default constructor"""
        v = vtkVariant()
        self.assertEqual(v.IsValid(), False)
        self.assertEqual(v.GetType(), 0)

    def testCopyConstructor(self):
        """Construct from another vtkVariant"""
        u = vtkVariant('test')
        v = vtkVariant(u)
        self.assertEqual(v.GetType(), VTK_STRING)
        self.assertEqual(v.ToString(), u.ToString())

    def testIntConstructor(self):
        """Construct from int"""
        v = vtkVariant(10)
        self.assertEqual(v.GetType(), VTK_INT)
        self.assertEqual(v.ToInt(), 10)

    def testFloatConstructor(self):
        """Construct from float"""
        v = vtkVariant(10.0)
        self.assertEqual(v.GetType(), VTK_DOUBLE)
        self.assertEqual(v.ToDouble(), 10.0)

    def testStringConstructor(self):
        """Construct from string"""
        v = vtkVariant('hello')
        self.assertEqual(v.GetType(), VTK_STRING)
        self.assertEqual(v.ToString(), 'hello')

    def testBytesConstructor(self):
        """Construct from bytes"""
        v = vtkVariant(b'hello')
        self.assertEqual(v.GetType(), VTK_STRING)
        self.assertEqual(v.ToString(), 'hello')

    def testUnicodeConstructor(self):
        """Construct from unicode"""
        v = vtkVariant(cedilla)
        self.assertEqual(v.GetType(), VTK_STRING)
        self.assertEqual(v.ToString(), cedilla)

    def testObjectConstructor(self):
        """Construct from VTK object"""
        o = vtkIntArray()
        v = vtkVariant(o)
        self.assertEqual(v.GetType(), VTK_OBJECT)
        self.assertEqual(v.GetTypeAsString(), o.GetClassName())
        self.assertEqual(v.ToVTKObject(), o)

    def testTwoArgConstructor(self):
        """Construct with a specific type"""
        # construct with conversion to int
        v = vtkVariant('10')
        u = vtkVariant(v, VTK_INT)
        self.assertEqual(u.GetType(), VTK_INT)
        self.assertEqual(v.ToInt(), u.ToInt())

        # construct with conversion to double
        v = vtkVariant(10)
        u = vtkVariant(v, VTK_DOUBLE)
        self.assertEqual(u.GetType(), VTK_DOUBLE)
        self.assertEqual(u.ToDouble(), 10.0)

        # failed conversion to vtkObject
        v = vtkVariant(10)
        u = vtkVariant(v, VTK_OBJECT)
        self.assertEqual(u.IsValid(), False)

    def testAutomaticArgConversion(self):
        """Automatic construction of variants to resolve args"""
        # use with one of vtkVariant's own constructors
        v = vtkVariant('10', VTK_INT)
        self.assertEqual(v.ToInt(), 10)
        self.assertEqual(v.GetType(), VTK_INT)

        # use with vtkVariantArray
        a = vtkVariantArray()
        i = a.InsertNextValue(10)
        v = a.GetValue(i)
        self.assertEqual(v.GetType(), VTK_INT)
        self.assertEqual(v.ToInt(), 10)
        i = a.InsertNextValue(10.0)
        v = a.GetValue(i)
        self.assertEqual(v.GetType(), VTK_DOUBLE)
        self.assertEqual(v.ToDouble(), 10.0)
        i = a.InsertNextValue('10')
        v = a.GetValue(i)
        self.assertEqual(v.GetType(), VTK_STRING)
        self.assertEqual(v.ToString(), '10')

    def testCompare(self):
        """Use comparison operators to sort a list of vtkVariants"""
        original = [1, 2.5, vtkVariant(), "0", cedilla]
        ordered = [vtkVariant(), "0", 1, 2.5, cedilla]
        l = [vtkVariant(x) for x in original]
        s = [vtkVariant(x) for x in ordered]
        l.sort()
        self.assertEqual(l, s)

    def testComparisonMethods(self):
        v1 = vtkVariant(10)
        v2 = vtkVariant("10")
        # compare without regards to type
        self.assertEqual(vtkVariantEqual(v1, v2), True)
        self.assertEqual(vtkVariantLessThan(v1, v2), False)
        # compare with different types being non-equivalent
        self.assertEqual(vtkVariantStrictEquality(v1, v2), False)
        self.assertEqual(vtkVariantStrictWeakOrder(v1, v2), True)

    def testStrictWeakOrder(self):
        """Use vtkVariantStrictWeakOrder to sort a list of vtkVariants"""
        original = [1, 2.5, vtkVariant(), "0", cedilla]
        ordered = [vtkVariant(), 1, 2.5, "0", cedilla]
        l = [vtkVariant(x) for x in original]
        s = [vtkVariant(x) for x in ordered]
        l.sort(key=vtkVariantStrictWeakOrderKey)
        self.assertEqual(l, s)

    def testVariantExtract(self):
        """Use vtkVariantExtract"""
        l = [1, '2', cedilla, 4.0, vtkObject()]
        s = [vtkVariant(x) for x in l]
        m = [vtkVariantExtract(x) for x in s]
        self.assertEqual(l, m)

    def testHash(self):
        """Use a variant as a dict key"""
        d = {}
        # doubles, ints, strings, all hash as strings
        d[vtkVariant(1.0)] = 'double'
        d[vtkVariant(1)] = 'int'
        self.assertEqual(d[vtkVariant('1')], 'int')

        # every vtkObject is hashed by memory address
        o1 = vtkIntArray()
        o2 = vtkIntArray()
        d[vtkVariant(o1)] = 'vtkIntArray1'
        d[vtkVariant(o2)] = 'vtkIntArray2'
        self.assertEqual(d[vtkVariant(o1)], 'vtkIntArray1')
        self.assertEqual(d[vtkVariant(o2)], 'vtkIntArray2')

        # invalid variants all hash the same
        d[vtkVariant()] = 'invalid'
        self.assertEqual(d[vtkVariant()], 'invalid')

    def testPassByValueReturnByReference(self):
        """Pass vtkVariant by value, return by reference"""
        a = vtkVariantArray()
        a.SetNumberOfValues(1)
        v = vtkVariant(1)
        a.SetValue(0, v)
        u = a.GetValue(0)
        self.assertEqual(u.ToInt(), v.ToInt())
        self.assertEqual(u.GetType(), v.GetType())
        self.assertEqual(u.IsValid(), v.IsValid())

    def testPassByReferenceReturnByValue(self):
        """Pass vtkVariant by reference, return by value."""
        a = vtkArray.CreateArray(1, VTK_INT)
        a.Resize(1,1)
        v = vtkVariant(1)
        a.SetVariantValue(0, 0, v)
        u = a.GetVariantValue(0, 0)
        self.assertEqual(u.ToInt(), v.ToInt())
        self.assertEqual(u.GetType(), v.GetType())
        self.assertEqual(u.IsValid(), v.IsValid())

if __name__ == "__main__":
    Testing.main([(TestVariant, 'test')])
