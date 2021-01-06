#include <mutex>
#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <condition_variable>
#include <future>
#include <atomic>

namespace GYS{

    class CLthreadPool{
    public:
        typedef std::function<void(void)> task_function_type;

        explicit CLthreadPool(int n);
        ~CLthreadPool();

        template<typename FUNC, typename... PARAMS>
        std::future<typename std::result_of<FUNC(PARAMS...)>::type > add_task(FUNC&&, PARAMS&&... );  // 返回future对象使得主函数不因为返回值而阻塞

    private:
        void stop();

        std::mutex m_mt;  // 互斥处理任务队列

        std::vector<std::thread> m_threads; //所有线程

        std::queue<task_function_type> m_task; // 任务队列

        std::condition_variable m_cond;   // 同步任务和线程

        std::atomic<bool> m_stop;    // 标记线程池是否关闭   


    private:
        CLthreadPool(CLthreadPool&&) = delete;
        CLthreadPool(const CLthreadPool&) = delete;
        CLthreadPool& operator=(CLthreadPool&&) = delete;
        CLthreadPool& operator=(const CLthreadPool&) = delete;
    };

}

template<typename FUNC, typename... PARAMS>
std::future<typename std::result_of<FUNC(PARAMS...)>::type > GYS::CLthreadPool::add_task(FUNC&& func, PARAMS&&... params){

    typedef typename std::result_of<FUNC(PARAMS...)>::type task_return_type;   // result_of模板参数为类型FUNC，别输错为对象func

    auto f = std::bind(std::move(func), std::move(params)...);    //绑定函数和其参数

    auto ptr = std::make_shared< std::packaged_task<task_return_type()> > (move(f)); //打包任务，模板参数为可调用对象。 f返回值（f参数）

    if(this->m_stop.load(std::memory_order_acquire)) throw std::runtime_error("gysThreadPool has stopped!!!");  // 线程池已经关闭，不要继续加任务了

    {
        this->m_mt.lock();

        this->m_task.emplace(
            [ptr]()->void{  //千万别传&ptr
                (*ptr)();
            }
        );

        this->m_mt.unlock();
    }

    this->m_cond.notify_all();

    return ptr->get_future();

}
