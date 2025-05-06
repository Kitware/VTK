# Contributing to Viskores

We are currently in the process of adopting software from [VTK-m]. During
this transitional period, we have no formal method of accepting
contributions outside of our core contributors. For the time being, new
contributions should be directed at the [VTK-m](https://m.vtk.org) project.

When Viskores is ready to accept its own features, the contributing process
will be similar to [that of VTK-m's](https://gitlab.kitware.com/vtk/vtk-m/-/blob/main/CONTRIBUTING.md).

The author of all contributions must agree to the [Developer Certificate of
Origin] sign-off. Any developer who submits a contribution to Viskores
implicitly agrees to these conditions. That said, contributors remain as
the copyright holder for independent works of authorship and that no
contributor or copyright holder will be required to assign copyrights to
Viskores.


## Requirements ##

This page documents how to develop Viskores through [Git](http://git-scm.com).

Git is an extremely powerful version control tool that supports many
different "workflows" for individual development and collaboration. Here we
document procedures used by the Viskores development community. In the
interest of simplicity and brevity we do *not* provide an explanation of
why we use this approach.


## Setup ##

Before you begin, perform initial setup:

1.  Create an account in [GitHub] and select a user name.

2.  [Fork Viskores] into your user's namespace on [GitHub].

3.  Use Git to create a local clone of the main VTK repository:

        $ git clone git@github.com:viskores/viskores.git
        $ cd viskores

    The main repository will be configured as your `origin` remote.

4.  Run the developer setup script to prepare your Viskores work tree and
    create Git command aliases used below:

        $ ./Utilities/SetupForDevelopment.sh

5. (Optional but highly recommended.)
    Install the official [GitHub CLI utility] on your developing station. This
    tool greatly simplifies interacting with GitHub from the command line.

6. (Optional but highly recommended.)
    [Register with the Viskores dashboard] on Kitware's CDash instance to
    better know how your code performs in regression tests. After
    registering and signing in, click on "All Dashboards" link in the upper
    left corner, scroll down and click "Subscribe to this project" on the
    right of Viskores.

7.  (Optional but highly recommended.)
    [Join us in the Viskores discussions] to communicate with other developers
    and users.


## Workflow ##

Viskores development uses a [branchy workflow] based on topic branches. Our
collaboration workflow consists of three main steps:

1.  Local Development:
      * [Update](#update)
      * [Create a Topic](#create-a-topic)

2.  Code Review (requires [GitHub Access]):
      * [Share a Topic](#share-a-topic)
      * [Create a pull request](#create-a-merge-request)
      * [Review a pull request](#review-a-merge-request)
      * [Reformat a Topic](#reformat-a-topic)
      * [Revise a Topic](#revise-a-topic)

3.  Integrate Changes:
      * [Merge a Topic](#merge-a-topic) (requires permission in GitHub)


## Update ##

1.  Update your local `main` branch:

        $ git checkout main
        $ git pull

2.  Optionally push `main` to your fork in GitHub:

        $ git push github main


## Create a Topic ##

All new work must be committed on topic branches. Name topics like you
might name functions: concise but precise. A reader should have a general
idea of the feature or fix to be developed given just the branch name.

1.  To start a new topic branch:

        $ git fetch origin

2.  For new development, start the topic from `origin/main`:

        $ git checkout -b my-topic origin/main

3.  Edit files and create commits (repeat as needed):

        $ edit file1 file2 file3
        $ git add file1 file2 file3
        $ git commit

    Caveats:
      * Data files must be placed under a folder explicitly named 'data' at the
        root level in the source directory. This is required as Viskores uses a
        git submodule to efficiently support data files.

4.  If you are adding a new feature or making significant changes to API,
    make sure to add a entry to `docs/changelog`. This allows release
    notes to properly capture all relevant changes.

### Guidelines for Commit Messages ###

Remember to *motivate & summarize*. When writing commit messages. Get into
the habit of creating messages that have enough information for any
developer to read and glean relevant information such as:

1.  Is this change important and why?
2.  If addressing an issue, which issue(s)?
3.  If a new feature, why is it useful and/or necessary?
4.  Are there background references or documentation?

A short description of what the issue being addressed and how will go a
long way towards making the log more readable and the software more
maintainable. Viskores requires that your message start with a single subject
line, followed by a blank line, followed by the message body which contains
the more detailed explanatory text for the commit. You can consider a
commit message to very similar to an email with the first line being the
subject of an email and the rest of the text as the body.

Style guidelines for commit messages are as follows:

1.   Separate subject from body with a blank line
2.   Limit the subject line to 78 characters
3.   Capitalize the subject line
4.   Use the imperative mood in the subject line e.g. "Refactor foo" or
     "Fix Issue #12322", instead of "Refactoring foo", or "Fixing issue
     #12322".
5.   Wrap the body at 80 characters
6.   Use the body to explain `what` and `why` and if applicable a brief
    `how`.


## Share a Topic ##

When a topic is ready for review and possible inclusion, share it by
pushing to a fork of your repository in GitHub. Be sure you have registered
and signed in for [GitHub Access] and created your fork by visiting the
main [Viskores GitHub] repository page and using the "Fork" button in the
upper right.

1.  Checkout the topic if it is not your current branch:

        $ git checkout my-topic

2.  Push commits in your topic branch to your fork in GitHub:

        $ git push -u github

    Notes:
      * If you are revising a previously pushed topic and have rewritten
        the topic history, add `-f` or `--force` to overwrite the
        destination.
      * If you have created or modified data files (inside the `data/`
        directory), you will need to push them separately by opening a pull
        request at https://github.com/viskores/viskores-data.

    The output will include a link to the topic branch in your fork in
    GitHub and a link to a page for creating a pull request.


## Create a pull request ##

When you [pushed your topic branch](#share-a-topic), it will provide you
with a url of the form

    https://github.com/<username>/viskores/pull/new/<branch_name>

You can copy/paste that into your web browser to create a new merge
request. Alternately, you can visit your fork in GitHub, browse to the
"**pull requests**" link on the left, and use the "**New pull request**"
button in the upper right.

Once at the create pull request page, follow these steps. Many of these
will be filled out for you.

1.  In the "**Source branch**" box select the `<username>/viskores` repository
    and the `my-topic` branch.

2.  In the "**Base branch**" box select the `viskores/viskores` repository and
    the `main` branch. It should be the default.

3.  Use the "**Compare branches**" button to proceed to the next page and
    fill out the pull request creation form.

4.  In the "**Title**" field provide a one-line summary of the entire
    topic. This will become the title of the pull request.

    Example pull request Title:

        Add OpenMP Device Adapter

5.  In the "**Description**" field provide a high-level description of the
    change the topic makes and any relevant information about how to try
    it.
    *   Use `@username` syntax to draw attention of specific developers.
        This syntax may be used anywhere outside literal text and code
        blocks.  Or, wait until the [next step](#review-a-merge-request)
        and add comments to draw attention of developers.
    *   Optionally use a fenced code block with type `message` to specify
        text to be included in the generated merge commit message when the
        topic is [merged](#merge-a-topic).

    Example pull request Description:

        This branch adds a new device adapter that uses new OpenMP 4+ features
        including Task groups to better handle unbalanced and irregular domains

        ```message
        Add a OpenMP 4+ task-based device adapter.
        ```

        Cc: @user1 @user2

6.  The "**Assignees**", "**Milestone**", and "**Labels**" fields may be
    left blank.

7.  Enable the "** Allow edits and access to secrets by maintainers.**" option,
    so that reviewers can modify the pull request. This allows reviewers to change
    minor style issues without overwhelming the author with change requests.

7.  Use the "**Open pull request**" button to create the pull request
    and visit its page.

### Guidelines for pull requests ###

Remember to *motivate & summarize*. When creating a pull request, consider
the reviewers and future users of the software. Provide enough information to
motivate the pull request such as:

1.  Is this pull request important and why?
2.  If addressing an issue, which issue(s)?
3.  If a new feature, why is it useful and/or necessary?
4.  Are there background references or documentation?

Also provide a summary statement expressing what you did and if there is a
choice in implementation or design pattern, the rationale for choosing a
certain path. Notable software or data features should be mentioned as
well.

A well written pull request will motivate your reviewers, and bring them
up to speed faster. Future software developers will be able to understand
the reasons why something was done, and possibly avoid chasing down dead
ends, Although it may take you a little more time to write a good merge
request, you’ll likely see payback in faster reviews and better understood
and maintainable software.


## Review a pull request ##

Add comments mentioning specific developers using `@username` syntax to
draw their attention and have the topic reviewed. After typing `@` and some
text, GitHub will offer completions for developers whose real names or user
names match.

Comments use [GitHub Flavored Markdown] for formatting. See GitHub
documentation on [Special GitHub References] to add links to things like
pull requests and commits in other repositories.


### Reviews ###

Reviewers may add comments providing feedback or to acknowledge their
approval. All comments use the [GitHub Flavored Markdown].

#### Fetching Changes ####

One may fetch the changes associated with a pull request by using either using
the `gh pr checkout` command line shown at the top of the pull request page or
by manually using `git fetch` to fetch the git head of the pull request as
shown below:

    $ git fetch origin pull/$branch/head

This updates the local `FETCH_HEAD` to refer to the branch.

There are a few options for checking out the changes in a work tree:

  * One may checkout the branch:

        $ git checkout FETCH_HEAD -b $branch

    or checkout the commit without creating a local branch:

        $ git checkout FETCH_HEAD

  * Or, one may cherry-pick the commits to minimize rebuild time:

        $ git cherry-pick ..FETCH_HEAD


### Testing ###

Each time a pull request is created or updated automated testing
is automatically triggered, and shows up under the pipeline tab.

Developers can track the status of the pipeline for a merge
request by using the Pipeline tab on a pull request or by
clicking on stage icons as shown below:

![alt text](docs/build_stage.png "Pipeline")

When trying to diagnose why a build or tests stage has failed it
generally is easier to look at the pruned information reported
on [Viskores's CDash Dashboard](https://open.cdash.org/index.php?project=VISKORES).
To make it easier to see only the results for a given pull request
you can click the `cdash` link under the external stage ( rightmost pipeline stage icon )

![alt text](docs/external_stage.png "CDash Link")

## Revise a Topic ##

Revising a topic is a special way to modify the commits within a topic.
Normally during a review of a pull request a developer will resolve issues
brought up during review by adding more commits to the topic. While this is
sufficient for most issues, some issues can only be resolved by rewriting
the history of the topic.

### Starting Revisions ###

Regardless of what revisions need to be made, you first must make sure that
your topic is the current branch. To make your topic branch current:

    $ git checkout my-topic

(As always, you can get the current branch with the `git status` command.)

A common need for revisions is to update your topic branch to the latest
version of Viskores. Even if you a revising your topic branch for some other
reason, also updating to the latest main is usually not a bad idea. To
update to the latest commit in main, you need to make sure that the
latest commit is in your local repository. To do that run

    $ git pull-main

### Make Local Revisions ###

The easiest way to make changes to the commits on your topic branch is to
rebase it to the main branch:

    $ git rebase -i main

When you run this command, git will open your text editor with a list of
all the commits that will be changed. The first word of each lines
indicates a command for that commit. By default, all commits are `pick`ed,
which means that they will be simply passed without change.

If you need to revise the commit message of one of the commits, then
replace `pick` with `reword` (or just `r`). When the rebase starts, a new
editor will be provided to let you change the commit message.

If you need to make changes to files within a commit, then replace `pick`
with `edit` (or just `e`). When the rebase gets to this commit, it will
pause to let you make changes to the files.

If you need to merge commits together, use the `squash` (or `s`) command.
The rebase will give you a change to edit the commit message of the merged
commit.

Once you exit your editor, the rebase will begin. If you have requested to
edit any commits or if git detects a conflict while applying a commit, it
will stop running so that you can make changes. Make the changes you need,
use `git add` to stage those changes, and then use

    $ git rebase --continue

to have git continue the rebase process. You can always run `git status` to
get help about what to do next.

### Push to GitHub ###

To push commits in your topic branch to your fork in GitHub:

    $ git push -f

Note: You need have the `-f` or `--force` to overwrite the destination as
you are revising a previously pushed topic and have rewritten the topic
history.

## Merge a Topic ##

After a topic has been reviewed and approved in a GitHub pull request,
authorized developers may accept the pull request and merge by clicking the
merge button.

### Merge Success ###

If the merge succeeds the topic will appear in the upstream repository
`main` branch and the pull request will be closed automatically.

### Merge Failure ###

If the merge fails (likely due to a conflict), fetch the latest upstream
history and rebase on it:

    $ git fetch origin
    $ git rebase origin/main

Return to the [above step](#share-a-topic) to share the revised topic.

## Fixing Problems ##

There are a lot of instructions in this document, and if you are not
familiar with contributing to Viskores, you may get your repository in a bad
state that will cause problems with the other instructions. This section
attempts to capture common problems contributors have and the fixes for
them.

### Wrong origin Remote ###

The Viskores contribution workflow assumes that your `origin` remote is
attached to the main Viskores GitHub repository. If it is not, that will cause
problems with updating your repository. To check which remote repository
origin refers to, run

    $ git remote -v

It will give you a list of remotes and their URLs that you have configured.
If you have a line like

    origin  https://github.com/viskores/viskores.git (fetch)

or

    origin  git@github.com:viskores/viskores.git (fetch)

then everything is OK. If it is anything else (for example, it has your
GitHub username in it), then you have a problem. Fortunately, you can fix
it by simply changing the remote's URL:

    $ git remote set-url origin https://github.com/Viskores/viskores.git

### main Not Tracking origin ###

The instructions in this document assume that your `main` branch is
tracking the remote `main` branch at `origin` (which, as specified above,
should be the main Viskores repository). This should be set up if you
correctly cloned the main Viskores repository, but can get accidentally
changed.

To check which remote branch `main` is tracking, call

    $ git rev-parse --abbrev-ref --symbolic-full-name main@{upstream}

Git should respond with `origin/main`. If it responds with anything else,
you need to reset the tracking:

    $ git branch -u origin/main main

### Local Edits on the main Branch ###

The first step in the [contributing workflow](#workflow) is that you
[create a topic branch](#create-a-topic) on which to make changes. You are
not supposed to add your commits directly to `main`. However, it is easy
to forget to create the topic branch.

To find out if you have local commits on your main branch, check its
status:

    $ git checkout main
    $ git status

If status responds that your branch is up to date or that your branch is
_behind_ the `origin/main` remote branch, then everything is fine. (If
your branch is behind you might want to update it with `git pull`.)

If the status responds that your branch and `origin/main` have diverged
or that your branch is _ahead_ of `origin/main`, then you have local
commits on the main branch. Those local commits need to move to a topic
branch.

1.  Create a topic branch:

        $ git branch my-topic

    Of course, replace `my-topic` with something that better describes your
    changes.

2.  Reset the local main branch to the remote main branch:

        $ git reset --hard origin/main

3.  Check out the topic branch to continue working on it:

        $ git checkout my-topic

[branchy workflow]: http://public.kitware.com/Wiki/Git/Workflow/Topic
[Developer Certificate of Origin]: http://developercertificate.org/
[Fork Viskores]: https://github.com/Viskores/viskores/fork
[GitHub Flavored Markdown]: https://github.github.com/gfm/
[GitHub]: https://github.com
[GitHub CLI utility]: https://cli.github.com/
[Join us in the Viskores discussions]: https://github.com/Viskores/viskores/discussions
[Register with the Viskores dashboard]: https://open.cdash.org/register
[Special GitHub References]: https://docs.github.com/en/issues/tracking-your-work-with-issues/using-issues/linking-a-pull-request-to-an-issue
[Viskores GitHub]: https://github.com/viskores/viskores
