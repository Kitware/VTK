GitLab and Releases
===================

GitLab has support for milestones and they should be used for keeping track of
branches for the release. They allow keeping track of issues and merge requests
which should be "done" for the milestone to be considered complete. For each
release (including release candidates), a milestone should be created with a
plausible due date. The milestone page allows for an easy overview of branches
which need wrangling for a release.

Issues
------

Currently, Mantis is still used, there are no maintenance tasks on the GitLab
side here.

Merge Requests
--------------

Merge requests which need to be rebased onto the relevant release branch
should be marked with the `needs-rebase-for-release` tag and commented on how
the branch can be rebased properly:

    This branch is marked for a release, but includes other commits in
    `master`. Please either rebase the branch on top of the release branch and
    remove the `needs-rebase-for-release` tag from the merge request:

    ```sh
    $ git rebase --onto=origin/release origin/master $branch_name
    $ git gitlab-push -f
    ```

    or, if there are conflicts when using a single branch, open a new branch
    and open a merge request against the `release` branch:

    ```sh
    $ git checkout -b ${branch_name}-release $branch_name
    $ git rebase --onto=origin/release origin/master ${branch_name}-release
    $ git gitlab-push
    ```

    Thanks!

Wrangling Branches
------------------

Branches may be wrangled using the filters in the merge request page. Replace
`$release` at the end with the relevant milestone name:

    https://gitlab.kitware.com/vtk/vtk/merge_requests?state=all&milestone_title=$release

The following states of a merge request indicate where they are in the flow:

  - open for `master`: get into `master` first
  - open for `release`: ensure it is already in `master`
  - open with `needs-rebase-for-release` tag: wait for contributor to rebase
    properly; ping if necessary
  - `MERGED`: merge into `release`

There is currently no good way of marking a branch that went towards `master`
is also in `release` already since tags cannot be added to closed merge
requests. Suggestions welcome :) .
