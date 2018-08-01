#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <poppler/qt5/poppler-qt5.h>
#include <QDir>
#include <fstream>
#include <vector>

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	view=new QWebEngineView();
	connect(view, SIGNAL(loadStarted()), this, SLOT(onLoadStarted()));
	connect(view, SIGNAL(loadFinished(bool)), this, SLOT(onLoadingFinished(bool)));
	connect(view, SIGNAL(loadProgress(int)), this, SLOT(onLoadProgress(int)));
	connect(view->page(), SIGNAL(pdfPrintingFinished(QString,bool)), this, SLOT(onPdfPrintingFinished(QString,bool)));
	std::ifstream ini;
	QString path=QDir::currentPath()+"/ini/connect-test.ini";
	ini.open(path.toStdString());
	std::string host, usr, pwd, db;
	if(ini.is_open())
	{

		ini>>host;
		ini>>usr;
		ini>>pwd;
		ini>>db;
	}
	ini.close();
	client=new MySQLClient(host.c_str(), usr.c_str(), pwd.c_str(), db.c_str());
	if(client->connect())
	{
		std::vector <std::string> *vec=new std::vector<std::string>;
		client->executeQuery("SELECT url FROM scraping_sites", *vec);
		urlList=new QStringList();
		for(uint8_t i=0;i<vec->size();i++)
		{
			QString url=QString::fromStdString(vec->at(i));
			qDebug()<<url;
			if(url.indexOf(".pdf")==-1)
	//тут должен хватать значения с базы, но пока так
				//loadNext();
				//view->load(url);
				urlList->append(url);
			else
			{
				QString name, filename=QDir().currentPath();
				client->executeQuery("SELECT title FROM scraping_sites WHERE id="+QString::number(currUrlIndex+1).toLatin1(),*vec);
				name=QString::fromStdString(vec->at(0));
				filename=filename+name+".pdf";
				QFile file(filename);
				if(file.exists())
					filename=filename.replace(".pdf", "_new.pdf");
				system("wget "+url.toLatin1()+" -O "+filename.toLatin1());
				bool compare=this->compareFiles(name);
				if(compare)
					qDebug()<<"same";
				else
					qDebug()<<"different";
			}
		}
		//prepare directories fro pdfs
		view->load(QUrl(urlList->at(currUrlIndex++)));
		delete vec;
	}
}


void MainWindow::onLoadStarted()
{
	qDebug()<<"Start loading "+view->url().toString();
}

void MainWindow::onLoadingFinished(bool ok)
{
	if(ok)
	{
		//QString filename="/home/gin/pdfs/toMeCard/1.pdf";
		std::vector<std::string> *vec=new std::vector<std::string>;
		client->executeQuery("SELECT title FROM scraping_sites WHERE id="+QString::number(currUrlIndex).toLatin1(), *vec);
		QString name=QString::fromStdString(vec->at(0));
		QString filename=QDir().currentPath()+"/pdfs/"+name+".pdf";
		QFile file(filename);
		if(file.exists())
			filename=filename.replace(".pdf","_new.pdf");
		qDebug()<<filename;
		//view->page()->printToPdf(filename);
		if(currUrlIndex<urlList->size())
			view->load(QUrl(urlList->at(currUrlIndex++)));
		delete vec;
	}
}

void MainWindow::onLoadProgress(int progress)
{
	qDebug()<<progress;
}
bool MainWindow::compareFiles(QString currentTable)
{

	QString a((QDir().currentPath()+"/pdfs/"+currentTable+".pdf")), b((QDir().currentPath()+"/pdfs/"+currentTable+"_new.pdf"));
	Poppler::Document *c=Poppler::Document::load(a),
			*d=Poppler::Document::load(b);
	Poppler::Page *page1=c->page(0), *page2=d->page(0);
	uint8_t i=0;
	while((page1!=0)&&(page2!=0))
	{
		QImage img1=page1->renderToImage(), img2=page2->renderToImage();
		if(!(img1.isNull()&&img2.isNull()))
			if(img1!=img2)
				return false;
		page1=c->page(++i);
		page2=d->page(i);
	}

	delete c;
	delete d;
	return true;

}

void MainWindow::onPdfPrintingFinished(QString name, bool success)
{
	if((success)&&(name.indexOf("_new")!=-1))
	{
		std::vector<std::string> *vec=new std::vector<std::string>;
		client->executeQuery("SELECT title FROM scraping_sites WHERE id="+QString::number(currUrlIndex).toLatin1(), *vec);
		QString pdfName=QString::fromStdString(vec->at(0));

		bool compareRes=this->compareFiles(pdfName);
			//тут должны записывать итоги сравнения в базу
		if(compareRes)
			qDebug()<<"same";
		else
			qDebug()<<"different";
		delete vec;
	}
}

MainWindow::~MainWindow()
{
	client->closeConnection();
	delete urlList;
	delete client;
	delete view;
	delete ui;
}
