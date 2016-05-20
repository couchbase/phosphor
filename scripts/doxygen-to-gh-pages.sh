#!/bin/bash

# Example taken from
#   https://github.com/ReadyTalk/swt-bling/blob/master/.utility/push-javadoc-to-gh-pages.sh
#   http://benlimmer.com/2013/12/26/automatically-publish-javadoc-to-gh-pages-with-travis-ci/

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
${TMPDIR-/tmp}/${DOXYGEN_VER}/bin/doxygen

if [ "$TRAVIS_PULL_REQUEST" == "false" ] && [ "$TRAVIS_BRANCH" == "master" ]; then

    echo -e "Publishing doxygen. \n"

    cd $HOME
    git config --global user.email "travis@travis-ci.org"
    git config --global user.name "travis-ci"
    git clone --quiet --branch=gh-pages https://${GH_TOKEN}@github.com/${TRAVIS_REPO_SLUG} gh-pages > /dev/null

    cd gh-pages
    git rm -rf *
    cp -Rf $TRAVIS_BUILD_DIR/doxydocs/html/* .
    git add -f .
    git commit -m "[Travis] Update documentation"
    git push -fq origin gh-pages

    echo -e "Published doxygen to gh-pages. \n"

fi