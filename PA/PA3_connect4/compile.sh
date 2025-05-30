root_dir="/mnt/e/codebase/IAI/PA/PA3_connect4/Batch/Linux"  #your path here
for file in `ls "${root_dir}/Sourcecode"`
do
    cd "${root_dir}/Sourcecode/${file}"
    pwd
    g++ -Wall -std=c++11 -O2 -fpic -shared *.cpp -o "${root_dir}/so/${file}.so"
    cd -
done
