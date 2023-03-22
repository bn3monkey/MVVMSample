#include <functional>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>



class ThreadPool
{
public:
    static ThreadPool& getInstance() {
        return _instance;
    }

    ThreadPool initialize()
    {
        is_running = true;
        _routine = std::thread([&](){
            routine();
        });
    }
    ThreadPool release() {
        is_running = false;
        _routine.join();
    }


    bool isInThreadPool() {
        return _routine_id == std::this_thread::get_id();
    }

    template<class F, class ...Args>
    void run(F&& f, Args&&... args)
    {
        auto binded_function = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        {
            std::unique_lock<std::mutex> lock(_queue_mutex);
            _tasks.emplace(
                [&binded_function](){
                    bind_function();
                }
            )
        }
        _queue_cv.notify_one();
    } 

private:
    static ThreadPool _instance;
    std::queue<std::function<void()>> _tasks;
    std::mutex _queue_mutex;
    std::condition_variable _queue_cv;
    std::thread::id _routine_id;
    std::thread _routine;
    std::atomic<bool> is_running;
     
    
    void routine()
    {
        _routine_id = std::this_thread::get_id();
        while (is_running)
        {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(_queue_mutex);
                _queue_cv.wait(lock, [&] {
                    return !is_running || !_tasks.empty();
                });
                if (!is_running)
                    return;
                task = std::move(_tasks.front());
                _tasks.pop();
            }
            task();
        }
    }
};

template <typename Type>
class Property
{
public:
    Type get()
    {
        // thread pool 바깥이면 set이 끝나면 수행
        if (!ThreadPool::getInstance().isInThreadPool())
        {
            std::unique_lock<std::mutex> lock(_mtx); 
            _cv.wait(lock, [&]{ return set_count == 0;});
        }
        // thread pool이면 그대로 수행
        return _value;
    }
    void set(Type type)
    {
        // thread pool 바깥이면 바깥에서 set count 1 올림.
        if (!ThreadPool::getInstance().isInThreadPool())
        {
            set_count += 1;
            ThreadPool::getInstance().run([&](){
                setValue(type);
                notify();
            });
        }
        else
        {
            setValue(type);
        }
    }

    void registerPropertyChanged(void (*callback)(double value))
    {
        _callback = [](Type value) {
            callback(value);
        };
    }

private:
    Type _value;

    int set_count {0};

    std::mutex _mtx;
    std::condition_variable _cv;

    std::function<void(Type)> _callback;
    void setValue(Type type)
    {
        _value = value;
        if (_callback)
        {
            _callback(_value);
        }
    }
    void notify()
    {
        {
            std::unique_lock<std::mutex> lock(_mtx); 
            set_count -= 1;
        }
        _cv.notify_all();
    }
};