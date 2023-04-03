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

	auto main = ScopedTaskScope("main");
	auto device = ScopedTaskScope("device");
	auto ip = ScopedTaskScope("ip");

	main.run("main1", [&]() {
		say("Main1");

		});

	main.run("main2", [&]() {

		device.run("device1", [&]() {
			say("Device1");
			});


		main.run("main3", [&]() {
			say("Main3");
			});

		main.run("main4", [&]() {
			say("Main4");
			});

		say("Main 2");

		
		});

	device.run("device2", [&]() {
		say("Device2");
		});

	ip.run("ip1", [&]() {
		say("IP1");
		});

	device.run("device3", [&]() {
		say("Device3");
		});

	ip.run("ip2", [&]() {
		say("IP2");
		});


	main.run("(1) main", [&]() {
		device.run("(2) device", [&]() {
			ip.run("(3) ip", [&]() {
				main.run("(4) main", [&]() {
					device.run("(5) device", [&]() {
						ip.run("(6) ip", [&]() {
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

		auto main = ScopedTaskScope("main");
		auto device = ScopedTaskScope("device");
		auto ip = ScopedTaskScope("ip");

		main.run("(1) main", [&]() {
			device.run("(2) device", [&]() {
				ip.run("(3) ip", [&]() {
					main.run("(4) main", [&]() {
						device.run("(5) device", [&]() {
							ip.run("(6) ip", [&]() {
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
	auto task = scope.call(task_name, inner);
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

	auto main = ScopedTaskScope("main");
	auto device = ScopedTaskScope("device");
	auto ip = ScopedTaskScope("ip");
	auto ui = ScopedTaskScope("ui");

	std::vector<ScopedTaskResult<int>> tasks;

	for (int i = 1; i <= 10; i++)
	{
		auto task = main.call("Main1", [&, i]() {
			say("Main1 Called (%d)", i);
			int sum = 0;

			auto device1 = device.call("Device1", [&, i]() {
				say("Device1 Called (%d)", i);
				return i*1;
				});

			auto ip1 = ip.call("Ip1", [&, i]() {
				say("Ip1 Called (%d)", i);
				return i * 1000;
				});

			auto device2 = device.call("Device2", [&, i]() {
				say("Device2 Called (%d)", i);
				return i * 10;
				});

			auto device3 = device.call("Device3", [&, i]() {
				say("Device3 Called (%d)", i);
				int sum = 0;
				auto ui1 = ui.call("UI1", [&, i]() {
					say("UI1 Called (%d)", i);
					return i * 20;
					});
				auto ui2 = ui.call("UI2", [&, i]() {
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

			auto ip2 = ip.call("Ip2", [&, i]() {
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

void testScopedTaskRunner()
{

	Bn3Monkey::Bn3MemoryPool::initialize(32, 32, 32, 32, 32, 32, 32, 32);

	for (size_t i = 0; i < 100; i++)
	{
		test_run(true);
		test_runCancelled(true);
		test_call(false);
	}
	Bn3Monkey::Bn3MemoryPool::release();
}