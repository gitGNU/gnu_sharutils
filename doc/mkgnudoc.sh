#! /bin/sh

# Time-stamp: "2011-01-14 17:41:02 bkorb"
# Version:    "$Revision: 1.6 $

## Create the manual hierarchy
##
## Copyright (C) 2005-2007, 2011 Free Software Foundation, Inc.
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
## You should have received a copy of the GNU General Public License along
## with this program; if not, write to the Free Software Foundation, Inc.,
## 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

MAKE=${MAKE:-make}
pkgsrcdir=`dirname $0`
pkgsrcdir=`cd ${pkgsrcdir} ; pwd`
set -x
test "X$1" = "XGNU" && shift
pkg="${1}"
ver="${2}"
shift ; shift

ddir=${pkg}-${ver}
test -d ${ddir} && rm -rf ${ddir}
mkdir ${ddir} || {
  echo cannot make directory ${ddir} >&2
  exit 1
}

( cd ${ddir}
  dirlist='html_mono info text dvi pdf ps texi'
  mkdir ${dirlist} || {
    echo cannot make subdirectories: >&2
    echo ${dirlist} >&2
    exit 1
  }
) || exit 1

echo
echo "Making documentation hierarchy for ${ddir}"
echo

test -f ${pkg}.info || ${MAKE}

texiargs='--ifinfo -menu -verbose'

texi2html ${texiargs} -split=none ${pkg}.texi
mv -f ${pkg}.html ${ddir}/html_mono/.
echo mono done

texi2html ${texiargs} -split=chapter ${pkg}.texi
if test -d ${pkg}/.
then mv -f ${pkg} ${ddir}/html_chapter
else mkdir ${ddir}/html_chapter
     mv -f ${pkg}*.htm* ${ddir}/html_chapter/.
fi
echo chapter done

texi2html ${texiargs} -split=node ${pkg}.texi
if test -d ${pkg}/.
then mv -f ${pkg} ${ddir}/html_node
else mkdir ${ddir}/html_node
     mv -f ${pkg}*.htm* ${ddir}/html_node/.
fi
echo node done

for f in ${pkg}*.info*
do gzip -c $f > ${ddir}/info/$f.gz
done

test -f ${pkg}.ps  || ${MAKE} ${pkg}.ps  || exit 1
test -f ${pkg}.txt || ${MAKE} ${pkg}.txt || exit 1
test -f ${pkg}.pdf || ${MAKE} ${pkg}.pdf || exit 1

gzip -c ${pkg}.dvi  > ${ddir}/dvi/${pkg}.dvi.gz   || exit 1
gzip -c ${pkg}.pdf  > ${ddir}/pdf/${pkg}.pdf.gz   || exit 1
gzip -c ${pkg}.ps   > ${ddir}/ps/${pkg}.ps.gz     || exit 1
gzip -c ${pkg}.texi > ${ddir}/texi/${pkg}.texi.gz || exit 1
gzip -c ${pkg}.txt  > ${ddir}/text/${pkg}.txt.gz  || exit 1
cp   -f ${pkg}.txt    ${ddir}/text/.              || exit 1

echo generating doc page
cd ${ddir}
cat > TAG <<EOF
<p align="center"><a href="http://www.anybrowser.org/campaign/"
   ><img src="/software/${pkg}/pix/abrowser.png"
   width="118" height="32" alt="Viewable With Any Browser"
   border="0"></a>
&nbsp;&nbsp;<a href="/software/${pkg}/"
><img src="/software/${pkg}/pix/${pkg}_header.png"
     width="188" height="50" border="0" alt="${pkg} Home"></a></p>
EOF
body-end -i TAG */*.html

(cd html_mono
 gzip -c --best ${pkg}.html > ${pkg}.html.gz )
(cd html_chapter
 tar cf - ${pkg}*.html | gzip --best > ${pkg}_chapter_html.tar.gz )
(cd html_node
 tar cf - ${pkg}*.html | gzip --best > ${pkg}_node_html.tar.gz )

autogen --base-name=${pkg} -T ${pkgsrcdir}/gnudoc.tpl - <<- _EODefs_
	autogen definitions gnudoc;
	title = '${pkg} - shell archive utilities';
	project = '${pkg}';
	version = '${ver}';
	_EODefs_

rm -f TAG
cd ..
tar cf - ${ddir} | gzip > ${ddir}-doc.tar.gz

## Local Variables:
## Mode: shell-script
## tab-width: 4
## indent-tabs-mode: nil
## sh-indentation: 2
## sh-basic-offset: 2
## End:
