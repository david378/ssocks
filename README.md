
# build static ssocks by cmake 
## Add cmake file, build ssocks more easy.

### how to build?

windows:

```
git clone ...
cd ssocks
mkdir build
cd build
cmake ../
cmake --build . --config release
```

 linux:

```
git clone ...
cd ssocks
mkdir build
cd build
cmake ../
make
```

cross build:

```
git clone ...
cd ssocks
mkdir build
cd build
cmake ../ -DCMAKE_C_COMPILER=arm-linux-musleabi-gcc
make
```



default build static execute

original verison https://github.com/tostercx/ssocks


