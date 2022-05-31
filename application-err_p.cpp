#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <sstream>
#include "executive.h"
#include "busy_wait.h"

Executive exec(5, 4);

void thread_wcet(int milli, int n){
	std::ostringstream os, os1;
	os << "Sono il task numero " << n << ", priorità: " << rt::this_thread::get_priority() << std::endl;
	std::cout << os.str();
	auto last = std::chrono::high_resolution_clock::now();
	busy_wait(milli);
	auto next = std::chrono::high_resolution_clock::now();	
	std::chrono::duration<double, std::milli> elapsed(next - last);
	os1<< "Task " << n << " Time elapsed: " << elapsed.count() << "ms" << std::endl;
	std::cout << os1.str();
	}

void task0()
{
	int n = 1;
	thread_wcet(9, n);
	/* Custom Code */
}

void task1()
{
	int n = 2;
	//std::cout << "Sono il task numero " << n << ", priorità: " << rt::this_thread::get_priority() << std::endl;
	thread_wcet(30, n);
	/* Custom Code */
}

void task2()
{
	int n = 3;
	//std::cout << "Sono il task numero " << n << ", priorità: " << rt::this_thread::get_priority() << std::endl;
	thread_wcet(9, n);
	/* Custom Code */
}

void task3()
{
	int n = 4;
	//std::cout << "Sono il task numero " << n << ", priorità: " << rt::this_thread::get_priority() << std::endl;
	thread_wcet(28, n);
	/* Custom Code */
}

void task4()
{
	int n = 5;
	//std::cout << "Sono il task numero " << n << ", priorità: " << rt::this_thread::get_priority() << std::endl;
	thread_wcet(9, n);
	/* Custom Code */
}

/* Nota: nel codice di uno o piu' task periodici e' lecito chiamare Executive::ap_task_request() */

void ap_task()
{
	std::cout << "Sono un task aperiodico, priorità: " << rt::this_thread::get_priority() << std::endl;
	/* Custom Code */
}

int main()
{

	busy_wait_init();

	exec.set_periodic_task(0, task0, 1); // tau_1
	exec.set_periodic_task(1, task1, 2); // tau_2
	exec.set_periodic_task(2, task2, 1); // tau_3,1
	exec.set_periodic_task(3, task3, 3); // tau_3,2
	exec.set_periodic_task(4, task4, 1); // tau_3,3
	/* ... */
	
	exec.set_aperiodic_task(ap_task, 2);
	
	exec.add_frame({0,1,2});
	exec.add_frame({0,3});
	exec.add_frame({0,1});
	exec.add_frame({0,1});
	exec.add_frame({0,1,4});   //0->1   1->2   2->31   3->32   4->33
	/* ... */
	exec.run();
	
	return 0;
}