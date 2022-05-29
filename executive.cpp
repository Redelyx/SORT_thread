#include <cassert>
#include "executive.h"

//std::mutex mtx; 
std::condition_variable cond;


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
	rt::affinity aff("1");

	for (size_t id = 0; id < p_tasks.size(); ++id)
	{
		assert(p_tasks[id].function); // Fallisce se set_periodic_task() non e' stato invocato per questo id
		
		p_tasks[id].thread = std::thread(&Executive::task_function, std::ref(p_tasks[id]), std::ref(mutex));
		p_tasks[id].status = 0;
		rt::set_affinity(p_tasks[id].thread, aff);

		try
		{
			rt::set_priority(p_tasks[id].thread, --prio);
		}
		catch (rt::permission_error &)
		{
			std::cerr << "Warning: RT priorities are not available"<< std::endl;
		}
		/* ... */
	}
	
	assert(ap_task.function); // Fallisce se set_aperiodic_task() non e' stato invocato
	
	ap_task.thread = std::thread(&Executive::task_function, std::ref(ap_task), std::ref(mutex));
	
	std::thread exec_thread(&Executive::exec_function, this);

	rt::set_affinity(exec_thread, aff);

	try
	{
		rt::set_priority(exec_thread, rt::priority::rt_max-1);
	}
	catch (rt::permission_error &)
	{
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
	/* ... */
}


void Executive::task_function(Executive::task_data & task, std::mutex & mutex)
{

	while (true)
	{		
		std::unique_lock<std::mutex> l(mutex);	// mutex acquired here	

		while( ! task.status )						// status 0 sospeso, 1 attivo ???
			task.cond.wait(l);

		task.function();							// mutex released here
		task.status = 0;
	}
	/* ... */

}

void Executive::exec_function()
{
	std::cout << "--- Executive, priorità: " << rt::this_thread::get_priority() << std::endl;
	auto frame_id = 0;
	auto frame_n = 0;

	/* ... */

	auto start = std::chrono::high_resolution_clock::now();

	while (true)
	{
			
		{
			std::unique_lock<std::mutex> l(mutex);			// mutex acquired here
			std::cout << "--- Frame: " << frame_id+1 << std::endl;
			for(int i  = 0; i<frames[frame_id].size(); i++){   //scorro nel frame gli id dei task da mettere in esecuzione nel frame corrente
				
				size_t id = frames[frame_id][i];

				p_tasks[id].status = 1;   //??? devo risvegliare tutti quelli del frame e farli gestire con le priorità
				p_tasks[id].cond.notify_one();

			}
		}
		auto last = std::chrono::high_resolution_clock::now();
		
		auto point = std::chrono::steady_clock::now();
		
		point += std::chrono::milliseconds(frame_length);
		std::this_thread::sleep_until(point);
		
		auto next = std::chrono::high_resolution_clock::now();
		
		std::chrono::duration<double, std::milli> elapsed(next - last);

		std::cout << "From start: " << elapsed.count() << "ms" << std::endl;

		
		/* Rilascio dei task periodici del frame corrente e aperiodico (se necessario)... */
				
		/* Attesa fino al prossimo inizio frame ... */
		
		/* Controllo delle deadline ... */
		
	for (size_t id = 0; id < p_tasks.size(); ++id)
	{
		if(p_tasks[id].status == 1){
			std::cout << "DEADLINE MISS!!!!!!!!!!!!!! TASK: " << id+1 << std::endl;
			p_tasks[id].status = 0;
		}

		/* ... */
	}
		if (++frame_id == frames.size())
			frame_id = 0;
			frame_n++;
	}
}


