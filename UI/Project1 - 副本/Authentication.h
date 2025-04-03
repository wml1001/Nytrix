#pragma once
//Authentication.h
#include "imGUI/imgui.h"
#include "AuthenticationManager.h"

namespace Authentication {
    static const int bufSize = 16;

	bool authentication(AuthenticationManager* authenticationManager);
}