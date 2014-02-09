
#ifndef SIM_H
#define SIM_H

#include <vector>
#include <list>
#include <map>
#include <string>
#include <cstdlib>
#include <memory.h>
#include <math.h>
#include <iostream>

struct Task;
struct Card;

using namespace std;

typedef vector<Task *> TaskList;
typedef list<Task *> TaskQueue;

#define dbg	cout

// shorthands for adding dependencies between tasks
void operator <<(Task &a, Task &b);
void operator >>(Task &a, Task &b);

ostream &operator <<(ostream &str, Task &t);

struct Task
{
	TaskList prev;
	TaskList next;
	int slotsFilled;
	int workerIndex; // the index of the worker that ran this task
	int cardIndex;	// the index of the card that ran this task
	
	TaskList E;
	map<Task *, int> dist;
	int minDist;
	string name;

	Task(const string &name) :
		prev(),
		next(),
		slotsFilled(0),
		workerIndex(-1),
		cardIndex(-1),
		E(), dist(), minDist(0),
		name(name)
	{}

	void addDep(Task *t)
	{
		prev.push_back(t);
		t->next.push_back(this);
	}

	int owningCard(int cardCount)
	{
		int count[cardCount];
		int maxCard = 0, maxCount = 0;
		memset(&count, 0, sizeof(int) * cardCount);
		int curCard;
		for(auto iter = prev.begin(); iter != prev.end(); iter++) {
			curCard = (*iter)->cardIndex;
			count[curCard]++;
			if(count[curCard] >= maxCount) {
				maxCard = curCard;
				maxCount = count[curCard];
			}
		}

		return maxCard;
	}

	int calcMaxDist(Task *task)
	{
		Task *pred;
		int maxDist = -1;
		int curDist;
		for(auto iter = prev.begin(); iter != prev.end(); iter++) {
			pred = *iter;
			if(pred->dist.find(task) != pred->dist.end()) {
				curDist = pred->dist[task];
				if(curDist > maxDist)
					maxDist = curDist;	
			}
		}

		return maxDist + 1;
	}

	void mergeDists(Task *pred)
	{
		for(auto iter = pred->dist.begin(); iter != pred->dist.end(); iter++) {
			dist[iter->first] = calcMaxDist(iter->first);
		}
	}

	void calcDistances()
	{
		int curDist;
		minDist = 1000;
		for(auto iter = E.begin(); iter != E.end(); iter++) {
			curDist = calcMaxDist(*iter);
			dist[*iter] = curDist;
			if(curDist < minDist)
				minDist = curDist;
		}

		for(auto iter = prev.begin(); iter != prev.end(); iter++) {
			mergeDists(*iter);
		}
	}
};


struct Card
{
	Task *xferTask; // task currently being stolen
	int xferTimeout; // how much longer until xferTask can be run
	int nextWorker;
	int workerCount;

	Card(int workers) :
		xferTask(NULL),
		xferTimeout(0),
		nextWorker(0),
		workerCount(workers)
	{}
};

class SchedSim
{
public:

	enum {
		dbCost = 1,
		cardCount = 2
	};

	TaskQueue *taskQueue;
	TaskQueue *newQueue;
	vector<TaskList> timeline;
	vector<Card> cards;
	int time;
	int pendingSteals;
	int numStealsPerformed;
	int numUnhealthySteals;
	Task *start;

	SchedSim(vector<Card> cards, Task *start) :
		taskQueue(new TaskQueue),
		newQueue(new TaskQueue),
		timeline(),
		cards(cards),
		time(0),
		pendingSteals(0),
		numStealsPerformed(0),
		numUnhealthySteals(0),
		start(start)
	{}
		

	int calcXferTime(Task *task, int cardIndex)
	{
		int t = 0;
		Task *dep;
		for(auto iter = task->prev.begin(); iter != task->prev.end(); iter++) {
			dep = *iter;
			if(dep->cardIndex != cardIndex)
				t += dbCost;
		}
		return t;
	}

	void dbgPrintDists(Task *t)
	{
		dbg << "Distances for task " << *t << endl;
		for(auto iter = t->dist.begin(); iter != t->dist.end(); iter++) {
			dbg << "\t" << *(iter->first) << ": " << iter->second << endl; 
		}
		dbg << "\tMin dist: " << t->minDist << endl;
	}

	void queueTaskDeps(Task *task)
	{
		Task *dep;
		for(auto iter = task->next.begin(); iter != task->next.end(); iter++) {
			dep = *iter;
			dep->slotsFilled++;
			if(dep->slotsFilled == dep->prev.size()) {
				dep->calcDistances();
				//dbgPrintDists(dep);
				newQueue->push_back(dep);				
			}
		}
	}

	// executes task
	void execTask(Task *task, int workerIndex, int cardIndex)
	{
		dbg << "exec " << *task << endl;
		task->workerIndex = workerIndex;
		task->cardIndex = cardIndex;
		TaskList &curTime = timeline[time];
		curTime.push_back(task);
		queueTaskDeps(task);
	}

	// steals task to other card
	bool stealTask(Task *task, int cardIndex)
	{
		if(cards[cardIndex].xferTask != NULL) return false;
		dbg << "steal " << *task << endl;
		task->cardIndex = cardIndex;
		cards[cardIndex].xferTask = task;
		cards[cardIndex].xferTimeout = calcXferTime(task, cardIndex);
		pendingSteals++;
		numStealsPerformed++;
		if(task->name[1] == 'i')
			numUnhealthySteals++;
		return true;
	}
	
	bool shouldISteal(Task *task, int stealFromCard, int &stealToCard)
	{
		if(cards.size() < 2) return false;
		const double pmax = 0.4, pmin = 0.02;
		const double distmax = 50.0;
		double a = (pmax - pmin) / pow(distmax, 2);
		double p = pmin + a * pow((float) task->minDist, 2);
		if(p > pmax) p = pmax;
		if(p < pmin) p = pmin;

		int val = rand() % 100;
		if(val < p * 100) {
			// TODO: this is not very good for > 2 cards
			stealToCard = (stealFromCard + 1) % cards.size();
			return true;
		}
		else
			return false;
	}

	// if all of task's deps are on a single card:
	//	steal (probability p) OR
	//	exec locally (1-p)
	// if task's deps are spread between cards:
	// 	move all deps to card that has most deps & exec there
	bool execOrStealTask(Task *task)
	{

		int owningCard = task->owningCard(cards.size());
		int xferCost = calcXferTime(task, owningCard);
		if(xferCost > 0)
			return stealTask(task, owningCard);
		else {
			Card &owner = cards[owningCard];
//			int stealCard;			
//			bool steal = shouldISteal(task, owningCard, stealCard);
//			if(steal)
//				return stealTask(task, stealCard);
//			else if(owner.nextWorker == owner.workerCount)
//				return false;
//			else {
//				execTask(task, owner.nextWorker++, 
//					owningCard);
//				return true;
//			}

			if(owner.nextWorker == owner.workerCount) {
				// allow probabilistic steal
				bool steal;
				int stealCard;
				steal = shouldISteal(task, owningCard, stealCard);
				if(steal)
					return stealTask(task, stealCard);
				else
					return false;
			}
			else {
				// execute on current card
				execTask(task, owner.nextWorker++, 
					owningCard);
				return true;
			}			
		}
	}

	void updateStealStatuses()
	{
		int cardIndex;
		for(cardIndex = 0; cardIndex < cards.size(); cardIndex++) {
			Card *cur = &cards[cardIndex];
			if(cur->xferTimeout > 0) {
				cur->xferTimeout--;
				if(cur->xferTimeout == 0) {
					// data xfer is complete, run the task
					execTask(cur->xferTask, cur->nextWorker++, cardIndex);
					cur->xferTask = NULL;
					pendingSteals--;
				}
			}
		}
	}
	

	bool step()
	{
		if(taskQueue->empty() && pendingSteals == 0)
			return false;
if(pendingSteals)
	dbg << "pending steal" << endl;
else
	dbg << "taskqueue not empty" << endl;

		newQueue->clear();
		timeline.push_back(TaskList());
		// reset workers
		for(auto iter = cards.begin(); iter != cards.end(); iter++)
			iter->nextWorker = 0;

		// if data transfers for any steals are complete, run the stolen tasks
		// on their respective cards
		updateStealStatuses();

		// push each task to a local worker if one is available, otherwise
		// allow a steal
		for(auto task = taskQueue->begin(); task != taskQueue->end(); task++) {
			bool success = execOrStealTask(*task);
			// if no steal/exec occurs, leave task on ready queue			
			if(!success)
				newQueue->push_back(*task);
		}

		TaskQueue *temp;
		temp = taskQueue;
		taskQueue = newQueue;
		newQueue = temp;

		time++;
		return true;
	}

	void reset()
	{
		time = 0;
		taskQueue->clear();
		timeline.clear();
		pendingSteals = 0;
		numStealsPerformed = 0;
		start->calcDistances();
		taskQueue->push_back(start);
		start->cardIndex = 0;
	}

	void run(Task *start)
	{
		time = 0;
		taskQueue->clear();
		timeline.clear();
		pendingSteals = 0;

		start->calcDistances();
		taskQueue->push_back(start);
		start->cardIndex = 0;
		while(!taskQueue->empty() || pendingSteals > 0) {
			dbg << "running step " << time << " of simulation" << endl;
			dbg << "task queue size: " << taskQueue->size() << endl;
			step();
			char line[10];
			cin.getline(line, 10);
			
		}
	}
};

#endif

