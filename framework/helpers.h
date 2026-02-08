// Generated C++ file by Il2CppInspectorPro - https://github.com/jadis0x
// Helper functions

#pragma once

#include <string>
#include <sstream>
#include <iomanip>

#include "il2cpp-metadata-version.h"

// Helper function to get the module base address
uintptr_t il2cppi_get_base_address();

// Helper function to append text to a file
void il2cppi_log_write(std::string text);

void LogError(const char* msg, bool showBox = false);

// Helper function to open a new console window and redirect stdout there
void il2cppi_new_console();

#if _MSC_VER >= 1920
// Helper function to convert Il2CppString to std::string
std::string il2cppi_to_string(Il2CppString* str);

// Helper function to convert System.String to std::string
std::string il2cppi_to_string(app::String* str);

app::String* convert_to_system_string(const char* str);

std::string ToString(app::Object* Object);
app::String* ToString(app::Object_1* Object);
#endif

// Helper function to check if a metadata usage pointer is initialized
template<typename T> bool il2cppi_is_initialized(T* metadataItem) {
#if __IL2CPP_METADATA_VERISON < 270
 return *metadataItem != 0;
#else
 // Metadata >=27 (Unity 2020.2)
 return !((uintptr_t)*metadataItem & 1);
#endif
}

// Helper function to convert a pointer to hex
template<typename T> std::string to_hex_string(T i) {
 std::stringstream stream;
 stream << "0x" << std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex << i;
 return stream.str();
}