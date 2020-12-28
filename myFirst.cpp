#include <iostream>
#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <future>
using namespace std;


class threadPool{
public:

    typedef function<void(void)> job_type;

    threadPool(int n):m_stop(false){  // m_stop必须要初始化, 否则memory_order_relaxed
        if(n < 2) n = 2;

        if(m_stop.load(memory_order_relaxed)) cout << " acquire" << endl;

        for(int i = 0; i<n; ++i){
            this->m_threads.push_back(thread(
                [this]()->void{
                    while(!this->m_stop.load(memory_order_acquire)){
                        job_type job;

                        {
                            unique_lock<mutex> u_lock(this->m_mt);

                            this->m_cond.wait(u_lock, [this]()->bool{
                                return this->m_stop.load(memory_order_acquire) || this->m_jobs.size();
                            });
                            if(this->m_stop.load(memory_order_acquire)) return;
                            job = move(this->m_jobs.front());

                            this->m_jobs.pop();
                        }

                        job();
                    }
                }
            ));
        }

    }

    ~threadPool(){
        this->stop();
        this->m_cond.notify_all();
        for(auto &s : this->m_threads) if(s.joinable()) s.join();
    }

    void stop(){
        this->m_stop.store(true, memory_order_release);
    }

    template<typename FUNC, typename ...args>
    future<typename result_of<FUNC(args...)>::type> add(FUNC&& func,args&& ...params){  //future来获得返回值，否则主函数不知道其返回值，主函数用future<return_type>对象保存返回的future
        typedef typename result_of<FUNC(args...)>::type return_type;

        auto originalFUNC = bind(move(func), move(params)...);   // bind将函数和其参数绑定，返回值是一个返回值为return_type的函数对象

        //originalFUNC();   //可以验证这里直接调用的话会多一次输出

        auto t = make_shared<packaged_task<return_type()>> (move(originalFUNC));  

        if(this->m_stop.load(memory_order_acquire)) throw runtime_error("stopped pool");

        this->m_jobs.emplace(
            [t]()->void{
                (*t)();
        });

        this->m_cond.notify_all();

        return t->get_future();

    }

    vector<thread> m_threads;

    atomic<bool> m_stop;

    condition_variable m_cond;

    queue<job_type> m_jobs;

    mutex m_mt;
    

};

int main(){

    mutex m_cout;

    threadPool tp(3);
    vector<future<string>> return_val;
    for(int i = 0; i<10; ++i){
        return_val.push_back(
        tp.add([&m_cout](int k)->string{
            m_cout.lock();
            cout << k << "---" << this_thread::get_id() << endl;
            m_cout.unlock();

            return "hello";
        },i)
        );
    }
   // for(auto &s: return_val) cout << s.get() << endl;

    system("pause");

    return 0;
}