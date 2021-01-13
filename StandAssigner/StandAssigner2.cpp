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





#define MY_PLUGIN_NAME      "Stand Assigner for VATSIM"
#define MY_PLUGIN_VERSION   "1.0"
#define MY_PLUGIN_DEVELOPER "Nils Dornbusch"
#define MY_PLUGIN_COPYRIGHT "Free to be distributed as source code"
#define MY_PLUGIN_VIEW      ""


const	int		TAG_ITEM_ROUTE_VALID = 123123;


const   int     TAG_FUNC_EDIT = 15;
const int TAG_FUNC_ASSIGN_POPUP = 16;



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

	RegisterTagItemType("Stand", TAG_ITEM_ROUTE_VALID);

	RegisterTagItemFunction("Edit Stand", TAG_FUNC_EDIT);

	RegisterTagItemFunction("Assign Stand", TAG_FUNC_ASSIGN_POPUP);




	LOG_F(INFO, "Everything registered. Ready to go!");


	dir += "OMDB.csv";
	io::CSVReader<5, io::trim_chars<' '>, io::no_quote_escape<','>> in(dir);
	in.read_header(io::ignore_extra_column, "Dep", "Dest", "Evenodd", "Restriction", "Route");
	std::string Dep, Dest, evenodd, LevelR, Routing;
	while (in.read_row(Dep, Dest, evenodd, LevelR, Routing))
	{
		
	}
}




CStandAssigner::~CStandAssigner(void)
{
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
	
		return;
	
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

	auto fpdata = fp.GetFlightPlanData();
	std::string dep = fpdata.GetOrigin();

	// switch by the function ID
	switch (FunctionId)
	{

	}// switch by the function ID
}

void 



