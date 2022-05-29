#include <iostream>
#include <chrono>
#include <thread>
#include "executive.h"
#include "busy_wait.h"

Executive exec(5, 4);

void thread_wcet(int milli){
	auto last = std::chrono::steady_clock::now();
	//auto point = std::chrono::steady_clock::now();
	//point += std::chrono::seconds(sec);
	//std::this_thread::sleep_until(point);	
	busy_wait(milli);
	auto next = std::chrono::steady_clock::now();	
	std::chrono::duration<double, std::milli> elapsed(next - last);
	std::cout << "Time elapsed: " << elapsed.count() << "ms" << std::endl;
	}

void task0()
{
	std::cout << "Sono il task numero 1, priorità: " << rt::this_thread::get_priority() << std::endl;
	thread_wcet(1000);
	/* Custom Code */
}

void task1()
{
	std::cout << "Sono il task numero 2, priorità: " << rt::this_thread::get_priority() << std::endl;
	thread_wcet(20);
	/* Custom Code */
}

void task2()
{
	std::cout << "Sono il task numero 31, priorità: " << rt::this_thread::get_priority() << std::endl;
	thread_wcet(10);
	/* Custom Code */
}

void task3()
{
	std::cout << "Sono il task numero 32, priorità: " << rt::this_thread::get_priority() << std::endl;
	thread_wcet(30);
	/* Custom Code */
}

void task4()
{
	std::cout << "Sono il task numero 33, priorità: " << rt::this_thread::get_priority() << std::endl;
	thread_wcet(10);
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
