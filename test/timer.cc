#include <iostream>
#include <string>
#include "StateMachine.hh"

using namespace fsm;

//define some states
struct Printer{
  std::string name;
  Printer(const std::string& Name) : name(Name)
  { std::cout<<"Entering "<<name<<std::endl; }
  ~Printer(){ std::cout<<"Exiting "<<name<<std::endl; }
};

class State2;

struct State1{
  Printer p = Printer("State1");
  mstick_t wait = 3500;
  stateid_t HandleTick(VState* st)
  {
    mstick_t dt = mstick() - st->GetTimeEntered();
    if(dt > wait)
      return GetStateID<State2>();
    std::cout<<p.name<<": "<< wait-dt<<" ms remaining."<<std::endl;
    return nullstate;
  }
};

class State2 : public VState{
public:
  Printer p = Printer("State2");
  mstick_t wait = 10000;

  State2(StateMachine* fsm = nullptr) : VState(fsm) {
    if(fsm){
      fsm->SetTimeout(2500);
    }
  } 

  void HandleTick(){
    mstick_t dt = mstick() - time_entered;
    if(dt > wait){
      sm->Stop();
    }
    else{
      std::cout<<p.name<<": "<< wait-dt<<" ms remaining."<<std::endl;
    }
  }
};

int main(){
  StateMachine sm;
  sm.RegisterState<State1>("State1");
  sm.RegisterState<State2>("State2");
  sm.RegisterEventHandler<State1>(StateMachine::TIMEOUT, &State1::HandleTick);
  sm.RegisterEventHandler<State2>(StateMachine::TIMEOUT, &State2::HandleTick);
  
  sm.Start(GetStateID<State1>(), 200);
  return 0;
}
