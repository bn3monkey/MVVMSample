

template <typename Type>
class Property
{
public:
    Property property(std::string name)
    {

    }
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