#include <iostream>
#include <string>
#include "StateMachine.hh"

using namespace fsm;


struct TimedState{
  std::string name;
  mstick_t wait;
  stateid_t next;

  TimedState(const std::string& Name, mstick_t Wait, stateid_t Next) : 
    name(Name), wait(Wait), next(Next)
  {
    std::cout<<"Entering state "<<name<<std::endl;
  }

  ~TimedState()
  {
    std::cout<<"Leaving state "<<name<<std::endl;
  }

  stateid_t HandleTimeout(VState* st){
    mstick_t dt = mstick() - st->GetTimeEntered();
    if(dt > wait)
      return next;
    std::cout<<name<<": "<<wait-dt<<" ms remaining."<<std::endl;
    return nullstate;
  }
};


struct State1;
struct State2;
struct State3;

struct State1 : public TimedState{
  State1() : TimedState("State1", 2500, GetStateID<State2>()) {}
};

struct State2 : public TimedState{
  State2() : TimedState("State2", 10000, GetStateID<State3>()) {}
};

struct State3 : public TimedState{
  State3() : TimedState("State3", 400, GetStateID<State1>()) {}
};

template<class T> void reg(StateMachine& sm, const std::string& statename)
{
  sm.RegisterState<T>(statename);
  sm.RegisterEventHandler<T>(StateMachine::TIMEOUT, &T::HandleTimeout);
}

void Cleanup(){
  std::cout<<"Cleanup. Make sure everything is back to uninitialized state"
	   <<std::endl;
}

int main(){
  StateMachine sm;
  reg<State1>(sm, "State1");
  reg<State2>(sm, "State2");
  reg<State3>(sm, "State3");
  sm.RegisterEventHandler(StateMachine::STOP,&Cleanup);
  std::cout<<"Enter a number to change timeout period, Ctrl-D to quit."
	   <<"\n Starting in 2 seconds..."
	   <<std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(2));
  sm.Start(GetStateID<State1>(), 2000);

  long timeout;
  while( std::cin>>timeout )
    sm.SetTimeout(timeout);
  
  std::cout<<"Stopping state machine now"<<std::endl;
  sm.Stop(true);
  
  return 0;
}
