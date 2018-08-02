#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWebEngineView>
#include "mysqlClient.hpp"
#include "curl/curl.h"
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

private:
	bool compareFiles(QString currentTable);
	Ui::MainWindow *ui;
	QWebEngineView *view;
	MySQLClient *client;
	uint8_t currUrlIndex=0, recordsCount=0, proceed=0;
	QStringList *urlList;
	CURL *curl;
	int returnHttpCode(const char* url);
private slots:
	void onLoadingFinished(bool ok);
	void onPdfPrintingFinished(QString name, bool success);
	void onLoadStarted();
	void onLoadProgress(int progress);
	void onClose();
signals:
	void close();
};

#endif // MAINWINDOW_H
