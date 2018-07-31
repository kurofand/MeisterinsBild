#include "mysqlClient.hpp"

//#include "mysql_connection.h"
//#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <fstream>

MySQLClient::MySQLClient(const char* host, const char* userName, const char* pass, const char* dbName)
{
	this->host=host;
	this->userName=userName;
	this->password=pass;
	this->dbName=dbName;
}
//по неизвестной мне причине если пытаться загрузить значения из файла тут, а не в мейне, то они ломаются при выходе из функции
MySQLClient::MySQLClient(const char* fileName)
{
	std::ifstream ini;
	ini.open(fileName);
	std::string params[4], line;
	if(ini.is_open())
	{

		for(uint8_t i=0;i<4;i++)
			ini>>params[i];
		ini.close();
		this->host=params[0].c_str();
		this->userName=params[1].c_str();
		this->password=params[2].c_str();
		this->dbName=params[3].c_str();
	}
	else
		std::cout<<"Error with loading ini file!"<<std::endl;
}

bool MySQLClient::connect()
{
	this->driver=get_driver_instance();
	try{
//собираем строку для коннекта к заданной базе
		std::string buf(this->host);
		std::cout<<this->host<<std::endl;
		buf.append("/");
		buf.append(this->dbName);
		this->con=driver->connect(buf, this->userName, this->password);
		return true;
	}
	catch(const sql::SQLException &e){
		std::cout<<"Connection was not estabilished!:"<<std::endl<<e.what()<<std::endl;
		return false;
	}
}

void MySQLClient::executeQuery(const char* query, std::vector<std::string>& resVec)
{
	sql::Statement *stmt;
	sql::ResultSet *stmtRes;
	stmt=this->con->createStatement();
	std::string queryType;
	uint8_t i=0;
	while(query[i]!=*" ")
		queryType+=query[i++];
	if(queryType=="SELECT")
	{
		sql::ResultSet *stmtRes;
		stmtRes=stmt->executeQuery(query);
		while(stmtRes->next())
		{
			i=0;
			std::string rowStr;
			while(true)
				try{
					rowStr.append(stmtRes->getString(++i));
					rowStr.append("|");
				}
				catch(...){
					if(rowStr.back()==*"|")
						rowStr.pop_back();
					resVec.push_back(rowStr);
					break;
				}
		}
		delete stmtRes;
	}
	else
		stmt->execute(query);

	delete stmt;
}

void MySQLClient::closeConnection()
{
	this->con->close();
	if(this->con->isValid())
		std::cout<<"Close connection failed!"<<std::endl;
}

MySQLClient::~MySQLClient()
{
	delete this->con;
}
