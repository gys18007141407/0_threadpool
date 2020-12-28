#include "gysThreadPool.h"

GYS::CLthreadPool::CLthreadPool(int n) : m_stop(false){

    if(n < 2) n = 2;

    for(int i = 0; i < n ; ++i){

        this->m_threads.push_back(std::thread(

            [this]()->void{

                while(!this->m_stop.load(std::memory_order_acquire)){
                    task_function_type task;

                    {  
                        std::unique_lock<std::mutex> u_lock(this->m_mt);

                        this->m_cond.wait(u_lock, [this]()->bool{
                            return this->m_stop.load(std::memory_order_acquire) || this->m_task.size();
                        });

                        if(this->m_stop.load(std::memory_order_acquire)) return;

                        task = std::move(this->m_task.front());

                        this->m_task.pop();
                    }

                    task();

                }

            }

        ));

    }

}

GYS::CLthreadPool::~CLthreadPool(){
    this->stop();

    this->m_cond.notify_all();

    for(auto & s : this->m_threads) if(s.joinable()) s.join();

}

void GYS::CLthreadPool::stop(){
    this->m_stop.store(true, std::memory_order_release);
}