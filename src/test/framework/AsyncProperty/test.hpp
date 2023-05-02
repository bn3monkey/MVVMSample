#include <MemoryPool/MemoryPool.hpp>
#include <ScopedTask/ScopedTask.hpp>
#include <AsyncProperty/AsyncProperty.hpp>

#include "test_helper.hpp"
#include "../test_helper.hpp"

#include <fstream>

void test_asyncpropertycontainer(bool value)
{
	if (!value)
		return;

	using namespace Bn3Monkey;

	AsyncPropertyContainer container{Bn3Tag("test"), ScopedTaskScope(Bn3Tag("main"))};
	
	char* content = new char[1024 * 1024];
	memset(content, 0, 1024 * 1024);

	std::ifstream ifs;
	ifs.open("test.json");
	if (ifs.is_open())
	{
		ifs.read(content, 1024 * 1024);
	}


	container.create(content);
	
	delete content;
	return;
}

void test_asyncproperty(bool value)
{
	if (!value)
		return;

	using namespace Bn3Monkey;

	ScopedTaskScope main_scope{ Bn3Tag("main") };
	ScopedTaskScope device_scope{ Bn3Tag("device") };
	ScopedTaskScope ip_scope{ Bn3Tag("ip") };
	ScopedTaskScope ui_scope{ Bn3Tag("ui") };

	
	struct Application
	{
		double depth;
		int gain;

		void depthRefresh(double value)
		{
			say("UI Depth is changed");
			depth = value;
			say("UI Depth Notified : %f", depth);
		}
		void gainRefresh(int value)
		{
			say("UI Gain is changed");
			gain = value;
			say("UI Gain Notified : %d", gain);
		}

		void errorMessage()
		{
			say("Error!\n");
		}
	} application;

	struct LibraryUI
	{
		LibraryUI(const ScopedTaskScope& scope) : depth(Bn3Tag("ui_depth"), scope, 2.5), gain(Bn3Tag("ui_gain"), scope, 2) {}
		AsyncProperty<double> depth;
		AsyncProperty<int> gain;
	} ui(ui_scope);

	struct LibraryMain
	{
		LibraryMain(const ScopedTaskScope& scope) : depth(Bn3Tag("main_depth"), scope, 2.5), gain(Bn3Tag("main_gain"), scope, 2) {}
		AsyncProperty<double> depth;
		AsyncProperty<int> gain;
	} main(main_scope);

	struct LibraryDevice
	{
		LibraryDevice(const ScopedTaskScope& scope) : depth(Bn3Tag("device_depth"), scope, 2.5), gain(Bn3Tag("device_gain"), scope, 2) {}
		AsyncProperty<double> depth;
		AsyncProperty<int> gain;
	} device(device_scope);

	struct LibraryIP
	{
		LibraryIP(const ScopedTaskScope& scope) : depth(Bn3Tag("ip_depth"), scope, 2.5), gain(Bn3Tag("ip_gain"), scope, 2) {}
		AsyncProperty<double> depth;
		AsyncProperty<int> gain;
	} ip(ip_scope);
	
		
	main.depth.registerOnPropertyNotified(device_scope, [&](const double& value) {
		say("Depth : Main -> Device");
		bool ret = device.depth.set(value);
		if (ret)
		{
			return device.depth.notify();
		}
		return false;
	});

	main.depth.registerOnPropertyNotified(ip_scope, [&](const double& value) {
		say("Depth : Main -> IP");
		bool ret= ip.depth.set(value);
		if (ret)
		{
			return ip.depth.notify();
		}
		return false;
	});

	main.depth.registerOnPropertyUpdated(ui_scope, [&](const double& value, bool success)
		{
			say("Depth : Main -> UI");
			if (success)
			{
				ui.depth.set(value);
				ui.depth.update(true);
			}
			else
			{
				ui.depth.update(false);
			}
		});

	main.gain.registerOnPropertyNotified(device_scope, [&](const int& value) {
		say("Gain : Main -> Device");
		bool ret = device.gain.set(value);
		if (ret)
		{
			return device.gain.notify();
		}
		return false;
	});

	main.gain.registerOnPropertyNotified(ip_scope, [&](const int& value) {
		say("Gain : Main -> IP");
		bool ret = ip.gain.set(value);
		if (ret)
		{
			return ip.gain.notify();
		}
		return false;
	});

	main.gain.registerOnPropertyUpdated(ui_scope, [&](const int& value, bool success)
		{
			say("Gain : Main -> UI");
			if (success)
			{
				ui.gain.set(value);
				ui.gain.update(true);
			}
			else
			{
				ui.gain.update(false);
			}
		});


	ui.depth.registerOnPropertyNotified(main_scope, [&](const double& value) {
		say("Depth : UI -> Main");
		bool ret = main.depth.set(value);
		if (ret)
		{
			return main.depth.notify();
		}
		return false;
	});

	ui.depth.registerOnPropertyUpdated(ui_scope, [&](const double& value, bool success) {
		say("Depth : UI -> Application");
		application.depthRefresh(value);
		});

	ui.gain.registerOnPropertyNotified(main_scope, [&](const int& value) {
		say("Gain : UI -> Main");
		bool ret = main.gain.set(value);
		if (ret)
		{
			return main.gain.notify();
		}
		return false;
	});

	ui.gain.registerOnPropertyUpdated(ui_scope, [&](const int& value, bool success) {
		say("Gain : UI -> Application");
		application.gainRefresh(value);
		});

	device.depth.registerOnPropertyNotified(device_scope, [&](const double& value) {
		say("Depth : Device -> Logic");
		say("Depth : Device (1)");
		say("Depth : Device (2)");
		say("Depth : Device (3)");
		return true;
	});

	device.gain.registerOnPropertyNotified(device_scope, [&](const double& value) {
		say("Gain : Device -> Logic");
		say("Gain : Device (1)");
		say("Gain : Device (2)");
		say("Gain : Device (3)");
		return true;
		});

	ip.depth.registerOnPropertyNotified(device_scope, [&](const double& value) {
		say("Depth : IP -> Logic");
		say("Depth : IP (1)");
		say("Depth : IP (2)");
		say("Depth : IP (3)");
		return true;
		});

	ip.gain.registerOnPropertyNotified(device_scope, [&](const double& value) {
		say("Gain : IP -> Logic");
		say("Gain : IP (1)");
		say("Gain : IP (2)");
		say("Gain : IP (3)");
		return true;
		});


	// 1. Set Synchronously

	ui.depth.set(4.0);
	ui.gain.set(5);
	ui.depth.notify();
	ui.gain.notify();

	{
		auto value = ui.depth.get();
		application.depthRefresh(value);
	}
	{
		auto value = ui.gain.get();
		application.gainRefresh(value);
	}

	// 1-2. Set Synchronously
	ui.depth.set(2.0);
	ui.gain.set(1);
	if (ui.depth.notify())
		ui.depth.update(true);
	if (ui.gain.notify())
		ui.gain.update(true);

	using namespace std::chrono_literals;
	std::this_thread::sleep_for(1s);

	// 2. Load From Library

	main.depth.setAsync(5.0);
	main.gain.setAsync(6);

	using namespace std::chrono_literals;
	std::this_thread::sleep_for(1s);

	// 3. Set Asychrnously

	ui.depth.setAsync(6.0);
	ui.gain.setAsync(7);

	using namespace std::chrono_literals;
	std::this_thread::sleep_for(1s);

	AsyncProperty<Bn3StaticString> test{ Bn3Tag("test"), main_scope, Bn3StaticString("Do you know kimchi?") };
	test.registerOnPropertyNotified(main_scope, [](const Bn3StaticString& other) {
		say("notified : %s\n", other.data());
		return true;
		});
	test.registerOnPropertyUpdated(main_scope, [](const Bn3StaticString& other, bool value) {
		if (value)
		{
			say("Updated : %s", other.data());
		}
		});


	test.set(Bn3StaticString("abcd"));
	auto ret = test.get();
	say("ret : %s", ret.data());

	if (test.notify())
		test.update(true);

	test.setAsync(Bn3StaticString("def"));

	using namespace std::chrono_literals;
	std::this_thread::sleep_for(1s);
	{
		auto ret = test.get();
		say("ret : %s", ret.data());
	}
	return;
}

void test_asyncpropertyarray(bool value)
{
	if (!value)
		return;

	using namespace Bn3Monkey;

	ScopedTaskScope main_scope{ Bn3Tag("main") };
	ScopedTaskScope device_scope{ Bn3Tag("device") };
	ScopedTaskScope ip_scope{ Bn3Tag("ip") };
	ScopedTaskScope ui_scope{ Bn3Tag("ui") };

	
	struct Application
	{
		double x[128];
		int y[128];

		void refreshX(const double* values, size_t start, size_t end)
		{
			
			std::copy(values, values + end - start, x + start);

			std::stringstream ss;
			ss << "UI x is changed\n";
			for (size_t i = 0; i < 128; i++)
			{
				ss << x[i] << " ";
				if (i % 8 == 7)
					ss << "\n";
			}
			ss << "\n";
			say(ss.str().c_str());
		}
		void refreshY(const int* values, size_t start, size_t end)
		{

			std::copy(values, values + end - start, y + start);

			std::stringstream ss;
			ss << "UI y is changed\n";
			for (size_t i = 0; i < 128; i++)
			{
				ss << y[i] << " ";
				if (i % 8 == 7)
					ss << "\n";
			}
			ss << "\n";
			say(ss.str().c_str());
		}

		void errorMessage()
		{
			say("Error!\n");
		}
	} application;

	double initial_x[128];
	int initial_y[128];

	for (size_t i = 0; i < 128; i++)
	{
		application.x[i] = initial_x[i] = (double)i;
		application.y[i] = initial_y[i] = i;
	}


	struct LibraryUI
	{
		LibraryUI(const ScopedTaskScope& scope, const double (&initial_x)[128], const int (&initial_y)[128]) 
			: x(Bn3Tag("ui_x"), scope, initial_x, 128), y(Bn3Tag("ui_y"), scope, initial_y, 128) {}
		AsyncPropertyArray<double, 128> x;
		AsyncPropertyArray<int, 128> y;
	} ui(ui_scope, initial_x, initial_y);


	struct LibraryMain
	{
		LibraryMain(const ScopedTaskScope& scope, const double(&initial_x)[128], const int(&initial_y)[128])
			: x(Bn3Tag("main_x"), scope, initial_x, 128), y(Bn3Tag("main_y"), scope, initial_y, 128) {}

		AsyncPropertyArray<double, 128> x;
		AsyncPropertyArray<int, 128> y;
	} main(main_scope, initial_x, initial_y);

	struct LibraryDevice
	{
		LibraryDevice(const ScopedTaskScope& scope, const double(&initial_x)[128], const int(&initial_y)[128])
			: x(Bn3Tag("main_x"), scope, initial_x, 128), y(Bn3Tag("main_y"), scope, initial_y, 128) {}

		AsyncPropertyArray<double, 128> x;
		AsyncPropertyArray<int, 128> y;
	} device(device_scope, initial_x, initial_y);

	struct LibraryIP
	{
		LibraryIP(const ScopedTaskScope& scope, const double(&initial_x)[128], const int(&initial_y)[128])
			: x(Bn3Tag("main_x"), scope, initial_x, 128), y(Bn3Tag("main_y"), scope, initial_y, 128) {}

		AsyncPropertyArray<double, 128> x;
		AsyncPropertyArray<int, 128> y;
	} ip(ip_scope, initial_x, initial_y);


	main.x.registerOnPropertyNotified(device_scope, [&](const double* value, size_t start, size_t end) {
		std::stringstream ss;
		ss << "x : Main -> Device (start : " << start << " end : " << end << ")\n";
		for (size_t i = 0; i < end - start; i++)
		{
			ss << value[i] << " ";
		}
		ss << "\n";
		say(ss.str().c_str());

		bool ret = device.x.set(value, start, end);
		if (ret)
		{
			return device.x.notify(start, end);
		}
		return false;
		});

	main.x.registerOnPropertyNotified(ip_scope, [&](const double* value, size_t start, size_t end) {
		std::stringstream ss;
		ss << "x : Main -> IP (start : " << start << " end : " << end << ")\n";
		for (size_t i = 0; i < end- start; i++)
		{
			ss << value[i] << " ";
		}
		ss << "\n";
		say(ss.str().c_str());

		bool ret = ip.x.set(value, start, end);
		if (ret)
		{
			return ip.x.notify(start, end);
		}
		return false;
		});

	main.x.registerOnPropertyUpdated(ui_scope, [&](const double* value, size_t start, size_t end, bool success)
		{
			std::stringstream ss;
			ss << "x : Main -> UI (start : " << start << " end : " << end << ")\n";
			for (size_t i = 0; i < end - start; i++)
			{
				ss << value[i] << " ";
			}
			ss << "\n";
			say(ss.str().c_str());

			if (success)
			{
				ui.x.set(value, start, end);
				ui.x.update(start, end, true);
			}
			else
			{
				ui.x.update(start, end, false);
			}
		});

	main.y.registerOnPropertyNotified(device_scope, [&](const int* value, size_t start, size_t end) {
		std::stringstream ss;
		ss << "y : Main -> Device (start : " << start << " end : " << end << ")\n";
		for (size_t i = 0; i < end - start; i++)
		{
			ss << value[i] << " ";
		}
		ss << "\n";
		say(ss.str().c_str());

		bool ret = device.y.set(value, start, end);
		if (ret)
		{
			return device.y.notify(start, end);
		}
		return false;
		});

	main.y.registerOnPropertyNotified(ip_scope, [&](const int* value, size_t start, size_t end) {
		std::stringstream ss;
		ss << "y : Main -> IP (start : " << start << " end : " << end << ")\n";
		for (size_t i = 0; i < end - start; i++)
		{
			ss << value[i] << " ";
		}
		ss << "\n";
		say(ss.str().c_str());

		bool ret = ip.y.set(value, start, end);
		if (ret)
		{
			return ip.y.notify(start, end);
		}
		return false;
		});

	main.y.registerOnPropertyUpdated(ui_scope, [&](const int* value, size_t start, size_t end, bool success)
		{
			std::stringstream ss;
			ss << "y : Main -> UI (start : " << start << " end : " << end << ")\n";
			for (size_t i = 0; i < end - start; i++)
			{
				ss << value[i] << " ";
			}
			ss << "\n";
			say(ss.str().c_str());

			if (success)
			{
				ui.y.set(value, start, end);
				ui.y.update(start, end, true);
			}
			else
			{
				ui.y.update(start, end, false);
			}
		});


	ui.x.registerOnPropertyNotified(main_scope, [&](const double* value, size_t start, size_t end) {
		
		std::stringstream ss;
		ss << "x : UI -> Main (start : " << start << " end : " << end << ")\n";
		for (size_t i = 0; i < end - start; i++)
		{
			ss << value[i] << " ";
		}
		ss << "\n";
		say(ss.str().c_str());

		bool ret = main.x.set(value, start, end);
		if (ret)
		{
			return main.x.notify(start, end);
		}
		return false;
		});

	ui.x.registerOnPropertyUpdated(ui_scope, [&](const double* value, size_t start, size_t end, bool success) {
		std::stringstream ss;
		ss << "x : UI -> Application (start : " << start << " end : " << end << ")\n";
		for (size_t i = 0; i < end - start; i++)
		{
			ss << value[i] << " ";
		}
		ss << "\n";
		say(ss.str().c_str());

		application.refreshX(value, start, end);
		});

	ui.y.registerOnPropertyNotified(main_scope, [&](const int* value, size_t start, size_t end) {
		std::stringstream ss;
		ss << "y : UI -> Main (start : " << start << " end : " << end << ")\n";
		for (size_t i = 0; i < end - start; i++)
		{
			ss << value[i] << " ";
		}
		ss << "\n";
		say(ss.str().c_str());

		bool ret = main.y.set(value, start, end);
		if (ret)
		{
			return main.y.notify(start, end);
		}
		return false;
		});

	ui.y.registerOnPropertyUpdated(ui_scope, [&](const int* value, size_t start, size_t end, bool success) {
		std::stringstream ss;
		ss << "y : UI -> Application (start : " << start << " end : " << end << ")\n";
		for (size_t i = 0; i < end - start; i++)
		{
			ss << value[i] << " ";
		}
		ss << "\n";
		say(ss.str().c_str());

		application.refreshY(value, start, end);
		});

	device.x.registerOnPropertyNotified(device_scope, [&](const double* value, size_t start, size_t end) {
		std::stringstream ss;
		ss << "x : Device -> Logic start : " << start << " end : " << end << ")\n";
		for (size_t i = 0; i < end - start; i++)
		{
			ss << value[i] << " ";
		}
		ss << "\n";
		say(ss.str().c_str());

		return true;
		});

	device.y.registerOnPropertyNotified(device_scope, [&](const int* value, size_t start, size_t end) {
		std::stringstream ss;
		ss << "y : Device -> Logic (start : " << start << " end : " << end << ")\n";
		for (size_t i = 0; i < end - start; i++)
		{
			ss << value[i] << " ";
		}
		ss << "\n";
		say(ss.str().c_str());

		return true;
		});

	ip.x.registerOnPropertyNotified(device_scope, [&](const double* value, size_t start, size_t end) {
		std::stringstream ss;
		ss << "x : IP -> Logic (start : " << start << " end : " << end << ")\n";
		for (size_t i = 0; i < end - start; i++)
		{
			ss << value[i] << " ";
		}
		ss << "\n";
		say(ss.str().c_str());

		return true;
		});

	ip.y.registerOnPropertyNotified(device_scope, [&](const int* value, size_t start, size_t end) {
		std::stringstream ss;
		ss << "y : IP -> Logic (start : " << start << " end : " << end << ")\n";
		for (size_t i = 0; i < end - start; i++)
		{
			ss << value[i] << " ";
		}
		ss << "\n";
		say(ss.str().c_str());

		return true;
		});

	{
		// 1. Set Synchronously
		{
			double new_x[] = { 100, 100 };
			int new_y[] = {100, 100};

			ui.x.set(new_x, 1, 3);
			ui.y.set(new_y, 1, 3);
			ui.x.notify(1, 3);
			ui.y.notify(1, 3);

			{
				double x[128];
				ui.x.get(x, 1, 3);
				application.refreshX(x, 1, 3);
			}
			{
				int y[128];
				ui.y.get(y, 1, 3);
				application.refreshY(y, 1, 3);
			}
		}

		// 1-2. Set Synchronously
		{
			double new_x[] = { 200, 200, 200 };
			int new_y[] = { 200, 200, 200 };

			ui.x.set(new_x, 3, 6);
			ui.y.set(new_y, 3, 6);
			if (ui.x.notify(3, 6))
				ui.x.update(3, 6, true);
			if (ui.y.notify(3, 6))
				ui.y.update(3, 6, true);

			using namespace std::chrono_literals;
			std::this_thread::sleep_for(1s);
		}

		// 2. Load From Library
		{
			double new_x[] = { 300, 300, 300, 300 };
			int new_y[] = { 300, 300, 300, 300 };

			ui.x.setAsync(new_x, 6, 10);
			ui.y.setAsync(new_y, 6, 10);

			using namespace std::chrono_literals;
			std::this_thread::sleep_for(1s);
		}

		// 3. Set Asychrnously
		{
			double new_x[] = { 400, 400, 400, 400, 400 };
			int new_y[] = { 400, 400, 400, 400, 400 };

			ui.x.setAsync(new_x, 10, 15);
			ui.y.setAsync(new_y, 10, 15);

			using namespace std::chrono_literals;
			std::this_thread::sleep_for(1s);
		}
	}
		
}

void testAsyncProperty(bool value)
{
	if (!value)
		return;

	Bn3Monkey::Bn3MemoryPool::initialize({ 64, 32, 128, 32, 32, 32, 32, 32, 4});
	Bn3Monkey::ScopedTaskRunner().initialize();

	test_asyncpropertycontainer(false);
	test_asyncproperty(false);
	test_asyncpropertyarray(true);

	Bn3Monkey::ScopedTaskRunner().release();
	Bn3Monkey::Bn3MemoryPool::release();
}

