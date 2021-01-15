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
#include <random>





#define MY_PLUGIN_NAME      "Stand Assigner for VATSIM"
#define MY_PLUGIN_VERSION   "1.0"
#define MY_PLUGIN_DEVELOPER "Nils Dornbusch"
#define MY_PLUGIN_COPYRIGHT "Free to be distributed as source code"
#define MY_PLUGIN_VIEW      ""


const	int		TAG_ITEM_STAND = 15278;
std::unordered_map<std::string, Stand> data;
std::unordered_map<std::string, Stand> standmapping;
std::vector<Stand> standsUAE;
std::vector<Stand> standsPAX;
std::vector<Stand> standsCARGO;
std::vector<Stand> standsLOWCOST;
std::vector<Stand> standsVIP;
std::vector<Stand> standsGA;
std::vector<Stand> standsOverflow;

const   int     TAG_FUNC_EDIT = 15;
const int TAG_FUNC_ASSIGN_POPUP = 16;
const int TAG_FUNC_ASSIGN_AUTO = 17;
const int TAG_FUNC_ASSIGN_CARGO = 18;
const int TAG_FUNC_ASSIGN_PAX = 19;
const int TAG_FUNC_ASSIGN_UAE = 31854;
const int TAG_FUNC_ASSIGN_LOWCOST = 20;
const int TAG_FUNC_ASSIGN_VIP = 21;
const int TAG_FUNC_ASSIGN_GA = 23;
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
	for (auto stand : data)
	{
		auto code = stand.second.mAirlinecode;
		if (code == "UAE")
		{
			standsUAE.push_back(stand.second);
			continue;
		}
		if (code == "PAX")
		{
			standsPAX.push_back(stand.second);
			continue;
		}
			
		if (code == "CARGO" || code == "CLC")
			standsCARGO.push_back(stand.second);
		if (code == "LWC" || code == "CLC")
			standsLOWCOST.push_back(stand.second);
		if (code == "GA")
		{
			standsGA.push_back(stand.second);
			continue;
		}
		if (code == "VIP")
		{
			standsVIP.push_back(stand.second);
			continue;
		}
		if (code == "ALL")
		{
			standsOverflow.push_back(stand.second);
			continue;
		}
			

	}
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
		else
		{
			auto fpdata = FlightPlan.GetFlightPlanData();
			std::string remarks = fpdata.GetRemarks();
			std::regex re = std::regex(R"(\/STAND([A-Z]\d{1,2}))");
			std::smatch match;
			if (std::regex_search(remarks, match, re))
			{
				std::string stand = match.str(1);
				auto found = data.find(stand);
				if ( found != data.end())
				{
					found->second.isAssigned = true;
					std::pair<std::string, Stand> temp(FlightPlan.GetCallsign(), found->second);
					standmapping.insert(temp);
				}
			}
				
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
			auto mystand = data.at(stand);
			auto code = data.at(stand).mAirlinecode;
			if (code == "UAE")
			{
				for (auto &temp : standsUAE)
				{
					if (temp.number == mystand.number)
						temp.isEmpty = false;
				}
			}
			if (code == "PAX")
			{
				for (auto &temp : standsPAX)
				{
					if (temp.number == mystand.number)
						temp.isEmpty = false;
				}
			}

			if (code == "CARGO" || code == "CLC")
			{
				for (auto &temp : standsCARGO)
				{
					if (temp.number == mystand.number)
						temp.isEmpty = false;
				}
			}

			if (code == "LWC" || code == "CLC")
			{
				for (auto &temp : standsLOWCOST)
				{
					if (temp.number == mystand.number)
						temp.isEmpty = false;
				}
			}
			if (code == "GA")
			{
				for (auto &temp : standsGA)
				{
					if (temp.number == mystand.number)
						temp.isEmpty = false;
				}
			}
			if (code == "VIP")
			{
				for (auto &temp : standsVIP)
				{
					if (temp.number == mystand.number)
						temp.isEmpty = false;
				}
			}
			if (code == "ALL")
			{
				for (auto &temp : standsOverflow)
				{
					if (temp.number == mystand.number)
						temp.isEmpty = false;
				}
			}
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
			std::string code = data.at(input).mAirlinecode;
			auto it = data.at(input);
			if (code == "UAE")
			{
				for (auto &temp : standsUAE)
				{
					if (temp.number == it.number)
						temp.isAssigned = true;
				}
			}
			if (code == "PAX")
			{
				for (auto &temp : standsPAX)
				{
					if (temp.number == it.number)
						temp.isAssigned = true;
				}
			}

			if (code == "CARGO" || code == "CLC")
			{
				for (auto &temp : standsCARGO)
				{
					if (temp.number == it.number)
						temp.isAssigned = true;
				}
			}

			if (code == "LWC" || code == "CLC")

			{
				for (auto &temp : standsLOWCOST)
				{
					if (temp.number == it.number)
						temp.isAssigned = true;
				}
			}
			if (code == "GA")
			{
				for (auto &temp : standsGA)
				{
					if (temp.number == it.number)
						temp.isAssigned = true;
				}
			}
			if (code == "VIP")
			{
				for (auto &temp : standsVIP)
				{
					if (temp.number == it.number)
						temp.isAssigned = true;
				}
			}
			if (code == "ALL")
			{
				for (auto &temp : standsOverflow)
				{
					if (temp.number == it.number)
						temp.isAssigned = true;
				}
			}
			std::pair<std::string, Stand> temp2(fp.GetCallsign(), data.at(input));
			standmapping.insert(temp2);
			auto fpdata = fp.GetFlightPlanData();
			std::string remarks = fpdata.GetRemarks();
			std::smatch match;
			if(std::regex_search(remarks,match, std::regex(R"(\/STAND[A-Z]\d{1,2})")))
				return;
			remarks += "/STAND" + input;
			fpdata.SetRemarks(remarks.c_str());
			fpdata.AmendFlightPlan();
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
			auto fpdata = fp.GetFlightPlanData();
			standmapping.erase(found);
			std::string remarks = fpdata.GetRemarks();
			remarks = std::regex_replace(remarks, std::regex(R"(\/STAND[A-Z]\d{1,2})"), "");
			fpdata.SetRemarks(remarks.c_str());
			fpdata.AmendFlightPlan();
			
		}
		break;
	}
	case TAG_FUNC_ASSIGN_POPUP:
	{
		OpenPopupList(Area, "Assign Stand", 1);


		//AddPopupListElement("Assign Auto", "", TAG_FUNC_ASSIGN_AUTO);
		AddPopupListElement("Assign UAE", "", TAG_FUNC_ASSIGN_UAE);
		AddPopupListElement("Assign CARGO", "", TAG_FUNC_ASSIGN_CARGO);
		AddPopupListElement("Assign PAX", "", TAG_FUNC_ASSIGN_PAX);
		AddPopupListElement("Assign LOWCOST", "", TAG_FUNC_ASSIGN_LOWCOST);
		AddPopupListElement("Assign VIP", "", TAG_FUNC_ASSIGN_VIP);
		AddPopupListElement("Assign GA", "", TAG_FUNC_ASSIGN_GA);
		AddPopupListElement("Clear", "", TAG_FUNC_CLEAR);
		break;
	}
	case TAG_FUNC_ASSIGN_AUTO:
	{
		break;
	}
	case TAG_FUNC_ASSIGN_CARGO:
	{
		auto found = standmapping.find(fp.GetCallsign());
		if (found != standmapping.end())
			break;
		auto size = determineAircraftCat(fp);
		auto stand = extractRandomStand(standsCARGO, size);
		if (stand.number == "Z00")
			break;
		data.at(stand.number).isAssigned = true;
		std::pair<std::string, Stand> temp(fp.GetCallsign(), stand);
		standmapping.insert(temp);
		for (auto &temp : standsCARGO)
		{
			if (temp.number == stand.number)
				temp.isAssigned = true;
		}
		for (auto &temp : standsLOWCOST)
		{
			if (temp.number == stand.number)
				temp.isAssigned = true;
		}
		auto fpdata = fp.GetFlightPlanData();
		std::string remarks = fpdata.GetRemarks();
		std::smatch match;
		if (std::regex_search(remarks, match, std::regex(R"(\/STAND[A-Z]\d{1,2})")))
			return;
		remarks += "/STAND" + stand.number;
		fpdata.SetRemarks(remarks.c_str());
		fpdata.AmendFlightPlan();
		break;
	}
	case TAG_FUNC_ASSIGN_PAX:
	{
		auto found = standmapping.find(fp.GetCallsign());
		if (found != standmapping.end())
			break;
		auto size = determineAircraftCat(fp);
		auto stand = extractRandomStand(standsPAX, size);
		if (stand.number == "Z00")
			break;
		data.at(stand.number).isAssigned = true;
		std::pair<std::string, Stand> temp(fp.GetCallsign(), stand);
		standmapping.insert(temp);
		for (auto &temp : standsPAX)
		{
			if (temp.number == stand.number)
				temp.isAssigned = true;
		}
		auto fpdata = fp.GetFlightPlanData();
		std::string remarks = fpdata.GetRemarks();
		std::smatch match;
		if (std::regex_search(remarks, match, std::regex(R"(\/STAND[A-Z]\d{1,2})")))
			return;
		remarks += "/STAND" + stand.number;
		fpdata.SetRemarks(remarks.c_str());
		fpdata.AmendFlightPlan();
		break;
	}
	case TAG_FUNC_ASSIGN_UAE:
	{
		auto found = standmapping.find(fp.GetCallsign());
		if (found != standmapping.end())
			break;
		auto size = determineAircraftCat(fp);
		auto stand = extractRandomStand(standsUAE, size);
		if (stand.number == "Z00")
			break;
		data.at(stand.number).isAssigned = true;
		std::pair<std::string, Stand> temp(fp.GetCallsign(), stand);
		standmapping.insert(temp);
		for (auto &temp : standsUAE)
		{
			if (temp.number == stand.number)
				temp.isAssigned = true;
		}
		auto fpdata = fp.GetFlightPlanData();
		std::string remarks = fpdata.GetRemarks();
		std::smatch match;
		if (std::regex_search(remarks, match, std::regex(R"(\/STAND[A-Z]\d{1,2})")))
			return;
		remarks += "/STAND" + stand.number;
		fpdata.SetRemarks(remarks.c_str());
		fpdata.AmendFlightPlan();
		break;
	}
	case TAG_FUNC_ASSIGN_VIP:
	{
		auto found = standmapping.find(fp.GetCallsign());
		if (found != standmapping.end())
			break;
		auto size = determineAircraftCat(fp);
		auto stand = extractRandomStand(standsVIP, size);
		if (stand.number == "Z00")
			break;
		data.at(stand.number).isAssigned = true;
		std::pair<std::string, Stand> temp(fp.GetCallsign(), stand);
		standmapping.insert(temp);
		for (auto &temp : standsVIP)
		{
			if (temp.number == stand.number)
				temp.isAssigned = true;
		}
		auto fpdata = fp.GetFlightPlanData();
		std::string remarks = fpdata.GetRemarks();
		std::smatch match;
		if (std::regex_search(remarks, match, std::regex(R"(\/STAND[A-Z]\d{1,2})")))
			return;
		remarks += "/STAND" + stand.number;
		fpdata.SetRemarks(remarks.c_str());
		fpdata.AmendFlightPlan();
		break;
	}
	case TAG_FUNC_ASSIGN_LOWCOST:
	{
		auto found = standmapping.find(fp.GetCallsign());
		if (found != standmapping.end())
			break;
		auto size = determineAircraftCat(fp);
		auto stand = extractRandomStand(standsLOWCOST, size);
		if (stand.number == "Z00")
			break;
		data.at(stand.number).isAssigned = true;
		std::pair<std::string, Stand> temp(fp.GetCallsign(), stand);
		standmapping.insert(temp);
		for (auto &temp : standsCARGO)
		{
			if (temp.number == stand.number)
				temp.isAssigned = true;
		}
		for (auto &temp : standsLOWCOST)
		{
			if (temp.number == stand.number)
				temp.isAssigned = true;
		}
		auto fpdata = fp.GetFlightPlanData();
		std::string remarks = fpdata.GetRemarks();
		std::smatch match;
		if (std::regex_search(remarks, match, std::regex(R"(\/STAND[A-Z]\d{1,2})")))
			return;
		remarks += "/STAND" + stand.number;
		fpdata.SetRemarks(remarks.c_str());
		fpdata.AmendFlightPlan();
		break;
	}
	case TAG_FUNC_ASSIGN_GA:
	{
		auto found = standmapping.find(fp.GetCallsign());
		if (found != standmapping.end())
			break;
		auto size = determineAircraftCat(fp);
		auto stand = extractRandomStand(standsGA, size);
		if (stand.number == "Z00")
			break;
		data.at(stand.number).isAssigned = true;
		std::pair<std::string, Stand> temp(fp.GetCallsign(), stand);
		standmapping.insert(temp);
		for (auto &temp : standsGA)
		{
			if (temp.number == stand.number)
				temp.isAssigned = true;
		}
		auto fpdata = fp.GetFlightPlanData();
		std::string remarks = fpdata.GetRemarks();
		std::smatch match;
		if (std::regex_search(remarks, match, std::regex(R"(\/STAND[A-Z]\d{1,2})")))
			return;
		remarks += "/STAND" + stand.number;
		fpdata.SetRemarks(remarks.c_str());
		fpdata.AmendFlightPlan();
		break;
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
		if (it.second.isAssigned)
		{
			auto first = FlightPlanSelectFirst();
			std::string cs = first.GetCallsign();
			auto found = standmapping.find(cs);
			if (found != standmapping.end())
				if(it.second.number == found->second.number)
					goto outer;
			auto temp = FlightPlanSelectNext(first);
			while (temp.IsValid())
			{
				cs = temp.GetCallsign();
				auto found = standmapping.find(cs);
				if (found != standmapping.end())
					if (it.second.number == found->second.number)
						goto outer;
				temp = FlightPlanSelectNext(temp);
			}
			it.second.isAssigned = false;
			std::string code = it.second.mAirlinecode;
			if (code == "UAE")
			{
				for (auto &temp : standsUAE)
				{
					if (temp.number == it.second.number)
						temp.isAssigned = false;
				}
			}
			if (code == "PAX")
			{
				for (auto &temp : standsPAX)
				{
					if (temp.number == it.second.number)
						temp.isAssigned = false;
				}
			}

			if (code == "CARGO" || code == "CLC")
			{
				for (auto &temp : standsCARGO)
				{
					if (temp.number == it.second.number)
						temp.isAssigned = false;
				}
			}

			if (code == "LWC" || code == "CLC")

			{
				for (auto &temp : standsLOWCOST)
				{
					if (temp.number == it.second.number)
						temp.isAssigned = false;
				}
			}
			if (code == "GA")
			{
				for (auto &temp : standsGA)
				{
					if (temp.number == it.second.number)
						temp.isAssigned = false;
				}
			}
			if (code == "VIP")
			{
				for (auto &temp : standsVIP)
				{
					if (temp.number == it.second.number)
						temp.isAssigned = false;
				}
			}
			if (code == "ALL")
			{
				for (auto &temp : standsOverflow)
				{
					if (temp.number == it.second.number)
						temp.isAssigned = false;
				}
			}
			
		}
	outer:
		continue;
	}
	
	
 }
Stand CStandAssigner::extractRandomStand(std::vector<Stand> stands, char size)
{
	
	std::shuffle(std::begin(stands), std::end(stands), std::random_device());
	for (auto stand : stands)
	{
		if (!stand.isAssigned && stand.isEmpty && stand.isInFlytampa && stand.mSize >= size)
		{
			return stand;
		}

	}
	std::shuffle(std::begin(standsOverflow), std::end(standsOverflow), std::random_device());
	for (auto stand : standsOverflow)
	{
		if (!stand.isAssigned && stand.isEmpty && stand.isInFlytampa && stand.mSize >= size)
		{
			return stand;
		}
	}
	DisplayUserMessage("StandAssigner", "OMDB", "Error assigning stand", true, true, true, true, true);
	auto errorval = standsOverflow.at(0);
	errorval.number = "Z00";
	return errorval;
}
char CStandAssigner::determineAircraftCat(EuroScopePlugIn::CFlightPlan fp)
{
	char wtc = fp.GetFlightPlanData().GetAircraftWtc();
	std::string actype = fp.GetFlightPlanData().GetAircraftFPType();
	if (wtc == 'L')
		return 'B';
	if (wtc == 'J')
		return 'F';
	if (wtc == 'H')
		return 'E';
	if (wtc == 'M')
		return 'C';
}





