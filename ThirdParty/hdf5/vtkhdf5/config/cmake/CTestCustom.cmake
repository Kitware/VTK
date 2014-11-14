set (CTEST_CUSTOM_MAXIMUM_NUMBER_OF_WARNINGS 3000)
 
set (CTEST_CUSTOM_WARNING_EXCEPTION
    ${CTEST_CUSTOM_WARNING_EXCEPTION}
    "H5detect.c.[0-9]+.[ \t]*:[ \t]*warning C4090:"
    "H5detect.c.[0-9]+.[ \t]*:[ \t]*warning:[ \t]*passing argument"
    "H5detect.c[0-9 \t:]*warning:[ \t]*passing argument"
    "note.*expected.*void.*but argument is of type.*volatile"
    "H5Tconv.c[0-9 \t:]*warning:[ \t]*comparison is always false due to limited range of data type"
    "H5Ztrans.c.[0-9]+.[ \t]*:[ \t]*warning C4244"
    "SZIP.src.*:[ \t]*warning"
    "POSIX name for this item is deprecated"
    "disabling jobserver mode"
    "config.cmake.xlatefile.c"
    "warning.*implicit declaration of function"
    "note: expanded from macro"
#    "fpp:[ \t]*warning:[ \t]*cannot remove H5_DEBUG_API - not a predefined macro"
)
 
set (CTEST_CUSTOM_MEMCHECK_IGNORE
    ${CTEST_CUSTOM_MEMCHECK_IGNORE}
    H5TEST-flush1           #designed to fail
    H5TEST-flush2           #designed to need flush1
    H5TEST-error_test       #uses runTest.cmake
    H5TEST-err_compat       #uses runTest.cmake
    H5TEST-links_env        #uses runTest.cmake
    H5TEST-testlibinfo      #uses grepTest.cmake
    H5TEST-clear-testhdf5-objects
    H5TEST-clear-objects
    H5TEST-clear-cache-objects
    H5TEST-clear-cache_api-objects
    H5TEST-clear-ttsafe-objects
    H5TEST-clear-err_compat-objects
    H5TEST-clear-error_test-objects
    H5TEST-clear-links_env-objects
    PERFORM_h5perform-clear-objects
    HL_TOOLS-clear-objects
    HL_test-clear-objects
    HL_fortran_test-clear-objects
    ######### tools/h5copy #########
    H5COPY-clearall-objects
    ######### tools/h5diff #########
    H5DIFF-clearall-objects
    ######### tools/h5dump #########
    H5DUMP-clearall-objects
    H5DUMP_PACKED_BITS-clearall-objects
    H5DUMP-XML-clearall-objects
    ######### tools/h5import #########
    H5IMPORT-clear-objects
    ######### tools/h5jam #########
    H5JAM-SETUP-N_twithub_u10_c-clear-objects
    H5JAM-SETUP-N_twithub_u10_c
    H5JAM-N_twithub_u10_c-clear-objects
    H5JAM-NONE_COPY-N_twithub_u10_c
    H5JAM-CHECKFILE-N_twithub_u10_c-clear-objects
    H5JAM-SETUP-N_twithub_u511_c-clear-objects
    H5JAM-SETUP-N_twithub_u511_c
    H5JAM-N_twithub_u511_c-clear-objects
    H5JAM-NONE_COPY-N_twithub_u511_c
    H5JAM-CHECKFILE-N_twithub_u511_c-clear-objects
    H5JAM-SETUP-N_twithub_u512_c-clear-objects
    H5JAM-SETUP-N_twithub_u512_c
    H5JAM-N_twithub_u512_c-clear-objects
    H5JAM-NONE_COPY-N_twithub_u512_c
    H5JAM-CHECKFILE-N_twithub_u512_c-clear-objects
    H5JAM-SETUP-N_twithub_u513_c-clear-objects
    H5JAM-SETUP-N_twithub_u513_c
    H5JAM-N_twithub_u513_c-clear-objects
    H5JAM-NONE_COPY-N_twithub_u513_c
    H5JAM-CHECKFILE-N_twithub_u513_c-clear-objects
    H5JAM-SETUP-N_twithub513_u10_c-clear-objects
    H5JAM-SETUP-N_twithub513_u10_c
    H5JAM-N_twithub513_u10_c-clear-objects
    H5JAM-NONE_COPY-N_twithub513_u10_c
    H5JAM-CHECKFILE-N_twithub513_u10_c-clear-objects
    H5JAM-SETUP-N_twithub513_u511_c-clear-objects
    H5JAM-SETUP-N_twithub513_u511_c
    H5JAM-N_twithub513_u511_c-clear-objects
    H5JAM-NONE_COPY-N_twithub513_u511_c
    H5JAM-CHECKFILE-N_twithub513_u511_c-clear-objects
    H5JAM-SETUP-N_twithub513_u512_c-clear-objects
    H5JAM-SETUP-N_twithub513_u512_c
    H5JAM-N_twithub513_u512_c-clear-objects
    H5JAM-NONE_COPY-N_twithub513_u512_c
    H5JAM-CHECKFILE-N_twithub513_u512_c-clear-objects
    H5JAM-SETUP-N_twithub513_u513_c-clear-objects
    H5JAM-SETUP-N_twithub513_u513_c
    H5JAM-N_twithub513_u513_c-clear-objects
    H5JAM-NONE_COPY-N_twithub513_u513_c
    H5JAM-CHECKFILE-N_twithub513_u513_c-clear-objects
    H5JAM-CHECKFILE-twithub_u10_c-clear-objects
    H5JAM-twithub_u511_c-clear-objects
    H5JAM-CHECKFILE-twithub_u511_c-clear-objects
    H5JAM-twithub_u512_c-clear-objects
    H5JAM-CHECKFILE-twithub_u512_c-clear-objects
    H5JAM-twithub_u513_c-clear-objects
    H5JAM-CHECKFILE-twithub_u513_c-clear-objects
    H5JAM-twithub513_u10_c-clear-objects
    H5JAM-CHECKFILE-twithub513_u10_c-clear-objects
    H5JAM-twithub513_u511_c-clear-objects
    H5JAM-CHECKFILE-twithub513_u511_c-clear-objects
    H5JAM-twithub513_u512_c-clear-objects
    H5JAM-CHECKFILE-twithub513_u512_c-clear-objects
    H5JAM-twithub513_u513_c-clear-objects
    H5JAM-CHECKFILE-twithub513_u513_c-clear-objects
    H5JAM-SETUP-twithub_tall-clear-objects
    H5JAM-SETUP-twithub_tall
    H5JAM-UNJAM-twithub_tall-clear-objects
    H5JAM-UNJAM_D-twithub_tall-clear-objects
    H5JAM-CHECKFILE-twithub_tall-clear-objects
    H5JAM-SETUP-twithub513_tall-clear-objects
    H5JAM-SETUP-twithub513_tall
    H5JAM-UNJAM-twithub513_tall-clear-objects
    H5JAM-UNJAM_D-twithub513_tall-clear-objects
    H5JAM-CHECKFILE-twithub513_tall-clear-objects
    H5JAM-SETUP-N_twithub_tall-clear-objects
    H5JAM-SETUP-N_twithub_tall
    H5JAM-UNJAM-N_twithub_tall-clear-objects
    H5JAM-UNJAM_D-N_twithub_tall-clear-objects
    H5JAM-CHECKFILE-N_twithub_tall-clear-objects
    H5JAM-SETUP-N_twithub513_tall-clear-objects
    H5JAM-SETUP-N_twithub513_tall
    H5JAM-UNJAM-N_twithub513_tall-clear-objects
    H5JAM-UNJAM_D-N_twithub513_tall-clear-objects
    H5JAM-CHECKFILE-N_twithub513_tall-clear-objects
    H5JAM-SETUP-D_twithub_tall-clear-objects
    H5JAM-SETUP-D_twithub_tall
    H5JAM-UNJAM-D_twithub_tall-clear-objects
    H5JAM-UNJAM_D-D_twithub_tall-clear-objects
    H5JAM-CHECKFILE-D_twithub_tall-clear-objects
    H5JAM-SETUP-D_twithub513_tall-clear-objects
    H5JAM-SETUP-D_twithub513_tall
    H5JAM-UNJAM-D_twithub513_tall-clear-objects
    H5JAM-UNJAM_D-D_twithub513_tall-clear-objects
    H5JAM-CHECKFILE-D_twithub513_tall-clear-objects
    H5JAM-CHECKFILE-ta_u513-clear-objects
    H5JAM-twithub_u10-clear-objects
    H5JAM-CHECKFILE-twithub_u10-clear-objects
    H5JAM-twithub_u511-clear-objects
    H5JAM-CHECKFILE-twithub_u511-clear-objects
    H5JAM-twithub_u512-clear-objects
    H5JAM-CHECKFILE-twithub_u512-clear-objects
    H5JAM-twithub_u513-clear-objects
    H5JAM-CHECKFILE-twithub_u513-clear-objects
    H5JAM-twithub513_u10-clear-objects
    H5JAM-CHECKFILE-twithub513_u10-clear-objects
    H5JAM-twithub513_u511-clear-objects
    H5JAM-CHECKFILE-twithub513_u511-clear-objects
    H5JAM-twithub513_u512-clear-objects
    H5JAM-CHECKFILE-twithub513_u512-clear-objects
    H5JAM-twithub513_u513-clear-objects
    H5JAM-CHECKFILE-twithub513_u513-clear-objects
    H5JAM-twithub_u10_c-clear-objects
    H5JAM-tall_u10-clear-objects
    H5JAM-CHECKFILE-tall_u10-clear-objects
    H5JAM-tall_u511-clear-objects
    H5JAM-CHECKFILE-tall_u511-clear-objects
    H5JAM-tall_u512-clear-objects
    H5JAM-CHECKFILE-tall_u512-clear-objects
    H5JAM-tall_u513-clear-objects
    H5JAM-CHECKFILE-tall_u513-clear-objects
    H5JAM-SETUP-ta_u10-clear-objects
    H5JAM-SETUP-ta_u10
    H5JAM-ta_u10-clear-objects
    H5JAM-NONE_COPY-ta_u10
    H5JAM-CHECKFILE-ta_u10-clear-objects
    H5JAM-SETUP-ta_u511-clear-objects
    H5JAM-SETUP-ta_u511
    H5JAM-ta_u511-clear-objects
    H5JAM-NONE_COPY-ta_u511
    H5JAM-CHECKFILE-ta_u511-clear-objects
    H5JAM-SETUP-ta_u512-clear-objects
    H5JAM-SETUP-ta_u512
    H5JAM-ta_u512-clear-objects
    H5JAM-NONE_COPY-ta_u512
    H5JAM-CHECKFILE-ta_u512-clear-objects
    H5JAM-SETUP-ta_u513-clear-objects
    H5JAM-SETUP-ta_u513
    H5JAM-ta_u513-clear-objects
    H5JAM-NONE_COPY-ta_u513
    ######### tools/h5ls #########
    H5LS-clearall-objects
    ######### tools/h5repack #########
    H5REPACK-clearall-objects
    H5REPACK-gzip_verbose_filters                       #uses runTest.cmake
    H5REPACK_VERIFY_LAYOUT-dset2_chunk_20x10            #uses grepTest.cmake
    H5REPACK_VERIFY_LAYOUT_ALL-chunk_20x10              #uses grepTest.cmake
    H5REPACK_VERIFY_LAYOUT-dset2_conti                  #uses grepTest.cmake
    H5REPACK_VERIFY_LAYOUT_ALL-conti                    #uses grepTest.cmake
    H5REPACK_VERIFY_LAYOUT-dset2_compa                  #uses grepTest.cmake
    H5REPACK_VERIFY_LAYOUT_ALL-compa                    #uses grepTest.cmake
    H5REPACK_VERIFY_LAYOUT-dset_compa_conti             #uses grepTest.cmake
    H5REPACK_VERIFY_LAYOUT-dset_compa_chunk             #uses grepTest.cmake
    H5REPACK_VERIFY_LAYOUT-dset_compa_compa             #uses grepTest.cmake
    H5REPACK_VERIFY_LAYOUT-dset_conti_compa             #uses grepTest.cmake
    H5REPACK_VERIFY_LAYOUT-dset_conti_chunk             #uses grepTest.cmake
    H5REPACK_VERIFY_LAYOUT-dset_conti_conti             #uses grepTest.cmake
    H5REPACK_VERIFY_LAYOUT-chunk_compa                  #uses grepTest.cmake
    H5REPACK_VERIFY_LAYOUT-chunk_conti                  #uses grepTest.cmake
    H5REPACK_VERIFY_LAYOUT-chunk_18x13                  #uses grepTest.cmake
    H5REPACK_VERIFY_LAYOUT-contig_small_compa           #uses grepTest.cmake
    H5REPACK_VERIFY_LAYOUT-contig_small_fixed_compa     #uses grepTest.cmake
    H5REPACK_VERIFY_LAYOUT_ALL-layout_long_switches     #uses grepTest.cmake
    H5REPACK_VERIFY_LAYOUT_ALL-layout_short_switches    #uses grepTest.cmake
    H5REPACK-plugin
    ######### tools/h5stat #########
    H5STAT-clearall-objects
    ######### tools/misc #########
    H5REPART-clearall-objects
    H5MKGRP-clearall-objects
    ######### examples #########
    EXAMPLES-clear-objects
    CPP_ex-clear-objects
)
