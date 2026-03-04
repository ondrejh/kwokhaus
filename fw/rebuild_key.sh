rm -rf build
mkdir build
cd build
cmake ../src_key -DPICO_BOARD=pico_w
make
