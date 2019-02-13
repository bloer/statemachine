#include <vector>
#include <string>
#include <map>
#include <queue>
#include <unordered_map>
#include <typeinfo>
#include <typeindex>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "Message.hh"
#include "EventHandler.hh"
#include "State.hh"
#include "StateFactory.hh"
#include "define.hh"

namespace fsm{
  
  class StateMachine{
  public:
    static const event_t ERROR_DEFAULT;
    static const event_t TIMEOUT;
    static const event_t STOP;
  
  public:
    enum STATUSCODE {
      STATUS_OK = 0,
      CURRENT_STATE_UNDEFINED = -1,
      UNKNOWN_STATE_REQUESTED = -2,
    };

    enum SEQUENCE {
      SEQ_FIRST    = 0,
      SEQ_DEFAULT  = 50,
      SEQ_LAST     = 100,
      SEQ_OVERRIDE = -1,
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
    const VState* GetCurrentState() const { return _current_state.get(); }
    
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
    virtual status_t Handle(Message&& msg);

    ///explicitly handle a bare event
    status_t Handle(const event_t& event){ return Handle(Message(event)); }
    
    ///don't process any more handlers for this event
    void StopProcessingCurrentEvent(){ _stop_processing = true; }
  
    ///register a state to handle events
    template<class T> void RegisterState(std::string name="",
					 bool override=false){
      static const bool isvstate = std::is_base_of<VState, T>::value;
      if(override || _statefactory.count(GetStateID<T>()) == 0){
	if(name.empty()) 
	  name = GetStateID<T>().name();
	_statefactory[GetStateID<T>()] = 
	  std::unique_ptr<VStateFactory>(new StateFactory<T,isvstate>(name));
      }
    }  

    /** Register a callback function when an event is received.
	If `state` is given, it only fires if the state machine is in that state
	@param evt      The event type to handle
	@param handler  The function callback. Can be any of several types;
	                see EventHandlers.hh for call signatures
	@param sequence If multiple callbacks are registered for the same event,
	                sequence determines the order in which they fire. 
			Ordinarily in the range 0-100, lower numbers fire first.
			Order for multiple callbacks with the same sequence is 
			undefined.  Special: if any callback has a sequence 
			<0, it is considered an override, and no other callbacks
			will fire. 
	@param state    Optional ID of the state to associate callback to. If
	                `nullstate` (default), will fire in any state
	---
	@returns an integer with 0 indicating success
    */	
    template<class Handler> 
    int RegisterEventHandler(const event_t& evt, Handler handler,
			     int sequence=SEQ_DEFAULT,
			     const stateid_t& state=nullstate)
    {
      _eventhandlers[evt].insert({sequence, statehandler{state, 
	      eh::MakeEventHandler(handler)} });
      return 0;
    }

    ///Alternate signature to register handler, giving state as template param
    template<class State, class Handler> 
    int RegisterEventHandler(const event_t& evt, Handler handler, 
			     int sequence=SEQ_DEFAULT)
    {
      //allow silently registering the state too
      RegisterState<State>();
      return RegisterEventHandler(evt, handler, sequence, GetStateID<State>());
      
    }
    
    ///Remove a previously registered event handler
    int RemoveEventHandler(const event_t& evt, int sequence,
			   const stateid_t& st=nullstate);
    
    ///Remove all event handlers for the given event, or all totally
    int RemoveAllHandlers(const event_t& evt="");
						   
    ///Set the main loop timeout time
    void SetTimeout(long mstimeout) { _mstimeout = mstimeout; }
    
    ///Get the main loop timeout interval
    long GetTimeout() const { return _mstimeout; }

    ///start the machine running
    virtual status_t Start(const stateid_t& initialState, long mstimeout=-1,
			   bool block=false);

    ///stop running (no-op for base class). wait for thread to join if wait=true
    virtual status_t Stop(bool wait=false);
  
    using objkey_t = std::string;
    
    ///Store an object with longer-than-state duration
    template<class T> void RegisterObject(const objkey_t& key, 
					  const T& obj);

    ///Store an object, move semantics
    template<class T> void RegisterObject(const objkey_t& key,
					  T&& obj);

    ///Special override to treat const char* as std::string
    void RegisterObject(const objkey_t& key, const char* obj)
    { RegisterObject(key, std::string(obj)); }
    
    ///Retrieve a previously registered object; statically unless typecheck=true
    template<class T> T& GetObject(const objkey_t& key, bool typecheck=false);
    template<class T> const T& GetObject(const objkey_t& key, 
					 bool typecheck=false) const;

    ///Remove reference to a previously registered object
    template<class T> void RemoveObject(const objkey_t& key);
    
  
  protected:
    status_t status;
    std::string status_msg;
    long _mstimeout = 100;
    bool _stop_processing = false;
    std::unique_ptr<VState> _current_state = nullptr;
    stateid_t _previous_state;
    status_t ProduceError(status_t code, const std::string& message);
 
    std::map<stateid_t, std::unique_ptr<VStateFactory> > _statefactory;
    struct statehandler{stateid_t state; EventHandler handler;};
    using evhsequence = std::multimap<int, statehandler>;
    std::unordered_map<event_t, evhsequence> _eventhandlers;
      
    virtual status_t ProcessMessage(const Message& msg);
    virtual status_t Transition(stateid_t nextid, bool checkfirst=false);
    virtual status_t MainLoop();

    struct VObjectHolder { virtual ~VObjectHolder(){} };
    template<class T> struct TObjectHolder : public VObjectHolder {
      T obj;
      TObjectHolder(const T& t) : obj(t) {}
      TObjectHolder(T&& t) : obj(t) {} //move constructor
    };
    
    using objptr = std::unique_ptr<VObjectHolder>;
    using objmap = std::unordered_map<objkey_t, objptr>;
    objmap _stored_objects;
    
    std::queue<Message> _msgq;
    std::mutex _msgq_mutex;
    std::condition_variable _msg_avail;
    std::thread _main_loop;
  };

}

template<class T> inline 
void fsm::StateMachine::RegisterObject(const fsm::StateMachine::objkey_t& key,
				       const T& obj)
{
  _stored_objects[key] = objptr(new TObjectHolder<T>(obj));
}
				      
template<class T> inline 
void fsm::StateMachine::RegisterObject(const fsm::StateMachine::objkey_t& key,
				       T&& obj)
{
  _stored_objects[key] = objptr(new TObjectHolder<T>(obj));
}

template<class T> inline 
const T& fsm::StateMachine::GetObject(const fsm::StateMachine::objkey_t& key, 
				      bool typecheck) const
{
  auto it = _stored_objects.find(key);
  if(it == _stored_objects.end()){//couldn't find it
    std::stringstream err;
    err<<"No object registered with key "<<key;
    throw std::invalid_argument(err.str());
  }
  if(typecheck && !dynamic_cast<const TObjectHolder<T>*>(it->second.get())){
    //object is the wrong type
    std::stringstream err;
    err<<"Object registered with key "<<key<<" is not of requested type "
       <<typeid(T).name();
    throw std::invalid_argument(err.str());
  }
  return static_cast<const TObjectHolder<T>*>(it->second.get())->obj;
}

template<class T> inline 
T& fsm::StateMachine::GetObject(const fsm::StateMachine::objkey_t& key, 
			       bool typecheck)
{
  //call the const version
  const StateMachine* me = this;
  return const_cast<T&>(me->GetObject<T>(key, typecheck));
}

template<class T> inline 
void fsm::StateMachine::RemoveObject(const fsm::StateMachine::objkey_t& key)
{
  _stored_objects.erase(key);
}
