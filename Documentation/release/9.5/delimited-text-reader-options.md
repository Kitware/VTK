# vtkDelimitedTextReader improvements

## Preview

vtkDelimitedTextReader offers a `GetPreview` method to inspect the file content.
During the `RequestInformation` pass, it reads the `PreviewNumberOfLines`
first lines of the input file.
By default, nothing is done and Preview is empty.

This is useful to configure the reader options (like `XXXDelimiters` or `HaveHeaders`)
before the `RequestData` pass.

## SkippedRecords

The new `SkippedRecords` option allows to skip the first N records (usually lines).
This is useful as some file format may contain non standard header lines.
See for instance the [gslib](http://www.gslib.com/gslib_help/format.html) format.

## Comments

vtkDelimitedTextReader supports the concept of comments: starting at on of the given
`CommentCharacters`, the remaining data of the current record is skipped.
