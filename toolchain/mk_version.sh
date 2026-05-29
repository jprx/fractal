#!/bin/bash
# Generates src/version.c, which contains build-time strings

echo "// AUTO-GENERATED: DO NOT EDIT (or do if you want, I don't care.)"
echo "#include <fractal.h>"
echo "const char *fractal_version_builder = \""`whoami`"\";"
echo "const char *fractal_version_build_date = \"`date`\";"
echo "const char *fractal_version_str = \"fractal-\" ARCH_STRING \"-`git rev-parse --abbrev-ref HEAD`-`git rev-list --count HEAD`\";"
echo "const char *fractal_version_num_str = \"`git rev-parse --abbrev-ref HEAD`-`git rev-list --count HEAD`\";"
