// Generated C++ file by Il2CppInspectorPro - https://github.com/jadis0x

#include "pch-il2cpp.h"
#include "main.h"
#include <Windows.h>
#include <iostream>
#include <cstdio>

#include "il2cpp-appdata.h"
#include "il2cpp-init.h"
#include "helpers.h"
#include "hooks/InitHooks.h"

HMODULE hModule;
HANDLE hUnloadEvent;

// Set the name of your log file here
extern const LPCWSTR LOG_FILE = L"Logs.txt";

#define GET_ARRAY_ELEMENT(array, index) (((Il2CppObject**)((char*)(array) + 0x20))[index])

class IL2CPPHelper {
public:

	/**
	 * @brief Finds a loaded assembly image (DLL) by name.
	 * @param assemblyName Name of the DLL (e.g., "Assembly-CSharp.dll").
	 * @return Pointer to Il2CppImage or nullptr if not found.
	 */
	static const Il2CppImage* GetImage(const char* assemblyName) {
		Il2CppDomain* domain = il2cpp_domain_get();
		size_t count = 0;
		const Il2CppAssembly** assemblies = il2cpp_domain_get_assemblies(domain, &count);

		for (size_t i = 0; i < count; ++i) {
			const Il2CppImage* img = il2cpp_assembly_get_image(assemblies[i]);
			const char* imgName = il2cpp_image_get_name(img);
			if (imgName && strcmp(imgName, assemblyName) == 0) {
				return img;
			}
		}
		return nullptr;
	}

	/**
	 * @brief Resolves a class pointer from an assembly.
	 * @param assembly Name of the assembly (e.g., "Assembly-CSharp.dll").
	 * @param ns Namespace of the class (empty string "" if none).
	 * @param name Name of the class (e.g., "NolanRankController").
	 */
	static Il2CppClass* GetClass(const char* assembly, const char* ns, const char* name) {
		const Il2CppImage* img = GetImage(assembly);
		if (!img) return nullptr;

		Il2CppClass* klass = il2cpp_class_from_name(img, ns, name);
		if (!klass) {
			std::cout << "[IL2CPP] GetClass Failed: '" << ns << "." << name << "' not found in " << assembly << std::endl;
		}
		return klass;
	}

	/**
	 * @brief The "Universal Finder".
	 * Scans the active scene for an instance of the target class.
	 * Uses FindObjectsOfType to ensure we catch the object even if standard lookups fail.
	 */
	static Il2CppObject* FindInstance(const char* assembly, const char* ns, const char* className) {
		// 1. Resolve metadata for the target class
		Il2CppClass* targetClass = GetClass(assembly, ns, className);
		if (!targetClass) return nullptr;

		// 2. Locate the base UnityEngine.Object class to access engine-level search functions
		Il2CppClass* unityObjClass = GetClass("UnityEngine.CoreModule.dll", "UnityEngine", "Object");
		if (!unityObjClass) unityObjClass = GetClass("UnityEngine.dll", "UnityEngine", "Object");
		if (!unityObjClass) return nullptr;

		// 3. Resolve the plural "FindObjectsOfType" for maximum reliability
		const MethodInfo* findMethod = il2cpp_class_get_method_from_name(unityObjClass, "FindObjectsOfType", 1);
		if (!findMethod) return nullptr;

		// 4. Convert metadata pointer to a Managed System.Type object required by Unity
		auto typeObj = il2cpp_type_get_object(il2cpp_class_get_type(targetClass));

		void* params[] = { typeObj };
		Il2CppException* ex = nullptr;

		// 5. Invoke the search. Results return as a managed Il2CppArray
		Il2CppArray* results = (Il2CppArray*)il2cpp_runtime_invoke(findMethod, nullptr, params, &ex);

		if (ex || !results) return nullptr;

		// 6. If the array is populated, grab the first element (index 0)
		if (il2cpp_array_length(results) > 0) {
			return GET_ARRAY_ELEMENT(results, 0);
		}

		return nullptr;
	}


	/**
	 * @brief Creates a managed C# string from a C-string.
	 * @note Allocated on the GC heap.
	 */
	static Il2CppString* NewString(const char* str) {
		return il2cpp_string_new(str);
	}

	/**
	 * @brief UNBOXING MAGIC: Extracts raw value from a managed object wrapper.
	 * @tparam T The expected C++ type (int, float, bool, etc.).
	 * @param obj The managed object returned by Invoke.
	 */
	template <typename T>
	static T Unbox(Il2CppObject* obj) {
		if (!obj) return T{}; // Return default (0/false) if null

		// Il2cppObject* for value types is just a wrapper (box) around the data.
		// il2cpp_object_unbox returns the memory address of the actual data.
		// We cast that address to T* and dereference it to get the value.
		return *(T*)il2cpp_object_unbox(obj);
	}


	/**
	 * @brief Calls a method on an object or class (static).
	 * Automatically handles Unboxing for value types!
	 * * @tparam T Return type (e.g., int, Il2CppString*). Defaults to void*.
	 * @param instance Object instance (pass nullptr for STATIC methods).
	 * @param methodName Name of the method to call.
	 * @param argCount Number of arguments.
	 * @param params Array of arguments (void* args[]).
	 */
	template <typename T = void*>
	static T Invoke(Il2CppObject* instance, Il2CppClass* klass, const char* methodName, int argCount, void** params) {
		if (!klass) return T{};

		const MethodInfo* method = il2cpp_class_get_method_from_name(klass, methodName, argCount);
		if (!method) {
			std::cout << "[IL2CPP] Method not found: " << methodName << std::endl;
			return T{};
		}

		Il2CppException* ex = nullptr;
		Il2CppObject* result = il2cpp_runtime_invoke(method, instance, params, &ex);

		if (ex) {
			std::cout << "[IL2CPP] Exception in " << methodName << std::endl;
			return T{};
		}

		if (!result) return T{};

		// Compile-time check: Is T a pointer? (Reference Type) or a Value? (Value Type)
		if constexpr (std::is_pointer_v<T>) {
			return (T)result; // It's already a pointer (String*, Object*), just cast it.
		}
		else {
			return Unbox<T>(result); // It's a boxed value (int, float), unbox it!
		}
	}

	// Overload: Looks up class by name (Slower, handy for one-offs)
	template <typename T = void*>
	static T Invoke(Il2CppObject* instance, const char* asmName, const char* ns, const char* clsName, const char* mtdName, int argsCount, void** params) {
		Il2CppClass* klass = GetClass(asmName, ns, clsName);
		return Invoke<T>(instance, klass, mtdName, argsCount, params);
	}


	/**
	* @brief Sets a field value, automatically handling static or instance fields.
	*/
	template <typename T>
	static void SetField(Il2CppObject* instance, Il2CppClass* klass, const char* fieldName, T value) {
		if (!klass) return;

		FieldInfo* field = il2cpp_class_get_field_from_name(klass, fieldName);
		if (!field) return;

		// Check if the field is STATIC (Attribute 0x0010)
		if (field->type->attrs & 0x0010) {
			// For static fields, instance pointer is ignored
			il2cpp_field_static_set_value(field, &value);
		}
		else if (instance) {
			// For instance fields
			if constexpr (std::is_pointer_v<T>) {
				il2cpp_field_set_value(instance, field, (void*)value);
			}
			else {
				il2cpp_field_set_value(instance, field, &value);
			}
		}
	}

	/**
	 * @brief Gets the value of a field.
	 * @return The value (unboxed if necessary).
	 */
	template <typename T>
	static T GetField(Il2CppObject* instance, Il2CppClass* klass, const char* fieldName) {
		if (!klass) return T{};

		FieldInfo* field = il2cpp_class_get_field_from_name(klass, fieldName);
		if (!field) {
			std::cout << "[IL2CPP] Field not found: " << fieldName << std::endl;
			return T{};
		}

		T buffer{}; // Temp buffer to hold the value

		if (instance) {
			il2cpp_field_get_value(instance, field, &buffer);
		}
		else {
			il2cpp_field_static_get_value(field, &buffer);
		}

		return buffer;
	}


	/**
 * @brief Dumps all field information (name, type, offset) to the console.
 * Use this to map out the memory layout of a class and find where variables live.
 */
	static void DumpFields(Il2CppClass* klass) {
		if (!klass) return;

		std::cout << "\n[DEBUG] Fields: " << il2cpp_class_get_namespace(klass) << "." << il2cpp_class_get_name(klass) << "\n";
		std::cout << "--------------------------------------------------\n";

		void* iter = nullptr;
		// Iterate through all fields in the class metadata
		while (FieldInfo* field = il2cpp_class_get_fields(klass, &iter)) {
			const char* name = il2cpp_field_get_name(field);
			const Il2CppType* type = il2cpp_field_get_type(field);
			const char* typeName = il2cpp_type_get_name(type);
			uint32_t offset = il2cpp_field_get_offset(field);

			// Printing Name, Type, and Hex Offset (Crucial for direct memory reading)
			std::cout << "Field: " << std::left << std::setw(25) << name
				<< " | Type: " << std::left << std::setw(15) << typeName
				<< " | Offset: 0x" << std::hex << std::uppercase << offset << std::dec << "\n";
		}
	}

	/**
	 * @brief Lists all methods and their parameter counts.
	 * Useful for finding the exact method name and argument count for 'Invoke' calls.
	 */
	static void DumpMethods(Il2CppClass* klass) {
		if (!klass) return;

		std::cout << "\n[DEBUG] Methods: " << il2cpp_class_get_namespace(klass) << "." << il2cpp_class_get_name(klass) << "\n";
		std::cout << "--------------------------------------------------\n";

		void* iter = nullptr;
		// Walk through the method table (vtable) of the class
		while (const MethodInfo* method = il2cpp_class_get_methods(klass, &iter)) {
			std::cout << "Method: " << std::left << std::setw(30) << il2cpp_method_get_name(method)
				<< " | Params: " << il2cpp_method_get_param_count(method) << "\n";
		}
	}

	/**
	 * @brief Lists all properties (getters/setters).
	 * Properties in C# are actually special methods; dumping them helps identify state logic.
	 */
	static void DumpProperties(Il2CppClass* klass) {
		if (!klass) return;

		std::cout << "\n[DEBUG] Properties: " << il2cpp_class_get_namespace(klass) << "." << il2cpp_class_get_name(klass) << "\n";
		std::cout << "--------------------------------------------------\n";

		void* iter = nullptr;
		// API requires non-const PropertyInfo* due to internal structure
		while (PropertyInfo* prop = const_cast<PropertyInfo*>(il2cpp_class_get_properties(klass, &iter))) {
			std::cout << "Property: " << il2cpp_property_get_name(prop) << "\n";
		}
	}
};





void Run(LPVOID lpParam)
{
	hModule = static_cast<HMODULE>(lpParam);

#ifdef _DEBUG
	il2cppi_new_console();
	SetConsoleTitleA("Debug Console");
#endif

	il2cppi_log_write("Initializing...");

	init_il2cpp();

	if (!AttachIl2Cpp()) return;

	il2cppi_log_write("IL2CPP Thread Attached Successfully.");

	DetourInitilization();

	hUnloadEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (!hUnloadEvent) {
		LogError("Unload Event could not be created!");
		return;
	}

	while (true)
	{
		if (GetAsyncKeyState(VK_F1) & 0x8000) {
			Il2CppClass* menuClass = IL2CPPHelper::GetClass("Assembly-CSharp.dll", "Horror", "Menu");

			if (menuClass) {
				IL2CPPHelper::DumpFields(menuClass);
				IL2CPPHelper::DumpMethods(menuClass);
				IL2CPPHelper::DumpProperties(menuClass);
			}
		}

		Sleep(200);
	}

	HANDLE hThread = CreateThread(nullptr, 0, UnloadWatcherThread, hUnloadEvent, 0, nullptr);
	if (hThread) {
		CloseHandle(hThread);
	}
	else {
		LogError("Unload Watcher Thread could not be started!");
	}
}

bool AttachIl2Cpp()
{
	Il2CppDomain* domain = il2cpp_domain_get();
	if (!domain) {
		LogError("IL2CPP Domain not found!", true);
		return false;
	}

	Il2CppThread* thread = il2cpp_thread_attach(domain);
	if (!thread) {
		LogError("IL2CPP Thread attach edilemedi!", true);
		return false;
	}
	return true;
}

DWORD WINAPI UnloadWatcherThread(LPVOID lpParam)
{
	HANDLE eventHandle = static_cast<HANDLE>(lpParam);
	if (!eventHandle) return 0;

	if (WaitForSingleObject(eventHandle, INFINITE) == WAIT_OBJECT_0) {
		std::cout << "\n[INFO]  Unload signal received, exiting..." << std::endl;

		DetourUninitialization();

		fclose(stdout);
		FreeConsole();

		if (hUnloadEvent) {
			CloseHandle(hUnloadEvent);
			hUnloadEvent = nullptr;
		}

		FreeLibraryAndExitThread(hModule, 0);
	}
	return 0;
}