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
  Conversions.h
  ConvertToRGB.h
  ConvertToRGBA.h
  LookupTable.h
  Portals.h
  ShiftScaleToRGB.h
  ShiftScaleToRGBA.h
  TransferFunction.h
  )

set_source_files_properties(Conversions.h
                            ConvertToRGB.h
                            ConvertToRGBA.h
                            ShiftScaleToRGB.h
                            ShiftScaleToRGBA.h
                            PROPERTIES Viskores_CANT_BE_HEADER_TESTED TRUE)
#-----------------------------------------------------------------------------
viskores_declare_headers(${headers})
