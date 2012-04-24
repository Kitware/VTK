In order to update VTK's libxml2 version, see below.

MOST RECENTLY MERGED TAG: VTK-libxml2-upstream-2-6-27

------------------------------------------------------------------------------
This libxml2 was converted from upstream as follows:

1.) Export libxml2 source from SVN:

  svn export http://svn.gnome.org/svn/libxml2/tags/LIBXML2_2_6_27 \
             libxml2-2.6.27

2.) Fix trio*.h files:
  "Id"  -->  "Id: ..."

3.) Remove extra directories and files, fix permissions:

  rm -rf doc result python optim bakefile xstc vms macos example test win32
  rm *.py *.pl *.sh install-sh test*.c test*.h ac* missing mkinstalldirs TODO*
  rm README* NEWS MAINTAINERS INSTALL HACKING
  rm global.data ChangeLog chvalid.def configure.in
  rm Makefile* include/Makefile* include/libxml/Makefile*
  rm libxml-2.0* libxml.3 libxml.spec.in libxml.m4 xml2* include/win32config.h
  chmod 644 chvalid.c

4.) Checkin files to VTK-libxml2-upstream branch.
    Create the tag VTK-libxml2-upstream-2-6-27 on the branch.
    VTK-specific changes are then performed on the main tree.

5.) Mangle with vtk_libxml2_mangle.h included in include/libxml/xmlexports.h.
    Hack up globals.c and include/libxml/globals.h to fix some mangling.

6.) Hack up include/libxml/*.h to use double-quote includes without
    libxml/ prefix.  This should cause each header to include the
    other headers it needs from the same directory.
------------------------------------------------------------------------------
To upgrade to a newer libxml2 version:

1.) Follow steps 1-3 above.
2.) Commit the new upstream version on the VTK-libxml2-upstream branch.
    Create the tag VTK-libxml2-upstream-<major-minor-patch> to indicate
    the version.

3.) Merge the changes between the previous and current versions to the
    main tree:

  cvs update -dAP
  cvs tag VTK-libxml2-<new>-merge-pre
  cvs update -d -j VTK-libxml2-upstream-<old> -j VTK-libxml2-upstream-<new>

4.) Manually resolve conflicts and commit.

  cvs commit -m "ENH: Updating libxml2 to <new>"
  cvs tag VTK-libxml2-<new>-merge-post

5.) Update the tag at the top of this file indicating the most recently
    merged version.
