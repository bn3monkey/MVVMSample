#include <cstring>

namespace Bn3Monkey
{
    template<class F, class... Args>
    class ScopedTaskImpl
    {
        using ReturnValue = std::result_of<F(Args...)>;
        explicit ScopedTaskImpl(const char* task_name, F&& f, Args&&... args) :
            _task(std::forward<F>(f), std::forward<Args>(args)...), _is_finished(false)
        {
            strncpy(_name, task_name, 256);
            
        }
        ScopedTaskImpl(const ScopedTaskImpl& other) : _task(other.task), _is_finished(other._is_finished)
        {
            strncpy(other._name, _name, 256);
            strncpy(other._ret, _ret, sizeof(ReturnValue));  
        }


    private:
        void invoke()
        {
            auto& ret = _task();
            _is_finished = true;
            memcpy(_ret, ret, sizeof(ReturnValue));
        }

        template<typename ReturnType>
        ReturnType invoke_impl(std::true_type)
        {
            _ret = _task();
        }
        template<typename ReturnType>
        ReturnType invoke_impl(std::false_type)
        {
            auto& _ret = _task();
            memcpy(_ret, ret, sizeof(ReturnValue));
        }


        char _name[256];
        std::function<ReturnValue()> _task;
        bool _is_finished;
        std::aligned_storage_t<sizeof(ReturnType) != 0 ? sizeof(ReturnType) : 1, alignof(ReturnType)> t;
    }

}