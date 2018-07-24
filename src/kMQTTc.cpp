
#include <memory>

#include <functional>
#include <iostream>
#include <thread>
#include <mutex>

int main(int argc, char* argv[]) {

    std::shared_ptr<std::mutex> mtx_ptr;
    std::thread p1([mtx_ptr]()
    {
        std::thread p2([mtx_ptr]()
        {
            std::cout << "aaa" << std::endl;
        });
        std::cout << "bbb" << std::endl;
    });

    getchar();
    return 0;
}
