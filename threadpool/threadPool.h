#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <vector>
#include <queue>
#include <mutex>
#include <thread>

class ThreadPool{
public:
    typedef std::function<void(void)> TASKTYPE;

    explicit ThreadPool(int n = 2);
    ~ThreadPool();

    void start();

    void stop();

    void doneThenStop();

    int getNumOfThreads();

    template<typename FUNC, typename... PARAMS>
    std::future< typename std::result_of< FUNC(PARAMS...) >::type > add(FUNC&& , PARAMS&&...);


private:
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;


    std::atomic<bool> _sync;

    std::vector<std::thread> _threads;

    std::queue<TASKTYPE> _tasks;

    std::mutex _mt_tasks;

    std::condition_variable _cond_tasks;

    std::condition_variable _cond_stop;

    int _numThreads;

};




template<typename FUNC, typename... PARAMS>
std::future< typename std::result_of<FUNC(PARAMS...)>::type > ThreadPool::add(FUNC&& func, PARAMS&&... params){

    typedef typename std::result_of<FUNC(PARAMS...)>::type RETURNTYPE;

    auto p = std::make_shared< std::packaged_task< RETURNTYPE(void) > >( std::bind(func, params...));

    if(this->_sync.load(std::memory_order_acquire)) throw std::runtime_error("threadpool has stopped!!!");

    {
        std::lock_guard<std::mutex> g_lock(this->_mt_tasks);

        this->_tasks.emplace(
            [p]()->void{
                (*p)();
            }
        );

    }

    this->_cond_tasks.notify_all();

    return p->get_future();

}






#endif