#include <iostream>
#include <list>
#include <mutex>
#include <functional>
#include <cassert>
#include <unordered_map>
#include <memory>
#include <utility>
#include <queue>
#include <unordered_set>

class LuaObject;
using LuaObjectPtr = std::shared_ptr<LuaObject>;
class Task;
using TaskPtr = std::shared_ptr<Task>;
class SchedulerTask;
using SchedulerTaskPtr = std::shared_ptr<SchedulerTask>;
