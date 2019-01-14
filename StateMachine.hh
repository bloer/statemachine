#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <typeinfo>
#include <typeindex>
#include <memory>

#include "Message.hh"
#include "EventHandler.hh"
#include "State.hh"
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

    
  class StateMachine{
  public:
    static const event_t ERROR_DEFAULT;
  
  public:
    enum STATUSCODE {
      STATUS_OK = 0,
      CURRENT_STATE_UNDEFINED = -1,
      UNKNOWN_STATE_REQUESTED = -2,
    };

    //some useful utility states
    class DefaultErrorHandler : public VState{
    public:
      DefaultErrorHandler(StateMachine* sm);
    };
    
    static const stateid_t nullstate;
  
    ///Constructor takes no arguments
    StateMachine(); 

    ///Destructor does nothing
    virtual ~StateMachine(){}

    ///Get the current status code for the machine
    status_t GetStatus() const { return status; }

    ///Get the message relating to the current status
    std::string GetStatusMsg() const { return status_msg; }

    ///Reset any errors
    void ResetStatus() { status = STATUS_OK; status_msg=""; }
    
    ///Get the current state
    const VState* GetCurrentState() const { return _current_state; }
    
    ///Get the ID of the current state
    stateid_t GetCurrentStateID() const 
    { return _current_state ? _current_state->GetID() : nullstate; }
    
    ///Get the name of the current state
    std::string GetCurrentStateName() //todo: constify
    { return _current_state ? _statefactory[GetCurrentStateID()]->name : ""; }

    ///Get the previous state ID
    stateid_t GetPreviousStateID() const { return _previous_state; }
    
    ///Get thje previous state name
    std::string GetPreviousStateName() //todo: constify
    { return _statefactory.count(_previous_state) ? 
	_statefactory[_previous_state]->name : "" ; 
    }
  
    ///handle an incoming message (event)
    virtual status_t Handle(const Message& msg);

    ///explicitly handle a bare event
    status_t Handle(const event_t& event){ return Handle(Message(event)); }
  
    ///register a state to handle events
    template<class T> void RegisterState(const std::string& name){
      static const bool isvstate = std::is_base_of<VState, T>::value;
      _statefactory[GetStateID<T>()] = 
	std::unique_ptr<VStateFactory>(new StateFactory<T,isvstate>(name));
    }  

    ///register an event callback for a given state. can be mem function
    template<class T> int RegisterEventHandler(const event_t& evt, 
					       const stateid_t& state,
					       T handler)
    {
      _statefactory[state]->event_handlers[evt] = eh::MakeEventHandler(handler);
    }

    ///register a global event handler, can be overridden by state-specific
    template<class T> int RegisterGlobalEventHandler(const event_t& evt,
						     T handler)
    {
      _globalhandlers[evt] = eh::MakeEventHandler(handler); 
    }
    
						   
    virtual status_t Start(const stateid_t& initialState);
    virtual status_t Stop(){}
  
  protected:
    status_t status;
    std::string status_msg;
    VState* _current_state = nullptr;
    stateid_t _previous_state;
    status_t ProduceError(status_t code, const std::string& message);
  
    std::map<stateid_t, std::unique_ptr<VStateFactory> > _statefactory;
    std::unordered_map<event_t, EventHandler> _globalhandlers;
      
    virtual status_t Transition(stateid_t nextid);
  
  };

}
