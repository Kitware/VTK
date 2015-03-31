The ThirdParty/verdict/vtkverdict directory contains a reduced
distribution of the Verdict source tree with only the library
source code needed by VTK.  It is not a submodule; the actual
content is part of our source tree and changes can be made and
committed directly.

We update from upstream using Git's "subtree" merge strategy.  A
special branch contains commits of upstream verdict snapshots and
nothing else.  No Git ref points explicitly to the head of this
branch, but it is merged into our history.

Update verdict from upstream as follows.  Create a local branch to
explicitly reference the upstream snapshot branch head:

 git branch verdict-upstream 42017336

Use a temporary directory to checkout the branch:

 mkdir verdict-tmp
 cd verdict-tmp
 git init
 git pull .. verdict-upstream
 rm -rf *

Now place the (reduced) verdict content in this directory.  See
instructions shown by

 git log 42017336

for help extracting the content from the upstream tarball.  Then run
the following commands to commit the new version.  Substitute the
appropriate date and version number:

 git add --all

 GIT_AUTHOR_NAME='VTK Developers' \
 GIT_AUTHOR_EMAIL='vtk-developers@vtk.org' \
 GIT_AUTHOR_DATE='2012-08-24 14:00:00 -0400' \
 git commit -m 'verdict 2012-08-24 (reduced)' &&
 git commit --amend

Edit the commit message to describe the procedure used to obtain the
content.  Then push the changes back up to the main local repository:

 git push .. HEAD:verdict-upstream
 cd ..
 rm -rf verdict-tmp

Create a topic in the main repository on which to perform the update:

 git checkout -b update-verdict master

Merge the verdict-upstream branch as a subtree:

 git merge -s recursive -X subtree=ThirdParty/verdict/vtkverdict \
           verdict-upstream

If there are conflicts, resolve them and commit.  Build and test the
tree.  Commit any additional changes needed to succeed.

Finally, run

 git rev-parse --short=8 verdict-upstream

to get the commit from which the verdict-upstream branch must be started
on the next update.  Edit the "git branch verdict-upstream" line above to
record it, and commit this file.
