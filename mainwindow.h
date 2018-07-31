#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWebEngineView>
#include "mysqlClient.hpp"
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
private slots:
	void onLoadingFinished(bool ok);
	void onPdfPrintingFinished(QString name, bool success);
};

#endif // MAINWINDOW_H
