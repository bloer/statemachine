#ifndef STATE_h
#define STATE_h

#include "define.hh"

namespace fsm{
  class StateMachine;
  /**** functions to get IDs of states **/

  ///Get the ID of a state class
  template<class T> inline stateid_t GetStateID(const T&) { return typeid(T); }
  template<class T> inline stateid_t GetStateID(const T*) { return typeid(T); }
  template<class T> inline stateid_t GetStateID()         { return typeid(T); }


  class VState{
  protected:
    StateMachine* sm;
    mstick_t time_entered;
  public:
    VState(StateMachine* fsm = nullptr) : sm(fsm), time_entered(mstick()) {}
    StateMachine* GetStateMachine() const { return sm; }
    mstick_t GetTimeEntered() const { return time_entered; }
    stateid_t GetPreviousStateID() const;

    virtual stateid_t GetID(){ return GetStateID(this); }
  };
  
  template<class T> class TState : public VState{
    T _stateobj;
  public:
    TState(StateMachine* sm) : VState(sm) {}
    T& GetStateObj() { return _stateobj; }

    virtual stateid_t GetID(){ return GetStateID<T>(); }
  };



  
  
  /* these don't seem to work
  //nullptr id is unique
  template<> inline stateid_t GetStateID<std::nullptr_t>()
  { return typeid(nullptr); }
  template<> inline stateid_t GetStateID<std::nullptr_t>(std::nullptr_t)
  { return typeid(nullptr); }

  //TState's should default to the base
  template<class T> inline stateid_t
  GetStateID<TState<T>>() { return typeid(T); }
  template<class T> inline stateid_t 
  GetStateID(const TState<T>*)  {return typeid(T);}
  template<class T> inline stateid_t 
  GetStateID(const TState<T>&)  {return typeid(T);}  
  */
}

#endif
