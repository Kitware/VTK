The IO/Xdmf2 directory contains a reduced distribution of the
xdmf2/vtk (pv branch) source tree with only the library
source code needed by VTK.  It is not a submodule; the actual
content is part of our source tree and changes can be made and
committed directly.

We update from upstream using Git's "subtree" merge strategy.  A
special branch contains commits of upstream xdmf2/vtk snapshots and
nothing else.  No Git ref points explicitly to the head of this
branch, but it is merged into our history.

Update xdmf2/vtk from upstream as follows.  Create a local branch to
explicitly reference the upstream snapshot branch head:

 git branch xdmf2vtk-upstream f40916ae

Use a temporary directory to checkout the branch:

 mkdir xdmf2vtk-tmp
 cd xdmf2vtk-tmp
 git init
 git pull .. xdmf2vtk-upstream
 rm -rf *

Now place the (reduced) xdmf2/vtk content in this directory.  See
instructions shown by

 git log f40916ae

for help extracting the content from the upstream tarball.  Then run
the following commands to commit the new version.  Substitute the
appropriate date and version number:

 git add --all

 GIT_AUTHOR_NAME='XDMF Developers' \
 GIT_AUTHOR_EMAIL='xdmf@lists.kitware.com' \
 GIT_AUTHOR_DATE='2012-08-01 16:14:03 -0500' \
 git commit -m 'xdmf2/vtk 2012-08-01 (reduced)' &&
 git commit --amend

Edit the commit message to describe the procedure used to obtain the
content. Be sure to add a gerrit change-id line to the bottom of the
message. Then push the changes back up to the main local repository:

 git push .. HEAD:xdmf2vtk-upstream
 cd ..
 rm -rf xdmf2vtk-tmp

Create a topic in the main repository on which to perform the update:

 git checkout -b update-xdmf2 master

Merge the xdmf2vtk-upstream branch as a subtree:

 git merge -s recursive -X subtree=IO/Xdmf2 \
           xdmf2vtk-upstream

If there are conflicts, resolve them and commit.  Build and test the
tree.  Commit any additional changes needed to succeed.

Finally, run

 git rev-parse --short=8 xdmf2vtk-upstream

to get the commit from which the xdmf2vtk-upstream branch must be started
on the next update.  Edit the "git branch xdmf2vtk-upstream" line above to
record it, and commit this file.
