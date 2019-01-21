#include <iostream>
#include "StateMachine.hh"

using namespace fsm;

//define some events
const event_t POLL       = "simple::POLL";
const event_t ACTIVATE   = "simple::ACTIVATE";
const event_t DEACTIVATE = "simple::DEACTIVATE";
const event_t BREAK      = "simple::BREAK";
const event_t STOREMSG   = "simple::STOREMSG";
const event_t PRINTMSG     = "simple::PRINTMSG";

struct NoState{}; //test what happens when we give unregistered

struct InActive;

struct Active{
  Active(){ std::cout<<"Entering Active state"<<std::endl; }
  ~Active(){ std::cout<<"Leaving Active state"<<std::endl; }
  
  void poll(){ 
    std::cout<<"in Active state"<<std::endl;
  }
  
  stateid_t deactivate(){ 
    return GetStateID<InActive>();
  }

  void storemsg(VState* st){
    st->GetStateMachine()->
      RegisterObject("message", new std::string("This is the active message"));
  }
    
};

struct InActive{
  InActive(){ std::cout<<"Entering InActive state"<<std::endl; }
  ~InActive(){ std::cout<<"Leaving InActive state"<<std::endl; }
  
  void poll(){ 
    std::cout<<"in InActive state"<<std::endl;
  }
  
  stateid_t activate(){ 
    return GetStateID<Active>();
  }
  
  void storemsg(VState* st){
    st->GetStateMachine()->
      RegisterObject("message",new std::string("This is the inactive message"));
  }
  
  stateid_t Break(){ return GetStateID<NoState>(); }
};

void printmsg(VState* st)
{
  StateMachine* sm = st->GetStateMachine();
  //this version does not do any typechecking!
  std::cout<<"State is: "<<sm->GetCurrentStateName()<<": "
	   <<"Message is: "<< *sm->GetObject<std::string>("message") 
	   <<std::endl;
}

int main(int argc, char** argv)
{
  //make sure state IDs behave as we expect
  StateMachine sm;
  sm.RegisterState<Active>("Active");
  sm.RegisterState<InActive>("InActive");
  sm.RegisterEventHandler(POLL, GetStateID<Active>(), &Active::poll);
  sm.RegisterEventHandler(POLL, GetStateID<InActive>(), &InActive::poll);
  sm.RegisterEventHandler(ACTIVATE, GetStateID<InActive>(),&InActive::activate);
  sm.RegisterEventHandler(DEACTIVATE, GetStateID<Active>(),&Active::deactivate);
  sm.RegisterEventHandler(BREAK, GetStateID<InActive>(),&InActive::Break);
  sm.RegisterEventHandler(STOREMSG, GetStateID<Active>(), &Active::storemsg);
  sm.RegisterEventHandler(STOREMSG, GetStateID<InActive>(),&InActive::storemsg);
  sm.RegisterGlobalEventHandler(PRINTMSG, printmsg);
  
  std::cout<<"simple.cc: Starting state machine..."<<std::endl;
  sm.Start(GetStateID<InActive>());
  std::cout<<"simple.cc: Polling state machine..."<<std::endl;
  sm.Handle(POLL);
  std::cout<<"simple.cc: Activating state machine..."<<std::endl;
  sm.Handle(ACTIVATE);
  std::cout<<"simple.cc: Polling state machine..."<<std::endl;
  sm.Handle(POLL);
  std::cout<<"Storing message..."<<std::endl;
  sm.Handle(STOREMSG);
  std::cout<<"simple.cc: Deactivating state machine..."<<std::endl;
  sm.Handle(DEACTIVATE);
  std::cout<<"simple.cc: Polling state machine..."<<std::endl;
  sm.Handle(POLL);
  std::cout<<"Printing stored message..."<<std::endl;
  sm.Handle(PRINTMSG);
  
  std::cout<<"simple.cc: Breaking..."<<std::endl;
  sm.Handle(BREAK);
  std::cout<<"simple.cc: Polling state machine..."<<std::endl;
  sm.Handle(POLL);
  std::cout<<"simple.cc: Exiting"<<std::endl;
  
  return 0;
  
}
