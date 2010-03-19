#!/bin/sh

# This script sets up a build environment for the command-line Visual C
# compiler (like the one from the free Visual Studio Express version)
# for usage under Linux with WINE. You must have set up a proper WINE
# environment including ability to run EXE files directly.
#
# The following variables will influence the way this script sets up
# the environment:
#
# MSVC_BASE    - the base path to msvc compiler suite, e.g. /usr/msvc
# MSVC_INCLUDE - additional include paths (UNIX semantic), separated by ':'
# MSVC_LIB     - additional library path(-s), separated by ':'. Note that
#                $MSVC_BASE/lib will be always added to library path.
# MSVC_DLL     - additional paths where DLLs needed by running executables
#                are placed. These are converted to WINEDLLPATH
#
# If certain part of the above variables does not start with a slash,
# MSVC_BASE is automatically prepended. For example, MSVC_LIB="lib:../gtk/lib"
# will be interpreted as "$MSVC_BASE/lib:$MSVC_BASE/../gtk/lib"

if [ -z "$MSVC_BASE" ]; then
    # You can specify the path to MSVC compiler in three ways:
    # a) Set the MSVC_BASE variable before invoking this script
    # b) Via environment, e.g. "export MSVC_BASE=/usr/somewhere/msvc"
    # c) Via command line, e.g. "./msvc.sh /usr/somewhere/msvc"
    if [ -n "$1" ]; then
        MSVC_BASE="$1"
    elif [ -z "$MSVC_BASE" ]; then
        MSVC_BASE="/usr/local/msvc"
    fi
fi

old_IFS="$IFS"
IFS=:

paths="$MSVC_BASE/bin:$MSVC_PATH"
for x in $paths; do
    [ -z "$x" ] && continue
    [ "${x:0:1}" != "/" ] && x="$MSVC_BASE/$x"
    [ -n "$PATH" ] && PATH="$PATH;$x" || PATH="$x"
done

paths="$MSVC_BASE/lib:$MSVC_DLL"
for x in $paths; do
    [ -z "$x" ] && continue
    [ "${x:0:1}" != "/" ] && x="$MSVC_BASE/$x"
    [ -n "$WINEDLLPATH" ] && WINEDLLPATH="$WINEDLLPATH;$x" || WINEDLLPATH="$x"
done

paths="$MSVC_BASE/include:$MSVC_INCLUDE"
for x in $paths; do
    [ -z "$x" ] && continue
    [ "${x:0:1}" != "/" ] && x="$MSVC_BASE/$x"
    i="`winepath -w \"$x\" 2>/dev/null`"
    [ -n "$INCLUDE" ] && INCLUDE="$INCLUDE;$i" || INCLUDE="$i"
done

paths="$MSVC_BASE/lib:$MSVC_LIB"
for x in $paths; do
    [ -z "$x" ] && continue
    [ "${x:0:1}" != "/" ] && x="$MSVC_BASE/$x"
    l="`winepath -w \"$x\" 2>/dev/null`"
    [ -n "$LIB" ] && LIB="$LIB;$l" || LIB="$l"
done

IFS="$old_IFS"
export WINEDLLPATH INCLUDE LIB

if [ -z "$2" ]; then
        echo ""
        echo "The environment is set up for using the Microsoft Visual C compiler."
        echo "Now you can execute e.g. configure or make, or alternatively you may"
        echo "type 'exit' to end this session."
        echo ""

        exec ${SHELL:-/bin/sh}
elif [ "$2" != "x" ]; then
        exec $2
fi
