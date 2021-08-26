#!/bin/bash
echo "- Building"

cd lib
make clean && make

cd ..
make clean && make

if [ ! -z "$1" ] && [ $1 == "--install" ] ; then
        echo "- Installing"
        cd lib
        sudo make install
        cd ..
        sudo make install
fi

echo "- Done"