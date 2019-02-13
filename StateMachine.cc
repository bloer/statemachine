#include "StateMachine.hh"
#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

using namespace fsm;

//static initializers
const stateid_t nullstate = GetStateID(nullptr); //this *should* be in State.cc
const event_t StateMachine::ERROR_DEFAULT = "fsm::StateMachine::ERROR_DEFAULT";
const event_t StateMachine::TIMEOUT = "fsm::StateMachine::TIMEOUT";
const event_t StateMachine::STOP = "fsm::StateMachine::STOP";


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
  std::lock_guard<std::mutex> lock(_msgq_mutex);
  _msgq.push(msg); 
  _msg_avail.notify_one();
  //should we wait for a return value??
  return status;
}


status_t StateMachine::Handle(Message&& msg)
{
  std::lock_guard<std::mutex> lock(_msgq_mutex);
  _msgq.push(std::move(msg));
  _msg_avail.notify_one();
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

status_t StateMachine::Start(const stateid_t& initialstate, long mstimeout,
			     bool block)
{
  //make sure we're in sane state
  if(_main_loop.joinable()){
    std::cerr<<"StateMachine::Start called when already running; ignoring!\n";
    return status;
  }
  if(mstimeout >= 0) SetTimeout(mstimeout);
  status = Transition(initialstate);
  if(status != STATUS_OK)
    return status;
  if(block)
    MainLoop();
  else
    _main_loop = std::thread(&StateMachine::MainLoop, this);
  return status;
}

status_t StateMachine::Stop(bool wait)
{
  Handle(STOP);
  if(wait && _main_loop.joinable())
    _main_loop.join();
  return status;
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
  bool running = true;
  while(running){
    //are there any messages in the queue?
    std::unique_lock<std::mutex> lock(_msgq_mutex);
    if(_msgq.empty()){
      //nothing in queue, wait for a new message
      if(_msg_avail.wait_for(lock, std::chrono::milliseconds(_mstimeout))  == 
	 std::cv_status::timeout){
	//no message received, so issue a timeout event
	_msgq.emplace(TIMEOUT); //should we just call Process here??
      }
    }
    while(_msgq.size()){
      Message m(std::move(_msgq.front()));
      _msgq.pop();
      lock.unlock(); //unlock after we've processed the message
      ProcessMessage(m);
      if(m.event == STOP){ //we're done
	running = false;
	break;
      }
      lock.lock(); //need to relock to evaluate while condition
    }
  }
  //someone has called Stop. Make sure we exit the active state
  _current_state.reset(nullptr);
  return status;
}
