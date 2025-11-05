# Kraken
A simple toy render for experimenting with USD Hydra and multiple GPU backends

# Build
``` bash
git clone --recursive https://github.com/sherholz/kraken.git
```

## Building via console
``` bash
cd kraken
mkdir build
cd build
cmake -DCMAKE_PREFIX_PATH=../deps/pxr -DCMAKE_INSTALL_PREFIX=../install ..
```

``` bash
make -j
make -j install
```
## Building using VSCode