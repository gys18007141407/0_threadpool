#include "threadPool.h"

ThreadPool::ThreadPool(int n):_sync(false){
    if(n < 2) n = 2;
    _numThreads = n;
}

ThreadPool::~ThreadPool(){

    stop();

    for(auto & td : _threads){
        if(td.joinable()) td.join();
    }
}

int ThreadPool::getNumOfThreads(){
    return this->_numThreads;
}

void ThreadPool::start(){
    
    for(int i = 0; i<_numThreads; ++i){
        _threads.push_back(

            std::thread(
            [this]()->void{

                TASKTYPE task;

                while(!this->_sync.load(std::memory_order_acquire)){

                    {    
                        std::unique_lock<std::mutex> u_lock(this->_mt_tasks);

                        this->_cond_stop.notify_all();

                        this->_cond_tasks.wait(u_lock, [this]()->bool{
                            return this->_sync.load(std::memory_order_acquire) || this->_tasks.size();
                        });

                        if(this->_sync.load(std::memory_order_acquire)) return;

                        task = std::move(this->_tasks.front());

                        this->_tasks.pop();
                    }


                    task();

                }

            }
            )

        );
    }

}

void ThreadPool::stop(){

    _sync.store(true, std::memory_order_release);

    _cond_tasks.notify_all();

}

void ThreadPool::doneThenStop(){

    std::unique_lock<std::mutex> u_lock(this->_mt_tasks);

    this->_cond_stop.wait(u_lock, [this]()->bool{
        return this->_tasks.size() == 0;
    });

    stop();
}