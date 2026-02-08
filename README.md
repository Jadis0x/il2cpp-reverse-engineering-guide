
[![GitHub stars](https://img.shields.io/github/stars/jadis0x/il2cpp-reverse-engineering-guide.svg?style=flat&label=Stars&color=ffcc66)](https://github.com/jadis0x/il2cpp-reverse-engineering-guide/stargazers)
[![GitHub contributors](https://img.shields.io/github/contributors/jadis0x/il2cpp-reverse-engineering-guide.svg)](https://github.com/jadis0x/il2cpp-reverse-engineering-guide/graphs/contributors)
[![License](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)

<h3 align="center">Il2CppInspector: C++ Scaffold Guide</h3>

### Introduction
IL2CPP (Intermediate Language to C++) is a scripting backend developed by the Unity game engine that compiles managed .NET code (C#) into native C++ code Ahead of Time (AOT). While this process significantly improves game performance, it makes reverse engineering considerably more complex compared to traditional .NET workflows (e.g., using dnSpy).

### Prerequisites
To get started, you'll need Windows with Visual Studio 2019 (or newer) and a local copy of your target Unity game. This guide assumes you have already generated a C++ scaffold using [Il2CppInspector](https://github.com/jadis0x/Il2CppInspectorPro/releases) and possess a solid understanding of C++, C#, and Unity's architecture.

### Under the Hood: AOT & Method Mapping
When a project hits the IL2CPP backend, the C# logic is flattened. Every class becomes a C++ struct, and every method is turned into a standalone C++ function.

**The most important thing to remember:** Instance methods always take an extra first parameter. This is the this pointer (often called ```__this``` or instance in the scaffold), representing the object the method is acting on. For example, a C# ```Vector3.ToString()``` isn't a method anymore; it's a global function like ```Vector3_ToString_m2315(Vector3* __this, MethodInfo* method)```.

### Metadata System (global-metadata.dat)
The compiled C++ code does not directly contain original C# class names, variable names, or method signatures. This information is stored in a binary file named global-metadata.dat located in the application's data folder. When the IL2CPP runtime starts (il2cpp_init), this file is loaded into memory. IL2CPP API functions (e.g., il2cpp_class_from_name) map the C++ structures in memory to the names in this metadata file. If this metadata file is corrupted or encrypted, API functions will be unable to locate classes by name.   

### Fundamental Data Structures

|Data|Description and Role|
|-|------|
|Il2CppDomain|Represents the application's AppDomain. All assemblies and threads are attached to this domain. Usually, there is only one domain.|
|Il2CppAssembly|The in-memory representation of compiled units like Assembly-CSharp.dll or UnityEngine.dll. Contains an Il2CppImage pointer.|
|Il2CppImage|Hosts the type (class) definitions within an assembly. Class lookup operations usually start from an Image.|
|Il2CppClass|The runtime representation of a C# class. It acts as the "blueprint" in memory, containing the VTable (virtual method table), field offsets, static data, and parent/interface links. In reverse engineering, you use this structure to resolve methods and access fields.|
|MethodInfo|Holds the method's signature, return type, parameters, and most importantly, the memory address of the compiled C++ function (methodPointer).|
|FieldInfo|Specifies the variable's type, name, and memory offset within the object.|
|Il2CppObject|The header for any object on the Managed Heap. All C# objects (reference types) are considered to derive from this structure.|
|Il2CppArray|The counterpart to C# arrays (Array). Contains array bounds and the memory region (vector) where elements reside.|
|Il2CppString|Managed representation of System.String. Contains an Il2CppObject header, string length, and a UTF-16 character buffer.|

### Initialization and Domain Management
Before interacting with the application, one must check the state of the IL2CPP runtime and obtain the main entry point (Domain).

###### il2cpp_domain_get
- Signature: ```Il2CppDomain* il2cpp_domain_get()```
  - It is the starting point for retrieving the assembly list, attaching threads, or performing global operations.

###### il2cpp_init / il2cpp_shutdown
- Signature: ```void il2cpp_init(const char* domain_name) / void il2cpp_shutdown()```
  - Usually called automatically by the Unity Player at game launch. Calling these during external injection is unnecessary and can be dangerous as it may corrupt the current state.

#### Thread Management
Since IL2CPP has a Garbage Collector (GC), every native thread interacting with managed objects must be known to the GC. Otherwise, the GC cannot scan that thread's stack, potentially leading to accidental deletion of used objects or thread crashes.

###### il2cpp_thread_attach
- Signature: ```Il2CppThread* il2cpp_thread_attach(Il2CppDomain* domain)```
  -  Registers the current native thread (e.g., one created by the OS) with the IL2CPP runtime.
  -  When injecting an external DLL (e.g., via CreateRemoteThread), your code runs on a thread not created by Unity. You must call this function before invoking any IL2CPP API functions. Failure to do so results in "Access Violation" or immediate crashes.

###### il2cpp_thread_detach
- Signature: ```void il2cpp_thread_detach(Il2CppThread* thread)```
  -  Detaches the thread from the IL2CPP system.

#### Assembly and Class Discovery
Accessing a class requires a hierarchical search: Domain -> Assembly -> Image -> Class.

- ```il2cpp_domain_get_assemblies```
  - **Signature:** ```const Il2CppAssembly** il2cpp_domain_get_assemblies(const Il2CppDomain* domain, size_t* size)```
  -  **Description:** Returns a list of all loaded assemblies as an array of pointers.
  -  Typically used in a loop to get the image via il2cpp_assembly_get_image and locate the library containing the desired class.

- ```il2cpp_class_from_name```
  - **Signature:** ```Il2CppClass* il2cpp_class_from_name(const Il2CppImage* image, const char* namespaze, const char* name)```
  -  **Description:** Finds a class with the specified namespace and name.

- ```il2cpp_class_get_methods```
  - **Signature:** ```const MethodInfo* il2cpp_class_get_methods(Il2CppClass* klass, void** iter)```
  -  **Description:** Returns all methods in a class sequentially (Iterator pattern).

  **Example:**
  
  ```cpp
  void* iter = NULL;
  const MethodInfo* method = NULL;
  while ((method = il2cpp_class_get_methods(klass, &iter))) {
    printf("Method: %s\n", il2cpp_method_get_name(method));
  }
  ```

#### Method Manipulation and Invocation
The core of IL2CPP modding is the ability to trigger any C# method whenever you want. Whether it's forcing a "GiveGold" function or manually triggering a "SaveGame" event, il2cpp_runtime_invoke is your primary tool.

- ```il2cpp_class_get_method_from_name```
  - **Signature:** ```const MethodInfo* il2cpp_class_get_method_from_name(Il2CppClass* klass, const char* name, int argsCount)```
  -  **Description:** Finds a specific method by name and parameter count.
  - **Important Note:** Method overloading is common in C#. This function only checks the parameter count, not the types. If two methods have the same name and parameter count, the return value is ambiguous or it returns the first match. In such cases, iterating with ```il2cpp_class_get_methods``` to check parameter types is safer.

- ```il2cpp_runtime_invoke```
  - **Signature:** ```Il2CppObject* il2cpp_runtime_invoke(const MethodInfo* method, void* obj, void** params, Il2CppException** exc)```
  -  **Description:** Safely invokes the specified method via the virtual machine.
  -  The return value is always Il2CppObject* (boxed); unboxing is required for primitive return types.

- ```il2cpp_resolve_icall```
  - **Signature:** ```void* il2cpp_resolve_icall(const char* name)```
  -  **Description:** Resolves the address of methods marked as "Internal Call", whose bodies are defined in the C++ engine code rather than C#.
  -  Used to access performance-critical engine functions like ```UnityEngine.Object::Instantiate``` or ```UnityEngine.Time::set_timeScale```. This function returns the direct native function signature, not necessarily an Il2CppObject*, making it much faster.

#### Object and Field Access

- ```il2cpp_object_new```
  - **Signature:** ```Il2CppObject* il2cpp_object_new(const Il2CppClass* klass)```
  -  **Description:** Allocates memory for a new object of the specified class on the Managed Heap.
  -  This corresponds only to the allocation part of the C# new ClassName() operator. The constructor (.ctor) is not called automatically. You must manually call the .ctor method using ```il2cpp_runtime_invoke``` after ```il2cpp_object_new``` to make the object usable.

- ```il2cpp_field_get_value / il2cpp_field_set_value```
  - **Signature:** 
    - ```void il2cpp_field_get_value(Il2CppObject* obj, FieldInfo* field, void* value)```
    - ```void il2cpp_field_set_value(Il2CppObject* obj, FieldInfo* field, void* value)```
  -  **Description:** Reads or writes the value of a field on an object instance.
  - **Example:** To read an int field: int result; ```il2cpp_field_get_value(obj, field, &result);```

#### Generic and Array Operations

- ```il2cpp_array_new```
  - **Signature:** ```Il2CppArray* il2cpp_array_new(Il2CppClass* elementTypeInfo, il2cpp_array_size_t length)```
  -  **Description:**  Creates a single-dimensional array (SZArray) of the specified type and length.
  - Used when passing an array parameter to C# methods. For example, to pass data to a method expecting an object, create the array with this function first, then populate elements using ```il2cpp_array_get_elements``` or direct memory access.

- ```il2cpp_value_box```
  - **Signature:** ```Il2CppObject* il2cpp_value_box(Il2CppClass* klass, void* data)```
  -  **Description:**  Converts a value type (```struct, int, enum```) into a reference type (```System.Object```) (Boxing).

#### Garbage Collection (GC) Control
- ```il2cpp_gc_disable / il2cpp_gc_enable```
  - **Signature:** ```void il2cpp_gc_disable() / void il2cpp_gc_enable()```
  -  **Description:** Temporarily disables or re-enables the Garbage Collector.
  - Used to prevent GC interruptions in critical, real-time code blocks.
  - If too much memory allocation occurs while GC is disabled, unused memory cannot be cleared, causing rapid memory growth and potential termination by the OS (Out Of Memory).   

### Practical Implementation Guide and Example Scenarios
The following scenarios demonstrate how to combine IL2CPP API functions in real-world contexts. Code examples are in C++ and assume a DLL Injection context.

### Scenario 1: Basic Injection and "Debug.Log" Call
This is the "Hello World" of the IL2CPP world. The goal is to print a message to the game console (or log file) from the outside.

```cpp
#include "il2cpp-api-functions.h"
#include <iostream>
#include <string>

// String conversion helper (Platform dependent, simplified here)
Il2CppString* CreateIl2CppString(const char* str) {
    return il2cpp_string_new(str);
}

void HelloWorldExample() {
    // Attach to IL2CPP GC before API calls
	/*
	* This example is self-contained. 
	* In a real project, call attach once at your thread's entry point, as shown in the following scenarios.
	*/
    Il2CppDomain* domain = il2cpp_domain_get();
    Il2CppThread* thread = il2cpp_thread_attach(domain);

    // Resolve CoreModule image
    size_t assemblyCount = 0;
    const Il2CppAssembly** assemblies = il2cpp_domain_get_assemblies(domain, &assemblyCount);
    const Il2CppImage* coreImage = nullptr;

    for (size_t i = 0; i < assemblyCount; ++i) {
        const char* name = il2cpp_image_get_name(il2cpp_assembly_get_image(assemblies[i]));
        // In modern Unity versions, base classes are in "UnityEngine.CoreModule.dll"
        if (std::string(name).find("UnityEngine.CoreModule") != std::string::npos) {
            coreImage = il2cpp_assembly_get_image(assemblies[i]);
            break;
        }
    }

    if (!coreImage) {
        std::cout << "[Error] UnityEngine.CoreModule not found." << std::endl;
        return;
    }

    // 3. Find Class: UnityEngine.Debug
    Il2CppClass* debugClass = il2cpp_class_from_name(coreImage, "UnityEngine", "Debug");
    if (!debugClass) {
        std::cout << "[Error] Debug class not found." << std::endl;
        return;
    }

    // 4. Find Method: Log(object message)
    // "Log" has multiple overloads. We are looking for the one with 1 parameter.
    const MethodInfo* logMethod = il2cpp_class_get_method_from_name(debugClass, "Log", 1);
    if (!logMethod) {
        std::cout << "[Error] Log method not found." << std::endl;
        return;
    }

    // 5. Parameter Preparation and Call
    Il2CppString* message = CreateIl2CppString("[C++] Hello Unity World!");
    
    // Runtime Invoke expects parameters as a void* array
    void* params[] = { message };
    Il2CppException* exception = nullptr;

    il2cpp_runtime_invoke(logMethod, nullptr, params, &exception); // Static method -> obj = nullptr

    if (exception) {
        std::cout << "[Error] Exception occurred during Log call." << std::endl;
    } else {
        std::cout << " Message sent to Unity console." << std::endl;
    }
    
    // Thread detach is usually done when the app closes or the thread terminates
    // il2cpp_thread_detach(thread);
}
```

### Scenario 2: The Master IL2CPP Helper Class
In Scenario 1, we wrote nearly 50 lines of code just to log a message. That approach is not scalable for complex mods. We need a robust, reusable toolset that handles:

Below is the IL2CPPHelper, a header-only library designed to make IL2CPP interaction as simple as writing standard C++.

```cpp
/**
 * @brief Direct Memory Access to Managed Array Elements.
 * * In 64-bit IL2CPP, an Il2CppArray has a header of 32 bytes (0x20). 
 * This includes the base Il2CppObject (16 bytes) and array-specific bounds/length metadata (16 bytes).
 * After this header, the raw pointers to the elements begin.
 * * @warning This macro only works for REFERENCE TYPES (objects). 
 * If the array contains value types (int, float, structs), this will return garbage or crash.
 * @param array Pointer to the Il2CppArray.
 * @param index The zero-based index of the element.
 */
#define GET_ARRAY_ELEMENT(array, index) (((Il2CppObject**)((char*)(array) + 0x20))[index])

class IL2CPPHelper {
public:

    /**
     * @brief Finds a loaded assembly image (DLL) by its filename.
     */
    static const Il2CppImage* GetImage(const char* assemblyName) {
        // Obtain the application's main AppDomain (the starting point)
        Il2CppDomain* domain = il2cpp_domain_get();
        size_t count = 0;
        
        // Retrieve a list of all loaded assemblies (DLLs) in the current domain
        const Il2CppAssembly** assemblies = il2cpp_domain_get_assemblies(domain, &count);

        for (size_t i = 0; i < count; ++i) {
            // Extract the metadata image from the assembly
            const Il2CppImage* img = il2cpp_assembly_get_image(assemblies[i]);
            const char* imgName = il2cpp_image_get_name(img);
            
            // Check if the image name matches our target DLL
            if (imgName && strcmp(imgName, assemblyName) == 0) {
                return img;
            }
        }
        return nullptr;
    }

    /**
     * @brief Resolves a class pointer based on its namespace and name.
     */
    static Il2CppClass* GetClass(const char* assembly, const char* ns, const char* name) {
        // First, locate the specific DLL (Image) containing the class
        const Il2CppImage* img = GetImage(assembly);
        if (!img) return nullptr;

        // Fetch the class structure (vtable, fields, etc.) from metadata
        Il2CppClass* klass = il2cpp_class_from_name(img, ns, name);
        if (!klass) {
            std::cout << "[IL2CPP] GetClass Failed: '" << ns << "." << name << "' not found in " << assembly << std::endl;
        }
        return klass;
    }

    /**
     * @brief Scans the active scene for an instance of a specific class.
     */
    static Il2CppObject* FindInstance(const char* assembly, const char* ns, const char* className) {
        // 1. Resolve the metadata for the target class
        Il2CppClass* targetClass = GetClass(assembly, ns, className);
        if (!targetClass) return nullptr;

        // 2. Find Unity's base "Object" class (where FindObjectsOfType resides)
        Il2CppClass* unityObjClass = GetClass("UnityEngine.CoreModule.dll", "UnityEngine", "Object");
        if (!unityObjClass) unityObjClass = GetClass("UnityEngine.dll", "UnityEngine", "Object");
        if (!unityObjClass) return nullptr;

        // 3. Resolve the "FindObjectsOfType" method by name and parameter count
        const MethodInfo* findMethod = il2cpp_class_get_method_from_name(unityObjClass, "FindObjectsOfType", 1);
        if (!findMethod) return nullptr;

        // 4. Convert our C++ class pointer into a managed "System.Type" object for Unity
        auto typeObj = il2cpp_type_get_object(il2cpp_class_get_type(targetClass));

        void* params[] = { typeObj }; // List of arguments for the method
        Il2CppException* ex = nullptr;

        // 5. Invoke the method; results are returned as a managed array (Il2CppArray)
        Il2CppArray* results = (Il2CppArray*)il2cpp_runtime_invoke(findMethod, nullptr, params, &ex);

        if (ex || !results) return nullptr;

        // 6. If the array has elements, return the first one (index 0)
        if (il2cpp_array_length(results) > 0) {
            return GET_ARRAY_ELEMENT(results, 0);
        }

        return nullptr;
    }

    /**
     * @brief Creates a managed C# string (System.String) on the Unity GC heap.
     */
    static Il2CppString* NewString(const char* str) {
        return il2cpp_string_new(str);
    }

    /**
     * @brief Extracts a raw C++ value from a boxed managed object.
     */
    template <typename T>
    static T Unbox(Il2CppObject* obj) {
        if (!obj) return T{}; 

        // il2cpp_object_unbox returns the memory address of the raw data inside the object.
        // We cast that address to T* and dereference it to get the value.
        return *(T*)il2cpp_object_unbox(obj);
    }

    /**
     * @brief Invokes a method and automatically unboxes the result for value types.
     */
    template <typename T = void*>
    static T Invoke(Il2CppObject* instance, Il2CppClass* klass, const char* methodName, int argCount, void** params) {
        if (!klass) return T{};

        // Find the method within the class metadata
        const MethodInfo* method = il2cpp_class_get_method_from_name(klass, methodName, argCount);
        if (!method) {
            std::cout << "[IL2CPP] Method not found: " << methodName << std::endl;
            return T{};
        }

        Il2CppException* ex = nullptr;
        // Execute the method within the IL2CPP virtual machine
        Il2CppObject* result = il2cpp_runtime_invoke(method, instance, params, &ex);

        if (ex) {
            std::cout << "[IL2CPP] Exception in " << methodName << std::endl;
            return T{};
        }

        if (!result) return T{};

        // If T is a pointer (Reference Type), just cast and return
        if constexpr (std::is_pointer_v<T>) {
            return (T)result; 
        }
        else {
            // If T is a value type (int, bool, struct), unbox the data
            return Unbox<T>(result); 
        }
    }

    /**
     * @brief Sets a field value, automatically handling both static and instance fields.
     */
    template <typename T>
    static void SetField(Il2CppObject* instance, Il2CppClass* klass, const char* fieldName, T value) {
        if (!klass) return;

        // Resolve the field by name
        FieldInfo* field = il2cpp_class_get_field_from_name(klass, fieldName);
        if (!field) return;

        // Check if the field is STATIC (using the 0x0010 attribute mask)
        if (field->type->attrs & 0x0010) {
            // Static fields don't need an instance; set the value directly at the address
            il2cpp_field_static_set_value(field, &value);
        }
        else if (instance) {
            // For instance fields, check if we are passing a pointer or a raw value
            if constexpr (std::is_pointer_v<T>) {
                il2cpp_field_set_value(instance, field, (void*)value);
            }
            else {
                il2cpp_field_set_value(instance, field, &value);
            }
        }
    }

    /**
     * @brief Reads a field value from an instance or a static class.
     */
    template <typename T>
    static T GetField(Il2CppObject* instance, Il2CppClass* klass, const char* fieldName) {
        if (!klass) return T{};

        FieldInfo* field = il2cpp_class_get_field_from_name(klass, fieldName);
        if (!field) {
            std::cout << "[IL2CPP] Field not found: " << fieldName << std::endl;
            return T{};
        }

        T buffer{}; // Temporary buffer to store the copied value

        if (instance) {
            // Read value from an object instance
            il2cpp_field_get_value(instance, field, &buffer);
        }
        else {
            // Read value from a static field
            il2cpp_field_static_get_value(field, &buffer);
        }

        return buffer;
    }
};
```

#### Usage Demonstration
Here is how the spaghetti code from Scenario 1 looks when using the Helper class:

```cpp
void CleanHelloWorld() {
    // Prepare Argument
    Il2CppString* msg = IL2CPPHelper::NewString("[C++] Hello via Helper!");
    void* args[] = { msg };

    // One-Line Invoke
    // We call UnityEngine.Debug.Log(object message)
    IL2CPPHelper::Invoke(
        nullptr, // Static method
        "UnityEngine.CoreModule.dll", "UnityEngine", "Debug", // Location
        "Log", 1, args // Method
    );
}
```

### Scenario 3: Metadata Inspection (Dumping)
When reverse engineering, understanding the internal structure of a class is crucial. Since IL2CPP's API uses specific iterators for metadata, we can create generic tools to dump all fields, methods, and properties of any given class at runtime.

Below are the generic dumping functions to be added to IL2CPPHelper

```cpp
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
```

**Example:**
```cpp
Il2CppClass* menuClass = IL2CPPHelper::GetClass("Assembly-CSharp.dll", "Horror", "Menu");

if (menuClass) {
	IL2CPPHelper::DumpFields(menuClass);
	IL2CPPHelper::DumpMethods(menuClass);
	IL2CPPHelper::DumpProperties(menuClass);
}
```

**Output:**

```cpp
[DEBUG] Fields: Horror.Menu
--------------------------------------------------
Field: titleText                 | Type: UnityEngine.UI.Text | Offset: 0x38
Field: backgroundMusic           | Type: UnityEngine.AudioSource | Offset: 0x40
Field: postProcessVolume         | Type: UnityEngine.Rendering.PostProcessing.PostProcessVolume | Offset: 0x48
Field: mainCamera                | Type: UnityEngine.GameObject | Offset: 0x50
Field: uiCamera                  | Type: UnityEngine.Camera | Offset: 0x58
Field: steamLobbyID              | Type: Steamworks.CSteamID | Offset: 0xFF0
Field: steamName                 | Type: System.String   | Offset: 0xFF8
Field: steamID                   | Type: System.String   | Offset: 0x1000
Field: steamAuthTicket           | Type: Steamworks.HAuthTicket | Offset: 0x1008
Field: soloMode                  | Type: System.Boolean  | Offset: 0x100C
Field: gameMode                  | Type: Horror.GameMode | Offset: 0x1010
Field: boltSessionID             | Type: System.String   | Offset: 0x1018
Field: hostSteamID               | Type: Steamworks.CSteamID | Offset: 0x1020
Field: steamInvitePassword       | Type: System.String   | Offset: 0x1028
.
.
[DEBUG] Properties: Horror.Menu
--------------------------------------------------
Property: boltConfig
Property: vrCameraCenterY
Property: vrIsUsingViveControllers
.
.
[DEBUG] Methods: Horror.Menu
--------------------------------------------------
Method: get_boltConfig                 | Params: 0
Method: get_vrCameraCenterY            | Params: 0
Method: set_vrCameraCenterY            | Params: 1
Method: get_vrIsUsingViveControllers   | Params: 0
Method: set_vrIsUsingViveControllers   | Params: 1
Method: Awake                          | Params: 0
Method: Start                          | Params: 0
Method: CombinePaths                   | Params: 2
Method: OnEnable                       | Params: 0
Method: WaitToWarnNoSteamworks         | Params: 0
Method: Update                         | Params: 0
Method: ControllerReselect             | Params: 0
Method: ControllerReselectDelayed      | Params: 0
Method: OnSensitivitySliderChanged     | Params: 1
Method: OnSensitivityInputEndEdit      | Params: 1
Method: OnFPSLimitInputEndEdit         | Params: 1
Method: OnSoloModeButtonClick          | Params: 0
.
.
```

### Scenario 4: Field Manipulation
Once you have identified a field's offset and type via metadata dumping, you can modify its value at runtime. This scenario demonstrates how to update a string field, handling both the search for the object and the specific memory requirements for managed strings.

```cpp
/**
* @brief Overwriting a Steam Name field.
*/
void Scenario4_UpdateField() {
	// Search for the target object in the current scene.
	// Our "Universal Finder" handles both active and inactive objects.
	Il2CppObject* menuInstance = IL2CPPHelper::FindInstance("Assembly-CSharp.dll", "Horror", "Menu");

	if (menuInstance) {
		// Prepare the new value.
		// Allocate System.String on Unity heap
		// Never pass a raw C++ const char* directly to a System.String field.
		Il2CppString* hackedStr = IL2CPPHelper::NewString("testestest");
		Il2CppClass* menuClass = il2cpp_object_get_class(menuInstance);

		// Perform the overwrite.
		// The helper automatically detects if 'steamName' is a static or instance field.
		IL2CPPHelper::SetField(menuInstance, menuClass, "steamName", hackedStr);

		std::cout << "[IL2CPP] Success: Field updated.\n";
	}
	else {
		std::cout << "[IL2CPP] Error: Menu instance not found in scene.\n";
	}
}
```

### Scenario 5: Engine Navigation (Tags, Components, and Vectors)
In this scenario, we use the Unity Engine's internal search to find a specific player and read their real-time coordinates. This demonstrates how to chain method calls across different engine classes.

```cpp
struct Vector3 {
	float x, y, z;
};

void Scenario5_GetPlayerPosition() {
	// Find the GameObject by Tag
	// GameObject.FindWithTag(string tag) is a static method in UnityEngine.CoreModule
	Il2CppString* tagStr = IL2CPPHelper::NewString("Player");
	void* findParams[] = { tagStr };

	Il2CppObject* playerGO = IL2CPPHelper::Invoke<Il2CppObject*>(
		nullptr, // Static call
		"UnityEngine.CoreModule.dll", "UnityEngine", "GameObject",
		"FindWithTag", 1, findParams
	);

	if (!playerGO) {
		std::cout << "[IL2CPP] Player with tag 'Player' not found.\n";
		return;
	}

	// Get the Transform component
	// Every GameObject has a 'transform' property, which is actually the 'get_transform' method.
	Il2CppClass* goClass = il2cpp_object_get_class(playerGO);
	Il2CppObject* transform = IL2CPPHelper::Invoke<Il2CppObject*>(playerGO, goClass, "get_transform", 0, nullptr);

	if (!transform) return;

	// Get the Position (Vector3)
	// Transform.get_position() returns a Vector3 struct. 
	// Our Invoke<T> will automatically call Unbox<Vector3> because Vector3 is not a pointer.
	Il2CppClass* transformClass = il2cpp_object_get_class(transform);
	Vector3 pos = IL2CPPHelper::Invoke<Vector3>(transform, transformClass, "get_position", 0, nullptr);

	std::cout << "[IL2CPP] Player Position -> X: " << pos.x
		<< ", Y: " << pos.y
		<< ", Z: " << pos.z << "\n";
}
```

### Scenario 6: Runtime Component Injection & State Management

```cpp
/**
* @brief Finds the Player by tag and ensures a specific component is active.
* @param enable Whether to add/enable the target component.
*/
void Scenario6_AttachHandlerToPlayer(bool enable) {
	// FIND THE PLAYER: GameObject.FindWithTag("Player")
	// This is a static method in the GameObject class.
	Il2CppString* tagStr = IL2CPPHelper::NewString("Player");
	void* findParams[] = { tagStr };

	Il2CppObject* playerGO = IL2CPPHelper::Invoke<Il2CppObject*>(
		nullptr, // Static call
		"UnityEngine.CoreModule.dll", "UnityEngine", "GameObject",
		"FindWithTag", 1, findParams
	);

	if (!playerGO) {
		std::cout << "[IL2CPP] Player tag not found in current scene.\n";
		return;
	}

	// RESOLVE COMPONENT TYPE: Opsive.UltimateCharacterController.Character.UltimateCharacterLocomotionHandler
	// NOTE: Namespaces and class names are passed separately in IL2CPP API
	Il2CppClass* handlerClass = IL2CPPHelper::GetClass(
		"Opsive.UltimateCharacterController.dll",
		"Opsive.UltimateCharacterController.Character",
		"UltimateCharacterLocomotionHandler"
	);

	if (!handlerClass) return;
	auto handlerTypeObj = il2cpp_type_get_object(il2cpp_class_get_type(handlerClass));

	// GET OR ADD COMPONENT
	Il2CppClass* goClass = il2cpp_object_get_class(playerGO);
	void* typeParams[] = { handlerTypeObj };

	// Check if player already has the component
	Il2CppObject* component = IL2CPPHelper::Invoke<Il2CppObject*>(
		playerGO, goClass, "GetComponent", 1, typeParams
	);

	// If missing and we want to enable, inject it
	if (!component && enable) {
		std::cout << "[IL2CPP] Player found. Injecting Locomotion Handler...\n";
		component = IL2CPPHelper::Invoke<Il2CppObject*>(
			playerGO, goClass, "AddComponent", 1, typeParams
		);
	}

	// TOGGLE STATE: set_enabled(bool)
	if (component) {
		Il2CppClass* compClass = il2cpp_object_get_class(component);
		// TRICK: IL2CPP runtime_invoke expects a pointer to the value for primitives (Value Types).
		// Passing 'enable' directly would fail; we must pass '&enable'.
		void* enableParams[] = { &enable }; // Address-of is required for value types

		IL2CPPHelper::Invoke(component, compClass, "set_enabled", 1, enableParams);
		std::cout << "[IL2CPP] Player Logic State: " << (enable ? "ON" : "OFF") << "\n";
	}
}
```

### Disclaimer
This guide is for educational and research purposes only. The author is not responsible for any misuse, bans, or legal issues resulting from the use of these techniques. Use these tools ethically and respect the terms of service of the software you analyze.
