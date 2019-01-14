#include "StateMachine.hh"
#include <iostream>
#include <cassert>

using namespace fsm;

//static initializers
const stateid_t StateMachine::nullstate = GetStateID(nullptr);
const event_t StateMachine::ERROR_DEFAULT = "fsm::StateMachine::ERROR_DEFAULT";

//constructor
StateMachine::StateMachine() : 
  _previous_state(GetStateID(nullptr))
{
  ResetStatus();
  RegisterState<DefaultErrorHandler>("DefaultErrorHandler");
}

status_t StateMachine::Handle(const Message& msg)
{

  status = STATUS_OK;
  if(!_current_state){
    return ProduceError(CURRENT_STATE_UNDEFINED,
			"Current state is undefined");
  }
  
  stateid_t currentid = _current_state->GetID();
  stateid_t nextid = currentid;
  //see if the state has a response for this event
  if(!_statefactory.count(currentid)){
    std::cerr<<"Current state is "<<currentid<<"; known states:\n";
    for(auto& it : _statefactory){
      std::cerr<<it.first<<std::endl;
    }
    std::cerr<<"Is default err handler ? "<<dynamic_cast<DefaultErrorHandler*>(_current_state)<<std::endl;
    abort();
  }
  auto* handlers = &(_statefactory[currentid]->event_handlers);
  auto it = handlers->find(msg.event);
  if(it == handlers->end()){
    //see if there is a global handler registered
    handlers = &_globalhandlers;
    it = handlers->find(msg.event);
  }
  if(it != handlers->end()){
    //we found an appropriate handler
    nextid = (it->second)(_current_state, msg);
  }
  else{
    //TODO: optionally have error if we don't know how to handle this event
  }
  
  return Transition(nextid); 
}

status_t StateMachine::Transition(stateid_t nextid)
{
  stateid_t currentid = GetCurrentStateID();
  if( nextid != currentid && nextid != nullstate ){
    if(!_statefactory.count(nextid)){
      ProduceError(UNKNOWN_STATE_REQUESTED,
		   "Request for transition to unknown state");
      //transition to some error state now
      //TODO: allow user to override
      nextid = GetStateID<DefaultErrorHandler>();
    }
    //make sure the current state's exit gets called first
    delete _current_state;
    //now instantiate the new state
    _current_state = _statefactory[nextid]->enter(this);
  }
  return status;
}

status_t StateMachine::Start(const stateid_t& initialstate)
{
  return Transition(initialstate);
}

status_t StateMachine::ProduceError(status_t code, const std::string& msg)
{
  status = code;
  status_msg = msg;
  return status;
}

StateMachine::DefaultErrorHandler::DefaultErrorHandler(StateMachine* sm) : 
  VState(sm)
{
  std::cerr<<"State machine has encountered error "<<sm->GetStatus()<<":\n"
	   <<sm->GetStatusMsg()<<'\n';
}
