## Arrays have been added and deprecated

The following arrays have been deprecated.

1. `vtkAffineArray`'s concrete specializations:
   1. `vtkAffineCharArray`
   2. `vtkAffineDoubleArray`
   3. `vtkAffineFloatArray`
   4. `vtkAffineIntArray`
   5. `vtkAffineLongArray`
   6. `vtkAffineLongLongArray`
   7. `vtkAffineShortArray`
   8. `vtkAffineSignedCharArray`
   9. `vtkAffineUnsignedCharArray`
   10. `vtkAffineUnsignedIntArray`
   11. `vtkAffineUnsignedLongArray`
   12. `vtkAffineUnsignedLongLongArray`
   13. `vtkAffineUnsignedShortArray`
   14. `vtkAffineIdTypeArray`
2. `vtkCompositeArray`'s concrete specializations:
   1. `vtkCompositeCharArray`
   2. `vtkCompositeDoubleArray`
   3. `vtkCompositeFloatArray`
   4. `vtkCompositeIntArray`
   5. `vtkCompositeLongArray`
   6. `vtkCompositeLongLongArray`
   7. `vtkCompositeShortArray`
   8. `vtkCompositeSignedCharArray`
   9. `vtkCompositeUnsignedCharArray`
   10. `vtkCompositeUnsignedIntArray`
   11. `vtkCompositeUnsignedLongArray`
   12. `vtkCompositeUnsignedLongLongArray`
   13. `vtkCompositeUnsignedShortArray`
   14. `vtkCompositeIdTypeArray`
3. `vtkConstantArray`s concrete specializations:
   1. `vtkConstantCharArray`
   2. `vtkConstantDoubleArray`
   3. `vtkConstantFloatArray`
   4. `vtkConstantIntArray`
   5. `vtkConstantLongArray`
   6. `vtkConstantLongLongArray`
   7. `vtkConstantShortArray`
   8. `vtkConstantSignedCharArray`
   9. `vtkConstantUnsignedCharArray`
   10. `vtkConstantUnsignedIntArray`
   11. `vtkConstantUnsignedLongArray`
   12. `vtkConstantUnsignedLongLongArray`
   13. `vtkConstantUnsignedShortArray`
   14. `vtkConstantIdTypeArray`
4. `vtkIndexedArray`'s concrete specializations:
   1. `vtkIndexedCharArray`
   2. `vtkIndexedDoubleArray`
   3. `vtkIndexedFloatArray`
   4. `vtkIndexedIntArray`
   5. `vtkIndexedLongArray`
   6. `vtkIndexedLongLongArray`
   7. `vtkIndexedShortArray`
   8. `vtkIndexedSignedCharArray`
   9. `vtkIndexedUnsignedCharArray`
   10. `vtkIndexedUnsignedIntArray`
   11. `vtkIndexedUnsignedLongArray`
   12. `vtkIndexedUnsignedLongLongArray`
   13. `vtkIndexedUnsignedShortArray`
   14. `vtkIndexedIdTypeArray`

The following fixed size arrays have been added as replacements.

1. `vtkAffineArray`'s fixed size specialization:
   1. `vtkAffineTypeFloat32Array`
   2. `vtkAffineTypeFloat64Array`
   3. `vtkAffineTypeInt8Array`
   4. `vtkAffineTypeInt16Array`
   5. `vtkAffineTypeInt32Array`
   6. `vtkAffineTypeInt64Array`
   7. `vtkAffineTypeUInt8Array`
   8. `vtkAffineTypeUInt16Array`
   9. `vtkAffineTypeUInt32Array`
   10. `vtkAffineTypeUInt64Array`
2. `vtkCompositeArray`'s fixed size specialization:
   1. `vtkCompositeTypeFloat32Array`
   2. `vtkCompositeTypeFloat64Array`
   3. `vtkCompositeTypeInt8Array`
   4. `vtkCompositeTypeInt16Array`
   5. `vtkCompositeTypeInt32Array`
   6. `vtkCompositeTypeInt64Array`
   7. `vtkCompositeTypeUInt8Array`
   8. `vtkCompositeTypeUInt16Array`
   9. `vtkCompositeTypeUInt32Array`
   10. `vtkCompositeTypeUInt64Array`
3. `vtkConstantArray`'s fixed size specialization:
   1. `vtkConstantTypeFloat32Array`
   2. `vtkConstantTypeFloat64Array`
   3. `vtkConstantTypeInt8Array`
   4. `vtkConstantTypeInt16Array`
   5. `vtkConstantTypeInt32Array`
   6. `vtkConstantTypeInt64Array`
   7. `vtkConstantTypeUInt8Array`
   8. `vtkConstantTypeUInt16Array`
   9. `vtkConstantTypeUInt32Array`
   10. `vtkConstantTypeUInt64Array`
4. `vtkIndexedArray`'s fixed size specialization:
   1. `vtkIndexedTypeFloat32Array`
   2. `vtkIndexedTypeFloat64Array`
   3. `vtkIndexedTypeInt8Array`
   4. `vtkIndexedTypeInt16Array`
   5. `vtkIndexedTypeInt32Array`
   6. `vtkIndexedTypeInt64Array`
   7. `vtkIndexedTypeUInt8Array`
   8. `vtkIndexedTypeUInt16Array`
   9. `vtkIndexedTypeUInt32Array`
   10. `vtkIndexedTypeUInt64Array`

Additionally, the following concrete fixed size arrays have been added:

1. `vtkScaledSOADataArrayTemplate`'s fixed size specialization:
   1. `vtkScaledSOATypeFloat32Array`
   2. `vtkScaledSOATypeFloat64Array`
   3. `vtkScaledSOATypeInt8Array`
   4. `vtkScaledSOATypeInt16Array`
   5. `vtkScaledSOATypeInt32Array`
   6. `vtkScaledSOATypeInt64Array`
   7. `vtkScaledSOATypeUInt8Array`
   8. `vtkScaledSOATypeUInt16Array`
   9. `vtkScaledSOATypeUInt32Array`
   10. `vtkScaledSOATypeUInt64Array`
2. `vtkSOADataDataArrayTemplate`'s fixed size specialization:
   1. `vtkSOATypeFloat32Array`
   2. `vtkSOATypeFloat64Array`
   3. `vtkSOATypeInt8Array`
   4. `vtkSOATypeInt16Array`
   5. `vtkSOATypeInt32Array`
   6. `vtkSOATypeInt64Array`
   7. `vtkSOATypeUInt8Array`
   8. `vtkSOATypeUInt16Array`
   9. `vtkSOATypeUInt32Array`
   10. `vtkSOATypeUInt64Array`
3. `vtkStdFunctionArray`'s fixed size specialization:
   1. `vtkStdFunctionTypeFloat32Array`
   2. `vtkStdFunctionTypeFloat64Array`
   3. `vtkStdFunctionTypeInt8Array`
   4. `vtkStdFunctionTypeInt16Array`
   5. `vtkStdFunctionTypeInt32Array`
   6. `vtkStdFunctionTypeInt64Array`
   7. `vtkStdFunctionTypeUInt8Array`
   8. `vtkStdFunctionTypeUInt16Array`
   9. `vtkStdFunctionTypeUInt32Array`
   10. `vtkStdFunctionTypeUInt64Array`
4. `vtkStridedArray`'s fixed size specialization:
   1. `vtkStridedTypeFloat32Array`
   2. `vtkStridedTypeFloat64Array`
   3. `vtkStridedTypeInt8Array`
   4. `vtkStridedTypeInt16Array`
   5. `vtkStridedTypeInt32Array`
   6. `vtkStridedTypeInt64Array`
   7. `vtkStridedTypeUInt8Array`
   8. `vtkStridedTypeUInt16Array`
   9. `vtkStridedTypeUInt32Array`
   10. `vtkStridedTypeUInt64Array`
