
#pragma once

#include <functional>

namespace psi::thread {

using CrashNotifyFn = std::function<void(const char *err, const char *st)>;
extern thread_local CrashNotifyFn *t_crash_notifier;

} // namespace psi::thread
