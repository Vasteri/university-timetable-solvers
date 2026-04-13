my_path="$(dirname $0)/../.."

if [[ ! -d "$my_path/build/" ]]; then
    mkdir "$my_path/build/"
fi
cp $my_path/src/CrossUI/build/Desktop-Debug/CrossUI $my_path/build/