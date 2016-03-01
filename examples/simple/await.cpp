#include "await.hpp"

#ifdef USE_AWAIT
namespace co {
thread_local task* task::current_task = nullptr;
}
#endif
