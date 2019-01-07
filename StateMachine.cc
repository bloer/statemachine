#include "StateMachine.hh"

stateid_t StateMachine::Handle(const Message& msg)
{

  status = STATUS_OK;
  if(!_current_state){
    return ProduceError(CURRENT_STATE_UNDEFINED,
			"Current state is undefined");
  }
  
  stateid_t currentid = _current_state->GetID();
  stateid_t nextid;
  //see if the state has an erorr handler
  auto& handlers = _statefactory[currentid]->event_handlers;
  auto it = handlers.find(msg.event);
  if(it != handlers.end()){
    nextid = *(it->second)(_current_state.get(), msg);
  }
  else{
    //check for a global event handler
    handlers = _statefactory[GetStateID<AllStates>()]->event_handlers;
    it = handlers.find(msg.event);
    if(it != handlers.end())
      nextid = *(it->second)(_current_state.get(), msg);
  }

  return Transition(nextid); 
}

stateid_t StateMachine::Transition(stateid_t nextid)
{
  stateid_t currentid = GetCurrentStateID();
  if( nextid != currentid ){
    if(!_statefactory.count(nextid)){
      ProduceError(UNKNOWN_STATE_REQUETSED,
		   "Request for transition to unknown state");
      nextid = _errhandler;
    }
    //make sure the current state's exit gets called first
    _current_state.reset();
    //now instantiate the new state
    _current_state.reset(_statefactory[nextid]->enter(this, ));
  }
  return status;
}

int StateMachine::Start(stateid_t initialstate)
{
  return Transition(initialstate);
}

int StateMachine::RegisterEventHandler(event_t evt, stateid_t state,
				       const EventHandler& handler);{
  if(!_statefeactory.count(stateid_t)){
    return ProduceError(UNKNOWN_STATE_REQUESTED,
			"Attempt to register eventhandler w/unknown stateid");
  }
  _statefactory[state]->event_handlers[evt] = handler;
  return STATUS_OK;
}

status_t StateMachine::ProduceError(status_t code, const std::string& msg)
{
  status = code;
  status_msg = msg;
  std::cerr<<msg<<'\n';
  return status;
}
