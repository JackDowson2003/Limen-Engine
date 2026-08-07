#pragma once

#include <algorithm>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#if defined(LIMEN_PLATFORM_WINDOWS)
    #include <Windows.h>
#elif defined(LIMEN_PLATFORM_MACOS) || defined(LIMEN_PLATFORM_LINUX)
    #include <fcntl.h>
    #include <pthread.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <unistd.h>
    #include <chrono>
    #include <thread>
#endif
