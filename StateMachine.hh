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
#include "StateFactory.hh"
#include "define.hh"

namespace fsm{
  
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
    
    ///Constructor takes no arguments
    StateMachine(); 

    ///Destructor
    virtual ~StateMachine();

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
      return 0;
    }

    ///register a global event handler, can be overridden by state-specific
    template<class T> int RegisterGlobalEventHandler(const event_t& evt,
						     T handler)
    {
      _globalhandlers[evt] = eh::MakeEventHandler(handler); 
      return 0;
    }
    
    ///Remove a previously registered event handler
    int RemoveEventHandler(const event_t& evt, const stateid_t& st=nullstate);
						   
    ///start the machine running
    virtual status_t Start(const stateid_t& initialState);

    ///stop running (no-op for base class)
    virtual status_t Stop(){}

    using objkey_t = std::string;
    
    ///Store an object shared_ptr with longer-than-state duration
    template<class T> void RegisterObject(const objkey_t& key, 
					  const std::shared_ptr<T>& obj);
    
    ///Store an object raw pointer with longer-than-state duration
    template<class T> void RegisterObject(const objkey_t& key, 
					  T* obj);
    
    ///Retrieve a previously registered object; statically unless typecheck=true
    template<class T> 
    const std::shared_ptr<T>& GetObject(const objkey_t& key, 
					bool typecheck=false) const;

    ///Remove reference to a previously registered object
    template<class T> void RemoveObject(const objkey_t& key);
    
  
  protected:
    status_t status;
    std::string status_msg;
    VState* _current_state = nullptr;
    stateid_t _previous_state;
    status_t ProduceError(status_t code, const std::string& message);
  
    std::map<stateid_t, std::unique_ptr<VStateFactory> > _statefactory;
    std::unordered_map<event_t, EventHandler> _globalhandlers;
      
    virtual status_t Transition(stateid_t nextid);

    struct VObjectHolder { virtual ~VObjectHolder(){} };
    template<class T> struct TObjectHolder : public VObjectHolder {
      std::shared_ptr<T> ptr;
      TObjectHolder(std::shared_ptr<T> ob) : ptr(ob) {}
      TObjectHolder(T* t=nullptr) : ptr(t) {}
    };
    
    using objptr = std::unique_ptr<VObjectHolder>;
    using objmap = std::unordered_map<objkey_t, objptr>;
    objmap _stored_objects;
    
  };

}

template<class T> inline 
void fsm::StateMachine::RegisterObject(const fsm::StateMachine::objkey_t& key,
				       const std::shared_ptr<T>& obj)
{
  _stored_objects[key] = objptr(new TObjectHolder<T>(obj));
}
				      
template<class T> inline 
void fsm::StateMachine::RegisterObject(const fsm::StateMachine::objkey_t& key,
				       T* obj)
{
  _stored_objects[key] = objptr(new TObjectHolder<T>(obj));
}
				      
template<class T> inline const std::shared_ptr<T>& 
fsm::StateMachine::GetObject(const fsm::StateMachine::objkey_t& key, 
			     bool typecheck) const
{
  auto it = _stored_objects.find(key);
  if(it == _stored_objects.end()) //couldn't find it
    return nullptr;
  if(typecheck && dynamic_cast<TObjectHolder<T>*>(it->second.get()) == nullptr){
    //object is the wrong type
    return nullptr;
  }
  return static_cast<TObjectHolder<T>*>(it->second.get())->ptr;
}

template<class T> inline 
void fsm::StateMachine::RemoveObject(const fsm::StateMachine::objkey_t& key)
{
  _stored_objects.erase(key);
}
