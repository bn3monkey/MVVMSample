#include "ScopedTask.hpp"
#include "ScopedTaskScopeImpl.hpp"
/*
class Fuck
{
public:
    Fuck(int a)
    {
        printf("Creaated\n");
        num = a;

    }
    Fuck(const Fuck& other)
    {
        printf("Copied\n");
        num = other.num;
    }
    Fuck(Fuck&& other)
    {
        printf("Moved\n");
        num = other.num;
    }

    int num;
};


int func1(int a, int b)
{
    printf("func1 : %d %d\n", a, b);
    return a + b;
}
void func2(int a)
{
    printf("func2 : %d\n", a);
}
void testScopedTask()
{
    using namespace Bn3Monkey;
    ScopedTaskResultImpl<int> r1("result 1", []() {}, []() {});
    ScopedTaskNotifier<int> p1("result 1", &r1);

    ScopedTaskResultImpl<void> r2("result 2", []() {}, []() {});
    ScopedTaskNotifier<void> p2("result 2", &r2);

    ScopedTask task1("task1", []() {}, []() {});
    ScopedTask task2("task2", []() {}, []() {});
    auto result1 = task1.make(func1, 1, 2);
    auto result2 = task2.make(func2, 1);

    task1.invoke();
    auto* ret1 = result1.wait();
    if (ret1)
    {
        printf("test1 result : %d", *ret1);
    }

    task2.invoke();
    result2.wait();
}

struct Request {
    Bn3Monkey::ScopedTaskScopeImpl* scope {nullptr};
    bool is_activated{false};
    bool* is_done{ nullptr };
    Request() {}
    Request(Bn3Monkey::ScopedTaskScopeImpl* scope, bool is_activated, bool* is_done) : scope(scope), is_activated(is_activated), is_done(is_done) {}

};

std::queue<Request> _requests;
bool _is_running;
std::mutex _request_mtx;
std::condition_variable _request_cv;

std::mutex _response_mtx;
std::condition_variable _response_cv;

void manager()
{
    LOG_V("Manager starts");
    

    for (;;)
    {
        Request request;

        {
            std::unique_lock<std::mutex> lock(_request_mtx);
            _request_cv.wait(lock, [&]() {
                return !_is_running || !_requests.empty();
                });
            if (!_is_running)
            {
                break;
            }
            request = std::move(_requests.front());
            _requests.pop();
        }

        if (request.is_activated)
        {
            LOG_D("Manager starts Worker (%s)", request.scope->name());
            request.scope->start();
        }
        else {
            LOG_D("Manager stops Worker (%s)", request.scope->name());
            request.scope->stop();
        }

        {
            std::unique_lock<std::mutex> lock(_response_mtx);
            *request.is_done = true;
        }
        _response_cv.notify_one();
    }
    
}
void start(Bn3Monkey::ScopedTaskScopeImpl& task_scope)
{
    LOG_V("Request manager to start task scope (%s)", task_scope.name());
    
    bool is_done {false};

    {
        std::unique_lock<std::mutex> lock(_request_mtx);
        _requests.emplace(&task_scope, true, &is_done);
    }
    _request_cv.notify_all();

    {
        std::unique_lock<std::mutex> lock(_response_mtx);
        _response_cv.wait(lock, [&]() {
            return is_done;
        });
    }
}
void stop(Bn3Monkey::ScopedTaskScopeImpl& task_scope)
{
    LOG_V("Request manager to stop task scope (%s)", task_scope.name());
    bool is_done{ false };

    {
       {
            std::unique_lock<std::mutex> lock(_request_mtx);
            _requests.emplace(&task_scope, false, &is_done);
        }
        _request_cv.notify_all();
    }

    {
        std::unique_lock<std::mutex> lock(_response_mtx);
        _response_cv.wait(lock, [&]() {
            return is_done;
            });
    }
}

void testScopedTaskScope()
{


    using namespace Bn3Monkey;
    auto* main = ScopedTaskScopeImpl::getScope("main",
        [](ScopedTaskScopeImpl& scope) {
            start(scope);
        },
        [](ScopedTaskScopeImpl& scope) {
            stop(scope);
        });

    auto* device = ScopedTaskScopeImpl::getScope("device",
        [](ScopedTaskScopeImpl& scope) {
            start(scope);
        },
        [](ScopedTaskScopeImpl& scope) {
            stop(scope);
        });

    auto* ip = ScopedTaskScopeImpl::getScope("ip",
        [](ScopedTaskScopeImpl& scope) {
            start(scope);
        },
        [](ScopedTaskScopeImpl& scope) {
            stop(scope);
        });

    _is_running = true;
    std::thread _thread = std::thread(manager);

    main->run("big task", []() {
        for (int count = 5; count != 0; count -= 1)
        {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(1s);
            printf("Count : %d\n", count);
        }
        });

    main->run("big task2", []() {
        for (int count = 3; count != 0; count -= 1)
        {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(1s);
            printf("SANS : %d\n", count);
        }
        });
    
    auto main1 = main->call("main1", [&]() {
        printf("Main1\n");
        auto deviceA = device->call("deviceA", [&]() {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.1s);
            printf("deviceA\n");
            return 1;
            });
        auto deviceB = device->call("deviceB", [&]() {

            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.3s);
            printf("deviceB\n");
            return 10;
            });

        auto ipA = ip->call("ipA", [&]() {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.2s);
            printf("ipA\n");
            return 100;
            });
        auto ipB = ip->call("ipB", [&]() {

            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.4s);
            printf("ipB\n");
            return 1000;
            });

        int sum = 0;

        {
            auto* ret = deviceA.wait();
            if (ret)
            {
                sum += *ret;
            }
        }
        {
            auto* ret = deviceB.wait();
            if (ret)
            {
                sum += *ret;
            }
        }
        {
            auto* ret = ipA.wait();
            if (ret)
            {
                sum += *ret;
            }
        }
        {
            auto* ret = ipB.wait();
            if (ret)
            {
                sum += *ret;
            }
        }
        printf("Sum : %d\n", sum);
        });
    main1.wait();
    
    printf("-------- Main 1 Relaunch! --------\n");

    main->call("main1", [&]() {
        printf("Main1\n");
        auto deviceA = device->call("deviceA", [&]() {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.1s);
            printf("deviceA\n");
            return 1;
            });
        auto deviceB = device->call("deviceB", [&]() {

            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.3s);
            printf("deviceB\n");
            return 10;
            });

        auto ipA = ip->call("ipA", [&]() {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.2s);
            printf("ipA\n");
            return 100;
            });
        auto ipB = ip->call("ipB", [&]() {

            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.4s);
            printf("ipB\n");
            return 1000;
            });

        int sum = 0;

        {
            auto* ret = deviceA.wait();
            if (ret)
            {
                sum += *ret;
            }
        }
        {
            auto* ret = deviceB.wait();
            if (ret)
            {
                sum += *ret;
            }
        }
        {
            auto* ret = ipA.wait();
            if (ret)
            {
                sum += *ret;
            }
        }
        {
            auto* ret = ipB.wait();
            if (ret)
            {
                sum += *ret;
            }
        }
        printf("Sum : %d\n", sum);
        });

    printf("-------- Main 1 Relaunch! --------\n");

    auto main11 = main->call("main1", [&]() {
        printf("Main1\n");
        auto deviceA = device->call("deviceA", [&]() {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.1s);
            printf("deviceA\n");
            return 1;
            });
        auto deviceB = device->call("deviceB", [&]() {

            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.3s);
            printf("deviceB\n");
            return 10;
            });

        auto ipA = ip->call("ipA", [&]() {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.2s);
            printf("ipA\n");
            return 100;
            });
        auto ipB = ip->call("ipB", [&]() {

            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.4s);
            printf("ipB\n");
            return 1000;
            });

        int sum = 0;

        {
            auto* ret = deviceA.wait();
            if (ret)
            {
                sum += *ret;
            }
        }
        {
            auto* ret = deviceB.wait();
            if (ret)
            {
                sum += *ret;
            }
        }
        {
            auto* ret = ipA.wait();
            if (ret)
            {
                sum += *ret;
            }
        }
        {
            auto* ret = ipB.wait();
            if (ret)
            {
                sum += *ret;
            }
        }
        printf("Sum : %d\n", sum);
        return sum;
        });

    auto main_ret = main11.wait();
    if (main_ret)
    {
        printf("%d\n", *main_ret);
    }

    auto main2 = main->call("main2", [&]() {
        printf("main2\n");
        auto device2 = device->call("device2", [&]() {
            printf("device2\n");
            printf("maybe error");
            auto main3 = main->call("main3", [&]() {
                return 4;
            });
            auto* main3_result = main3.wait();
            return 3;
        });
        auto* device2_ret = device2.wait();
        return 2;
    });
    auto ret2 = main2.wait();
    if (ret2)
    {
        printf("%d\n", *ret2);
    }

    auto main3 = main->call("main3", [&]() {
        printf("main3\n");
        auto device3 = device->call("device3", [&]() {
            printf("device3\n");
            auto ip3 = ip->call("ip3", [&]() {
                printf("ip3\n");
                printf("maybe error!");
                auto main4 = main->call("main4", [&]() {

                    });

                return 4;
                });
            auto* ip3_result = ip3.wait();
            return 3;
            });
        auto* device3_ret = device3.wait();
        return 2;
        });
    auto ret3 = main3.wait();
    if (ret3)
    {
        printf("%d\n", *ret3);
    }


    _is_running = false;
    _request_cv.notify_all();
    _thread.join();
}

void main()
{
    testScopedTaskScope();
    return;
}
*/

class SANS
{
public:
    bool fuck {false};
    std::function<void()> _function;

    SANS() {
        _function = [&]() {
            fuck = true;
        };
    }
    void operator()() {
        _function();
    }
};

void main()
{
    SANS sans1;
    SANS sans2 = sans1;

    sans2();
    printf("sans?");
}