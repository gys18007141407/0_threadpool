#include "threadPool.hpp"
#include <cstdio>

int main(){
    ThreadPool b(4);
    b.start();

    for(int i = 0; i<1000; ++i){
        b.add(
            [i]()->void{
                printf("%d  ----- %ld\n", i, std::this_thread::get_id());
            }
        );
    }

    //b.stop();
    b.doneThenStop();

    return 0;
}