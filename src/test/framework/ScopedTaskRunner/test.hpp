#include <ScopedTask/ScopedTask.hpp>
#include "../test_helper.hpp"

using namespace std::chrono_literals;


void test_run(bool value)
{
	if (!value)
		return;

	using namespace Bn3Monkey;

	say("RUN TEST");

	ScopedTaskRunner().initialize();

	auto main = ScopedTaskScope(Bn3Tag("main"));
	auto device = ScopedTaskScope(Bn3Tag("device"));
	auto ip = ScopedTaskScope(Bn3Tag("ip"));

	main.run(Bn3Tag("main1"), [&]() {
		say("Main1");

		});

	main.run(Bn3Tag("main2"), [&]() {

		device.run(Bn3Tag("device1"), [&]() {
			say("Device1");
			});


		main.run(Bn3Tag("main3"), [&]() {
			say("Main3");
			});

		main.run(Bn3Tag("main4"), [&]() {
			say("Main4");
			});

		say("Main 2");

		
		});

	device.run(Bn3Tag("device2"), [&]() {
		say("Device2");
		});

	ip.run(Bn3Tag("ip1"), [&]() {
		say("IP1");
		});

	device.run(Bn3Tag("device3"), [&]() {
		say("Device3");
		});

	ip.run(Bn3Tag("ip2"), [&]() {
		say("IP2");
		});


	main.run(Bn3Tag("(1) main"), [&]() {
		device.run(Bn3Tag("(2) device"), [&]() {
			ip.run(Bn3Tag("(3) ip"), [&]() {
				main.run(Bn3Tag("(4) main"), [&]() {
					device.run(Bn3Tag("(5) device"), [&]() {
						ip.run(Bn3Tag("(6) ip"), [&]() {
								say("(6) ip");

							});

						say("(5) device");
						});

					say("(4) main");
					});

				});
				say("(3) ip");

			say("(2) device");
			});

		say("(1) main");
		});

	using namespace std::chrono_literals;
	std::this_thread::sleep_for(50ms);

	ScopedTaskRunner().release();
}

void test_runCancelled(bool value)
{
	if (!value)
		return;

	say("CANCEL TEST");

	using namespace Bn3Monkey;

	for (unsigned long long value = 0; value < 20; value++)
	{
		say("Ms value : %d", value);

		ScopedTaskRunner().initialize();

		auto main = ScopedTaskScope(Bn3Tag("main"));
		auto device = ScopedTaskScope(Bn3Tag("device"));
		auto ip = ScopedTaskScope(Bn3Tag("ip"));

		main.run(Bn3Tag("(1) main"), [&]() {
			device.run(Bn3Tag("(2) device"), [&]() {
				ip.run(Bn3Tag("(3) ip"), [&]() {
					main.run(Bn3Tag("(4) main"), [&]() {
						device.run(Bn3Tag("(5) device"), [&]() {
							ip.run(Bn3Tag("(6) ip"), [&]() {
								say("(6) ip");

								});

							say("(5) device");
							});

						say("(4) main");
						});

					});
				say("(3) ip");

				say("(2) device");
				});

			say("(1) main");
			});


		std::this_thread::sleep_for(std::chrono::microseconds(value));
		ScopedTaskRunner().release();
	}
}

int call_and_collect(const char* task_name, Bn3Monkey::ScopedTaskScope& scope, std::function<int()> inner)
{
	auto task = scope.call(Bn3Monkey::Bn3Tag(task_name), inner);
	auto result = task.wait();
	if (result)
	{
		say("%s Result : %d", task_name);
		return *result;
	}
	return 0;
}

void test_call(bool value)
{
	if (!value)
		return;

	say("CALL TEST");

	using namespace Bn3Monkey;

	ScopedTaskRunner().initialize();

	auto main = ScopedTaskScope(Bn3Tag("main"));
	auto device = ScopedTaskScope(Bn3Tag("device"));
	auto ip = ScopedTaskScope(Bn3Tag("ip"));
	auto ui = ScopedTaskScope(Bn3Tag("ui"));

	std::vector<ScopedTaskResult<int>> tasks;

	for (int i = 1; i <= 10; i++)
	{
		auto task = main.call(Bn3Tag("Main1"), [&, i]() {
			say("Main1 Called (%d)", i);
			int sum = 0;

			auto device1 = device.call(Bn3Tag("Device1"), [&, i]() {
				say("Device1 Called (%d)", i);
				return i*1;
				});

			auto ip1 = ip.call(Bn3Tag("Ip1"), [&, i]() {
				say("Ip1 Called (%d)", i);
				return i * 1000;
				});

			auto device2 = device.call(Bn3Tag("Device2"), [&, i]() {
				say("Device2 Called (%d)", i);
				return i * 10;
				});

			auto device3 = device.call(Bn3Tag("Device3"), [&, i]() {
				say("Device3 Called (%d)", i);
				int sum = 0;
				auto ui1 = ui.call(Bn3Tag("UI1"), [&, i]() {
					say("UI1 Called (%d)", i);
					return i * 20;
					});
				auto ui2 = ui.call(Bn3Tag("UI2"), [&, i]() {
					say("UI2 Called (%d)", i);
					return i * 80;
					});

				{
					auto ret = ui1.wait();
					if (ret)
					{
						say("UI 1 Waited (%d)", i);
						sum += *ret;
					}
				}

				{
					auto ret = ui2.wait();
					if (ret)
					{
						say("UI 2 Waited (%d)", i);
						sum += *ret;
					}
				}

				return sum;
				});

			auto ip2 = ip.call(Bn3Tag("Ip2"), [&, i]() {
				say("Ip2 Called (%d)", i);
				return i * 10000;
				});

			{
				auto result = device1.wait();
				if (result)
				{
					say("Device 1 Waited (%d)", i);
					sum += *result;
				}
			}

			{
				auto result = device2.wait();
				if (result)
				{
					say("Device 2 Waited (%d)", i);
					sum += *result;
				}
			}

			{
				auto result = device3.wait();
				if (result)
				{
					say("Device 3 Waited (%d)", i);
					sum += *result;
				}
			}

			{
				auto result = ip1.wait();
				if (result)
				{
					say("IP 1 Waited (%d)", i);
					sum += *result;
				}
			}

			{
				auto result = ip2.wait();
				if (result)
				{
					say("IP 2 Waited (%d)", i);
					sum += *result;
				}
			}

			return sum;
			});

		tasks.push_back(std::move(task));
	}

	for (size_t i = 1; i <= 10; i++)
	{
		auto& task = tasks[i-1];
		auto result = task.wait();
		if (result)
		{
			say("%s (%d) Result : %d", "Main1", i, *result);
		}
		if (i == 9)
		{
			std::this_thread::sleep_for(10s);
		}
	}

 	ScopedTaskRunner().release();
}

void test_looper(bool value)
{
	if (!value)
		return;

	say("Looper TEST");

	using namespace Bn3Monkey;

	ScopedTaskRunner().initialize();

	ScopedTaskLooper looper1(Bn3Tag("10ms"));
	ScopedTaskLooper looper2(Bn3Tag("50ms"));
	ScopedTaskLooper looper3(Bn3Tag("100ms"));
	ScopedTaskLooper looper4(Bn3Tag("500ms"));
	ScopedTaskLooper looper5(Bn3Tag("1s"));

	auto main = ScopedTaskScope(Bn3Tag("main"));
	auto device = ScopedTaskScope(Bn3Tag("device"));
	auto ip = ScopedTaskScope(Bn3Tag("ip"));

	std::mutex mtx;
	long long int time = 0;

	std::vector<std::chrono::milliseconds> times[5];

	using namespace std::chrono_literals;
	looper1.start(1s, main, [&]() {
		{
			static std::chrono::steady_clock::time_point prev_time = std::chrono::steady_clock::now();
			auto duration = std::chrono::steady_clock::now() - prev_time;
			std::chrono::milliseconds duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
			times[0].push_back(duration_ms);
		}
		});
	looper2.start(2s, main, [&]() {
		{
			static std::chrono::steady_clock::time_point prev_time = std::chrono::steady_clock::now();
			auto duration = std::chrono::steady_clock::now() - prev_time;
			std::chrono::milliseconds duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
			times[1].push_back(duration_ms);
		}
		});
	looper3.start(3s, device, [&]() {
		{
			static std::chrono::steady_clock::time_point prev_time = std::chrono::steady_clock::now();
			auto duration = std::chrono::steady_clock::now() - prev_time;
			std::chrono::milliseconds duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
			times[2].push_back(duration_ms);
		}
		});
	looper4.start(4s, ip, [&]() {
		{
			static std::chrono::steady_clock::time_point prev_time = std::chrono::steady_clock::now();
			auto duration = std::chrono::steady_clock::now() - prev_time;
			std::chrono::milliseconds duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
			times[3].push_back(duration_ms);
		}
		});
	looper5.start(5s, main, [&]() {
		{
			static std::chrono::steady_clock::time_point prev_time = std::chrono::steady_clock::now();
			auto duration = std::chrono::steady_clock::now() - prev_time;
			std::chrono::milliseconds duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
			times[4].push_back(duration_ms);
		}
		});

	std::this_thread::sleep_for(10s);

	looper1.stop();
	looper2.stop();
	looper3.stop();
	looper4.stop();
	looper5.stop();

	ScopedTaskRunner().release();
}

void testScopedTaskRunner(bool value)
{
	if (!value)
		return;

	Bn3Monkey::Bn3MemoryPool::initialize({ 32, 32, 128, 32, 32, 32, 32, 32, 4 });

	test_looper(true);

	for (size_t i = 0; i < 100; i++)
	{
		test_run(true);
		test_runCancelled(true);
		test_call(true);
	}

	printf("%s\n", analyzer.analyzeAll().c_str());
	printf("%s\n", analyzer.analyzePool(0).c_str());

	Bn3Monkey::Bn3MemoryPool::release();
}