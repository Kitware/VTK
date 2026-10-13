# AI Policy

VTK is a project whose contributions are expected to originate with humans. AI assistance is allowed under the guidelines below.
Generative AI is not just another tool, and as such special guidelines should be used when using it.
Note that general contribution guidelines [here](https://docs.vtk.org/en/latest/developers_guide/git/develop.html) still apply.

When using generative AI in the context of the VTK project, keep in mind the following principles:

- Understanding: Always review what you generate as if you wrote it yourself
- Accountability: Always assume responsibility for what you share and commit, even when AI generated
- Transparency: Always disclose when and how you are using generative AI
- Respect for reviewer/maintainer time

## For contributors

1. When commiting, use your own name or alias and do not add an AI tool as a co-author
2. When creating a merge request, disclose when and how you are using AI using the MR template
3. When running CI on unreviewed AI-generated MR, use draft status and write in the description that it is yet to be reviewed
4. Before asking for any review, review everything yourself and ensure everything is understood and done the right way
5. Always separate your contributions in different parts and merge them iteratively or separately to reduce the quantity of changes to aid for review, with a goal a maximum 1k lines change by MR
6. Always ensure generated comments, documentation and release notes are concise, so that it is comfortable to read for humans
7. If anything is unclear, not understood yet or too verbose, rework the generated code yourself until it is clearer and before asking for an actual review
8. When asked about code behavior during the review, do not copy and paste AI output, but answer questions yourself as you already know the answer as per above guideline

## For reviewers

1. If a contribution does not follow our contribution guidelines, please reference the relevant guidelines and request that they be addressed before a review can begin
2. If a contributor explicitly declines to make the requested changes or follow the guideline, invite a maintainer to help resolve the issue
3. You are free to ask any questions about how the code works and the contributor is expected to answer without copy-pasting AI
4. You are free to ask the contributor to make comments and documentation leaner and more readable
5. Do not approve (aka "+2") unless you have actually reviewed and understood the code, when only acknowledging a MR, use +1 instead
6. By reviewing and approving a change (aka "+2"), you take on responsibility for its code quality and design choices
