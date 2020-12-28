#include "gysThreadPool.h"
#include <iostream>

int main(){

    std::vector<std::future<int>> v_int;

    GYS::CLthreadPool tp(3);

    std::mutex m_cout;

    for(int i = 0; i<100; ++i){
        v_int.push_back(
            tp.add_task(
                [&m_cout](int j, int k)->int{
                    m_cout.lock();   //以后不要这样用了，这里忘记unlock当时debug花了一个小时，淦
                    std::cout << "thread_id=" <<std::this_thread::get_id() << " print " << j << std::endl;
                    m_cout.unlock();
                    return k;
                },
                i, i+1997
            )
        );
    }

    for(auto &s : v_int) std::cout << s.get() << std::endl;

    system("pause");
    return 0;
}