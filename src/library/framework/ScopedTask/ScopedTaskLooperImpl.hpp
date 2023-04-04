#ifndef __BN3MONKEY_TASK_LOOPER__
#define __BN3MONKEY_TASK_LOOPER__

#include "../Tag/Tag.hpp"
#include "ScopedTaskScopeImpl.hpp"

#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <unordered_map>

namespace Bn3Monkey
{
	class ScopedTaskLooperScheduler;

	class ScopedTaskLooperImpl
	{
	public:
		ScopedTaskLooperImpl(
			const Bn3Tag& tag, 
			const ScopedTaskScopeImpl& scope, 
			std::function<void()> task,
			std::function<ScopedTaskLooperImpl&>);
		ScopedTaskLooperImpl(const ScopedTaskLooperImpl& other) = delete;
		ScopedTaskLooperImpl(ScopedTaskLooperImpl&& other);

		void start(std::chrono::microseconds us);
		void start(std::chrono::milliseconds ms);
		void start(std::chrono::seconds s);
		void stop();

	private:
		Bn3Tag tag;
		ScopedTaskScopeImpl _scope;
		std::function<void()> _task;

		std::chrono::duration<long long, std::chrono::microseconds> _interval;

		friend class ScopedTaskLooperScheduler;
	}

	class ScopedTaskLooperScheduler
	{
	public:
		ScopedTaskLooperScheduler();

		inline std::function<void()>& onStart() {
			return _onStart;
		}
		inline std::function<void()>& onStop() {
			return _onStop;
		}

		ScopedTaskLooperImpl& getLooper(const Bn3Tag& tag, const ScopedTaskScopeImpl& scope, std::function<void()> task);

	private:
		void start();
		void stop();
		void routine();

		std::function<void()> _onStart;
		std::function<void()> _onStop;

		std::vector<ScopedTask> 
		std::vector<ScopedTask&>

		bool _is_running;
		std::mutex _mtx;
		std::condition_variable _cv;
	};

	
}

#endif