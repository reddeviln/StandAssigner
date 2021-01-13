#pragma once
#pragma warning(disable : 4996)
#include "EuroScopePlugIn.h"
#include "pch.h"
#include <vector>
#include "csv.h"
#include "loguru.hpp"
#define INT_PART 4
#define DEC_PART 3


class Stand
{
	//this class stores all information regarding one particular stand
public:
	std::string number;
	double mlat, mLong;
	EuroScopePlugIn::CPosition position;
	std::string mAirlinecode;
	std::string mNeighbor1;
	std::string mNeighbor2;
	bool isEmpty = true; 
	bool isAssigned = false;
	char mSize;
	bool isInFlytampa = false;
	Stand(std::string StandNumber, std::string lat, std::string Long, std::string airlinecode, std::string neighbor1, std::string neighbor2, std::string size, std::string flytampa)
	{
		number = StandNumber;
		mlat = Str2LatLong(lat);
		mLong = Str2LatLong(Long);
		position = EuroScopePlugIn::CPosition();
		position.m_Latitude = mlat;
		position.m_Longitude = mLong;
		mNeighbor1 = neighbor1;
		mNeighbor2 = neighbor2;
		
		mSize = size.at(0);
		mAirlinecode = airlinecode;
		if (flytampa == "yes")
		{
			isInFlytampa = true;
		}
		else if (flytampa == "no")
		{
			isInFlytampa = false;
		}
	}

	double Str2LatLong(std::string coord)

	{
		std::vector<double> splitted;
		std::string delimiter = "-";
		size_t pos = 0;
		std::string token;
		while ((pos = coord.find(delimiter)) != std::string::npos) {
			token = coord.substr(0, pos);
			switch (token.at(0))
			{
			case 'N':
			case 'E':
				token = token.substr(1, token.length() - 1);
				splitted.push_back(std::stod(token));
				break;
			case 'S':
			case 'W':
				token = token.substr(1, token.length() - 1);
				splitted.push_back(std::stod(token)*-1);
			default:
				splitted.push_back(std::stod(token));
			}
			coord.erase(0, pos + delimiter.length());
		}
		splitted.push_back(std::stod(coord));
		return splitted[0] + splitted[1] / 60 + splitted[2] / 3600;
	}

};
class CStandAssigner:
	//The class that holds all our functions 
	public EuroScopePlugIn::CPlugIn
{
public:
	//the list displayed in euroscope

	

	CStandAssigner(void);



	virtual ~CStandAssigner(void);


	/*
	This function overrides a Euroscope function. If you type ".showtolist" in the euroscope textbox it will show the t/o sequence list
	Input: sCommandLine (the textbox string)
	*/
	static bool test()
	{
		return true;
	}

	bool fileExists(const std::string& filename)
	{
		struct stat buf;
		if (stat(filename.c_str(), &buf) != -1)
		{
			return true;
		}
		return false;
	}




	virtual void  OnGetTagItem(EuroScopePlugIn::CFlightPlan FlightPlan,
		EuroScopePlugIn::CRadarTarget RadarTarget,
		int ItemCode,
		int TagData,
		char sItemString[16],
		int * pColorCode,
		COLORREF * pRGB,
		double * pFontSize);
	/*This function overrides a Euroscope function. It determines what value to show for our new TAG items "CTOT" and "TOBT" and is called automatically by euroscope every few seconds
	  detailed info on the input and output values can be found in the EuroScopePlugIn.h header
	*/



	virtual void    OnFunctionCall(int FunctionId,
		const char * sItemString,
		POINT Pt,
		RECT Area);
	inline  virtual void    OnRadarTargetPositionUpdate(EuroScopePlugIn::CRadarTarget RadarTarget);


	inline  virtual bool    OnCompileCommand(const char * sCommandLine);
	inline  virtual void    OnTimer(int Counter);
	void cleanupStands();

};


