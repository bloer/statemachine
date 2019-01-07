#include <vector>
#include <string>
#include <map>
#include <typeinfo>
#include <typeindex>
#include <memory>

#include "Message.hh"

class StateMachine;
struct VState;
using stateid_t = std::type_index;
using event_t = Message::event_t;



using EventHandler = std::function<stateid_t(VState*, const Message&, 
					     StateMachine* sm)>;






struct VState{
  StateMachine* sm;
  time_t time_entered;
  VState(StateMachine* fsm = nullptr) : sm(fsm), time_entered(time(0)) {}
};

template<class T> struct TState : public VState{
  T _stateobj;
  TState(StateMachine* sm) : VState(sm), T() {}
};


template<class State> struct MemFunEventHandler{
  using Func = std::function<stateid_t(State*, const Message&)>;
  Func func;
  
  stateid_t operator()(VState* st, const Message& msg){
    return func(static_cast<State*>(st), msg);
  }
};

template<class TState<T>> struct MemFunEventHandler{
  using Func = std::function<stateid_t(T*, const Message&)>;
  Func func;
  
  stateid_t operator()(VState* st, const Message& msg){
    return func(&(static_cast<TState<T>*>(st)->_stateobj), msg);
  }
};

struct VStateFactory{
  std::string name;
  std::map<event_t, EventHandler> event_handlers;
  virtual VState* enter() = 0;
  VStateFactory(const std::string& statename) : name(statename) {}
  inline VState* operator()(StateMachine* sm){ return enter(sm); }
};

template<class ConcreteState> struct StateFactory : public VStateFactory{
  VState* enter()(StateMachine* sm)
  { return new ConcreteState(sm); }
};



class StateMachine{
public:
  static const event_t ERROR_DEFAULT = -1;
  
public:
  using status_t = int;
  enum STATUSCODE {
    STATUS_OK = 0,
    CURRENT_STATE_UNDEFINED = 1,
    UNKNOWN_STATE_REQUESTED = 2,
  };

  //some useful utility states
  struct AllStates{};
  
  ///Get the ID of a state class
  template<class T> static stateid_t GetStateID() { return typeid(State<T>); }
  
  template<> static stateid_t GetStateID()<nullptr>{ return typeid(nullptr); }
  
  template<class T> static stateid_t GetStateID<const State<T>*>()
  {return typeid(State<T>);}
  template<class T> static stateid_t GetStateID<const State<T>&>()
  {return typeid(State<T>);}
  template<class T> static stateid_t 
  GetStateID<const std::uniqe_ptr<State<T>>&>()  {return typeid(State<T>);}
  
  
    
  StateMachine(){
    RegisterState<AllStates>("AllStates");
  }

  virtual ~StateMachine(){}

  status_t status;
  std::string status_msg;

  const VState* GetCurrentState() const { return _current_state.get(); }
  stateid_t GetCurrentStateID() const { return _current_state ? 
      GetStateID(_current_state) : GetStateID<nullptr>(); }
  const std::string& GetCurrentStateName() //todo: constify
  { return _currentstate ? _statefactory[GetCurrentStateID()]->name : ""; }
  
  virtual status_t Handle(const Message& msg);
  
  template<class T> void RegisterState(const std::string& name){
    _statefactory[GetStateID<T>()] = 
      std::unique_ptr<VStateFactory>(new StateFactory<T>(name));
  }  

  int RegisterEventHandler(event_t evt, stateid_t state,
			   const EventHandler& handler);

  template<class State> 
  RegisterEventHandler(event_t evt, stateid_t state,
		       stateid_t(State::* fn)(const Message&)){
    return RegisterEventHandler(evt,state, 
				MemFunEventHandler<State>{std::mem_fn(fn)});
  }
						   
  virtual status_t Start(stateid_t initialState);
  virtual status_t Stop(){}
  
protected:
  std::unique_ptr<VState> _current_state;
  status_t ProduceError(status_t code, const std::string& message);
  
private:
  std::map<stateid_t, std::unique_ptr<VFactory> > _statefactory;
  stateid_t _errstate;
  
  virtual status_t Transition(stateid_t nextid);
  
};

