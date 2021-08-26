#!/usr/bin/env python

from dataclasses import dataclass
from urllib.error import HTTPError
from urllib.request import urlretrieve
import pathlib
import re
import fileinput


# -------------------------------------------------------------------------------------------------
@dataclass(frozen=True)
class refs:
    raw_base_url: str = "https://raw.githubusercontent.com/sankhesh/vtk-examples/"
    base_url: str = "https://kitware.github.io/vtk-examples/"
    site_url: str = base_url + "site/"
    hash: str = "acc1229ce68f1f36e89133bdc67389db6ada44cb"
    selectExamples: str = "src/Python/Utilities/SelectExamples.py"
    img_base: str =\
        "https://raw.githubusercontent.com/Kitware/vtk-examples/gh-pages/src/Testing/Baseline/"


# -------------------------------------------------------------------------------------------------
def parse_arguments():
    import argparse

    description = 'Cross-reference vtk-examples with classes'
    epilog = '''
This script generates a 'Class to Examples' page cross-linking each class to a specific number of
examples that use that class. The examples are hyperlinked to the vtk-examples GitHub site.
'''

    parser = argparse.ArgumentParser(description=description, epilog=epilog,
                                     formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('-d', '--dir', type=pathlib.Path,
                        help='Directory containing the vtk classes')
    parser.add_argument('-t', '--tmpdir', type=pathlib.Path,
                        help='temp directory')
    parser.add_argument('-l', '--label', default="Online Examples",
                        help="Label to use in the documentation")
    parser.add_argument('-n', '--number', type=int, default=10,
                        help='Maximum number of examples to show')
    args = parser.parse_args()
    return args


# -------------------------------------------------------------------------------------------------
def download_file(dl_path, dl_url, overwrite=False):
    """
    Use the URL to get a file.

    :param dl_path: The path to download the file to.
    :param dl_url: The URL of the file.
    :param overwrite: If true, do a download even if the file exists.
    :return: The path to the file as a pathlib Path.
    """
    file_name = dl_url.split('/')[-1]
    # Create necessary sub-directories in the dl_path
    # (if they don't exist).
    pathlib.Path(dl_path).mkdir(parents=True, exist_ok=True)
    # Download if it doesn't exist in the directory overriding if overwrite is True.
    path = pathlib.Path(dl_path, file_name)
    if not path.is_file() or overwrite:
        try:
            urlretrieve(dl_url, path)
        except HTTPError as e:
            raise RuntimeError(f'Failed to download {dl_url}. {e.reason}')
    return path


# -------------------------------------------------------------------------------------------------
def preamble_for_label(label):
    """
    Return the preamble for the documentation block for the given label.

    :param label: The label to use as the paragraph title.
    :return: The string that should be preamble of the new section.
    """
    preamble = str("\n" + " "*4 + "@par " + label + ":\n\n")
    preamble += str(" "*4 + "@htmlonly\n\n")
    return preamble


# -------------------------------------------------------------------------------------------------
def conclusion_block():
    """
    Return the conclusion for the documentation block.

    :return: The string representing the conclusion of the documentation block added by this script.
    """
    return str("\n" + " "*4 + "@endhtmlonly\n")


# -------------------------------------------------------------------------------------------------
def docblock_from_example(example_url, label):
    """
    Return the documentation block to be inserted in the header file for the given example.

    :param example_url: The URL for the example page.
    :param label: The label to use as the header.
    :return: The documentation string to be inserted.
    """
    doc_block = "\n"

    e = decompose_exampleurl(example_url)
    if e is None:
        print("ERROR: Could not decompose example url to its language, module and name. Skipping..",
              example_url)
        return None
    doc_block += " "*6 + "<a href=\"" + example_url + "\">\n"
    example_img_url = imageurl_from_exampleurl(e[0], e[1], e[2])
    if example_img_url is not None:
        # Hover feedback container
        doc_block += " "*8 + "<div class=\"examplegrid_container\">\n"
        # Image url
        doc_block += " "*10 + "<img src=\"" + example_img_url + "\">\n"
        # Overlay
        doc_block += " "*10 + "<div class=\"examplegrid_overlay\">\n"
        # Overlay text
        doc_block += " "*12 + \
            "<div class=\"examplegrid_text\">" + e[2] + "</div>\n"
        # Close overlay
        doc_block += " "*10 + "</div>\n"
        # Close hover feedback container
        doc_block += " "*8 + "</div>\n"
    doc_block += " "*6 + "</a>\n"
    return doc_block


# -------------------------------------------------------------------------------------------------
def decompose_exampleurl(example_url):
    """
    Given an example url, decompose it to find the language, module and name.

    :param: example_url: The URL for the example page.
    :return: lang: The language that the example is written in.
    :return: module: The VTK module that the example belongs to.
    :return: name: The name of the example.
    """
    example_name_matcher_str = r'^{u}(Cxx|CSharp|Python)/(.*)/(.*)$'.format(
        u=refs.site_url)
    example_name_matcher = re.compile(example_name_matcher_str)
    example_name_match = example_name_matcher.match(example_url)
    if example_name_match is None:
        print("ERROR: Could not parse example name from url:",
              example_url, example_name_matcher_str)
        return None
    if len(example_name_match.groups()) != 3:
        print("ERROR: Example regex matcher could not find sufficient matches:",
              example_name_match.groups())
        return None
    return [i for i in example_name_match.groups()]

# -------------------------------------------------------------------------------------------------


def imageurl_from_exampleurl(lang, module, name):
    """
    Return a valid url for the image on the example page for given language, module and  example
    name.

    :param example_url: The URL for the example page.
    :return: The URL for the image or None if not found.
    """

    example_img_url = refs.img_base + \
        str("/") + lang + "/" + module + str("/Test") + name + str(".png")
    return example_img_url


# -------------------------------------------------------------------------------------------------
def main(indir, tmpdir, label, max_num):

    # download the select examples script from the vtk-examples repository
    selUrl = refs.raw_base_url + "/" + refs.hash + "/" + refs.selectExamples
    path = download_file(tmpdir, selUrl, overwrite=False)

    # import the select examples script
    import importlib.util
    spec = importlib.util.spec_from_file_location("SelectExamples", path)
    selectExamples = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(selectExamples)

    # iterate over all header files
    headerlist = pathlib.Path(indir).rglob('*.h')
    # eliminate_matcher
    eliminate_matcher = re.compile(r'^vtkCommand.h$')
    # class name matcher
    class_matcher = re.compile(r'^.*@class\s+(.*)')
    # end of documentation block matcher
    enddoc_matcher = re.compile(r'^.*\*\/')
    # label for the new documentation par
    label_matcher_str = r'^\s*\@par\s+{l}:\s*$'.format(l=label)
    label_matcher = re.compile(label_matcher_str)
    for headerfile in headerlist:
        # eliminate matcher
        if eliminate_matcher.match(headerfile.name) is not None:
            print("Ignoring header:", headerfile.name)
            continue
        classname = None
        classlineno = -1
        enddoclineno = -1
        labellineno = -1
        with open(headerfile, "r") as f:
            num = 0
            for line in f:
                if classlineno < 0:
                    class_match = class_matcher.match(line)
                    if class_match is not None:
                        # found the @class block
                        classlineno = num
                        classname = class_match.group(1)
                        print("Class name found @",
                              classlineno, ":", classname)
                elif enddoclineno < 0:
                    # Haven't found the end of the documentation block yet.
                    enddoc_match = enddoc_matcher.match(line)
                    # Is this the end of the documentation block?
                    if enddoc_match is None:
                        # This is not the end doc line.
                        # So, check if the xref is not already there
                        label_match = label_matcher.match(line)
                        if label_match is not None:
                            # The xref is already there.
                            labellineno = num
                            print("Label found", labellineno, ":", line)
                    else:
                        # end of doc found. Record and abort
                        enddoclineno = num
                        break
                num += 1

            # If we reach here and couldn't find either classname, enddocline number, etc.
            # This file might not have the standard documentation
            if classname is None or classlineno < 0 or enddoclineno < 0:
                print(
                    "ERROR: Could not process documentation block in file:", headerfile.name)
                continue
            elif labellineno > -1:
                # This file already contains the examples block
                print("INFO: Already contains par block:", headerfile.name)
                continue
        print("Colleced info:", classname, classlineno, labellineno, enddoclineno)
        t, examples = selectExamples.get_examples(selectExamples.get_crossref_dict(tmpdir),
                                                  classname, 'Cxx', all_values=False, number=max_num)
        if not examples or examples is None:
            print("INFO: No examples found for class", classname)
            continue
        num_found = len(examples)
        doxblock = preamble_for_label(label)
        # Add a new grid div
        doxblock += " "*4 + "<div class=\"examplegrid\">\n"
        for e in examples:
            d = docblock_from_example(e, label)
            doxblock += d + "\n" if d is not None else ""
        # Close the existing grid div
        doxblock += " "*4 + "</div>\n"
        doxblock += conclusion_block()

        # Finally, write the new documentation block to the header file
        with open(headerfile, "r") as f:
            contents = f.readlines()
        contents.insert(enddoclineno, doxblock)
        with open(headerfile, "w") as f:
            contents = "".join(contents)
            f.write(contents)


# -------------------------------------------------------------------------------------------------
if __name__ == "__main__":
    args = parse_arguments()

    main(args.dir, args.tmpdir, args.label, args.number)
