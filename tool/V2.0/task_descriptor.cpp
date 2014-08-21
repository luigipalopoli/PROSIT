#include "task_descriptor.hpp"

namespace PrositCore {
  void GenericTaskDescriptor::insert_deadline(PrositTypes::DeadLineUnit deadline) {
    if (deadline%deadline_step)
      EXC_PRINT_2("Wrong deadline values set for task ", name);
    pair<unsigned int, double> entry(deadline,0.0);
    if(!probabilistic_deadlines.insert(entry))
      EXC_PRINT_2("cannot create deadline for task ", name);
    return;
  };
  
  void GenericTaskDescriptor::compute_probability() {
    
    if(!probability_solver)
      EXC_PRINT_2("Probability solver unset for task ", name);
    
    if(probabilistic_deadlines.empty())
      EXC_PRINT_2("No deadline specified for task ", name);

    if (solved)
      return;
    
    probability_solver->solve(probabilistic_deadlines, deadline_step);
    
  };
   
  double GenericTaskDescriptor::get_probability(PrositTypes::DeadLineUnit deadline) {
    DeadlineProbabilityMapIter it = probabilistic_deadlines.find(deadline);    
    if(!solved)
      compute_probability();
    if (it == proabilistic_deadlines.end())
      EXC_PRINT_2("Deadline does not exist for task ",name); 
    
    return it->second;
  };
  
  void GenericTaskDescriptor::set_solver(ProbabilitySolver * psd) {
    probability_solver = psd;
    solved = false;
    probability_solver->register_task(this);
  };
    
}  