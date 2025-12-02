
#include "Plugin.hpp"
#include <dlfcn.h>
#include <iostream>
#include <vector>
#include <memory>


class PluginManager
{
public:
	std::vector<IPlugin*> loadedPlugins;
	std::vector<void*> handles;

	void loadPlugin(const std::string& path)
	{
		// 1. 打开动态库文件 (.dylib / .so)
		void* handle = dlopen(path.c_str(), RTLD_LAZY);
		if(!handle)
		{
			std::cerr << "Cannot open library: " << dlerror() << '\n';
		}



		//2. 寻找 "create" 函数
		//reset errors
		dlerror();
		CreatePluginFunc create = (CreatePluginFunc) dlsym(handle, "create");
		const char* dlsym_error = dlerror();
		if(dlsym_error)
		{
			std::cerr << "Cannot load symbol create: " << dlsym_error << '\n';
			dlclose(handle);
			return;
		}

		//3. 调用函数创建插件实例
		IPlugin* plugin = create();
		loadedPlugins.push_back(plugin);
		handles.push_back(handle);

		std::cout << "Loaded plugin: " << plugin->getName() << std::endl;
	}

	~PluginManager()
	{
		for(auto p : loadedPlugins) delete p;
		for(auto h : handles) dlclose(h);
	}
};




























