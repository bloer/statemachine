#ifndef STATE_h
#define STATE_h

#include "define.hh"

namespace fsm{
  class StateMachine;
  
  class VState{
    StateMachine* sm;
    mstick_t time_entered;
  public:
    VState(StateMachine* fsm = nullptr) : sm(fsm), time_entered(mstick()) {}
    StateMachine* GetStateMachine() const { return sm; }
    mstick_t GetTimeEntered() const { return time_entered; }
    stateid_t GetPreviousStateID() const;
  };
  
  template<class T> class TState : public VState{
    T _stateobj;
  public:
    TState(StateMachine* sm) : VState(sm), T() {}
    T& GetStateObj() { return _stateobj; }
  };
}

#endif
