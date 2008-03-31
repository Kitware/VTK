//
// To install MetaIO in VTK, this file should be copied into the directory
//   /VTK/Utilities and renamed to metaIOConfig.h
//
// This file is automatically included by
//   MetaIO/localMetaConfiguration.h
//
// This file is needed to configure the local MetaIO files at compile time.
// Local MetaIO configuration changes include enabling/disabling the use
// of a MetaIO namespace, format of stl and cout calls, etc.   These
// configuration changes differentiate ITK's MetaIO distribution from
// VTK's MetaIO distribution and allow them to share the same code base,
// i.e., the same MetaIO cvs repository is linked into both projects' cvs
// repository. The common MetaIO cvs tree is located in KWPublic/MetaIO/src.
//

#undef METAIO_FOR_ITK

#define METAIO_FOR_VTK 1

