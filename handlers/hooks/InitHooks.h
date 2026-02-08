#pragma once

void DetourInitilization();
void DetourUninitialization();

// Unity.Debug
void dDebug_Log(app::Object* message, MethodInfo* method);
void dDebug_LogWarning(app::Object* message, MethodInfo* method);
/*
void dDebug_LogError(app::Object* message, MethodInfo* method);
void dDebug_LogException(app::Exception* exception, MethodInfo* method);
*/
