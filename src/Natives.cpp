/*
*  Version: MPL 1.1
*
*  The contents of this file are subject to the Mozilla Public License Version
*  1.1 (the "License"); you may not use this file except in compliance with
*  the License. You may obtain a copy of the License at
*  http://www.mozilla.org/MPL/
*
*  Software distributed under the License is distributed on an "AS IS" basis,
*  WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
*  for the specific language governing rights and limitations under the
*  License.
*
*  The Original Code is the YSI 2.0 SA:MP plugin.
*
*  The Initial Developer of the Original Code is Alex "Y_Less" Cole.
*  Portions created by the Initial Developer are Copyright (C) 2008
*  the Initial Developer. All Rights Reserved. The development was abandobed
*  around 2010, afterwards kurta999 has continued it.
*
*  Contributor(s):
*
*	0x688, balika011, Gamer_Z, iFarbod, karimcambridge, Mellnik, P3ti, Riddick94
*	Slice, sprtik, uint32, Whitetigerswt, Y_Less, ziggi and complete SA-MP community
*
*  Special Thanks to:
*
*	SA:MP Team past, present and future
*	Incognito, maddinat0r, OrMisicL, Zeex
*
*/

#include "main.h"
#include <memory>

//----------------------------------------------------

// native execute(const command[], saveoutput=0, index=0);
AMX_DECLARE_NATIVE(Natives::execute)
{
	CHECK_PARAMS(3, NO_FLAGS);
	
	std::string command;
	int saveoutput, index;
	CScriptParams::Get()->Read(&command, &saveoutput, &index);

	auto thFunc = [](std::string command, int saveoutput, int index) 
	{
		FILE *pPipe;
		char szBuffer[512];
		
		CServer::SysExec_t exec;
		exec.index = index;
		exec.output = "";
		exec.success = false;
	
		if ((pPipe = popen(command.c_str(), "r")) != NULL)
		{
			while (saveoutput && fgets(szBuffer, sizeof(szBuffer), pPipe))
				exec.output.append(szBuffer);

			exec.retval = pclose(pPipe);
			exec.success = true;
		}	

		std::lock_guard<std::mutex> lock(CServer::Get()->m_SysExecMutex);
		CServer::Get()->m_SysExecQueue.push(exec);
	};

	std::thread(thFunc, command, saveoutput, index).detach();
	return 1;
}

#ifdef _WIN32
	// native ffind(const pattern[], filename[], len, &idx);
	AMX_DECLARE_NATIVE(Natives::ffind)
	{
		// Find a file, idx determines which one of a number of matches to use
		CHECK_PARAMS(4, NO_FLAGS);
		cell
			*cptr;
		char
			*szSearch;
		// Get the search pattern
		amx_StrParam(amx, params[1], szSearch);
		if (szSearch)
		{
			// Get associated search information
			amx_GetAddr(amx, params[4], &cptr);
			cell
				count = *cptr;
			WIN32_FIND_DATA
				ffd;
			TCHAR
				szDir[MAX_PATH] = TEXT("./scriptfiles/");
			StringCchCat(szDir, MAX_PATH, szSearch);
			// Get a serch handle
			HANDLE
				hFind = FindFirstFile(szDir, &ffd);
			if (hFind != INVALID_HANDLE_VALUE)
			{
				do
				{
					// Check that this isn't a directory
					if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
					{
						// It's not - update idx
						if (!count)
						{
							// No files left to skip, return the data
							(*cptr)++;
							amx_GetAddr(amx, params[2], &cptr);
							amx_SetString(cptr, ffd.cFileName, 0, 0, params[3]);
							FindClose(hFind);
							return 1;
						}
						count--;
					}
				}
				while (FindNextFile(hFind, &ffd) != 0);
				FindClose(hFind);
			}
		}
		return 0;
	}
	
	// native dfind(const pattern[], filename[], len, &idx);
	AMX_DECLARE_NATIVE(Natives::dfind)
	{
		// Find a directory, idx determines which one of a number of matches to use
		// Identical to ffind in all but 1 line
		CHECK_PARAMS(4, NO_FLAGS);
		cell
			*cptr;
		char
			*szSearch;
		// Get the search pattern
		amx_StrParam(amx, params[1], szSearch);
		if (szSearch)
		{
			// Get associated search information
			amx_GetAddr(amx, params[4], &cptr);
			cell
				count = *cptr;
			WIN32_FIND_DATA
				ffd;
			TCHAR
				szDir[MAX_PATH] = TEXT("./scriptfiles/");
			StringCchCat(szDir, MAX_PATH, szSearch);
			// Get a serch handle
			HANDLE
				hFind = FindFirstFile(szDir, &ffd);
			if (hFind != INVALID_HANDLE_VALUE)
			{
				do
				{
					// Check that this is a directory
					if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					{
						// It is - update idx
						if (!count)
						{
							// No files left to skip, return the data
							(*cptr)++;
							amx_GetAddr(amx, params[2], &cptr);
							amx_SetString(cptr, ffd.cFileName, 0, 0, params[3]);
							FindClose(hFind);
							return 1;
						}
						count--;
					}
				}
				while (FindNextFile(hFind, &ffd) != 0);
				FindClose(hFind);
			}
		}
		return 0;
	}
#else
	// native ffind(const pattern[], filename[], len, &idx);
	AMX_DECLARE_NATIVE(Natives::ffind)
	{
		// Find a file, idx determines which one of a number of matches to use
		CHECK_PARAMS(4, NO_FLAGS);
		cell
			*cptr;
		char
			*szSearch;
		// Get the search pattern
		amx_StrParam(amx, params[1], szSearch);
		if (szSearch)
		{
			// Get associated search information
			amx_GetAddr(amx, params[4], &cptr);
			cell
				count = *cptr;
			// Find the end of the directory name
			int
				end = strlen(szSearch) - 1;
			if (end == -1)
			{
				return 0;
			}
			while (szSearch[end] != '\\' && szSearch[end] != '/')
			{
				if (!end)
				{
					break;
				}
				end--;
			}
			// Split up the information
			// Ensure that we search in scriptfiles
			// And separate out the filename and path
			char
				*szDir = (char *)alloca(end + 16),
				*szFile;
			strcpy(szDir, "./scriptfiles/");
			if (end)
			{
				szFile = &szSearch[end + 1];
				szSearch[end] = '\0';
				strcpy(szDir + 14, szSearch);
				strcpy(szDir + strlen(szDir), "/");
			}
			else
			{
				szFile = szSearch;
			}
			end = strlen(szDir);
			DIR
				*dp = opendir(szDir);
			if (dp)
			{
				// Loop through all files in the directory
				struct dirent
					*ep;
				while (ep = readdir(dp))
				{
					// Check if this file matches the pattern
					if (!fnmatch(szFile, ep->d_name, FNM_NOESCAPE))
					{
						// Check if this is a directory
						// There MUST be an easier way to do this!
						char
							*full = (char *)malloc(strlen(ep->d_name) + end + 1);
						if (!full)
						{
							closedir(dp);
							return 0;
						}
						strcpy(full, szDir);
						strcpy(full + end, ep->d_name);
						DIR
							*xp = opendir(full);
						free(full);
						if (xp)
						{
							closedir(xp);
							continue;
						}
						// Check if there's any left to skip
						if (!count)
						{
							// No files left to skip, return the data
							(*cptr)++;
							amx_GetAddr(amx, params[2], &cptr);
							amx_SetString(cptr, ep->d_name, 0, 0, params[3]);
							closedir(dp);
							return 1;
						}
						count--;
					}
				}
				closedir(dp);
				return 0;
			}
		}
		return 0;
	}
	
	// native dfind(const pattern[], filename[], len, &idx);
	AMX_DECLARE_NATIVE(Natives::dfind)
	{
		// Find a file, idx determines which one of a number of matches to use
		CHECK_PARAMS(4, NO_FLAGS);
		cell
			*cptr;
		char
			*szSearch;
		// Get the search pattern
		amx_StrParam(amx, params[1], szSearch);
		if (szSearch)
		{
			// Get associated search information
			amx_GetAddr(amx, params[4], &cptr);
			cell
				count = *cptr;
			// Find the end of the directory name
			int
				end = strlen(szSearch) - 1;
			if (end == -1)
			{
				return 0;
			}
			while (szSearch[end] != '\\' && szSearch[end] != '/')
			{
				if (!end)
				{
					break;
				}
				end--;
			}
			// Split up the information
			// Ensure that we search in scriptfiles
			// And separate out the filename and path
			char
				*szDir = (char *)alloca(end + 16),
				*szFile;
			strcpy(szDir, "./scriptfiles/");
			if (end)
			{
				szFile = &szSearch[end + 1];
				szSearch[end] = '\0';
				strcpy(szDir + 14, szSearch);
				strcpy(szDir + strlen(szDir), "/");
			}
			else
			{
				szFile = szSearch;
			}
			end = strlen(szDir);
			DIR
				*dp = opendir(szDir);
			if (dp)
			{
				// Loop through all files in the directory
				struct dirent
					*ep;
				while (ep = readdir(dp))
				{
					// Check if this file matches the pattern
					if (!fnmatch(szFile, ep->d_name, FNM_NOESCAPE))
					{
						// Check if this is a directory
						// There MUST be an easier way to do this!
						char
							*full = (char *)malloc(strlen(ep->d_name) + end + 1);
						if (!full)
						{
							closedir(dp);
							return 0;
						}
						strcpy(full, szDir);
						strcpy(full + end, ep->d_name);
						DIR
							*xp = opendir(full);
						free(full);
						if (!xp)
						{
							continue;
						}
						closedir(xp);
						// Check if there's any left to skip
						if (!count)
						{
							// No files left to skip, return the data
							(*cptr)++;
							amx_GetAddr(amx, params[2], &cptr);
							amx_SetString(cptr, ep->d_name, 0, 0, params[3]);
							closedir(dp);
							return 1;
						}
						count--;
					}
				}
				closedir(dp);
				return 0;
			}
		}
		return 0;
	}
#endif

// native dcreate(const name[]);
AMX_DECLARE_NATIVE(Natives::dcreate)
{
	// Creates a directory
	CHECK_PARAMS(1, NO_FLAGS);
	char
		*szSearch;
	// Get the search pattern
	amx_StrParam(amx, params[1], szSearch);
	if (szSearch)
	{
		#ifdef _WIN32
			TCHAR
				szDir[MAX_PATH] = TEXT("./scriptfiles/");
			StringCchCat(szDir, MAX_PATH, szSearch);
			return (cell)CreateDirectory(szDir, NULL);
		#else
			char
				*szDir = (char *)alloca(strlen(szSearch) + 15);
			strcpy(szDir, "./scriptfiles/");
			strcpy(szDir + 14, szSearch);
			return (cell)mkdir(szDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		#endif
	}
	return 0;
}

// native frename(const oldname[], const newname[]);
AMX_DECLARE_NATIVE(Natives::frename)
{
	// Creates a directory
	CHECK_PARAMS(2, NO_FLAGS);
	char
		*szOld,
		*szNew;
	// Get the search pattern
	amx_StrParam(amx, params[1], szOld);
	amx_StrParam(amx, params[2], szNew);
	if (szOld && szNew)
	{
		char
			*szO = (char *)alloca(strlen(szOld) + 16),
			*szN = (char *)alloca(strlen(szNew) + 16);
		strcpy(szO, "./scriptfiles/");
		strcpy(szO + 14, szOld);
		strcpy(szN, "./scriptfiles/");
		strcpy(szN + 14, szNew);
		return (cell)rename(szO, szN);
	}
	return 0;
}

// native drename(const oldname[], const newname[]);
AMX_DECLARE_NATIVE(Natives::drename)
{
	// Creates a directory
	CHECK_PARAMS(2, NO_FLAGS);
	char
		*szOld,
		*szNew;
	// Get the search pattern
	amx_StrParam(amx, params[1], szOld);
	amx_StrParam(amx, params[2], szNew);
	if (szOld && szNew)
	{
		char
			*szO = (char *)alloca(strlen(szOld) + 16),
			*szN = (char *)alloca(strlen(szNew) + 16);
		strcpy(szO, "./scriptfiles/");
		strcpy(szO + 14, szOld);
		int
			end;
		end = strlen(szO);
		if (szO[end - 1] != '/')
		{
			szO[end] = '/';
			szO[end + 1] = '\0';
		}
		strcpy(szN, "./scriptfiles/");
		strcpy(szN + 14, szNew);
		end = strlen(szN);
		if (szN[end - 1] != '/')
		{
			szN[end] = '/';
			szN[end + 1] = '\0';
		}
		return (cell)rename(szO, szN);
	}
	return 0;
}

// native SetModeRestartTime(Float:time);
AMX_DECLARE_NATIVE(Natives::SetModeRestartTime)
{
	CHECK_PARAMS(1, LOADED);
	if (!CAddress::VAR_pRestartWaitTime) return 0;

	*(float*)CAddress::VAR_pRestartWaitTime = amx_ctof(params[1]);
	return 1;
}

// native Float:GetModeRestartTime();
AMX_DECLARE_NATIVE(Natives::GetModeRestartTime)
{
	if (!CServer::Get()->IsInitialized()) return std::numeric_limits<int>::lowest(); // If unknown server version
	if (!CAddress::VAR_pRestartWaitTime) return 0;

	float fRestartTime = *(float*)CAddress::VAR_pRestartWaitTime;
	return amx_ftoc(fRestartTime);
}

// native SetMaxPlayers(maxplayers);
AMX_DECLARE_NATIVE(Natives::SetMaxPlayers)
{
	CHECK_PARAMS(1, LOADED);

	const int maxplayers = CScriptParams::Get()->ReadInt();
	if(maxplayers < 1 || maxplayers > MAX_PLAYERS) return 0;

	CSAMPFunctions::SetIntVariable("maxplayers", maxplayers);
	return 1;
}

// native SetMaxNPCs(maxnpcs);
AMX_DECLARE_NATIVE(Natives::SetMaxNPCs)
{
	CHECK_PARAMS(1, LOADED);

	const int maxnpcs = CScriptParams::Get()->ReadInt();
	if(maxnpcs < 0 || maxnpcs > MAX_PLAYERS) return 0;

	CSAMPFunctions::SetIntVariable("maxnpc", maxnpcs);
	return 1;
}

// native GetSyncBounds(&Float:hmin, &Float:hmax, &Float:vmin, &Float:vmax);
AMX_DECLARE_NATIVE(Natives::GetSyncBounds)
{
	CHECK_PARAMS(4, LOADED);

	float fBounds[4];
	for (BYTE i = 0; i != 4; ++i)
		fBounds[i] = *(float*)CAddress::VAR_pPosSyncBounds[i];

	CScriptParams::Get()->Add(fBounds[0], fBounds[1], fBounds[2], fBounds[3]);
	return 1;
}

// native SetSyncBounds(Float:hmin, Float:hmax, Float:vmin, Float:vmax);
AMX_DECLARE_NATIVE(Natives::SetSyncBounds)
{
	CHECK_PARAMS(4, LOADED);

	for (BYTE i = 0; i != 4; ++i)
		*(float*)CAddress::VAR_pPosSyncBounds[i] = CScriptParams::Get()->ReadFloat();
	return 1;
}

// native SetPlayerAdmin(playerid, bool:admin);
AMX_DECLARE_NATIVE(Natives::SetPlayerAdmin)
{
	CHECK_PARAMS(2, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;

	pNetGame->pPlayerPool->bIsAnAdmin[playerid] = static_cast<BOOL>(CScriptParams::Get()->ReadInt());
	return 1;
}

// native LoadFilterScript(scriptname[]);
AMX_DECLARE_NATIVE(Natives::LoadFilterScript)
{
	CHECK_PARAMS(1, LOADED);
	
	std::string name;
	CScriptParams::Get()->Read(&name);
	if (!name.empty())
	{
		return CSAMPFunctions::LoadFilterscript(name.c_str());
	}
	return 0;
}

// UnLoadFilterScript(scriptname[]);
AMX_DECLARE_NATIVE(Natives::UnLoadFilterScript)
{
	CHECK_PARAMS(1, LOADED);
	
	std::string name;
	CScriptParams::Get()->Read(&name);
	if(!name.empty())
	{
		return CSAMPFunctions::UnLoadFilterscript(name.c_str());
	}
	return 0;
}

// native GetFilterScriptCount();
AMX_DECLARE_NATIVE(Natives::GetFilterScriptCount)
{
	if (!CServer::Get()->IsInitialized()) return std::numeric_limits<int>::lowest(); // If unknown server version

	return pNetGame->pFilterScriptPool->iFilterScriptCount;
}

// native GetFilterScriptName(id, name[], len = sizeof(name));
AMX_DECLARE_NATIVE(Natives::GetFilterScriptName)
{
	CHECK_PARAMS(3, LOADED);

	int id = CScriptParams::Get()->ReadInt();
	if(id < 0 || id >= MAX_FILTER_SCRIPTS) return 0;

	CScriptParams::Get()->Add(&pNetGame->pFilterScriptPool->szFilterScriptName[id][0]);
	return 1;
}

// native AddServerRule(name[], value[], flags = CON_VARFLAG_RULE);
AMX_DECLARE_NATIVE(Natives::AddServerRule)
{
	CHECK_PARAMS(3, LOADED);

	std::string name, value;
	CScriptParams::Get()->Read(&name, &value);

	if (!name.empty() && !value.empty())
	{
		ConsoleVariable_s* ConVar = CSAMPFunctions::FindVariable(const_cast<char*>(name.c_str()));
		if (ConVar == NULL)
		{
			CSAMPFunctions::AddStringVariable(const_cast<char*>(name.c_str()), CScriptParams::Get()->ReadInt(), const_cast<char*>(value.c_str()), NULL);
			return 1;
		}
	}
	return 0;
}

// native SetServerRule(name[], value[]);
AMX_DECLARE_NATIVE(Natives::SetServerRule)
{
	CHECK_PARAMS(2, LOADED);

	std::string name, value;
	CScriptParams::Get()->Read(&name, &value);
	if (!name.empty() && !value.empty())
	{
		ConsoleVariable_s* ConVar = CSAMPFunctions::FindVariable(const_cast<char*>(name.c_str()));
		if (ConVar != NULL)
		{
			CSAMPFunctions::SetStringVariable(const_cast<char*>(name.c_str()), const_cast<char*>(value.c_str()));
			return 1;
		}
	}
	return 0;
}

// native SetServerRuleInt(name[], value);
AMX_DECLARE_NATIVE(Natives::SetServerRuleInt)
{
	CHECK_PARAMS(2, LOADED);

	std::string name;
	CScriptParams::Get()->Read(&name);
	if (!name.empty())
	{
		ConsoleVariable_s* ConVar = CSAMPFunctions::FindVariable(const_cast<char*>(name.c_str()));
		if (ConVar != NULL)
		{
			CSAMPFunctions::SetIntVariable(const_cast<char*>(name.c_str()), CScriptParams::Get()->ReadInt());
			return 1;
		}
		return 1;
	}
	return 0;
}

// native IsValidServerRule(name[]);
AMX_DECLARE_NATIVE(Natives::IsValidServerRule)
{
	CHECK_PARAMS(1, LOADED);

	std::string name;
	CScriptParams::Get()->Read(&name);
	if (!name.empty())
	{
		ConsoleVariable_s* ConVar = CSAMPFunctions::FindVariable(const_cast<char*>(name.c_str()));
		return ConVar != NULL;
	}
	return 0;
}

// native RemoveServerRule(name[]);
AMX_DECLARE_NATIVE(Natives::RemoveServerRule)
{
	CHECK_PARAMS(1, LOADED);

	std::string name;
	CScriptParams::Get()->Read(&name);
	if (!name.empty())
	{
		//RemoveServerRule(name.c_str());
		return 1;
	}
	return 0;
}

// native SetServerRuleFlags(name[], flags);
AMX_DECLARE_NATIVE(Natives::SetServerRuleFlags)
{
	CHECK_PARAMS(2, LOADED);
	
	std::string name;
	CScriptParams::Get()->Read(&name);
	if (!name.empty())
	{
		CSAMPFunctions::ModifyVariableFlags(const_cast<char*>(name.c_str()), (DWORD)params[2]);
		return 1;
	}
	return 0;
}

// native GetServerRuleFlags(name[]);
AMX_DECLARE_NATIVE(Natives::GetServerRuleFlags)
{
	CHECK_PARAMS(1, LOADED);
	
	std::string name;
	CScriptParams::Get()->Read(&name);

	ConsoleVariable_s* ConVar = CSAMPFunctions::FindVariable(const_cast<char*>(name.c_str()));
	if (ConVar != NULL)
	{
		return ConVar->VarFlags;
	}
	return 0;
}

// native GetServerSettings(&showplayermarkes, &shownametags, &stuntbonus, &useplayerpedanims, &bLimitchatradius, &disableinteriorenterexits, &nametaglos, &manualvehicleengine, 
//		&limitplayermarkers, &vehiclefriendlyfire, &defaultcameracollision, &Float:fGlobalchatradius, &Float:fNameTagDrawDistance, &Float:fPlayermarkerslimit);
AMX_DECLARE_NATIVE(Natives::GetServerSettings)
{
	CHECK_PARAMS(14, LOADED);

	CScriptParams::Get()->Add(
		  pNetGame->bShowPlayerMarkers
		, pNetGame->byteShowNameTags
		, pNetGame->byteStuntBonus
		, pNetGame->bUseCJWalk
		, pNetGame->bLimitGlobalChatRadius
		, pNetGame->byteDisableEnterExits
		, pNetGame->byteNameTagLOS
		, pNetGame->bManulVehicleEngineAndLights
		, pNetGame->bLimitPlayerMarkers
		, pNetGame->bVehicleFriendlyFire
		, pNetGame->byteDefaultCameraCollision
		, pNetGame->fGlobalChatRadius
		, pNetGame->fNameTagDrawDistance
		, pNetGame->fPlayerMarkesLimit
	);
	return 1;
}

// native GetNPCCommandLine(npcid, npcscript[], length = sizeof(npcscript));
AMX_DECLARE_NATIVE(Natives::GetNPCCommandLine)
{
	CHECK_PARAMS(3, LOADED);

	const int npcid = CScriptParams::Get()->ReadInt();
	if (!IsPlayerConnected(npcid)) return 0;

	char *szCommandLine = CServer::Get()->GetNPCCommandLine(static_cast<WORD>(npcid));
	if (szCommandLine == NULL) return 0;

	CScriptParams::Get()->Add(&szCommandLine[0]);
	return 1;
}

// native GetRCONCommandName(const cmdname[], changedname[]);
AMX_DECLARE_NATIVE(Natives::ChangeRCONCommandName)
{
	CHECK_PARAMS(2, LOADED);

	std::string name, newname;
	CScriptParams::Get()->Read(&name, &newname);

	return CServer::Get()->ChangeRCONCommandName(name, newname);
}

// native GetRCONCommandName(const cmdname[], changedname[], len = sizeof(changedname));
AMX_DECLARE_NATIVE(Natives::GetRCONCommandName)
{
	CHECK_PARAMS(3, LOADED);

	std::string name;
	CScriptParams::Get()->Read(&name);

	std::string changedname;
	bool ret = CServer::Get()->GetRCONCommandName(name, changedname);
	CScriptParams::Get()->Add(changedname);
	return ret;
}

// This function based on maddinat0r's function - Thanks (MySQL plugin/CCallback.cpp)
// native CallFunctionInScript(const scriptname[], const function[], const format[], {Float,_}:...);
AMX_DECLARE_NATIVE(Natives::CallFunctionInScript)
{
	if (CScriptParams::Get()->Setup(3, "CallFunctionInScript", CScriptParams::Flags::MORE_PARAMETER_ALLOWED, amx, params)) return CScriptParams::Get()->HandleError();

	std::string scriptname, function, formatparams;
	CScriptParams::Get()->Read(&scriptname, &function, &formatparams);

	AMX* pAMX = nullptr;
	if(scriptname == "GameMode")
	{
		pAMX = &pNetGame->pGameModePool->amx;
	}
	else
	{
		for (BYTE i = 0; i != 16; ++i)
		{
			if (scriptname == pNetGame->pFilterScriptPool->szFilterScriptName[i])
			{
				pAMX = pNetGame->pFilterScriptPool->pFilterScripts[i];
				break;
			}
		}
	}
	
	if (pAMX == nullptr)
	{
		logprintf("script \"%s\" does not exist", scriptname.c_str());
		return 0;
	}
	
	int cb_idx = -1;
	if (amx_FindPublic(pAMX, function.c_str(), &cb_idx) != AMX_ERR_NONE)
	{
		logprintf("callback \"%s\" does not exist", function.c_str());
		return 0;
	}
	
	size_t len = formatparams.length();
	char* format = new char[len + 1];
	strcpy(format, formatparams.c_str());

	const size_t param_offset = 4;
	const size_t num_params = len;

	if ((params[0] / sizeof(cell) - (param_offset - 1)) != num_params)
	{
		logprintf("parameter count does not match format specifier length %d - %d", num_params, (params[0] / sizeof(cell) - (param_offset - 1)));
		return 0;
	}

	cell param_idx = len - 1;
	cell *address_ptr = nullptr;
	cell *array_addr_ptr = nullptr;
	cell amx_address = -1;
	do
	{
		cell tmp_addr;		
		switch (*(format + (len - 1)))
		{
			case 'd': //decimal
			case 'i': //integer
			case 'b':
			case 'f':
			{
				amx_GetAddr(amx, params[param_offset + param_idx], &address_ptr);
				cell value = *address_ptr;
				amx_Push(pAMX, value);
			}
			break;
			case 's': //string
			{
				char *str = nullptr;
				amx_StrParam(amx, params[param_offset + param_idx], str);
				
				if (str == nullptr)
				{
					str = new char[5];
					strcpy(str, "NULL");
				}

				amx_PushString(pAMX, &tmp_addr, nullptr, str, 0, 0);

				delete[] str;
			}
			break;
			case 'a': //array
			{
				cell *arraySize;
				amx_GetAddr(amx, params[param_offset + param_idx], &array_addr_ptr);
				if(amx_GetAddr(amx, params[param_offset + (param_idx + 1)], &arraySize) != AMX_ERR_NONE)
				{
					logprintf("missing 'd' / 'i' specifier for array size");
					return 0;
				}

				if ((*(format + (len))) != 'd' && (*(format + (len))) != 'i')
				{
					logprintf("expected 'd'/'i' specifier for array size (got '%c' instead)", *(format + (len)));
					return 0;
				}

				if (arraySize <= 0)
				{
					logprintf("invalid array size '%d'", arraySize);
					return 0;
				}

				cell *copied_array = static_cast<cell *>(malloc(*arraySize * sizeof(cell)));
				memcpy(copied_array, array_addr_ptr, *arraySize * sizeof(cell));

				amx_PushArray(pAMX, &tmp_addr, nullptr, copied_array, *arraySize);
				free(copied_array);

				if (amx_address < 0)
					amx_address = tmp_addr;

				array_addr_ptr = nullptr;
			}
			break;
			default:
			{
				logprintf("invalid format specifier '%c'", *(format + (len - 1)));
				return 0;
				break;
			}
		}
		param_idx--;
		len--;
	} 
	while (len);
	
	cell ret;
	amx_Exec(pAMX, &ret, cb_idx);

	if (amx_address >= 0)
		amx_Release(pAMX, amx_address);

	delete[] format;
	return ret;
}

// native EnableConsoleMSGsForPlayer(playerid, color);
AMX_DECLARE_NATIVE(Natives::EnableConsoleMSGsForPlayer)
{
	CHECK_PARAMS(2, LOADED);

	int playerid, color;
	CScriptParams::Get()->Read(&playerid, &color);
	if (!IsPlayerConnected(playerid)) return 0;

	CServer::Get()->AddConsolePlayer(static_cast<WORD>(playerid), static_cast<DWORD>(color));
	return 1;
}

// native DisableConsoleMSGsForPlayer(playerid);
AMX_DECLARE_NATIVE(Natives::DisableConsoleMSGsForPlayer)
{
	CHECK_PARAMS(1, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if (!IsPlayerConnected(playerid)) return 0;

	CServer::Get()->RemoveConsolePlayer(static_cast<WORD>(playerid));
	return 1;
}

// native HasPlayerConsoleMessages(playerid, &color = 0);
AMX_DECLARE_NATIVE(Natives::HasPlayerConsoleMessages)
{
	CHECK_PARAMS(2, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if (!IsPlayerConnected(playerid)) return 0;

	DWORD color;
	bool ret = CServer::Get()->IsConsolePlayer(static_cast<WORD>(playerid), color);
	CScriptParams::Get()->Add(color);
	return ret;
}

// native IsValidNickName(name[]);
AMX_DECLARE_NATIVE(Natives::IsValidNickName)
{
	CHECK_PARAMS(1, LOADED);

	std::string name;
	CScriptParams::Get()->Read(&name);
	return CServer::Get()->IsValidNick(const_cast<char*>(name.c_str()));
}

// native AllowNickNameCharacter(character, bool:allow);
AMX_DECLARE_NATIVE(Natives::AllowNickNameCharacter)
{
	CHECK_PARAMS(2, LOADED);
	
	const char character = static_cast<const char>(params[1]);

	// Enable %s is disallowed
	if(character == '%') return 0;

	CServer::Get()->AllowNickNameCharacter(character, static_cast<int>(params[2]) != 0);
	return 1;
}

// native IsNickNameCharacterAllowed(character);
AMX_DECLARE_NATIVE(Natives::IsNickNameCharacterAllowed)
{
	CHECK_PARAMS(1, LOADED);

	return CServer::Get()->IsNickNameCharacterAllowed(static_cast<char>(params[1]));
}

/////////////// Timers

// native GetAvailableClasses();
AMX_DECLARE_NATIVE(Natives::GetAvailableClasses)
{
	if (!CServer::Get()->IsInitialized()) return std::numeric_limits<int>::lowest(); // If unknown server version

	return pNetGame->iSpawnsAvailable;
}

// native RemoveLastClass();
AMX_DECLARE_NATIVE(Natives::RemoveLastClass)
{
	if (!CServer::Get()->IsInitialized()) return std::numeric_limits<int>::lowest(); // If unknown server version

	if(pNetGame->iSpawnsAvailable <= 0)
		return 0;

	pNetGame->iSpawnsAvailable--;
	return 1;
}

// native GetPlayerClass(classid, &teamid, &modelid, &Float:spawn_x, &Float:spawn_y, &Float:spawn_z, &Float:z_angle, &weapon1, &weapon1_ammo, &weapon2, &weapon2_ammo,& weapon3, &weapon3_ammo);
AMX_DECLARE_NATIVE(Natives::GetPlayerClass)
{
	CHECK_PARAMS(13, LOADED);

	const int classid = CScriptParams::Get()->ReadInt();
	if(classid < 0 || classid > pNetGame->iSpawnsAvailable) return 0;

	CPlayerSpawnInfo *pSpawn = &pNetGame->AvailableSpawns[classid];

	CScriptParams::Get()->Add(pSpawn->byteTeam, pSpawn->iSkin, pSpawn->vecPos, pSpawn->fRotation,
		pSpawn->iSpawnWeapons[0], pSpawn->iSpawnWeaponsAmmo[0],
		pSpawn->iSpawnWeapons[1], pSpawn->iSpawnWeaponsAmmo[1],
		pSpawn->iSpawnWeapons[2], pSpawn->iSpawnWeaponsAmmo[2]);
	return 1;
}

// native EditPlayerClass(classid, teamid, modelid, Float:spawn_x, Float:spawn_y, Float:spawn_z, Float:z_angle, weapon1, weapon1_ammo, weapon2, weapon2_ammo, weapon3, weapon3_ammo);
AMX_DECLARE_NATIVE(Natives::EditPlayerClass)
{
	CHECK_PARAMS(13, LOADED);

	const int classid = CScriptParams::Get()->ReadInt();
	if (classid < 0 || classid > pNetGame->iSpawnsAvailable) return 0;

	CPlayerSpawnInfo *pSpawn = &pNetGame->AvailableSpawns[classid];

	CScriptParams::Get()->Read(&pSpawn->byteTeam, &pSpawn->iSkin, &pSpawn->vecPos, &pSpawn->fRotation,
		&pSpawn->iSpawnWeapons[0], &pSpawn->iSpawnWeaponsAmmo[0],
		&pSpawn->iSpawnWeapons[1], &pSpawn->iSpawnWeaponsAmmo[1],
		&pSpawn->iSpawnWeapons[2], &pSpawn->iSpawnWeaponsAmmo[2]);
	return 1;
}

// native GetActiveTimers();
AMX_DECLARE_NATIVE(Natives::GetRunningTimers)
{
	if (!CServer::Get()->IsInitialized()) return std::numeric_limits<int>::lowest(); // If unknown server version

	return pNetGame->pScriptTimers->dwTimerCount;
}

// native Float:GetGravity();
AMX_DECLARE_NATIVE(Natives::YSF_GetGravity)
{
	if (!CServer::Get()->IsInitialized()) return std::numeric_limits<int>::lowest(); // If unknown server version

	float fGravity = pNetGame->fGravity;
	return amx_ftoc(fGravity);
}

// native SetPlayerGravity(playerid, Float:gravity);
AMX_DECLARE_NATIVE(Natives::SetPlayerGravity)
{
	CHECK_PARAMS(2, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;

	// Update stored values
	pPlayerData[playerid]->fGravity = CScriptParams::Get()->ReadFloat();

	RakNet::BitStream bs;
	bs.Write(pPlayerData[playerid]->fGravity);
	CSAMPFunctions::RPC(&RPC_Gravity, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, CSAMPFunctions::GetPlayerIDFromIndex(playerid), 0, 0);
	return 1;
}

// native Float:GetPlayerGravity(playerid);
AMX_DECLARE_NATIVE(Natives::GetPlayerGravity)
{
	CHECK_PARAMS(1, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;

	return amx_ftoc(pPlayerData[playerid]->fGravity);
}

// native SetPlayerTeamForPlayer(playerid, teamplayerid, teamid);
AMX_DECLARE_NATIVE(Natives::SetPlayerTeamForPlayer)
{
	CHECK_PARAMS(3, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int teamplayerid = CScriptParams::Get()->ReadInt();
	const int team = CScriptParams::Get()->ReadInt();

	if (!IsPlayerConnected(playerid) || !IsPlayerConnected(teamplayerid)) return 0;
	if (team < 0 || team > NO_TEAM) return 0;

	pPlayerData[playerid]->SetPlayerTeamForPlayer(static_cast<WORD>(teamplayerid), team);
	return 1;
}

// native GetPlayerTeamForPlayer(playerid, teamplayerid);
AMX_DECLARE_NATIVE(Natives::GetPlayerTeamForPlayer)
{
	CHECK_PARAMS(2, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int teamplayerid = CScriptParams::Get()->ReadInt();

	if (!IsPlayerConnected(playerid) || !IsPlayerConnected(teamplayerid)) return 0;

	return pPlayerData[playerid]->GetPlayerTeamForPlayer(static_cast<WORD>(teamplayerid));
}

AMX_DECLARE_NATIVE(Natives::YSF_SetPlayerTeam)
{
	CHECK_PARAMS(2, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if(pSetPlayerTeam(amx, params))
	{
		for(WORD i = 0; i != MAX_PLAYERS; ++i)
		{
			if(IsPlayerConnected(i))
				pPlayerData[i]->ResetPlayerTeam(static_cast<WORD>(playerid));
		}
		return 1;
	}
	return 0;
}

// native SetPlayerSkinForPlayer(playerid, skinplayerid, skin);
AMX_DECLARE_NATIVE(Natives::SetPlayerSkinForPlayer)
{
	CHECK_PARAMS(3, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int skinplayerid = CScriptParams::Get()->ReadInt();
	const int skin = CScriptParams::Get()->ReadInt();

	if (!IsPlayerConnected(playerid) || !IsPlayerConnected(skinplayerid)) return 0;
	if (skin < 0 || skin > 300) return 0;

	pPlayerData[playerid]->SetPlayerSkinForPlayer(static_cast<WORD>(skinplayerid), skin);
	return 1;
}

// native GetPlayerSkinForPlayer(playerid, skinplayerid);
AMX_DECLARE_NATIVE(Natives::GetPlayerSkinForPlayer)
{
	CHECK_PARAMS(2, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int skinplayerid = CScriptParams::Get()->ReadInt();

	if (!IsPlayerConnected(playerid) || !IsPlayerConnected(skinplayerid)) return 0;

	return pPlayerData[playerid]->GetPlayerSkinForPlayer(static_cast<WORD>(skinplayerid));
}

AMX_DECLARE_NATIVE(Natives::YSF_SetPlayerSkin)
{
	CHECK_PARAMS(2, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if(pSetPlayerSkin(amx, params))
	{
		for(WORD i = 0; i != MAX_PLAYERS; ++i)
		{
			if(IsPlayerConnected(i))
				pPlayerData[i]->ResetPlayerSkin(static_cast<WORD>(playerid));
		}
		return 1;
	}
	return 0;
}

// native SetPlayerNameForPlayer(playerid, nameplayerid, playername[]);
AMX_DECLARE_NATIVE(Natives::SetPlayerNameForPlayer)
{
	CHECK_PARAMS(3, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int nameplayerid = CScriptParams::Get()->ReadInt();
	
	std::string name;
	CScriptParams::Get()->Read(&name);

	if (!IsPlayerConnected(playerid) || !IsPlayerConnected(nameplayerid)) return 0;

	pPlayerData[playerid]->SetPlayerNameForPlayer(static_cast<WORD>(nameplayerid), name.c_str());
	return 1;
}

// native GetPlayerNameForPlayer(playerid, nameplayerid, playername[], size = sizeof(playername));
AMX_DECLARE_NATIVE(Natives::GetPlayerNameForPlayer)
{
	CHECK_PARAMS(4, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int nameplayerid = CScriptParams::Get()->ReadInt();

	if (!IsPlayerConnected(playerid) || !IsPlayerConnected(nameplayerid)) return 0;

	CScriptParams::Get()->Add((char*)&pPlayerData[playerid]->GetPlayerNameForPlayer(static_cast<WORD>(nameplayerid))[0]);
	return 1;
}

AMX_DECLARE_NATIVE(Natives::YSF_SetPlayerName)
{
	CHECK_PARAMS(2, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int ret = pSetPlayerName(amx, params);

	if(ret == 1)
	{
		for(WORD i = 0; i != MAX_PLAYERS; ++i)
		{
			if(IsPlayerConnected(i))
				pPlayerData[i]->ResetPlayerName(static_cast<WORD>(playerid));
		}
	}
	return ret;
}

// native SetPlayerFightStyleForPlayer(playerid, styleplayerid, style);
AMX_DECLARE_NATIVE(Natives::SetPlayerFightStyleForPlayer)
{
	CHECK_PARAMS(3, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int styleplayerid = CScriptParams::Get()->ReadInt();
	const int style = CScriptParams::Get()->ReadInt();

	if (!IsPlayerConnected(playerid) || !IsPlayerConnected(styleplayerid)) return 0;

	pPlayerData[playerid]->SetPlayerFightingStyleForPlayer(static_cast<WORD>(styleplayerid), style);
	return 1;
}

// native GetPlayerFightStyleForPlayer(playerid, skinplayerid);
AMX_DECLARE_NATIVE(Natives::GetPlayerFightStyleForPlayer)
{
	CHECK_PARAMS(2, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int styleplayerid = CScriptParams::Get()->ReadInt();

	if (!IsPlayerConnected(playerid) || !IsPlayerConnected(styleplayerid)) return 0;

	return pPlayerData[playerid]->GetPlayerFightingStyleForPlayer(static_cast<WORD>(styleplayerid));
}

AMX_DECLARE_NATIVE(Natives::YSF_SetPlayerFightingStyle)
{
	CHECK_PARAMS(2, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if(pSetPlayerFightingStyle(amx, params))
	{
		for(WORD i = 0; i != MAX_PLAYERS; ++i)
		{
			if(IsPlayerConnected(i))
				pPlayerData[i]->ResetPlayerFightingStyle(static_cast<WORD>(playerid));
		}
		return 1;
	}
	return 0;
}

// native SetPlayerPosForPlayer(playerid, posplayerid, Float:fX, Float:fY, Float:fZ, bool:forcesync = true);
AMX_DECLARE_NATIVE(Natives::SetPlayerPosForPlayer)
{
	CHECK_PARAMS(6, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int posplayerid = CScriptParams::Get()->ReadInt();
	const bool forcesync = static_cast<int>(params[6]) != 0;
	if (!IsPlayerConnected(playerid) || !IsPlayerConnected(posplayerid)) return 0;

	if(!forcesync)
	{
		if(pPlayerData[playerid]->customPos.find(posplayerid) != pPlayerData[playerid]->customPos.end())
		{
			pPlayerData[playerid]->customPos.erase(posplayerid);
		}
		return 1;
	}

	CVector vecPos;
	CScriptParams::Get()->Read(&vecPos);
	
	pPlayerData[playerid]->customPos[posplayerid] = std::make_unique<CVector>(std::move(vecPos));
	return 1;
}

// native SetPlayerRotationQuatForPlayer(playerid, quatplayerid, Float:w, Float:x, Float:y, Float:z, bool:forcesync = true);
AMX_DECLARE_NATIVE(Natives::SetPlayerRotationQuatForPlayer)
{
	CHECK_PARAMS(7, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int posplayerid = CScriptParams::Get()->ReadInt();
	const bool forcesync = static_cast<int>(params[7]) != 0;
	if (!IsPlayerConnected(playerid) || !IsPlayerConnected(posplayerid)) return 0;

	if(!forcesync)
	{
		pPlayerData[playerid]->bCustomQuat[posplayerid] = false;
		return 1;
	}
	
	CPlayer *p = pNetGame->pPlayerPool->pPlayer[playerid];

	CScriptParams::Get()->Read(&pPlayerData[playerid]->fCustomQuat[posplayerid][0], &pPlayerData[playerid]->fCustomQuat[posplayerid][1], &pPlayerData[playerid]->fCustomQuat[posplayerid][2], &pPlayerData[playerid]->fCustomQuat[posplayerid][3]);
	pPlayerData[playerid]->bCustomQuat[posplayerid] = true;
	return 1;
}

// native ApplyAnimationForPlayer(playerid, animplayerid, animlib[], animname[], Float:fDelta, loop, lockx, locky, freeze, time);
AMX_DECLARE_NATIVE(Natives::ApplyAnimationForPlayer)
{
	CHECK_PARAMS(10, LOADED);
	
	RakNet::BitStream bsSend;
	char *szAnimLib;
	char *szAnimName;
	BYTE byteAnimLibLen;
	BYTE byteAnimNameLen;
	float fS;
	bool opt1,opt2,opt3,opt4;
	int time;
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int animplayerid = CScriptParams::Get()->ReadInt();
	if (!IsPlayerConnected(playerid) || !IsPlayerConnected(animplayerid)) return 0;

	amx_StrParam(amx, params[3], szAnimLib);
	amx_StrParam(amx, params[4], szAnimName);

	if(!szAnimLib || !szAnimName) return 0;

	byteAnimLibLen = static_cast<BYTE>(strlen(szAnimLib));
	byteAnimNameLen = static_cast<BYTE>(strlen(szAnimName));

	fS = amx_ctof(params[5]);
	opt1 = !!params[6];
	opt2 = !!params[7];
	opt3 = !!params[8];
	opt4 = !!params[9];
	time = static_cast<int>(params[10]);

	bsSend.Write((WORD)animplayerid);
	bsSend.Write(byteAnimLibLen);
	bsSend.Write(szAnimLib,byteAnimLibLen);
	bsSend.Write(byteAnimNameLen);
	bsSend.Write(szAnimName,byteAnimNameLen);
	bsSend.Write(fS);
	bsSend.Write(opt1);
	bsSend.Write(opt2);
	bsSend.Write(opt3);
	bsSend.Write(opt4);
	bsSend.Write(time);

	CSAMPFunctions::RPC(&RPC_ScrApplyAnimation, &bsSend, MEDIUM_PRIORITY, UNRELIABLE, 0, CSAMPFunctions::GetPlayerIDFromIndex(playerid), false, false);
	return 1;
}

// native SetPlayerWeather(playerid, weatherid);
AMX_DECLARE_NATIVE(Natives::YSF_SetPlayerWeather)
{
	CHECK_PARAMS(2, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	
	if (pSetPlayerWeather(amx, params) && IsPlayerConnected(playerid))
	{
		pPlayerData[playerid]->byteWeather = static_cast<BYTE>(CScriptParams::Get()->ReadInt());
		return 1;
	}
	return 0;
}

// native GetPlayerWeather(playerid);
AMX_DECLARE_NATIVE(Natives::GetPlayerWeather)
{
	CHECK_PARAMS(1, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;

	return pPlayerData[playerid]->byteWeather;
}

// native SetPlayerWorldBounds(playerid, Float:x_max, Float:x_min, Float:y_max, Float:y_min)
AMX_DECLARE_NATIVE(Natives::YSF_SetPlayerWorldBounds)
{
	CHECK_PARAMS(5, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if(pSetPlayerWorldBounds(amx, params) && IsPlayerConnected(playerid))
	{
		for (BYTE i = 0; i != 4; ++i)
		{
			pPlayerData[playerid]->fBounds[i] = CScriptParams::Get()->ReadFloat();
		}
		return 1;
	}
	return 0;
}

// native DestroyObject(objectid)
AMX_DECLARE_NATIVE(Natives::YSF_DestroyObject)
{
	CHECK_PARAMS(1, LOADED);

	const int objectid = CScriptParams::Get()->ReadInt();

	if(objectid < 0 || objectid > MAX_OBJECTS) return 0;
	if(!pNetGame->pObjectPool->pObjects[objectid]) return 0;

	if(pDestroyObject(amx, params))
	{
		CServer::Get()->COBJECT_AttachedObjectPlayer[objectid] = INVALID_PLAYER_ID;
		return 1;
	}
	return 0;
}

// native DestroyPlayerObject(playerid, objectid)
AMX_DECLARE_NATIVE(Natives::YSF_DestroyPlayerObject)
{
	CHECK_PARAMS(2, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int objectid = CScriptParams::Get()->ReadInt();

	if(objectid < 0 || objectid > MAX_OBJECTS) return 0;
	if(!pNetGame->pObjectPool->bPlayerObjectSlotState[playerid][objectid]) return 0;
	
	if (IsPlayerConnected(playerid))
	{
		for(std::multimap<WORD, std::pair<BYTE, std::string>>::iterator o = pPlayerData[playerid]->m_PlayerObjectMaterialText.begin(); o != pPlayerData[playerid]->m_PlayerObjectMaterialText.end(); ++o)
		{
			if(o->first == objectid)
			{
				o = pPlayerData[playerid]->m_PlayerObjectMaterialText.erase(o);
			}
		}
		pPlayerData[playerid]->DeleteObjectAddon(static_cast<WORD>(objectid));
	}

	if(pDestroyPlayerObject(amx, params))
	{
		return 1;
	}
	return 0;
}

// native TogglePlayerControllable(playerid, bool:toggle)
AMX_DECLARE_NATIVE(Natives::YSF_TogglePlayerControllable)
{
	CHECK_PARAMS(2, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const bool toggle = CScriptParams::Get()->ReadBool();

	if(pTogglePlayerControllable(amx, params) && IsPlayerConnected(playerid))
	{
		pPlayerData[playerid]->bControllable = toggle;
		return 1;
	}
	return 0;
}

// native ChangeVehicleColor(vehicleid, color1, color2)
AMX_DECLARE_NATIVE(Natives::YSF_ChangeVehicleColor)
{
	CHECK_PARAMS(3, LOADED);

	const int vehicleid = CScriptParams::Get()->ReadInt();
	if(pChangeVehicleColor(amx, params))
	{
		CServer::Get()->bChangedVehicleColor[vehicleid] = true;
		return 1;
	}
	return 0;
}

// native DestroyVehicle(vehicleid);
AMX_DECLARE_NATIVE(Natives::YSF_DestroyVehicle)
{
	CHECK_PARAMS(1, LOADED);

	const int vehicleid = CScriptParams::Get()->ReadInt();
	if(pDestroyVehicle(amx, params))
	{
		CServer::Get()->bChangedVehicleColor[vehicleid] = false;
		auto v = CServer::Get()->vehicleSpawnData.find(vehicleid);
		if (v != CServer::Get()->vehicleSpawnData.end())
		{
			CServer::Get()->vehicleSpawnData.erase(v);
		}
		return 1;
	}
	return 0;
}

// native ShowPlayerDialog(playerid, dialogid, style, caption[], info[], button1[], button2[]);
AMX_DECLARE_NATIVE(Natives::YSF_ShowPlayerDialog)
{
	CHECK_PARAMS(7, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int dialogid = CScriptParams::Get()->ReadInt();

	if (pShowPlayerDialog(amx, params) && IsPlayerConnected(playerid))
	{
		pPlayerData[playerid]->wDialogID = dialogid;
		return 1;
	}
	return 0;
}

// native SetPlayerObjectMaterial(playerid, objectid, materialindex, modelid, txdname[], texturename[], materialcolor=0);
AMX_DECLARE_NATIVE(Natives::YSF_SetPlayerObjectMaterial)
{
	CHECK_PARAMS(7, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();

	if (pSetPlayerObjectMaterial(amx, params) && IsPlayerConnected(playerid))
	{
		const int objectid = CScriptParams::Get()->ReadInt();

		CObject *pObject = pNetGame->pObjectPool->pPlayerObjects[playerid][objectid];
		int index = pObject->dwMaterialCount;
		if (index < MAX_OBJECT_MATERIAL)
		{
			BYTE slot;
			WORD modelid;
			DWORD color;
			std::string szTXD, szTexture;
			CScriptParams::Get()->Read(&slot, &modelid, &szTXD, &szTexture, &color);
			/*
			if (pObject->szMaterialText[index])
			{
				free(pObject->szMaterialText[index]);
				pObject->szMaterialText[index] = NULL;
			}
			*/

			for (std::multimap<WORD, std::pair<BYTE, std::string>>::iterator o = pPlayerData[playerid]->m_PlayerObjectMaterialText.begin(); o != pPlayerData[playerid]->m_PlayerObjectMaterialText.end(); ++o)
			{
				if (o->first == objectid)
				{
					o = pPlayerData[playerid]->m_PlayerObjectMaterialText.erase(o);
				}
			}

			pObject->Material[index].byteSlot = slot;
			pObject->Material[index].wModelID = modelid;
			pObject->Material[index].byteUsed = 1;
			pObject->Material[index].dwMaterialColor = color;

			strncpy(pObject->Material[index].szMaterialTXD, szTXD.c_str(), 64u);
			strncpy(pObject->Material[index].szMaterialTexture, szTexture.c_str(), 64u);
			pObject->dwMaterialCount++;
		}
		return 1;
	}
	return 0;
}

// native SetPlayerObjectMaterialText(playerid, objectid, text[], materialindex = 0, materialsize = OBJECT_MATERIAL_SIZE_256x128, fontface[] = "Arial", fontsize = 24, bold = 1, fontcolor = 0xFFFFFFFF, backcolor = 0, textalignment = 0);
AMX_DECLARE_NATIVE(Natives::YSF_SetPlayerObjectMaterialText)
{
	CHECK_PARAMS(11, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();

	if (pSetPlayerObjectMaterialText(amx, params) && IsPlayerConnected(playerid))
	{
		const int objectid = CScriptParams::Get()->ReadInt();

		CObject *pObject = pNetGame->pObjectPool->pPlayerObjects[playerid][objectid];
		int index = pObject->dwMaterialCount;
		if (index < MAX_OBJECT_MATERIAL && CServer::Get()->m_bStorePlayerObjectsMaterial)
		{
			std::string szText, szFontFace;
			BYTE slot, materialsize, fontsize, bold, textalignment;
			DWORD fontcolor, backcolor;
			CScriptParams::Get()->Read(&szText, &slot, &materialsize, &szFontFace, &fontsize, &bold, &fontcolor, &backcolor, &textalignment);

			/*
			if (pObject->szMaterialText[index])
				free(pObject->szMaterialText[index]);
			
			pObject->szMaterialText[index] = (char *)calloc(1u, szText.length() + 1);
			strcpy(pObject->szMaterialText[index], szText.c_str());
			*/

			WORD objid = objectid;
			pPlayerData[playerid]->m_PlayerObjectMaterialText.emplace(objid, std::make_pair((BYTE)slot, std::move(szText)));
			pObject->Material[index].byteSlot = slot;
			pObject->Material[index].byteUsed = 2;
			pObject->Material[index].byteMaterialSize = materialsize;
			strncpy(pObject->Material[index].szFont, szFontFace.c_str(), 64u);
			pObject->Material[index].byteBold = bold;
			pObject->Material[index].byteFontSize = fontsize;
			pObject->Material[index].byteAlignment = textalignment;
			pObject->Material[index].dwFontColor = fontcolor;
			pObject->Material[index].dwBackgroundColor = backcolor;
			pObject->dwMaterialCount++;
		}
		return 1;
	}
	return 0;
}

// native TogglePlayerWidescreen(playerid, bool:set);
AMX_DECLARE_NATIVE(Natives::TogglePlayerWidescreen)
{
	CHECK_PARAMS(2, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;

	BYTE set = static_cast<BYTE>(CScriptParams::Get()->ReadInt()) != 0;
	pPlayerData[playerid]->bWidescreen = !!set;

	RakNet::BitStream bs;
	bs.Write(set);
	CSAMPFunctions::RPC(&RPC_Widescreen, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, CSAMPFunctions::GetPlayerIDFromIndex(playerid), 0, 0);
	return 1;
}

// native IsPlayerWidescreenToggled(playerid);
AMX_DECLARE_NATIVE(Natives::IsPlayerWidescreenToggled)
{
	CHECK_PARAMS(1, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;

	return pPlayerData[playerid]->bWidescreen;
}

// native GetSpawnInfo(playerid, &teamid, &modelid, &Float:spawn_x, &Float:spawn_y, &Float:spawn_z, &Float:z_angle, &weapon1, &weapon1_ammo, &weapon2, &weapon2_ammo,& weapon3, &weapon3_ammo);
AMX_DECLARE_NATIVE(Natives::GetSpawnInfo)
{
	CHECK_PARAMS(13, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;

	CPlayerSpawnInfo *pSpawn = &pNetGame->pPlayerPool->pPlayer[playerid]->spawn;
	CScriptParams::Get()->Add(pSpawn->byteTeam, pSpawn->iSkin, pSpawn->vecPos, pSpawn->fRotation,
		pSpawn->iSpawnWeapons[0], pSpawn->iSpawnWeaponsAmmo[0],
		pSpawn->iSpawnWeapons[1], pSpawn->iSpawnWeaponsAmmo[1],
		pSpawn->iSpawnWeapons[2], pSpawn->iSpawnWeaponsAmmo[2]);
	return 1;
}

// native GetPlayerSkillLevel(playerid, skill);
AMX_DECLARE_NATIVE(Natives::GetPlayerSkillLevel)
{
	CHECK_PARAMS(2, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int skillid = CScriptParams::Get()->ReadInt();
	
	if(!IsPlayerConnected(playerid)) return 0;
	if(skillid < 0 || skillid > 10) return 0;

	return pNetGame->pPlayerPool->pPlayer[playerid]->wSkillLevel[skillid];
}

// native IsPlayerCheckpointActive(playerid);
AMX_DECLARE_NATIVE(Natives::IsPlayerCheckpointActive)
{
	CHECK_PARAMS(1, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if (!IsPlayerConnected(playerid)) return 0;

	return pNetGame->pPlayerPool->pPlayer[playerid]->bShowCheckpoint;
}

// native GetPlayerCheckpoint(playerid, &Float:fX, &Float:fY, &Float:fZ, &Float:fSize);
AMX_DECLARE_NATIVE(Natives::GetPlayerCheckpoint)
{
	CHECK_PARAMS(5, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	if (!IsPlayerConnected(playerid)) return 0;

	CPlayer *pPlayer = pNetGame->pPlayerPool->pPlayer[playerid];
	CScriptParams::Get()->Add(pPlayer->vecCPPos, pPlayer->fCPSize);
	return 1;
}

// native IsPlayerRaceCheckpointActive(playerid);
AMX_DECLARE_NATIVE(Natives::IsPlayerRaceCheckpointActive)
{
	CHECK_PARAMS(1, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if (!IsPlayerConnected(playerid)) return 0;

	return pNetGame->pPlayerPool->pPlayer[playerid]->bShowRaceCheckpoint;
}

// native GetPlayerRaceCheckpoint(playerid, &Float:fX, &Float:fY, &Float:fZ, &Float:fNextX, &Float:fNextY, &fNextZ, &Float:fSize);
AMX_DECLARE_NATIVE(Natives::GetPlayerRaceCheckpoint)
{
	CHECK_PARAMS(8, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;

	CPlayer *pPlayer = pNetGame->pPlayerPool->pPlayer[playerid];
	CScriptParams::Get()->Add(pPlayer->vecRaceCPPos, pPlayer->vecRaceCPNextPos, pPlayer->fRaceCPSize);
	return 1;
}

// native GetPlayerWorldBounds(playerid, &Float:x_max, &Float:x_min, &Float:y_max, &Float:y_min);
AMX_DECLARE_NATIVE(Natives::GetPlayerWorldBounds)
{
	CHECK_PARAMS(5, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;

	CScriptParams::Get()->Add(pPlayerData[playerid]->fBounds[0], pPlayerData[playerid]->fBounds[1], pPlayerData[playerid]->fBounds[2], pPlayerData[playerid]->fBounds[3]);
	return 1;
}

// native IsPlayerInModShop(playerid);
AMX_DECLARE_NATIVE(Natives::IsPlayerInModShop)
{
	CHECK_PARAMS(1, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;

	return pNetGame->pPlayerPool->pPlayer[playerid]->bIsInModShop;
}

// native GetPlayerSirenState(playerid);
AMX_DECLARE_NATIVE(Natives::GetPlayerSirenState)
{
	CHECK_PARAMS(1, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;

	if(!pNetGame->pPlayerPool->pPlayer[playerid]->wVehicleId) return 0;

	return pNetGame->pPlayerPool->pPlayer[playerid]->vehicleSyncData.byteSirenState;
}

// native GetPlayerLandingGearState(playerid);
AMX_DECLARE_NATIVE(Natives::GetPlayerLandingGearState)
{
	CHECK_PARAMS(1, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;

	if(!pNetGame->pPlayerPool->pPlayer[playerid]->wVehicleId) return 0;

	return pNetGame->pPlayerPool->pPlayer[playerid]->vehicleSyncData.byteGearState;
}

// native GetPlayerHydraReactorAngle(playerid);
AMX_DECLARE_NATIVE(Natives::GetPlayerHydraReactorAngle)
{
	CHECK_PARAMS(1, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;

	if(!pNetGame->pPlayerPool->pPlayer[playerid]->wVehicleId) return 0;

	return pNetGame->pPlayerPool->pPlayer[playerid]->vehicleSyncData.wHydraReactorAngle[0];
}

// native Float:GetPlayerTrainSpeed(playerid);
AMX_DECLARE_NATIVE(Natives::GetPlayerTrainSpeed)
{
	CHECK_PARAMS(1, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;

	if(!pNetGame->pPlayerPool->pPlayer[playerid]->wVehicleId) return 0;

	return amx_ftoc(pNetGame->pPlayerPool->pPlayer[playerid]->vehicleSyncData.fTrainSpeed);
}

// native Float:GetPlayerZAim(playerid);
AMX_DECLARE_NATIVE(Natives::GetPlayerZAim)
{
	CHECK_PARAMS(1, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;

	return amx_ftoc(pNetGame->pPlayerPool->pPlayer[playerid]->aimSyncData.fZAim);
}

// native GetPlayerSurfingOffsets(playerid, &Float:fOffsetX, &Float:fOffsetY, &Float:fOffsetZ);
AMX_DECLARE_NATIVE(Natives::GetPlayerSurfingOffsets)
{
	CHECK_PARAMS(4, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;

	CScriptParams::Get()->Add(pNetGame->pPlayerPool->pPlayer[playerid]->syncData.vecSurfing);
	return 1;
}

// native GetPlayerRotationQuat(playerid, &Float:w, &Float:x, &Float:y, &Float:z);
AMX_DECLARE_NATIVE(Natives::GetPlayerRotationQuat)
{
	CHECK_PARAMS(5, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;

	CPlayer *pPlayer = pNetGame->pPlayerPool->pPlayer[playerid];
	CScriptParams::Get()->Add(pPlayer->fQuaternion[0], pPlayer->fQuaternion[1], pPlayer->fQuaternion[2], pPlayer->fQuaternion[3]);
	return 1;
}

// native GetPlayerDialogID(playerid);
AMX_DECLARE_NATIVE(Natives::GetPlayerDialogID)
{
	CHECK_PARAMS(1, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;

	return pNetGame->pPlayerPool->pPlayer[playerid]->wDialogID;
}

// native GetPlayerSpectateID(playerid);
AMX_DECLARE_NATIVE(Natives::GetPlayerSpectateID)
{
	CHECK_PARAMS(1, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;

	return pNetGame->pPlayerPool->pPlayer[playerid]->wSpectateID;
}

// native GetPlayerSpectateType(playerid);
AMX_DECLARE_NATIVE(Natives::GetPlayerSpectateType)
{
	CHECK_PARAMS(1, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;

	return pNetGame->pPlayerPool->pPlayer[playerid]->byteSpectateType;
}

// native GetPlayerLastSyncedVehicleID(playerid);
AMX_DECLARE_NATIVE(Natives::GetPlayerLastSyncedVehicleID)
{
	CHECK_PARAMS(1, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if (!IsPlayerConnected(playerid)) return 0;

	return static_cast<cell>(pNetGame->pPlayerPool->pPlayer[playerid]->vehicleSyncData.wVehicleId);
}

// native GetPlayerLastSyncedTrailerID(playerid);
AMX_DECLARE_NATIVE(Natives::GetPlayerLastSyncedTrailerID)
{
	CHECK_PARAMS(1, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if (!IsPlayerConnected(playerid)) return 0;

	return pNetGame->pPlayerPool->pPlayer[playerid]->trailerSyncData.wTrailerID;
}

// native GetActorSpawnInfo(actorid, &skinid, &Float:fX, &Float:fY, &Float:fZ, &Float:fAngle);
AMX_DECLARE_NATIVE(Natives::GetActorSpawnInfo)
{
	CHECK_PARAMS(6, LOADED);

	const int actorid = CScriptParams::Get()->ReadInt();
	if(actorid < 0 || actorid > MAX_PLAYERS) return 0;

	CActor *pActor = pNetGame->pActorPool->pActor[actorid];
	if(!pActor) return 0;

	CScriptParams::Get()->Add(pActor->iSkinID, pActor->vecSpawnPos, pActor->fSpawnAngle);
	return 1;
}

// native GetActorSkin(actorid);
AMX_DECLARE_NATIVE(Natives::GetActorSkin)
{
	CHECK_PARAMS(1, LOADED);

	const int actorid = CScriptParams::Get()->ReadInt();
	if(actorid < 0 || actorid > MAX_PLAYERS) return 0;

	CActor *pActor = pNetGame->pActorPool->pActor[actorid];
	if(!pActor) return 0;

	return pActor->iSkinID;
}


// native GetActorAnimation(actorid, animlib[], animlibsize = sizeof(animlib), animname[], animnamesize = sizeof(animname), &Float:fDelta, &loop, &lockx, &locky, &freeze, &time)
AMX_DECLARE_NATIVE(Natives::GetActorAnimation)
{
	CHECK_PARAMS(11, LOADED);

	const int actorid = CScriptParams::Get()->ReadInt();
	if(actorid < 0 || actorid > MAX_PLAYERS) return 0;

	CActor *pActor = pNetGame->pActorPool->pActor[actorid];
	if(!pActor) return 0;

	CScriptParams::Get()->Add(&pActor->anim.szAnimLib[0], &pActor->anim.szAnimName[0], pActor->anim.fDelta, pActor->anim.byteLoop, pActor->anim.byteLockX, 
		pActor->anim.byteLockY, pActor->anim.byteFreeze, pActor->anim.iTime);
	return 1;
}

// native SendBulletData(senderid, forplayerid = -1, weaponid, hittype, hitid, Float:fHitOriginX, Float:fHitOriginY, Float:fHitOriginZ, Float:fHitTargetX, Float:fHitTargetY, Float:fHitTargetZ, Float:fCenterOfHitX, Float:fCenterOfHitY, Float:fCenterOfHitZ);
AMX_DECLARE_NATIVE(Natives::SendBulletData) 
{
	CHECK_PARAMS(14, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int forplayerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;

	if(forplayerid != -1)
	{
		if(!IsPlayerConnected(forplayerid)) return 0;
	}

	CBulletSyncData bulletSync;
	CScriptParams::Get()->Read(&bulletSync.byteWeaponID, &bulletSync.byteHitType, &bulletSync.wHitID, &bulletSync.vecHitOrigin, &bulletSync.vecHitTarget, &bulletSync.vecCenterOfHit);

	RakNet::BitStream bs;
	bs.Write((BYTE)ID_BULLET_SYNC);
	bs.Write((WORD)playerid);
	bs.Write((char*)&bulletSync, sizeof(CBulletSyncData));

	if(forplayerid == -1)
	{
		CSAMPFunctions::Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, UNASSIGNED_PLAYER_ID, true);
	}
	else
	{
		CSAMPFunctions::Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, CSAMPFunctions::GetPlayerIDFromIndex(forplayerid), false);
	}
	return 1;
}

// native ShowPlayerForPlayer(forplayerid, playerid);
AMX_DECLARE_NATIVE(Natives::ShowPlayerForPlayer)
{
	CHECK_PARAMS(2, LOADED);

	const int forplayerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(forplayerid)) return 0;

	const int playerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;

	if(playerid == forplayerid) return 0;

	RakNet::BitStream bs;
	bs.Write((WORD)playerid);
	CSAMPFunctions::RPC(&RPC_WorldPlayerAdd, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, CSAMPFunctions::GetPlayerIDFromIndex(forplayerid), 0, 0);
	return 1;
}

// native HidePlayerForPlayer(forplayerid, playerid);
AMX_DECLARE_NATIVE(Natives::HidePlayerForPlayer)
{
	CHECK_PARAMS(2, LOADED);

	const int forplayerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(forplayerid)) return 0;

	const int playerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;

	if(playerid == forplayerid) return 0;

	RakNet::BitStream bs;
	bs.Write((WORD)playerid);
	CSAMPFunctions::RPC(&RPC_WorldPlayerRemove, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, CSAMPFunctions::GetPlayerIDFromIndex(forplayerid), 0, 0);
	return 1;
}

// native AddPlayerForPlayer(forplayerid, playerid, isnpc = 0);
AMX_DECLARE_NATIVE(Natives::AddPlayerForPlayer)
{
	CHECK_PARAMS(3, LOADED);

	const int forplayerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(forplayerid)) return 0;

	const int playerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;

	if(playerid == forplayerid) return 0;

	const bool npc = CScriptParams::Get()->ReadBool();
	const char* szName = GetPlayerName(playerid);
	BYTE len = static_cast<BYTE>(strlen(szName));

	RakNet::BitStream bs;
	bs.Write((WORD)playerid);
	bs.Write((DWORD)0);
	bs.Write((BYTE)npc); //  // isNPC
	bs.Write(len);
	bs.Write(szName, len);
	CSAMPFunctions::RPC(&RPC_ServerJoin, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, CSAMPFunctions::GetPlayerIDFromIndex(forplayerid), 0, 0);
	return 1;
}

// native RemovePlayerForPlayer(forplayerid, playerid);
AMX_DECLARE_NATIVE(Natives::RemovePlayerForPlayer)
{
	CHECK_PARAMS(2, LOADED);

	const int forplayerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(forplayerid)) return 0;

	const int playerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;

	if(playerid == forplayerid) return 0;

	RakNet::BitStream bs;
	bs.Write((WORD)playerid);
	bs.Write((BYTE)0); 
	CSAMPFunctions::RPC(&RPC_ServerQuit, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, CSAMPFunctions::GetPlayerIDFromIndex(forplayerid), 0, 0);
	return 1;
}

// native SetPlayerChatBubbleForPlayer(forplayerid, playerid, text[], color, Float:drawdistance, expiretime);
AMX_DECLARE_NATIVE(Natives::SetPlayerChatBubbleForPlayer)
{
	CHECK_PARAMS(6, LOADED);

	const int forplayerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(forplayerid)) return 0;

	const int playerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;

	std::string text;
	int color;
	float drawdistance;
	int expiretime;
	CScriptParams::Get()->Read(&text, &color, &drawdistance, &expiretime);

	if(!text.empty())
	{
		BYTE len = static_cast<BYTE>(text.length());
		RakNet::BitStream bs;
		bs.Write((WORD)playerid);
		bs.Write(color);
		bs.Write(drawdistance);
		bs.Write(expiretime);
		bs.Write(len);
		bs.Write(text.c_str(), len);
		CSAMPFunctions::RPC(&RPC_ChatBubble, &bs, LOW_PRIORITY, RELIABLE, 0, CSAMPFunctions::GetPlayerIDFromIndex(forplayerid), 0, 0);
		return 1;
	}
	return 0;
}

// native ResetPlayerMarkerForPlayer(playerid, resetplayerid)
AMX_DECLARE_NATIVE(Natives::ResetPlayerMarkerForPlayer)
{
	CHECK_PARAMS(2, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int resetplayerid = CScriptParams::Get()->ReadInt();

	if (!IsPlayerConnected(playerid) || !IsPlayerConnected(resetplayerid)) return 0;
	
	pPlayerData[playerid]->ResetPlayerMarkerForPlayer(static_cast<WORD>(resetplayerid));
	return 1;
}
 
// native SetPlayerVersion(playerid, version[];
AMX_DECLARE_NATIVE(Natives::SetPlayerVersion)
{
	CHECK_PARAMS(2, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;
	
	std::string version;
	CScriptParams::Get()->Read(&version);

	if (!version.empty() && version.length() < 20)
	{
		pNetGame->pPlayerPool->szVersion[playerid][0] = NULL;
		strcpy(pNetGame->pPlayerPool->szVersion[playerid], version.c_str());
		return 1;
	}
	return 0;
}

// native IsPlayerSpawned(playerid);
AMX_DECLARE_NATIVE(Natives::IsPlayerSpawned)
{
	CHECK_PARAMS(1, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;

	BYTE state = pNetGame->pPlayerPool->pPlayer[playerid]->byteState;
	switch (state) 
	{
		case PLAYER_STATE_ONFOOT: 
		case PLAYER_STATE_DRIVER: 
		case PLAYER_STATE_PASSENGER:			
		case PLAYER_STATE_SPAWNED: 
		{
			return true;
		}
	}
	return false;
}

// native IsPlayerControllable(playerid);
AMX_DECLARE_NATIVE(Natives::IsPlayerControllable)
{
	CHECK_PARAMS(1, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;

	return pPlayerData[playerid]->bControllable;
}

// native SpawnForWorld(playerid);
AMX_DECLARE_NATIVE(Natives::SpawnForWorld)
{
	CHECK_PARAMS(1, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if (!IsPlayerConnected(playerid)) return 0;

	CSAMPFunctions::SpawnPlayer(playerid);
	return 1;
}

// native BroadcastDeath(playerid);
AMX_DECLARE_NATIVE(Natives::BroadcastDeath)
{
	CHECK_PARAMS(1, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if (!IsPlayerConnected(playerid)) return 0;

	RakNet::BitStream bsData;
	bsData.Write((WORD)playerid);
	CSAMPFunctions::RPC(&RPC_DeathBroadcast, &bsData, HIGH_PRIORITY, RELIABLE_ORDERED, 0, CSAMPFunctions::GetPlayerIDFromIndex(playerid), true, false);
	return 1;
}

// native IsPlayerCameraTargetEnabled(playerid);
AMX_DECLARE_NATIVE(Natives::IsPlayerCameraTargetEnabled)
{
	CHECK_PARAMS(1, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;

	return pNetGame->pPlayerPool->pPlayer[playerid]->bCameraTarget;
}

// native SetPlayerDisabledKeysSync(playerid, keys, updown = 0, leftright = 0);
AMX_DECLARE_NATIVE(Natives::SetPlayerDisabledKeysSync)
{
	CHECK_PARAMS(4, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;

	pPlayerData[playerid]->wDisabledKeys = static_cast<WORD>(CScriptParams::Get()->ReadInt());
	pPlayerData[playerid]->wDisabledKeysUD = static_cast<WORD>(CScriptParams::Get()->ReadInt());
	pPlayerData[playerid]->wDisabledKeysLR = static_cast<WORD>(CScriptParams::Get()->ReadInt());
	return 1;
}

// native GetPlayerDisabledKeysSync(playerid, &keys, &updown, &leftright);
AMX_DECLARE_NATIVE(Natives::GetPlayerDisabledKeysSync)
{
	CHECK_PARAMS(4, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;

	CScriptParams::Get()->Add(pPlayerData[playerid]->wDisabledKeys, static_cast<short>(pPlayerData[playerid]->wDisabledKeysUD), static_cast<short>(pPlayerData[playerid]->wDisabledKeysLR));
	return 1;
}

// Scoreboard manipulation
// native TogglePlayerScoresPingsUpdate(playerid, bool:toggle);
AMX_DECLARE_NATIVE(Natives::TogglePlayerScoresPingsUpdate)
{
	CHECK_PARAMS(2, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const bool toggle = CScriptParams::Get()->ReadBool();

	if(!IsPlayerConnected(playerid)) return 0;

	pPlayerData[playerid]->bUpdateScoresPingsDisabled = !toggle;
	return 1;
}

// native TogglePlayerFakePing(playerid, bool:toggle);
AMX_DECLARE_NATIVE(Natives::TogglePlayerFakePing)
{
	CHECK_PARAMS(2, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	bool toggle = CScriptParams::Get()->ReadBool();

	if(!IsPlayerConnected(playerid)) return 0;

	pPlayerData[playerid]->bFakePingToggle = toggle;
	return 1;
}

// native SetPlayerFakePing(playerid, ping);
AMX_DECLARE_NATIVE(Natives::SetPlayerFakePing)
{
	CHECK_PARAMS(2, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	int fakeping = CScriptParams::Get()->ReadInt();

	if(!IsPlayerConnected(playerid)) return 0;

	pPlayerData[playerid]->dwFakePingValue = fakeping;
	return 1;
}

// native SetPlayerNameInServerQuery(playerid, const name[]);
AMX_DECLARE_NATIVE(Natives::SetPlayerNameInServerQuery)
{
	CHECK_PARAMS(2, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if (!IsPlayerConnected(playerid)) return 0;

	std::string name;
	CScriptParams::Get()->Read(&name);
	if (name.length() >= MAX_PLAYER_NAME) return 0;

	pPlayerData[playerid]->strNameInQuery = std::move(name);
	return 1;
}

// native GetPlayerNameInServerQuery(playerid, name[], len = sizeof(name));
AMX_DECLARE_NATIVE(Natives::GetPlayerNameInServerQuery)
{
	CHECK_PARAMS(3, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if (!IsPlayerConnected(playerid)) return 0;

	CScriptParams::Get()->Add(pPlayerData[playerid]->strNameInQuery);
	return 1;
}

// native IsPlayerPaused(playerid);
AMX_DECLARE_NATIVE(Natives::IsPlayerPaused)
{
	CHECK_PARAMS(1, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;

	return pPlayerData[playerid]->bAFKState;
}

// native GetPlayerPausedTime(playerid);
AMX_DECLARE_NATIVE(Natives::GetPlayerPausedTime)
{
	CHECK_PARAMS(1, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();

	if(!IsPlayerConnected(playerid)) return 0;
	if(!pPlayerData[playerid]->bAFKState) return 0;

	return static_cast<cell>(std::chrono::duration_cast<std::chrono::milliseconds>(default_clock::now() - pPlayerData[playerid]->LastUpdateTick).count());
}

// Objects - global
// native Float:GetObjectDrawDistance(objectid);
AMX_DECLARE_NATIVE(Natives::GetObjectDrawDistance)
{
	CHECK_PARAMS(1, LOADED);

	const int objectid = CScriptParams::Get()->ReadInt();
	if(objectid < 0 || objectid >= MAX_OBJECTS) return 0;
	if(!pNetGame->pObjectPool->bObjectSlotState[objectid]) return 0;

	return amx_ftoc(pNetGame->pObjectPool->pObjects[objectid]->fDrawDistance);
}

// native Float:SetObjectMoveSpeed(objectid, Float:fSpeed);
AMX_DECLARE_NATIVE(Natives::SetObjectMoveSpeed)
{
	CHECK_PARAMS(2, LOADED);

	const int objectid = CScriptParams::Get()->ReadInt();
	if(objectid < 0 || objectid >= MAX_OBJECTS) return 0;
	if(!pNetGame->pObjectPool->bObjectSlotState[objectid]) return 0;

	pNetGame->pObjectPool->pObjects[objectid]->fMoveSpeed = CScriptParams::Get()->ReadFloat();
	return 1;
}

// native Float:GetObjectMoveSpeed(objectid);
AMX_DECLARE_NATIVE(Natives::GetObjectMoveSpeed)
{
	CHECK_PARAMS(1, LOADED);

	const int objectid = CScriptParams::Get()->ReadInt();
	if(objectid < 0 || objectid >= MAX_OBJECTS) return 0;
	if(!pNetGame->pObjectPool->bObjectSlotState[objectid]) return 0;

	return amx_ftoc(pNetGame->pObjectPool->pObjects[objectid]->fMoveSpeed);
}

// native GetObjectTarget(objectid, &Float:fX, &Float:fY, &Float:fZ);
AMX_DECLARE_NATIVE(Natives::GetObjectTarget)
{
	CHECK_PARAMS(4, LOADED);

	const int objectid = CScriptParams::Get()->ReadInt();
	if(objectid < 0 || objectid >= MAX_OBJECTS) return 0;
	if(!pNetGame->pObjectPool->bObjectSlotState[objectid]) return 0;

	CObject *pObject = pNetGame->pObjectPool->pObjects[objectid];
	CScriptParams::Get()->Add(pObject->matTarget.pos);
	return 1;
}

// native GetObjectAttachedData(objectid, &vehicleid, &objectid, &attachedplayerid);
AMX_DECLARE_NATIVE(Natives::GetObjectAttachedData)
{
	CHECK_PARAMS(4, LOADED);

	const int objectid = CScriptParams::Get()->ReadInt();
	if(objectid < 0 || objectid >= MAX_OBJECTS) return 0;
	
	if(!pNetGame->pObjectPool->bObjectSlotState[objectid]) return 0;

	CObject *pObject = pNetGame->pObjectPool->pObjects[objectid];
	CScriptParams::Get()->Add(pObject->wAttachedVehicleID, pObject->wAttachedObjectID, CServer::Get()->COBJECT_AttachedObjectPlayer[objectid]);
	return 1;
}

// native GetObjectAttachedOffset(objectid, &Float:fX, &Float:fY, &Float:fZ, &Float:fRotX, &Float:fRotY, &Float:fRotZ);
AMX_DECLARE_NATIVE(Natives::GetObjectAttachedOffset)
{
	CHECK_PARAMS(7, LOADED);

	const int objectid = CScriptParams::Get()->ReadInt();
	if(objectid < 0 || objectid >= MAX_OBJECTS) return 0;

	if(!pNetGame->pObjectPool->bObjectSlotState[objectid]) return 0;

	CObject *pObject = pNetGame->pObjectPool->pObjects[objectid];
	CScriptParams::Get()->Add(pObject->vecAttachedOffset, pObject->vecAttachedRotation);
	return 1;
}

// native IsObjectMaterialSlotUsed(objectid, materialindex); // Return values: 1 = material, 2 = material text
AMX_DECLARE_NATIVE(Natives::IsObjectMaterialSlotUsed)
{
	CHECK_PARAMS(2, LOADED);

	const int objectid = CScriptParams::Get()->ReadInt();
	if(objectid < 0 || objectid >= MAX_OBJECTS) return 0;

	const int materialindex = CScriptParams::Get()->ReadInt();
	if(materialindex < 0 || materialindex >= 16) return 0;

	if(!pNetGame->pObjectPool->bObjectSlotState[objectid]) return 0;

	int i = 0;
	CObject *pObject = pNetGame->pObjectPool->pObjects[objectid];
	
	// Nothing to comment here..
	while(i != MAX_OBJECT_MATERIAL)
	{
		if(pObject->Material[i].byteSlot == materialindex) break;
		i++;
	}
	if(i == MAX_OBJECT_MATERIAL) return 0;

	return pObject->Material[i].byteUsed;
}

// native GetObjectMaterial(objectid, materialindex, &modelid, txdname[], txdnamelen = sizeof(txdname), texturename[], texturenamelen = sizeof(texturename), &materialcolor);
AMX_DECLARE_NATIVE(Natives::GetObjectMaterial)
{
	CHECK_PARAMS(8, LOADED);

	const int objectid = CScriptParams::Get()->ReadInt();
	const int materialindex = CScriptParams::Get()->ReadInt();

	if(objectid < 0 || objectid >= MAX_OBJECTS) return 0;
	if(materialindex < 0 || materialindex >= 16) return 0;

	if(!pNetGame->pObjectPool->bObjectSlotState[objectid]) return 0;

	int i = 0;
	CObject *pObject = pNetGame->pObjectPool->pObjects[objectid];

	// Nothing to comment here..
	while(i != MAX_OBJECT_MATERIAL)
	{
		if(pObject->Material[i].byteSlot == materialindex) break;
		i++;
	}
	if(i == MAX_OBJECT_MATERIAL) return 0;
	
	CScriptParams::Get()->Add(pObject->Material[i].wModelID, &pObject->Material[i].szMaterialTXD[0], &pObject->Material[i].szMaterialTexture[0], ABGR_ARGB(pObject->Material[i].dwMaterialColor));
	return 1;
}

// native GetObjectMaterialText(objectid, materialindex, text[], textlen = sizeof(text), &materialsize, fontface[], fontfacelen = sizeof(fontface), &fontsize, &bold, &fontcolor, &backcolor, &textalignment);
AMX_DECLARE_NATIVE(Natives::GetObjectMaterialText)
{
	CHECK_PARAMS(12, LOADED);

	const int objectid = CScriptParams::Get()->ReadInt();
	const int materialindex = CScriptParams::Get()->ReadInt();

	if(objectid < 0 || objectid >= MAX_OBJECTS) return 0;
	if(materialindex < 0 || materialindex >= 16) return 0;

	if(!pNetGame->pObjectPool->bObjectSlotState[objectid]) return 0;

	int i = 0;
	CObject *pObject = pNetGame->pObjectPool->pObjects[objectid];

	// Nothing to comment here..
	while(i != MAX_OBJECT_MATERIAL)
	{
		if(pObject->Material[i].byteSlot == materialindex) break;
		i++;
	}
	if(i == MAX_OBJECT_MATERIAL) return 0;

	CScriptParams::Get()->Add(pObject->szMaterialText[i], pObject->Material[i].byteMaterialSize, pObject->Material[i].szFont, pObject->Material[i].byteFontSize,
		pObject->Material[i].byteBold, pObject->Material[i].dwFontColor, pObject->Material[i].dwBackgroundColor, pObject->Material[i].byteAlignment);
	return 1;
}

// native IsObjectNoCameraCol(objectid);
AMX_DECLARE_NATIVE(Natives::IsObjectNoCameraCol)
{
	CHECK_PARAMS(1, LOADED);

	const int objectid = CScriptParams::Get()->ReadInt();
	if(objectid < 0 || objectid >= MAX_OBJECTS) return 0;

	if(!pNetGame->pObjectPool->bObjectSlotState[objectid]) return 0;

	return pNetGame->pObjectPool->pObjects[objectid]->bNoCameraCol;
}

// native Float:GetPlayerObjectDrawDistance(playerid, objectid);
AMX_DECLARE_NATIVE(Natives::GetPlayerObjectDrawDistance)
{
	CHECK_PARAMS(2, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int objectid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;
	if(objectid < 0 || objectid >= 1000) return 0;

	if(!pNetGame->pObjectPool->bPlayerObjectSlotState[playerid][objectid]) return 0;

	return amx_ftoc(pNetGame->pObjectPool->pPlayerObjects[playerid][objectid]->fDrawDistance);
}

// native Float:SetPlayerObjectMoveSpeed(playerid, objectid, Float:fSpeed);
AMX_DECLARE_NATIVE(Natives::SetPlayerObjectMoveSpeed)
{
	CHECK_PARAMS(3, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int objectid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;
	if(objectid < 0 || objectid >= MAX_OBJECTS) return 0;

	if(!pNetGame->pObjectPool->bPlayerObjectSlotState[playerid][objectid]) return 0;

	pNetGame->pObjectPool->pPlayerObjects[playerid][objectid]->fMoveSpeed = amx_ctof(params[3]);
	return 1;
}

// native Float:GetPlayerObjectMoveSpeed(playerid, objectid);
AMX_DECLARE_NATIVE(Natives::GetPlayerObjectMoveSpeed)
{
	CHECK_PARAMS(2, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int objectid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;
	if(objectid < 0 || objectid >= MAX_OBJECTS) return 0;

	if(!pNetGame->pObjectPool->bPlayerObjectSlotState[playerid][objectid]) return 0;

	return amx_ftoc(pNetGame->pObjectPool->pPlayerObjects[playerid][objectid]->fMoveSpeed);
}

// native Float:GetPlayerObjectTarget(playerid, objectid, &Float:fX, &Float:fY, &Float:fZ);
AMX_DECLARE_NATIVE(Natives::GetPlayerObjectTarget)
{
	CHECK_PARAMS(5, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int objectid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;
	if(objectid < 0 || objectid >= MAX_OBJECTS) return 0;

	if(!pNetGame->pObjectPool->bPlayerObjectSlotState[playerid][objectid]) return 0;

	CObject *pObject = pNetGame->pObjectPool->pPlayerObjects[playerid][objectid];
	CScriptParams::Get()->Add(pObject->matTarget.pos);
	return 1;
}

// native GetPlayerObjectAttachedData(playerid, objectid, &vehicleid, &objectid, &attachedplayerid);
AMX_DECLARE_NATIVE(Natives::GetPlayerObjectAttachedData)
{
	CHECK_PARAMS(5, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int objectid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;
	if(objectid < 0 || objectid >= MAX_OBJECTS) return 0;
	
	if(!pNetGame->pObjectPool->bPlayerObjectSlotState[playerid][objectid]) return 0;

	CObject *pObject = pNetGame->pObjectPool->pPlayerObjects[playerid][objectid];

	WORD attachedobjectid = INVALID_OBJECT_ID;
	WORD attachedplayerid = INVALID_PLAYER_ID;
	const std::shared_ptr<CPlayerObjectAttachAddon> pAddon = pPlayerData[playerid]->FindObjectAddon(objectid);
	if (pAddon)
	{
		attachedobjectid = pAddon->wObjectID;
		attachedplayerid = pAddon->wAttachPlayerID;
	}

	CScriptParams::Get()->Add(pObject->wAttachedVehicleID, attachedobjectid, attachedplayerid);
	return 1;
}

// native GetPlayerObjectAttachedOffset(playerid, objectid, &Float:fX, &Float:fY, &Float:fZ, &Float:fRotX, &Float:fRotY, &Float:fRotZ);
AMX_DECLARE_NATIVE(Natives::GetPlayerObjectAttachedOffset)
{
	CHECK_PARAMS(8, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int objectid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;
	if(objectid < 0 || objectid >= MAX_OBJECTS) return 0;

	if(!pNetGame->pObjectPool->bPlayerObjectSlotState[playerid][objectid]) return 0;

	CObject *pObject = pNetGame->pObjectPool->pPlayerObjects[playerid][objectid];
	CVector vecOffset;
	CVector vecRot;
	
	if(pObject->wAttachedVehicleID)
	{
		vecOffset = pObject->vecAttachedOffset;
		vecRot = pObject->vecAttachedRotation;
	}
	else
	{
		const std::shared_ptr<CPlayerObjectAttachAddon> pAddon = pPlayerData[playerid]->FindObjectAddon(objectid);
		if (pAddon) 
		{
			vecOffset = pAddon->vecOffset;
			vecOffset = pAddon->vecRot;
		}
	}
	
	CScriptParams::Get()->Add(vecOffset, vecRot);
	return 1;
}

// native IsPlayerObjectMaterialSlotUsed(playerid, objectid, materialindex); // Return values: 1 = material, 2 = material text
AMX_DECLARE_NATIVE(Natives::IsPlayerObjectMaterialSlotUsed)
{
	CHECK_PARAMS(3, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int objectid = CScriptParams::Get()->ReadInt();
	const int materialindex = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;
	if(objectid < 0 || objectid >= MAX_OBJECTS) return 0;
	if(materialindex < 0 || materialindex >= 16) return 0;

	if(!pNetGame->pObjectPool->bPlayerObjectSlotState[playerid][objectid]) return 0;

	int i = 0;
	CObject *pObject = pNetGame->pObjectPool->pPlayerObjects[playerid][objectid];
	
	// Nothing to comment here..
	while(i != MAX_OBJECT_MATERIAL)
	{
		if(pObject->Material[i].byteSlot == materialindex) break;
		i++;
	}
	if(i == MAX_OBJECT_MATERIAL) return 0;

	return pObject->Material[i].byteUsed;
}

// native GetPlayerObjectMaterial(playerid, objectid, materialindex, &modelid, txdname[], txdnamelen = sizeof(txdname), texturename[], texturenamelen = sizeof(texturename), &materialcolor);
AMX_DECLARE_NATIVE(Natives::GetPlayerObjectMaterial)
{
	CHECK_PARAMS(9, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int objectid = CScriptParams::Get()->ReadInt();
	const int materialindex = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;
	if(objectid < 0 || objectid >= MAX_OBJECTS) return 0;
	if(materialindex < 0 || materialindex >= 16) return 0;

	if(!pNetGame->pObjectPool->bPlayerObjectSlotState[playerid][objectid]) return 0;

	int i = 0;
	CObject *pObject = pNetGame->pObjectPool->pPlayerObjects[playerid][objectid];
	
	// Nothing to comment here..
	while(i != MAX_OBJECT_MATERIAL)
	{
		if(pObject->Material[i].byteSlot == materialindex) break;
		i++;
	}
	if(i == MAX_OBJECT_MATERIAL) return 0;

	CScriptParams::Get()->Add(pObject->Material[i].wModelID, &pObject->Material[i].szMaterialTXD[0], &pObject->Material[i].szMaterialTexture[0], ABGR_ARGB(pObject->Material[i].dwMaterialColor));
	return 1;
}

// native GetPlayerObjectMaterialText(playerid, objectid, materialindex, text[], textlen = sizeof(text), &materialsize, fontface[], fontfacelen = sizeof(fontface), &fontsize, &bold, &fontcolor, &backcolor, &textalignment);
AMX_DECLARE_NATIVE(Natives::GetPlayerObjectMaterialText)
{
	CHECK_PARAMS(13, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int objectid = CScriptParams::Get()->ReadInt();
	const int materialindex = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;
	if(objectid < 0 || objectid >= MAX_OBJECTS) return 0;
	if(materialindex < 0 || materialindex >= 16) return 0;

	if(!pNetGame->pObjectPool->bPlayerObjectSlotState[playerid][objectid]) return 0;

	int i = 0;
	CObject *pObject = pNetGame->pObjectPool->pPlayerObjects[playerid][objectid];
	
	// Nothing to comment here..
	while(i != MAX_OBJECT_MATERIAL)
	{
		if(pObject->Material[i].byteSlot == materialindex) break;
		i++;
	}
	if(i == MAX_OBJECT_MATERIAL) return 0;

	std::string text;
	for (std::multimap<WORD, std::pair<BYTE, std::string>>::iterator o = pPlayerData[playerid]->m_PlayerObjectMaterialText.begin(); o != pPlayerData[playerid]->m_PlayerObjectMaterialText.end(); ++o)
	{
		if (o->first == objectid)
		{
			if(o->second.first == materialindex)
			{
				text = o->second.second;
			}
			break;
		}
	}

	CScriptParams::Get()->Add(text, pObject->Material[i].byteMaterialSize, &pObject->Material[i].szFont[0], pObject->Material[i].byteFontSize,
		pObject->Material[i].byteBold, pObject->Material[i].dwFontColor, pObject->Material[i].dwBackgroundColor, pObject->Material[i].byteAlignment);
	return 1;
}

// native IsPlayerObjectNoCameraCol(playerid, objectid);
AMX_DECLARE_NATIVE(Natives::IsPlayerObjectNoCameraCol)
{
	CHECK_PARAMS(2, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int objectid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;
	if(objectid < 0 || objectid >= MAX_OBJECTS) return 0;

	if(!pNetGame->pObjectPool->bPlayerObjectSlotState[playerid][objectid]) return 0;

	return pNetGame->pObjectPool->pPlayerObjects[playerid][objectid]->bNoCameraCol;
}

// native GetPlayerSurfingPlayerObjectID(playerid);
AMX_DECLARE_NATIVE(Natives::GetPlayerSurfingPlayerObjectID)
{
	CHECK_PARAMS(1, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return INVALID_OBJECT_ID;

	const int surf = pPlayerData[playerid]->wSurfingInfo - MAX_VEHICLES;
	if(surf >= 0 && surf < MAX_OBJECTS)
	{
		if(pNetGame->pObjectPool->bPlayerObjectSlotState[playerid][surf])
			return surf;
	}
	return INVALID_OBJECT_ID;
}

// native GetPlayerCameraTargetPlayerObj(playerid);
AMX_DECLARE_NATIVE(Natives::GetPlayerCameraTargetPlayerObj)
{
	CHECK_PARAMS(1, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return INVALID_OBJECT_ID;
	
	CPlayer *pPlayer = pNetGame->pPlayerPool->pPlayer[playerid];
	if(!pPlayer->bCameraTarget) return INVALID_OBJECT_ID;

	const int target = pNetGame->pPlayerPool->pPlayer[playerid]->wCameraObject;
	if(target >= 0 && target < MAX_OBJECTS)
	{
		if(pNetGame->pObjectPool->bPlayerObjectSlotState[playerid][target])
			return target;
	}
	return INVALID_OBJECT_ID;
}

// native GetObjectType(playerid, objectid);
AMX_DECLARE_NATIVE(Natives::GetObjectType)
{
	CHECK_PARAMS(2, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int objectid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;
	if(objectid < 0 || objectid >= MAX_OBJECTS) return 0;

	BYTE ret;
	if(pNetGame->pObjectPool->bObjectSlotState[objectid])
		ret = SELECT_OBJECT_GLOBAL_OBJECT;
	else if(pNetGame->pObjectPool->bPlayerObjectSlotState[playerid][objectid])
		ret = SELECT_OBJECT_PLAYER_OBJECT;
	else 
		ret = 0;
	return ret;
}

// native GetPlayerAttachedObject(playerid, index, &modelid, &bone, &Float:fX, &Float:fY, &Float:fZ, &Float:fRotX, &Float:fRotY, &Float:fRotZ, Float:&fSacleX, Float:&fScaleY, Float:&fScaleZ, &materialcolor1, &materialcolor2);
AMX_DECLARE_NATIVE(Natives::GetPlayerAttachedObject)
{
	CHECK_PARAMS(15, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int slot = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;
	if(slot < 0 || slot >= MAX_PLAYER_ATTACHED_OBJECTS) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->attachedObjectSlot[slot]) return 0;

	CAttachedObject *pObject = &pNetGame->pPlayerPool->pPlayer[playerid]->attachedObject[slot];

	CScriptParams::Get()->Add(pObject->iModelID, pObject->iBoneiD, pObject->vecPos, pObject->vecRot, pObject->vecScale, 
		RGBA_ABGR(pObject->dwMaterialColor1), RGBA_ABGR(pObject->dwMaterialColor2));
	return 1;
}

// native SetPlayerAttachedObjForPlayer(forplayerid, attachtoplayerid, index, modelid, bone, Float:fOffsetX = 0.0, Float : OffsetY = 0.0, Float : fOffsetZ = 0.0, Float : fRotX = 0.0, Float : fRotY = 0.0, Float : fRotZ = 0.0, Float : fScaleX = 1.0, Float : fScaleY = 1.0, Float : fScaleZ = 1.0, materialcolor1 = 0, materialcolor2 = 0);
AMX_DECLARE_NATIVE(Natives::SetPlayerAttachedObjForPlayer)
{
	CHECK_PARAMS(16, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if (!IsPlayerConnected(playerid)) return 0;

	const int attachplayerid = CScriptParams::Get()->ReadInt();
	if (!IsPlayerConnected(attachplayerid)) return 0;

	const int index = CScriptParams::Get()->ReadInt();
	if (index < 0 || index >= MAX_PLAYER_ATTACHED_OBJECTS) return 0;

	CAttachedObject objData;
	CScriptParams::Get()->Read(&objData.iModelID, &objData.iBoneiD, &objData.vecPos, &objData.vecRot, &objData.vecScale, &objData.dwMaterialColor1, &objData.dwMaterialColor2);
	objData.dwMaterialColor1 = ABGR_RGBA(objData.dwMaterialColor1);
	objData.dwMaterialColor2 = ABGR_RGBA(objData.dwMaterialColor2);

	RakNet::BitStream bsData;
	bsData.Write((WORD)attachplayerid);
	bsData.Write(index);
	bsData.Write(true);
	bsData.Write((char*)&objData, sizeof(CAttachedObject));
	CSAMPFunctions::RPC(&RPC_SetPlayerAttachedObject, &bsData, HIGH_PRIORITY, RELIABLE_ORDERED, 0, CSAMPFunctions::GetPlayerIDFromIndex(playerid), false, false);

	for (std::multimap<WORD, std::pair<WORD, std::unique_ptr<CAttachedObject>>>::iterator o = pPlayerData[playerid]->holdingObjects.begin(); o != pPlayerData[playerid]->holdingObjects.end(); ++o)
	{
		if (o->first == attachplayerid)
		{
			if (o->second.first == index)
			{
				o = pPlayerData[playerid]->holdingObjects.erase(o);
				break;
			}
		}
	}
	pPlayerData[playerid]->holdingObjects.emplace(attachplayerid, std::make_pair(index, std::make_unique<CAttachedObject>(std::move(objData))));
	return 1;
}

// native GetPlayerAttachedObjForPlayer(forplayerid, attachtoplayerid, index, &modelid, &bone, &Float:fX, &Float:fY, &Float:fZ, &Float:fRotX, &Float:fRotY, &Float:fRotZ, Float:&fSacleX, Float:&fScaleY, Float:&fScaleZ, &materialcolor1, &materialcolor2);
AMX_DECLARE_NATIVE(Natives::GetPlayerAttachedObjForPlayer)
{
	CHECK_PARAMS(16, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if (!IsPlayerConnected(playerid)) return 0;

	const int removefromplayerid = CScriptParams::Get()->ReadInt();
	if (!IsPlayerConnected(removefromplayerid)) return 0;

	const int index = CScriptParams::Get()->ReadInt();
	if (index < 0 || index >= MAX_PLAYER_ATTACHED_OBJECTS) return 0;

	CAttachedObject *pObject = nullptr;
	for (std::multimap<WORD, std::pair<WORD, std::unique_ptr<CAttachedObject>>>::iterator o = pPlayerData[playerid]->holdingObjects.begin(); o != pPlayerData[playerid]->holdingObjects.end(); ++o)
	{
		if (o->first == removefromplayerid)
		{
			if (o->second.first == index)
			{
				pObject = o->second.second.get();
				break;
			}
		}
	}

	if (!pObject) return 0;

	CScriptParams::Get()->Add(pObject->iModelID, pObject->iBoneiD, pObject->vecPos, pObject->vecRot, pObject->vecScale, 
		RGBA_ABGR(pObject->dwMaterialColor1), RGBA_ABGR(pObject->dwMaterialColor2));
	return 1;
}

// native RemPlayerAttachedObjForPlayer(forplayerid, removefromplayerid, index);
AMX_DECLARE_NATIVE(Natives::RemPlayerAttachedObjForPlayer)
{
	CHECK_PARAMS(3, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if (!IsPlayerConnected(playerid)) return 0;

	const int removefromplayerid = CScriptParams::Get()->ReadInt();
	if (!IsPlayerConnected(removefromplayerid)) return 0;

	const int index = CScriptParams::Get()->ReadInt();
	if (index < 0 || index >= MAX_PLAYER_ATTACHED_OBJECTS) return 0;

	for (std::multimap<WORD, std::pair<WORD, std::unique_ptr<CAttachedObject>>>::iterator o = pPlayerData[playerid]->holdingObjects.begin(); o != pPlayerData[playerid]->holdingObjects.end(); ++o)
	{
		if (o->first == removefromplayerid)
		{
			if (o->second.first == index)
			{
				o = pPlayerData[playerid]->holdingObjects.erase(o);
				break;
			}
		}
	}

	RakNet::BitStream bsData;
	bsData.Write((WORD)playerid);
	bsData.Write(index);
	bsData.Write(false);
	CSAMPFunctions::RPC(&RPC_SetPlayerAttachedObject, &bsData, HIGH_PRIORITY, RELIABLE_ORDERED, 0, CSAMPFunctions::GetPlayerIDFromIndex(removefromplayerid), false, false);
	return 1;
}

// native IsPlayerAttachedObjForPlayer(forplayerid, attachtoplayerid, index);
AMX_DECLARE_NATIVE(Natives::IsPlayerAttachedObjForPlayer)
{
	CHECK_PARAMS(3, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if (!IsPlayerConnected(playerid)) return 0;

	const int removefromplayerid = CScriptParams::Get()->ReadInt();
	if (!IsPlayerConnected(removefromplayerid)) return 0;

	const int index = CScriptParams::Get()->ReadInt();
	if (index < 0 || index >= MAX_PLAYER_ATTACHED_OBJECTS) return 0;

	for (std::multimap<WORD, std::pair<WORD, std::unique_ptr<CAttachedObject>>>::iterator o = pPlayerData[playerid]->holdingObjects.begin(); o != pPlayerData[playerid]->holdingObjects.end(); ++o)
	{
		if (o->first == removefromplayerid)
		{
			if (o->second.first == index)
			{
				return 1;
			}
		}
	}
	return 0;
}

// Vehicle functions
// native GetVehicleSpawnInfo(vehicleid, &Float:fX, &Float:fY, &Float:fZ, &Float:fRot, &color1, &color2);
AMX_DECLARE_NATIVE(Natives::GetVehicleSpawnInfo)
{
	CHECK_PARAMS(7, LOADED);

	const int vehicleid = CScriptParams::Get()->ReadInt();
	if(vehicleid < 1 || vehicleid > MAX_VEHICLES) return 0;
	
	CVehicle* pVehicle = pNetGame->pVehiclePool->pVehicle[vehicleid];
	if(!pVehicle) 
		return 0;

	CVehicleSpawn spawn; 
	auto v = CServer::Get()->vehicleSpawnData.find(vehicleid);
	if(v == CServer::Get()->vehicleSpawnData.end())
	{
		spawn.vecPos = pVehicle->customSpawn.vecPos;
		spawn.fRot = pVehicle->customSpawn.fRot;
		spawn.iColor1 = pVehicle->customSpawn.iColor1;
		spawn.iColor2 = pVehicle->customSpawn.iColor2;
	}
	else
	{
		spawn.vecPos = v->second.vecPos;
		spawn.fRot = v->second.fRot;
		spawn.iColor1 = v->second.iColor1;
		spawn.iColor2 = v->second.iColor2;	
	}
	CScriptParams::Get()->Add(spawn.vecPos, spawn.fRot, spawn.iColor1, spawn.iColor2);
	return 1;
}

// native SetVehicleSpawnInfo(vehicleid, modelid, Float:fX, Float:fY, Float:fZ, Float:fAngle, color1, color2, respawntime = -2, interior = -2);
AMX_DECLARE_NATIVE(Natives::SetVehicleSpawnInfo)
{
	if (!CAddress::FUNC_CVehicle__Respawn) return 0;
	CHECK_PARAMS(10, LOADED);
	 
	const int vehicleid = CScriptParams::Get()->ReadInt();
	if(vehicleid < 1 || vehicleid > MAX_VEHICLES) return 0;
	
	int modelid = CScriptParams::Get()->ReadInt();
	if(modelid < 400 || modelid > 611) return 0;

	CVehicle *pVehicle = pNetGame->pVehiclePool->pVehicle[vehicleid]; 
	if(!pVehicle) 
		return 0;

	bool bStreamedIn = false;
	CPlayerPool *pPlayerPool = pNetGame->pPlayerPool;		

	for(WORD i = 0; i != MAX_PLAYERS; ++i)
	{
		if(IsPlayerConnected(i))
		{
			if(pPlayerPool->pPlayer[i]->byteVehicleStreamedIn[pVehicle->wVehicleID])
			{
				bStreamedIn = true;
				break;
			}
		}
	}

	CVehicleSpawn spawn;
	spawn.iModelID = modelid;
	CScriptParams::Get()->Read(&spawn.vecPos, &spawn.fRot, &spawn.iColor1, &spawn.iColor2);
	spawn.iRespawnTime = pVehicle->customSpawn.iRespawnTime;
	spawn.iInterior = pVehicle->customSpawn.iInterior;

	// logprintf("spawndata: %d, %d", spawn.iRespawnTime, spawn.iInterior);

	int respawntime = CScriptParams::Get()->ReadInt();
	if(respawntime >= -1)
	{
		spawn.iRespawnTime = respawntime;
	}

	int interior = CScriptParams::Get()->ReadInt();
	if(interior != -2)
	{
		spawn.iInterior = interior;
	}

	CServer::Get()->vehicleSpawnData[vehicleid] = spawn;

	// logprintf("streamedin: %d, iRespawnTime: %d, interior: %d", bStreamedIn, respawntime, interior);

	if(!bStreamedIn)
	{
		pVehicle->customSpawn.iModelID = spawn.iModelID;
		pVehicle->customSpawn.fRot = spawn.fRot;
		pVehicle->customSpawn.iColor1 = spawn.iColor1;
		pVehicle->customSpawn.iColor2 = spawn.iColor2;
		pVehicle->customSpawn.iRespawnTime = spawn.iRespawnTime;
		pVehicle->customSpawn.iInterior= spawn.iInterior;
	}
	return 1;
}

// native GetVehicleModelCount(modelid);
AMX_DECLARE_NATIVE(Natives::GetVehicleModelCount)
{
	CHECK_PARAMS(1, LOADED);

	const int modelid = CScriptParams::Get()->ReadInt();
	if(modelid < 400 || modelid > 611) return 0;
	
	return pNetGame->pVehiclePool->byteVehicleModelsUsed[modelid - 400];
}

// native GetVehicleModelsUsed();
AMX_DECLARE_NATIVE(Natives::GetVehicleModelsUsed)
{
	if (!CServer::Get()->IsInitialized()) return std::numeric_limits<int>::lowest(); // If unknown server version

	BYTE byteModelsUsed = 0;
	for(BYTE i = 0; i != 212; ++i)
	{
		if(pNetGame->pVehiclePool->byteVehicleModelsUsed[i] != 0)
			byteModelsUsed++;
	}

	return byteModelsUsed;
}

// native GetVehicleColor(vehicleid, &color1, &color2);
AMX_DECLARE_NATIVE(Natives::GetVehicleColor)
{
	CHECK_PARAMS(3, LOADED);

	const int vehicleid = CScriptParams::Get()->ReadInt();
	if(vehicleid < 1 || vehicleid > MAX_VEHICLES) return 0;
	
	if(!pNetGame->pVehiclePool->pVehicle[vehicleid]) 
		return 0;
	
	CVehicle *pVehicle = pNetGame->pVehiclePool->pVehicle[vehicleid];
	const int color1 = CServer::Get()->bChangedVehicleColor[vehicleid] ? pVehicle->vehModInfo.iColor1 : pVehicle->customSpawn.iColor1;
	const int color2 = CServer::Get()->bChangedVehicleColor[vehicleid] ? pVehicle->vehModInfo.iColor2 : pVehicle->customSpawn.iColor2;

	CScriptParams::Get()->Add(color1, color2);
	return 1;
}

// native GetVehiclePaintjob(vehicleid);
AMX_DECLARE_NATIVE(Natives::GetVehiclePaintjob)
{
	CHECK_PARAMS(1, LOADED);

	const int vehicleid = CScriptParams::Get()->ReadInt();
	if(vehicleid < 1 || vehicleid > MAX_VEHICLES) return 0;
	
	if(!pNetGame->pVehiclePool->pVehicle[vehicleid]) 
		return 0;

	return pNetGame->pVehiclePool->pVehicle[vehicleid]->vehModInfo.bytePaintJob - 1;
}

// native GetVehicleInterior(vehicleid);
AMX_DECLARE_NATIVE(Natives::GetVehicleInterior)
{
	CHECK_PARAMS(1, LOADED);

	const int vehicleid = CScriptParams::Get()->ReadInt();
	if(vehicleid < 1 || vehicleid > MAX_VEHICLES) return 0;
	
	if(!pNetGame->pVehiclePool->pVehicle[vehicleid]) 
		return 0;

	return pNetGame->pVehiclePool->pVehicle[vehicleid]->customSpawn.iInterior;
}

// native GetVehicleNumberPlate(vehicleid, plate[], len = sizeof(plate));
AMX_DECLARE_NATIVE(Natives::GetVehicleNumberPlate)
{
	CHECK_PARAMS(3, LOADED);

	const int vehicleid = CScriptParams::Get()->ReadInt();
	if(vehicleid < 1 || vehicleid > MAX_VEHICLES) return 0;
	
	if(!pNetGame->pVehiclePool->pVehicle[vehicleid]) 
		return 0;

	char szPlate[32 + 1];
	if (pNetGame->pVehiclePool->pVehicle[vehicleid]->szNumberplate[0])
	{
		strcpy(szPlate, pNetGame->pVehiclePool->pVehicle[vehicleid]->szNumberplate);
	}
	else
	{
		strcpy(szPlate, "XYZSR998");
	}
	CScriptParams::Get()->Add(&szPlate[0]);
	return 1;
}

// native SetVehicleRespawnDelay(vehicleid, delay);
AMX_DECLARE_NATIVE(Natives::SetVehicleRespawnDelay)
{
	CHECK_PARAMS(2, LOADED);

	const int vehicleid = CScriptParams::Get()->ReadInt();
	if(vehicleid < 1 || vehicleid > MAX_VEHICLES) return 0;
	
	if(!pNetGame->pVehiclePool->pVehicle[vehicleid]) 
		return 0;

	pNetGame->pVehiclePool->pVehicle[vehicleid]->customSpawn.iRespawnTime = (CScriptParams::Get()->ReadInt() * 1000);
	return 1;
}

// native GetVehicleRespawnDelay(vehicleid);
AMX_DECLARE_NATIVE(Natives::GetVehicleRespawnDelay)
{
	CHECK_PARAMS(1, LOADED);

	const int vehicleid = CScriptParams::Get()->ReadInt();
	if(vehicleid < 1 || vehicleid > MAX_VEHICLES) return 0;
	
	if(!pNetGame->pVehiclePool->pVehicle[vehicleid]) 
		return 0;

	return pNetGame->pVehiclePool->pVehicle[vehicleid]->customSpawn.iRespawnTime / 1000;
}

// native SetVehicleOccupiedTick(vehicleid, ticks);
AMX_DECLARE_NATIVE(Natives::SetVehicleOccupiedTick)
{
	CHECK_PARAMS(2, LOADED);

	const int vehicleid = CScriptParams::Get()->ReadInt();
	if(vehicleid < 1 || vehicleid > MAX_VEHICLES) return 0;
	
	if(!pNetGame->pVehiclePool->pVehicle[vehicleid]) 
		return 0;

	pNetGame->pVehiclePool->pVehicle[vehicleid]->vehOccupiedTick = CScriptParams::Get()->ReadInt();
	return 1;
}

// native GetVehicleOccupiedTick(vehicleid);
AMX_DECLARE_NATIVE(Natives::GetVehicleOccupiedTick)
{
	CHECK_PARAMS(1, LOADED);

	const int vehicleid = CScriptParams::Get()->ReadInt();
	if(vehicleid < 1 || vehicleid > MAX_VEHICLES) return 0;
	
	if(!pNetGame->pVehiclePool->pVehicle[vehicleid]) 
		return 0;

	return pNetGame->pVehiclePool->pVehicle[vehicleid]->vehOccupiedTick;
}

// native SetVehicleRespawnTick(vehicleid, ticks);
AMX_DECLARE_NATIVE(Natives::SetVehicleRespawnTick)
{
	CHECK_PARAMS(2, LOADED);

	const int vehicleid = CScriptParams::Get()->ReadInt();
	if(vehicleid < 1 || vehicleid > MAX_VEHICLES) return 0;
	
	if(!pNetGame->pVehiclePool->pVehicle[vehicleid]) 
		return 0;

	pNetGame->pVehiclePool->pVehicle[vehicleid]->vehRespawnTick = CScriptParams::Get()->ReadInt();
	return 1;
}

// native GetVehicleRespawnTick(vehicleid);
AMX_DECLARE_NATIVE(Natives::GetVehicleRespawnTick)
{
	CHECK_PARAMS(1, LOADED);

	const int vehicleid = CScriptParams::Get()->ReadInt();
	if(vehicleid < 1 || vehicleid > MAX_VEHICLES) return 0;
	
	if(!pNetGame->pVehiclePool->pVehicle[vehicleid]) 
		return 0;

	return pNetGame->pVehiclePool->pVehicle[vehicleid]->vehRespawnTick;
}

// native GetVehicleLastDriver(vehicleid);
AMX_DECLARE_NATIVE(Natives::GetVehicleLastDriver)
{
	CHECK_PARAMS(1, LOADED);

	const int vehicleid = CScriptParams::Get()->ReadInt();
	if(vehicleid < 1 || vehicleid > MAX_VEHICLES) return 0;
	
	if(!pNetGame->pVehiclePool->pVehicle[vehicleid]) 
		return 0;

	return pNetGame->pVehiclePool->pVehicle[vehicleid]->wLastDriverID;
}

// native GetVehicleCab(vehicleid);
AMX_DECLARE_NATIVE(Natives::GetVehicleCab)
{
	CHECK_PARAMS(1, LOADED);

	const int vehicleid = CScriptParams::Get()->ReadInt();
	if(vehicleid < 1 || vehicleid > MAX_VEHICLES) return 0;
	
	if(!pNetGame->pVehiclePool->pVehicle[vehicleid]) 
		return 0;
	
	CVehicle *pVeh;
	for(WORD i = 0; i != MAX_VEHICLES; ++i)
	{
		pVeh = pNetGame->pVehiclePool->pVehicle[i];
		if(!pVeh) continue;

		if(pVeh->wTrailerID != 0 && pVeh->wTrailerID == vehicleid)
			return i;
	}
	return 0;
}

// native HasVehicleBeenOccupied(vehicleid);
AMX_DECLARE_NATIVE(Natives::HasVehicleBeenOccupied)
{
	CHECK_PARAMS(1, LOADED);

	const int vehicleid = CScriptParams::Get()->ReadInt();
	if(vehicleid < 1 || vehicleid > MAX_VEHICLES) return 0;
	
	if(!pNetGame->pVehiclePool->pVehicle[vehicleid]) 
		return 0;

	return pNetGame->pVehiclePool->pVehicle[vehicleid]->bOccupied;
}

// native SetVehicleBeenOccupied(vehicleid, occupied);
AMX_DECLARE_NATIVE(Natives::SetVehicleBeenOccupied)
{
	CHECK_PARAMS(2, LOADED);

	const int vehicleid = CScriptParams::Get()->ReadInt();
	if(vehicleid < 1 || vehicleid > MAX_VEHICLES) return 0;
	
	if(!pNetGame->pVehiclePool->pVehicle[vehicleid]) 
		return 0;

	pNetGame->pVehiclePool->pVehicle[vehicleid]->bOccupied = static_cast<BYTE>(params[2]) != 0;
	return 1;
}

// native IsVehicleOccupied(vehicleid);
AMX_DECLARE_NATIVE(Natives::IsVehicleOccupied)
{
	CHECK_PARAMS(1, LOADED);

	const int vehicleid = CScriptParams::Get()->ReadInt();
	if(vehicleid < 1 || vehicleid > MAX_VEHICLES) return 0;
	
	CPlayer *pPlayer;
	for(WORD i = 0; i != MAX_PLAYERS; ++i)
	{
		if(!IsPlayerConnected(i)) continue; 
		pPlayer = pNetGame->pPlayerPool->pPlayer[i];

		if(pPlayer->wVehicleId == vehicleid && (pPlayer->byteState == PLAYER_STATE_DRIVER || pPlayer->byteState == PLAYER_STATE_PASSENGER)) 
			return 1;
	}
	return 0;
}

// native IsVehicleDead(vehicleid);
AMX_DECLARE_NATIVE(Natives::IsVehicleDead)
{
	CHECK_PARAMS(1, LOADED);

	const int vehicleid = CScriptParams::Get()->ReadInt();
	if(vehicleid < 1 || vehicleid > MAX_VEHICLES) return 0;
	
	if(!pNetGame->pVehiclePool->pVehicle[vehicleid]) 
		return 0;

	return pNetGame->pVehiclePool->pVehicle[vehicleid]->bDead;
}

// native SetVehicleParamsSirenState(vehicleid, sirenState);
AMX_DECLARE_NATIVE(Natives::SetVehicleParamsSirenState)
{
	CHECK_PARAMS(2, LOADED);

	const int vehicleid = CScriptParams::Get()->ReadInt();
	if (vehicleid < 1 || vehicleid > MAX_VEHICLES) return 0;

	if (!pNetGame->pVehiclePool->pVehicle[vehicleid])
		return 0;

	BYTE sirenState = static_cast<BYTE>(params[2]);

	pNetGame->pVehiclePool->pVehicle[vehicleid]->vehParamEx.siren = sirenState;
	return 1;
}

// native ToggleVehicleSirenEnabled(vehicleid, enabled);
AMX_DECLARE_NATIVE(Natives::ToggleVehicleSirenEnabled)
{
	CHECK_PARAMS(2, LOADED);

	const int vehicleid = CScriptParams::Get()->ReadInt();
	if (vehicleid < 1 || vehicleid > MAX_VEHICLES) return 0;

	if (!pNetGame->pVehiclePool->pVehicle[vehicleid])
		return 0;

	BYTE enabled = static_cast<BYTE>(params[2]);

	pNetGame->pVehiclePool->pVehicle[vehicleid]->byteSirenEnabled = enabled;
	return 1;
}

// native IsVehicleSirenEnabled(vehicleid);
AMX_DECLARE_NATIVE(Natives::IsVehicleSirenEnabled)
{
	CHECK_PARAMS(1, LOADED);

	const int vehicleid = CScriptParams::Get()->ReadInt();
	if (vehicleid < 1 || vehicleid > MAX_VEHICLES) return 0;

	if (!pNetGame->pVehiclePool->pVehicle[vehicleid])
		return 0;

	return pNetGame->pVehiclePool->pVehicle[vehicleid]->byteSirenEnabled;
}

// native GetVehicleMatrix(vehicleid, &Float:rightX, &Float:rightY, &Float:rightZ, &Float:upX, &Float:upY, &Float:upZ, &Float:atX, &Float:atY, &Float:atZ);
AMX_DECLARE_NATIVE(Natives::GetVehicleMatrix)
{
	CHECK_PARAMS(10, LOADED);

	const int vehicleid = CScriptParams::Get()->ReadInt();
	if (vehicleid < 1 || vehicleid > MAX_VEHICLES) return 0;

	if (!pNetGame->pVehiclePool->pVehicle[vehicleid])
		return 0;

	MATRIX4X4* matrix = &pNetGame->pVehiclePool->pVehicle[vehicleid]->vehMatrix;

	// IS4 Note for the future - vehicles remotely updated have the "at" vector zeroed, which corrupts results from GetVehicleRotationQuat
	// Might be a reasonable method to replace IsValidQuaternion from i_quat
	CScriptParams::Get()->Add(matrix->right, matrix->up, matrix->at);
	return 1;
}

// Gangzone functions
// native IsValidGangZone(zoneid);
AMX_DECLARE_NATIVE(Natives::IsValidGangZone)
{
	CHECK_PARAMS(1, LOADED);
	
	const int zoneid = CScriptParams::Get()->ReadInt();
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;
	
	return CServer::Get()->pGangZonePool->GetSlotState(static_cast<WORD>(zoneid));
}

// native IsGangZoneVisibleForPlayer(playerid, zoneid);
AMX_DECLARE_NATIVE(Natives::IsGangZoneVisibleForPlayer)
{
	CHECK_PARAMS(2, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int zoneid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;

	if(!CServer::Get()->pGangZonePool->GetSlotState(static_cast<WORD>(zoneid))) return 0;

	return !!(pPlayerData[playerid]->GetGangZoneIDFromClientSide(static_cast<WORD>(zoneid)) != 0xFF);
}

// native GangZoneGetColorForPlayer(playerid, zoneid);
AMX_DECLARE_NATIVE(Natives::GangZoneGetColorForPlayer)
{
	CHECK_PARAMS(2, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int zoneid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;
	
	if(!CServer::Get()->pGangZonePool->GetSlotState(static_cast<WORD>(zoneid))) return 0;

	WORD id = pPlayerData[playerid]->GetGangZoneIDFromClientSide(static_cast<WORD>(zoneid));
	if(id != 0xFFFF) 
	{
		return pPlayerData[playerid]->dwClientSideZoneColor[id];
	}
	return 0;
}

// native GangZoneGetFlashColorForPlayer(playerid, zoneid);
AMX_DECLARE_NATIVE(Natives::GangZoneGetFlashColorForPlayer)
{
	CHECK_PARAMS(2, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int zoneid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;
	
	if(!CServer::Get()->pGangZonePool->GetSlotState(static_cast<WORD>(zoneid))) return 0;

	WORD id = pPlayerData[playerid]->GetGangZoneIDFromClientSide(static_cast<WORD>(zoneid));
	if(id != 0xFFFF) 
	{
		return pPlayerData[playerid]->dwClientSideZoneFlashColor[id];
	}
	return 0;
}

// native IsGangZoneFlashingForPlayer(playerid, zoneid);
AMX_DECLARE_NATIVE(Natives::IsGangZoneFlashingForPlayer)
{
	CHECK_PARAMS(2, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int zoneid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;
	
	if(!CServer::Get()->pGangZonePool->GetSlotState(static_cast<WORD>(zoneid))) return 0;

	WORD id = pPlayerData[playerid]->GetGangZoneIDFromClientSide(static_cast<WORD>(zoneid));
	if(id != 0xFFFF) 
	{
		return pPlayerData[playerid]->bIsGangZoneFlashing[id];
	}
	return 0;
}

// native IsPlayerInGangZone(playerid, zoneid);
AMX_DECLARE_NATIVE(Natives::IsPlayerInGangZone)
{
	CHECK_PARAMS(2, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int zoneid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;
	
	if(!CServer::Get()->pGangZonePool->GetSlotState(static_cast<WORD>(zoneid))) return 0;

	WORD id = pPlayerData[playerid]->GetGangZoneIDFromClientSide(static_cast<WORD>(zoneid));
	if(id != 0xFFFF) 
	{
		return pPlayerData[playerid]->bInGangZone[id];
	}
	return 0;
}

// native GangZoneGetPos(zoneid, &Float:fMinX, &Float:fMinY, &Float:fMaxX, &Float:fMaxY);
AMX_DECLARE_NATIVE(Natives::GangZoneGetPos)
{
	CHECK_PARAMS(5, LOADED);

	const int zoneid = CScriptParams::Get()->ReadInt();
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;
	
	if(!CServer::Get()->pGangZonePool->GetSlotState(static_cast<WORD>(zoneid)))  return 0;
	
	CGangZone *pGangZone = CServer::Get()->pGangZonePool->pGangZone[zoneid];
	CScriptParams::Get()->Add(pGangZone->fGangZone[0], pGangZone->fGangZone[1], pGangZone->fGangZone[2], pGangZone->fGangZone[3]);
	return 1;
}

// Textdraw functions
// native IsValidTextDraw(Text:textdrawid);
AMX_DECLARE_NATIVE(Natives::IsValidTextDraw)
{
	CHECK_PARAMS(1, LOADED);
	
	const int textdrawid = CScriptParams::Get()->ReadInt();
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	return pNetGame->pTextDrawPool->bSlotState[textdrawid];
}

// native IsTextDrawVisibleForPlayer(playerid, Text:textdrawid);
AMX_DECLARE_NATIVE(Natives::IsTextDrawVisibleForPlayer)
{
	CHECK_PARAMS(2, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	if(playerid < 0 || playerid >= MAX_PLAYERS) return 0;

	const int textdrawid = CScriptParams::Get()->ReadInt();
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	if(!pNetGame->pTextDrawPool->bSlotState[textdrawid]) return 0;

	return pNetGame->pTextDrawPool->bHasText[textdrawid][playerid];
}

// native TextDrawGetString(Text:textdrawid, text[], len = sizeof(text));
AMX_DECLARE_NATIVE(Natives::TextDrawGetString)
{
	CHECK_PARAMS(3, LOADED);
	
	const int textdrawid = CScriptParams::Get()->ReadInt();
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	const char *szText = (pNetGame->pTextDrawPool->bSlotState[textdrawid]) ? pNetGame->pTextDrawPool->szFontText[textdrawid] : '\0';
	return set_amxstring(amx, params[2], szText, params[3]);
}

// native TextDrawSetPos(Text:textdrawid, Float:fX, Float:fY);
AMX_DECLARE_NATIVE(Natives::TextDrawSetPos)
{
	CHECK_PARAMS(3, LOADED);
	
	const int textdrawid = CScriptParams::Get()->ReadInt();
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	if(!pNetGame->pTextDrawPool->bSlotState[textdrawid]) return 0;
	CTextdraw *pTD = pNetGame->pTextDrawPool->TextDraw[textdrawid];

	CScriptParams::Get()->Read(&pTD->vecPos);
	return 1;
}

// native TextDrawGetLetterSize(Text:textdrawid, &Float:fX, &Float:fY);
AMX_DECLARE_NATIVE(Natives::TextDrawGetLetterSize)
{
	CHECK_PARAMS(3, LOADED);
	
	const int textdrawid = CScriptParams::Get()->ReadInt();
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	if(!pNetGame->pTextDrawPool->bSlotState[textdrawid]) return 0;
	CTextdraw *pTD = pNetGame->pTextDrawPool->TextDraw[textdrawid];

	CScriptParams::Get()->Add(pTD->vecLetter);
	return 1;
}

// native TextDrawGetTextSize(Text:textdrawid, &Float:fX, &Float:fY);
AMX_DECLARE_NATIVE(Natives::TextDrawGetTextSize)
{
	CHECK_PARAMS(3, LOADED);
	
	const int textdrawid = CScriptParams::Get()->ReadInt();
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	if(!pNetGame->pTextDrawPool->bSlotState[textdrawid]) return 0;
	CTextdraw *pTD = pNetGame->pTextDrawPool->TextDraw[textdrawid];

	CScriptParams::Get()->Add(pTD->vecLine);
	return 1;
}

// native TextDrawGetPos(Text:textdrawid, &Float:fX, &Float:fY);
AMX_DECLARE_NATIVE(Natives::TextDrawGetPos)
{
	CHECK_PARAMS(3, LOADED);
	
	const int textdrawid = CScriptParams::Get()->ReadInt();
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	if(!pNetGame->pTextDrawPool->bSlotState[textdrawid]) return 0;
	CTextdraw *pTD = pNetGame->pTextDrawPool->TextDraw[textdrawid];

	CScriptParams::Get()->Add(pTD->vecPos);
	return 1;
}

// native TextDrawGetColor(Text:textdrawid);
AMX_DECLARE_NATIVE(Natives::TextDrawGetColor)
{
	CHECK_PARAMS(1, LOADED);
	
	const int textdrawid = CScriptParams::Get()->ReadInt();
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	if(!pNetGame->pTextDrawPool->bSlotState[textdrawid]) return 0;
	CTextdraw *pTD = pNetGame->pTextDrawPool->TextDraw[textdrawid];

	return ABGR_RGBA(pTD->dwLetterColor);
}

// native TextDrawGetBoxColor(Text:textdrawid);
AMX_DECLARE_NATIVE(Natives::TextDrawGetBoxColor)
{
	CHECK_PARAMS(1, LOADED);
	
	const int textdrawid = CScriptParams::Get()->ReadInt();
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	if(!pNetGame->pTextDrawPool->bSlotState[textdrawid]) return 0;
	CTextdraw *pTD = pNetGame->pTextDrawPool->TextDraw[textdrawid];

	return ABGR_RGBA(pTD->dwBoxColor);
}

// native TextDrawGetBackgroundColor(Text:textdrawid);
AMX_DECLARE_NATIVE(Natives::TextDrawGetBackgroundColor)
{
	CHECK_PARAMS(1, LOADED);
	
	const int textdrawid = CScriptParams::Get()->ReadInt();
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	if(!pNetGame->pTextDrawPool->bSlotState[textdrawid]) return 0;
	CTextdraw *pTD = pNetGame->pTextDrawPool->TextDraw[textdrawid];

	return ABGR_RGBA(pTD->dwBackgroundColor);
}

// native TextDrawGetShadow(Text:textdrawid);
AMX_DECLARE_NATIVE(Natives::TextDrawGetShadow)
{
	CHECK_PARAMS(1, LOADED);
	
	const int textdrawid = CScriptParams::Get()->ReadInt();
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	if(!pNetGame->pTextDrawPool->bSlotState[textdrawid]) return 0;
	CTextdraw *pTD = pNetGame->pTextDrawPool->TextDraw[textdrawid];

	return pTD->byteShadow;
}

// native TextDrawGetOutline(Text:textdrawid);
AMX_DECLARE_NATIVE(Natives::TextDrawGetOutline)
{
	CHECK_PARAMS(1, LOADED);
	
	const int textdrawid = CScriptParams::Get()->ReadInt();
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	if(!pNetGame->pTextDrawPool->bSlotState[textdrawid]) return 0;
	CTextdraw *pTD = pNetGame->pTextDrawPool->TextDraw[textdrawid];

	return pTD->byteOutline;
}

// native TextDrawGetFont(Text:textdrawid);
AMX_DECLARE_NATIVE(Natives::TextDrawGetFont)
{
	CHECK_PARAMS(1, LOADED);
	
	const int textdrawid = CScriptParams::Get()->ReadInt();
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	if(!pNetGame->pTextDrawPool->bSlotState[textdrawid]) return 0;
	CTextdraw *pTD = pNetGame->pTextDrawPool->TextDraw[textdrawid];

	return pTD->byteStyle;
}

// native TextDrawIsBox(Text:textdrawid);
AMX_DECLARE_NATIVE(Natives::TextDrawIsBox)
{
	CHECK_PARAMS(1, LOADED);
	
	const int textdrawid = CScriptParams::Get()->ReadInt();
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	if(!pNetGame->pTextDrawPool->bSlotState[textdrawid]) return 0;
	CTextdraw *pTD = pNetGame->pTextDrawPool->TextDraw[textdrawid];

	return pTD->byteBox;
}

// native TextDrawIsProportional(Text:textdrawid);
AMX_DECLARE_NATIVE(Natives::TextDrawIsProportional)
{
	CHECK_PARAMS(1, LOADED);
	
	const int textdrawid = CScriptParams::Get()->ReadInt();
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	if(!pNetGame->pTextDrawPool->bSlotState[textdrawid]) return 0;
	CTextdraw *pTD = pNetGame->pTextDrawPool->TextDraw[textdrawid];

	return pTD->byteProportional;
}

// native TextDrawIsSelectable(Text:textdrawid);
AMX_DECLARE_NATIVE(Natives::TextDrawIsSelectable)
{
	CHECK_PARAMS(1, LOADED);
	
	const int textdrawid = CScriptParams::Get()->ReadInt();
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	if(!pNetGame->pTextDrawPool->bSlotState[textdrawid]) return 0;
	CTextdraw *pTD = pNetGame->pTextDrawPool->TextDraw[textdrawid];

	return pTD->byteSelectable;
}

// native TextDrawGetAlignment(Text:textdrawid);
AMX_DECLARE_NATIVE(Natives::TextDrawGetAlignment)
{
	CHECK_PARAMS(1, LOADED);
	
	const int textdrawid = CScriptParams::Get()->ReadInt();
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	if(!pNetGame->pTextDrawPool->bSlotState[textdrawid]) return 0;
	CTextdraw *pTD = pNetGame->pTextDrawPool->TextDraw[textdrawid];

	BYTE ret = 0;

	if(pTD->byteCenter) ret = 2;
	else if(pTD->byteLeft) ret = 1;
	else if(pTD->byteRight) ret = 3;
	return ret;
}

// native TextDrawGetPreviewModel(Text:textdrawid);
AMX_DECLARE_NATIVE(Natives::TextDrawGetPreviewModel)
{
	CHECK_PARAMS(1, LOADED);
	
	const int textdrawid = CScriptParams::Get()->ReadInt();
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	if(!pNetGame->pTextDrawPool->bSlotState[textdrawid]) return 0;
	CTextdraw *pTD = pNetGame->pTextDrawPool->TextDraw[textdrawid];

	return pTD->dwModelIndex;
}

// native TextDrawGetPreviewRot(Text:textdrawid, &Float:fRotX, &Float:fRotY, &Float:fRotZ, &Float:fZoom);
AMX_DECLARE_NATIVE(Natives::TextDrawGetPreviewRot)
{
	CHECK_PARAMS(5, LOADED);
	
	const int textdrawid = CScriptParams::Get()->ReadInt();
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	if(!pNetGame->pTextDrawPool->bSlotState[textdrawid]) return 0;
	CTextdraw *pTD = pNetGame->pTextDrawPool->TextDraw[textdrawid];
	
	CScriptParams::Get()->Add(pTD->vecRot, pTD->fZoom);
	return 1;
}

// native TextDrawGetPreviewVehCol(Text:textdrawid, &color1, &color2);
AMX_DECLARE_NATIVE(Natives::TextDrawGetPreviewVehCol)
{
	CHECK_PARAMS(3, LOADED);
	
	const int textdrawid = CScriptParams::Get()->ReadInt();
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	if(!pNetGame->pTextDrawPool->bSlotState[textdrawid]) return 0;
	CTextdraw *pTD = pNetGame->pTextDrawPool->TextDraw[textdrawid];
	
	CScriptParams::Get()->Add(pTD->color1, pTD->color2);
	return 1;
}

// native TextDrawSetStringForPlayer(Text:textdrawid, playerid, const string[], {Float,_}:...);
AMX_DECLARE_NATIVE(Natives::TextDrawSetStringForPlayer)
{
	if (CScriptParams::Get()->Setup(3, __FUNCTION__, static_cast<CScriptParams::Flags>(CScriptParams::Flags::LOADED | CScriptParams::Flags::MORE_PARAMETER_ALLOWED), amx, params)) return CScriptParams::Get()->HandleError();

	const int textdrawid = CScriptParams::Get()->ReadInt();
	if (textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;

	const int playerid = CScriptParams::Get()->ReadInt();
	if (!IsPlayerConnected(playerid)) return 0;

	if (!pNetGame->pTextDrawPool->bSlotState[textdrawid]) return 0;
	CTextdraw *pTD = pNetGame->pTextDrawPool->TextDraw[textdrawid];

	int len;
	char* szMessage = CSAMPFunctions::format_amxstring(amx, params, 3, len);
	if (!szMessage) return 0;

	RakNet::BitStream bs;
	bs.Write((WORD)textdrawid);
	bs.Write((WORD)len);
	bs.Write(szMessage, len + 1);

	CSAMPFunctions::RPC(&RPC_SetTextDrawString, &bs, HIGH_PRIORITY, RELIABLE, 0, CSAMPFunctions::GetPlayerIDFromIndex(playerid), false, false);
	return 1;
}

// Per-Player textdraws
// native IsValidPlayerTextDraw(playerid, PlayerText:textdrawid);
AMX_DECLARE_NATIVE(Natives::IsValidPlayerTextDraw)
{
	CHECK_PARAMS(2, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int textdrawid = CScriptParams::Get()->ReadInt();

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid]) return 0;

	return pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid];
}

// native IsPlayerTextDrawVisible(playerid, PlayerText:textdrawid);
AMX_DECLARE_NATIVE(Natives::IsPlayerTextDrawVisible)
{
	CHECK_PARAMS(2, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int textdrawid = CScriptParams::Get()->ReadInt();

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid]) return 0;

	return pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bHasText[textdrawid];
}

// native PlayerTextDrawGetString(playerid, PlayerText:textdrawid, text[], len = sizeof(text));
AMX_DECLARE_NATIVE(Natives::PlayerTextDrawGetString)
{
	CHECK_PARAMS(4, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int textdrawid = CScriptParams::Get()->ReadInt();

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	
	bool bIsValid = static_cast<int>(pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid]) != 0;
	if(!bIsValid) return 0;

	const char *szText = (bIsValid) ? pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->szFontText[textdrawid]: '\0';
	return set_amxstring(amx, params[3], szText, params[4]);
}

// native PlayerTextDrawSetPos(playerid, PlayerText:textdrawid, Float:fX, Float:fY);
AMX_DECLARE_NATIVE(Natives::PlayerTextDrawSetPos)
{
	CHECK_PARAMS(4, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int textdrawid = CScriptParams::Get()->ReadInt();

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid]) return 0;

	CTextdraw *pTD = pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->TextDraw[textdrawid];

	CScriptParams::Get()->Read(&pTD->vecPos);
	return 1;
}

// native PlayerTextDrawGetLetterSize(playerid, PlayerText:textdrawid, &Float:fX, &Float:fY);
AMX_DECLARE_NATIVE(Natives::PlayerTextDrawGetLetterSize)
{
	CHECK_PARAMS(4, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int textdrawid = CScriptParams::Get()->ReadInt();

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid]) return 0;

	CTextdraw *pTD = pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->TextDraw[textdrawid];

	CScriptParams::Get()->Add(pTD->vecLetter);
	return 1;
}

// native PlayerTextDrawGetTextSize(playerid, PlayerText:textdrawid, &Float:fX, &Float:fY);
AMX_DECLARE_NATIVE(Natives::PlayerTextDrawGetTextSize)
{
	CHECK_PARAMS(4, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int textdrawid = CScriptParams::Get()->ReadInt();

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid]) return 0;

	CTextdraw *pTD = pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->TextDraw[textdrawid];
	
	CScriptParams::Get()->Add(pTD->vecLine);
	return 1;
}

// native PlayerTextDrawGetPos(playerid, PlayerText:textdrawid, &Float:fX, &Float:fY);
AMX_DECLARE_NATIVE(Natives::PlayerTextDrawGetPos)
{
	CHECK_PARAMS(4, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int textdrawid = CScriptParams::Get()->ReadInt();

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid]) return 0;

	CTextdraw *pTD = pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->TextDraw[textdrawid];

	CScriptParams::Get()->Add(pTD->vecPos);
	return 1;
}

// native PlayerTextDrawGetColor(playerid, PlayerText:textdrawid);
AMX_DECLARE_NATIVE(Natives::PlayerTextDrawGetColor)
{
	CHECK_PARAMS(2, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int textdrawid = CScriptParams::Get()->ReadInt();

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid]) return 0;

	CTextdraw *pTD = pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->TextDraw[textdrawid];
	return ABGR_RGBA(pTD->dwLetterColor);
}

// native PlayerTextDrawGetBoxColor(playerid, PlayerText:textdrawid);
AMX_DECLARE_NATIVE(Natives::PlayerTextDrawGetBoxColor)
{
	CHECK_PARAMS(2, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int textdrawid = CScriptParams::Get()->ReadInt();

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid]) return 0;

	CTextdraw *pTD = pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->TextDraw[textdrawid];
	return ABGR_RGBA(pTD->dwBoxColor);
}

// native PlayerTextDrawGetBackgroundCol(playerid, PlayerText:textdrawid);
AMX_DECLARE_NATIVE(Natives::PlayerTextDrawGetBackgroundCol)
{
	CHECK_PARAMS(2, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int textdrawid = CScriptParams::Get()->ReadInt();

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid]) return 0;

	CTextdraw *pTD = pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->TextDraw[textdrawid];
	return ABGR_RGBA(pTD->dwBackgroundColor);
}

// native PlayerTextDrawGetShadow(playerid, PlayerText:textdrawid);
AMX_DECLARE_NATIVE(Natives::PlayerTextDrawGetShadow)
{
	CHECK_PARAMS(2, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int textdrawid = CScriptParams::Get()->ReadInt();

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid]) return 0;

	CTextdraw *pTD = pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->TextDraw[textdrawid];
	return pTD->byteShadow;
}

// native PlayerTextDrawGetOutline(playerid, PlayerText:textdrawid);
AMX_DECLARE_NATIVE(Natives::PlayerTextDrawGetOutline)
{
	CHECK_PARAMS(2, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int textdrawid = CScriptParams::Get()->ReadInt();

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid]) return 0;

	CTextdraw *pTD = pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->TextDraw[textdrawid];
	return pTD->byteOutline;
}

// native PlayerTextDrawGetFont(playerid, PlayerText:textdrawid);
AMX_DECLARE_NATIVE(Natives::PlayerTextDrawGetFont)
{
	CHECK_PARAMS(2, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int textdrawid = CScriptParams::Get()->ReadInt();

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid]) return 0;

	CTextdraw *pTD = pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->TextDraw[textdrawid];
	return pTD->byteStyle;
}

// native PlayerTextDrawIsBox(playerid, PlayerText:textdrawid);
AMX_DECLARE_NATIVE(Natives::PlayerTextDrawIsBox)
{
	CHECK_PARAMS(2, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int textdrawid = CScriptParams::Get()->ReadInt();

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid]) return 0;

	CTextdraw *pTD = pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->TextDraw[textdrawid];
	return pTD->byteBox;
}

// native PlayerTextDrawIsProportional(playerid, PlayerText:textdrawid);
AMX_DECLARE_NATIVE(Natives::PlayerTextDrawIsProportional)
{
	CHECK_PARAMS(2, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int textdrawid = CScriptParams::Get()->ReadInt();

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid]) return 0;

	CTextdraw *pTD = pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->TextDraw[textdrawid];
	return pTD->byteProportional;
}

// native PlayerTextDrawIsSelectable(playerid, PlayerText:textdrawid);
AMX_DECLARE_NATIVE(Natives::PlayerTextDrawIsSelectable)
{
	CHECK_PARAMS(2, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int textdrawid = CScriptParams::Get()->ReadInt();

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid]) return 0;

	CTextdraw *pTD = pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->TextDraw[textdrawid];
	return pTD->byteSelectable;
}

// native PlayerTextDrawGetAlignment(playerid, PlayerText:textdrawid);
AMX_DECLARE_NATIVE(Natives::PlayerTextDrawGetAlignment)
{
	CHECK_PARAMS(2, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int textdrawid = CScriptParams::Get()->ReadInt();

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid]) return 0;

	CTextdraw *pTD = pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->TextDraw[textdrawid];
	BYTE ret = 0;

	if(pTD->byteCenter) ret = 2;
	else if(pTD->byteLeft) ret = 1;
	else if(pTD->byteRight) ret = 3;
	return ret;
}

// native PlayerTextDrawGetPreviewModel(playerid, PlayerText:textdrawid);
AMX_DECLARE_NATIVE(Natives::PlayerTextDrawGetPreviewModel)
{
	CHECK_PARAMS(2, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int textdrawid = CScriptParams::Get()->ReadInt();

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid]) return 0;

	CTextdraw *pTD = pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->TextDraw[textdrawid];
	return pTD->dwModelIndex;
}

// native PlayerTextDrawGetPreviewRot(playerid, PlayerText:textdrawid, &Float:fRotX, &Float:fRotY, &Float:fRotZ, &Float:fZoom);
AMX_DECLARE_NATIVE(Natives::PlayerTextDrawGetPreviewRot)
{
	CHECK_PARAMS(6, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int textdrawid = CScriptParams::Get()->ReadInt();

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid]) return 0;

	CTextdraw *pTD = pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->TextDraw[textdrawid];

	CScriptParams::Get()->Add(pTD->vecRot, pTD->fZoom);
	return 1;
}

// native PlayerTextDrawGetPreviewVehCol(playerid, PlayerText:textdrawid, &color1, &color2);
AMX_DECLARE_NATIVE(Natives::PlayerTextDrawGetPreviewVehCol)
{
	CHECK_PARAMS(4, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int textdrawid = CScriptParams::Get()->ReadInt();

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid]) return 0;

	CTextdraw *pTD = pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->TextDraw[textdrawid];

	CScriptParams::Get()->Add(pTD->color1, pTD->color2);
	return 1;
}

// 3D Text labels
// native IsValid3DTextLabel(Text3D:id);
AMX_DECLARE_NATIVE(Natives::IsValid3DTextLabel)
{
	CHECK_PARAMS(1, LOADED);
	
	int id = CScriptParams::Get()->ReadInt();
	if(0 < id || id >= MAX_3DTEXT_GLOBAL) return 0;
	
	return pNetGame->p3DTextPool->bIsCreated[id];
}

// native Is3DTextLabelStreamedIn(playerid, Text3D:id);
AMX_DECLARE_NATIVE(Natives::Is3DTextLabelStreamedIn)
{
	CHECK_PARAMS(2, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	int id = CScriptParams::Get()->ReadInt();

	if(!IsPlayerConnected(playerid)) return 0;
	if(0 < id || id >= MAX_3DTEXT_PLAYER) return 0;
	
	return pNetGame->pPlayerPool->pPlayer[playerid]->byte3DTextLabelStreamedIn[id];
}

// native Get3DTextLabelText(id, text[], len = sizeof(text));
AMX_DECLARE_NATIVE(Natives::Get3DTextLabelText)
{
	CHECK_PARAMS(3, LOADED);
	
	const int id = CScriptParams::Get()->ReadInt();
	if(id < 0 || id >= MAX_3DTEXT_GLOBAL) return 0;

	if(!pNetGame->p3DTextPool->bIsCreated[id]) return 0;

	const char *szText = pNetGame->p3DTextPool->TextLabels[id].szText;
	return set_amxstring(amx, params[2], szText, params[3]);
}

// native Get3DTextLabelColor(id);
AMX_DECLARE_NATIVE(Natives::Get3DTextLabelColor)
{
	CHECK_PARAMS(1, LOADED);
	
	const int id = CScriptParams::Get()->ReadInt();
	if(id < 0 || id >= MAX_3DTEXT_GLOBAL) return 0;
	
	if(!pNetGame->p3DTextPool->bIsCreated[id]) return 0;
	C3DText &p3DText = pNetGame->p3DTextPool->TextLabels[id];

	return p3DText.dwColor;
}

// native Get3DTextLabelPos(id, &Float:fX, &Float:fY, &Float:fZ);
AMX_DECLARE_NATIVE(Natives::Get3DTextLabelPos)
{
	CHECK_PARAMS(4, LOADED);
	
	const int id = CScriptParams::Get()->ReadInt();
	if(id < 0 || id >= MAX_3DTEXT_GLOBAL) return 0;
	
	if(!pNetGame->p3DTextPool->bIsCreated[id]) return 0;
	C3DText p3DText = pNetGame->p3DTextPool->TextLabels[id];

	CScriptParams::Get()->Add(p3DText.vecPos);
	return 1;
}

// native Float:Get3DTextLabelDrawDistance(id);
AMX_DECLARE_NATIVE(Natives::Get3DTextLabelDrawDistance)
{
	CHECK_PARAMS(1, LOADED);
	
	const int id = CScriptParams::Get()->ReadInt();
	if(id < 0 || id >= MAX_3DTEXT_GLOBAL) return 0;
	
	if(!pNetGame->p3DTextPool->bIsCreated[id]) return 0;
	C3DText p3DText = pNetGame->p3DTextPool->TextLabels[id];

	return amx_ftoc(p3DText.fDrawDistance);
}

// native Get3DTextLabelLOS(id);
AMX_DECLARE_NATIVE(Natives::Get3DTextLabelLOS)
{
	CHECK_PARAMS(1, LOADED);
	
	const int id = CScriptParams::Get()->ReadInt();
	if(id < 0 || id >= MAX_3DTEXT_GLOBAL) return 0;
	
	if(!pNetGame->p3DTextPool->bIsCreated[id]) return 0;
	C3DText p3DText = pNetGame->p3DTextPool->TextLabels[id];

	return p3DText.bLineOfSight;
}

// native Get3DTextLabelVirtualWorld(id);
AMX_DECLARE_NATIVE(Natives::Get3DTextLabelVirtualWorld)
{
	CHECK_PARAMS(1, LOADED);
	
	const int id = CScriptParams::Get()->ReadInt();
	if(id < 0 || id >= MAX_3DTEXT_GLOBAL) return 0;
	
	if(!pNetGame->p3DTextPool->bIsCreated[id]) return 0;
	C3DText p3DText = pNetGame->p3DTextPool->TextLabels[id];

	return p3DText.iWorld;
}

// native Get3DTextLabelAttachedData(id, &attached_playerid, &attached_vehicleid);
AMX_DECLARE_NATIVE(Natives::Get3DTextLabelAttachedData)
{
	CHECK_PARAMS(3, LOADED);
	
	const int id = CScriptParams::Get()->ReadInt();
	if(id < 0 || id >= MAX_3DTEXT_GLOBAL) return 0;
	
	if(!pNetGame->p3DTextPool->bIsCreated[id]) return 0;
	C3DText p3DText = pNetGame->p3DTextPool->TextLabels[id];

	CScriptParams::Get()->Add(p3DText.wAttachedToPlayerID, p3DText.wAttachedToVehicleID);
	return 1;
}

// native IsValidPlayer3DTextLabel(playerid, PlayerText3D:id);
AMX_DECLARE_NATIVE(Natives::IsValidPlayer3DTextLabel)
{
	CHECK_PARAMS(2, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int id = CScriptParams::Get()->ReadInt();

	if(!IsPlayerConnected(playerid)) return 0;
	if(id < 0 || id >= MAX_3DTEXT_PLAYER) return 0;
	
	return pNetGame->pPlayerPool->pPlayer[playerid]->p3DText->isCreated[id];
}

// native GetPlayer3DTextLabelText(playerid, PlayerText3D:id, text[], len = sizeof(text));
AMX_DECLARE_NATIVE(Natives::GetPlayer3DTextLabelText)
{
	CHECK_PARAMS(4, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int id = CScriptParams::Get()->ReadInt();

	if(!IsPlayerConnected(playerid)) return 0;
	if(id < 0 || id >= MAX_3DTEXT_PLAYER) return 0;
	
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->p3DText->isCreated[id]) return 0;

	const char *szText = (pNetGame->pPlayerPool->pPlayer[playerid]->p3DText->TextLabels[id].szText) ? pNetGame->pPlayerPool->pPlayer[playerid]->p3DText->TextLabels[id].szText : '\0';
	return set_amxstring(amx, params[3], szText, params[4]);
}

// native GetPlayer3DTextLabelColor(playerid, PlayerText3D:id);
AMX_DECLARE_NATIVE(Natives::GetPlayer3DTextLabelColor)
{
	CHECK_PARAMS(2, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int id = CScriptParams::Get()->ReadInt();

	if(!IsPlayerConnected(playerid)) return 0;
	if(id < 0 || id >= MAX_3DTEXT_PLAYER) return 0;
	
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->p3DText->isCreated[id]) return 0;

	C3DText p3DText = pNetGame->pPlayerPool->pPlayer[playerid]->p3DText->TextLabels[id];
	return p3DText.dwColor;
}

// native GetPlayer3DTextLabelPos(playerid, PlayerText3D:id, &Float:fX, &Float:fY, &Float:fZ);
AMX_DECLARE_NATIVE(Natives::GetPlayer3DTextLabelPos)
{
	CHECK_PARAMS(5, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int id = CScriptParams::Get()->ReadInt();

	if(!IsPlayerConnected(playerid)) return 0;
	if(id < 0 || id >= MAX_3DTEXT_PLAYER) return 0;

	if(!pNetGame->pPlayerPool->pPlayer[playerid]->p3DText->isCreated[id]) return 0;
	C3DText p3DText = pNetGame->pPlayerPool->pPlayer[playerid]->p3DText->TextLabels[id];

	CScriptParams::Get()->Add(p3DText.vecPos);
	return 1;
}

// native Float:GetPlayer3DTextLabelDrawDist(playerid, PlayerText3D:id);
AMX_DECLARE_NATIVE(Natives::GetPlayer3DTextLabelDrawDist)
{
	CHECK_PARAMS(2, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int id = CScriptParams::Get()->ReadInt();

	if(!IsPlayerConnected(playerid)) return 0;
	if(id < 0 || id >= MAX_3DTEXT_PLAYER) return 0;
	
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->p3DText->isCreated[id]) return 0;

	C3DText p3DText = pNetGame->pPlayerPool->pPlayer[playerid]->p3DText->TextLabels[id];
	return amx_ftoc(p3DText.fDrawDistance);
}

// native GetPlayer3DTextLabelLOS(playerid, PlayerText3D:id);
AMX_DECLARE_NATIVE(Natives::GetPlayer3DTextLabelLOS)
{
	CHECK_PARAMS(2, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int id = CScriptParams::Get()->ReadInt();

	if(!IsPlayerConnected(playerid)) return 0;
	if(id < 0 || id >= MAX_3DTEXT_PLAYER) return 0;
	
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->p3DText->isCreated[id]) return 0;

	C3DText p3DText = pNetGame->pPlayerPool->pPlayer[playerid]->p3DText->TextLabels[id];
	return p3DText.bLineOfSight;
}

// native GetPlayer3DTextLabelVirtualW(playerid, PlayerText3D:id);
AMX_DECLARE_NATIVE(Natives::GetPlayer3DTextLabelVirtualW)
{
	CHECK_PARAMS(2, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int id = CScriptParams::Get()->ReadInt();

	if(!IsPlayerConnected(playerid)) return 0;
	if(id < 0 || id >= MAX_3DTEXT_PLAYER) return 0;
	
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->p3DText->isCreated[id]) return 0;

	C3DText p3DText = pNetGame->pPlayerPool->pPlayer[playerid]->p3DText->TextLabels[id];
	return p3DText.iWorld;
}

// native GetPlayer3DTextLabelAttached(playerid, PlayerText3D:id, &attached_playerid, &attached_vehicleid);
AMX_DECLARE_NATIVE(Natives::GetPlayer3DTextLabelAttached)
{
	CHECK_PARAMS(4, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int id = CScriptParams::Get()->ReadInt();

	if(!IsPlayerConnected(playerid)) return 0;
	if(id < 0 || id >= MAX_3DTEXT_PLAYER) return 0;
	
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->p3DText->isCreated[id]) return 0;
	C3DText p3DText = pNetGame->pPlayerPool->pPlayer[playerid]->p3DText->TextLabels[id];
	
	CScriptParams::Get()->Add(p3DText.wAttachedToPlayerID, p3DText.wAttachedToVehicleID);
	return 1;
}

// native AttachObjectToPlayer(objectid, playerid, Float:OffsetX, Float:OffsetY, Float:OffsetZ, Float:rX, Float:rY, Float:rZ)
AMX_DECLARE_NATIVE(Natives::YSF_AttachObjectToPlayer)
{
	CHECK_PARAMS(8, LOADED);
	
	const int objectid = CScriptParams::Get()->ReadInt();
	if (objectid < 1 || objectid >= MAX_OBJECTS) return 0;

	const int playerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;

	CObject *pObject = pNetGame->pObjectPool->pObjects[objectid];
	if(!pObject) return 0;

	// FUCK SAMP -.- n_AttachObjectToPlayer always return 0
	pAttachObjectToPlayer(amx, params);

	// Store values which should be server purpose not mine
	CServer::Get()->COBJECT_AttachedObjectPlayer[objectid] = static_cast<WORD>(playerid);
	CScriptParams::Get()->Read(&pObject->vecAttachedOffset, &pObject->vecAttachedRotation);
	return 1;
}

// native AttachPlayerObjectToPlayer(objectplayer, objectid, attachplayer, Float:OffsetX, Float:OffsetY, Float:OffsetZ, Float:rX, Float:rY, Float:rZ, onlyaddtoinstance = 0)
AMX_DECLARE_NATIVE(Natives::YSF_AttachPlayerObjectToPlayer)
{
	CHECK_PARAMS(9, MORE_PARAMETER_ALLOWED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int objectid = CScriptParams::Get()->ReadInt();
	const int attachplayerid = CScriptParams::Get()->ReadInt();
	bool bOnlyAddToInstance;

	if(!IsPlayerConnected(playerid)) return 0;
	if(!IsPlayerConnected(attachplayerid)) return 0;

	if(objectid < 1 || objectid >= MAX_OBJECTS) return 0;
	if(!pNetGame->pObjectPool->bPlayerObjectSlotState[playerid][objectid]) return 0;
	
	// Find the space where to store data
	std::shared_ptr<CPlayerObjectAttachAddon> pAddon = pPlayerData[playerid]->GetObjectAddon(objectid);
	if (pAddon == NULL)
		return logprintf("AttachPlayerObjectToPlayer: ERROR!!!!"), 0;

	// Store data
	pAddon->wObjectID = static_cast<WORD>(objectid);
	pAddon->wAttachPlayerID = static_cast<WORD>(attachplayerid);
	pAddon->creation_timepoint = default_clock::now();

	// Read parameters into our map pointer
	CScriptParams::Get()->Read(&pAddon->vecOffset, &pAddon->vecRot, &bOnlyAddToInstance);

	// If it's allowed to create object immendiately or player attach this object to herself, then create it now
	if(!bOnlyAddToInstance || playerid == attachplayerid)
	{
		if(pNetGame->pPlayerPool->pPlayer[playerid]->byteStreamedIn[attachplayerid] || playerid == attachplayerid)
		{
			RakNet::BitStream bs;
			bs.Write((WORD)objectid); // wObjectID
			bs.Write((WORD)attachplayerid); // wPlayerid
			bs.Write((char*)&pAddon->vecOffset, sizeof(CVector));
			bs.Write((char*)&pAddon->vecRot, sizeof(CVector));
			pAddon->bCreated = true;
			pAddon->bAttached = true;
			CSAMPFunctions::RPC(&RPC_AttachObject, &bs, LOW_PRIORITY, RELIABLE_ORDERED, 0, CSAMPFunctions::GetPlayerIDFromIndex(playerid), 0, 0);
		}
	}
	else
	{
		// We'll attach it later to prevent crashes
		if (pPlayerData[playerid]->m_PlayerObjectsAttachQueue.find(objectid) != pPlayerData[playerid]->m_PlayerObjectsAttachQueue.end())
			pPlayerData[playerid]->m_PlayerObjectsAttachQueue.erase(objectid);

		// This case GOTTA called only from streamer when object ALREADY created.
		pPlayerData[playerid]->m_PlayerObjectsAttachQueue.insert(objectid);
		pAddon->bCreated = true;
	}
	return 1;
}

// native AttachPlayerObjectToObject(playerid, objectid, attachtoid, Float:OffsetX, Float:OffsetY, Float:OffsetZ, Float:RotX, Float:RotY, Float:RotZ, SyncRotation = 1);
AMX_DECLARE_NATIVE(Natives::AttachPlayerObjectToObject)
{
	CHECK_PARAMS(10, LOADED);

	const int forplayerid = CScriptParams::Get()->ReadInt();
	const int objectid = CScriptParams::Get()->ReadInt();
	const int attachtoid = CScriptParams::Get()->ReadInt();

	if(!IsPlayerConnected(forplayerid)) return 0;

	if(objectid < 1 || objectid >= MAX_OBJECTS) return 0;
	if(attachtoid < 1 || attachtoid >= MAX_OBJECTS) return 0;

	CObjectPool *pObjectPool = pNetGame->pObjectPool;
	if(!pObjectPool->pPlayerObjects[forplayerid][objectid] || !pObjectPool->pPlayerObjects[forplayerid][attachtoid]) return 0; // Check if object is exist

	// Find the space where to store data
	std::shared_ptr<CPlayerObjectAttachAddon> pAddon = pPlayerData[forplayerid]->GetObjectAddon(objectid);
	if (pAddon == NULL)
		return logprintf("AttachPlayerObjectToPlayer: ERROR!!!!"), 0;

	// Geting data
	BYTE byteSyncRot;
	CScriptParams::Get()->Read(&pAddon->vecOffset, &pAddon->vecRot, &byteSyncRot);
	
	// Storing data
	pAddon->wObjectID = static_cast<WORD>(attachtoid);
	pAddon->creation_timepoint = default_clock::now();

	// Attach it
	int &iModelID = pObjectPool->pPlayerObjects[forplayerid][objectid]->iModel;
	CVector &vecPos = pObjectPool->pPlayerObjects[forplayerid][objectid]->matWorld.pos;
	CVector &vecRot = pObjectPool->pPlayerObjects[forplayerid][objectid]->vecRot;
	float fDrawDistance = 299.0;
	BYTE byteNoCameraCol = pObjectPool->pPlayerObjects[forplayerid][objectid]->bNoCameraCol;

	RakNet::BitStream bs;
	bs.Write((WORD)objectid);
	bs.Write(iModelID);
	bs.Write(vecPos);
	bs.Write(vecRot);
	bs.Write(fDrawDistance); // 159
	bs.Write(byteNoCameraCol);
	bs.Write((WORD)-1); // attached vehicle
	bs.Write((WORD)attachtoid); // attached object
	bs.Write(pAddon->vecOffset);
	bs.Write(pAddon->vecRot);
	bs.Write(byteSyncRot);
	
	CSAMPFunctions::RPC(&RPC_CreateObject, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, CSAMPFunctions::GetPlayerIDFromIndex(forplayerid), 0, 0); // Send this on same RPC as CreateObject
	return 1;
}

// native SetExclusiveBroadcast(toggle);
AMX_DECLARE_NATIVE(Natives::SetExclusiveBroadcast)
{
	CHECK_PARAMS(1, LOADED);

	const int toggle = CScriptParams::Get()->ReadInt();
	CServer::Get()->SetExclusiveBroadcast(!!toggle);
	return 1;
}

// native BroadcastToPlayer(playerid, toggle=1);
AMX_DECLARE_NATIVE(Natives::BroadcastToPlayer)
{
	CHECK_PARAMS(2, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int toggle = CScriptParams::Get()->ReadInt();
	
	if (!IsPlayerConnected(playerid)) return 0;

	pPlayerData[playerid]->bBroadcastTo = !!toggle;
	return 1;
}

// native SetRecordingDirectory(const dir[]);
AMX_DECLARE_NATIVE(Natives::SetRecordingDirectory)
{
	CHECK_PARAMS(1, LOADED);

	std::string dir;
	CScriptParams::Get()->Read(&dir);
	if (!CAddress::ADDR_RecordingDirectory) return 0;

	strcpy(gRecordingDataPath, dir.c_str());
	strcat(gRecordingDataPath, "/%s.rec");
	return 1;
}

// native GetRecordingDirectory(dir[], len = sizeof(dir));
AMX_DECLARE_NATIVE(Natives::GetRecordingDirectory)
{
	CHECK_PARAMS(2, LOADED);
	
	if(!CAddress::ADDR_RecordingDirectory) return 0;

	char temp[MAX_PATH];
	const size_t len = strlen(gRecordingDataPath);
	strcpy(temp, gRecordingDataPath);
	temp[len - 7] = 0;

	CScriptParams::Get()->Add(&temp[0]);
	return 1;
}

// native SendClientMessagef(playerid, color, const message[], {Float,_}:...);
AMX_DECLARE_NATIVE(Natives::SendClientMessagef)
{
	if (!CServer::Get()->IsInitialized()) return std::numeric_limits<int>::lowest(); // If unknown server version

	const int playerid = static_cast<int>(params[1]);
	if(!IsPlayerConnected(playerid)) return 0;

	int len;
	char* szMessage = CSAMPFunctions::format_amxstring(amx, params, 3, len);
	if(!szMessage) return 0;

	RakNet::BitStream bsParams;
	bsParams.Write((DWORD)params[2]);
	bsParams.Write((DWORD)len);
	bsParams.Write(szMessage, len);
	CSAMPFunctions::RPC(&RPC_ClientMessage, &bsParams, HIGH_PRIORITY, RELIABLE_ORDERED, 0, CSAMPFunctions::GetPlayerIDFromIndex(playerid), false, false);
	return 1;
}

// native SendClientMessageToAllf(color, const message[], {Float,_}:...);
AMX_DECLARE_NATIVE(Natives::SendClientMessageToAllf)
{
	if (!CServer::Get()->IsInitialized()) return std::numeric_limits<int>::lowest(); // If unknown server version

	int len;
	char* szMessage = CSAMPFunctions::format_amxstring(amx, params, 2, len);
	if(!szMessage) return 0;

	RakNet::BitStream bsParams;
	bsParams.Write((DWORD)params[1]);
	bsParams.Write((DWORD)len);
	bsParams.Write(szMessage, len);
	CSAMPFunctions::RPC(&RPC_ClientMessage, &bsParams, HIGH_PRIORITY, RELIABLE_ORDERED, 0, UNASSIGNED_PLAYER_ID, true, false);
	return 1;
}

// native GameTextForPlayerf(playerid, displaytime, style, const message[], {Float,_}:...);
AMX_DECLARE_NATIVE(Natives::GameTextForPlayerf)
{
	if (!CServer::Get()->IsInitialized()) return std::numeric_limits<int>::lowest(); // If unknown server version

	const int playerid = static_cast<int>(params[1]);
	if(!IsPlayerConnected(playerid)) return 0;

	int len;
	char* szMessage = CSAMPFunctions::format_amxstring(amx, params, 4, len);
	if(!szMessage) return 0;

	RakNet::BitStream bsParams;
	bsParams.Write((int)params[3]);
	bsParams.Write((int)params[2]);
	bsParams.Write(len);
	bsParams.Write(szMessage, len);
	CSAMPFunctions::RPC(&RPC_ScrDisplayGameText, &bsParams, HIGH_PRIORITY, RELIABLE_ORDERED, 0, CSAMPFunctions::GetPlayerIDFromIndex(playerid), false, false);
	return 1;
}

// native GameTextForAllf(displaytime, style, const message[], {Float,_}:...);
AMX_DECLARE_NATIVE(Natives::GameTextForAllf)
{
	if (!CServer::Get()->IsInitialized()) return std::numeric_limits<int>::lowest(); // If unknown server version

	int len;
	char* szMessage = CSAMPFunctions::format_amxstring(amx, params, 3, len);
	if(!szMessage) return 0;

	RakNet::BitStream bsParams;
	bsParams.Write((int)params[2]);
	bsParams.Write((int)params[1]);
	bsParams.Write(len);
	bsParams.Write(szMessage, len);
	CSAMPFunctions::RPC(&RPC_ScrDisplayGameText, &bsParams, HIGH_PRIORITY, RELIABLE_ORDERED, 0, UNASSIGNED_PLAYER_ID, true, false);
	return 1;
}

// native SendPlayerMessageToPlayerf(playerid, senderid, const message[], {Float,_}:...);
AMX_DECLARE_NATIVE(Natives::SendPlayerMessageToPlayerf)
{
	if (!CServer::Get()->IsInitialized()) return std::numeric_limits<int>::lowest(); // If unknown server version

	const int playerid = static_cast<int>(params[1]);
	if(!IsPlayerConnected(playerid)) return 0;

	int senderid = static_cast<int>(params[2]);
	if(!IsPlayerConnected(senderid)) return 0;

	int len;
	char* szMessage = CSAMPFunctions::format_amxstring(amx, params, 3, len);
	if(!szMessage) return 0;

	RakNet::BitStream bsParams;
	bsParams.Write((WORD)senderid);
	bsParams.Write((BYTE)len);
	bsParams.Write(szMessage, len);
	CSAMPFunctions::RPC(&RPC_Chat, &bsParams, HIGH_PRIORITY, RELIABLE_ORDERED, 0, CSAMPFunctions::GetPlayerIDFromIndex(playerid), false, false);
	return 1;
}

// native SendPlayerMessageToAllf(senderid, const message[], {Float,_}:...);
AMX_DECLARE_NATIVE(Natives::SendPlayerMessageToAllf)
{
	if (!CServer::Get()->IsInitialized()) return std::numeric_limits<int>::lowest(); // If unknown server version

	int senderid = static_cast<int>(params[1]);
	if(!IsPlayerConnected(senderid)) return 0;

	int len;
	char* szMessage = CSAMPFunctions::format_amxstring(amx, params, 2, len);
	if(!szMessage) return 0;

	RakNet::BitStream bsParams;
	bsParams.Write((WORD)senderid);
	bsParams.Write((BYTE)len);
	bsParams.Write(szMessage, len);
	CSAMPFunctions::RPC(&RPC_Chat, &bsParams, HIGH_PRIORITY, RELIABLE_ORDERED, 0, UNASSIGNED_PLAYER_ID, true, false);
	return 1;
}

// native SendRconCommandf(command[], {Float,_}:...);
AMX_DECLARE_NATIVE(Natives::SendRconCommandf)
{
	if (!CServer::Get()->IsInitialized()) return std::numeric_limits<int>::lowest(); // If unknown server version

	int len;
	char* szMessage = CSAMPFunctions::format_amxstring(amx, params, 1, len);
	if(!szMessage) return 0;

	CSAMPFunctions::Execute(szMessage);
	return 1;
}

// native YSF_SetTickRate(ticks);
AMX_DECLARE_NATIVE(Natives::YSF_SetTickRate)
{
	CHECK_PARAMS(1, LOADED);

	int rate = CScriptParams::Get()->ReadInt();
	if(rate < -1 || rate == 0) return 0; // -1 = no update

	CServer::Get()->SetTickRate(rate);
	return 1;
}

// native YSF_GetTickRate();
AMX_DECLARE_NATIVE(Natives::YSF_GetTickRate)
{
	if (!CServer::Get()->IsInitialized()) return std::numeric_limits<int>::lowest(); // If unknown server version

	return static_cast<cell>(CServer::Get()->GetTickRate());
}

// native YSF_EnableNightVisionFix(enable);
AMX_DECLARE_NATIVE(Natives::YSF_EnableNightVisionFix)
{
	CHECK_PARAMS(1, LOADED);

	CServer::Get()->EnableNightVisionFix(CScriptParams::Get()->ReadBool());
	return 1;
}

// native YSF_IsNightVisionFixEnabled();
AMX_DECLARE_NATIVE(Natives::YSF_IsNightVisionFixEnabled)
{
	if (!CServer::Get()->IsInitialized()) return std::numeric_limits<int>::lowest(); // If unknown server version

	return static_cast<cell>(CServer::Get()->IsNightVisionFixEnabled());
}

// native YSF_ToggleOnServerMessage(toggle);
AMX_DECLARE_NATIVE(Natives::YSF_ToggleOnServerMessage)
{
	CHECK_PARAMS(1, LOADED);

	CServer::Get()->ToggleOnServerMessage(CScriptParams::Get()->ReadBool());
	return 1;
}

// native YSF_IsOnServerMessageEnabled();
AMX_DECLARE_NATIVE(Natives::YSF_IsOnServerMessageEnabled)
{
	if (!CServer::Get()->IsInitialized()) return std::numeric_limits<int>::lowest(); // If unknown server version

	return static_cast<cell>(CServer::Get()->IsOnServerMessageEnabled());
}

// native YSF_SetExtendedNetStatsEnabled(enable);
AMX_DECLARE_NATIVE(Natives::YSF_SetExtendedNetStatsEnabled)
{
	CHECK_PARAMS(1, LOADED);

	CServer::Get()->SetExtendedNetStatsEnabled(CScriptParams::Get()->ReadBool());
	return 1;
}

// native YSF_IsExtendedNetStatsEnabled();
AMX_DECLARE_NATIVE(Natives::YSF_IsExtendedNetStatsEnabled)
{
	if (!CServer::Get()->IsInitialized()) return std::numeric_limits<int>::lowest(); // If unknown server version

	return static_cast<cell>(CServer::Get()->IsExtendedNetStatsEnabled());
}

// native YSF_SetAFKAccuracy(time_ms);
AMX_DECLARE_NATIVE(Natives::YSF_SetAFKAccuracy)
{
	CHECK_PARAMS(1, LOADED);

	CServer::Get()->SetAFKAccuracy(static_cast<DWORD>(CScriptParams::Get()->ReadInt()));
	return 1;
}

// native YSF_GetAFKAccuracy();
AMX_DECLARE_NATIVE(Natives::YSF_GetAFKAccuracy)
{
	if (!CServer::Get()->IsInitialized()) return std::numeric_limits<int>::lowest(); // If unknown server version

	return static_cast<cell>(CServer::Get()->GetAFKAccuracy());
}

AMX_DECLARE_NATIVE(Natives::YSF_GangZoneCreate)
{
	CHECK_PARAMS(4, LOADED);

	float fMinX, fMinY, fMaxX, fMaxY;
	CScriptParams::Get()->Read(&fMinX, &fMinY, &fMaxX, &fMaxY);

	// If coordinates are wrong, then won't create bugged zone!
	if(fMaxX <= fMinX || fMaxY <= fMinY) 
	{
		//logprintf("GangZoneCreate: MaxX, MaxY must be bigger than MinX, MinY. Not inversely!");
		//logprintf("GangZoneCreate: %f, %f, %f, %f",fMinX, fMinY, fMaxX, fMaxY);
		return -1;
	}

	WORD ret = CServer::Get()->pGangZonePool->New(fMinX, fMinY, fMaxX, fMaxY);

	return (ret == 0xFFFF) ? -1 : ret;
}

AMX_DECLARE_NATIVE(Natives::YSF_GangZoneDestroy)
{
	CHECK_PARAMS(1, LOADED);

	CGangZonePool *pGangZonePool = CServer::Get()->pGangZonePool;
	const int zoneid = CScriptParams::Get()->ReadInt();
	if(!pGangZonePool || !pGangZonePool->GetSlotState(static_cast<WORD>(zoneid))) return 0;

	pGangZonePool->Delete(static_cast<WORD>(zoneid));
	return 1;
}

// native YSF_GangZoneShowForPlayer(playerid, zone, color);
AMX_DECLARE_NATIVE(Natives::YSF_GangZoneShowForPlayer)
{
	CHECK_PARAMS(3, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int zoneid = CScriptParams::Get()->ReadInt();

	// For security
	if(!IsPlayerConnected(playerid)) return 0;
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;

	return CServer::Get()->pGangZonePool->ShowForPlayer(static_cast<WORD>(playerid), static_cast<WORD>(zoneid), static_cast<DWORD>(CScriptParams::Get()->ReadInt()));
}

// native YSF_GangZoneHideForPlayer(playerid, zone);
AMX_DECLARE_NATIVE(Natives::YSF_GangZoneHideForPlayer)
{
	CHECK_PARAMS(2, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int zoneid = CScriptParams::Get()->ReadInt();

	// For security
	if(!IsPlayerConnected(playerid)) return 0;
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;

	CServer::Get()->pGangZonePool->HideForPlayer(static_cast<WORD>(playerid), static_cast<WORD>(zoneid), false, true);
	return 1;
}

// native YSF_GangZoneShowForAll(zone, color);
AMX_DECLARE_NATIVE(Natives::YSF_GangZoneShowForAll)
{
	CHECK_PARAMS(2, LOADED);

	const int zoneid = CScriptParams::Get()->ReadInt();
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;

	CServer::Get()->pGangZonePool->ShowForAll(static_cast<WORD>(zoneid), static_cast<DWORD>(params[2]));
	return 1;
}

// native YSF_GangZoneHideForAll(zone);
AMX_DECLARE_NATIVE(Natives::YSF_GangZoneHideForAll)
{
	CHECK_PARAMS(1, LOADED);

	const int zoneid = CScriptParams::Get()->ReadInt();
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;

	CServer::Get()->pGangZonePool->HideForAll(static_cast<WORD>(zoneid));
	return 1;
}

AMX_DECLARE_NATIVE(Natives::YSF_GangZoneFlashForPlayer)
{
	CHECK_PARAMS(3, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int zoneid = CScriptParams::Get()->ReadInt();

	// For security
	if(!IsPlayerConnected(playerid)) return 0;
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;

	CServer::Get()->pGangZonePool->FlashForPlayer(static_cast<WORD>(playerid), static_cast<WORD>(zoneid), static_cast<DWORD>(CScriptParams::Get()->ReadInt()));
	return 1;
}

AMX_DECLARE_NATIVE(Natives::YSF_GangZoneFlashForAll)
{
	CHECK_PARAMS(2, LOADED);

	const int zoneid = CScriptParams::Get()->ReadInt();
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;

	CServer::Get()->pGangZonePool->FlashForAll(static_cast<WORD>(zoneid), static_cast<DWORD>(params[2]));
	return 1;
}

AMX_DECLARE_NATIVE(Natives::YSF_GangZoneStopFlashForPlayer)
{
	CHECK_PARAMS(2, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int zoneid = CScriptParams::Get()->ReadInt();

	// For security
	if(!IsPlayerConnected(playerid)) return 0;
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;

	CServer::Get()->pGangZonePool->StopFlashForPlayer(static_cast<WORD>(playerid), static_cast<WORD>(zoneid));
	return 1;
}

AMX_DECLARE_NATIVE(Natives::YSF_GangZoneStopFlashForAll)
{
	CHECK_PARAMS(1, LOADED);

	const int zoneid = CScriptParams::Get()->ReadInt();
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;

	CServer::Get()->pGangZonePool->StopFlashForAll(static_cast<WORD>(zoneid));
	return 1;
}

// Menu functions
// native IsMenuDisabled(Menu:menuid);
AMX_DECLARE_NATIVE(Natives::IsMenuDisabled)
{
	CHECK_PARAMS(1, LOADED);
	
	const int menuid = CScriptParams::Get()->ReadInt();
	if(menuid < 1 || menuid >= MAX_MENUS) return 0;
	
	if(!pNetGame->pMenuPool->bIsCreated[menuid]) return 0;
	CMenu *pMenu = pNetGame->pMenuPool->pMenu[menuid];

	return !!(!pMenu->interaction.Menu);
}

// native IsMenuRowDisabled(Menu:menuid, row);
AMX_DECLARE_NATIVE(Natives::IsMenuRowDisabled)
{
	CHECK_PARAMS(2, LOADED);
	
	const int menuid = CScriptParams::Get()->ReadInt();
	if(menuid < 1 || menuid >= MAX_MENUS) return 0;
	
	const int itemid = CScriptParams::Get()->ReadInt();
	if(itemid < 0 || itemid >= 12) return 0;

	if(!pNetGame->pMenuPool->bIsCreated[menuid]) return 0;
	CMenu *pMenu = pNetGame->pMenuPool->pMenu[menuid];

	return !!(!pMenu->interaction.Row[itemid]);
}

// native GetMenuColumns(menuid);
AMX_DECLARE_NATIVE(Natives::GetMenuColumns)
{
	CHECK_PARAMS(1, LOADED);
	
	const int menuid = CScriptParams::Get()->ReadInt();
	if(menuid < 1 || menuid >= MAX_MENUS) return 0;
	
	if(!pNetGame->pMenuPool->bIsCreated[menuid]) return 0;
	CMenu *pMenu = pNetGame->pMenuPool->pMenu[menuid];

	return pMenu->byteColumnsNumber;
}

// native GetMenuItems(menuid, column);
AMX_DECLARE_NATIVE(Natives::GetMenuItems)
{
	CHECK_PARAMS(2, LOADED);
	
	const int menuid = CScriptParams::Get()->ReadInt();
	if(menuid < 1 || menuid >= MAX_MENUS) return 0;

	const int column = CScriptParams::Get()->ReadInt();
	if(menuid < 0 || menuid > 2) return 0;

	if(!pNetGame->pMenuPool->bIsCreated[menuid]) return 0;
	CMenu *pMenu = pNetGame->pMenuPool->pMenu[menuid];

	return pMenu->byteItemsCount[column];
}

// native GetMenuPos(menuid, &Float:fX, &Float:fY);
AMX_DECLARE_NATIVE(Natives::GetMenuPos)
{
	CHECK_PARAMS(3, LOADED);
	
	const int menuid = CScriptParams::Get()->ReadInt();
	if(menuid < 1 || menuid >= MAX_MENUS) return 0;

	if(!pNetGame->pMenuPool->bIsCreated[menuid]) return 0;
	CMenu *pMenu = pNetGame->pMenuPool->pMenu[menuid];

	CScriptParams::Get()->Add(pMenu->vecPos);
	return 1;
}

// native GetMenuColumnWidth(menuid, &Float:fColumn1, &Float:fColumn2);
AMX_DECLARE_NATIVE(Natives::GetMenuColumnWidth)
{
	CHECK_PARAMS(4, LOADED);
	
	const int menuid = CScriptParams::Get()->ReadInt();
	if(menuid < 1 || menuid >= MAX_MENUS) return 0;

	if(!pNetGame->pMenuPool->bIsCreated[menuid]) return 0;
	CMenu *pMenu = pNetGame->pMenuPool->pMenu[menuid];

	CScriptParams::Get()->Add(pMenu->fColumn1Width, pMenu->fColumn2Width);
	return 1;
}

// native GetMenuColumnHeader(menuid, column, header[], len = sizeof(header));
AMX_DECLARE_NATIVE(Natives::GetMenuColumnHeader)
{
	CHECK_PARAMS(4, LOADED);
	
	const int menuid = CScriptParams::Get()->ReadInt();
	if(menuid < 1 || menuid >= MAX_MENUS) return 0;

	const int column = CScriptParams::Get()->ReadInt();
	if(menuid < 0 || menuid > 2) return 0;

	if(!pNetGame->pMenuPool->bIsCreated[menuid]) return 0;
	CMenu *pMenu = pNetGame->pMenuPool->pMenu[menuid];

	CScriptParams::Get()->Add(&pMenu->szHeaders[column][0]);
	return 1;
}

// native GetMenuItem(menuid, column, itemid, item[], len = sizeof(item));
AMX_DECLARE_NATIVE(Natives::GetMenuItem)
{
	CHECK_PARAMS(5,  LOADED);
	
	const int menuid = CScriptParams::Get()->ReadInt();
	if(menuid < 1 || menuid >= MAX_MENUS) return 0;

	const int column = CScriptParams::Get()->ReadInt();
	if(menuid < 0 || menuid > 2) return 0;

	const int itemid = CScriptParams::Get()->ReadInt();
	if(itemid < 0 || itemid >= 12) return 0;

	if(!pNetGame->pMenuPool->bIsCreated[menuid]) return 0;
	CMenu *pMenu = pNetGame->pMenuPool->pMenu[menuid];
	
	CScriptParams::Get()->Add(&pMenu->szItems[itemid][column][0]);
	return 1;
}

// native CreatePlayerGangZone(playerid, Float:minx, Float:miny, Float:maxx, Float:maxy);
AMX_DECLARE_NATIVE(Natives::CreatePlayerGangZone)
{
	CHECK_PARAMS(5, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;

	float fMinX, fMinY, fMaxX, fMaxY;
	CScriptParams::Get()->Read(&fMinX, &fMinY, &fMaxX, &fMaxY);
	 
	// If coordinates are wrong, then won't create bugged zone!
	if(fMaxX <= fMinX || fMaxY <= fMinY) 
	{
		logprintf("CreatePlayerGangZone: MaxX, MaxY must be bigger than MinX, MinY. Not inversely!");
		logprintf("CreatePlayerGangZone: %f, %f, %f, %f", fMinX, fMinY, fMaxX, fMaxY);
		return -1;
	}

	WORD ret = CServer::Get()->pGangZonePool->New(static_cast<WORD>(playerid), fMinX, fMinY, fMaxX, fMaxY);
	if (ret == 0xFFFF) return -1;

	return ret;
}

// native PlayerGangZoneShow(playerid, zoneid, color);
AMX_DECLARE_NATIVE(Natives::PlayerGangZoneShow)
{
	CHECK_PARAMS(3, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int zoneid = CScriptParams::Get()->ReadInt();
	int color = CScriptParams::Get()->ReadInt();

	if(!IsPlayerConnected(playerid)) return 0;
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;

	if(!pPlayerData[playerid]->pPlayerZone[zoneid]) return 0;

	CServer::Get()->pGangZonePool->ShowForPlayer(static_cast<WORD>(playerid), static_cast<WORD>(zoneid), static_cast<DWORD>(color), true);
	return 1;
}

// native PlayerGangZoneHide(playerid, zoneid);
AMX_DECLARE_NATIVE(Natives::PlayerGangZoneHide)
{
	CHECK_PARAMS(2, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int zoneid = CScriptParams::Get()->ReadInt();

	if(!IsPlayerConnected(playerid)) return 0;
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;

	if(!pPlayerData[playerid]->pPlayerZone[zoneid]) return 0;

	CServer::Get()->pGangZonePool->HideForPlayer(static_cast<WORD>(playerid), static_cast<WORD>(zoneid), true);
	return 1;
}

// native PlayerGangZoneFlash(playerid, zoneid, color);
AMX_DECLARE_NATIVE(Natives::PlayerGangZoneFlash)
{
	CHECK_PARAMS(3, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int zoneid = CScriptParams::Get()->ReadInt();
	int color = CScriptParams::Get()->ReadInt();
	
	if(!IsPlayerConnected(playerid)) return 0;
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;
	if(!pPlayerData[playerid]->pPlayerZone[zoneid]) return 0;

	CServer::Get()->pGangZonePool->FlashForPlayer(static_cast<WORD>(playerid), static_cast<WORD>(zoneid), static_cast<DWORD>(color), true);
	return 1;
}

// native PlayerGangZoneStopFlash(playerid, zoneid);
AMX_DECLARE_NATIVE(Natives::PlayerGangZoneStopFlash)
{
	CHECK_PARAMS(2, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int zoneid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;

	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;

	if(!pPlayerData[playerid]->pPlayerZone[zoneid]) return 0;

	CServer::Get()->pGangZonePool->StopFlashForPlayer(static_cast<WORD>(playerid), static_cast<WORD>(zoneid), true);
	return 1;
}

// native PlayerGangZoneDestroy(playerid, zoneid);
AMX_DECLARE_NATIVE(Natives::PlayerGangZoneDestroy)
{
	CHECK_PARAMS(2, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int zoneid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;

	CServer::Get()->pGangZonePool->HideForPlayer(static_cast<WORD>(playerid), static_cast<WORD>(zoneid), true);
	return 1;
}

// native IsValidPlayerGangZone(playerid, zoneid);
AMX_DECLARE_NATIVE(Natives::IsValidPlayerGangZone)
{
	CHECK_PARAMS(2, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int zoneid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;
	
	return pPlayerData[playerid]->pPlayerZone[zoneid] != NULL;
}

// native IsPlayerInPlayerGangZone(playerid, zoneid);
AMX_DECLARE_NATIVE(Natives::IsPlayerInPlayerGangZone)
{
	CHECK_PARAMS(2, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int zoneid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;
	
	if(!pPlayerData[playerid]->pPlayerZone[zoneid]) return 0;

	WORD id = pPlayerData[playerid]->GetGangZoneIDFromClientSide(static_cast<WORD>(zoneid), true);
	if(id != 0xFFFF) 
	{
		return pPlayerData[playerid]->bInGangZone[id];
	}
	return 0;
}

// native PlayerGangZoneGetPos(playerid, zoneid, &Float:fMinX, &Float:fMinY, &Float:fMaxX, &Float:fMaxY);
AMX_DECLARE_NATIVE(Natives::PlayerGangZoneGetPos)
{
	CHECK_PARAMS(6, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;

	const int zoneid = CScriptParams::Get()->ReadInt();
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;
	
	if(!pPlayerData[playerid]->pPlayerZone[zoneid]) return 0;
	
	WORD id = pPlayerData[playerid]->GetGangZoneIDFromClientSide(static_cast<WORD>(zoneid), true);
	if(id != 0xFFFF) 
	{
		CGangZone *pGangZone = pPlayerData[playerid]->pPlayerZone[zoneid];
		CScriptParams::Get()->Add(pGangZone->fGangZone[0], pGangZone->fGangZone[1], pGangZone->fGangZone[2], pGangZone->fGangZone[3]);
		return 1;
	}
	return 0;
}

// native IsPlayerGangZoneVisible(playerid, zoneid);
AMX_DECLARE_NATIVE(Natives::IsPlayerGangZoneVisible)
{
	CHECK_PARAMS(2, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int zoneid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;
	
	if(!pPlayerData[playerid]->pPlayerZone[zoneid]) return 0;

	return pPlayerData[playerid]->GetGangZoneIDFromClientSide(static_cast<WORD>(zoneid), true) != 0xFFFF;
}

// native PlayerGangZoneGetColor(playerid, zoneid);
AMX_DECLARE_NATIVE(Natives::PlayerGangZoneGetColor)
{
	CHECK_PARAMS(2, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int zoneid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;
	
	if(!pPlayerData[playerid]->pPlayerZone[zoneid]) return 0;

	WORD id = pPlayerData[playerid]->GetGangZoneIDFromClientSide(static_cast<WORD>(zoneid), true);
	if(id != 0xFFFF) 
	{
		return pPlayerData[playerid]->dwClientSideZoneColor[id];
	}
	return 0;
}

// native PlayerGangZoneGetFlashColor(playerid, zoneid);
AMX_DECLARE_NATIVE(Natives::PlayerGangZoneGetFlashColor)
{
	CHECK_PARAMS(2, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int zoneid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;
	
	if(!pPlayerData[playerid]->pPlayerZone[zoneid]) return 0;

	WORD id = pPlayerData[playerid]->GetGangZoneIDFromClientSide(static_cast<WORD>(zoneid), true);
	if(id != 0xFFFF) 
	{
		return pPlayerData[playerid]->dwClientSideZoneFlashColor[id];
	}
	return 0;
}

// native IsPlayerGangZoneFlashing(playerid, zoneid);
AMX_DECLARE_NATIVE(Natives::IsPlayerGangZoneFlashing)
{
	CHECK_PARAMS(2, LOADED);
	
	const int playerid = CScriptParams::Get()->ReadInt();
	const int zoneid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;
	
	if(!pPlayerData[playerid]->pPlayerZone[zoneid]) return 0;

	WORD id = pPlayerData[playerid]->GetGangZoneIDFromClientSide(static_cast<WORD>(zoneid), true);
	if(id != 0xFFFF) 
	{
		return pPlayerData[playerid]->bIsGangZoneFlashing[id];
	}
	return 0;
}

#ifdef NEW_PICKUP_SYSTEM

// native IsValidPickup(pickupid);
AMX_DECLARE_NATIVE(Natives::IsValidPickup)
{
	CHECK_PARAMS(1, LOADED);

	const int id = CScriptParams::Get()->ReadInt();
	if(id < 0 || id >= MAX_PICKUPS) return 0;

	return CServer::Get()->pPickupPool->FindPickup(id) != 0;
}

// native IsPickupStreamedIn(playerid, pickupid);
AMX_DECLARE_NATIVE(Natives::IsPickupStreamedIn)
{
	CHECK_PARAMS(2, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int id = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;
	if(id < 0 || id >= MAX_PICKUPS) return 0;

	CPickup *pPickup = CServer::Get()->pPickupPool->FindPickup(id);
	if(!pPickup) return 0;

	return CServer::Get()->pPickupPool->IsStreamed(playerid, pPickup);
}

// native GetPickupPos(pickupid, &Float:fX, &Float:fY, &Float:fZ);
AMX_DECLARE_NATIVE(Natives::GetPickupPos)
{
	CHECK_PARAMS(4, LOADED);

	const int id = CScriptParams::Get()->ReadInt();
	if(id < 0 || id >= MAX_PICKUPS) return 0;

	CPickup *pPickup = CServer::Get()->pPickupPool->FindPickup(id);
	if(!pPickup) return 0;

	CScriptParams::Get()->Add(pPickup->vecPos);
	return 1;
}

// native GetPickupModel(pickupid);
AMX_DECLARE_NATIVE(Natives::GetPickupModel)
{
	CHECK_PARAMS(1, LOADED);

	const int id = CScriptParams::Get()->ReadInt();
	if(id < 0 || id >= MAX_PICKUPS) return 0;

	CPickup *pPickup = CServer::Get()->pPickupPool->FindPickup(id);
	if(!pPickup) return 0;

	return pPickup->iModel;
}

// native GetPickupType(pickupid);
AMX_DECLARE_NATIVE(Natives::GetPickupType)
{
	CHECK_PARAMS(1, LOADED);

	const int id = CScriptParams::Get()->ReadInt();
	if(id < 0 || id >= MAX_PICKUPS) return 0;

	CPickup *pPickup = CServer::Get()->pPickupPool->FindPickup(id);
	if(!pPickup) return 0;

	return pPickup->iType;
}

// native GetPickupVirtualWorld(pickupid);
AMX_DECLARE_NATIVE(Natives::GetPickupVirtualWorld)
{
	CHECK_PARAMS(1, LOADED);

	const int id = CScriptParams::Get()->ReadInt();
	if(id < 0 || id >= MAX_PICKUPS) return 0;

	CPickup *pPickup = CServer::Get()->pPickupPool->FindPickup(id);
	if(!pPickup) return 0;

	return pPickup->iWorld;
}

// CreatePlayerPickup(playerid, model, type, Float:X, Float:Y, Float:Z, virtualworld = 0);
AMX_DECLARE_NATIVE(Natives::CreatePlayerPickup)
{
	CHECK_PARAMS(7, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;

	return CServer::Get()->pPickupPool->New(playerid, (int)params[2], (int)params[3], CVector(amx_ctof(params[4]), amx_ctof(params[5]), amx_ctof(params[6])), (int)params[7]);
}

// native DestroyPlayerPickup(playerid, pickupid);
AMX_DECLARE_NATIVE(Natives::DestroyPlayerPickup)
{
	CHECK_PARAMS(2, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int id = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;	
	if(id < 0 || id >= MAX_PICKUPS) return 0;
	
	CPickup *pPickup = CServer::Get()->pPickupPool->FindPickup(playerid, id);
	if(!pPickup) return 0;

	CServer::Get()->pPickupPool->Destroy((WORD)playerid, id);
	return 1;
}

// native IsValidPlayerPickup(playerid, pickupid);
AMX_DECLARE_NATIVE(Natives::IsValidPlayerPickup)
{
	CHECK_PARAMS(2, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int id = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;	
	if(id < 0 || id >= MAX_PICKUPS) return 0;

	return CServer::Get()->pPickupPool->FindPickup(playerid, id) != 0;
}

// native IsPlayerPickupStreamedIn(playerid, pickupid);
AMX_DECLARE_NATIVE(Natives::IsPlayerPickupStreamedIn)
{
	CHECK_PARAMS(2, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int id = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;	
	if(id < 0 || id >= MAX_PICKUPS) return 0;
	
	CPickup *pPickup = CServer::Get()->pPickupPool->FindPickup(playerid, id);
	if(!pPickup) return 0;

	return CServer::Get()->pPickupPool->IsStreamed(playerid, pPickup);
}

// native GetPlayerPickupPos(playerid, pickupid, &Float:fX, &Float:fY, &Float:fZ);
AMX_DECLARE_NATIVE(Natives::GetPlayerPickupPos)
{
	CHECK_PARAMS(5, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int id = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;	
	if(id < 0 || id >= MAX_PICKUPS) return 0;
	
	CPickup *pPickup = CServer::Get()->pPickupPool->FindPickup(playerid, id);
	if(!pPickup) return 0;

	CScriptParams::Get()->Add(pPickup->vecPos);
	return 1;
}

// native GetPlayerPickupModel(playerid, pickupid);
AMX_DECLARE_NATIVE(Natives::GetPlayerPickupModel)
{
	if (!CServer::Get()->IsInitialized()) return std::numeric_limits<int>::lowest(); // If unknown server version
	CHECK_PARAMS(2, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int id = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;	
	if(id < 0 || id >= MAX_PICKUPS) return 0;
	
	CPickup *pPickup = CServer::Get()->pPickupPool->FindPickup(playerid, id);
	if(!pPickup) return 0;

	return pPickup->iModel;
}

// native GetPlayerPickupType(playerid, pickupid);
AMX_DECLARE_NATIVE(Natives::GetPlayerPickupType)
{
	CHECK_PARAMS(2, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int id = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;	
	if(id < 0 || id >= MAX_PICKUPS) return 0;
	
	CPickup *pPickup = CServer::Get()->pPickupPool->FindPickup(playerid, id);
	if(!pPickup) return 0;

	return pPickup->iType;
}

// native GetPlayerPickupVirtualWorld(playerid, pickupid);
AMX_DECLARE_NATIVE(Natives::GetPlayerPickupVirtualWorld)
{
	CHECK_PARAMS(2, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int id = CScriptParams::Get()->ReadInt();
	if(!IsPlayerConnected(playerid)) return 0;	
	if(id < 0 || id >= MAX_PICKUPS) return 0;
	
	CPickup *pPickup = CServer::Get()->pPickupPool->FindPickup(playerid, id);
	if(!pPickup) return 0;

	return pPickup->iWorld;
}

#else

// native IsValidPickup(pickupid);
AMX_DECLARE_NATIVE(Natives::IsValidPickup)
{
	CHECK_PARAMS(1, LOADED);

	const int id = CScriptParams::Get()->ReadInt();
	if (id < 0 || id >= MAX_PICKUPS)
		return 0;

	return pNetGame->pPickupPool->bActive[id];
}

// native IsPickupStreamedIn(playerid, pickupid);
AMX_DECLARE_NATIVE(Natives::IsPickupStreamedIn)
{
	CHECK_PARAMS(2, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	const int pickupid = CScriptParams::Get()->ReadInt();
	if (!IsPlayerConnected(playerid)) return 0;
	if (pickupid < 0 || pickupid >= MAX_PICKUPS) return 0;

	return pNetGame->pPlayerPool->pPlayer[playerid]->bPickupStreamedIn[pickupid];
}

// native GetPickupPos(pickupid, &Float:fX, &Float:fY, &Float:fZ);
AMX_DECLARE_NATIVE(Natives::GetPickupPos)
{
	CHECK_PARAMS(4, LOADED);

	const int id = CScriptParams::Get()->ReadInt();
	if (id < 0 || id >= MAX_PICKUPS)
		return 0;

	if (!pNetGame->pPickupPool->bActive[id]) return 0;

	CScriptParams::Get()->Add(pNetGame->pPickupPool->Pickup[id].vecPos);
	return 1;
}

// native GetPickupModel(pickupid);
AMX_DECLARE_NATIVE(Natives::GetPickupModel)
{
	CHECK_PARAMS(1, LOADED);

	const int id = CScriptParams::Get()->ReadInt();
	if (id < 0 || id >= MAX_PICKUPS)
		return 0;

	if (!pNetGame->pPickupPool->bActive[id]) return 0;

	return pNetGame->pPickupPool->Pickup[id].iModel;
}

// native GetPickupType(pickupid);
AMX_DECLARE_NATIVE(Natives::GetPickupType)
{
	CHECK_PARAMS(1, LOADED);

	const int id = CScriptParams::Get()->ReadInt();
	if (id < 0 || id >= MAX_PICKUPS)
		return 0;

	if (!pNetGame->pPickupPool->bActive[id]) return 0;

	return pNetGame->pPickupPool->Pickup[id].iType;
}

// native GetPickupVirtualWorld(pickupid);
AMX_DECLARE_NATIVE(Natives::GetPickupVirtualWorld)
{
	CHECK_PARAMS(1, LOADED);

	const int id = CScriptParams::Get()->ReadInt();
	if (id < 0 || id >= MAX_PICKUPS)
		return 0;

	if (!pNetGame->pPickupPool->bActive[id]) return 0;

	return pNetGame->pPickupPool->iWorld[id];
}

#endif
// RakServer functions //
// native ClearBanList();
AMX_DECLARE_NATIVE(Natives::ClearBanList)
{
	if (!CServer::Get()->IsInitialized()) return std::numeric_limits<int>::lowest(); // If unknown server version

	CSAMPFunctions::ClearBanList();
	CServer::Get()->ClearBans();
	return 1;
}

// native IsBanned(ipaddress[]);
AMX_DECLARE_NATIVE(Natives::IsBanned)
{
	CHECK_PARAMS(1, LOADED);

	char *ip;
	amx_StrParam(amx, params[1], ip);
	return (ip) ? CServer::Get()->IsBanned(ip) : 0;
}

// native SetTimeoutTime(playerid, time);
AMX_DECLARE_NATIVE(Natives::SetTimeoutTime)
{
	CHECK_PARAMS(2, LOADED);
	
	const PlayerID playerId = CSAMPFunctions::GetPlayerIDFromIndex(CScriptParams::Get()->ReadInt());
	if(playerId.binaryAddress == UNASSIGNED_PLAYER_ID.binaryAddress || !IsPlayerConnected(static_cast<int>(params[1])))
		return 0;

	CSAMPFunctions::SetTimeoutTime(static_cast<RakNetTime>(CScriptParams::Get()->ReadInt()), playerId);
	return 1;
}

// native GetLocalIP(index, localip[], len = sizeof(localip));
AMX_DECLARE_NATIVE(Natives::GetLocalIP)
{
	CHECK_PARAMS(3, LOADED);

	return set_amxstring(amx, params[2], CSAMPFunctions::GetLocalIP(CScriptParams::Get()->ReadInt()), params[3]);
}

// native SendRPC(playerid, RPC, {Float,_}:...)
AMX_DECLARE_NATIVE(Natives::SendRPC)
{
	if (!CServer::Get()->IsInitialized()) return std::numeric_limits<int>::lowest(); // If unknown server version

	const bool bBroadcast = static_cast<int>(params[1]) == -1;
	BYTE rpcid = static_cast<BYTE>(params[2]);
	
	PlayerID playerId = bBroadcast ? UNASSIGNED_PLAYER_ID : CSAMPFunctions::GetPlayerIDFromIndex(static_cast<int>(params[1]));
	
	if (playerId.binaryAddress == UNASSIGNED_PLAYER_ID.binaryAddress && !bBroadcast)
		return 0;
	
	RakNet::BitStream bs;
	cell *type = (cell*)0;
	cell *data = (cell*)0;

	for (int i = 0; i < (int)((params[0]/sizeof(cell)) - 2); i+=2)
	{
		amx_GetAddr(amx, params[i + 3], &type);
		amx_GetAddr(amx, params[i + 4], &data);
					
		if (type && data)
		{
			switch (*type)
			{
			case BS_BOOL:
				bs.Write((bool)(*data!=0));
				break;
			case BS_CHAR:
				bs.Write(*(char*)data);
				break;
			case BS_UNSIGNEDCHAR:
				bs.Write(*(unsigned char*)data);
				break;
			case BS_SHORT:
				bs.Write(*(short*)data);
				break;
			case BS_UNSIGNEDSHORT:
				bs.Write(*(unsigned short*)data);
				break;
			case BS_INT:
				bs.Write(*(int*)data);
				break;
			case BS_UNSIGNEDINT:
				bs.Write(*(unsigned int*)data);
				break;
			case BS_FLOAT:
				bs.Write(*(float*)data);
				break;
			case BS_STRING:
				{
					int len;
					amx_StrLen(data, &len);
					len++;
					char* str = new char[len];
					amx_GetString(str, data, 0, len);
					bs.Write(str, len - 1);
					//logprintf("str: %s", str);
					delete [] str;
				}
				break;
			}
		}
	}

	if(bBroadcast)
	{
		CSAMPFunctions::RPC(&rpcid, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, UNASSIGNED_PLAYER_ID, true, 0);
	}
	else
	{
		CSAMPFunctions::RPC(&rpcid, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, playerId, 0, 0);
	}
	return 1;
}

// native SendData(playerid, {Float,_}:...)
AMX_DECLARE_NATIVE(Natives::SendData)
{
	if (!CServer::Get()->IsInitialized()) return std::numeric_limits<int>::lowest(); // If unknown server version

	const bool bBroadcast = static_cast<int>(params[1]) == -1;
	const PlayerID playerId = bBroadcast ? UNASSIGNED_PLAYER_ID : CSAMPFunctions::GetPlayerIDFromIndex(static_cast<int>(params[1]));

	if (playerId.binaryAddress == UNASSIGNED_PLAYER_ID.binaryAddress && !bBroadcast)
		return 0;
	
	RakNet::BitStream bs;
	cell *type = (cell*)0;
	cell *data = (cell*)0;

	for (int i = 0; i < (int)((params[0]/sizeof(cell)) - 2); i+=2)
	{
		amx_GetAddr(amx, params[i + 2], &type);
		amx_GetAddr(amx, params[i + 3], &data);
					
		if (type && data)
		{
			switch (*type)
			{
			case BS_BOOL:
				bs.Write((bool)(*data!=0));
				break;
			case BS_CHAR:
				bs.Write(*(char*)data);
				break;
			case BS_UNSIGNEDCHAR:
				bs.Write(*(unsigned char*)data);
				break;
			case BS_SHORT:
				bs.Write(*(short*)data);
				break;
			case BS_UNSIGNEDSHORT:
				bs.Write(*(unsigned short*)data);
				break;
			case BS_INT:
				bs.Write(*(int*)data);
				break;
			case BS_UNSIGNEDINT:
				bs.Write(*(unsigned int*)data);
				break;
			case BS_FLOAT:
				bs.Write(*(float*)data);
				break;
			case BS_STRING:
				{
					int len;
					amx_StrLen(data, &len);
					len++;
					char* str = new char[len];
					amx_GetString(str, data, 0, len);
					bs.Write(str, len - 1);
					//logprintf("str: %s", str);
					delete [] str;
				}
				break;
			}
		}
	}

	if(bBroadcast)
	{
		CSAMPFunctions::Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, UNASSIGNED_PLAYER_ID, true);
	}
	else
	{
		CSAMPFunctions::Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, playerId, 0);
	}
	return 1;
}

// native GetColCount();
AMX_DECLARE_NATIVE(Natives::GetColCount)
{
	return CModelSizes::GetColCount();
}

// native Float:GetColSphereRadius(modelid);
AMX_DECLARE_NATIVE(Natives::GetColSphereRadius)
{
	CHECK_PARAMS(1,  NO_FLAGS);
	
	float fRet = CModelSizes::GetColSphereRadius(CScriptParams::Get()->ReadInt());
	return amx_ftoc(fRet);
}

// native GetColSphereOffset(modelid, &Float:fX, &Float:fY, &Float:fZ);
AMX_DECLARE_NATIVE(Natives::GetColSphereOffset)
{
	CHECK_PARAMS(4, NO_FLAGS);

	CVector vecOffset = CModelSizes::GetColSphereOffset(CScriptParams::Get()->ReadInt());
	CScriptParams::Get()->Add(vecOffset);
	return 1;
}

// native GetWeaponSlot(weaponid);
AMX_DECLARE_NATIVE(Natives::GetWeaponSlot)
{
	CHECK_PARAMS(1, NO_FLAGS);
	
	return Utility::GetWeaponSlot(CScriptParams::Get()->ReadInt());
}

// native GetWeaponName(weaponid, weaponname[], len = sizeof(weaponname));
AMX_DECLARE_NATIVE(Natives::FIXED_GetWeaponName)
{
	CHECK_PARAMS(3, NO_FLAGS);

	return set_amxstring(amx, params[2], Utility::GetWeaponName(static_cast<BYTE>(params[1])), params[3]);
}

// native IsPlayerConnected(playerid);
AMX_DECLARE_NATIVE(Natives::FIXED_IsPlayerConnected)
{
	CHECK_PARAMS(1, LOADED);

	const int playerid = CScriptParams::Get()->ReadInt();
	if (playerid < 0 || playerid >= MAX_PLAYERS) return 0;

	return pNetGame->pPlayerPool->pPlayer[playerid] != NULL;
}

#ifdef NEW_PICKUP_SYSTEM
// native CreatePickup(model, type, Float:X, Float:Y, Float:Z, virtualworld = 0);
AMX_DECLARE_NATIVE(Natives::CreatePickup)
{
	CHECK_PARAMS(6, LOADED);

	return CServer::Get()->pPickupPool->New((int)params[1], (int)params[2], CVector(amx_ctof(params[3]), amx_ctof(params[4]), amx_ctof(params[5])), (int)params[6]);
}

// native DestroyPickup(pickupid);
AMX_DECLARE_NATIVE(Natives::DestroyPickup)
{
	CHECK_PARAMS(1, LOADED);

	CServer::Get()->pPickupPool->Destroy((int)params[1]);
	return 1;
}

// native SetPickupStreamingEnabled(enabled);
AMX_DECLARE_NATIVE(Natives::SetPickupStreamingEnabled)
{
	CHECK_PARAMS(1, LOADED);

	CServer::Get()->pPickupPool->SetStreamingEnabled(!!params[1]);
	return 1;
}
#endif

// And an array containing the native function-names and the functions specified with them
AMX_NATIVE_INFO native_list[] =
{
	// Execute
	AMX_DEFINE_NATIVE(execute)

	// File
	AMX_DEFINE_NATIVE(ffind)
	AMX_DEFINE_NATIVE(frename)
	
	// Directory
	AMX_DEFINE_NATIVE(dfind)
	AMX_DEFINE_NATIVE(dcreate)
	AMX_DEFINE_NATIVE(drename)

	// Generic
	AMX_DEFINE_NATIVE(SetModeRestartTime)
	AMX_DEFINE_NATIVE(GetModeRestartTime)
	AMX_DEFINE_NATIVE(SetMaxPlayers) // R8
	AMX_DEFINE_NATIVE(SetMaxNPCs) // R8
	AMX_DEFINE_NATIVE(GetSyncBounds) // R19
	AMX_DEFINE_NATIVE(SetSyncBounds) // R19

	AMX_DEFINE_NATIVE(SetPlayerAdmin)
	AMX_DEFINE_NATIVE(LoadFilterScript)
	AMX_DEFINE_NATIVE(UnLoadFilterScript)
	AMX_DEFINE_NATIVE(GetFilterScriptCount)
	AMX_DEFINE_NATIVE(GetFilterScriptName)

	AMX_DEFINE_NATIVE(AddServerRule)
	AMX_DEFINE_NATIVE(SetServerRule)
	AMX_DEFINE_NATIVE(SetServerRuleInt)
	AMX_DEFINE_NATIVE(IsValidServerRule)
	AMX_DEFINE_NATIVE(RemoveServerRule) // Doesn't work!
	AMX_DEFINE_NATIVE(SetServerRuleFlags)
	AMX_DEFINE_NATIVE(GetServerRuleFlags)

	// Server settings
	AMX_DEFINE_NATIVE(GetServerSettings)
	AMX_DEFINE_NATIVE(GetNPCCommandLine) // R19

	// RCON Commands
	AMX_DEFINE_NATIVE(ChangeRCONCommandName) // R19
	AMX_DEFINE_NATIVE(GetRCONCommandName) // R19

	// Per AMX function calling
	AMX_DEFINE_NATIVE(CallFunctionInScript) // R19

	// Nick name
	AMX_DEFINE_NATIVE(IsValidNickName)	// R8
	AMX_DEFINE_NATIVE(AllowNickNameCharacter) // R7
	AMX_DEFINE_NATIVE(IsNickNameCharacterAllowed) // R7

	// Player classes
	AMX_DEFINE_NATIVE(GetAvailableClasses) // R6
	AMX_DEFINE_NATIVE(RemoveLastClass) // R16
	AMX_DEFINE_NATIVE(GetPlayerClass) // R6
	AMX_DEFINE_NATIVE(EditPlayerClass) // R6
	
	// Timers
	AMX_DEFINE_NATIVE(GetRunningTimers) // R8

	// Special
	AMX_DEFINE_NATIVE(SetPlayerGravity)
	AMX_DEFINE_NATIVE(GetPlayerGravity)
	
	AMX_DEFINE_NATIVE(SetPlayerTeamForPlayer) // R5
	AMX_DEFINE_NATIVE(GetPlayerTeamForPlayer) // R5
	AMX_DEFINE_NATIVE(SetPlayerSkinForPlayer) // R11
	AMX_DEFINE_NATIVE(GetPlayerSkinForPlayer) // R11
	AMX_DEFINE_NATIVE(SetPlayerNameForPlayer) // R11
	AMX_DEFINE_NATIVE(GetPlayerNameForPlayer) // R11
	AMX_DEFINE_NATIVE(SetPlayerFightStyleForPlayer) // R11
	AMX_DEFINE_NATIVE(GetPlayerFightStyleForPlayer) // R11
	AMX_DEFINE_NATIVE(SetPlayerPosForPlayer) // R12
	AMX_DEFINE_NATIVE(SetPlayerRotationQuatForPlayer) // R12
	AMX_DEFINE_NATIVE(ApplyAnimationForPlayer) // R11
	AMX_DEFINE_NATIVE(GetPlayerWeather)
	AMX_DEFINE_NATIVE(GetPlayerWorldBounds)
	AMX_DEFINE_NATIVE(TogglePlayerWidescreen)
	AMX_DEFINE_NATIVE(IsPlayerWidescreenToggled)
	AMX_DEFINE_NATIVE(GetSpawnInfo) // R8
	AMX_DEFINE_NATIVE(GetPlayerSkillLevel) // R3
	AMX_DEFINE_NATIVE(IsPlayerCheckpointActive) // R10
	AMX_DEFINE_NATIVE(GetPlayerCheckpoint) // R4
	AMX_DEFINE_NATIVE(IsPlayerRaceCheckpointActive) // R10
	AMX_DEFINE_NATIVE(GetPlayerRaceCheckpoint) // R4
	AMX_DEFINE_NATIVE(GetPlayerWorldBounds) // R5
	AMX_DEFINE_NATIVE(IsPlayerInModShop) // R4
	AMX_DEFINE_NATIVE(SendBulletData) // R6
	AMX_DEFINE_NATIVE(ShowPlayerForPlayer) // R8
	AMX_DEFINE_NATIVE(HidePlayerForPlayer) // R8
	AMX_DEFINE_NATIVE(AddPlayerForPlayer) // R17
	AMX_DEFINE_NATIVE(RemovePlayerForPlayer) // R17
	AMX_DEFINE_NATIVE(SetPlayerChatBubbleForPlayer) // R10
	AMX_DEFINE_NATIVE(SetPlayerVersion) // R9
	AMX_DEFINE_NATIVE(IsPlayerSpawned) // R9
	AMX_DEFINE_NATIVE(IsPlayerControllable) // R11
	AMX_DEFINE_NATIVE(SpawnForWorld) // R10
	AMX_DEFINE_NATIVE(BroadcastDeath) // R13
	AMX_DEFINE_NATIVE(IsPlayerCameraTargetEnabled) // R13
	AMX_DEFINE_NATIVE(SetPlayerDisabledKeysSync) // R16
	AMX_DEFINE_NATIVE(GetPlayerDisabledKeysSync) // R16
	
	// Special things from syncdata
	AMX_DEFINE_NATIVE(GetPlayerSirenState)
	AMX_DEFINE_NATIVE(GetPlayerLandingGearState)
	AMX_DEFINE_NATIVE(GetPlayerHydraReactorAngle)
	AMX_DEFINE_NATIVE(GetPlayerTrainSpeed)
	AMX_DEFINE_NATIVE(GetPlayerZAim)
	AMX_DEFINE_NATIVE(GetPlayerSurfingOffsets)
	AMX_DEFINE_NATIVE(GetPlayerRotationQuat) // R3
	AMX_DEFINE_NATIVE(GetPlayerDialogID) // R8
	AMX_DEFINE_NATIVE(GetPlayerSpectateID) // R8
	AMX_DEFINE_NATIVE(GetPlayerSpectateType) // R8
	AMX_DEFINE_NATIVE(GetPlayerLastSyncedVehicleID) // R10
	AMX_DEFINE_NATIVE(GetPlayerLastSyncedTrailerID) // R10

	// Actor
	AMX_DEFINE_NATIVE(GetActorSpawnInfo) // R13
	AMX_DEFINE_NATIVE(GetActorSkin) // R13
	AMX_DEFINE_NATIVE(GetActorAnimation) // R17

	// Scoreboard manipulation
	AMX_DEFINE_NATIVE(TogglePlayerScoresPingsUpdate) // R8
	AMX_DEFINE_NATIVE(TogglePlayerFakePing) // R8
	AMX_DEFINE_NATIVE(SetPlayerFakePing) // R8
	AMX_DEFINE_NATIVE(SetPlayerNameInServerQuery) // R11
	AMX_DEFINE_NATIVE(GetPlayerNameInServerQuery) // R11
	AMX_DEFINE_NATIVE(SetPlayerNameInServerQuery) // R11
	AMX_DEFINE_NATIVE(GetPlayerNameInServerQuery) // R11

	// AFK
	AMX_DEFINE_NATIVE(IsPlayerPaused)
	AMX_DEFINE_NATIVE(GetPlayerPausedTime)
	
	// Objects get - global
	AMX_DEFINE_NATIVE(GetObjectDrawDistance)
	AMX_DEFINE_NATIVE(SetObjectMoveSpeed) // R6
	AMX_DEFINE_NATIVE(GetObjectMoveSpeed) // R6
	AMX_DEFINE_NATIVE(GetObjectTarget) // R6
	AMX_DEFINE_NATIVE(GetObjectAttachedData)
	AMX_DEFINE_NATIVE(GetObjectAttachedOffset)
	AMX_DEFINE_NATIVE(IsObjectMaterialSlotUsed) // R6
	AMX_DEFINE_NATIVE(GetObjectMaterial) // R6
	AMX_DEFINE_NATIVE(GetObjectMaterialText) // R6
	AMX_DEFINE_NATIVE(IsObjectNoCameraCol) // R13

	// Objects get - player
	AMX_DEFINE_NATIVE(GetPlayerObjectDrawDistance)
	AMX_DEFINE_NATIVE(SetPlayerObjectMoveSpeed) // R6
	AMX_DEFINE_NATIVE(GetPlayerObjectMoveSpeed) // R6
	AMX_DEFINE_NATIVE(GetPlayerObjectTarget) // R6
	AMX_DEFINE_NATIVE(GetPlayerObjectAttachedData)
	AMX_DEFINE_NATIVE(GetPlayerObjectAttachedOffset)
	AMX_DEFINE_NATIVE(IsPlayerObjectMaterialSlotUsed) // R6
	AMX_DEFINE_NATIVE(GetPlayerObjectMaterial) // R6
	AMX_DEFINE_NATIVE(GetPlayerObjectMaterialText) // R6
	AMX_DEFINE_NATIVE(IsPlayerObjectNoCameraCol) // R13
	AMX_DEFINE_NATIVE(GetPlayerSurfingPlayerObjectID) // R12
	AMX_DEFINE_NATIVE(GetPlayerCameraTargetPlayerObj) // R13
	AMX_DEFINE_NATIVE(GetObjectType)// R12

	// special - for attached objects
	AMX_DEFINE_NATIVE(GetPlayerAttachedObject) // R3
	AMX_DEFINE_NATIVE(SetPlayerAttachedObjForPlayer) // R19-2
	AMX_DEFINE_NATIVE(GetPlayerAttachedObjForPlayer) // R19-2
	AMX_DEFINE_NATIVE(RemPlayerAttachedObjForPlayer) // R19-2
	AMX_DEFINE_NATIVE(IsPlayerAttachedObjForPlayer) // R19-2

	// Vehicle functions
	AMX_DEFINE_NATIVE(GetVehicleSpawnInfo)
	AMX_DEFINE_NATIVE(SetVehicleSpawnInfo) // R16
	AMX_DEFINE_NATIVE(GetVehicleColor)
	AMX_DEFINE_NATIVE(GetVehiclePaintjob)
	AMX_DEFINE_NATIVE(GetVehicleInterior)
	AMX_DEFINE_NATIVE(GetVehicleNumberPlate)
	AMX_DEFINE_NATIVE(SetVehicleRespawnDelay)
	AMX_DEFINE_NATIVE(GetVehicleRespawnDelay)
	AMX_DEFINE_NATIVE(SetVehicleOccupiedTick) // R6
	AMX_DEFINE_NATIVE(GetVehicleOccupiedTick)
	AMX_DEFINE_NATIVE(SetVehicleRespawnTick)
	AMX_DEFINE_NATIVE(GetVehicleRespawnTick)
	AMX_DEFINE_NATIVE(GetVehicleLastDriver)
	AMX_DEFINE_NATIVE(GetVehicleCab) // R9
	AMX_DEFINE_NATIVE(HasVehicleBeenOccupied) // R9
	AMX_DEFINE_NATIVE(SetVehicleBeenOccupied) // R9
	AMX_DEFINE_NATIVE(IsVehicleOccupied) // R9
	AMX_DEFINE_NATIVE(IsVehicleDead) // R9
	AMX_DEFINE_NATIVE(SetVehicleParamsSirenState) // R19
	AMX_DEFINE_NATIVE(ToggleVehicleSirenEnabled) // R19
	AMX_DEFINE_NATIVE(IsVehicleSirenEnabled) // R19
	AMX_DEFINE_NATIVE(GetVehicleMatrix) // R19
	AMX_DEFINE_NATIVE(GetVehicleModelCount) // R17
	AMX_DEFINE_NATIVE(GetVehicleModelsUsed) // R17

	// Gangzone - Global
	AMX_DEFINE_NATIVE(IsValidGangZone)
	AMX_DEFINE_NATIVE(IsPlayerInGangZone)
	AMX_DEFINE_NATIVE(IsGangZoneVisibleForPlayer)
	AMX_DEFINE_NATIVE(GangZoneGetColorForPlayer)
	AMX_DEFINE_NATIVE(GangZoneGetFlashColorForPlayer)
	AMX_DEFINE_NATIVE(IsGangZoneFlashingForPlayer) // R6
	AMX_DEFINE_NATIVE(GangZoneGetPos)

	// Gangzone - Player
	AMX_DEFINE_NATIVE(CreatePlayerGangZone)
	AMX_DEFINE_NATIVE(PlayerGangZoneDestroy)
	AMX_DEFINE_NATIVE(PlayerGangZoneShow)
	AMX_DEFINE_NATIVE(PlayerGangZoneHide)
	AMX_DEFINE_NATIVE(PlayerGangZoneFlash)
	AMX_DEFINE_NATIVE(PlayerGangZoneStopFlash)
	AMX_DEFINE_NATIVE(IsValidPlayerGangZone)
	AMX_DEFINE_NATIVE(IsPlayerInPlayerGangZone)
	AMX_DEFINE_NATIVE(IsPlayerGangZoneVisible)
	AMX_DEFINE_NATIVE(PlayerGangZoneGetColor)
	AMX_DEFINE_NATIVE(PlayerGangZoneGetFlashColor)
	AMX_DEFINE_NATIVE(IsPlayerGangZoneFlashing) // R6
	AMX_DEFINE_NATIVE(PlayerGangZoneGetPos)

	// Textdraw functions
	AMX_DEFINE_NATIVE(IsValidTextDraw)
	AMX_DEFINE_NATIVE(IsTextDrawVisibleForPlayer)
	AMX_DEFINE_NATIVE(TextDrawGetString)
	AMX_DEFINE_NATIVE(TextDrawSetPos)
	AMX_DEFINE_NATIVE(TextDrawGetLetterSize)
	AMX_DEFINE_NATIVE(TextDrawGetTextSize)
	AMX_DEFINE_NATIVE(TextDrawGetPos)
	AMX_DEFINE_NATIVE(TextDrawGetColor)
	AMX_DEFINE_NATIVE(TextDrawGetBoxColor)
	AMX_DEFINE_NATIVE(TextDrawGetBackgroundColor)
	AMX_DEFINE_NATIVE(TextDrawGetShadow)
	AMX_DEFINE_NATIVE(TextDrawGetOutline)
	AMX_DEFINE_NATIVE(TextDrawGetFont)
	AMX_DEFINE_NATIVE(TextDrawIsBox)
	AMX_DEFINE_NATIVE(TextDrawIsProportional)
	AMX_DEFINE_NATIVE(TextDrawIsSelectable) // R6
	AMX_DEFINE_NATIVE(TextDrawGetAlignment)
	AMX_DEFINE_NATIVE(TextDrawGetPreviewModel)
	AMX_DEFINE_NATIVE(TextDrawGetPreviewRot)
	AMX_DEFINE_NATIVE(TextDrawGetPreviewVehCol)
	AMX_DEFINE_NATIVE(TextDrawSetStringForPlayer) // R19

	// Per-Player Textdraw functions - R4
	AMX_DEFINE_NATIVE(IsValidPlayerTextDraw)
	AMX_DEFINE_NATIVE(IsPlayerTextDrawVisible)
	AMX_DEFINE_NATIVE(PlayerTextDrawGetString)
	AMX_DEFINE_NATIVE(PlayerTextDrawSetPos)
	AMX_DEFINE_NATIVE(PlayerTextDrawGetLetterSize)
	AMX_DEFINE_NATIVE(PlayerTextDrawGetTextSize)
	AMX_DEFINE_NATIVE(PlayerTextDrawGetPos)
	AMX_DEFINE_NATIVE(PlayerTextDrawGetColor)
	AMX_DEFINE_NATIVE(PlayerTextDrawGetBoxColor)
	AMX_DEFINE_NATIVE(PlayerTextDrawGetBackgroundCol)
	AMX_DEFINE_NATIVE(PlayerTextDrawGetShadow)
	AMX_DEFINE_NATIVE(PlayerTextDrawGetOutline)
	AMX_DEFINE_NATIVE(PlayerTextDrawGetFont)
	AMX_DEFINE_NATIVE(PlayerTextDrawIsBox)
	AMX_DEFINE_NATIVE(PlayerTextDrawIsProportional)
	AMX_DEFINE_NATIVE(PlayerTextDrawIsSelectable) // R6
	AMX_DEFINE_NATIVE(PlayerTextDrawGetAlignment)
	AMX_DEFINE_NATIVE(PlayerTextDrawGetPreviewModel)
	AMX_DEFINE_NATIVE(PlayerTextDrawGetPreviewRot)
	AMX_DEFINE_NATIVE(PlayerTextDrawGetPreviewVehCol)
	AMX_DEFINE_NATIVE(TextDrawSetStringForPlayer) // R19

	// 3D Text
	AMX_DEFINE_NATIVE(IsValid3DTextLabel) // R4
	AMX_DEFINE_NATIVE(Is3DTextLabelStreamedIn) // R9
	AMX_DEFINE_NATIVE(Get3DTextLabelText)
	AMX_DEFINE_NATIVE(Get3DTextLabelColor)
	AMX_DEFINE_NATIVE(Get3DTextLabelPos)
	AMX_DEFINE_NATIVE(Get3DTextLabelDrawDistance)
	AMX_DEFINE_NATIVE(Get3DTextLabelLOS)
	AMX_DEFINE_NATIVE(Get3DTextLabelVirtualWorld)
	AMX_DEFINE_NATIVE(Get3DTextLabelAttachedData)

	// Per-Player 3D Text
	AMX_DEFINE_NATIVE(IsValidPlayer3DTextLabel) // R4
	AMX_DEFINE_NATIVE(GetPlayer3DTextLabelText) // R4
	AMX_DEFINE_NATIVE(GetPlayer3DTextLabelColor) // R4
	AMX_DEFINE_NATIVE(GetPlayer3DTextLabelPos) // R4
	AMX_DEFINE_NATIVE(GetPlayer3DTextLabelDrawDist)
	AMX_DEFINE_NATIVE(GetPlayer3DTextLabelLOS) // R4
	AMX_DEFINE_NATIVE(GetPlayer3DTextLabelVirtualW) // R4
	AMX_DEFINE_NATIVE(GetPlayer3DTextLabelAttached) // R9

	// Menus
	AMX_DEFINE_NATIVE(IsMenuDisabled) // R5 
	AMX_DEFINE_NATIVE(IsMenuRowDisabled) // R5
	AMX_DEFINE_NATIVE(GetMenuColumns)
	AMX_DEFINE_NATIVE(GetMenuItems)
	AMX_DEFINE_NATIVE(GetMenuPos)
	AMX_DEFINE_NATIVE(GetMenuColumnWidth)
	AMX_DEFINE_NATIVE(GetMenuColumnHeader)
	AMX_DEFINE_NATIVE(GetMenuItem)
	
	// Pickups - Global
	AMX_DEFINE_NATIVE(IsValidPickup) // R10
	AMX_DEFINE_NATIVE(IsPickupStreamedIn) // R10
	AMX_DEFINE_NATIVE(GetPickupPos) // R10
	AMX_DEFINE_NATIVE(GetPickupModel) // R10
	AMX_DEFINE_NATIVE(GetPickupType) // R10
	AMX_DEFINE_NATIVE(GetPickupVirtualWorld) // R10
#ifdef NEW_PICKUP_SYSTEM
	// Pickups - Per-player
	AMX_DEFINE_NATIVE(CreatePlayerPickup) // R10
	AMX_DEFINE_NATIVE(DestroyPlayerPickup) // R10
	AMX_DEFINE_NATIVE(IsValidPlayerPickup) // R10
	AMX_DEFINE_NATIVE(IsPlayerPickupStreamedIn) // R10
	AMX_DEFINE_NATIVE(GetPlayerPickupPos) // R10
	AMX_DEFINE_NATIVE(GetPlayerPickupModel) // R10
	AMX_DEFINE_NATIVE(GetPlayerPickupType) // R10
	AMX_DEFINE_NATIVE(GetPlayerPickupVirtualWorld) // R10
#endif
	// RakServer functions
	AMX_DEFINE_NATIVE(ClearBanList)
	AMX_DEFINE_NATIVE(IsBanned)

	AMX_DEFINE_NATIVE(SetTimeoutTime)
	AMX_DEFINE_NATIVE(GetLocalIP)

	AMX_DEFINE_NATIVE(SendRPC)
	AMX_DEFINE_NATIVE(SendData)
	AMX_DEFINE_NATIVE(YSF_SetTickRate)
	AMX_DEFINE_NATIVE(YSF_GetTickRate)
	AMX_DEFINE_NATIVE(YSF_EnableNightVisionFix)
	AMX_DEFINE_NATIVE(YSF_IsNightVisionFixEnabled)
	AMX_DEFINE_NATIVE(YSF_ToggleOnServerMessage) // R18-2
	AMX_DEFINE_NATIVE(YSF_IsOnServerMessageEnabled) // R18-2
	AMX_DEFINE_NATIVE(YSF_SetExtendedNetStatsEnabled) // R17
	AMX_DEFINE_NATIVE(YSF_IsExtendedNetStatsEnabled) // R17
	AMX_DEFINE_NATIVE(YSF_SetAFKAccuracy) // R17
	AMX_DEFINE_NATIVE(YSF_GetAFKAccuracy) // R17 
	AMX_DEFINE_NATIVE(EnableConsoleMSGsForPlayer) // R18
	AMX_DEFINE_NATIVE(DisableConsoleMSGsForPlayer) // R18
	AMX_DEFINE_NATIVE(HasPlayerConsoleMessages) // R18 

	AMX_DEFINE_NATIVE(AttachPlayerObjectToObject)

	// Exclusive RPC broadcast
	AMX_DEFINE_NATIVE(SetExclusiveBroadcast)
	AMX_DEFINE_NATIVE(BroadcastToPlayer)
	
	// Recording functions
	AMX_DEFINE_NATIVE(SetRecordingDirectory) // R17
	AMX_DEFINE_NATIVE(GetRecordingDirectory) // R17

	// Format functions
	AMX_DEFINE_NATIVE(SendClientMessagef)
	AMX_DEFINE_NATIVE(SendClientMessageToAllf)
	AMX_DEFINE_NATIVE(GameTextForPlayerf)
	AMX_DEFINE_NATIVE(GameTextForAllf)
	AMX_DEFINE_NATIVE(SendPlayerMessageToPlayerf)
	AMX_DEFINE_NATIVE(SendPlayerMessageToAllf)
	AMX_DEFINE_NATIVE(SendRconCommandf)

	// Other
	AMX_DEFINE_NATIVE(GetColCount)
	AMX_DEFINE_NATIVE(GetColSphereRadius)
	AMX_DEFINE_NATIVE(GetColSphereOffset)

	AMX_DEFINE_NATIVE(GetWeaponSlot)

#ifdef NEW_PICKUP_SYSTEM
	AMX_DEFINE_NATIVE(SetPickupStreamingEnabled)
#endif
	{ NULL,								NULL }
};

AMX_NATIVE_INFO redirected_native_list[] =
{
	// File
	{ "AttachObjectToPlayer",			Natives::YSF_AttachObjectToPlayer },
	{ "AttachPlayerObjectToPlayer",		Natives::YSF_AttachPlayerObjectToPlayer },
	{ "GetGravity",						Natives::YSF_GetGravity },
	{ "SetPlayerWeather",				Natives::YSF_SetPlayerWeather },
	{ "SetPlayerWorldBounds",			Natives::YSF_SetPlayerWorldBounds },
	{ "DestroyObject",					Natives::YSF_DestroyObject },
	{ "DestroyPlayerObject",			Natives::YSF_DestroyPlayerObject },
	{ "TogglePlayerControllable",		Natives::YSF_TogglePlayerControllable},
	{ "ChangeVehicleColor",				Natives::YSF_ChangeVehicleColor},
	{ "DestroyVehicle",					Natives::YSF_DestroyVehicle},
	{ "ShowPlayerDialog",				Natives::YSF_ShowPlayerDialog },
	{ "SetPlayerObjectMaterial",		Natives::YSF_SetPlayerObjectMaterial },
	{ "SetPlayerObjectMaterialText",	Natives::YSF_SetPlayerObjectMaterialText },

	
	{ "GangZoneCreate",					Natives::YSF_GangZoneCreate },
	{ "GangZoneDestroy",				Natives::YSF_GangZoneDestroy },
	{ "GangZoneShowForPlayer",			Natives::YSF_GangZoneShowForPlayer },
	{ "GangZoneHideForPlayer",			Natives::YSF_GangZoneHideForPlayer },
	{ "GangZoneShowForAll",				Natives::YSF_GangZoneShowForAll },
	{ "GangZoneHideForAll",				Natives::YSF_GangZoneHideForAll },
								
	{ "GangZoneFlashForPlayer",			Natives::YSF_GangZoneFlashForPlayer },
	{ "GangZoneFlashForAll",			Natives::YSF_GangZoneFlashForAll },
	{ "GangZoneStopFlashForPlayer",		Natives::YSF_GangZoneStopFlashForPlayer },
	{ "GangZoneStopFlashForAll",		Natives::YSF_GangZoneStopFlashForAll },
#ifdef NEW_PICKUP_SYSTEM
	{ "CreatePickup",					Natives::CreatePickup },
	{ "AddStaticPickup",				Natives::CreatePickup },
	{ "DestroyPickup",					Natives::DestroyPickup },
#endif
	{ "GetWeaponName",					Natives::FIXED_GetWeaponName },
	{ "IsPlayerConnected",				Natives::FIXED_IsPlayerConnected },

	{ "SetPlayerTeam",					Natives::YSF_SetPlayerTeam },
	{ "SetPlayerSkin",					Natives::YSF_SetPlayerSkin },
	{ "SetPlayerName",					Natives::YSF_SetPlayerName },
	{ "SetPlayerFightingStyle",			Natives::YSF_SetPlayerFightingStyle },

	{ NULL,								NULL }
};

int InitNatives(AMX *amx)
{
	return amx_Register(amx, native_list, -1);
}
