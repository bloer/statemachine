#ifndef STATE_h
#define STATE_h

#include "define.hh"
#include <iostream>
#include <type_traits>
#include <memory>

namespace fsm{
  /**** functions to get IDs of states **/

  ///Get the ID of a state class
  template<class T> inline stateid_t GetStateID()  
  { return typeid(typename std::remove_pointer<T>::type); }

  template<class T> inline stateid_t GetStateID(T&&)
  { return GetStateID<T>(); }

  template<class T> inline stateid_t GetStateID(std::shared_ptr<T>&&)
  { return GetStateID<T>(); }
  
  template<class T> inline stateid_t GetStateID(std::unique_ptr<T>&&)
  { return GetStateID<T>(); }

  template<class T> inline stateid_t GetStateID(T* t)
  { return t ? typeid(*t) : typeid(std::nullptr_t); }
  
  //forward declaration
  class StateMachine;
  
  
  ///Abstract state class, used to store in stl containers
  class VState{
  protected:
    StateMachine* sm;
    mstick_t time_entered;
  public:
    ///Constructor, needs pointer to finite state machine
    VState(StateMachine* fsm = nullptr) : sm(fsm), time_entered(mstick()) {}

    ///Need virtual destructor to make class polymorphic
    virtual ~VState() {}

    ///Get associated state machine
    StateMachine* GetStateMachine() const { return sm; }

    ///Get the time state was entered in ms since UNIX epoch
    mstick_t GetTimeEntered() const { return time_entered; }
    
    ///Get the id of the previous state we were in
    stateid_t GetPreviousStateID() const;

    virtual stateid_t GetID(){ return GetStateID(this); }
  };
  
  ///semi-concrete templated state, wraps any object
  template<class T> class TState : public VState{
    T _stateobj;
  public:
    ///constructor, takes state machine
    TState(StateMachine* sm = nullptr) : VState(sm) {}
    
    ///Get access to the wrapped object doing the actual work
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
