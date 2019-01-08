#include <iostream>
#include "StateMachine.hh"

using namespace fsm;

enum EVENTS {
  POLL = 1,
  ACTIVATE = 2,
  DEACTIVATE = 3,
};

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
};

int main(int argc, char** argv)
{
  StateMachine sm;
  sm.RegisterState<Active>("Active");
  sm.RegisterState<InActive>("InActive");
  sm.RegisterEventHandler(POLL, GetStateID<Active>(), &Active::poll);
  sm.RegisterEventHandler(POLL, GetStateID<InActive>(), &InActive::poll);
  sm.RegisterEventHandler(ACTIVATE, GetStateID<InActive>(),&InActive::activate);
  sm.RegisterEventHandler(DEACTIVATE, GetStateID<Active>(),&Active::deactivate);
  
  sm.Start(GetStateID<InActive>());
  sm.Handle(POLL);
  sm.Handle(ACTIVATE);
  sm.Handle(POLL);
  sm.Handle(DEACTIVATE);
  sm.Handle(POLL);

  return 0;
  
}
