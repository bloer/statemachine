#include "StateMachine.hh"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>

using namespace fsm;

//static initializers
const stateid_t nullstate = GetStateID(nullptr); //this *should* be in State.cc
const event_t StateMachine::ERROR_DEFAULT = "fsm::StateMachine::ERROR_DEFAULT";
const event_t StateMachine::TIMEOUT = "fsm::StateMachine::TIMEOUT";

//constructor
StateMachine::StateMachine() : 
  _previous_state(GetStateID(nullptr))
{
  ResetStatus();
  RegisterState<DefaultErrorHandler>("DefaultErrorHandler");
}

StateMachine::~StateMachine()
{
}

status_t StateMachine::Handle(const Message& msg)
{
  _msgq.push(msg); //should we wait for a return value??
  return status;
}


status_t StateMachine::Handle(Message&& msg)
{
  _msgq.push(std::move(msg));
  return status;
}

status_t StateMachine::ProcessMessage(const Message& msg)
{
  status = STATUS_OK; // do we really want to do this?
  stateid_t currentid = GetCurrentStateID();
  auto it = _eventhandlers.find(msg.event);
  _stop_processing = false;
  if(it == _eventhandlers.end()){
    //todo: do we want to cause an error if we don't have a handler?
  }
  else{
    for(auto& seqhandler : it->second){
      statehandler& sh = seqhandler.second;
      //make sure this handler is referencing this state
      if(sh.state != nullstate && sh.state != currentid)
	continue;
      //call the callback
      stateid_t nextid = (sh.handler)(_current_state.get(), msg);
      //do we need to transition?
      if(nextid != nullstate && nextid != currentid){
	Transition(nextid);
	currentid = GetCurrentStateID();
	//todo: handle errors generated during transition
      }
      //is this an override sequence?
      if(seqhandler.first < 0 || _stop_processing)
	break;
    }
  }
  return status;
}


status_t StateMachine::Transition(stateid_t nextid, bool checkfirst)
{
  if(checkfirst){
    stateid_t currentid = GetCurrentStateID();
    if(nextid == currentid || nextid == nullstate) //nothing to do
      return status; 
  }
  if(!_statefactory.count(nextid)){
    ProduceError(UNKNOWN_STATE_REQUESTED,
		 "Request for transition to unknown state");
    //transition to some error state now
    //TODO: allow user to override
    nextid = GetStateID<DefaultErrorHandler>();
  }
  //make sure the current state's exit gets called first
  _current_state.reset(nullptr);
  //now instantiate the new state
  _current_state.reset(_statefactory[nextid]->enter(this));
  
  return status;
}

status_t StateMachine::Start(const stateid_t& initialstate, long mstimeout)
{
  if(mstimeout >= 0) SetTimeout(mstimeout);
  status = Transition(initialstate);
  if(status != STATUS_OK)
    return status;
  return MainLoop();
}

status_t StateMachine::ProduceError(status_t code, const std::string& msg)
{
  status = code;
  status_msg = msg;
  return status;
}


int StateMachine::RemoveEventHandler(const event_t& evt, int sequence,
				     const stateid_t& st)
{
  int nfound = 0;
  auto matchrange = _eventhandlers[evt].equal_range(sequence);
  for(auto it = matchrange.first; it != matchrange.second; ++it){
    statehandler& sh = it->second;
    if(sh.state == st) {
      _eventhandlers[evt].erase(it);
      ++nfound;
    }
  }
  return nfound;
  if(nfound == 0){
    std::cerr<<"Warning in RemoveEventHandler; no handler registered for \n"
	     <<"event "<<evt<<" sequence "<<sequence
	     <<" and state "<<st<<std::endl;
  }
  return nfound;
}

int StateMachine::RemoveAllHandlers(const event_t& evt)
{
  int nfound = 0;
  if(evt == ""){
    nfound = _eventhandlers.size();
    _eventhandlers.clear();
  }
  else{
    nfound = _eventhandlers.erase(evt);
  }
  return nfound;
}

StateMachine::DefaultErrorHandler::DefaultErrorHandler(StateMachine* sm) : 
  VState(sm)
{
  std::cerr<<"State machine has encountered error "<<sm->GetStatus()<<":\n"
	   <<sm->GetStatusMsg()<<'\n';
}

fsm::status_t StateMachine::MainLoop()
{
  //make sure we can get into the first transition
  _running = true;
  while(_running){
    //are there any messages in the queue?
    while(_msgq.size() && _running/*allow Stop with msgs in queue*/){
      Message& m = _msgq.front(); //need to hold onto lock longer this way...
      ProcessMessage(m);
      _msgq.pop();
    }
    //if we get here, queue is empty, so sleep
    if(_running){
      std::this_thread::sleep_for(std::chrono::milliseconds(_mstimeout));
      Handle(TIMEOUT);
    }
  }
  //someone has called Stop. Make sure we exit the active state
  _current_state.reset(nullptr);
  return status;
}
