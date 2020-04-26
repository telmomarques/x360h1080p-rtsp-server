/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// Copyright (c) 1996-2020, Live Networks, Inc.  All rights reserved
// A test program that demonstrates how to stream - via unicast RTP
// - various kinds of file on demand, using a built-in RTSP server.
// main program

#include <iostream>
#include <fstream>
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "json.hpp"

using json = nlohmann::json;

UsageEnvironment* env;

// To make the second and subsequent client for each stream reuse the same
// input stream as the first client (rather than playing the file from the
// start for each client), change the following "False" to "True":
Boolean reuseFirstSource = True;

// To stream *only* MPEG-1 or 2 video "I" frames
// (e.g., to reduce network bandwidth),
// change the following "False" to "True":
Boolean iFramesOnly = False;

enum EncodingType {
	H264,
	H265
};

static void announceStream(RTSPServer* rtspServer, ServerMediaSession* sms,
			   char const* streamName, char const* inputFileName); // fwd

bool fileExists(char* filePath) {
  /*if(filePath != NULL) {
    if (FILE *file = fopen(filePath, "r")) {
      fclose(file);
      return true;
    }
  }
  
  return false;*/

  return true;
}

void validateArgs(char* configPath, char* mainstreamPipe, char* substreamPipe) {

  if(!fileExists(configPath)) {
    std::cout << "Configuration file does not exist.\n";
    exit(2);
  }

  if(mainstreamPipe == NULL && substreamPipe == NULL) {
    std::cout << "At least one stream (mainstream/substream) must be specified.\n";
    exit(3);
  }

  if(mainstreamPipe != NULL && !fileExists(mainstreamPipe)) {
    std::cout << "Mainstream input file does not exist.\n";
    exit(4);
  }

  if(substreamPipe != NULL && !fileExists(substreamPipe)) {
    std::cout << "Substream input file does not exist.\n";
    exit(5);
  }
}

void printUsage(char* binaryName) {
  std::cout << "Usage: ";
  std::cout << binaryName;
  std::cout << " -c [CONFIG FILE] -m [MAINSTREAM FILE] -s [SUBSTREAM FILE]\n\n";

  std::cout << "Config file: path to configuration file\n";
  std::cout << "Mainstream file: mainstream input file\n";
  std::cout << "Substream file: substream input file\n";
}

int main(int argc, char** argv) {
  json config;
  OutPacketBuffer::maxSize = 600000;
  int opt;
  char* configPath = (char*)"config.json";
  char* mainstreamPipe = NULL;
  char* substreamPipe = NULL;
  
  while ((opt = getopt (argc, argv, "m:s:c:h")) != -1)
  {
	  switch(opt) {
      case 'c':
        configPath = optarg;
      break;

		  case 'm':
			  mainstreamPipe = optarg;
		  break;
		  
		  case 's':
			  substreamPipe = optarg;
		  break;

      case 'h':
      default:
        printUsage(argv[0]);
        exit(6);
      break;
	  }
  }

  if(argc == 1) {
    printUsage(argv[0]);
    exit(6);
  }

  validateArgs(configPath, mainstreamPipe, substreamPipe);
  
  // Read configuration
  std::ifstream configFile(configPath);
  try {
  configFile >> config;
  } catch(...) {
    std::cout << "Invalid configuration file.\n";
    exit(7);
  }
  
  EncodingType encodingType = EncodingType::H265;
  if(config["encodingType"] == "h264") {
	  encodingType = EncodingType::H264;
  }

  std::cout << "Streaming encoding: ";
  std::cout << config["encodingType"];
  std::cout << "\n";

  // Begin by setting up our usage environment:
  TaskScheduler* scheduler = BasicTaskScheduler::createNew();
  env = BasicUsageEnvironment::createNew(*scheduler);

  UserAuthenticationDatabase* authDB = NULL;
#ifdef ACCESS_CONTROL
  // To implement client access control to the RTSP server, do the following:
  authDB = new UserAuthenticationDatabase;
  authDB->addUserRecord("username1", "password1"); // replace these with real strings
  // Repeat the above with each <username>, <password> that you wish to allow
  // access to the server.
#endif

  // Create the RTSP server:
  RTSPServer* rtspServer = RTSPServer::createNew(*env, 8554, authDB);
  if (rtspServer == NULL) {
    *env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
    exit(1);
  }

  char const* descriptionString = "";
  
  if(encodingType == EncodingType::H265) {
	if(mainstreamPipe != NULL) {
		char const* streamName = "mainstream";
		
		ServerMediaSession* sms = ServerMediaSession::createNew(*env, streamName, streamName,descriptionString);
		sms->addSubsession(H265VideoFileServerMediaSubsession::createNew(*env, mainstreamPipe, reuseFirstSource));
		rtspServer->addServerMediaSession(sms);
		announceStream(rtspServer, sms, streamName, mainstreamPipe);
	}
	
	if(substreamPipe != NULL) {
		char const* streamName = "substream";
		
		ServerMediaSession* sms = ServerMediaSession::createNew(*env, streamName, streamName,descriptionString);
		sms->addSubsession(H265VideoFileServerMediaSubsession::createNew(*env, substreamPipe, reuseFirstSource));
		rtspServer->addServerMediaSession(sms);
		announceStream(rtspServer, sms, streamName, substreamPipe);
	}
  }
  else {
	if(mainstreamPipe != NULL) {
		char const* streamName = "mainstream";
		
		ServerMediaSession* sms = ServerMediaSession::createNew(*env, streamName, streamName, descriptionString);
		sms->addSubsession(H264VideoFileServerMediaSubsession::createNew(*env, mainstreamPipe, reuseFirstSource));
		rtspServer->addServerMediaSession(sms);
		announceStream(rtspServer, sms, streamName, mainstreamPipe);
	}
	
	if(substreamPipe != NULL) {
		char const* streamName = "substream";
		
		ServerMediaSession* sms = ServerMediaSession::createNew(*env, streamName, streamName, descriptionString);
		sms->addSubsession(H264VideoFileServerMediaSubsession::createNew(*env, substreamPipe, reuseFirstSource));
		rtspServer->addServerMediaSession(sms);
		announceStream(rtspServer, sms, streamName, substreamPipe);
	}
  }

  env->taskScheduler().doEventLoop(); // does not return
  return 0; // only to prevent compiler warning
}

static void announceStream(RTSPServer* rtspServer, ServerMediaSession* sms, char const* streamName, char const* inputFileName) {
  char* url = rtspServer->rtspURL(sms);
  UsageEnvironment& env = rtspServer->envir();
  env << "\n\"" << streamName << "\" stream, from the file \""
      << inputFileName << "\"\n";
  env << "Play this stream using the URL \"" << url << "\"\n";
  delete[] url;
}
