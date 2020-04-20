if (FALSE) # XXX(kitware): cmake versions are handled by VTK.
cmake_minimum_required (VERSION 3.10)
endif ()
project (HDF5_SRC C)

#-----------------------------------------------------------------------------
# List Source Files
#-----------------------------------------------------------------------------
set (H5_SOURCES
    ${HDF5_SRC_DIR}/H5.c
    ${HDF5_SRC_DIR}/H5checksum.c
    ${HDF5_SRC_DIR}/H5dbg.c
    ${HDF5_SRC_DIR}/H5system.c
    ${HDF5_SRC_DIR}/H5timer.c
    ${HDF5_SRC_DIR}/H5trace.c
)

set (H5_HDRS
    ${HDF5_SRC_DIR}/hdf5.h
    ${HDF5_SRC_DIR}/H5api_adpt.h
    ${HDF5_SRC_DIR}/H5public.h
    ${HDF5_SRC_DIR}/vtk_hdf5_mangle.h
    #${HDF5_SRC_DIR}/H5version.h
    #${HDF5_SRC_DIR}/H5overflow.h
)
IDE_GENERATED_PROPERTIES ("H5" "${H5_HDRS}" "${H5_SOURCES}" )

set (H5A_SOURCES
    ${HDF5_SRC_DIR}/H5A.c
    ${HDF5_SRC_DIR}/H5Abtree2.c
    ${HDF5_SRC_DIR}/H5Adense.c
    ${HDF5_SRC_DIR}/H5Adeprec.c
    ${HDF5_SRC_DIR}/H5Aint.c
    ${HDF5_SRC_DIR}/H5Atest.c
)

set (H5A_HDRS
    ${HDF5_SRC_DIR}/H5Apublic.h
)
IDE_GENERATED_PROPERTIES ("H5A" "${H5A_HDRS}" "${H5A_SOURCES}" )

set (H5AC_SOURCES
    ${HDF5_SRC_DIR}/H5AC.c
    ${HDF5_SRC_DIR}/H5ACdbg.c
    ${HDF5_SRC_DIR}/H5ACmpio.c
    ${HDF5_SRC_DIR}/H5ACproxy_entry.c
)

set (H5AC_HDRS
    ${HDF5_SRC_DIR}/H5ACpublic.h
)
IDE_GENERATED_PROPERTIES ("H5AC" "${H5AC_HDRS}" "${H5AC_SOURCES}" )

set (H5B_SOURCES
    ${HDF5_SRC_DIR}/H5B.c
    ${HDF5_SRC_DIR}/H5Bcache.c
    ${HDF5_SRC_DIR}/H5Bdbg.c
)
set (H5B_HDRS
)
IDE_GENERATED_PROPERTIES ("H5B" "${H5B_HDRS}" "${H5B_SOURCES}" )


set (H5B2_SOURCES
    ${HDF5_SRC_DIR}/H5B2.c
    ${HDF5_SRC_DIR}/H5B2cache.c
    ${HDF5_SRC_DIR}/H5B2dbg.c
    ${HDF5_SRC_DIR}/H5B2hdr.c
    ${HDF5_SRC_DIR}/H5B2int.c
    ${HDF5_SRC_DIR}/H5B2internal.c
    ${HDF5_SRC_DIR}/H5B2leaf.c
    ${HDF5_SRC_DIR}/H5B2stat.c
    ${HDF5_SRC_DIR}/H5B2test.c
)
set (H5B2_HDRS
)
IDE_GENERATED_PROPERTIES ("H5B2" "${H5B2_HDRS}" "${H5B2_SOURCES}" )


set (H5C_SOURCES
    ${HDF5_SRC_DIR}/H5C.c
    ${HDF5_SRC_DIR}/H5Cdbg.c
    ${HDF5_SRC_DIR}/H5Cepoch.c
    ${HDF5_SRC_DIR}/H5Cimage.c
    ${HDF5_SRC_DIR}/H5Clog.c
    ${HDF5_SRC_DIR}/H5Clog_json.c
    ${HDF5_SRC_DIR}/H5Clog_trace.c
    ${HDF5_SRC_DIR}/H5Cmpio.c
    ${HDF5_SRC_DIR}/H5Cprefetched.c
    ${HDF5_SRC_DIR}/H5Cquery.c
    ${HDF5_SRC_DIR}/H5Ctag.c
    ${HDF5_SRC_DIR}/H5Ctest.c
)
set (H5C_HDRS
    ${HDF5_SRC_DIR}/H5Cpublic.h
)
IDE_GENERATED_PROPERTIES ("H5C" "${H5C_HDRS}" "${H5C_SOURCES}" )


set (H5CS_SOURCES
    ${HDF5_SRC_DIR}/H5CS.c
)
set (H5CS_HDRS
)
IDE_GENERATED_PROPERTIES ("H5CS" "${H5CS_HDRS}" "${H5CS_SOURCES}" )


set (H5CX_SOURCES
    ${HDF5_SRC_DIR}/H5CX.c
)
set (H5CX_HDRS
)
IDE_GENERATED_PROPERTIES ("H5CX" "${H5CX_HDRS}" "${H5CX_SOURCES}" )


set (H5D_SOURCES
    ${HDF5_SRC_DIR}/H5D.c
    ${HDF5_SRC_DIR}/H5Dbtree.c
    ${HDF5_SRC_DIR}/H5Dbtree2.c
    ${HDF5_SRC_DIR}/H5Dchunk.c
    ${HDF5_SRC_DIR}/H5Dcompact.c
    ${HDF5_SRC_DIR}/H5Dcontig.c
    ${HDF5_SRC_DIR}/H5Ddbg.c
    ${HDF5_SRC_DIR}/H5Ddeprec.c
    ${HDF5_SRC_DIR}/H5Dearray.c
    ${HDF5_SRC_DIR}/H5Defl.c
    ${HDF5_SRC_DIR}/H5Dfarray.c
    ${HDF5_SRC_DIR}/H5Dfill.c
    ${HDF5_SRC_DIR}/H5Dint.c
    ${HDF5_SRC_DIR}/H5Dio.c
    ${HDF5_SRC_DIR}/H5Dlayout.c
    ${HDF5_SRC_DIR}/H5Dmpio.c
    ${HDF5_SRC_DIR}/H5Dnone.c
    ${HDF5_SRC_DIR}/H5Doh.c
    ${HDF5_SRC_DIR}/H5Dscatgath.c
    ${HDF5_SRC_DIR}/H5Dselect.c
    ${HDF5_SRC_DIR}/H5Dsingle.c
    ${HDF5_SRC_DIR}/H5Dtest.c
    ${HDF5_SRC_DIR}/H5Dvirtual.c
)

set (H5D_HDRS
    ${HDF5_SRC_DIR}/H5Dpublic.h
)
IDE_GENERATED_PROPERTIES ("H5D" "${H5D_HDRS}" "${H5D_SOURCES}" )

set (H5E_SOURCES
    ${HDF5_SRC_DIR}/H5E.c
    ${HDF5_SRC_DIR}/H5Edeprec.c
    ${HDF5_SRC_DIR}/H5Eint.c
)

set (H5E_HDRS
    ${HDF5_SRC_DIR}/H5Epublic.h
)
IDE_GENERATED_PROPERTIES ("H5E" "${H5E_HDRS}" "${H5E_SOURCES}" )


set (H5EA_SOURCES
    ${HDF5_SRC_DIR}/H5EA.c
    ${HDF5_SRC_DIR}/H5EAcache.c
    ${HDF5_SRC_DIR}/H5EAdbg.c
    ${HDF5_SRC_DIR}/H5EAdblkpage.c
    ${HDF5_SRC_DIR}/H5EAdblock.c
    ${HDF5_SRC_DIR}/H5EAhdr.c
    ${HDF5_SRC_DIR}/H5EAiblock.c
    ${HDF5_SRC_DIR}/H5EAint.c
    ${HDF5_SRC_DIR}/H5EAsblock.c
    ${HDF5_SRC_DIR}/H5EAstat.c
    ${HDF5_SRC_DIR}/H5EAtest.c
)
set (H5EA_HDRS
)
IDE_GENERATED_PROPERTIES ("H5EA" "${H5EA_HDRS}" "${H5EA_SOURCES}" )


set (H5F_SOURCES
    ${HDF5_SRC_DIR}/H5F.c
    ${HDF5_SRC_DIR}/H5Faccum.c
    ${HDF5_SRC_DIR}/H5Fcwfs.c
    ${HDF5_SRC_DIR}/H5Fdbg.c
    ${HDF5_SRC_DIR}/H5Fdeprec.c
    ${HDF5_SRC_DIR}/H5Fefc.c
    ${HDF5_SRC_DIR}/H5Ffake.c
    ${HDF5_SRC_DIR}/H5Fint.c
    ${HDF5_SRC_DIR}/H5Fio.c
    ${HDF5_SRC_DIR}/H5Fmount.c
    ${HDF5_SRC_DIR}/H5Fmpi.c
    ${HDF5_SRC_DIR}/H5Fquery.c
    ${HDF5_SRC_DIR}/H5Fsfile.c
    ${HDF5_SRC_DIR}/H5Fspace.c
    ${HDF5_SRC_DIR}/H5Fsuper.c
    ${HDF5_SRC_DIR}/H5Fsuper_cache.c
    ${HDF5_SRC_DIR}/H5Ftest.c
)

set (H5F_HDRS
    ${HDF5_SRC_DIR}/H5Fpublic.h
)
IDE_GENERATED_PROPERTIES ("H5F" "${H5F_HDRS}" "${H5F_SOURCES}" )


set (H5FA_SOURCES
    ${HDF5_SRC_DIR}/H5FA.c
    ${HDF5_SRC_DIR}/H5FAcache.c
    ${HDF5_SRC_DIR}/H5FAdbg.c
    ${HDF5_SRC_DIR}/H5FAdblkpage.c
    ${HDF5_SRC_DIR}/H5FAdblock.c
    ${HDF5_SRC_DIR}/H5FAhdr.c
    ${HDF5_SRC_DIR}/H5FAint.c
    ${HDF5_SRC_DIR}/H5FAstat.c
    ${HDF5_SRC_DIR}/H5FAtest.c
)
set (H5FA_HDRS
)
IDE_GENERATED_PROPERTIES ("H5FA" "${H5FA_HDRS}" "${H5FA_SOURCES}" )


set (H5FD_SOURCES
    ${HDF5_SRC_DIR}/H5FD.c
    ${HDF5_SRC_DIR}/H5FDcore.c
    ${HDF5_SRC_DIR}/H5FDdirect.c
    ${HDF5_SRC_DIR}/H5FDfamily.c
    ${HDF5_SRC_DIR}/H5FDhdfs.c
    ${HDF5_SRC_DIR}/H5FDint.c
    ${HDF5_SRC_DIR}/H5FDlog.c
    ${HDF5_SRC_DIR}/H5FDmpi.c
    ${HDF5_SRC_DIR}/H5FDmpio.c
    ${HDF5_SRC_DIR}/H5FDmulti.c
    ${HDF5_SRC_DIR}/H5FDros3.c
    ${HDF5_SRC_DIR}/H5FDs3comms.c
    ${HDF5_SRC_DIR}/H5FDsec2.c
    ${HDF5_SRC_DIR}/H5FDspace.c
    ${HDF5_SRC_DIR}/H5FDstdio.c
    ${HDF5_SRC_DIR}/H5FDtest.c
    ${HDF5_SRC_DIR}/H5FDwindows.c
)

set (H5FD_HDRS
    ${HDF5_SRC_DIR}/H5FDcore.h
    ${HDF5_SRC_DIR}/H5FDdirect.h
    ${HDF5_SRC_DIR}/H5FDfamily.h
    ${HDF5_SRC_DIR}/H5FDhdfs.h
    ${HDF5_SRC_DIR}/H5FDlog.h
    ${HDF5_SRC_DIR}/H5FDmpi.h
    ${HDF5_SRC_DIR}/H5FDmpio.h
    ${HDF5_SRC_DIR}/H5FDmulti.h
    ${HDF5_SRC_DIR}/H5FDpublic.h
    ${HDF5_SRC_DIR}/H5FDros3.h
    ${HDF5_SRC_DIR}/H5FDs3comms.h
    ${HDF5_SRC_DIR}/H5FDsec2.h
    ${HDF5_SRC_DIR}/H5FDstdio.h
    ${HDF5_SRC_DIR}/H5FDwindows.h
)
IDE_GENERATED_PROPERTIES ("H5FD" "${H5FD_HDRS}" "${H5FD_SOURCES}" )


set (H5FL_SOURCES
    ${HDF5_SRC_DIR}/H5FL.c
)
set (H5FL_HDRS
)
IDE_GENERATED_PROPERTIES ("H5FL" "${H5FL_HDRS}" "${H5FL_SOURCES}" )


set (H5FO_SOURCES
    ${HDF5_SRC_DIR}/H5FO.c
)
set (H5FO_HDRS
)
IDE_GENERATED_PROPERTIES ("H5FO" "${H5FO_HDRS}" "${H5FO_SOURCES}" )


set (H5FS_SOURCES
    ${HDF5_SRC_DIR}/H5FS.c
    ${HDF5_SRC_DIR}/H5FScache.c
    ${HDF5_SRC_DIR}/H5FSdbg.c
    ${HDF5_SRC_DIR}/H5FSint.c
    ${HDF5_SRC_DIR}/H5FSsection.c
    ${HDF5_SRC_DIR}/H5FSstat.c
    ${HDF5_SRC_DIR}/H5FStest.c
)

set (H5FS_HDRS
)
IDE_GENERATED_PROPERTIES ("H5FS" "${H5FS_HDRS}" "${H5FS_SOURCES}" )

set (H5G_SOURCES
    ${HDF5_SRC_DIR}/H5G.c
    ${HDF5_SRC_DIR}/H5Gbtree2.c
    ${HDF5_SRC_DIR}/H5Gcache.c
    ${HDF5_SRC_DIR}/H5Gcompact.c
    ${HDF5_SRC_DIR}/H5Gdense.c
    ${HDF5_SRC_DIR}/H5Gdeprec.c
    ${HDF5_SRC_DIR}/H5Gent.c
    ${HDF5_SRC_DIR}/H5Gint.c
    ${HDF5_SRC_DIR}/H5Glink.c
    ${HDF5_SRC_DIR}/H5Gloc.c
    ${HDF5_SRC_DIR}/H5Gname.c
    ${HDF5_SRC_DIR}/H5Gnode.c
    ${HDF5_SRC_DIR}/H5Gobj.c
    ${HDF5_SRC_DIR}/H5Goh.c
    ${HDF5_SRC_DIR}/H5Groot.c
    ${HDF5_SRC_DIR}/H5Gstab.c
    ${HDF5_SRC_DIR}/H5Gtest.c
    ${HDF5_SRC_DIR}/H5Gtraverse.c
)

set (H5G_HDRS
    ${HDF5_SRC_DIR}/H5Gpublic.h
)
IDE_GENERATED_PROPERTIES ("H5G" "${H5G_HDRS}" "${H5G_SOURCES}" )

set (H5HF_SOURCES
    ${HDF5_SRC_DIR}/H5HF.c
    ${HDF5_SRC_DIR}/H5HFbtree2.c
    ${HDF5_SRC_DIR}/H5HFcache.c
    ${HDF5_SRC_DIR}/H5HFdbg.c
    ${HDF5_SRC_DIR}/H5HFdblock.c
    ${HDF5_SRC_DIR}/H5HFdtable.c
    ${HDF5_SRC_DIR}/H5HFhdr.c
    ${HDF5_SRC_DIR}/H5HFhuge.c
    ${HDF5_SRC_DIR}/H5HFiblock.c
    ${HDF5_SRC_DIR}/H5HFiter.c
    ${HDF5_SRC_DIR}/H5HFman.c
    ${HDF5_SRC_DIR}/H5HFsection.c
    ${HDF5_SRC_DIR}/H5HFspace.c
    ${HDF5_SRC_DIR}/H5HFstat.c
    ${HDF5_SRC_DIR}/H5HFtest.c
    ${HDF5_SRC_DIR}/H5HFtiny.c
)

set (H5HF_HDRS
)
IDE_GENERATED_PROPERTIES ("H5HF" "${H5HF_HDRS}" "${H5HF_SOURCES}" )

set (H5HG_SOURCES
    ${HDF5_SRC_DIR}/H5HG.c
    ${HDF5_SRC_DIR}/H5HGcache.c
    ${HDF5_SRC_DIR}/H5HGdbg.c
    ${HDF5_SRC_DIR}/H5HGquery.c
)

set (H5HG_HDRS
)
IDE_GENERATED_PROPERTIES ("H5HG" "${H5HG_HDRS}" "${H5HG_SOURCES}" )

set (H5HL_SOURCES
    ${HDF5_SRC_DIR}/H5HL.c
    ${HDF5_SRC_DIR}/H5HLcache.c
    ${HDF5_SRC_DIR}/H5HLdbg.c
    ${HDF5_SRC_DIR}/H5HLdblk.c
    ${HDF5_SRC_DIR}/H5HLint.c
    ${HDF5_SRC_DIR}/H5HLprfx.c
)

set (H5HL_HDRS
)
IDE_GENERATED_PROPERTIES ("H5HL" "${H5HL_HDRS}" "${H5HL_SOURCES}" )


set (H5HP_SOURCES
    ${HDF5_SRC_DIR}/H5HP.c
)
set (H5HP_HDRS
)
IDE_GENERATED_PROPERTIES ("H5HP" "${H5HP_HDRS}" "${H5HP_SOURCES}" )


set (H5I_SOURCES
    ${HDF5_SRC_DIR}/H5I.c
    ${HDF5_SRC_DIR}/H5Itest.c
)
set (H5I_HDRS
    ${HDF5_SRC_DIR}/H5Ipublic.h
)
IDE_GENERATED_PROPERTIES ("H5I" "${H5I_HDRS}" "${H5I_SOURCES}" )


set (H5L_SOURCES
    ${HDF5_SRC_DIR}/H5L.c
    ${HDF5_SRC_DIR}/H5Lexternal.c
)
set (H5L_HDRS
    ${HDF5_SRC_DIR}/H5Lpublic.h
)
IDE_GENERATED_PROPERTIES ("H5L" "${H5L_HDRS}" "${H5L_SOURCES}" )


set (H5MF_SOURCES
    ${HDF5_SRC_DIR}/H5MF.c
    ${HDF5_SRC_DIR}/H5MFaggr.c
    ${HDF5_SRC_DIR}/H5MFdbg.c
    ${HDF5_SRC_DIR}/H5MFsection.c
)

set (H5MF_HDRS
)
IDE_GENERATED_PROPERTIES ("H5MF" "${H5MF_HDRS}" "${H5MF_SOURCES}" )


set (H5MM_SOURCES
    ${HDF5_SRC_DIR}/H5MM.c
)
set (H5MM_HDRS
    ${HDF5_SRC_DIR}/H5MMpublic.h
)
IDE_GENERATED_PROPERTIES ("H5MM" "${H5MM_HDRS}" "${H5MM_SOURCES}" )


set (H5MP_SOURCES
    ${HDF5_SRC_DIR}/H5MP.c
    ${HDF5_SRC_DIR}/H5MPtest.c
)

set (H5MP_HDRS
)
IDE_GENERATED_PROPERTIES ("H5MP" "${H5MP_HDRS}" "${H5MP_SOURCES}" )

set (H5O_SOURCES
    ${HDF5_SRC_DIR}/H5O.c
    ${HDF5_SRC_DIR}/H5Oainfo.c
    ${HDF5_SRC_DIR}/H5Oalloc.c
    ${HDF5_SRC_DIR}/H5Oattr.c
    ${HDF5_SRC_DIR}/H5Oattribute.c
    ${HDF5_SRC_DIR}/H5Obogus.c
    ${HDF5_SRC_DIR}/H5Obtreek.c
    ${HDF5_SRC_DIR}/H5Ocache.c
    ${HDF5_SRC_DIR}/H5Ocache_image.c
    ${HDF5_SRC_DIR}/H5Ochunk.c
    ${HDF5_SRC_DIR}/H5Ocont.c
    ${HDF5_SRC_DIR}/H5Ocopy.c
    ${HDF5_SRC_DIR}/H5Odbg.c
    ${HDF5_SRC_DIR}/H5Odeprec.c
    ${HDF5_SRC_DIR}/H5Odrvinfo.c
    ${HDF5_SRC_DIR}/H5Odtype.c
    ${HDF5_SRC_DIR}/H5Oefl.c
    ${HDF5_SRC_DIR}/H5Ofill.c
    ${HDF5_SRC_DIR}/H5Oflush.c
    ${HDF5_SRC_DIR}/H5Ofsinfo.c
    ${HDF5_SRC_DIR}/H5Oginfo.c
    ${HDF5_SRC_DIR}/H5Oint.c
    ${HDF5_SRC_DIR}/H5Olayout.c
    ${HDF5_SRC_DIR}/H5Olinfo.c
    ${HDF5_SRC_DIR}/H5Olink.c
    ${HDF5_SRC_DIR}/H5Omessage.c
    ${HDF5_SRC_DIR}/H5Omtime.c
    ${HDF5_SRC_DIR}/H5Oname.c
    ${HDF5_SRC_DIR}/H5Onull.c
    ${HDF5_SRC_DIR}/H5Opline.c
    ${HDF5_SRC_DIR}/H5Orefcount.c
    ${HDF5_SRC_DIR}/H5Osdspace.c
    ${HDF5_SRC_DIR}/H5Oshared.c
    ${HDF5_SRC_DIR}/H5Oshmesg.c
    ${HDF5_SRC_DIR}/H5Ostab.c
    ${HDF5_SRC_DIR}/H5Otest.c
    ${HDF5_SRC_DIR}/H5Ounknown.c
)

set (H5O_HDRS
    ${HDF5_SRC_DIR}/H5Opublic.h
)
IDE_GENERATED_PROPERTIES ("H5O" "${H5O_HDRS}" "${H5O_SOURCES}" )

set (H5P_SOURCES
    ${HDF5_SRC_DIR}/H5P.c
    ${HDF5_SRC_DIR}/H5Pacpl.c
    ${HDF5_SRC_DIR}/H5Pdapl.c
    ${HDF5_SRC_DIR}/H5Pdcpl.c
    ${HDF5_SRC_DIR}/H5Pdeprec.c
    ${HDF5_SRC_DIR}/H5Pdxpl.c
    ${HDF5_SRC_DIR}/H5Pencdec.c
    ${HDF5_SRC_DIR}/H5Pfapl.c
    ${HDF5_SRC_DIR}/H5Pfcpl.c
    ${HDF5_SRC_DIR}/H5Pfmpl.c
    ${HDF5_SRC_DIR}/H5Pgcpl.c
    ${HDF5_SRC_DIR}/H5Pint.c
    ${HDF5_SRC_DIR}/H5Plapl.c
    ${HDF5_SRC_DIR}/H5Plcpl.c
    ${HDF5_SRC_DIR}/H5Pocpl.c
    ${HDF5_SRC_DIR}/H5Pocpypl.c
    ${HDF5_SRC_DIR}/H5Pstrcpl.c
    ${HDF5_SRC_DIR}/H5Ptest.c
)

set (H5P_HDRS
    ${HDF5_SRC_DIR}/H5Ppublic.h
)
IDE_GENERATED_PROPERTIES ("H5P" "${H5P_HDRS}" "${H5P_SOURCES}" )

set (H5PB_SOURCES
    ${HDF5_SRC_DIR}/H5PB.c
)

set (H5PB_HDRS
)
IDE_GENERATED_PROPERTIES ("H5PB" "${H5PB_HDRS}" "${H5PB_SOURCES}" )

set (H5PL_SOURCES
    ${HDF5_SRC_DIR}/H5PL.c
    ${HDF5_SRC_DIR}/H5PLint.c
    ${HDF5_SRC_DIR}/H5PLpath.c
    ${HDF5_SRC_DIR}/H5PLplugin_cache.c
)

set (H5PL_HDRS
    ${HDF5_SRC_DIR}/H5PLextern.h
    ${HDF5_SRC_DIR}/H5PLpublic.h
)
IDE_GENERATED_PROPERTIES ("H5PL" "${H5PL_HDRS}" "${H5PL_SOURCES}" )


set (H5R_SOURCES
    ${HDF5_SRC_DIR}/H5R.c
    ${HDF5_SRC_DIR}/H5Rdeprec.c
    ${HDF5_SRC_DIR}/H5Rint.c
)
set (H5R_HDRS
    ${HDF5_SRC_DIR}/H5Rpublic.h
)
IDE_GENERATED_PROPERTIES ("H5R" "${H5R_HDRS}" "${H5R_SOURCES}" )


set (H5RS_SOURCES
    ${HDF5_SRC_DIR}/H5RS.c
)
set (H5RS_HDRS
)
IDE_GENERATED_PROPERTIES ("H5RS" "${H5RS_HDRS}" "${H5RS_SOURCES}" )


set (H5S_SOURCES
    ${HDF5_SRC_DIR}/H5S.c
    ${HDF5_SRC_DIR}/H5Sall.c
    ${HDF5_SRC_DIR}/H5Sdbg.c
    ${HDF5_SRC_DIR}/H5Shyper.c
    ${HDF5_SRC_DIR}/H5Smpio.c
    ${HDF5_SRC_DIR}/H5Snone.c
    ${HDF5_SRC_DIR}/H5Spoint.c
    ${HDF5_SRC_DIR}/H5Sselect.c
    ${HDF5_SRC_DIR}/H5Stest.c
)

set (H5S_HDRS
    ${HDF5_SRC_DIR}/H5Spublic.h
)
IDE_GENERATED_PROPERTIES ("H5S" "${H5S_HDRS}" "${H5S_SOURCES}" )


set (H5SL_SOURCES
    ${HDF5_SRC_DIR}/H5SL.c
)
set (H5SL_HDRS
)
IDE_GENERATED_PROPERTIES ("H5SL" "${H5SL_HDRS}" "${H5SL_SOURCES}" )


set (H5SM_SOURCES
    ${HDF5_SRC_DIR}/H5SM.c
    ${HDF5_SRC_DIR}/H5SMbtree2.c
    ${HDF5_SRC_DIR}/H5SMcache.c
    ${HDF5_SRC_DIR}/H5SMmessage.c
    ${HDF5_SRC_DIR}/H5SMtest.c
)

set (H5SM_HDRS
)
IDE_GENERATED_PROPERTIES ("H5SM" "${H5SM_HDRS}" "${H5SM_SOURCES}" )


set (H5ST_SOURCES
    ${HDF5_SRC_DIR}/H5ST.c
)
set (H5ST_HDRS
)
IDE_GENERATED_PROPERTIES ("H5ST" "${H5ST_HDRS}" "${H5ST_SOURCES}" )


set (H5T_SOURCES
    ${HDF5_SRC_DIR}/H5T.c
    ${HDF5_SRC_DIR}/H5Tarray.c
    ${HDF5_SRC_DIR}/H5Tbit.c
    ${HDF5_SRC_DIR}/H5Tcommit.c
    ${HDF5_SRC_DIR}/H5Tcompound.c
    ${HDF5_SRC_DIR}/H5Tconv.c
    ${HDF5_SRC_DIR}/H5Tcset.c
    ${HDF5_SRC_DIR}/H5Tdbg.c
    ${HDF5_SRC_DIR}/H5Tdeprec.c
    ${HDF5_SRC_DIR}/H5Tenum.c
    ${HDF5_SRC_DIR}/H5Tfields.c
    ${HDF5_SRC_DIR}/H5Tfixed.c
    ${HDF5_SRC_DIR}/H5Tfloat.c
    ${HDF5_SRC_DIR}/H5Tnative.c
    ${HDF5_SRC_DIR}/H5Toffset.c
    ${HDF5_SRC_DIR}/H5Toh.c
    ${HDF5_SRC_DIR}/H5Topaque.c
    ${HDF5_SRC_DIR}/H5Torder.c
    ${HDF5_SRC_DIR}/H5Tpad.c
    ${HDF5_SRC_DIR}/H5Tprecis.c
    ${HDF5_SRC_DIR}/H5Tstrpad.c
    ${HDF5_SRC_DIR}/H5Tvisit.c
    ${HDF5_SRC_DIR}/H5Tvlen.c
)

set (H5T_HDRS
    ${HDF5_SRC_DIR}/H5Tpublic.h
)
IDE_GENERATED_PROPERTIES ("H5T" "${H5T_HDRS}" "${H5T_SOURCES}" )


set (H5TS_SOURCES
    ${HDF5_SRC_DIR}/H5TS.c
)
set (H5TS_HDRS
)
IDE_GENERATED_PROPERTIES ("H5TS" "${H5TS_HDRS}" "${H5TS_SOURCES}" )


set (H5UC_SOURCES
    ${HDF5_SRC_DIR}/H5UC.c
)
set (H5UC_HDRS
)
IDE_GENERATED_PROPERTIES ("H5UC" "${H5UC_HDRS}" "${H5UC_SOURCES}" )


set (H5VM_SOURCES
    ${HDF5_SRC_DIR}/H5VM.c
)
set (H5VM_HDRS
)
IDE_GENERATED_PROPERTIES ("H5VM" "${H5VM_HDRS}" "${H5VM_SOURCES}" )


set (H5WB_SOURCES
    ${HDF5_SRC_DIR}/H5WB.c
)
set (H5WB_HDRS
)
IDE_GENERATED_PROPERTIES ("H5WB" "${H5WB_HDRS}" "${H5WB_SOURCES}" )


set (H5Z_SOURCES
    ${HDF5_SRC_DIR}/H5Z.c
    ${HDF5_SRC_DIR}/H5Zdeflate.c
    ${HDF5_SRC_DIR}/H5Zfletcher32.c
    ${HDF5_SRC_DIR}/H5Znbit.c
    ${HDF5_SRC_DIR}/H5Zscaleoffset.c
    ${HDF5_SRC_DIR}/H5Zshuffle.c
    ${HDF5_SRC_DIR}/H5Zszip.c
    ${HDF5_SRC_DIR}/H5Ztrans.c
)
if (H5_ZLIB_HEADER)
  SET_PROPERTY(SOURCE ${HDF5_SRC_DIR}/H5Zdeflate.c PROPERTY
      COMPILE_DEFINITIONS H5_ZLIB_HEADER="${H5_ZLIB_HEADER}")
endif ()


set (H5Z_HDRS
    ${HDF5_SRC_DIR}/H5Zpublic.h
)
IDE_GENERATED_PROPERTIES ("H5Z" "${H5Z_HDRS}" "${H5Z_SOURCES}" )

set (common_SRCS
    ${H5_SOURCES}
    ${H5A_SOURCES}
    ${H5AC_SOURCES}
    ${H5B_SOURCES}
    ${H5B2_SOURCES}
    ${H5C_SOURCES}
    ${H5CS_SOURCES}
    ${H5CX_SOURCES}
    ${H5D_SOURCES}
    ${H5E_SOURCES}
    ${H5EA_SOURCES}
    ${H5F_SOURCES}
    ${H5FA_SOURCES}
    ${H5FD_SOURCES}
    ${H5FL_SOURCES}
    ${H5FO_SOURCES}
    ${H5FS_SOURCES}
    ${H5G_SOURCES}
    ${H5HF_SOURCES}
    ${H5HG_SOURCES}
    ${H5HL_SOURCES}
    ${H5HP_SOURCES}
    ${H5I_SOURCES}
    ${H5L_SOURCES}
    ${H5MF_SOURCES}
    ${H5MM_SOURCES}
    ${H5MP_SOURCES}
    ${H5O_SOURCES}
    ${H5P_SOURCES}
    ${H5PB_SOURCES}
    ${H5PL_SOURCES}
    ${H5R_SOURCES}
    ${H5UC_SOURCES}
    ${H5RS_SOURCES}
    ${H5S_SOURCES}
    ${H5SL_SOURCES}
    ${H5SM_SOURCES}
    ${H5ST_SOURCES}
    ${H5T_SOURCES}
    ${H5TS_SOURCES}
    ${H5VM_SOURCES}
    ${H5WB_SOURCES}
    ${H5Z_SOURCES}
)

set (H5_PUBLIC_HEADERS
    ${H5_HDRS}
    ${H5A_HDRS}
    ${H5AC_HDRS}
    ${H5B_HDRS}
    ${H5B2_HDRS}
    ${H5C_HDRS}
    ${H5D_HDRS}
    ${H5E_HDRS}
    ${H5EA_HDRS}
    ${H5F_HDRS}
    ${H5FA_HDRS}
    ${H5FD_HDRS}
    ${H5FS_HDRS}
    ${H5G_HDRS}
    ${H5HF_HDRS}
    ${H5HG_HDRS}
    ${H5HL_HDRS}
    ${H5I_HDRS}
    ${H5L_HDRS}
    ${H5MF_HDRS}
    ${H5MM_HDRS}
    ${H5MP_HDRS}
    ${H5O_HDRS}
    ${H5P_HDRS}
    ${H5PB_HDRS}
    ${H5PL_HDRS}
    ${H5R_HDRS}
    ${H5S_HDRS}
    ${H5SM_HDRS}
    ${H5T_HDRS}
    ${H5Z_HDRS}
)

set (H5_PRIVATE_HEADERS
    ${HDF5_SRC_DIR}/H5private.h

    ${HDF5_SRC_DIR}/H5Apkg.h
    ${HDF5_SRC_DIR}/H5Aprivate.h

    ${HDF5_SRC_DIR}/H5ACpkg.h
    ${HDF5_SRC_DIR}/H5ACprivate.h

    ${HDF5_SRC_DIR}/H5Bpkg.h
    ${HDF5_SRC_DIR}/H5Bprivate.h

    ${HDF5_SRC_DIR}/H5B2pkg.h
    ${HDF5_SRC_DIR}/H5B2private.h

    ${HDF5_SRC_DIR}/H5Clog.h
    ${HDF5_SRC_DIR}/H5Cpkg.h
    ${HDF5_SRC_DIR}/H5Cprivate.h

    ${HDF5_SRC_DIR}/H5CSprivate.h

    ${HDF5_SRC_DIR}/H5CXprivate.h

    ${HDF5_SRC_DIR}/H5Dpkg.h
    ${HDF5_SRC_DIR}/H5Dprivate.h

    ${HDF5_SRC_DIR}/H5Edefin.h
    ${HDF5_SRC_DIR}/H5Einit.h
    ${HDF5_SRC_DIR}/H5Epkg.h
    ${HDF5_SRC_DIR}/H5Eprivate.h
    ${HDF5_SRC_DIR}/H5Eterm.h

    ${HDF5_SRC_DIR}/H5EApkg.h
    ${HDF5_SRC_DIR}/H5EAprivate.h

    ${HDF5_SRC_DIR}/H5Fpkg.h
    ${HDF5_SRC_DIR}/H5Fprivate.h

    ${HDF5_SRC_DIR}/H5FApkg.h
    ${HDF5_SRC_DIR}/H5FAprivate.h

    ${HDF5_SRC_DIR}/H5FDpkg.h
    ${HDF5_SRC_DIR}/H5FDprivate.h

    ${HDF5_SRC_DIR}/H5FLprivate.h

    ${HDF5_SRC_DIR}/H5FOprivate.h

    ${HDF5_SRC_DIR}/H5FSpkg.h
    ${HDF5_SRC_DIR}/H5FSprivate.h

    ${HDF5_SRC_DIR}/H5Gpkg.h
    ${HDF5_SRC_DIR}/H5Gprivate.h

    ${HDF5_SRC_DIR}/H5HFpkg.h
    ${HDF5_SRC_DIR}/H5HFprivate.h

    ${HDF5_SRC_DIR}/H5HGpkg.h
    ${HDF5_SRC_DIR}/H5HGprivate.h

    ${HDF5_SRC_DIR}/H5HLpkg.h
    ${HDF5_SRC_DIR}/H5HLprivate.h

    ${HDF5_SRC_DIR}/H5HPprivate.h

    ${HDF5_SRC_DIR}/H5Ipkg.h
    ${HDF5_SRC_DIR}/H5Iprivate.h

    ${HDF5_SRC_DIR}/H5Lpkg.h
    ${HDF5_SRC_DIR}/H5Lprivate.h

    ${HDF5_SRC_DIR}/H5MFprivate.h

    ${HDF5_SRC_DIR}/H5MMprivate.h

    ${HDF5_SRC_DIR}/H5MPpkg.h
    ${HDF5_SRC_DIR}/H5MPprivate.h

    ${HDF5_SRC_DIR}/H5Opkg.h
    ${HDF5_SRC_DIR}/H5Oprivate.h
    ${HDF5_SRC_DIR}/H5Oshared.h

    ${HDF5_SRC_DIR}/H5Ppkg.h
    ${HDF5_SRC_DIR}/H5Pprivate.h

    ${HDF5_SRC_DIR}/H5PBprivate.h
    ${HDF5_SRC_DIR}/H5PBpkg.h

    ${HDF5_SRC_DIR}/H5PLpkg.h
    ${HDF5_SRC_DIR}/H5PLprivate.h

    ${HDF5_SRC_DIR}/H5Rpkg.h
    ${HDF5_SRC_DIR}/H5Rprivate.h

    ${HDF5_SRC_DIR}/H5RSprivate.h

    ${HDF5_SRC_DIR}/H5Spkg.h
    ${HDF5_SRC_DIR}/H5Sprivate.h

    ${HDF5_SRC_DIR}/H5SLprivate.h

    ${HDF5_SRC_DIR}/H5SMpkg.h
    ${HDF5_SRC_DIR}/H5SMprivate.h

    ${HDF5_SRC_DIR}/H5STprivate.h

    ${HDF5_SRC_DIR}/H5Tpkg.h
    ${HDF5_SRC_DIR}/H5Tprivate.h

    ${HDF5_SRC_DIR}/H5TSprivate.h

    ${HDF5_SRC_DIR}/H5UCprivate.h

    ${HDF5_SRC_DIR}/H5VMprivate.h

    ${HDF5_SRC_DIR}/H5WBprivate.h

    ${HDF5_SRC_DIR}/H5Zpkg.h
    ${HDF5_SRC_DIR}/H5Zprivate.h

    ${HDF5_SRC_DIR}/H5win32defs.h
)

set (H5_GENERATED_HEADERS
    ${HDF5_SRC_DIR}/H5Edefin.h
    ${HDF5_SRC_DIR}/H5Einit.h
    ${HDF5_SRC_DIR}/H5Epubgen.h
    ${HDF5_SRC_DIR}/H5Eterm.h
    ${HDF5_SRC_DIR}/H5version.h
    ${HDF5_SRC_DIR}/H5overflow.h
)

set (H5_PUBLIC_GENERATED_HEADERS
    ${HDF5_SRC_DIR}/H5Epubgen.h
    ${HDF5_SRC_DIR}/H5version.h
    ${HDF5_SRC_DIR}/H5overflow.h
)

if (FALSE) # XXX(kitware): Hardcode settings.
option (HDF5_GENERATE_HEADERS "Rebuild Generated Files" ON)
else ()
set(HDF5_GENERATE_HEADERS OFF)
endif ()
if (HDF5_GENERATE_HEADERS)
  set_source_files_properties(${H5_GENERATED_HEADERS} PROPERTIES GENERATED TRUE)
  find_package (Perl)
  if (PERL_FOUND)
    execute_process (
        COMMAND ${PERL_EXECUTABLE} ${HDF5_SOURCE_DIR}/bin/make_err ${HDF5_SRC_DIR}/H5err.txt OUTPUT_VARIABLE SCRIPT_OUTPUT
    )
    message(STATUS ${SCRIPT_OUTPUT})
    execute_process (
        COMMAND ${PERL_EXECUTABLE} ${HDF5_SOURCE_DIR}/bin/make_vers ${HDF5_SRC_DIR}/H5vers.txt OUTPUT_VARIABLE SCRIPT_OUTPUT
    )
    message(STATUS ${SCRIPT_OUTPUT})
    execute_process (
        COMMAND ${PERL_EXECUTABLE} ${HDF5_SOURCE_DIR}/bin/make_overflow ${HDF5_SRC_DIR}/H5overflow.txt OUTPUT_VARIABLE SCRIPT_OUTPUT
    )
    message(STATUS ${SCRIPT_OUTPUT})
  else ()
    message (STATUS "Cannot generate headers - perl not found")
  endif ()
endif ()

#-----------------------------------------------------------------------------
# Setup the H5detect utility which generates H5Tinit with platform
# specific type checks inside
#-----------------------------------------------------------------------------
if (HDF5_USE_PREGEN)
  set (HDF5_GENERATED_SOURCE_DIR ${HDF5_USE_PREGEN_DIR})
else ()
  set (HDF5_GENERATED_SOURCE_DIR ${HDF5_BINARY_DIR})
endif ()

if (BUILD_SHARED_LIBS)
  file (MAKE_DIRECTORY "${HDF5_BINARY_DIR}/shared")
endif ()

if (LOCAL_BATCH_TEST)
  if (LOCAL_BATCH_SCRIPT_COMMAND STREQUAL "raybsub")
    configure_file (
        ${HDF5_SOURCE_DIR}/bin/batch/${LOCAL_BATCH_SCRIPT_COMMAND}
        ${HDF5_BINARY_DIR}/${LOCAL_BATCH_SCRIPT_COMMAND} ESCAPE_QUOTES @ONLY
    )
  endif ()
  if (LOCAL_BATCH_SCRIPT_NAME)
    configure_file (
        ${HDF5_SOURCE_DIR}/bin/batch/${LOCAL_BATCH_SCRIPT_NAME}.in.cmake
        ${HDF5_BINARY_DIR}/${LOCAL_BATCH_SCRIPT_NAME} ESCAPE_QUOTES @ONLY
    )
  endif ()
  if (LOCAL_BATCH_SCRIPT_PARALLEL_NAME)
    configure_file (
        ${HDF5_SOURCE_DIR}/bin/batch/${LOCAL_BATCH_SCRIPT_PARALLEL_NAME}.in.cmake
        ${HDF5_BINARY_DIR}/${LOCAL_BATCH_SCRIPT_PARALLEL_NAME} ESCAPE_QUOTES @ONLY
    )
  endif ()
endif ()

if (NOT EXISTS "${HDF5_GENERATED_SOURCE_DIR}/H5Tinit.c")
  add_executable (H5detect ${HDF5_SRC_DIR}/H5detect.c)
  target_include_directories (H5detect PRIVATE "${HDF5_SRC_DIR};${HDF5_BINARY_DIR};$<$<BOOL:${HDF5_ENABLE_PARALLEL}>:${MPI_C_INCLUDE_DIRS}>")
  target_compile_definitions(H5detect PUBLIC ${HDF_EXTRA_C_FLAGS} ${HDF_EXTRA_FLAGS})
  TARGET_C_PROPERTIES (H5detect STATIC)
  target_link_libraries (H5detect
      PRIVATE "$<$<BOOL:${HDF5_ENABLE_PARALLEL}>:${MPI_C_LIBRARIES}>" $<$<OR:$<PLATFORM_ID:Windows>,$<PLATFORM_ID:MinGW>>:ws2_32.lib>
  )
  target_compile_options (H5detect
      PRIVATE "$<$<PLATFORM_ID:Emscripten>:-O0>"
  )
  set_target_properties(H5detect
    PROPERTIES
      OUTPUT_NAME "vtkH5detect")

  if (HDF5_BATCH_H5DETECT)
    configure_file (
        ${HDF5_SOURCE_DIR}/bin/batch/${HDF5_BATCH_H5DETECT_SCRIPT}.in.cmake
        ${HDF5_BINARY_DIR}/${HDF5_BATCH_H5DETECT_SCRIPT} ESCAPE_QUOTES @ONLY
    )
    add_custom_command (
        OUTPUT ${HDF5_GENERATED_SOURCE_DIR}/H5Tinit.c
               ${HDF5_GENERATED_SOURCE_DIR}/gen_SRCS.stamp1
        COMMAND ${HDF5_BATCH_CMD}
        ARGS ${HDF5_BINARY_DIR}/${HDF5_BATCH_H5DETECT_SCRIPT}
        COMMAND ${CMAKE_COMMAND}
        ARGS -E echo "Executed batch command to create H5Tinit.c"
        COMMAND    ${CMAKE_COMMAND}
        ARGS       -E touch ${HDF5_GENERATED_SOURCE_DIR}/gen_SRCS.stamp1
        DEPENDS H5detect
        WORKING_DIRECTORY ${HDF5_GENERATED_SOURCE_DIR}
    )
    add_custom_target (gen_H5Tinit
        COMMAND ${CMAKE_COMMAND} -P ${HDF5_SOURCE_DIR}/config/cmake/wait_H5Tinit.cmake
    )
    if (BUILD_SHARED_LIBS)
      add_custom_command (
          OUTPUT     ${HDF5_GENERATED_SOURCE_DIR}/shared/H5Tinit.c
                     ${HDF5_GENERATED_SOURCE_DIR}/shared/shared_gen_SRCS.stamp1
          COMMAND    ${CMAKE_COMMAND}
          ARGS       -E copy_if_different "${HDF5_GENERATED_SOURCE_DIR}/H5Tinit.c" "${HDF5_GENERATED_SOURCE_DIR}/shared/H5Tinit.c"
          COMMAND    ${CMAKE_COMMAND}
          ARGS       -E touch ${HDF5_GENERATED_SOURCE_DIR}/shared/shared_gen_SRCS.stamp1
          DEPENDS    gen_H5Tinit ${HDF5_GENERATED_SOURCE_DIR}/H5Tinit_created
          WORKING_DIRECTORY ${HDF5_GENERATED_SOURCE_DIR}
      )
      set_source_files_properties (${HDF5_GENERATED_SOURCE_DIR}/shared/H5Tinit.c PROPERTIES GENERATED TRUE)
    endif ()
  else ()
    add_custom_command (
        OUTPUT ${HDF5_GENERATED_SOURCE_DIR}/H5Tinit.c
               ${HDF5_GENERATED_SOURCE_DIR}/gen_SRCS.stamp1
        COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:H5detect>
        ARGS ${HDF5_GENERATED_SOURCE_DIR}/H5Tinit.c
        COMMAND    ${CMAKE_COMMAND}
        ARGS       -E touch ${HDF5_GENERATED_SOURCE_DIR}/gen_SRCS.stamp1
        DEPENDS H5detect
        WORKING_DIRECTORY ${HDF5_GENERATED_SOURCE_DIR}
    )
    if (BUILD_SHARED_LIBS)
      add_custom_command (
          OUTPUT     ${HDF5_GENERATED_SOURCE_DIR}/shared/H5Tinit.c
                     ${HDF5_GENERATED_SOURCE_DIR}/shared/shared_gen_SRCS.stamp1
          COMMAND    ${CMAKE_COMMAND}
          ARGS       -E copy_if_different "${HDF5_GENERATED_SOURCE_DIR}/H5Tinit.c" "${HDF5_GENERATED_SOURCE_DIR}/shared/H5Tinit.c"
          COMMAND    ${CMAKE_COMMAND}
          ARGS       -E touch ${HDF5_GENERATED_SOURCE_DIR}/shared/shared_gen_SRCS.stamp1
          DEPENDS    ${HDF5_GENERATED_SOURCE_DIR}/H5Tinit.c
          WORKING_DIRECTORY ${HDF5_GENERATED_SOURCE_DIR}
      )
      set_source_files_properties (${HDF5_GENERATED_SOURCE_DIR}/shared/H5Tinit.c PROPERTIES GENERATED TRUE)
    endif ()
  endif ()
  set_source_files_properties (${HDF5_GENERATED_SOURCE_DIR}/H5Tinit.c PROPERTIES GENERATED TRUE)
else ()
  add_custom_command (
      OUTPUT ${HDF5_GENERATED_SOURCE_DIR}/gen_SRCS.stamp1
      COMMAND    ${CMAKE_COMMAND}
      ARGS       -E touch ${HDF5_GENERATED_SOURCE_DIR}/gen_SRCS.stamp1
      DEPENDS    ${HDF5_GENERATED_SOURCE_DIR}/H5Tinit.c
      WORKING_DIRECTORY ${HDF5_GENERATED_SOURCE_DIR}
  )
  if (BUILD_SHARED_LIBS)
    add_custom_command (
        OUTPUT     ${HDF5_GENERATED_SOURCE_DIR}/shared/shared_gen_SRCS.stamp1
        COMMAND    ${CMAKE_COMMAND}
        ARGS       -E copy_if_different "${HDF5_GENERATED_SOURCE_DIR}/H5Tinit.c" "${HDF5_GENERATED_SOURCE_DIR}/shared/H5Tinit.c"
        COMMAND    ${CMAKE_COMMAND}
        ARGS       -E touch ${HDF5_GENERATED_SOURCE_DIR}/shared/shared_gen_SRCS.stamp1
        DEPENDS    ${HDF5_GENERATED_SOURCE_DIR}/H5Tinit.c
        WORKING_DIRECTORY ${HDF5_GENERATED_SOURCE_DIR}
    )
    set_source_files_properties (${HDF5_GENERATED_SOURCE_DIR}/shared/H5Tinit.c PROPERTIES GENERATED TRUE)
  endif ()
endif ()

add_executable (H5make_libsettings ${HDF5_SRC_DIR}/H5make_libsettings.c)
target_include_directories (H5make_libsettings PRIVATE "${HDF5_SRC_DIR};${HDF5_BINARY_DIR};$<$<BOOL:${HDF5_ENABLE_PARALLEL}>:${MPI_C_INCLUDE_DIRS}>")
target_compile_definitions(H5make_libsettings PUBLIC ${HDF_EXTRA_C_FLAGS} ${HDF_EXTRA_FLAGS})
TARGET_C_PROPERTIES (H5make_libsettings STATIC)
target_link_libraries (H5make_libsettings
    PRIVATE "$<$<BOOL:${HDF5_ENABLE_PARALLEL}>:${MPI_C_LIBRARIES}>" $<$<OR:$<PLATFORM_ID:Windows>,$<PLATFORM_ID:MinGW>>:ws2_32.lib>
)
target_compile_options (H5make_libsettings
    PRIVATE "$<$<PLATFORM_ID:Emscripten>:-O0>"
)
set_target_properties(H5make_libsettings
  PROPERTIES
    OUTPUT_NAME "vtkH5make_libsettings")

add_custom_command (
    OUTPUT ${HDF5_BINARY_DIR}/H5lib_settings.c
           ${HDF5_BINARY_DIR}/gen_SRCS.stamp2
    COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:H5make_libsettings>
    ARGS ${HDF5_BINARY_DIR}/H5lib_settings.c
    COMMAND    ${CMAKE_COMMAND}
    ARGS       -E touch ${HDF5_GENERATED_SOURCE_DIR}/gen_SRCS.stamp2
    DEPENDS H5make_libsettings
    WORKING_DIRECTORY ${HDF5_BINARY_DIR}
)
set_source_files_properties (${HDF5_BINARY_DIR}/H5lib_settings.c PROPERTIES GENERATED TRUE)
if (BUILD_SHARED_LIBS)
  add_custom_command (
      OUTPUT ${HDF5_BINARY_DIR}/shared/H5lib_settings.c
             ${HDF5_BINARY_DIR}/shared/shared_gen_SRCS.stamp2
      COMMAND    ${CMAKE_COMMAND}
      ARGS       -E copy_if_different "${HDF5_BINARY_DIR}/H5lib_settings.c" "${HDF5_BINARY_DIR}/shared/H5lib_settings.c"
      COMMAND    ${CMAKE_COMMAND}
      ARGS       -E touch ${HDF5_GENERATED_SOURCE_DIR}/shared/shared_gen_SRCS.stamp2
      DEPENDS    ${HDF5_BINARY_DIR}/H5lib_settings.c
      WORKING_DIRECTORY ${HDF5_BINARY_DIR}
  )
  set_source_files_properties (${HDF5_BINARY_DIR}/shared/H5lib_settings.c PROPERTIES GENERATED TRUE)
endif ()

## all_packages="AC,B,B2,D,F,FA,FL,FS,HL,I,O,S,ST,T,Z"
#all_packages="AC,B2,CX,D,F,HL,I,O,S,ST,T,Z"
if (FALSE) # XXX(kitware): Hardcode settings.
option (HDF5_ENABLE_DEBUG_APIS "Turn on extra debug output in all packages" OFF)
else ()
set(HDF5_ENABLE_DEBUG_APIS OFF)
endif ()

#-----------------------------------------------------------------------------
# Add H5Tinit source to build - generated by H5detect/CMake at configure time
#-----------------------------------------------------------------------------
if (FALSE) # XXX(kitware): Use VTK's module system.
if (NOT ONLY_SHARED_LIBS)
  set (gen_SRCS ${HDF5_GENERATED_SOURCE_DIR}/H5Tinit.c ${HDF5_BINARY_DIR}/H5lib_settings.c)
  add_custom_target (gen_${HDF5_LIB_TARGET} ALL DEPENDS ${HDF5_GENERATED_SOURCE_DIR}/gen_SRCS.stamp1 ${HDF5_GENERATED_SOURCE_DIR}/gen_SRCS.stamp2)

  add_library (${HDF5_LIB_TARGET} STATIC ${common_SRCS} ${gen_SRCS} ${H5_PUBLIC_HEADERS} ${H5_PRIVATE_HEADERS} ${H5_GENERATED_HEADERS})
  target_include_directories (${HDF5_LIB_TARGET}
      PRIVATE "${HDF5_SRC_DIR};${HDF5_BINARY_DIR};$<$<BOOL:${HDF5_ENABLE_PARALLEL}>:${MPI_C_INCLUDE_DIRS}>"
      INTERFACE "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>"
  )
  target_compile_definitions(${HDF5_LIB_TARGET}
      PUBLIC
          ${HDF_EXTRA_C_FLAGS}
          ${HDF_EXTRA_FLAGS}
      PRIVATE
          $<$<BOOL:${HDF5_ENABLE_TRACE}>:H5_DEBUG_API>  # Enable tracing of the API
          $<$<BOOL:${HDF5_ENABLE_DEBUG_APIS}>:H5Z_DEBUG;H5T_DEBUG;H5ST_DEBUG;H5S_DEBUG;H5O_DEBUG;H5I_DEBUG;H5HL_DEBUG;H5F_DEBUG;H5D_DEBUG;H5B2_DEBUG;H5AC_DEBUG>
  )
  TARGET_C_PROPERTIES (${HDF5_LIB_TARGET} STATIC)
  target_link_libraries (${HDF5_LIB_TARGET}
      PRIVATE ${LINK_LIBS} ${LINK_COMP_LIBS} "$<$<BOOL:${HDF5_ENABLE_PARALLEL}>:${MPI_C_LIBRARIES}>"
      PUBLIC $<$<NOT:$<PLATFORM_ID:Windows>>:${CMAKE_DL_LIBS}>
  )
  if (NOT WIN32)
    target_link_libraries (${HDF5_LIB_TARGET}
      PRIVATE $<$<BOOL:${HDF5_ENABLE_THREADSAFE}>:Threads::Threads>
    )
  endif ()
  set_global_variable (HDF5_LIBRARIES_TO_EXPORT ${HDF5_LIB_TARGET})
  H5_SET_LIB_OPTIONS (${HDF5_LIB_TARGET} ${HDF5_LIB_NAME} STATIC 0)
  set_target_properties (${HDF5_LIB_TARGET} PROPERTIES FOLDER libraries)
  add_dependencies (${HDF5_LIB_TARGET} gen_${HDF5_LIB_TARGET})

  set (install_targets ${HDF5_LIB_TARGET})
endif ()

else ()
set (gen_SRCS ${HDF5_GENERATED_SOURCE_DIR}/H5Tinit.c ${HDF5_BINARY_DIR}/H5lib_settings.c)
add_custom_target (gen_${HDF5_LIB_TARGET} ALL DEPENDS ${HDF5_GENERATED_SOURCE_DIR}/gen_SRCS.stamp1 ${HDF5_GENERATED_SOURCE_DIR}/gen_SRCS.stamp2)

add_library(vtkhdf5_src ${common_SRCS} ${gen_SRCS} ${H5_PUBLIC_HEADERS} ${H5_PRIVATE_HEADERS} ${H5_GENERATED_HEADERS})
target_include_directories(vtkhdf5_src
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${HDF5_BINARY_DIR}
  PUBLIC
    "$<BUILD_INTERFACE:${HDF5_BINARY_DIR}>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
    "$<INSTALL_INTERFACE:${_vtk_build_HEADERS_DESTINATION}/vtkhdf5>"
    "$<INSTALL_INTERFACE:${_vtk_build_HEADERS_DESTINATION}/vtkhdf5/src>")
vtk_module_install_headers(
  FILES   ${H5_PUBLIC_HEADERS}
          ${H5_GENERATED_HEADERS}
  SUBDIR  "vtkhdf5/src")
if (BUILD_SHARED_LIBS)
  target_compile_definitions(vtkhdf5_src
    PUBLIC
      H5_BUILT_AS_DYNAMIC_LIB
    PRIVATE
      hdf5_shared_EXPORTS)
endif ()
target_link_libraries(vtkhdf5_src
  PRIVATE
    VTK::zlib)
_vtk_module_apply_properties(vtkhdf5_src)
_vtk_module_install(vtkhdf5_src)

set(_vtk_add_module_output_name "vtkhdf5")
if (_vtk_build_LIBRARY_NAME_SUFFIX)
  string(APPEND _vtk_add_module_output_name "-${_vtk_build_LIBRARY_NAME_SUFFIX}")
endif ()

set_target_properties(vtkhdf5_src
  PROPERTIES
    OUTPUT_NAME "${_vtk_add_module_output_name}")
endif ()

set (install_targets ${HDF5_LIB_TARGET})

if (BUILD_SHARED_LIBS AND FALSE) # XXX(kitware): VTK handles shared/static flags.
  set (shared_gen_SRCS ${HDF5_GENERATED_SOURCE_DIR}/shared/H5Tinit.c ${HDF5_BINARY_DIR}/shared/H5lib_settings.c)
  add_custom_target (gen_${HDF5_LIBSH_TARGET} ALL DEPENDS ${HDF5_GENERATED_SOURCE_DIR}/shared/shared_gen_SRCS.stamp1 ${HDF5_GENERATED_SOURCE_DIR}/shared/shared_gen_SRCS.stamp2)

  add_library (${HDF5_LIBSH_TARGET} SHARED ${common_SRCS} ${shared_gen_SRCS} ${H5_PUBLIC_HEADERS} ${H5_PRIVATE_HEADERS} ${H5_GENERATED_HEADERS})
  target_include_directories (${HDF5_LIBSH_TARGET}
      PRIVATE "${HDF5_SRC_DIR};${HDF5_BINARY_DIR};$<$<BOOL:${HDF5_ENABLE_PARALLEL}>:${MPI_C_INCLUDE_DIRS}>"
      PUBLIC "$<$<BOOL:${HDF5_ENABLE_HDFS}>:${HDFS_INCLUDE_DIR}>"
      INTERFACE "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>"
  )
  target_compile_definitions(${HDF5_LIBSH_TARGET}
      PUBLIC
          "H5_BUILT_AS_DYNAMIC_LIB"
          ${HDF_EXTRA_C_FLAGS}
          ${HDF_EXTRA_FLAGS}
      PRIVATE
          $<$<BOOL:${HDF5_ENABLE_THREADSAFE}>:H5_HAVE_THREADSAFE>
          $<$<BOOL:${HDF5_ENABLE_TRACE}>:H5_DEBUG_API>  # Enable tracing of the API
          $<$<BOOL:${HDF5_ENABLE_DEBUG_APIS}>:H5Z_DEBUG;H5T_DEBUG;H5ST_DEBUG;H5S_DEBUG;H5O_DEBUG;H5I_DEBUG;H5HL_DEBUG;H5F_DEBUG;H5D_DEBUG;H5B2_DEBUG;H5AC_DEBUG>
  )
  TARGET_C_PROPERTIES (${HDF5_LIBSH_TARGET} SHARED)
  target_link_libraries (${HDF5_LIBSH_TARGET}
      PRIVATE ${LINK_LIBS} ${LINK_COMP_LIBS} "$<$<BOOL:${HDF5_ENABLE_PARALLEL}>:${MPI_C_LIBRARIES}>" $<$<BOOL:${HDF5_ENABLE_THREADSAFE}>:Threads::Threads>
      PUBLIC $<$<NOT:$<PLATFORM_ID:Windows>>:${CMAKE_DL_LIBS}>
  )
  set_global_variable (HDF5_LIBRARIES_TO_EXPORT "${HDF5_LIBRARIES_TO_EXPORT};${HDF5_LIBSH_TARGET}")
  H5_SET_LIB_OPTIONS (${HDF5_LIBSH_TARGET} ${HDF5_LIB_NAME} SHARED "LIB")
  set_target_properties (${HDF5_LIBSH_TARGET} PROPERTIES FOLDER libraries)
  add_dependencies (${HDF5_LIBSH_TARGET} gen_${HDF5_LIBSH_TARGET})

  set (install_targets ${install_targets} ${HDF5_LIBSH_TARGET})
endif ()

if (FALSE) # XXX(kitware): VTK handles installation.
#-----------------------------------------------------------------------------
# Add file(s) to CMake Install
#-----------------------------------------------------------------------------
if (NOT HDF5_INSTALL_NO_DEVELOPMENT)
  install (
      FILES
          ${H5_PUBLIC_HEADERS}
          ${H5_PUBLIC_GENERATED_HEADERS}
          ${HDF5_BINARY_DIR}/H5pubconf.h
      DESTINATION
          ${HDF5_INSTALL_INCLUDE_DIR}
      COMPONENT
          headers
  )
endif ()

#-----------------------------------------------------------------------------
# Add Target(s) to CMake Install for import into other projects
#-----------------------------------------------------------------------------
if (HDF5_EXPORTED_TARGETS)
  if (BUILD_SHARED_LIBS)
    INSTALL_TARGET_PDB (${HDF5_LIBSH_TARGET} ${HDF5_INSTALL_BIN_DIR} libraries)
  endif ()
  if (NOT ONLY_SHARED_LIBS)
    INSTALL_TARGET_PDB (${HDF5_LIB_TARGET} ${HDF5_INSTALL_LIB_DIR} libraries)
  endif ()

  install (
      TARGETS
          ${install_targets}
      EXPORT
          ${HDF5_EXPORTED_TARGETS}
      LIBRARY DESTINATION ${HDF5_INSTALL_LIB_DIR} COMPONENT libraries
      ARCHIVE DESTINATION ${HDF5_INSTALL_LIB_DIR} COMPONENT libraries
      RUNTIME DESTINATION ${HDF5_INSTALL_BIN_DIR} COMPONENT libraries
      FRAMEWORK DESTINATION ${HDF5_INSTALL_FWRK_DIR} COMPONENT libraries
      INCLUDES DESTINATION include
  )
endif ()

#-----------------------------------------------------------------------------
# Create pkgconfig files
#-----------------------------------------------------------------------------
set (_PKG_CONFIG_PREFIX ${CMAKE_INSTALL_PREFIX})
set (_PKG_CONFIG_EXEC_PREFIX \${prefix})
set (_PKG_CONFIG_LIBDIR \${exec_prefix}/lib)
set (_PKG_CONFIG_INCLUDEDIR \${prefix}/include)
set (_PKG_CONFIG_LIBNAME "${HDF5_LIB_CORENAME}")
set (_PKG_CONFIG_VERSION "${HDF5_PACKAGE_VERSION}")

foreach (libs ${LINK_LIBS} ${LINK_COMP_LIBS})
  set (_PKG_CONFIG_LIBS_PRIVATE "${_PKG_CONFIG_LIBS_PRIVATE} -l${libs}")
endforeach ()

if (NOT ONLY_SHARED_LIBS)
  set (_PKG_CONFIG_LIBS "${_PKG_CONFIG_LIBS} -l${HDF5_LIB_CORENAME}")
endif ()
if (BUILD_SHARED_LIBS)
  set (_PKG_CONFIG_SH_LIBS "${_PKG_CONFIG_SH_LIBS} -l${HDF5_LIB_CORENAME}")
endif ()

set (_PKG_CONFIG_REQUIRES)
set (_PKG_CONFIG_REQUIRES_PRIVATE)

configure_file (
    ${HDF_RESOURCES_DIR}/libhdf5.pc.in
    ${HDF5_BINARY_DIR}/CMakeFiles/${HDF5_LIB_CORENAME}-${HDF5_PACKAGE_VERSION}.pc
    @ONLY
)
install (
    FILES ${HDF5_BINARY_DIR}/CMakeFiles/${HDF5_LIB_CORENAME}-${HDF5_PACKAGE_VERSION}.pc
    DESTINATION ${HDF5_INSTALL_LIB_DIR}/pkgconfig
    COMPONENT libraries
)

if (NOT WIN32)
  set (_PKG_CONFIG_COMPILER ${CMAKE_C_COMPILER})
  configure_file (
      ${HDF_RESOURCES_DIR}/libh5cc.in
      ${HDF5_BINARY_DIR}/CMakeFiles/h5cc
      @ONLY
  )
  install (
      FILES ${HDF5_BINARY_DIR}/CMakeFiles/h5cc
      DESTINATION ${HDF5_INSTALL_BIN_DIR}
      PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
      COMPONENT libraries
  )
endif ()
endif ()
