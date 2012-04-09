The Utilities/vtkhdf5 directory contains a reduced distribution of
the hdf5 source tree with only the library source code and CMake
build system.  It is not a submodule; the actual content is part
of our source tree and changes can be made and committed directly.

We update from upstream using Git's "subtree" merge strategy.
A special branch contains commits of upstream hdf5 snapshots and
nothing else.  No Git ref points explicitly to the head of this
branch, but it is merged into our history.

Update hdf5 from upstream as follows.  Create a local branch to
explicitly reference the upstream snapshot branch head:

 git branch hdf5-upstream 8e37142d

Use a temporary directory to checkout the branch:

 mkdir hdf5-tmp
 cd hdf5-tmp
 git init
 git pull .. hdf5-upstream
 rm -rf *

Now place the (reduced) hdf5 content in this directory.  See
instructions shown by

 git log 8e37142d

for help extracting the content from the upstream svn repo.  Then run
the following commands to commit the new version.  Substitute the
appropriate date and version number:

 git add -u
 git add .

 GIT_AUTHOR_NAME='HDF Group' \
 GIT_AUTHOR_EMAIL='hdf-forum@hdfgroup.org' \
 GIT_AUTHOR_DATE='2010-08-17 02:34:15 -0400' \
 git commit -m 'hdf5 1.8.5-r19589 (reduced)' &&
 git commit --amend

Edit the commit message to describe the procedure used to obtain the
content.  Then push the changes back up to the main repository:

 git push .. HEAD:hdf5-upstream
 cd ..
 rm -rf hdf5-tmp

Create a topic in the main repository on which to perform the update:

 git checkout -b update-hdf5 master

Merge the hdf5-upstream branch as a subtree:

 git merge -s subtree hdf5-upstream

If there are conflicts, resolve them and commit.  Build and test the
tree.  Commit any additional changes needed to succeed.

Finally, run

 git rev-parse --short=8 hdf5-upstream

to get the commit from which the hdf5-upstream branch must be started
on the next update.  Edit the "git branch hdf5-upstream" line above to
record it, and commit this file.
