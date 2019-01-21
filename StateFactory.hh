#ifndef STATEFACTORY_h
#define STATEFACTORY_h
#include "define.hh"

namespace fsm{
  struct VStateFactory{
    std::string name;
    std::unordered_map<event_t, EventHandler> event_handlers;
    virtual VState* enter(StateMachine* sm) = 0;
    VStateFactory(const std::string& statename) : name(statename) {}
    inline VState* operator()(StateMachine* sm){ return enter(sm); }
  };

  template<class S, bool> struct StateFactory : public VStateFactory{};
  template<class S> struct StateFactory<S,true> : public VStateFactory{
    StateFactory(const std::string& statename) : VStateFactory(statename) {}

    VState* enter(StateMachine* sm)
    { return new S(sm); }
  };

  template<class S> struct StateFactory<S,false> : public VStateFactory{
    StateFactory(const std::string& statename) : VStateFactory(statename) {}

    VState* enter(StateMachine* sm)
    { return new TState<S>(sm); }
  };
};

#endif
