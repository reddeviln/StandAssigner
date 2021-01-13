#pragma once
#include "pch.h"
#include "StandAssigner2.h"
#include <string>
#include <ctime>
#include <algorithm>
#include <regex>
#include "resource.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include "loguru.cpp"
#include <unordered_map>
#include <utility>





#define MY_PLUGIN_NAME      "Stand Assigner for VATSIM"
#define MY_PLUGIN_VERSION   "1.0"
#define MY_PLUGIN_DEVELOPER "Nils Dornbusch"
#define MY_PLUGIN_COPYRIGHT "Free to be distributed as source code"
#define MY_PLUGIN_VIEW      ""


const	int		TAG_ITEM_STAND = 15278;
std::unordered_map<std::string, Stand> data;
std::unordered_map<std::string, Stand> standmapping;

const   int     TAG_FUNC_EDIT = 15;
const int TAG_FUNC_ASSIGN_POPUP = 16;
const int TAG_FUNC_MANUAL_FINISH = 22;
const int TAG_FUNC_CLEAR = 264;
const double TOL = 0.02;


//---CGMPHelper------------------------------------------

CStandAssigner::CStandAssigner(void)
	: CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE,
		MY_PLUGIN_NAME,
		MY_PLUGIN_VERSION,
		MY_PLUGIN_DEVELOPER,
		MY_PLUGIN_COPYRIGHT)
{
	char path[MAX_PATH];
	HMODULE hm = NULL;

	if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
		GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPCSTR)&CStandAssigner::test, &hm) == 0)
	{
		int ret = GetLastError();
		fprintf(stderr, "GetModuleHandle failed, error = %d\n", ret);
		// Return or however you want to handle an error.
	}
	if (GetModuleFileName(hm, path, sizeof(path)) == 0)
	{
		int ret = GetLastError();
		fprintf(stderr, "GetModuleFileName failed, error = %d\n", ret);
		// Return or however you want to handle an error.
	}
	std::string dir(path);
	std::string filename("StandAssigner.dll");
	size_t pos = dir.find(filename);
	dir.replace(pos, filename.length(), "");
	loguru::add_file("StandAssigner.log", loguru::Append, loguru::Verbosity_INFO);
	char logpath[1024];
	loguru::set_thread_name("StandAssigner");
	loguru::suggest_log_path(dir.c_str(), logpath, sizeof(logpath));
	loguru::add_file(logpath, loguru::FileMode::Truncate, loguru::Verbosity_INFO);
	LOG_F(INFO, "We successfully started StandAssigner. Opa!");

	RegisterTagItemType("Stand", TAG_ITEM_STAND);

	RegisterTagItemFunction("Edit Stand", TAG_FUNC_EDIT);

	RegisterTagItemFunction("Assign Stand", TAG_FUNC_ASSIGN_POPUP);

	RegisterTagItemFunction("Clear", TAG_FUNC_CLEAR);


	LOG_F(INFO, "Everything registered. Ready to go!");


	dir += "OMDB.csv";
	io::CSVReader<8, io::trim_chars<' '>, io::no_quote_escape<','>> in(dir);
	in.read_header(io::ignore_extra_column, "Standnumber", "latitude", "longitude", "airlinecode", "neighbor1", "neighbor2","size","flytampa");
	std::string StandNumber, lat, Long, airlinecode, neighbor1, neighbor2, size, flytampa;
	while (in.read_row(StandNumber, lat, Long, airlinecode, neighbor1, neighbor2, size, flytampa))
	{
		Stand temp =  Stand(StandNumber, lat, Long, airlinecode, neighbor1, neighbor2, size, flytampa);
		std::pair<std::string,Stand> temp2(StandNumber, temp);
		data.insert(temp2);
	}
	LOG_F(INFO, "Stand file read without issues!");
}




CStandAssigner::~CStandAssigner(void)
{
}
std::string getStandOfAircraft(EuroScopePlugIn::CPosition position)
{
	for (auto stand : data)
	{
		auto distance = position.DistanceTo(stand.second.position);
		if (distance < TOL)
			return stand.first;
	}
	return "-1";
}

void CStandAssigner::OnGetTagItem(EuroScopePlugIn::CFlightPlan FlightPlan,
	EuroScopePlugIn::CRadarTarget RadarTarget,
	int ItemCode,
	int TagData,
	char sItemString[16],
	int * pColorCode,
	COLORREF * pRGB,
	double * pFontSize)
{
	if (!FlightPlan.IsValid())
		return;
	switch (ItemCode)
	{
	case TAG_ITEM_STAND:
		
		auto found = standmapping.find(FlightPlan.GetCallsign());
		if (found != standmapping.end())
		{
			auto temp = found->second.number;
			strcpy(sItemString, temp.c_str());
		}
				
	}
		
}
bool CStandAssigner::OnCompileCommand(const char * sCommandLine)
{
	if (std::strcmp(sCommandLine, ".showstands") == 0)
	{
		std::string out;
		for (auto it : data)
		{
			if (!it.second.isEmpty || it.second.isAssigned)
				out += it.first;
		}
		DisplayUserMessage("Occupied Stands", "OMDB", out.c_str() , true, true, true, true, true);
		return true;
	}

}
void CStandAssigner::OnRadarTargetPositionUpdate(EuroScopePlugIn::CRadarTarget rt)
{
	if (rt.IsValid())
	{
		auto position = rt.GetPosition().GetPosition();
		auto stand = getStandOfAircraft(position);
		if (stand != "-1") {
			data.at(stand).isEmpty = false;
		}
	}
}


void  CStandAssigner::OnTimer(int Counter)
{
	if (Counter % 10 == 0)
	{
		cleanupStands();
	}
}


inline void CStandAssigner::OnFunctionCall(int FunctionId, const char * sItemString, POINT Pt, RECT Area)
{
	//handle our registered functions
	EuroScopePlugIn::CFlightPlan  fp;
	CString                         str;


	// get the flightplan we are dealing with
	fp = FlightPlanSelectASEL();
	if (!fp.IsValid())
		return;

	// select it from the sequence

	// switch by the function ID
	switch (FunctionId)
	{
	case TAG_FUNC_EDIT:
		OpenPopupEdit(Area,
			TAG_FUNC_MANUAL_FINISH,
			"");
		break;
	case TAG_FUNC_MANUAL_FINISH:
	{
		std::string input = sItemString;
		try {
			data.at(input).isAssigned = true;
			std::pair<std::string, Stand> temp2(fp.GetCallsign(), data.at(input));
			standmapping.insert(temp2);
			break;
		}
		catch (std::exception e)
		{
			break;
		}
	}
	case TAG_FUNC_CLEAR:
	{
		auto found = standmapping.find(fp.GetCallsign());
		if (found != standmapping.end())
		{
			auto temp2 = data.find(found->second.number);
			if (temp2 != data.end())
			{
				temp2->second.isAssigned = false;
			}
			standmapping.erase(found);
			
		}
	}
	}// switch by the function ID
}
void CStandAssigner::cleanupStands()
{
	for (auto& it : data)
	{
		
		if (!it.second.isEmpty)
		{
			auto first = RadarTargetSelectFirst();
			if (first.GetPosition().GetPosition().DistanceTo(it.second.position) < TOL)
				goto outer;
			auto temp = RadarTargetSelectNext(first);
			while (temp.IsValid())
			{
				if (temp.GetPosition().GetPosition().DistanceTo(it.second.position) < TOL)
				{
					goto outer;
				}
					
				temp = RadarTargetSelectNext(temp);
			}
			it.second.isEmpty = true;
		}
	outer:
		continue;
	}
 }





