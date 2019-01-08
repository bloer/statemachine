#ifndef DEFINE_h
#define DEFINE_h

#include <typeindex>
#include <chrono>

namespace fsm{
  using event_t = int;
  using stateid_t = std::type_index;
  
  using mstick_t = std::chrono::milliseconds::rep;
  inline mstick_t mstick(){
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now()
				       .time_since_epoch()).count();
  }
};

#endif
