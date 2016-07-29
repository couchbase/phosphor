#!/bin/bash

# Example taken from
#   https://github.com/ReadyTalk/swt-bling/blob/master/.utility/push-javadoc-to-gh-pages.sh
#   http://benlimmer.com/2013/12/26/automatically-publish-javadoc-to-gh-pages-with-travis-ci/

set -e

CDIR=$(pwd)

DOXYGEN_VER=doxygen-1.8.11
DOXYGEN_TAR=${DOXYGEN_VER}.linux.bin.tar.gz
DOXYGEN_URL="http://ftp.stack.nl/pub/users/dimitri/${DOXYGEN_TAR}"

doxygen_install()
{
	wget -O - "${DOXYGEN_URL}" | \
		tar xz -C ${TMPDIR-/tmp} ${DOXYGEN_VER}/bin/doxygen
    export PATH="${TMPDIR-/tmp}/${DOXYGEN_VER}/bin:$PATH"
}

doxygen_install
pushd phosphor > /dev/null
${TMPDIR-/tmp}/${DOXYGEN_VER}/bin/doxygen
popd phosphor > /dev/null

pushd $HOME > /dev/null
git config --global user.email "github@couchbase.com"
git config --global user.name "cb-robot"
rm -rf gh-pages
git clone --quiet --branch=gh-pages git@github.com:couchbase/phosphor.git gh-pages > /dev/null

pushd gh-pages > /dev/null
git rm -rf *
cp -Rf $CDIR/phosphor/doxydocs/html/* .
git add -f .
git commit -m "Update documentation"
git push -fq origin gh-pages
popd > /dev/null
rm -rf gh-pages
popd > /dev/null

echo -e "Published doxygen to gh-pages. \n"
