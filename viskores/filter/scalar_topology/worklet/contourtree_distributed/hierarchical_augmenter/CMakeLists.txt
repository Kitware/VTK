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
  IsAttachementPointPredicate.h
  IsAscendingDecorator.h
  IsAttachementPointNeededPredicate.h
  AttachmentSuperparentAndIndexComparator.h
  AttachmentIdsEqualComparator.h
  SetFirstAttachmentPointInRoundWorklet.h
  UpdateHyperstructureSetHyperarcsAndNodesWorklet.h
  UpdateHyperstructureSetSuperchildrenWorklet.h
  FindSuperparentForNecessaryNodesWorklet.h
  CopyBaseRegularStructureWorklet.h
  SetSuperparentSetDecorator.h
  AttachmentAndSupernodeComparator.h
  ResizeArraysBuildNewSupernodeIdsWorklet.h
  FillEmptyIterationWorklet.h
  CreateSuperarcsWorklet.h
  CreateSuperarcsData.h
  CreateSuperarcsSetFirstSupernodePerIterationWorklet.h
  HierarchicalAugmenterInOutData.h
)

viskores_declare_headers(${headers})
