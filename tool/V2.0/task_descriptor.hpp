/*
 * Classes and function for optimisation
 *
 * Copyright (C) 2014, University of Trento
 * Authors: Luigi Palopoli <palopoli@disi.unitn.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

///!@file task_descriptor.hpp
///This file contains the root elements of the hierarchy defining 
///task descriptor. At the root of the hiearchy we have the
///GenericTaskDescriptor, which models both periodic and aperiodic
///tasks depending on a flag. Then we have two subclasses that
///model a different management of the task: FixedPriorityTaskDescriptor (task
///scheduled using fixed priority), ResourceReservationTaskDescriptor (task scheduled using
///resource reservations). The key method in the hiearchy is compute_probability, which
///computes the probability of respecting a sequence of deadlines that have to be registered
///before calling the method and are multiple of a fixed basic deadline (an obvious choice
///for resource reservations is to have deadlines that are integer multiples of the server period).
///The computation of the probability relies on an externally provided solver (see the documentation
///of PrositCore::Solver class.
#ifndef TASK_DESCRIPTOR_HPP
#define TASK_DESCRIPTOR_HPP
#include <vector>
#include <string>
#include <utility>
#include <memory>
#include "prosit_types.hpp"
using namespace std;
#include "pmf.hpp"
namespace PrositCore {
  //! Generic class descriptor
  /*! It is the root of the hierarchy of task descriptors
   * This descriptor only contains the timing information.
   * The hierarchy is specialised on the basis of: 1) scheduling algorithm
   * 2) solver family
   */
  class GenericTaskDescriptor {
  protected:
    string name; /*!< Name of the task */
    unique_ptr<PrositAux::pmf> C; /*!< Distribution of the computation time */
    unique_ptr<PrositAux::pmf> Z; /*!< Distribution of the interarrival time */
  
    bool verbose; /*!< Flag to print out information while processing */
    bool periodic; /*!< Flag for periodic tasks */
    unsigned int P;  /*!< Period (has meaning onlly if periodic flag is set) */
    
    unsigned int deadline_step; /*!< Granularity to define probabilistic deadlines */

    DeadlineProbabilityMap probabilistic_deadlines; /*!< Map associating deadlines with probabilities */

    
    Solver * ProbabilitySolver; /*!< Pointer to the object containing the solution algorithm for probabilities */
  public:

    //! Constructor for aperiodic Tasks
    /*! \param nm unique identifier for the task
     *  \param Cd pointer to the distribution of the computation times 
     *  \param Zd distirbution of the interarrival time
     */
    GenericTaskDescriptor(const char * nm, unique_ptr<PrositAux::pmf> Cd, unique_ptr<PrositAux::pmf> Zd) throw(Exc):
      name(nm),
      C(Cd),
      Z(Zd),
      verbose(false),
      periodic(false),
      P(0),
      deadline_step(0),
      qos(0.0)
    {};
    
    //! Constructor for periodic tasks
    /*! \param nm unique identifier for the task
     *  \param Cd pointer to the distribution of the computation times 
     *  \param Pd task period
     */
    GenericTaskDescriptor(const char * nm, unique_ptr<PrositAux::pmf> Cd, unsigned int Pd) throw(Exc):
      name(nm),
      C(std::move(Cd)),
      Z(new PrositAux::pmf()),
      verbose(false),
      periodic(true),
      P(Pd),
      deadline_step(0),
      qos(0.0)
    {
      Z->set(Pd, 1.0);
    };
    
    //! Set verbose flag to a specified value
    /*! 
     * \param verbosed desidered value for the verbose flag
     * \return old value for the verbose flag
     */
    bool set_verbose(bool verbosed) {
      bool current=verbose;
      verbose = verbosed;
      return current;
    };
    
    //! Verify if the task is periodic
    bool is_periodic() const {
      return periodic;
    };
    
    //! Returns the task period
    /*! 
     * \return the task period (Exception raised if not periodic)
     */
    int get_period() const throw (Exc){
      if(!periodic)
	EXC_PRINT_2("Period wrongly required for aperiodic task ", name);
      return P;
    };
    
    //! Returns a copy of the computation time distribution
    /*!
     * \return a copy of the pmf related to the computation time
     */
    PrositAux::pmf * get_computation_time() const {
      return C.get();
    };

    //! Returns a copy of the interarrival time distribution
    /*!
     * \return a copy of the pmf related to the interarrival time (Exceprion of the task is periodic)
     */
    PrositAux::pmf * get_interarrival_time() const throw (Exc){
      if(periodic)
	EXC_PRINT_2("Interarrival time wrongly required for periodic task ", name);
      return Z.get();
    };    


    //! Define a deadline
    /*! \param deadline has to be a multiple of deadline_step
     * \return Exception thrown for multiple entries and for non-multiple
     * of the deadline_step
     */
    void insert_deadline(unsigned int deadline) throw (Exc) {
      if (deadline%deadline_step)
	EXC_PRINT_2("Wrong deadline values set for task ", name);
      pair<unsigned int, double> entry(deadline,0.0);
      if(!probabilistic_deadlines.insert(entry))
	EXC_PRINT_2("cannot create deadline for task ", name);
      return;
    };
    

    //! Computes the probability of respecting the deadlines
    /*! These probabilities are computed for a given configuration
     * of the scheduling parameters. (Deadlines have been previously registered 
     * by calling insert
     *      
     */
    virtual void compute_probability()=0;
 
    //!Returns the probability associated with a deadline
    /*!compute_probability is implicitly called if not called before.
     * \param deadline: the deadline for which the computations
     * is required.
     * \return The requested probability. An exception is thrown if the deadline
     * has not been registered.
     */
    double get_probability(unsigned int deadline) const throw(Exc);


    //! Destruct, which is virtual, being the class polymorphic
    virtual ~GenericTaskDescriptor() {};
  };
  

  //! Class for fixed priority task descriptors
  class FixedPriorityTaskDescriptor : public GenericTaskDescriptor {
    unsigned int priority; /*!<Scheduling priority of the task */
  public:
    //! Constructor for aperiodic Fixed Priority Tasks
    /*! \param nm unique identifier for the task
     *  \param Cd pointer to the distribution of the computation times 
     *  \param Zd distirbution of the interarrival time
     *  \param priorityd scheduling priority (in the range 0..99)
     */
    FixedPriorityTaskDescriptor(const char * nm, unique_ptr<PrositAux::pmf> Cd, unique_ptr<PrositAux::pmf> Zd, unsigned int priorityd) throw(Exc):
      GenericTaskDescriptor(nm,Cd,Zd)
    {
      if (priorityd>99)
	EXC_PRINT_2("Priority out of range for task ", nm);
      priority = priorityd;
    };
    
    //! Constructor for Fixed Priority periodic tasks
    /*! \param nm unique identifier for the task
     *  \param Cd pointer to the distribution of the computation times 
     *  \param Pd task period
     *  \param priorityd scheduling priority (in the range 0..99)
     */
    FixedPriorityTaskDescriptor(const char * nm, unique_ptr<PrositAux::pmf> Cd, unsigned int Pd, unsigned int priorityd) throw(Exc):
      GenericTaskDescriptor(nm,Cd,Pd)
    {
      if (priorityd>99)
	EXC_PRINT_2("Priority out of range for task ", nm);
      priority = priorityd;
    };
    
    //!Returns the task priority
    unsigned int get_priority() const {return priority;}; 

    //! Sets the task priority
    /*! \param priority new task priority
     * \return old priority
     */
    unsigned int set_priority(unsigned int priorityd) {
      unsigned int old_prio = priority;
      priority = priorityd;
      return old_prio;
    };
  };

  
  class ResourceReservationTaskDescriptor: public GenericTaskDescriptor {
    int Q; /*!< Reservation budget */
    int Ts; /*!< Reservation period */
   
  public:
    ResourceReservationTaskDescriptor(const char * nm, unique_ptr<ProxitAux::pmf> Cd, unique_ptr<PrositAux::pmf> Zd, const int Qd, const int Tsd) throw(Exc):
      GenericTaskDescriptor(nm, Cd, Zd), 
      Q(Qd),
      Ts(Tsd),
    {
      if (double(Qd)/double(Tsd) > 1.0)
	EXC_PRINT_2("Server period period too small for task", name);
    };

    virtual ~ResourceReservationTaskDescriptor() {
    };
    
    int get_budget() const {
      return Q;
    };
    int get_server_period() const {
      return Ts;
    };

    void set_budget(int Qd) throw(Exc) {
      if (double(Qd)/Ts > 1.0)
	EXC_PRINT_2("budget too large for object", name);
      Q=Qd;
    };
    int set_server_period(int Tsd) {
      if (double(Q)/double(Tsd) > 1.0)
	EXC_PRINT_2("server period too small for task ", name);
      Ts=Tsd;
    };
    //Need to redefine this for task specific QoS

  }
};