#include <cassert>
#include "executive.h"
#define IDLE 0
#define PENDING 1
#define RUNNING 2

//std::condition_variable cond;
std::mutex mtx;


Executive::Executive(size_t num_tasks, unsigned int frame_length, unsigned int unit_duration)
	: p_tasks(num_tasks), frame_length(frame_length), unit_time(unit_duration)
{
}


void Executive::set_periodic_task(size_t task_id, std::function<void()> periodic_task, unsigned int wcet)
{
	assert(task_id < p_tasks.size()); // Fallisce in caso di task_id non corretto (fuori range)
	
	p_tasks[task_id].function = periodic_task;
	p_tasks[task_id].wcet = wcet;
}

void Executive::set_aperiodic_task(std::function<void()> aperiodic_task, unsigned int wcet)
{
 	ap_task.function = aperiodic_task;
 	ap_task.wcet = wcet;
}
		
void Executive::add_frame(std::vector<size_t> frame)
{
	for (auto & id: frame)
		assert(id < p_tasks.size()); // Fallisce in caso di task_id non corretto (fuori range)
	
	frames.push_back(frame);

	/* ... */
}

void Executive::run()
{
	rt::priority prio(rt::priority::rt_max-1);
	ap_task.priority = --prio;
	rt::affinity aff("1");

	for (size_t id = 0; id < p_tasks.size(); ++id)
	{
		assert(p_tasks[id].function); // Fallisce se set_periodic_task() non e' stato invocato per questo id
		
		p_tasks[id].thread = std::thread(&Executive::task_function, std::ref(p_tasks[id]), std::ref(mutex));
		p_tasks[id].status = IDLE;
		rt::set_affinity(p_tasks[id].thread, aff);

		p_tasks[id].priority = --prio;

		try{
			rt::set_priority(p_tasks[id].thread, p_tasks[id].priority);
		}
		catch (rt::permission_error &){
			std::cerr << "Warning: RT priorities are not available"<< std::endl;
		}
		/* ... */
	}
	
	assert(ap_task.function); // Fallisce se set_aperiodic_task() non e' stato invocato
	
	ap_task.thread = std::thread(&Executive::task_function, std::ref(ap_task), std::ref(mutex));
	try{
		rt::set_priority(ap_task.thread, rt::priority::not_rt);
	}
	catch (rt::permission_error &){
		std::cerr << "Warning: RT priorities are not available"<< std::endl;
	}

	rt::set_affinity(ap_task.thread, aff);
	
	std::thread exec_thread(&Executive::exec_function, this);

	rt::set_affinity(exec_thread, aff);

	try{
		rt::set_priority(exec_thread, rt::priority::rt_max);
	}
	catch (rt::permission_error &){
		std::cerr << "Warning: RT priorities are not available"<< std::endl;
	}
	/* ... */
	
	exec_thread.join();
	
	ap_task.thread.join();
	
	for (auto & pt: p_tasks)
		pt.thread.join();
}

void Executive::ap_task_request()
{
	ap_task.rem_wcet = ap_task.wcet;
	ap_task.status = PENDING;	
	try{
		rt::set_priority(ap_task.thread, ap_task.priority);
	}
	catch (rt::permission_error &){
		std::cerr << "Warning: RT priorities are not available"<< std::endl;
	}
	/* ... */
}


void Executive::task_function(Executive::task_data & task, std::mutex & mutex)
{
	while (true)
	{		
		{
			std::unique_lock<std::mutex> l(mutex);
			while( task.status == IDLE )				// status 0 sospeso, 1 attivo ???
				task.cond.wait(l);
		}
		task.status = RUNNING;
		task.function();		
		std::unique_lock<std::mutex> l(mutex);			// mutex released here
		task.status = IDLE;
	}
	/* ... */

}

void Executive::exec_function()
{
	std::cout << "--- Executive, priorità: " << rt::this_thread::get_priority() << std::endl;
	auto frame_id = 0;
	auto frame_n = 0;
	auto wcet_tot = 0;
	//ap_on = false;
	ap_available = false;
	unsigned int slack_time;
	/* ... */
	auto start = std::chrono::high_resolution_clock::now();
	auto point = std::chrono::steady_clock::now();

	while (true)
	{	
		wcet_tot = 0;
		{
			std::unique_lock<std::mutex> l(mutex);// mutex acquired here
			std::cout << "---------- Frame: " << frame_id+1 << " ----------" << std::endl;
			for(int i  = 0; i<frames[frame_id].size(); i++){   //scorro nel frame gli id dei task da mettere in esecuzione nel frame corrente
				size_t id = frames[frame_id][i];
				wcet_tot += p_tasks[id].wcet;
				
				if(p_tasks[id].status == IDLE){
					try{
						rt::set_priority(p_tasks[id].thread, p_tasks[id].priority);					}
					catch (rt::permission_error &){
						std::cerr << "Warning: RT priorities are not available"<< std::endl;
					}
					p_tasks[id].status = PENDING;   // devo risvegliare tutti i task del frame e farli gestire con le priorità
					p_tasks[id].cond.notify_one();
				}
			}
			slack_time = frame_length - wcet_tot;
			std::cout << "Slack Time: " << slack_time*10 << "ms" << std::endl;
			if(slack_time>0 && ap_task.status == PENDING){
				ap_task.cond.notify_one();
			}
			else{
				if(slack_time>0 && ap_task.status == RUNNING){
					try{
						rt::set_priority(ap_task.thread, ap_task.priority);
					}
					catch (rt::permission_error &){
						std::cerr << "Warning: RT priorities are not available"<< std::endl;
					}
				}
			}
			point += std::chrono::milliseconds(frame_length*unit_time);
		}
		
		if(ap_task.status != IDLE){
			if(slack_time < ap_task.rem_wcet){
				std::cout << "Executing AP for " << (slack_time*unit_time).count() << "ms" << std::endl;
				point += std::chrono::milliseconds(slack_time*unit_time);
				std::this_thread::sleep_until(point);	
				ap_task.rem_wcet -= slack_time;	
			}else{
				std::cout << "Executing AP for " << (ap_task.rem_wcet*unit_time).count() << "ms" << std::endl;
				point += std::chrono::milliseconds(ap_task.rem_wcet*unit_time);
				std::this_thread::sleep_until(point);		
				ap_task.rem_wcet -= slack_time;	
			}
			try{
				rt::set_priority(ap_task.thread, rt::priority::not_rt);			//il task non ha terminato in tempo, per questo faccio terminare la sua esecuzione con priorità non real-time per non intralciare gli altri task
			}
			catch (rt::permission_error &){
				std::cerr << "Warning: RT priorities are not available"<< std::endl;
			}

			std::cout << "- Periodic schedule -" << std::endl;
			auto end_ap = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double, std::milli> elapsed(end_ap - start);
			std::cout << "Aperiodic execution time: " << elapsed.count() << "ms" << std::endl;
			std::this_thread::sleep_until(point);
		}else{
			//point += std::chrono::milliseconds(frame_length*unit_time);
			std::this_thread::sleep_until(point);
		}


		/* Rilascio dei task periodici del frame corrente e aperiodico (se necessario)... */
				
		/* Attesa fino al prossimo inizio frame ... */
		
		/* Controllo delle deadline ... */
		
		for(int i  = 0; i<frames[frame_id].size(); i++){   //scorro nel frame gli id dei task da mettere in esecuzione nel frame corrente
			size_t id = frames[frame_id][i];
			if(p_tasks[id].status == PENDING || p_tasks[id].status == RUNNING){
				std::cout << "DEADLINE MISS!!!!!!!!!!!!!! TASK: " << id+1 << std::endl;
				try{
					rt::set_priority(p_tasks[id].thread, rt::priority::not_rt);			//il task non ha terminato in tempo, per questo faccio terminare la sua esecuzione con priorità non real-time per non intralciare gli altri task
				}
				catch (rt::permission_error &){
					std::cerr << "Warning: RT priorities are not available"<< std::endl;
				}
			}

			/* ... */
		}


		auto next = std::chrono::high_resolution_clock::now();	
		std::chrono::duration<double, std::milli> elapsed(next - start);
		start=next;
	
		std::cout << "--- Frame duration: " << elapsed.count() << "ms" << " ---" <<std::endl;

		if (++frame_id == frames.size()){
			frame_id = 0;
			frame_n++;
		}

	}
}


