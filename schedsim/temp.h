
#include <QtGui>
#include <vector>
#include <iostream>
#include "sim.h"

class CardsFrame : public QFrame
{
	Q_OBJECT

private:
	QPalette workerPalette;
	QSize workerMinSize;
	std::vector<QFrame *> cardFrames;
	
	void addWorkers(QFrame *card, int numWorkersH, int numWorkersV)
	{
		QGridLayout *layout = new QGridLayout;
		card->setLayout(layout);

		QLabel *worker;
		for(int i = 0; i < numWorkersV; i++)
			for(int j = 0; j < numWorkersH; j++) {
				worker = new QLabel(this);
				worker->setAutoFillBackground(true);
				worker->setPalette(workerPalette);
				worker->setMinimumSize(workerMinSize);
				worker->setMaximumSize(workerMinSize);
				worker->setFrameStyle(QFrame::Box || QFrame::Plain);
				worker->setLineWidth(1);
				layout->addWidget(worker, i, j);
			}
	}

	void setStatus(int cardIndex, int workerIndex, 
		bool running, const QString &taskName = QString())
	{
		QPalette pal;
		pal.setColor(QPalette::Background, Qt::green);

		QFrame *card = cardFrames[cardIndex];
		QGridLayout *cardLayout = dynamic_cast<QGridLayout *>(
			card->layout());
		int row = workerIndex / cardLayout->columnCount();
		int col = workerIndex % cardLayout->columnCount();
		QLabel *worker = dynamic_cast<QLabel *>(
			cardLayout->itemAtPosition(row, col)->widget());

		if(running) {
			worker->setPalette(pal);
			worker->setText(taskName);
		}
		else {
			worker->setPalette(workerPalette);
			worker->setText(QString());
		}
	}

public:

	CardsFrame(QWidget *parent = NULL) :
		QFrame(parent)
	{
		workerPalette = palette();
		workerPalette.setColor(QPalette::Background, Qt::white);
		workerPalette.setColor(QPalette::Foreground, Qt::gray);
		workerMinSize = QSize(50, 50);

		QHBoxLayout *layout = new QHBoxLayout;
		setLayout(layout);
	}

	void addCard(const QString &name, 
		int numWorkersH, int numWorkersV)
	{
		QWidget *col = new QWidget(this);
		QVBoxLayout *colLayout = new QVBoxLayout;
		col->setLayout(colLayout);
		
		colLayout->addWidget(new QLabel(name, this));
		
		QFrame *card = new QFrame(this);
		card->setFrameStyle(QFrame::Box || QFrame::Plain);
		card->setLineWidth(2);
		colLayout->addWidget(card);
		addWorkers(card, numWorkersH, numWorkersV);

		layout()->addWidget(col);
		cardFrames.push_back(card);
	}

	void setRunning(int cardIndex, int workerIndex, 
		const QString &taskName)
	{
		setStatus(cardIndex, workerIndex, true, taskName);
	}

	void setInactive(int cardIndex, int workerIndex)
	{
		setStatus(cardIndex, workerIndex, false);
	}

	void resetWorkers()
	{
		for(int card = 0; card < cardFrames.size(); card++) {
			QGridLayout *curLayout = dynamic_cast<QGridLayout *>(
				cardFrames[card]->layout());
			for(int i = 0; i < curLayout->rowCount(); i++)
				for(int j = 0; j < curLayout->columnCount(); j++)
					setInactive(card, i * curLayout->columnCount() + j);
		}
	}

};


class MainWindow : public QMainWindow
{
	Q_OBJECT

private:
	CardsFrame *content;
	SchedSim &sim;

	void init()
	{
		int idx = 0;
		for(auto iter = sim.cards.begin(); iter != sim.cards.end();
			iter++)
		{
			int rows = (int) ceil(sqrt(iter->workerCount));
			int cols = (int) ceil((float) iter->workerCount / (float) rows);
			content->addCard(QString("card %1").arg(idx), rows, cols);
		}
	}

public slots:
	
	void resetSim()
	{
		sim.reset();
	}

	bool step()
	{
		bool ret = sim.step();
		std::cout << "here" << endl;

		// display all active workers
		content->resetWorkers();
		TaskList &snapshot = *sim.timeline.rbegin();
		for(auto cur = snapshot.begin(); cur != snapshot.end();
			cur++) {
			content->setRunning((*cur)->cardIndex, (*cur)->workerIndex, 
				QString((*cur)->name.c_str()));
		}

		repaint();
		return ret;
	}

public:

	MainWindow(SchedSim &sim, QWidget *parent = NULL) :
		QMainWindow(parent),
		sim(sim)
	{
		content = new CardsFrame(this);
		setCentralWidget(content);
		init();
	}

	void addCard(const QString &name, 
		int numWorkersH, int numWorkersV)
	{
		content->addCard(name, numWorkersH, numWorkersV);
	}

	void setRunning(int cardIndex, int workerIndex, 
		const QString &taskName)
	{
		content->setRunning(cardIndex, workerIndex, taskName);
	}

	void setInactive(int cardIndex, int workerIndex)
	{
		content->setInactive(cardIndex, workerIndex);
	}
};


class Controller : public QObject
{
	Q_OBJECT

public:

	Controller();
	virtual ~Controller() {}

private:
	MainWindow *win;
	QTimer *timer;
	SchedSim *sim;

	Task *createGraph();
	void createLoop(Task *s, Task *f, char prefix);
	void createFan(Task *s, Task *f, char prefix);

private slots:
	void update();


};



