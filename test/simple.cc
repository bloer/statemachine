#include <iostream>
#include "StateMachine.hh"

using namespace fsm;

//define some events
const event_t POLL       = "simple::POLL";
const event_t ACTIVATE   = "simple::ACTIVATE";
const event_t DEACTIVATE = "simple::DEACTIVATE";
const event_t BREAK      = "simple::BREAK";

struct NoState{}; //test what happens when we give unregistered

struct InActive;

struct Active{
  Active(){ std::cout<<"Entering Active state"<<std::endl; }
  ~Active(){ std::cout<<"Leaving Active state"<<std::endl; }
  
  stateid_t poll(){ 
    std::cout<<"in Active state"<<std::endl;
    return GetStateID(this);
  }
  
  stateid_t deactivate(){ 
    return GetStateID<InActive>();
  }
};

struct InActive{
  InActive(){ std::cout<<"Entering InActive state"<<std::endl; }
  ~InActive(){ std::cout<<"Leaving InActive state"<<std::endl; }
  
  stateid_t poll(){ 
    std::cout<<"in InActive state"<<std::endl;
    return GetStateID(this);
  }
  
  stateid_t activate(){ 
    return GetStateID<Active>();
  }

  stateid_t Break(){ return GetStateID<NoState>(); }
};

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
  
  std::cout<<"simple.cc: Starting state machine..."<<std::endl;
  sm.Start(GetStateID<InActive>());
  std::cout<<"simple.cc: Polling state machine..."<<std::endl;
  sm.Handle(POLL);
  std::cout<<"simple.cc: Activating state machine..."<<std::endl;
  sm.Handle(ACTIVATE);
  std::cout<<"simple.cc: Polling state machine..."<<std::endl;
  sm.Handle(POLL);
  std::cout<<"simple.cc: Deactivating state machine..."<<std::endl;
  sm.Handle(DEACTIVATE);
  std::cout<<"simple.cc: Polling state machine..."<<std::endl;
  sm.Handle(POLL);
  std::cout<<"simple.cc: Breaking..."<<std::endl;
  sm.Handle(BREAK);
  std::cout<<"simple.cc: Polling state machine..."<<std::endl;
  sm.Handle(POLL);
  std::cout<<"simple.cc: Exiting"<<std::endl;
  
  return 0;
  
}
