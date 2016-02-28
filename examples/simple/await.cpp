#include "await.hpp"

namespace co {
thread_local task* task::current_task = nullptr;
}