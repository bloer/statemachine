#ifndef DEFINE_h
#define DEFINE_h

#include <typeindex>
#include <chrono>
#include <string>
#include <ostream>

namespace fsm{
  using status_t = int;
  using event_t = std::string;
  using stateid_t = std::type_index;
  
  using mstick_t = std::chrono::milliseconds::rep;
  inline mstick_t mstick(){
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now()
				       .time_since_epoch()).count();
  }
};

inline std::ostream& operator<<(std::ostream& out, const fsm::stateid_t& sid)
{ return out<<sid.name(); }

#endif
