# MQTTd
MQTT Server Client



# 1. 依赖环境
    C++14                    clang 已测试AppleClang 9.1.0.9020039
    cmake 2.8+               编译脚本
    boost 1.67               asio 负责通讯




## 2. 编译boost v1.6.7
```shell
cd boost_1_67_0
mkdir build

cd build
../bootstrap.sh --with-toolset=clang

cd ..
build/b2 --build-dir=./build toolset=clang address-model=64 link=static runtime-link=static \
    --with-system --with-date_time --with-regex \
    cxxflags="-std=c++14 -stdlib=libc++" linkflags="-stdlib=libc++" \
    --stagedir=./build stage

cp -r ./boost/ ./build/include/boost/
```


## 3. 编译工程

``` shell
make build
cd build
rm -rf * && cmake -DCMAKE_CXX_COMPILER=clang -DCMAKE_BUILD_TYPE=DEBUG ..
make
```
