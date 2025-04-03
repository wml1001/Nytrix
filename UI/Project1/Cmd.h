#pragma once
//Cmd.h
#include "AuthenticationManager.h"
#include<thread>

void ProcessCommand(char* buf, AuthenticationManager* authenticationManager);

void waitMyCmdReturn(AuthenticationManager* authenticationManager);
