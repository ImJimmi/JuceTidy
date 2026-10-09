#!/usr/bin/env python3
"""
Runs a single clang-tidy check from the JuceTidy plugin over a test file, then
verifies that it warns on exactly the lines marked with:

    int value = 0; // EXPECT_WARNING

Check options can be set with:

    // OPTION: SomeOption=some value

The language standard can be overridden with:

    // STD: c++17

The test fails if a marked line has no warning, or if there's a warning or
error on any other line.
"""

import argparse
import pathlib
import re
import subprocess
import sys

EXPECT_WARNING = re.compile(r"//\s*EXPECT_WARNING\b")
OPTION_DIRECTIVE = re.compile(r"//\s*OPTION:\s*([A-Za-z]+)=(.*)$", re.MULTILINE)
STD_DIRECTIVE = re.compile(r"//\s*STD:\s*(\S+)", re.MULTILINE)
DIAGNOSTIC = re.compile(r"^(.+?):(\d+):\d+: (warning|error):", re.MULTILINE)


def quote(value):
    return "'" + value.replace("'", "''") + "'"


def make_config(check, source):
    options = ", ".join(
        f"{quote(check + '.' + name)}: {quote(value.strip())}"
        for name, value in OPTION_DIRECTIVE.findall(source)
    )
    return f"{{Checks: {quote('-*,' + check)}, CheckOptions: {{{options}}}}}"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--clang-tidy", required=True)
    parser.add_argument("--plugin", required=True)
    parser.add_argument("--check", required=True)
    parser.add_argument("test_file", type=pathlib.Path)
    parser.add_argument("compiler_args", nargs="*")
    args = parser.parse_args()

    source = args.test_file.read_text()
    standard = [f"-std={match}" for match in STD_DIRECTIVE.findall(source)]

    result = subprocess.run(
        [
            args.clang_tidy,
            f"--load={args.plugin}",
            f"--config={make_config(args.check, source)}",
            "--quiet",
            str(args.test_file),
            "--",
            *args.compiler_args,
            *standard,
        ],
        capture_output=True,
        text=True,
    )

    if "unknown check" in result.stderr or "Error loading" in result.stderr:
        sys.stderr.write(result.stderr)
        return 1

    test_file = args.test_file.resolve()
    expected = {
        (test_file, number, "warning")
        for number, line in enumerate(source.splitlines(), start=1)
        if EXPECT_WARNING.search(line)
    }
    reported = {
        (pathlib.Path(path).resolve(), int(line), severity)
        for path, line, severity in DIAGNOSTIC.findall(result.stdout)
    }

    for path, line, _ in sorted(expected - reported):
        sys.stderr.write(f"{path}:{line}: expected a warning\n")

    for path, line, severity in sorted(reported - expected):
        sys.stderr.write(f"{path}:{line}: unexpected {severity}\n")

    if reported != expected:
        print(result.stdout)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
