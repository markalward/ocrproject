
#include <time.h>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <QObject>
#include "sim.h"
#include "temp.h"

Controller::Controller() :
	QObject()
{
	// create dep graph
	Task *s = createGraph();

	srand(time(NULL));

	// create cards
	vector<Card> cards;
	cards.push_back(Card(20));
	cards.push_back(Card(20));
	cards.push_back(Card(20));
	cards.push_back(Card(20));

	// create simulation
	sim = new SchedSim(cards, s);
	sim->reset();

	// create timer
	timer = new QTimer;
	timer->setInterval(140);
	connect(timer, SIGNAL(timeout()), this, SLOT(update()));

	win = new MainWindow(*sim, NULL);
	win->show();

	timer->start();
}

void Controller::update()
{
	if(!win->step()) {
		std::cout << "Simulation finished" << endl;
		std::cout << "Total time: " << sim->time << endl;
		std::cout << "# of steals: " << sim->numStealsPerformed << endl;
		std::cout << "# of unhealty steals: " << sim->numUnhealthySteals << endl;
		timer->stop();
	}
}

Task *Controller::createGraph()
{
	Task *s = new Task("start");
	Task *f = new Task("finish");
	const int fanout = 4;
	char prefix = 'a';
	for(int i = 0; i < fanout; i++) {
		createLoop(s, f, prefix);
		prefix++;
	}
	return s;
}

void Controller::createLoop(Task *s, Task *fin, char prefix)
{
	const int iters = 20;
	Task *prev = s;
	Task *f;

	for(int i = 0; i < iters; i++) {
		f = new Task(std::string(1, prefix));
		f->E = TaskList({fin});
		createFan(prev, f, prefix);
		prev = f;
	}
	*f >> *fin;
}



void Controller::createFan(Task *s, Task *f, char prefix)
{
	const int fanout = 20;
	const int branchlen = 20;
	Task *prev;
	Task *cur;

	for(int branch = 0; branch < fanout; branch++) {
		prev = s;
		for(int i = 0; i < branchlen; i++) {
			cur = new Task(std::string(1, prefix) + 
				std::string("i"));
			cur->E = TaskList({f});
			*prev >> *cur;
			prev = cur;
		}
		*cur >> *f;
	}
}


void createFan(Task *s, Task *f, char prefix)
{
	const int fanout = 20;
	const int branchlen = 20;
	Task *prev;
	Task *cur;

	for(int branch = 0; branch < fanout; branch++) {
		prev = s;
		for(int i = 0; i < branchlen; i++) {
			QString name = QString("%1%2,%3").arg(
				QString(prefix), 
				QString::number(branch),
				QString::number(i));
			cur = new Task(name.toStdString());
			cur->E = TaskList({f});
			*prev >> *cur;
			prev = cur;
		}
		*cur >> *f;
	}
}

int main (int argc, char **argv)
{
	QApplication app(argc, argv);
Controller c;
//	Task *s = new Task("s");
//	Task *f = new Task("f");
//	createFan(s, f, 'd');

//	vector<Card> cards;
//	cards.push_back(Card(4));
//	cards.push_back(Card(4));
//	cards.push_back(Card(4));
//	cards.push_back(Card(4));
//	SchedSim *sim = new SchedSim(cards, s);
//	sim->timeline.push_back(TaskList());
//	
//	Task *d;

//	sim->execTask(s, 0, 0);
//	for(int i = 0; i < s->next.size(); i++) {
//		d = s->next[i];
//		sim->execTask(d, 0, 0);
//	}

//	d = d->next[0];
//	cout << d->minDist << endl;

	return app.exec();
}
