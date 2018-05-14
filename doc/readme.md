
# 1. 依赖环境

boost 1.67               asio 负责通讯
UnQLite / RaptorDB       负责 节点用户临时信息( VLR )
Postgresql               负责 用户信息( HLR )
Redis 1.16 / MangoDB     消息




## 编译boost v1.6.7
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


## 编译工程

``` shell
make build
cd build
rm -rf * && cmake -DCMAKE_CXX_COMPILER=clang -DCMAKE_BUILD_TYPE=DEBUG ..
make
```



# 2. 参考资料
http://www.unqlite.org/downloads.html

