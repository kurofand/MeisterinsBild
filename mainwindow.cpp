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
	qDebug()<<"start";
	view=new QWebEngineView();
	connect(view, SIGNAL(loadStarted()), this, SLOT(onLoadStarted()));
	connect(view, SIGNAL(loadFinished(bool)), this, SLOT(onLoadingFinished(bool)));
	connect(view, SIGNAL(loadProgress(int)), this, SLOT(onLoadProgress(int)));
	connect(view->page(), SIGNAL(pdfPrintingFinished(QString,bool)), this, SLOT(onPdfPrintingFinished(QString,bool)));
	connect(this, SIGNAL(close()), this, SLOT(onClose()));


	/*curl_global_init(CURL_GLOBAL_ALL);
	curl=curl_easy_init();*/


	std::ifstream ini;
	QString path=QDir::currentPath()+"/MeisterinsBild-Debug/ini/connect.ini";
	qDebug()<<path;
	ini.open(path.toStdString());
	std::string host, usr, pwd, db;
	if(ini.is_open())
	{

		ini>>host;
		ini>>usr;
		ini>>pwd;
		ini>>db;
	}
	else
	{
		qDebug()<<"ini was not found";

		qDebug()<<"searched at "+QDir::currentPath();
		emit close();
	}
	ini.close();
	client=new MySQLClient(host.c_str(), usr.c_str(), pwd.c_str(), db.c_str());
	qDebug()<<"connecting to "+QString::fromStdString(host)+" as "+QString::fromStdString(usr);
	if(client->connect())
	{
		std::vector <std::string> *vec=new std::vector<std::string>;
		client->executeQuery("SELECT url FROM scraping_sites", *vec);
		urlList=new QStringList();
		recordsCount=vec->size();
		for(uint8_t i=0;i<vec->size();i++)
		{
			QString url=QString::fromStdString(vec->at(i));
			qDebug()<<url;
			/*int responseCode=returnHttpCode(url.toLatin1());
			client->executeQuery("UPDATE scraping_sites SET http_status_code="+QString::number(responseCode).toLatin1()+" WHERE url=\""+url.toLatin1()+"\"", *vec);
			if(responseCode!=200)
				continue;*/
			if(url.indexOf(".pdf")==-1)
				urlList->append(url);
			else
			{
				QString name, filename=QDir().currentPath()+"/MeisterinsBild-Debug";
				std::vector<std::string> *res=new std::vector <std::string>;
				client->executeQuery("SELECT title FROM scraping_sites WHERE url=\""+url.toLatin1()+"\"",*res);
				name=QString::fromStdString(res->at(0));
				//name=name.replace(" ", "");
				filename=filename+"/pdfs/"+name+".pdf";
				QFile file(filename);
				bool exists=file.exists();
				if(exists)
				{
					filename.replace(".pdf", "_new.pdf");
					if(QFile(filename).exists())
					{
						//filename=filename.replace("_new","");
						QFile oldFile(filename.replace("_new",""));
						oldFile.remove();
						filename.replace(".pdf","_new.pdf");
						oldFile.setFileName(filename);

						oldFile.rename(filename.replace("_new",""));
						//oldFile.setFileName(filename.replace("_new",""));
						filename.replace(".pdf","_new.pdf");


					}

				}
				system("wget "+url.toLatin1()+" -O "+filename.toUtf8());
				if(exists)
				{
					bool compare=this->compareFiles(name.toUtf8());
					if(compare)
						qDebug()<<"same";
					else
						qDebug()<<"different";
				}
				emit close();
				delete res;
			}
		}
		//prepare directories fro pdfs
		view->load(QUrl(urlList->at(currUrlIndex++)));
		delete vec;
	}
	curl_easy_cleanup(curl);
	curl_global_cleanup();
}

int MainWindow::returnHttpCode(const char *url)
{
	int res=0;
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, *[](char* buf, size_t size, size_t nmemb, void* up)
	{
		return size*nmemb;
	});
	curl_easy_perform(curl);
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res);
	return res;
}

void MainWindow::onClose()
{
	proceed++;
	if(recordsCount==proceed)
	{
		qDebug()<<"all records are proceed. closing...";
		QApplication::quit();
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
		qDebug()<<"Loading finished";
		//QString filename="/home/gin/pdfs/toMeCard/1.pdf";
		std::vector<std::string> *vec=new std::vector<std::string>;
		client->executeQuery("SELECT title FROM scraping_sites WHERE id="+QString::number(currUrlIndex).toLatin1(), *vec);

		QString name=QString::fromStdString(vec->at(0));

		QString filename=QString(QDir().currentPath()+"/MeisterinsBild-Debug/pdfs/"+name+".pdf").toUtf8();

		QFile file(filename);
		qDebug()<<filename;
		if(file.exists())
		{
			filename.replace(".pdf","_new.pdf");
			if(QFile(filename).exists())
			{
				QFile oldFile(filename.replace("_new",""));
				oldFile.remove();

				filename.replace(".pdf","_new.pdf");
				oldFile.setFileName(filename);
				oldFile.rename(filename.replace("_new",""));
				filename.replace(".pdf","_new.pdf");
			}
		}

		qDebug()<<"Write pdf to "+filename;
		view->page()->printToPdf(filename);

		delete vec;
	}
}

void MainWindow::onLoadProgress(int progress)
{
	qDebug()<<progress;
}
bool MainWindow::compareFiles(QString currentTable)
{

	QString a((QDir().currentPath()+"/MeisteringBild-Debug/pdfs/"+currentTable+".pdf")), b((QDir().currentPath()+"/MeisteringBild-Debug/pdfs/"+currentTable+"_new.pdf"));
	Poppler::Document *c=Poppler::Document::load(a),
			*d=Poppler::Document::load(b);
	Poppler::Page *page1=c->page(0), *page2=d->page(0);
	uint8_t i=0;
	QDateTime time=QDateTime::currentDateTime();
	std::vector<std::string> *vec=new std::vector<std::string>;
	while((page1!=0)&&(page2!=0))
	{
		QImage img1=page1->renderToImage(), img2=page2->renderToImage();
		if(!(img1.isNull()&&img2.isNull()))
			if(img1!=img2)
			{
				qDebug()<<i;
				img1.save(QDir().currentPath()+"/MeisteringBild-Debug/pdfs/"+currentTable.toUtf8()+"1.png", "png");
				img2.save(QDir().currentPath()+"/MeisteringBild-Debug/pdfs/"+currentTable.toUtf8()+"2.png", "png");
				qDebug()<<"compare finished";


				client->executeQuery("UPDATE scraping_sites SET changed=1, http_status_code=200, accessed_at=\""+time.toString("yyyy-MM-dd hh:mm:ss").toLatin1()+"\", different_page_num="+QString::number(i+1).toLatin1()+" WHERE title=\""+currentTable.toUtf8()+"\"", *vec);
				delete vec;
				return false;
			}
		page1=c->page(++i);
		page2=d->page(i);
	}

	delete c;
	delete d;
	client->executeQuery("UPDATE scraping_sites SET changed=0, http_status_code=200, accessed_at=\""+time.toString("yyyy-MM-dd hh:mm:ss").toLatin1()+"\" WHERE title=\""+currentTable.toUtf8()+"\"", *vec);
	delete vec;
	qDebug()<<"compare finished";
	return true;

}

void MainWindow::onPdfPrintingFinished(QString name, bool success)
{
	if((success)&&(name.indexOf("_new")!=-1))
	{
		std::vector<std::string> *vec=new std::vector<std::string>;
		client->executeQuery("SELECT title FROM scraping_sites WHERE id="+QString::number(currUrlIndex).toLatin1(), *vec);
		QString pdfName=QString::fromStdString(vec->at(0));

		bool compareRes=this->compareFiles(pdfName.toUtf8());
			//тут должны записывать итоги сравнения в базу
		if(compareRes)
			qDebug()<<"same";
		else
		{
			qDebug()<<"different";
			std::vector <std::string> *res=new std::vector<std::string>;

			delete res;
		}
		delete vec;
	}
	qDebug()<<"saving finished";
	if(currUrlIndex<urlList->size())
		view->load(QUrl(urlList->at(currUrlIndex++)));
	emit close();
}

MainWindow::~MainWindow()
{
	qDebug()<<"enter to destructor";
	/*curl_easy_cleanup(curl);
	curl_global_cleanup();*/
	client->closeConnection();
	delete urlList;
	delete client;
	delete view;
	delete ui;
}
