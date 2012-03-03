#! /bin/bash

#  Shell script to pull updated .po files from the translation project

## Copyright (C) 2006, 2007, 2011 Free Software Foundation, Inc.
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 3, or (at your option)
## any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 51 Franklin Street, Fifth Floor,
## Boston, MA  02110-1301, USA.

host=http://translationproject.org
uri=${host}/domain/sharutils.html

dir=$(dirname $0)
cd ${dir}
test -d Download && rm -rf Download
mkdir Download
cd Download || exit 1

# Pull the list of translations for sharutils
#
wget -nd ${uri}

# Extract all the .po file names
#
list=$(
  sed -n -e 's@.* href="\.\./@@' -e 's/\.po".*/.po/p' sharutils.html | (
      nl=$'\n'
      last_pol=''
      last_po=''
      lang_list=''

      while read po
      do
          pol=$(echo "$po" | sed 's/\.po$//;s/.*\.//')
          test "X${last_pol}" = "X${pol}" || {
              test ${#last_po} -gt 0 && echo "lang_$last_pol='$last_po'"
              lang_list=${lang_list}${last_pol}${nl}
              last_pol=$pol
          }
          last_po=$po
      done
      echo "lang_$last_pol='$last_po'"
      echo "lang_list='${lang_list}${last_pol}'"
) )

eval "$list"

# For each unique language name, get the latest translation and see
# if it is newer than the one we have.  Replace if so.
#
for f in ${lang_list}
do
  eval uri=\${lang_${f}}
  file=$(basename ${uri})
  wget ${host}/${uri}
  test -f ${file} || {
    echo no file ${file}
    exit 1
  }

  test -f ../${f}.po || {
    mv -f ${file} ../${f}.po
    ( cd .. ; cvs add ${f}.po )
    continue
  }

  curr=$(egrep '^"PO-Revision-Date:' ../${f}.po)
  proj=$(egrep '^"PO-Revision-Date:' ${file})

  # Unchanged?  Continue
  test "X${curr}" = "X${proj}" && continue

  new=$(printf "$curr\n$proj" | sort | tail -1)
  # current is newer?  Continue
  test "${curr}" = "${new}" && continue

  # update!
  mv -f ${file} ../${f}.po
done

cd ..
rm -rf Download
