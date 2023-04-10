#include "ScopedTaskLooperImpl.hpp"

/*
Bn3Monkey::ScopedTaskLooperImpl::ScopedTaskLooperImpl(ScopedTaskLooperImpl&& other) : 
	_name(std::move(other._name)), 
	_onAdd(std::move(other._onAdd)),
	_onRemove(std::move(other._onRemove)),
	_interval(std::move(other._interval)),
	_scope(std::move(other._scope)),
	_task(std::move(other._task)),
	_is_started(std::move(other._is_started))

{

}


Bn3Monkey::ScopedTaskLooperScheduler::ScopedTaskLooperScheduler()
{
	_onStart = [&]() {
		start();
	};
	_onStop = [&]() {
		stop();
	};
	_onAdd = [&](ScopedTaskLooperImpl& looper) {
		add(looper);
	};
	_onRemove = [&](ScopedTaskLooperImpl& looper) {
		remove(looper);
	};

	_loopers = Bn3Deque(ScopedTaskLooperImpl) { Bn3DequeAllocator(ScopedTaskLooperImpl, Bn3Tag("loopers")) };
	_activated_loopers = Bn3List(ScopedTaskLooperImpl&) { Bn3ListAllocator(ScopedTaskLooperImpl&, Bn3Tag("activated_loopers")) };

}



Bn3Monkey::ScopedTaskLooperImpl& Bn3Monkey::ScopedTaskLooperScheduler::getLooper(const Bn3Tag& looper_name)
{
	_loopers.emplace_back(looper_name, _onAdd, _onRemove);
	auto& ret = _loopers.back();
	return ret;
}

void Bn3Monkey::ScopedTaskLooperScheduler::start()
{
	{
		std::unique_lock<std::mutex> lock(_mtx);
		_is_running = true;
		_thread = std::thread([&]() { routine(); });
	}
}

void Bn3Monkey::ScopedTaskLooperScheduler::stop()
{
	bool temp{ false };
	{
		std::unique_lock<std::mutex> lock(_mtx);
		if (_is_running)
		{
			_is_running = false;
			temp = true;
		}
	}

	if (temp)
	{
		_cv.notify_all();
		_thread.join();
	}
}

void Bn3Monkey::ScopedTaskLooperScheduler::routine()
{
	for (;;)
	{
		{
			std::unique_lock<std::mutex> lock(_mtx);
			
			if (_activated_loopers.empty())
			{
				_cv.wait(lock, [&]() {
					return !_is_running || !_activated_loopers.empty();
					});
			}
			else
			{
				auto now = std::chrono::steady_clock::now();

				for (auto& looper : _activated_loopers)
				{
					if (now >= looper._next_launch_time)
					{
						looper._task();
						looper._next_launch_time += looper._interval;
					}

				}

				std::chrono::steady_clock::time_point wait_until_time;
				_cv.wait_until(lock, wait_until_time, [&]() {
					return !_is_running || !_activated_loopers.empty();
					});
			}

			if (!_is_running)
				break;
		}
	}
}

void Bn3Monkey::ScopedTaskLooperScheduler::add(ScopedTaskLooperImpl& looper)
{
	{
		std::unique_lock<std::mutex> lock(_mtx);
		for (auto& activated_looper : _activated_loopers)
			if (activated_looper._name == looper._name)
			{
				return;
			}

		looper._next_launch_time = std::chrono::steady_clock::now();
		_activated_loopers.push_back(looper);
	}
}

void Bn3Monkey::ScopedTaskLooperScheduler::remove(ScopedTaskLooperImpl& looper)
{
	{
		std::unique_lock<std::mutex> lock(_mtx);
		for (auto& iter = _activated_loopers.begin(); iter != _activated_loopers.end(); iter++)
		{
			if (iter->_name == looper._name)
			{
				_activated_loopers.erase(iter);
				break;
			}
		}
	}
}
*/
