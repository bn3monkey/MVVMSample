#include <MemoryPool/MemoryPool.hpp>
#include <ScopedTask/ScopedTask.hpp>
#include <AsyncProperty/AsyncProperty.hpp>

#include "test_helper.hpp"
#include "../test_helper.hpp"




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

		void refreshX(const double* values, size_t offset, size_t length)
		{
			
			std::copy(values, values + length, x + offset);

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
		void refreshY(const int* values, size_t offset, size_t length)
		{

			std::copy(values, values + length, y + offset);

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


	main.x.registerOnPropertyNotified(device_scope, [&](const double* value, size_t offset, size_t length) {
		std::stringstream ss;
		ss << "x : Main -> Device (offset : " << offset << " length : " << length << ")\n";
		for (size_t i = 0; i < length; i++)
		{
			ss << value[i] << " ";
		}
		ss << "\n";
		say(ss.str().c_str());

		bool ret = device.x.set(value, offset, length);
		if (ret)
		{
			return device.x.notify(offset, length);
		}
		return false;
		});

	main.x.registerOnPropertyNotified(ip_scope, [&](const double* value, size_t offset, size_t length) {
		std::stringstream ss;
		ss << "x : Main -> IP (offset : " << offset << " length : " << length << ")\n";
		for (size_t i = 0; i < length; i++)
		{
			ss << value[i] << " ";
		}
		ss << "\n";
		say(ss.str().c_str());

		bool ret = ip.x.set(value, offset, length);
		if (ret)
		{
			return ip.x.notify(offset, length);
		}
		return false;
		});

	main.x.registerOnPropertyUpdated(ui_scope, [&](const double* value, size_t offset, size_t length, bool success)
		{
			std::stringstream ss;
			ss << "x : Main -> UI (offset : " << offset << " length : " << length << ")\n";
			for (size_t i = 0; i < length; i++)
			{
				ss << value[i] << " ";
			}
			ss << "\n";
			say(ss.str().c_str());

			if (success)
			{
				ui.x.set(value, offset, length);
				ui.x.update(offset, length, true);
			}
			else
			{
				ui.x.update(offset, length, false);
			}
		});

	main.y.registerOnPropertyNotified(device_scope, [&](const int* value, size_t offset, size_t length) {
		std::stringstream ss;
		ss << "y : Main -> Device (offset : " << offset << " length : " << length << ")\n";
		for (size_t i = 0; i < length; i++)
		{
			ss << value[i] << " ";
		}
		ss << "\n";
		say(ss.str().c_str());

		bool ret = device.y.set(value, offset, length);
		if (ret)
		{
			return device.y.notify(offset, length);
		}
		return false;
		});

	main.y.registerOnPropertyNotified(ip_scope, [&](const int* value, size_t offset, size_t length) {
		std::stringstream ss;
		ss << "y : Main -> IP (offset : " << offset << " length : " << length << ")\n";
		for (size_t i = 0; i < length; i++)
		{
			ss << value[i] << " ";
		}
		ss << "\n";
		say(ss.str().c_str());

		bool ret = ip.y.set(value, offset, length);
		if (ret)
		{
			return ip.y.notify(offset, length);
		}
		return false;
		});

	main.y.registerOnPropertyUpdated(ui_scope, [&](const int* value, size_t offset, size_t length, bool success)
		{
			std::stringstream ss;
			ss << "y : Main -> UI (offset : " << offset << " length : " << length << ")\n";
			for (size_t i = 0; i < length; i++)
			{
				ss << value[i] << " ";
			}
			ss << "\n";
			say(ss.str().c_str());

			if (success)
			{
				ui.y.set(value, offset, length);
				ui.y.update(offset, length, true);
			}
			else
			{
				ui.y.update(offset, length, false);
			}
		});


	ui.x.registerOnPropertyNotified(main_scope, [&](const double* value, size_t offset, size_t length) {
		
		std::stringstream ss;
		ss << "x : UI -> Main (offset : " << offset << " length : " << length << ")\n";
		for (size_t i = 0; i < length; i++)
		{
			ss << value[i] << " ";
		}
		ss << "\n";
		say(ss.str().c_str());

		bool ret = main.x.set(value, offset, length);
		if (ret)
		{
			return main.x.notify(offset, length);
		}
		return false;
		});

	ui.x.registerOnPropertyUpdated(ui_scope, [&](const double* value, size_t offset, size_t length, bool success) {
		std::stringstream ss;
		ss << "x : UI -> Application (offset : " << offset << " length : " << length << ")\n";
		for (size_t i = 0; i < length; i++)
		{
			ss << value[i] << " ";
		}
		ss << "\n";
		say(ss.str().c_str());

		application.refreshX(value, offset, length);
		});

	ui.y.registerOnPropertyNotified(main_scope, [&](const int* value, size_t offset, size_t length) {
		std::stringstream ss;
		ss << "y : UI -> Main (offset : " << offset << " length : " << length << ")\n";
		for (size_t i = 0; i < length; i++)
		{
			ss << value[i] << " ";
		}
		ss << "\n";
		say(ss.str().c_str());

		bool ret = main.y.set(value, offset, length);
		if (ret)
		{
			return main.y.notify(offset, length);
		}
		return false;
		});

	ui.y.registerOnPropertyUpdated(ui_scope, [&](const int* value, size_t offset, size_t length, bool success) {
		std::stringstream ss;
		ss << "y : UI -> Application (offset : " << offset << " length : " << length << ")\n";
		for (size_t i = 0; i < length; i++)
		{
			ss << value[i] << " ";
		}
		ss << "\n";
		say(ss.str().c_str());

		application.refreshY(value, offset, length);
		});

	device.x.registerOnPropertyNotified(device_scope, [&](const double* value, size_t offset, size_t length) {
		std::stringstream ss;
		ss << "x : Device -> Logic (offset : " << offset << " length : " << length << ")\n";
		for (size_t i = 0; i < length; i++)
		{
			ss << value[i] << " ";
		}
		ss << "\n";
		say(ss.str().c_str());

		return true;
		});

	device.y.registerOnPropertyNotified(device_scope, [&](const int* value, size_t offset, size_t length) {
		std::stringstream ss;
		ss << "y : Device -> Logic (offset : " << offset << " length : " << length << ")\n";
		for (size_t i = 0; i < length; i++)
		{
			ss << value[i] << " ";
		}
		ss << "\n";
		say(ss.str().c_str());

		return true;
		});

	ip.x.registerOnPropertyNotified(device_scope, [&](const double* value, size_t offset, size_t length) {
		std::stringstream ss;
		ss << "x : IP -> Logic (offset : " << offset << " length : " << length << ")\n";
		for (size_t i = 0; i < length; i++)
		{
			ss << value[i] << " ";
		}
		ss << "\n";
		say(ss.str().c_str());

		return true;
		});

	ip.y.registerOnPropertyNotified(device_scope, [&](const int* value, size_t offset, size_t length) {
		std::stringstream ss;
		ss << "y : IP -> Logic (offset : " << offset << " length : " << length << ")\n";
		for (size_t i = 0; i < length; i++)
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

			ui.x.set(new_x, 1, 2);
			ui.y.set(new_y, 1, 2);
			ui.x.notify(1, 2);
			ui.y.notify(1, 2);

			{
				double x[128];
				ui.x.get(x, 1, 2);
				application.refreshX(x, 1, 2);
			}
			{
				int y[128];
				ui.y.get(y, 1, 2);
				application.refreshY(y, 1, 2);
			}
		}

		// 1-2. Set Synchronously
		{
			double new_x[] = { 200, 200, 200 };
			int new_y[] = { 200, 200, 200 };

			ui.x.set(new_x, 3, 3);
			ui.y.set(new_y, 3, 3);
			if (ui.x.notify(3, 3))
				ui.x.update(3, 3, true);
			if (ui.y.notify(3, 3))
				ui.y.update(3, 3, true);

			using namespace std::chrono_literals;
			std::this_thread::sleep_for(1s);
		}

		// 2. Load From Library
		{
			double new_x[] = { 300, 300, 300, 300 };
			int new_y[] = { 300, 300, 300, 300 };

			ui.x.setAsync(new_x, 6, 4);
			ui.y.setAsync(new_y, 6, 4);

			using namespace std::chrono_literals;
			std::this_thread::sleep_for(1s);
		}

		// 3. Set Asychrnously
		{
			double new_x[] = { 400, 400, 400, 400, 400 };
			int new_y[] = { 400, 400, 400, 400, 400 };

			ui.x.setAsync(new_x, 10, 5);
			ui.y.setAsync(new_y, 10, 5);

			using namespace std::chrono_literals;
			std::this_thread::sleep_for(1s);
		}
	}

	{
		// 1. Set Synchronously
		{
			ui.x.set(-10.0, 0);
			ui.y.set(-10, 0);
			ui.x.notify(0);
			ui.y.notify(0);

			{
				double x[128];
				x[0] = ui.x.get(0);

				application.refreshX(x, 0, 1);
			}
			{
				int y[128];
				y[0] = ui.y.get(0);
				application.refreshY(y, 0, 1);
			}
		}

		// 1-2. Set Synchronously
		{
			ui.x.set(-20.0, 1);
			ui.y.set(-20, 1);

			if (ui.x.notify(1))
				ui.x.update(1, true);
			if (ui.y.notify(1))
				ui.y.update(1, true);

			using namespace std::chrono_literals;
			std::this_thread::sleep_for(1s);
		}

		// 2. Load From Library
		{
			ui.x.setAsync(-30.0, 2);
			ui.y.setAsync(-30, 2);

			using namespace std::chrono_literals;
			std::this_thread::sleep_for(1s);
		}

		// 3. Set Asychrnously
		{
			ui.x.setAsync(-40.0, 3);
			ui.y.setAsync(-40, 3);

			using namespace std::chrono_literals;
			std::this_thread::sleep_for(1s);
		}
	}
	
}

void testAsyncProperty(bool value)
{
	if (!value)
		return;

	Bn3Monkey::Bn3MemoryPool::initialize({ 64, 32, 128, 32, 32, 32, 32, 32 });
	Bn3Monkey::ScopedTaskRunner().initialize();

	test_asyncproperty(true);
	test_asyncpropertyarray(true);

	Bn3Monkey::ScopedTaskRunner().release();
	Bn3Monkey::Bn3MemoryPool::release();
}

