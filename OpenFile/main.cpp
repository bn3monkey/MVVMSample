#include "ScopedTask.hpp"
#include "ScopedTaskScopeImpl.hpp"


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
                LOG_D("Clear All Requests");
                while (!_requests.empty())
                {
                    request = std::move(_requests.front());
                    _requests.pop();
                    if (request.is_activated)
                    {
                        LOG_D("Manager starts Worker (%s)", request.scope->name());
                        request.scope->start();
                    }
                    else {
                        LOG_D("Manager stops Worker (%s)", request.scope->name());
                        request.scope->stop();
                    }
                }
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

    }
    

    Bn3Monkey::ScopedTaskScopeImpl::clearScope();
}
bool start(Bn3Monkey::ScopedTaskScopeImpl& task_scope)
{
    LOG_V("Request manager to start task scope (%s)", task_scope.name());
    
    bool is_done {false};

    {
        std::unique_lock<std::mutex> lock(_request_mtx);
        if (!_is_running)
        {
            LOG_D("Request manager is stopped");
            return false;
        }

        _requests.emplace(&task_scope, true, &is_done);
    }

    _request_cv.notify_all();
    return true;
}
bool stop(Bn3Monkey::ScopedTaskScopeImpl& task_scope)
{

    LOG_V("Request manager to stop task scope (%s)", task_scope.name());
    bool is_done{ false };

    {
       {
            std::unique_lock<std::mutex> lock(_request_mtx);
            _requests.emplace(&task_scope, false, &is_done);
        }


       if (!_is_running)
       {
           LOG_D("Request manager is stopped");
           return false;
       }
        _request_cv.notify_all();
    }
    return true;
}

void testScopedTaskScope()
{
    bool error[100]{ false };

    error[1] = false;


    using namespace Bn3Monkey;
    auto* main = ScopedTaskScopeImpl::getScope("main",
        [](ScopedTaskScopeImpl& scope) {
            return start(scope);
        },
        [](ScopedTaskScopeImpl& scope) {
            return stop(scope);
        });

    auto* device = ScopedTaskScopeImpl::getScope("device",
        [](ScopedTaskScopeImpl& scope) {
            return start(scope);
        },
        [](ScopedTaskScopeImpl& scope) {
            return stop(scope);
        });

    auto* ip = ScopedTaskScopeImpl::getScope("ip",
        [](ScopedTaskScopeImpl& scope) {
            return start(scope);
        },
        [](ScopedTaskScopeImpl& scope) {
            return stop(scope);
        });

    _is_running = true;
    std::thread _thread = std::thread(manager);

    main->run("big task", []() {
        for (int count = 5; count != 0; count -= 1)
        {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.2s);
            printf("\nCount : %d\n\n", count);
        }
        });

    main->run("big task2", []() {
        for (int count = 3; count != 0; count -= 1)
        {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.2s);
            printf("\nSANS : %d\n\n", count);
        }
        });
    
    auto main1 = main->call("main1", [&]() {
        printf("\nMain1\n\n");
        auto deviceA = device->call("deviceA", [&]() {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.1s);
            printf("\ndeviceA\n\n");
            return 1;
            });
        auto deviceB = device->call("deviceB", [&]() {

            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.3s);
            printf("\ndeviceB\n\n");
            return 10;
            });

        auto ipA = ip->call("ipA", [&]() {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.2s);
            printf("\nipA\n\n");
            return 100;
            });
        auto ipB = ip->call("ipB", [&]() {

            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.4s);
            printf("\nipB\n\n");
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
        printf("\nSum : %d\n\n", sum);
        });
    main1.wait();
    
    printf("-------- Main 1 Relaunch! --------\n");

    main->call("main1", [&]() {
        printf("\nMain1\n\n");
        auto deviceA = device->call("deviceA", [&]() {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.1s);
            printf("\ndeviceA\n\n");
            return 1;
            });
        auto deviceB = device->call("deviceB", [&]() {

            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.3s);
            printf("\ndeviceB\n\n");
            return 10;
            });

        auto ipA = ip->call("ipA", [&]() {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.2s);
            printf("\nipA\n\n");
            return 100;
            });
        auto ipB = ip->call("ipB", [&]() {

            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.4s);
            printf("\nipB\n\n");
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
        printf("\nSum : %d\n\n", sum);
        });

    printf("\n-------- Main 1 Relaunch! --------\n\n");

    auto main11 = main->call("main1", [&]() {
        printf("\nMain1\n\n");
        auto deviceA = device->call("deviceA", [&]() {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.1s);
            printf("\ndeviceA\n\n");
            return 1;
            });
        auto deviceB = device->call("deviceB", [&]() {

            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.3s);
            printf("\ndeviceB\n\n");
            return 10;
            });

        auto ipA = ip->call("ipA", [&]() {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.2s);
            printf("\nipA\n\n");
            return 100;
            });
        auto ipB = ip->call("ipB", [&]() {

            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.4s);
            printf("\nipB\n\n");
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
        printf("\nSum : %d\n\n", sum);
        return sum;
        });

    auto main_ret = main11.wait();
    if (main_ret)
    {
        printf("\nmain_ret : %d\n\n", *main_ret);
    }

    auto main2 = main->call("main2", [&]() {
        printf("\nmain2\n\n");
        auto device2 = device->call("device2", [&]() {
            printf("\ndevice2\n\n");

            if (error[0])
            {
                printf("\nmaybe error\n\n");
                auto main3 = main->call("main3", [&]() {
                    return 4;
                    });
                auto* main3_result = main3.wait();
                return 3;
            }
        });
        auto* device2_ret = device2.wait();
        return 2;
    });
    auto ret2 = main2.wait();
    if (ret2)
    {
        printf("\nret2 : %d\n\n", *ret2);
    }

    auto main3 = main->call("main3", [&]() {
        printf("\nmain3\n\n");
        auto device3 = device->call("device3", [&]() {
            printf("\ndevice3\n\n");
            auto ip3 = ip->call("ip3", [&]() {
                printf("\nip3\n\n");
                printf("\nmaybe error!\n\n");
                if (error[1])
                {
                    auto main4 = main->call("main4", [&]() {
                        printf("\nmain4\n\n");
                        });
                    return 4;
                }
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
        printf("\nret3 : %d\n\n", *ret3);
    }


    using namespace std::chrono_literals;

    class SANS
    {
    public:
        SANS(int a)
        {
            this->a = a;
        }
        int a;
    };

    auto ip_start = ip->call("ip_start", [&]() {
        printf("\nip Start\n\n");
        return SANS(2);
        });

    std::this_thread::sleep_for(9.5s);
    auto* sans2 = ip_start.wait();
    if (sans2)
    {
        printf("\nSANS : %d\n\n", sans2->a);
    }

    std::this_thread::sleep_for(1s);
    auto ip_end = ip->call("ip_end", [&]() {
        printf("\nip End\n\n");
        return SANS(4);
        });
    auto* sans4 = ip_end.wait();
    if (sans4)
    {
        printf("\nSANS : %d\n\n", sans4->a);
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
