#include "StateMachine.hh"
#include <iostream>

using namespace fsm;

//static initializers
const stateid_t StateMachine::AllStatesID=GetStateID<StateMachine::AllStates>();
const stateid_t StateMachine::nullstate = GetStateID(nullptr);

//constructor
StateMachine::StateMachine() : 
  _previous_state(GetStateID(nullptr))
{
  ResetStatus();
  RegisterState<AllStates>("AllStates");
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
  //see if the state has an erorr handler
  auto& handlers = _statefactory[currentid]->event_handlers;
  auto it = handlers.find(msg.event);
  if(it != handlers.end()){
    nextid = (it->second)(_current_state.get(), msg);
  }
  else{
    //check for a global event handler
    handlers = _statefactory[AllStatesID]->event_handlers;
    it = handlers.find(msg.event);
    if(it != handlers.end())
      nextid = (it->second)(_current_state.get(), msg);
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
    else{
      //make sure the current state's exit gets called first
      _current_state.reset();
      //now instantiate the new state
      _current_state.reset(_statefactory[nextid]->enter(this));
    }
  }
  return status;
}

int StateMachine::Start(stateid_t initialstate)
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
