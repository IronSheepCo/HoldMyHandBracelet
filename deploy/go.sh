DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
CURRENT_DIR=`pwd`
cd $DIR
cd ../pca10028/s130/armgcc && make
if [ $? -ne 0 ]
 then
    cd $CURRENT_DIR
    exit 1
fi
cd -
./loadbin.sh

cd $CURRENT_DIR
