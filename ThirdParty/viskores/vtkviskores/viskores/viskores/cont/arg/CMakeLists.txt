##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

set(headers
  ControlSignatureTagBase.h
  Transport.h
  TransportTagArrayIn.h
  TransportTagArrayInOut.h
  TransportTagArrayOut.h
  TransportTagAtomicArray.h
  TransportTagBitField.h
  TransportTagCellSetIn.h
  TransportTagExecObject.h
  TransportTagKeyedValuesIn.h
  TransportTagKeyedValuesInOut.h
  TransportTagKeyedValuesOut.h
  TransportTagKeysIn.h
  TransportTagTopologyFieldIn.h
  TransportTagWholeArrayIn.h
  TransportTagWholeArrayInOut.h
  TransportTagWholeArrayOut.h
  TypeCheck.h
  TypeCheckTagArrayIn.h
  TypeCheckTagArrayInOut.h
  TypeCheckTagArrayOut.h
  TypeCheckTagAtomicArray.h
  TypeCheckTagBitField.h
  TypeCheckTagCellSet.h
  TypeCheckTagCellSetStructured.h
  TypeCheckTagExecObject.h
  TypeCheckTagKeys.h
  )

viskores_declare_headers(${headers})
