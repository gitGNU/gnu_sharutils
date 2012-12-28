#! /bin/sh

case "X$VERBOSE" in
X[TtYy]* | 1 )
    set -x
    ;;
esac

target=$1 ; shift
tmpdir=$(mktemp -d mkhelp-XXXXXX)
trap "rm -rf $tmpdir" 0

die() {
    echo "mk-help fatal error:  $*"
    trap '' 0
    echo $tmpdir left intact
    exit 1
} 1>&2

get_help() {
    ./$1 $2 2>&1 | grep -v illegal > $tmpdir/$3.txt
    cmp -s $tmpdir/$3.txt $3.txt || {
        mv $tmpdir/$3.txt $3.txt
        echo $3 has been updated
    }
}

mkusage() {
    touch $tmpdir/usage-text

    for f in "$@"
    do
        test -x "$f" || die "$f program has not been built"
        get_help $f --help $f-full-help
        get_help $f --error-bogus-option-name $f-short-help
    done

    mv $tmpdir/usage-text .
}

mkhelp() {
    touch $tmpdir/help-text
    rm -f *-help.txt
    for f ; do autogen $f-opts.def ; done
    ${MAKE} "$@"
    target=usage-text
    mkusage "$@"
    for f ; do autogen $f-opts.def ; done
    ${MAKE} "$@"
    dd=$(pwd | sed s/-bld//)
    test "X$dd" = "X$PWD" && return 0
    for f in *-help.txt
    do
        rm -f $dd/$f
        ln $f $dd/.
    done
    mv $tmpdir/help-text .
}

case $target in
usage-text ) mkusage "$@" ;;
help-text )  mkhelp  "$@" ;;
esac

trap '' 0
exit 0
