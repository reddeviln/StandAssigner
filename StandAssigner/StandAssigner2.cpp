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
#include <filesystem>





#define MY_PLUGIN_NAME      "Stand Assigner for VATSIM"
#define MY_PLUGIN_VERSION   "1.2"
#define MY_PLUGIN_DEVELOPER "Nils Dornbusch"
#define MY_PLUGIN_COPYRIGHT "Free to be distributed as source code"
#define MY_PLUGIN_VIEW      ""


const	int		TAG_ITEM_STAND = 1548915;
std::unordered_map<std::string,std::unordered_map<std::string, Stand>> data;
std::unordered_map<std::string, std::unordered_map<std::string, Stand>> standmapping;
std::unordered_map<std::string, std::unordered_map<std::string, std::string>> callsignmap;
std::unordered_map<std::string,std::vector<Stand>> standsUAE;
std::unordered_map<std::string, std::vector<Stand>> standsPAX;
std::unordered_map<std::string, std::vector<Stand>> standsCARGO;
std::unordered_map<std::string, std::vector<Stand>> standsLOWCOST;
std::unordered_map<std::string, std::vector<Stand>> standsVIP;
std::unordered_map<std::string, std::vector<Stand>> standsGA;
std::unordered_map<std::string, std::vector<Stand>> standsOverflow;
std::unordered_map<std::string, std::vector<Stand>> standsABY;
std::unordered_map<std::string, std::vector<Stand>> standsCargoSpecial;
std::vector<std::string> activeAirports;
const   int     TAG_FUNC_EDIT = 423;
const int TAG_FUNC_ASSIGN_POPUP = 456456;
const int TAG_FUNC_ASSIGN_AUTO = 412;
const int TAG_FUNC_ASSIGN_CARGO = 4578;
const int TAG_FUNC_ASSIGN_PAX = 456;
const int TAG_FUNC_ASSIGN_UAE = 31854;
const int TAG_FUNC_ASSIGN_LOWCOST = 3458;
const int TAG_FUNC_ASSIGN_VIP = 4868;
const int TAG_FUNC_ASSIGN_GA = 486;
const int TAG_FUNC_ASSIGN_ABY = 2342;
const int TAG_FUNC_ASSIGN_CARGO2 = 23230;
const int TAG_FUNC_MANUAL_FINISH = 2345;
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
	std::string dir2;
	//dir2 = dir + "CallsignMap.csv";
	//dir += "OMDB.csv";
	std::regex icao(R"(.*\\([A-Z]{4}))");
	for (auto entry : std::filesystem::directory_iterator(dir))
	{
		if (entry.is_directory())
		{
			std::smatch m;
			std::string name = entry.path().string();
			if (std::regex_search(name, m, icao))
			{
				std::string temp = "/";
				auto temp2 = temp;
				temp += m.str(1);
				temp += ".csv";
				temp2 += "CallsignMap";
				temp2 += m.str(1);
				temp2 += ".csv";
				auto standpath = std::filesystem::path(name + temp);
				auto callsignpath = std::filesystem::path(name + temp2);
				if (std::filesystem::exists(standpath) && std::filesystem::exists(callsignpath))
				{
					readStandFile(standpath.string(), m.str(1));
					readCallsignFile(callsignpath.string(), m.str(1));
					activeAirports.push_back(m.str(1));
				}
				else
				{
					std::string logstring;
					logstring = "Not all required csv files found for ";
					logstring += m[0];
					logstring += ". Skipping this airport ...";
					LOG_F(INFO, logstring.c_str());
				}
			}
		}
	}
	LOG_F(INFO, "Done reading csv files.");

	for (auto airport : activeAirports)
	{
		auto found = data.find(airport);
		if (found == data.end()) break;
		std::vector<Stand> thisstandsUAE;
		std::vector<Stand> thisstandsABY;
		std::vector<Stand> thisstandsPAX;
		std::vector<Stand> thisstandsCARGO;
		std::vector<Stand> thisstandsCARGOspec;
		std::vector<Stand> thisstandsLOWCOST;
		std::vector<Stand> thisstandsGA;
		std::vector<Stand> thisstandsVIP;
		std::vector<Stand> thisstandsOverflow;
		for (auto stand : found->second)
		{
			auto code = stand.second.mAirlinecode;
			if (code == "UAE")
			{
				thisstandsUAE.push_back(stand.second);
				continue;
			}
			if (code == "ABY")
			{
				thisstandsABY.push_back(stand.second);
				continue;
			}
			if (code == "PAX")
			{
				thisstandsPAX.push_back(stand.second);
				continue;
			}

			if (code == "CARGO" || code == "CLC")
				thisstandsCARGO.push_back(stand.second);
			if (code == "LWC" || code == "CLC")
				thisstandsLOWCOST.push_back(stand.second);
			if (code == "GA")
			{
				thisstandsGA.push_back(stand.second);
				continue;
			}
			if (code == "CARGO1")
			{
				thisstandsCARGOspec.push_back(stand.second);
				continue;
			}
			if (code == "VIP")
			{
				thisstandsVIP.push_back(stand.second);
				continue;
			}
			if (code == "ALL")
			{
				thisstandsOverflow.push_back(stand.second);
				continue;
			}


		}
		std::pair<std::string, std::vector<Stand>> temp(airport, thisstandsUAE);
		standsUAE.insert(temp);
		std::pair<std::string, std::vector<Stand>> temp2(airport, thisstandsABY);
		standsABY.insert(temp2);
		std::pair<std::string, std::vector<Stand>> temp3(airport, thisstandsPAX);
		standsPAX.insert(temp3);
		std::pair<std::string, std::vector<Stand>> temp4(airport, thisstandsCARGO);
		standsCARGO.insert(temp4);
		std::pair<std::string, std::vector<Stand>> temp5(airport, thisstandsCARGOspec);
		standsCargoSpecial.insert(temp5);
		std::pair<std::string, std::vector<Stand>> temp6(airport, thisstandsLOWCOST);
		standsLOWCOST.insert(temp6);
		std::pair<std::string, std::vector<Stand>> temp7(airport, thisstandsGA);
		standsGA.insert(temp7);
		std::pair<std::string, std::vector<Stand>> temp8(airport, thisstandsVIP);
		standsVIP.insert(temp8);
		std::pair<std::string, std::vector<Stand>> temp9(airport, thisstandsOverflow);
		standsOverflow.insert(temp9);
		
	}
	
}




CStandAssigner::~CStandAssigner(void)
{
}
Stand getStandOfAircraft(EuroScopePlugIn::CPosition position)
{
	for (auto airport : activeAirports)
	{
		auto found = data.find(airport);
		if (found == data.end()) continue;
		for (auto stand : found->second)
		{
			auto distance = position.DistanceTo(stand.second.position);
			if (distance < TOL)
				return stand.second;
			if (distance > 5)
				break;
		}
	}
	Stand empty("ZZZ", "N000-00-00.0", "E000-00-00.0", "PAX", "", "", "F", "yes", "OMDB");
	return empty;
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
		auto icao = FlightPlan.GetFlightPlanData().GetDestination();
		auto found = standmapping.find(icao);
		if (found == standmapping.end()) return;
		auto found3 = found->second.find(FlightPlan.GetCallsign());
		//auto found = standmapping.find(FlightPlan.GetCallsign());
		if (found3 != found->second.end())
		{
			auto temp = found3->second.number;
			strcpy(sItemString, temp.c_str());
			auto found4 = data.find(icao);
			if (found4 == data.end()) return;
			auto found2 = found4->second.find(temp);
			if (found2 != found4->second.end())
			{
				auto aircraftposition = FlightPlan.GetCorrelatedRadarTarget().GetPosition().GetPosition();
				if (!FlightPlan.GetCorrelatedRadarTarget().IsValid())
					break;
				if (!found2->second.isEmpty && aircraftposition.DistanceTo(found2->second.position)>=TOL)
				{
					*pColorCode = EuroScopePlugIn::TAG_COLOR_EMERGENCY;
				}
			}
		}
		else
		{
			auto fpdata = FlightPlan.GetFlightPlanData();
			std::string remarks = fpdata.GetRemarks();
			auto dest = fpdata.GetDestination();
			std::regex reOMDB = std::regex(R"(\/STAND([A-Z]\d{1,2}))");
			std::regex reOMSJ = std::regex(R"(\/STAND\d{1,2}[A-Z]?)");
			
			std::smatch match;
			if ((std::regex_search(remarks, match, reOMDB) && strcmp(dest, "OMDB")==0) || (std::regex_search(remarks, match, reOMSJ) && strcmp(dest, "OMSJ") == 0))
			{
				std::string stand = match.str(1);
				auto found2 = data.find(icao);
				if (found2 == data.end()) break;
				auto found = found2->second.find(stand);
				if ( found != found2->second.end())
				{
					found->second.isAssigned = true;
					auto temp2 = standmapping.find(icao);
					auto copy = temp2->second;
					std::pair<std::string, Stand> temp(FlightPlan.GetCallsign(), found->second);
					copy.insert(temp);
					standmapping.at(icao) = copy;
				}
			}
				
		}
				
	}
		
}

bool CStandAssigner::OnCompileCommand(const char * sCommandLine)
{
	if (std::strcmp(sCommandLine, ".showstands") == 0)
	{
		
		for (auto airport : activeAirports)
		{
			std::string out;
			auto found = data.find(airport);
			for (auto it : found->second)
			{
				if (!it.second.isEmpty || it.second.isAssigned)
					out += it.first;
			}
			DisplayUserMessage("Occupied Stands", airport.c_str(), out.c_str(), true, true, true, true, true);
		}
		
		
		return true;
	}
	return false;
}

void CStandAssigner::OnRadarTargetPositionUpdate(EuroScopePlugIn::CRadarTarget rt)
{
	if (rt.IsValid())
	{
		if (rt.GetPosition().GetPressureAltitude() > 1000) return;
		auto position = rt.GetPosition().GetPosition();
		auto stand = getStandOfAircraft(position);
		auto icao = stand.mICAO;
		auto found = data.find(icao);
		if (found == data.end()) return;
		if (stand.number != "ZZZ") {
			found->second.at(stand.number).isEmpty = false;
			auto mystand = found->second.at(stand.number);
			auto code = mystand.mAirlinecode;
			if (code == "UAE")
			{
				for (auto &temp : standsUAE.at(icao))
				{
					if (temp.number == mystand.number)
						temp.isEmpty = false;
				}
			}
			if (code == "ABY" || code == "PAX")
			{
				for (auto &temp : standsABY.at(icao))
				{
					if (temp.number == mystand.number)
						temp.isEmpty = false;
				}
			}
			if (code == "PAX" || code == "ABY")
			{
				for (auto &temp : standsPAX.at(icao))
				{
					if (temp.number == mystand.number)
						temp.isEmpty = false;
				}
			}
			if (code == "CARGO1")
			{
				for (auto &temp : standsCargoSpecial.at(icao))
				{
					if (temp.number == mystand.number)
						temp.isEmpty = false;
				}
			}
			if (code == "CARGO" || code == "CLC")
			{
				for (auto &temp : standsCARGO.at(icao))
				{
					if (temp.number == mystand.number)
						temp.isEmpty = false;
				}
			}

			if (code == "LWC" || code == "CLC")
			{
				for (auto &temp : standsLOWCOST.at(icao))
				{
					if (temp.number == mystand.number)
						temp.isEmpty = false;
				}
			}
			if (code == "GA")
			{
				for (auto &temp : standsGA.at(icao))
				{
					if (temp.number == mystand.number)
						temp.isEmpty = false;
				}
			}
			if (code == "VIP")
			{
				for (auto &temp : standsVIP.at(icao))
				{
					if (temp.number == mystand.number)
						temp.isEmpty = false;
				}
			}
			if (code == "ALL")
			{
				for (auto &temp : standsOverflow.at(icao))
				{
					if (temp.number == mystand.number)
						temp.isEmpty = false;
				}
			}
		}
	}
}
void CStandAssigner::OnControllerDisconnect(EuroScopePlugIn::CController Controller)
{
	if (Controller.GetCallsign() == ControllerMyself().GetCallsign())
	{
		standmapping.clear();
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
		auto dest = fp.GetFlightPlanData().GetDestination();
		auto found = data.find(dest);
		if (found == data.end()) return;
		auto found2 = found->second.find(input);
		if (found2 == found->second.end()) return;
		found2->second.isAssigned = true;
		std::string code = found2->second.mAirlinecode;
		auto it = found2->second;
		if (code == "UAE" && standsUAE.find(dest) != standsUAE.end())
		{
			for (auto &temp : standsUAE.at(dest))
			{
				if (temp.number == it.number)
					temp.isAssigned = true;
			}
		}
		if ((code == "PAX" || code == "ABY") && standsPAX.find(dest) != standsPAX.end())
		{
			for (auto &temp : standsPAX.at(dest))
			{
				if (temp.number == it.number)
					temp.isAssigned = true;
			}
		}

		if ((code == "CARGO" || code == "CLC") && standsCARGO.find(dest) != standsCARGO.end())
		{
			for (auto &temp : standsCARGO.at(dest))
			{
				if (temp.number == it.number)
					temp.isAssigned = true;
			}
		}

		if ((code == "LWC" || code == "CLC") && standsLOWCOST.find(dest) != standsLOWCOST.end())

		{
			for (auto &temp : standsLOWCOST.at(dest))
			{
				if (temp.number == it.number)
					temp.isAssigned = true;
			}
		}
		if (code == "GA" && standsGA.find(dest) != standsGA.end())
		{
			for (auto &temp : standsGA.at(dest))
			{
				if (temp.number == it.number)
					temp.isAssigned = true;
			}
		}
		if (code == "VIP" && standsVIP.find(dest) != standsVIP.end())
		{
			for (auto &temp : standsVIP.at(dest))
			{
				if (temp.number == it.number)
					temp.isAssigned = true;
			}
		}
		if ((code == "ABY" || code == "PAX") && standsABY.find(dest) != standsABY.end())
		{
			for (auto &temp : standsABY.at(dest))
			{
				if (temp.number == it.number)
					temp.isAssigned = true;
			}
		}
		if (code == "CARGO1" && standsCargoSpecial.find(dest) != standsCargoSpecial.end())
		{
			for (auto &temp : standsCargoSpecial.at(dest))
			{
				if (temp.number == it.number)
					temp.isAssigned = true;
			}
		}
		if (code == "ALL" && standsOverflow.find(dest) != standsOverflow.end())
		{
			for (auto &temp : standsOverflow.at(dest))
			{
				if (temp.number == it.number)
					temp.isAssigned = true;
			}
		}
		auto found3 = standmapping.find(dest);
		std::unordered_map<std::string, Stand> copy;
		if (found3 != standmapping.end())
			std::unordered_map<std::string, Stand> copy = found3->second;
		std::pair<std::string, Stand> temp2(fp.GetCallsign(), found2->second);
		copy.insert(temp2);
		if (standmapping.find(dest) == standmapping.end())
		{
			std::pair<std::string, std::unordered_map<std::string, Stand>> temp3(dest, copy);
			standmapping.insert(temp3);
		}
		else
			standmapping.at(dest) = copy;
		
		auto fpdata = fp.GetFlightPlanData();
		std::string remarks = fpdata.GetRemarks();
		std::smatch match;
		if (strcmp(dest, "OMDB") == 0 && std::regex_search(remarks,match, std::regex(R"(\/STAND[A-Z]\d{1,2})")))
			return;
		if (strcmp(dest, "OMSJ") == 0 && std::regex_search(remarks, match, std::regex(R"(\/STAND\d{1,2}[A-Z]?)")))
			return;
		remarks += "/STAND" + input;
		fpdata.SetRemarks(remarks.c_str());
		fpdata.AmendFlightPlan();
		break;
		
		
	}
	case TAG_FUNC_CLEAR:
	{
		auto dest = fp.GetFlightPlanData().GetDestination();
		if (std::find(activeAirports.begin(), activeAirports.end(), dest) == activeAirports.end()) return;
		auto airportstanddata = data.find(dest);
		/*auto found2 = found->second.find(fp.GetCallsign());
		if (found2 == found->second.end()) return;
		
		auto temp2 = found->second.find(found2->second.number);
		if (temp2 != found->second.end())
		{
			temp2->second.isAssigned = false;
		}*/
		auto copy = standmapping.find(dest)->second;
		auto stand = copy.find(fp.GetCallsign());
		if (stand == copy.end()) return;
		auto fpdata = fp.GetFlightPlanData();
		/*auto getstand = airportstanddata->second.find(stand->second.number);
		if (getstand != airportstanddata->second.end())
		{
			getstand->second.isAssigned = false;
		}*/
		copy.erase(fp.GetCallsign());
		standmapping.at(dest) = copy;
		
		std::string remarks = fpdata.GetRemarks();
		if(strcmp(dest,"OMDB") == 0)
			remarks = std::regex_replace(remarks, std::regex(R"(\/STAND[A-Z]\d{1,2})"), "");
		else if(strcmp(dest, "OMSJ") == 0)
			remarks = std::regex_replace(remarks, std::regex(R"(\/STAND\d{1,2}[A-Z]?)"), "");
		fpdata.SetRemarks(remarks.c_str());
		fpdata.AmendFlightPlan();
			
		
		break;
	}
	case TAG_FUNC_ASSIGN_POPUP:
	{
		OpenPopupList(Area, "Assign Stand", 1);
		auto dest = fp.GetFlightPlanData().GetDestination();

		AddPopupListElement("Assign Auto", "", TAG_FUNC_ASSIGN_AUTO);
		if(strcmp(dest, "OMDB") == 0)
			AddPopupListElement("Assign UAE", "", TAG_FUNC_ASSIGN_UAE);
		if (strcmp(dest, "OMSJ") == 0)
		{
			AddPopupListElement("Assign ABY", "", TAG_FUNC_ASSIGN_ABY);
			AddPopupListElement("Assign SQC/GEC/UPS", "", TAG_FUNC_ASSIGN_CARGO2);
		}
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
		std::string callsign = fp.GetCallsign();
		std::string remarks = fp.GetFlightPlanData().GetRemarks();
		if (remarks.find("Cargo") != std::string::npos|| remarks.find("CARGO") != std::string::npos|| remarks.find("cargo")!= std::string::npos || remarks.find("freight") !=std::string::npos || remarks.find("Freight") != std::string::npos || remarks.find("FREIGHT") != std::string::npos)
		{
			std::string logstring;
			logstring = "Cargo Callsign through remarks for " + callsign + ", because remarks are "+remarks;
			LOG_F(INFO, logstring.c_str());
			goto CARGO;
		}
		if (callsign.length() < 3)
			goto PAX;
		std::string op = callsign.substr(0, 3);
		std::regex number = std::regex(R"(.*\d.*)");
		std::smatch match;
		auto test = fp.GetFlightPlanData().GetPlanType();
		if (std::regex_search(op, match, number)|| strcmp(test,"V") == 0)
		{
			std::string logstring;
			logstring = "Assigning GA stand for " + callsign + " because we found a number in the first three characters of the callsign or the flightrules are VFR.";
			LOG_F(INFO, logstring.c_str());
			goto GA;
		}
		auto icao = fp.GetFlightPlanData().GetDestination();
		auto found2 = callsignmap.find(icao);
		if (found2 == callsignmap.end()) return;
		auto found = found2->second.find(op);
		if (found != found2->second.end())
		{
			auto assignment = found->second;
			if (assignment == "UAE")
			{
				std::regex uaecargo = std::regex(R"(UAE9\d{3})");
				std::smatch match;
				if (std::regex_search(callsign, match, uaecargo))
				{
					std::string logstring;
					logstring = "Detected SkyCargo for Callsign " + callsign;
					LOG_F(INFO, logstring.c_str());
					goto CARGO;
				}
				std::string logstring;
				logstring = "Detected Emirates Callsign " + callsign;
				LOG_F(INFO, logstring.c_str());
				goto UAE;
			}
			if (assignment == "LWC")
			{
				std::string logstring;
				logstring = "Detected lowcost Callsign " + callsign;
				LOG_F(INFO, logstring.c_str());
				goto LWC;
			}
			if (assignment == "ABY")
			{
				std::string logstring;
				logstring = "Detected Air Arabia Callsign " + callsign;
				LOG_F(INFO, logstring.c_str());
				goto ABY;
			}
			if (assignment == "VIP")
			{
				std::string logstring;
				logstring = "Detected VIP Callsign " + callsign;
				LOG_F(INFO, logstring.c_str());
				goto VIP;
			}
			if (assignment == "CARGO")
			{
				std::string logstring;
				logstring = "Detected Cargo Callsign " + callsign;
				LOG_F(INFO, logstring.c_str());
				goto CARGO;
			}
			if (assignment == "CARGO1")
			{
				std::string logstring;
				logstring = "Detected Special Cargo Callsign " + callsign;
				LOG_F(INFO, logstring.c_str());
				goto CARGO1;
			}
			if (assignment == "GA")
			{
				std::string logstring;
				logstring = "Detected GA Callsign " + callsign;
				LOG_F(INFO, logstring.c_str());
				goto GA;
			}
		}
		std::string logstring;
		logstring = "Could not find any rule to assign " + callsign +" so we treated it as a normal international carrier.";
		LOG_F(INFO, logstring.c_str());
		goto PAX;
		break;
	}
	case TAG_FUNC_ASSIGN_CARGO:
	{
	CARGO:
		auto icao = fp.GetFlightPlanData().GetDestination();
		auto found = standmapping.find(icao);
		std::unordered_map<std::string, Stand> copy;
		if (found != standmapping.end())
		{
			auto found2 = found->second.find(fp.GetCallsign());
			if (found2 != found->second.end())
				break;
			copy = standmapping.at(icao);
		}
		auto size = determineAircraftCat(fp);
		auto standshere = standsCARGO.find(icao);
		if (standshere == standsCARGO.end()) break;
		auto stand = extractRandomStand(standshere->second, size, icao);
		if (stand.number == "Z00")
			break;
		data.at(icao).at(stand.number).isAssigned = true;
		std::pair<std::string, Stand> temp(fp.GetCallsign(), stand);
		copy.insert(temp);
		if (found == standmapping.end())
		{
			std::pair<std::string, std::unordered_map<std::string, Stand>> temp2(icao, copy);
			standmapping.insert(temp2);
		}
		else
			standmapping.at(icao) = copy;
		for (auto &temp : standsCARGO.at(icao))
		{
			if (temp.number == stand.number)
				temp.isAssigned = true;
		}
		for (auto &temp : standsLOWCOST.at(icao))
		{
			if (temp.number == stand.number)
				temp.isAssigned = true;
		}
		auto fpdata = fp.GetFlightPlanData();
		std::string remarks = fpdata.GetRemarks();
		std::smatch match;
		
		if (strcmp(icao, "OMDB") == 0 && std::regex_search(remarks, match, std::regex(R"(\/STAND[A-Z]\d{1,2})")))
			return;
		if (strcmp(icao, "OMSJ") == 0 && std::regex_search(remarks, match, std::regex(R"(\/STAND\d{1,2}[A-Z]?)")))
			return;
		remarks += "/STAND" + stand.number;
		fpdata.SetRemarks(remarks.c_str());
		fpdata.AmendFlightPlan();
		break;
	}
	case TAG_FUNC_ASSIGN_PAX:
	{
	PAX:
		auto icao = fp.GetFlightPlanData().GetDestination();
		auto found = standmapping.find(icao);
		std::unordered_map<std::string, Stand> copy;
		if (found != standmapping.end())
		{
			auto found2 = found->second.find(fp.GetCallsign());
			if (found2 != found->second.end())
				break;
			copy = standmapping.at(icao);
		}
		auto size = determineAircraftCat(fp);
		auto standshere = standsPAX.find(icao);
		if (standshere == standsPAX.end()) break;
		auto stand = extractRandomStand(standshere->second, size, icao);
		if (stand.number == "Z00")
			break;
		data.at(icao).at(stand.number).isAssigned = true;
		std::pair<std::string, Stand> temp(fp.GetCallsign(), stand);
		copy.insert(temp);
		if (found == standmapping.end())
		{
			std::pair<std::string, std::unordered_map<std::string, Stand>> temp2(icao, copy);
			standmapping.insert(temp2);
		}
		else	
			standmapping.at(icao) = copy;
		for (auto &temp : standsPAX.at(icao))
		{
			if (temp.number == stand.number)
				temp.isAssigned = true;
		}
		for (auto &temp : standsABY.at(icao))
		{
			if (temp.number == stand.number)
				temp.isAssigned = true;
		}
		auto fpdata = fp.GetFlightPlanData();
		std::string remarks = fpdata.GetRemarks();
		std::smatch match;
		if (strcmp(icao, "OMDB") == 0 && std::regex_search(remarks, match, std::regex(R"(\/STAND[A-Z]\d{1,2})")))
			return;
		if (strcmp(icao, "OMSJ") == 0 && std::regex_search(remarks, match, std::regex(R"(\/STAND\d{1,2}[A-Z]?)")))
			return;
		remarks += "/STAND" + stand.number;
		fpdata.SetRemarks(remarks.c_str());
		fpdata.AmendFlightPlan();
		break;
	}
	case TAG_FUNC_ASSIGN_UAE:
	{
	UAE:
		auto icao = fp.GetFlightPlanData().GetDestination();
		auto found = standmapping.find(icao);
		std::unordered_map<std::string, Stand> copy;
		if (found != standmapping.end())
		{
			auto found2 = found->second.find(fp.GetCallsign());
			if (found2 != found->second.end())
				break;
			copy = standmapping.at(icao);
		}
		auto size = determineAircraftCat(fp);
		auto standshere = standsUAE.find(icao);
		if (standshere == standsUAE.end()) break;
		auto stand = extractRandomStand(standshere->second, size, icao);
		if (stand.number == "Z00")
			break;
		data.at(icao).at(stand.number).isAssigned = true;
		std::pair<std::string, Stand> temp(fp.GetCallsign(), stand);
		copy.insert(temp);
		if (found == standmapping.end())
		{
			std::pair<std::string, std::unordered_map<std::string, Stand>> temp2(icao, copy);
			standmapping.insert(temp2);
		}
		else
			standmapping.at(icao) = copy;
		for (auto &temp : standsUAE.at(icao))
		{
			if (temp.number == stand.number)
				temp.isAssigned = true;
		}
		auto fpdata = fp.GetFlightPlanData();
		std::string remarks = fpdata.GetRemarks();
		std::smatch match;
		if (strcmp(icao, "OMDB") == 0 && std::regex_search(remarks, match, std::regex(R"(\/STAND[A-Z]\d{1,2})")))
			return;
		if (strcmp(icao, "OMSJ") == 0 && std::regex_search(remarks, match, std::regex(R"(\/STAND\d{1,2}[A-Z]?)")))
			return;
		remarks += "/STAND" + stand.number;
		fpdata.SetRemarks(remarks.c_str());
		fpdata.AmendFlightPlan();
		break;
	}
	case TAG_FUNC_ASSIGN_ABY:
	{
	ABY:
		auto icao = fp.GetFlightPlanData().GetDestination();
		auto found = standmapping.find(icao);
		std::unordered_map<std::string, Stand> copy;
		if (found != standmapping.end())
		{
			auto found2 = found->second.find(fp.GetCallsign());
			if (found2 != found->second.end())
				break;
			copy = standmapping.at(icao);
		}
		auto size = determineAircraftCat(fp);
		auto standshere = standsABY.find(icao);
		auto standsherePAX = standsPAX.find(icao);
		std::vector<Stand> joined;
		joined.reserve(standshere->second.size() + standsherePAX->second.size());
		joined.insert(joined.end(), standshere->second.begin(), standshere->second.end());
		joined.insert(joined.end(), standsherePAX->second.begin(), standsherePAX->second.end());
		if (standshere == standsABY.end()) break;
		//ABY parks everywhere in sharjah
		auto stand = extractRandomStand(joined, size, icao);
		if (stand.number == "Z00")
			break;
		data.at(icao).at(stand.number).isAssigned = true;
		std::pair<std::string, Stand> temp(fp.GetCallsign(), stand);
		copy.insert(temp);
		if (found == standmapping.end())
		{
			std::pair<std::string, std::unordered_map<std::string, Stand>> temp2(icao, copy);
			standmapping.insert(temp2);
		}
		else
			standmapping.at(icao) = copy;
		for (auto &temp : standsABY.at(icao))
		{
			if (temp.number == stand.number)
				temp.isAssigned = true;
		}
		for (auto &temp : standsPAX.at(icao))
		{
			if (temp.number == stand.number)
				temp.isAssigned = true;
		}
		auto fpdata = fp.GetFlightPlanData();
		std::string remarks = fpdata.GetRemarks();
		std::smatch match;
		if (strcmp(icao, "OMDB") == 0 && std::regex_search(remarks, match, std::regex(R"(\/STAND[A-Z]\d{1,2})")))
			return;
		if (strcmp(icao, "OMSJ") == 0 && std::regex_search(remarks, match, std::regex(R"(\/STAND\d{1,2}[A-Z]?)")))
			return;
		remarks += "/STAND" + stand.number;
		fpdata.SetRemarks(remarks.c_str());
		fpdata.AmendFlightPlan();
		break;
	}
	case TAG_FUNC_ASSIGN_VIP:
	{
	VIP:
		auto icao = fp.GetFlightPlanData().GetDestination();
		auto found = standmapping.find(icao);
		std::unordered_map<std::string, Stand> copy;
		if (found != standmapping.end())
		{
			auto found2 = found->second.find(fp.GetCallsign());
			if (found2 != found->second.end())
				break;
			copy = standmapping.at(icao);
		}
		auto size = determineAircraftCat(fp);
		auto standshere = standsVIP.find(icao);
		if (standshere == standsVIP.end()) break;
		auto stand = extractRandomStand(standshere->second, size, icao);
		if (stand.number == "Z00")
			break;
		data.at(icao).at(stand.number).isAssigned = true;
		std::pair<std::string, Stand> temp(fp.GetCallsign(), stand);
		copy.insert(temp);
		if (found == standmapping.end())
		{
			std::pair<std::string, std::unordered_map<std::string, Stand>> temp2(icao, copy);
			standmapping.insert(temp2);
		}
		else
			standmapping.at(icao) = copy;
		for (auto &temp : standsVIP.at(icao))
		{
			if (temp.number == stand.number)
				temp.isAssigned = true;
		}
		auto fpdata = fp.GetFlightPlanData();
		std::string remarks = fpdata.GetRemarks();
		std::smatch match;
		if (strcmp(icao, "OMDB") == 0 && std::regex_search(remarks, match, std::regex(R"(\/STAND[A-Z]\d{1,2})")))
			return;
		if (strcmp(icao, "OMSJ") == 0 && std::regex_search(remarks, match, std::regex(R"(\/STAND\d{1,2}[A-Z]?)")))
			return;
		remarks += "/STAND" + stand.number;
		fpdata.SetRemarks(remarks.c_str());
		fpdata.AmendFlightPlan();
		break;
	}
	case TAG_FUNC_ASSIGN_LOWCOST:
	{
	LWC:
		auto icao = fp.GetFlightPlanData().GetDestination();
		auto found = standmapping.find(icao);
		std::unordered_map<std::string, Stand> copy;
		if (found != standmapping.end())
		{
			auto found2 = found->second.find(fp.GetCallsign());
			if (found2 != found->second.end())
				break;
			copy = standmapping.at(icao);
		}
		auto size = determineAircraftCat(fp);
		auto standshere = standsLOWCOST.find(icao);
		if (standshere == standsLOWCOST.end()) break;
		auto stand = extractRandomStand(standshere->second, size, icao);
		if (stand.number == "Z00")
			break;
		data.at(icao).at(stand.number).isAssigned = true;
		std::pair<std::string, Stand> temp(fp.GetCallsign(), stand);
		copy.insert(temp);
		if (found == standmapping.end())
		{
			std::pair<std::string, std::unordered_map<std::string, Stand>> temp2(icao, copy);
			standmapping.insert(temp2);
		}
		else
			standmapping.at(icao) = copy;
		for (auto &temp : standsCARGO.at(icao))
		{
			if (temp.number == stand.number)
				temp.isAssigned = true;
		}
		for (auto &temp : standsLOWCOST.at(icao))
		{
			if (temp.number == stand.number)
				temp.isAssigned = true;
		}
		auto fpdata = fp.GetFlightPlanData();
		std::string remarks = fpdata.GetRemarks();
		std::smatch match;
		if (strcmp(icao, "OMDB") == 0 && std::regex_search(remarks, match, std::regex(R"(\/STAND[A-Z]\d{1,2})")))
			return;
		if (strcmp(icao, "OMSJ") == 0 && std::regex_search(remarks, match, std::regex(R"(\/STAND\d{1,2}[A-Z]?)")))
			return;
		remarks += "/STAND" + stand.number;
		fpdata.SetRemarks(remarks.c_str());
		fpdata.AmendFlightPlan();
		break;
	}
	case TAG_FUNC_ASSIGN_GA:
	{
	GA:
		auto icao = fp.GetFlightPlanData().GetDestination();
		auto found = standmapping.find(icao);
		std::unordered_map<std::string, Stand> copy;
		if (found != standmapping.end())
		{
			auto found2 = found->second.find(fp.GetCallsign());
			if (found2 != found->second.end())
				break;
			copy = standmapping.at(icao);
		}
		auto size = determineAircraftCat(fp);
		auto standshere = standsGA.find(icao);
		if (standshere == standsGA.end()) break;
		auto stand = extractRandomStand(standshere->second, size, icao);
		if (stand.number == "Z00")
			break;
		data.at(icao).at(stand.number).isAssigned = true;
		std::pair<std::string, Stand> temp(fp.GetCallsign(), stand);
		copy.insert(temp);
		if (found == standmapping.end())
		{
			std::pair<std::string, std::unordered_map<std::string, Stand>> temp2(icao, copy);
			standmapping.insert(temp2);
		}
		else
			standmapping.at(icao) = copy;
		for (auto &temp : standsGA.at(icao))
		{
			if (temp.number == stand.number)
				temp.isAssigned = true;
		}
		auto fpdata = fp.GetFlightPlanData();
		std::string remarks = fpdata.GetRemarks();
		std::smatch match;
		if (strcmp(icao, "OMDB") == 0 && std::regex_search(remarks, match, std::regex(R"(\/STAND[A-Z]\d{1,2})")))
			return;
		if (strcmp(icao, "OMSJ") == 0 && std::regex_search(remarks, match, std::regex(R"(\/STAND\d{1,2}[A-Z]?)")))
			return;
		remarks += "/STAND" + stand.number;
		fpdata.SetRemarks(remarks.c_str());
		fpdata.AmendFlightPlan();
		break;
	}
	case TAG_FUNC_ASSIGN_CARGO2:
	{
	CARGO1:
	auto icao = fp.GetFlightPlanData().GetDestination();
	auto found = standmapping.find(icao);
	std::unordered_map<std::string, Stand> copy;
	if (found != standmapping.end())
	{
		auto found2 = found->second.find(fp.GetCallsign());
		if (found2 != found->second.end())
			break;
		copy = standmapping.at(icao);
	}
	auto size = determineAircraftCat(fp);
	auto standshere = standsCargoSpecial.find(icao);
	if (standshere == standsCargoSpecial.end()) break;
	auto stand = extractRandomStand(standshere->second, size, icao);
	if (stand.number == "Z00")
		break;
	data.at(icao).at(stand.number).isAssigned = true;
	std::pair<std::string, Stand> temp(fp.GetCallsign(), stand);
	copy.insert(temp);
	if (found == standmapping.end())
	{
		std::pair<std::string, std::unordered_map<std::string, Stand>> temp2(icao, copy);
		standmapping.insert(temp2);
	}
	else
		standmapping.at(icao) = copy;
	for (auto &temp : standsCargoSpecial.at(icao))
	{
		if (temp.number == stand.number)
			temp.isAssigned = true;
	}
	auto fpdata = fp.GetFlightPlanData();
	std::string remarks = fpdata.GetRemarks();
	std::smatch match;
	if (strcmp(icao, "OMDB") == 0 && std::regex_search(remarks, match, std::regex(R"(\/STAND[A-Z]\d{1,2})")))
		return;
	if (strcmp(icao, "OMSJ") == 0 && std::regex_search(remarks, match, std::regex(R"(\/STAND\d{1,2}[A-Z]?)")))
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
	for (auto airport : activeAirports)
	{
		for (auto& it : data.at(airport))
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
				auto found = standmapping.at(airport).find(cs);
				if (found != standmapping.at(airport).end())
					if (it.second.number == found->second.number)
						goto outer;
				auto temp = FlightPlanSelectNext(first);
				while (temp.IsValid())
				{
					cs = temp.GetCallsign();
					auto found = standmapping.at(airport).find(cs);
					if (found != standmapping.at(airport).end())
						if (it.second.number == found->second.number)
							goto outer;
					temp = FlightPlanSelectNext(temp);
				}
				it.second.isAssigned = false;
				std::string code = it.second.mAirlinecode;
				if (code == "UAE")
				{
					for (auto &temp : standsUAE.at(airport))
					{
						if (temp.number == it.second.number)
							temp.isAssigned = false;
					}
				}
				if (code == "PAX" || code == "ABY")
				{
					for (auto &temp : standsPAX.at(airport))
					{
						if (temp.number == it.second.number)
							temp.isAssigned = false;
					}
				}
				if (code == "ABY" || code == "PAX")
				{
					for (auto &temp : standsABY.at(airport))
					{
						if (temp.number == it.second.number)
							temp.isAssigned = false;
					}
				}
				if (code == "CARGO1")
				{
					for (auto &temp : standsCargoSpecial.at(airport))
					{
						if (temp.number == it.second.number)
							temp.isAssigned = false;
					}
				}

				if (code == "CARGO" || code == "CLC")
				{
					for (auto &temp : standsCARGO.at(airport))
					{
						if (temp.number == it.second.number)
							temp.isAssigned = false;
					}
				}

				if (code == "LWC" || code == "CLC")

				{
					for (auto &temp : standsLOWCOST.at(airport))
					{
						if (temp.number == it.second.number)
							temp.isAssigned = false;
					}
				}
				if (code == "GA")
				{
					for (auto &temp : standsGA.at(airport))
					{
						if (temp.number == it.second.number)
							temp.isAssigned = false;
					}
				}
				if (code == "VIP")
				{
					for (auto &temp : standsVIP.at(airport))
					{
						if (temp.number == it.second.number)
							temp.isAssigned = false;
					}
				}
				if (code == "ALL")
				{
					for (auto &temp : standsOverflow.at(airport))
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
	
	
	
 }

Stand CStandAssigner::extractRandomStand(std::vector<Stand> stands, char size, std::string icao)
{
	
	std::shuffle(std::begin(stands), std::end(stands), std::random_device());
	for (auto stand : stands)
	{
		if (!stand.isAssigned && stand.isEmpty && stand.isInFlytampa && stand.mSize >= size)
		{
			return stand;
		}

	}
	std::shuffle(std::begin(standsOverflow.at(icao)), std::end(standsOverflow.at(icao)), std::random_device());
	for (auto stand : standsOverflow.at(icao))
	{
		if (!stand.isAssigned && stand.isEmpty && stand.isInFlytampa && stand.mSize >= size)
		{
			return stand;
		}
	}
	DisplayUserMessage("StandAssigner", icao.c_str(), "Error assigning stand", true, true, true, true, true);
	auto errorval = standsOverflow.at("OMDB").at(0);
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

void CStandAssigner::readStandFile(std::string dir, std::string airport)
{
	io::CSVReader<8, io::trim_chars<' '>, io::no_quote_escape<','>> in(dir);
	in.read_header(io::ignore_extra_column, "Standnumber", "latitude", "longitude", "airlinecode", "neighbor1", "neighbor2", "size", "flytampa");
	std::string StandNumber, lat, Long, airlinecode, neighbor1, neighbor2, size, flytampa;
	std::unordered_map<std::string, Stand> thisdata;
	while (in.read_row(StandNumber, lat, Long, airlinecode, neighbor1, neighbor2, size, flytampa))
	{
		Stand temp = Stand(StandNumber, lat, Long, airlinecode, neighbor1, neighbor2, size, flytampa,airport);
		std::pair<std::string, Stand> temp2(StandNumber, temp);
		thisdata.insert(temp2);
	}
	std::pair<std::string, std::unordered_map<std::string, Stand>> temp3(airport, thisdata);
	data.insert(temp3);
	std::string logstring;
	logstring = "Stand file read for ";
	logstring += airport;
	LOG_F(INFO, logstring.c_str());
}

void CStandAssigner::readCallsignFile(std::string dir, std::string airport)
{
	//reading callsign mapping
	io::CSVReader<2, io::trim_chars<' '>, io::no_quote_escape<','>> in(dir);
	in.read_header(io::ignore_extra_column, "Callsign", "ToAssign");
	std::string Callsign, ToAssign;
	std::unordered_map<std::string, std::string> thiscallsignmap;
	while (in.read_row(Callsign, ToAssign))
	{
		std::pair<std::string, std::string> temp3(Callsign, ToAssign);
		thiscallsignmap.insert(temp3);
	}
	std::pair<std::string, std::unordered_map<std::string, std::string>> temp(airport, thiscallsignmap);
	callsignmap.insert(temp);
	std::string logstring;
	logstring = "Callsignmapping file read for ";
	logstring += airport;
	LOG_F(INFO, logstring.c_str());
}



