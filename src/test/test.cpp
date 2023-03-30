#include "framework/ScopedTaskRunner/test.hpp"

void main()
{
    testScopedTaskRunner();
    /*
    using namespace std::chrono_literals;
    std::mutex mtx;
    std::mutex mtx2;
    std::condition_variable cv;
    int a = 0;

    std::thread thread([&]() {
        {
            std::unique_lock<std::mutex> lock(mtx);
            printf("Lock Start!\n");
            for (int i = 0; i < 10; i++)
            {
                std::this_thread::sleep_for(10ms);
                a += 1;
                printf("thread : %d\n", a);
            }
            printf("Lock End!\n");
        }
        });

    {
        std::unique_lock<std::mutex> lock(mtx2);
        printf("Wait Start!\n");
        bool ret = cv.wait_for(lock, 50ms, [&]() {
            return false;
            });

        if (!ret)
        {
            printf("Time Out!\n");
        }

        printf("%d\n", a);
        printf("Wait End!\n");
        for (int i = 0; i < 10; i++)
        {
            printf("It may be stealed a from thread : %d\n", a);
        }
    }

    thread.join();
    printf("what the fuck!\n");
    */

    /*
    class Fuck
    {
    public:
        Fuck(int* ptr) : ptr(ptr) {
            printf("create\n");
        }
        Fuck(Fuck&& other) : ptr(std::move(other.ptr)) {
            printf("move\n");
        }
        Fuck(const Fuck& other) : ptr(other.ptr)
        {
            printf("copy\n");
        }
        Fuck(const Fuck& other) = delete;
        ~Fuck() {
            printf("removed\n");
        }

        int* ptr;
    };

    int a = 2;
    Fuck f(&a);
    std::function<void()> fun1{ [f(std::move(f))] () {
        printf("%d\n", *f.ptr);
    } };
    auto fun2 = fun1;

    fun2();
    fun1();
    */

    return;
}
