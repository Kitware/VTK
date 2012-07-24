This directory contains a subset of the Freetype2 library (2.4.7) and
some custom changes that VTK needs.

We only include enough of the distribution to provide the functionalities
required by VTK.

We would like to thank the Freetype team for distributing this library.
http://www.freetype.org

Modifications
-------------
You can search for code for "VTK_FREETYPE_CHANGE" to find modifications
vs the original freetype code

Added Files
-----------

CMakeLists.txt
  -to support CMake builds

README.VTK.txt
  - this file

include/vtk_freetype_mangle.h
  -mangles all symbols exported from the freetype library
  -should be regenerated when updating freetype, see the file for instructions

include/vtk_ftmodule.h
  -new file, created from include/freetype/config/ftmodule.h
  -you'll need to manually merge changes from newer freetypes.
  -the changes from the file it's based on are marked with VTK_FREETYPE_CHANGE:
    -removed one module, which is not needed by VTK

include/vtkFreeTypeConfig.h.in
  -purpose unknown
  -does not appear to be based on a file from freetype itself

Changed Files
-------------
builds/unix/ftconfig.in:
  -use CMake variables

builds/unix/ftsystem.c:
  -include our configured ftconfig.h

include/ft2build.h:
  -extensive changes, see file for comments

include/freetype/config/ftoption.h:
  -disable FT_CONFIG_OPTION_INLINE_MULFIX, FT_CONFIG_OPTION_USE_LZW, FT_CONFIG_OPTION_USE_ZLIB, FT_CONFIG_OPTION_OLD_INTERNALS
  -conditional disabling of compiler warnings
  -additions to support DLL build for Windows

src/pshinter/pshalgo.c:
  -commented out piece of code to workaround a bug, see bug 10052.

searching for "VTK_FREETYPE_CHANGE" is a good idea too.

Update Freetype
---------------

The ThirdParty/freetype/vtkfreetype directory contains a reduced
distribution of the freetype source tree with only the library
source code needed by VTK.  It is not a submodule; the actual
content is part of our source tree and changes can be made and
committed directly.

We update from upstream using Git's "subtree" merge strategy.  A
special branch contains commits of upstream freetype snapshots and
nothing else.  No Git ref points explicitly to the head of this
branch, but it is merged into our history.

Update freetype from upstream as follows.  Create a local branch to
explicitly reference the upstream snapshot branch head:

 git branch freetype-upstream 6c09b8bd

Use a temporary directory to checkout the branch:

 mkdir freetype-tmp
 cd freetype-tmp
 git init
 git pull .. freetype-upstream
 rm -rf *

Now place the (reduced) freetype content in this directory.  See
instructions shown by

 git log 6c09b8bd

for help extracting the content from the upstream tarball.  Then run
the following commands to commit the new version.  Substitute the
appropriate date and version number:

 git add --all

 GIT_AUTHOR_NAME='FreeType Team' \
 GIT_AUTHOR_EMAIL='freetype@nongnu.org' \
 GIT_AUTHOR_DATE='Tue Oct 18 13:36:12 2011 +0200' \
 git commit -m 'freetype 2.4.7 (reduced)' &&
 git commit --amend

Edit the commit message to describe the procedure used to obtain the
content.  Then push the changes back up to the main local repository:

 git push .. HEAD:freetype-upstream
 cd ..
 rm -rf freetype-tmp

Create a topic in the main repository on which to perform the update:

 git checkout -b update-freetype master

Merge the freetype-upstream branch as a subtree:

 git merge -s recursive -X subtree=ThirdParty/freetype/vtkfreetype \
           freetype-upstream

If there are conflicts, resolve them and commit.  Build and test the
tree.  Commit any additional changes needed to succeed.

Finally, run

 git rev-parse --short=8 freetype-upstream

to get the commit from which the freetype-upstream branch must be started
on the next update.  Edit the "git branch freetype-upstream" line above to
record it, and commit this file.
