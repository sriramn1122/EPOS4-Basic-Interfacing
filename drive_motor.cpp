/*
 * Created Date: Thursday, December 5th 2019, 12:21:41 pm
 * Author: Sriram Narayanan
 * Email: ramsri28@gmail.com
 * -----
 * Last Modified: Thu Dec 12 2019 2:02:53 PM
 * Modified By: Sriram Narayanan
 * -----
 * Purpose:
 * ----------	---	----------------------------------------------------------
 * To drive motor through EPOS4 module ,
 * Adaptation of original example by Dawid Sienkiewicz, maxon motor ag 
 * 
 * Configuration :
 * 				Motor : DCX22l
 * 			 GearHead : GPX22lN
 * 			  Encoder : ENX16EASY 
 * 
 * Available Modes of Operation:
 * 			   vel    : Velocity mode
 * 			   pos    : Position mode
 * 
 * Motor Operating Limit:
 * 			  Max RPM :  104
 * 			  Min RPM : -104
 * 
 * 
 */

#include <iostream>
#include "Definitions.h"
#include <string.h>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <list>
#include <math.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/times.h>
#include <sys/time.h>
#include <chrono>
#include <thread>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <bits/stdc++.h>

using namespace std;

typedef void *HANDLE;
typedef int BOOL;

void *g_pKeyHandle = 0;
unsigned short g_usNodeId = 1;

int g_baudrate = 0;
int enc_cpt = 1024;
int gear_ratio = 111;
float g_logInterval = 1.0;

string destinationName;
string g_deviceName;
string g_protocolStackName;
string g_interfaceName;
string g_portName;
string g_opt;
const string g_programName = "drive_motor";
const string fileName = "motor_data";
const string directoryName = "motor_data";

ofstream outputFile;

enum MotorMode
{
	vel,
	pos
};

struct config
{
	string arg1 = "";
	string arg2 = "";
	string arg3 = "";
	string arg4 = "";
};

#ifndef MMC_SUCCESS
#define MMC_SUCCESS 0
#endif

#ifndef MMC_FAILED
#define MMC_FAILED 1
#endif

#ifndef MMC_MAX_LOG_MSG_SIZE
#define MMC_MAX_LOG_MSG_SIZE 512
#endif

#ifndef MICRO_TO_SEC
#define MICRO_TO_SEC pow(10, 6)
#endif

// List of functions being used

void LogError(string functionName, int p_lResult, unsigned int p_ulErrorCode);
void LogInfo(string message);
void SeparatorLine();
void PrintSettings();
void PrintHeader();
void PrintUsage();
void SetDefaultParameters();
int OpenDevice(unsigned int *p_pErrorCode);
int CloseDevice(unsigned int *p_pErrorCode);
int Setup(unsigned int *p_pErrorCode);
bool GetEnc(HANDLE p_DeviceHandle, unsigned short p_usNodeId, unsigned int &p_rlErrorCode);
bool ReadState(HANDLE p_DeviceHandle, unsigned short p_usNodeId, int &p_VelocityIs, int &p_PositionIs, short &p_CurrentIs, unsigned int &p_rlErrorCode);
bool DriveVelocityMode(HANDLE p_DeviceHandle, unsigned short p_usNodeId, unsigned int &p_rlErrorCode, long velocity);
bool DrivePositionMode(HANDLE p_DeviceHandle, unsigned short p_usNodeId, unsigned int &p_rlErrorCode, long double mode_val, unsigned int pos_vel);
int StartMotor(unsigned int *p_pErrorCode, long velocity);
void exit_callback_handler(int signum);

config ParseCommand(char **argv)
{
	config cfg;

	if (!argv[1])
	{
		PrintUsage();
		throw runtime_error("Invalid Number of Arguments, At least 2 args required");
	}
	cfg.arg1 = argv[1];

	if (!argv[2])
	{
		PrintUsage();
		throw runtime_error("Invalid Number of Arguments, At least 2 args required");
	}
	cfg.arg2 = argv[2];

	if (!argv[3])
		return cfg;
	cfg.arg3 = argv[3];

	if (!argv[4])
		return cfg;
	cfg.arg4 = argv[4];

	return cfg;
}

// Function definitions

void LogError(string functionName, int p_lResult, unsigned int p_ulErrorCode)
{
	cerr << g_programName << ": " << functionName << " failed (result=" << p_lResult << ", errorCode=0x" << std::hex << p_ulErrorCode << ")" << endl;
}

void LogInfo(string message)
{
	cout << message << endl;
}

void SeparatorLine()
{
	const int lineLength = 65;
	for (int i = 0; i < lineLength; i++)
	{
		cout << "-";
	}
	cout << endl;
}

void PrintSettings()
{
	stringstream msg;

	msg << "Default settings:" << endl;
	msg << "Node id             = " << g_usNodeId << endl;
	msg << "Device name         = " << g_deviceName << endl;
	msg << "Protocal stack name = " << g_protocolStackName << endl;
	msg << "Interface name      = " << g_interfaceName << endl;
	msg << "Port name           = " << g_portName << endl;
	msg << "Baudrate            = " << g_baudrate << endl;

	if (g_opt.compare("vel") == 0)
	{
		msg << "Mode                = "
			<< "Velocity" << endl;
	}
	else
	{
		msg << "Mode                = "
			<< "Position" << endl;
	}

	LogInfo(msg.str());
	SeparatorLine();
}

void PrintHeader()
{
	SeparatorLine();

	LogInfo("Drive Motor");

	SeparatorLine();
}

void PrintUsage()
{
	SeparatorLine();
	cout << "Usage: ./drive_motor option  argument_values\n";
	cout << "Available Options : \n";
	cout << "\tvel : Velocity Mode (Enter RPM between -104 and 104)" << endl;
	cout << "\tpos : Position Mode (Enter number of rotations of shaft)" << endl;
	cout << "To go back to 0 position, use 0 as value " << endl;
	SeparatorLine();
}

void SetDefaultParameters()
{
	g_usNodeId = 1;
	g_deviceName = "EPOS4";
	g_protocolStackName = "MAXON SERIAL V2";
	g_interfaceName = "USB";
	g_portName = "USB0";
	g_baudrate = 1000000;
}

int OpenDevice(unsigned int *p_pErrorCode)
{
	int lResult = MMC_FAILED;

	char *pDeviceName = new char[255];
	char *pProtocolStackName = new char[255];
	char *pInterfaceName = new char[255];
	char *pPortName = new char[255];

	strcpy(pDeviceName, g_deviceName.c_str());
	strcpy(pProtocolStackName, g_protocolStackName.c_str());
	strcpy(pInterfaceName, g_interfaceName.c_str());
	strcpy(pPortName, g_portName.c_str());

	LogInfo("Open device...");

	g_pKeyHandle = VCS_OpenDevice(pDeviceName, pProtocolStackName, pInterfaceName, pPortName, p_pErrorCode);

	if (g_pKeyHandle != 0 && *p_pErrorCode == 0)
	{
		unsigned int lBaudrate = 0;
		unsigned int lTimeout = 0;

		if (VCS_GetProtocolStackSettings(g_pKeyHandle, &lBaudrate, &lTimeout, p_pErrorCode) != 0)
		{
			if (VCS_SetProtocolStackSettings(g_pKeyHandle, g_baudrate, lTimeout, p_pErrorCode) != 0)
			{
				if (VCS_GetProtocolStackSettings(g_pKeyHandle, &lBaudrate, &lTimeout, p_pErrorCode) != 0)
				{
					if (g_baudrate == (int)lBaudrate)
					{
						lResult = MMC_SUCCESS;
					}
				}
			}
		}
	}
	else
	{
		g_pKeyHandle = 0;
	}

	delete[] pDeviceName;
	delete[] pProtocolStackName;
	delete[] pInterfaceName;
	delete[] pPortName;

	return lResult;
}

int CloseDevice(unsigned int *p_pErrorCode)
{
	int lResult = MMC_FAILED;

	*p_pErrorCode = 0;

	LogInfo("Close device");

	if (VCS_CloseDevice(g_pKeyHandle, p_pErrorCode) != 0 && *p_pErrorCode == 0)
	{
		lResult = MMC_SUCCESS;
	}

	return lResult;
}

int Setup(unsigned int *p_pErrorCode)
{
	int lResult = MMC_SUCCESS;
	BOOL oIsFault = 0;

	if (VCS_GetFaultState(g_pKeyHandle, g_usNodeId, &oIsFault, p_pErrorCode) == 0)
	{
		LogError("VCS_GetFaultState", lResult, *p_pErrorCode);
		lResult = MMC_FAILED;
	}

	if (lResult == 0)
	{
		if (oIsFault)
		{
			stringstream msg;
			msg << "clear fault, node = '" << g_usNodeId << "'";
			LogInfo(msg.str());

			if (VCS_ClearFault(g_pKeyHandle, g_usNodeId, p_pErrorCode) == 0)
			{
				LogError("VCS_ClearFault", lResult, *p_pErrorCode);
				lResult = MMC_FAILED;
			}
		}

		if (lResult == 0)
		{
			BOOL oIsEnabled = 0;

			if (VCS_GetEnableState(g_pKeyHandle, g_usNodeId, &oIsEnabled, p_pErrorCode) == 0)
			{
				LogError("VCS_GetEnableState", lResult, *p_pErrorCode);
				lResult = MMC_FAILED;
			}

			if (lResult == 0)
			{
				if (!oIsEnabled)
				{
					if (VCS_SetEnableState(g_pKeyHandle, g_usNodeId, p_pErrorCode) == 0)
					{
						LogError("VCS_SetEnableState", lResult, *p_pErrorCode);
						lResult = MMC_FAILED;
					}
				}
			}
		}
	}
	return lResult;
}

bool GetEnc(HANDLE p_DeviceHandle, unsigned short p_usNodeId, unsigned int &p_rlErrorCode)
{
	int lResult = MMC_SUCCESS;
	unsigned short p_SenType = 5;
	stringstream msg;

	if (VCS_GetSensorType(p_DeviceHandle, p_usNodeId, &p_SenType, &p_rlErrorCode) == 0)
	{
		LogError("VCS_SetSensorType", lResult, p_rlErrorCode);
		lResult = MMC_FAILED;
	}
	else
	{
		msg << "Sensor Type : " << p_SenType << endl;
		LogInfo(msg.str());
	}

	return lResult;
}

bool ReadState(HANDLE p_DeviceHandle, unsigned short p_usNodeId, int &p_VelocityIs, int &p_PositionIs, short &p_CurrentIs, unsigned int &p_rlErrorCode)
{
	int lResult = MMC_SUCCESS;
	stringstream msg;

	msg << "Reading State, Node I.D. = " << p_usNodeId << endl;

	if (VCS_GetVelocityIs(p_DeviceHandle, p_usNodeId, &p_VelocityIs, &p_rlErrorCode) == 0)
	{
		LogError("VCS_GetVelocityIs", lResult, p_rlErrorCode);
		lResult = MMC_FAILED;
	}
	else
	{
		msg << "Velocity = " << float(float(p_VelocityIs) / float(111)) << endl;
	}

	if (VCS_GetPositionIs(p_DeviceHandle, p_usNodeId, &p_PositionIs, &p_rlErrorCode) == 0)
	{
		LogError("VCS_GetPositionyIs", lResult, p_rlErrorCode);
		lResult = MMC_FAILED;
	}
	else
	{
		msg << "Position = " << p_PositionIs << endl;
	}

	if (VCS_GetCurrentIs(p_DeviceHandle, p_usNodeId, &p_CurrentIs, &p_rlErrorCode) == 0)
	{
		LogError("VCS_GetCurrentIs", lResult, p_rlErrorCode);
		lResult = MMC_FAILED;
	}
	else
	{
		msg << "Current = " << p_CurrentIs << endl;
		LogInfo(msg.str());
	}
	return lResult;
}

bool DriveVelocityMode(HANDLE p_DeviceHandle, unsigned short p_usNodeId, unsigned int &p_rlErrorCode, long double mode_val)
{
	int lResult = MMC_SUCCESS;
	stringstream msg;

	if (mode_val > 104 || mode_val < -104)
	{
		LogInfo("Enter a value between -104 and 104 RPM");
		if (VCS_SetDisableState(g_pKeyHandle, g_usNodeId, &p_rlErrorCode) == 0)
		{
			LogError("VCS_SetDisableState", lResult, p_rlErrorCode);
			lResult = MMC_FAILED;
		}
		exit(0);
	}

	msg << "Activating velocity mode" << endl
		<< "Node I.D. = " << p_usNodeId << endl;
	LogInfo(msg.str());

	if (VCS_ActivateVelocityMode(p_DeviceHandle, p_usNodeId, &p_rlErrorCode) == 0)
	{
		LogError("VCS_ActivateVelocityMode", lResult, p_rlErrorCode);
		lResult = MMC_FAILED;
	}
	else
	{
		long targetvelocity = mode_val * 111; // scaling due to gear head
		int VelocityIs = 0;
		int PositionIs = 0;
		short CurrentIs = 0;
		unsigned int lErrorCode = 0;

		msg << "Initializing move with target velocity = " << targetvelocity << " rpm, Node I.D. = " << p_usNodeId;
		LogInfo(msg.str());

		if (VCS_MoveWithVelocity(p_DeviceHandle, p_usNodeId, targetvelocity, &p_rlErrorCode) == 0)
		{
			lResult = MMC_FAILED;
			LogError("VCS_MoveWithVelocity", lResult, p_rlErrorCode);
		}

		while (true)
		{
			ReadState(p_DeviceHandle, p_usNodeId, VelocityIs, PositionIs, CurrentIs, lErrorCode);
			outputFile << float(float(VelocityIs) / float(111)) << "," << PositionIs << "," << CurrentIs << "," << time(nullptr) << endl;
			usleep(g_logInterval * MICRO_TO_SEC);
		}
	}

	return lResult;
}

bool DrivePositionMode(HANDLE p_DeviceHandle, unsigned short p_usNodeId, unsigned int &p_rlErrorCode, long double mode_val, unsigned int pos_vel)
{
	int lResult = MMC_SUCCESS;
	stringstream msg;
	bool absolute = true;
	bool immediate = true;
	unsigned int lErrorCode = 0;
	long double targetPosition = mode_val * 4 * enc_cpt * gear_ratio;
	unsigned short profileVelocity = pos_vel * gear_ratio;
	unsigned short profileAcc = 5000;
	unsigned short profileDec = 5000;

	// if(GetEnc(p_DeviceHandle, p_usNodeId, p_rlErrorCode) == 0)
	// {
	// 	LogError("SetEnc", lResult, p_rlErrorCode);
	// 	lResult = MMC_FAILED;
	// }

	if (VCS_ActivateProfilePositionMode(p_DeviceHandle, p_usNodeId, &p_rlErrorCode) == 0)
	{
		LogError("VCS_ActivateProfilePositionMode", lResult, p_rlErrorCode);
		lResult = MMC_FAILED;
	}
	else
	{
		if (VCS_SetPositionProfile(p_DeviceHandle, p_usNodeId, profileVelocity, profileAcc, profileDec, &p_rlErrorCode) == 0)
		{
			LogError("VCS_SetPositionProfile", lResult, p_rlErrorCode);
			lResult = MMC_FAILED;
		}
		msg << "Driving Motor to Position" << endl
			<< "Node I.D. = " << p_usNodeId << endl;

		if (VCS_MoveToPosition(p_DeviceHandle, p_usNodeId, targetPosition, absolute, immediate, &p_rlErrorCode) == 0)
		{
			LogError("VCSS_MoveToPosition", lResult, p_rlErrorCode);
			lResult = MMC_FAILED;
		}
		else
		{
			int VelocityIs = 0;
			int PositionIs = 0;
			short CurrentIs = 0;
			ReadState(p_DeviceHandle, p_usNodeId, VelocityIs, PositionIs, CurrentIs, lErrorCode);
			while (targetPosition != PositionIs)
			{
				VCS_MoveToPosition(p_DeviceHandle, p_usNodeId, targetPosition, absolute, immediate, &p_rlErrorCode);
				ReadState(p_DeviceHandle, p_usNodeId, VelocityIs, PositionIs, CurrentIs, lErrorCode);
				outputFile << float(float(VelocityIs) / float(gear_ratio)) << "," << PositionIs << "," << CurrentIs << "," << time(nullptr) << endl;
				usleep(g_logInterval * MICRO_TO_SEC);
			}
		}

		if (lResult == MMC_SUCCESS)
		{
			LogInfo("Halt position movement");

			if (VCS_HaltPositionMovement(p_DeviceHandle, p_usNodeId, &p_rlErrorCode) == 0)
			{
				lResult = MMC_FAILED;
				LogError("VCS_HaltPositionMovement", lResult, p_rlErrorCode);
			}
		}
	}
	return lResult;
}

int StartMotor(unsigned int *p_pErrorCode, string opt, long double mode_val, unsigned int pos_vel)
{
	int lResult = MMC_SUCCESS;
	unsigned int lErrorCode = 0;

	if (opt.compare("vel") == 0)
	{
		lResult = DriveVelocityMode(g_pKeyHandle, g_usNodeId, lErrorCode, mode_val);
	}
	if (opt.compare("pos") == 0)
	{
		lResult = DrivePositionMode(g_pKeyHandle, g_usNodeId, lErrorCode, mode_val, pos_vel);
	}
	if (lResult != MMC_SUCCESS)
	{
		LogError("StartMotor", lResult, lErrorCode);
	}
	else
	{
		if (VCS_SetDisableState(g_pKeyHandle, g_usNodeId, &lErrorCode) == 0)
		{
			LogError("VCS_SetDisableState", lResult, lErrorCode);
			lResult = MMC_FAILED;
		}
	}
	return lResult;
}

void exit_callback_handler(int signum)
{
	if (g_opt.compare("vel") == 0)
	{
		int lResult = MMC_SUCCESS;
		stringstream msg;
		unsigned int lErrorCode = 0;

		msg << "Killing velocity mode" << endl
			<< "Node I.D. = " << g_usNodeId;
		LogInfo(msg.str());
		SeparatorLine();

		if (lResult == MMC_SUCCESS)
		{
			LogInfo("Halt velocity movement");

			if (VCS_HaltVelocityMovement(g_pKeyHandle, g_usNodeId, &lErrorCode) == 0)
			{
				LogError("VCS_HaltVelocityMovement", lResult, lErrorCode);
				lResult = MMC_FAILED;
			}

			if (VCS_SetDisableState(g_pKeyHandle, g_usNodeId, &lErrorCode) == 0)
			{
				LogError("VCS_SetDisableState", lResult, lErrorCode);
				lResult = MMC_FAILED;
			}
		}
		exit(signum);
	}
}

// Main

int main(int argc, char **argv)
{
	config cfg = ParseCommand(argv);

	if (mkdir(directoryName.c_str(), 0777) == -1)
	{
		cout << "Directory already exists" << endl;
	}
	else
	{
		cout << "Directory created" << endl;
	}

	time_t t = time(0);
	struct tm *now = localtime(&t);
	char time_buffer[100];
	strftime(time_buffer, 100, "%Y_%m_%d_%H_%M_%S", now);
	string fileTime(time_buffer);
	destinationName = directoryName + "/" + fileName + "_" + fileTime + ".csv";
	outputFile.open(destinationName);
	outputFile << "Velocity"
			   << ","
			   << "Position"
			   << ","
			   << "Current"
			   << ","
			   << "Time"
			   << "," << endl;

	int lResult = MMC_FAILED;
	unsigned int ulErrorCode = 0;
	string opt = cfg.arg1;
	g_opt = opt;
	size_t sz;
	long double modeVal = stold(cfg.arg2);
	unsigned int posVel = 36; // setting default velocity profile to 36 RPM

	if (!cfg.arg3.empty())
	{
		if (opt.compare("vel") == 0)
		{
			g_logInterval = stof(cfg.arg3);
		}
		else
		{
			posVel = stoul(cfg.arg3);
		}
	}
	if (!cfg.arg4.empty())
	{
		if (opt.compare("pos") == 0)
		{
			g_logInterval = stof(cfg.arg4);
		}
	}

	PrintHeader();

	SetDefaultParameters();

	PrintSettings();

	signal(SIGINT, exit_callback_handler); // defining interrupt signal

	if ((lResult = OpenDevice(&ulErrorCode)) != MMC_SUCCESS)
	{
		LogError("OpenDevice", lResult, ulErrorCode);
		return lResult;
	}

	if ((lResult = Setup(&ulErrorCode)) != MMC_SUCCESS)
	{
		LogError("Setup", lResult, ulErrorCode);
		return lResult;
	}

	if ((lResult = StartMotor(&ulErrorCode, opt, modeVal, posVel)) != MMC_SUCCESS)
	{
		LogError("StartMotor", lResult, ulErrorCode);
		return lResult;
	}

	if ((lResult = CloseDevice(&ulErrorCode)) != MMC_SUCCESS)
	{
		LogError("CloseDevice", lResult, ulErrorCode);
		return lResult;
	}
	outputFile.close();
	return lResult;
}
