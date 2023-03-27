/*
#include <Windows.h>
#include <tchar.h>
#include <cstdio>
#include <functional>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

class Notifier;
class RResult;

void setResult(RResult* result, int value);


std::mutex mtx2;
std::condition_variable cv2;

class Notifier
{
public:
	friend void linkResultToNotifier(Notifier* notifier, RResult* result);
	friend void linkNotifierToResult(RResult* result, Notifier* notifier);
	friend void unlinkResultToNotifier(Notifier* notifier, RResult* result);
	friend void unlinkNotifierToResult(RResult* result, Notifier* notifier);

	void printReference() {
		printf("Notifier -> Notifier : %p RResult : %p\n", this, _result);
	}

	Notifier(RResult* result) {
		printf("Notifier created\n");
		_result = result;
		linkNotifierToResult(_result, this);
		
	}
	Notifier(const Notifier& other)
	{
		printf("Notifier copied\n");
		_result = other._result;
		linkNotifierToResult(_result, this);
	}
	Notifier(Notifier&& other)
	{
		printf("Notifier moved\n");
		_result = other._result;
		linkNotifierToResult(_result, this);
		other._result = nullptr;
	}


	void notify(int value) {
		printf("notify Value (%d)\n", value);
		if (_result)
		{
			setResult(_result, value);
		}
		
	}

	RResult* _result;
};

class RResult
{
public:
	friend void linkResultToNotifier(Notifier* notifier, RResult* result);
	friend void linkNotifierToResult(RResult* result, Notifier* notifier);
	friend void unlinkResultToNotifier(Notifier* notifier, RResult* result);
	friend void unlinkNotifierToResult(RResult* result, Notifier* notifier);

	void printReference() {
		printf("Result -> Notifier : %p RResult : %p\n", _notifier, this);
	}

	Notifier getNotifier() {
		Notifier ret(this);
		return ret;
	}

	RResult() {
		printf("Result created\n");
	}
	~RResult() {
		printf("Result released\n");
		unlinkResultToNotifier(_notifier, this);
	}
	RResult(const RResult& other)
	{
		printf("Result copied\n");
		_notifier = other._notifier;
		linkResultToNotifier(_notifier, this);
	}
	RResult(RResult&& other)
	{
		printf("Result moved\n");
		_notifier = other._notifier;
		linkResultToNotifier(_notifier, this);
		other._notifier = nullptr;
	}
	void set(int value) {
		printf("Set Value (%d)\n", value); 
		{
			std::unique_lock<std::mutex> lock(mtx2);
			_is_finished = true;
			_value = value;
		}
		cv2.notify_all();
	}
	int wait() {
		using namespace std::chrono_literals;
		{
			std::unique_lock<std::mutex> lock(mtx2);
			cv2.wait(lock, [&]() {
				return _is_finished;
				});
		}
		printf("Value is %d\n", _value);
		return _value;
	}

	bool _is_finished{false};
	int _value{ 0 };
	Notifier* _notifier{nullptr};
};

void linkResultToNotifier(Notifier* notifier, RResult* result)
{
	printf("link result(%p) to notifier (%p)\n", result, notifier);
	if (notifier)
		notifier->_result = result;
	
}
void linkNotifierToResult(RResult* result, Notifier* notifier)
{
	printf("link result(%p) to notifier (%p)\n", result, notifier);
	if (result)
		result->_notifier = notifier;
}
void unlinkResultToNotifier(Notifier* notifier, RResult* result)
{
	printf("unlink result(%p) to notifier (%p)\n", result, notifier);
	if (notifier)
		notifier->_result = nullptr;
}
void unlinkNotifierToResult(RResult* result, Notifier* notifier)
{
	printf("unlink notifier(%p) to result (%p)\n", notifier, result);
	if (result)
		result->_notifier = nullptr;
}
void setResult(RResult* result, int value)
{
	if (result)
		result->set(value);
}

bool is_activiated;
std::queue<std::function<void()>> queue;
std::mutex mtx;
std::condition_variable cv;


RResult makeResult()
{
	RResult result;
	Notifier notifier = result.getNotifier();
	printf("----\n");

	std::function<void()> task = [notifier=notifier]() mutable {
		notifier.printReference();
		notifier.notify(4);
	};
	printf("----\n");
	{
		std::unique_lock<std::mutex> lock(mtx);
		queue.push(std::move(task));
	}
	printf("----\n");
	cv.notify_one();
	return result;
}

int main()
{
	//ShellExecute(NULL, L"open", L"E:\\À½¾Ç\\private_music\\another\\Empty.gp", NULL, NULL, SW_SHOWNORMAL);
	std::thread _thread([]() {
		is_activiated = true;
		for (;;)
		{
			std::function<void()> task;
			{
				std::unique_lock<std::mutex> lock(mtx);
				cv.wait(lock, []() {
					return !is_activiated || !queue.empty();
					});
				if (!is_activiated)
					break;
				task = std::move(queue.front());
				printf("----\n");
				queue.pop();
			}

			printf("task is processed\n");
			task();
		}
	});

	auto result = makeResult();
	printf("----\n");
	result.printReference();
	printf("----\n");
	int a = result.wait();
	printf("Get Value (%d)\n", a);

	is_activiated = false;
	cv.notify_all();
	_thread.join();
	return 0;
}
*/
