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
			std::function<void(ScopedTaskLooperImpl&)> onAdd,
			std::function<void(ScopedTaskLooperImpl&)> onRemove);
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

		// std::chrono::duration<long long, std::chrono::microseconds> _interval;

		std::function<void(ScopedTaskLooperImpl&)> _onAdd;
		std::function<void(ScopedTaskLooperImpl&)> _onRemove;

		friend class ScopedTaskLooperScheduler;
	};

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

		inline std::function<void(ScopedTaskLooperImpl&)>& onAdd() {
			return _onAdd;
		}

		inline std::function<void(ScopedTaskLooperImpl&)>& onRemove() {
			return _onRemove;
		}

		ScopedTaskLooperImpl& getLooper(const Bn3Tag& tag, const ScopedTaskScopeImpl& scope, std::function<void()> task);

	private:
		void start();
		void stop();
		void routine();

		void add(ScopedTaskLooperImpl& looper);
		void remove(ScopedTaskLooperImpl& looper);

		std::function<void()> _onStart;
		std::function<void()> _onStop;
		std::function<void(ScopedTaskLooperImpl&)> _onAdd;
		std::function<void(ScopedTaskLooperImpl&)> _onRemove;

		// std::vector<ScopedTask> _loopers;
		// std::vector<ScopedTask&> _activated_loopers;

		bool _is_running;
		std::mutex _mtx;
		std::condition_variable _cv;
	};

	
}

#endif