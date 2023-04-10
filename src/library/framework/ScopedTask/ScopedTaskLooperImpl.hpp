#ifndef __BN3MONKEY_TASK_LOOPER__
#define __BN3MONKEY_TASK_LOOPER__

#include "../Tag/Tag.hpp"
#include "ScopedTaskScopeImpl.hpp"

#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <unordered_map>

#ifdef BN3MONKEY_DEBUG
#define FOR_DEBUG(t) t
#else 
#define FOR_DEBUG(t)
#endif

#include "../Log/Log.hpp"

#ifdef __BN3MONKEY_LOG__
#ifdef BN3MONKEY_DEBUG
#define LOG_D(text, ...) Bn3Monkey::Log::D(__FUNCTION__, text, __VA_ARGS__)
#else
#define LOG_D(text, ...)
#endif
#define LOG_V(text, ...) Bn3Monkey::Log::V(__FUNCTION__, text, __VA_ARGS__)
#define LOG_E(text, ...) Bn3Monkey::Log::E(__FUNCTION__, text, __VA_ARGS__)
#else
#define LOG_D(text, ...)    
#define LOG_V(text, ...) 
#define LOG_E(text, ...)
#endif

#include "../MemoryPool/MemoryPool.hpp"

#ifdef __BN3MONKEY_MEMORY_POOL__
#define MAKE_SHARED(TYPE, TAG, ...) Bn3Monkey::makeSharedFromMemoryPool<TYPE>(TAG, __VA_ARGS__)
#define Bn3List(TYPE) Bn3Monkey::Bn3Container::list<TYPE>
#define Bn3Queue(TYPE) Bn3Monkey::Bn3Container::queue<TYPE>
#define Bn3Map(KEY, VALUE) Bn3Monkey::Bn3Container::map<KEY, VALUE>
#define Bn3String() Bn3Monkey::Bn3Container::string
#define Bn3Vector(TYPE) Bn3Monkey::Bn3Container::vector<TYPE>
#define Bn3Deque(TYPE) Bn3Monkey::Bn3Container::deque<TYPE>

#define Bn3ListAllocator(TYPE, TAG) Bn3Monkey::Bn3Allocator<TYPE>(TAG)
#define Bn3QueueAllocator(TYPE, TAG) Bn3Monkey::Bn3Allocator<TYPE>(TAG)
#define Bn3MapAllocator(KEY, VALUE, TAG) Bn3Monkey::Bn3Allocator<std::pair<const KEY, VALUE>>(TAG)
#define Bn3StringAllocator(TAG) Bn3Monkey::Bn3Allocator<char>(TAG)
#define Bn3VectorAllocator(TYPE, TAG) Bn3Monkey::Bn3Allocator<TYPE>(TAG)
#define Bn3DequeAllocator(TYPE, TAG) Bn3Monkey::Bn3Allocator<TYPE>(TAG)

#else
#define MAKE_SHARED(TYPE, TAG, ...) std::shared_ptr<TYPE>(new TYPE(__VA_ARGS__))
#define Bn3List(TYPE) std::list<TYPE>
#define Bn3Queue(TYPE) std::queue<TYPE>
#define Bn3Map(KEY, VALUE) std::unordered_map<KEY, VALUE>
#define Bn3String() std::string
#define Bn3Vector(TYPE) std::vector<TYPE>
#define Bn3Deque(TYPE) std::deque<TYPE>

#define Bn3ListAllocator(TYPE, TAG)
#define Bn3QueueAllocator(TYPE, TAG) 
#define Bn3MapAllocator(KEY, VALUE, TAG) 
#define Bn3StringAllocator(TAG) 
#define Bn3VectorAllocator(TYPE, TAG) 
#define Bn3DequeAllocator(TYPE, TAG)
#endif

namespace Bn3Monkey
{
	class ScopedTaskLooperScheduler;

	class ScopedTaskLooperImpl
	{
	public:
		ScopedTaskLooperImpl(
			const Bn3Tag& looper_name,
			std::function<void(ScopedTaskLooperImpl&)> onAdd,
			std::function<void(ScopedTaskLooperImpl&)> onRemove) : _name(looper_name), _onAdd(onAdd), _onRemove(onRemove)
		{}

		ScopedTaskLooperImpl(const ScopedTaskLooperImpl& other) = delete;
		ScopedTaskLooperImpl(ScopedTaskLooperImpl&& other);

		template<class Func, class... Args>
		void start(std::chrono::microseconds interval, ScopedTaskScopeImpl& scope, Func func, Args... args)
		{
			if (!is_started)
			{
				LOG_D("looper %s runs every %lld us", static_cast<long long int>(interval));
				_is_started = true;
				_interval = std::chrono::duration_cast<std::chrono::steady_clock::duration>(interval);
				_scope = &scope;
				_task = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
				_onAdd(*this);
			}
		}


		template<class Func, class... Args>
		void start(std::chrono::milliseconds interval, ScopedTaskScopeImpl& scope, Func func, Args... args)
		{
			if (!_is_started)
			{
				LOG_D("looper %s runs every %lld ms", _name.str(), interval.count());
				_is_started = true;
				_interval = std::chrono::duration_cast<std::chrono::steady_clock::duration>(interval);
				_scope = &scope;
				_task = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);

				_onAdd(*this);
			}
		}


		template<class Func, class... Args>
		void start(std::chrono::seconds interval, ScopedTaskScopeImpl& scope, Func func, Args... args)
		{
			if (!_is_started)
			{
				LOG_D("looper %s runs every %lld s", _name.str(), interval.count());

				_is_started = true;

				_interval = std::chrono::duration_cast<std::chrono::steady_clock::duration>(interval);
				_scope = &scope;
				_task = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);

				_onAdd(*this);
			}
		}

		void stop()
		{
			if (_is_started)
			{
				_is_started = false;
				
				_scope = nullptr;
				_task = nullptr;

				_onRemove(*this);
			}
		}

	private:
		Bn3Tag _name;
		std::function<void(ScopedTaskLooperImpl&)> _onAdd;
		std::function<void(ScopedTaskLooperImpl&)> _onRemove;

		std::chrono::steady_clock::duration _interval;
		std::chrono::steady_clock::time_point _next_launch_time;

		ScopedTaskScopeImpl* _scope {nullptr};
		std::function<void()> _task;		
		bool _is_started {false};

		friend class ScopedTaskLooperScheduler;
	};

	class ScopedTaskLooperScheduler
	{
	public:
		ScopedTaskLooperScheduler();

		void start();
		void stop();

		inline std::function<void(ScopedTaskLooperImpl&)>& onAdd() {
			return _onAdd;
		}

		inline std::function<void(ScopedTaskLooperImpl&)>& onRemove() {
			return _onRemove;
		}

		ScopedTaskLooperImpl& getLooper(const Bn3Tag& looper_name);

	private:
		void routine();

		void add(ScopedTaskLooperImpl& looper);
		void remove(ScopedTaskLooperImpl& looper);

		std::function<void()> _onStart;
		std::function<void()> _onStop;
		std::function<void(ScopedTaskLooperImpl&)> _onAdd;
		std::function<void(ScopedTaskLooperImpl&)> _onRemove;

		Bn3Deque(ScopedTaskLooperImpl) _loopers;
		Bn3List(ScopedTaskLooperImpl*) _activated_loopers;

		bool _is_running;
		std::mutex _mtx;
		std::condition_variable _cv;

		std::thread _thread;
	};

}
#endif