
template<class ReturnType>
class TaskResult
{
    explicit TaskResult() {
        inner = new _TaskResult();
    }

    ReturnType wait()
    {   
        ReturnType value;
        if (inner)
        {
            std::unique_lock<std::mutex> lock(_mtx);
            inner->_cv.wait(lock, [&inner](){
                return inner->is_ended;
            });
            value = inner->value;
            delete inner;
            inner = nullptr;
        }
        return value; 
    }

private:
    void notify(ReturnType value)
    {
        if (inner)
        {
            std::unique_lock<std::mutex> lock(inner->_mtx);
            inner->value = value;
            inner->is_ended = true;
            inner->_cv.notify_all();
        }
    }
    struct _TaskResult<ReturnType> {
        ReturnType value;
        bool is_ended {false};
        std::mutex _mtx;
        std::condition_variable _cv;
    };
    _TaskResult* inner {nullptr}; 
};

template<>
class TaskResult<void>
{
    explicit TaskResult() {
        inner = new _TaskResult();
    }

    void wait()
    {   
        if (inner)
        {
            std::unique_lock<std::mutex> lock(inner->_mtx);
            inner->_cv.wait(lock, [&inner](){
                return inner->is_ended;
            });
            delete inner;
            inner = nullptr;
        }
    }
private:
    void notify()
    {
         if (inner)
        {
            std::unique_lock<std::mutex> lock(inner->_mtx);
            inner->is_ended = true;
            inner->_cv.notify_all();
        }
    }
    struct _TaskResult {
        bool is_ended {false};
        std::mutex _mtx;
        std::condition_variable _cv;
    };
    _TaskResult* inner {nullptr}; 
};