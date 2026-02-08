#pragma once

#include <Windows.h>
#include <string>
#include <vector>

#define GET_ARRAY_ELEMENT(array, index) (((Il2CppObject**)((char*)(array) + 0x20))[index])

namespace Resolver {
    namespace Protection {
        template<typename Func>
        inline bool safe_call(Func&& fn) {
            __try {
                fn();
                return true;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                return false;
            }
        }

        inline bool IsValidIl2CppObject(void* ptr) {
            if (!ptr) return false;
            uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
            if (addr < 0x10000 || addr > 0x7FFFFFFFFFFF) return false;

            __try {
                void* klass = *(void**)ptr;
                return klass != nullptr;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                return false;
            }
        }

        Il2CppObject* SafeRuntimeInvoke(const MethodInfo* method, Il2CppObject* obj, void** params = nullptr);

        template<typename T>
        T SafeUnbox(Il2CppObject* obj, T defaultValue = T()) {
            if (!obj || (uintptr_t)obj < 0x10000) return defaultValue;
            return *(T*)il2cpp_object_unbox(obj);
        }

        bool IsAlive(Il2CppObject* obj);
    }

	Il2CppClass* GetClass(const char* namespaze, const char* name);
	Il2CppClass* FindClass(const char* namespaze, const char* name);

    std::vector<Il2CppObject*> FindObjectsByType(Il2CppClass* targetClass);

	void GetFieldValue(Il2CppObject* obj, FieldInfo* field, void* outValue);

    template <typename T>
    inline T GetProperty(Il2CppObject* instance, const char* propertyName) {
        if (!Resolver::Protection::IsValidIl2CppObject(instance)) return T();

        Il2CppClass* klass = il2cpp_object_get_class(instance);
        std::string getterName = std::string("get_") + propertyName;
        const MethodInfo* getter = il2cpp_class_get_method_from_name(klass, getterName.c_str(), 0);

        if (!getter) return T();

        Il2CppObject* result = Resolver::Protection::SafeRuntimeInvoke(getter, instance, nullptr);
        if (!result) return T();

        if constexpr (std::is_pointer_v<T>) {
            return reinterpret_cast<T>(result);
        }
        else {
            void* unboxed = il2cpp_object_unbox(result);
            if (!unboxed) return T();
            return *reinterpret_cast<T*>(unboxed);
        }
    }


    template<typename T>
    inline void SetProperty(Il2CppObject* instance, const char* propertyName, T value)
    {
        if (!Resolver::Protection::IsValidIl2CppObject(instance)) return;

        Il2CppClass* klass = il2cpp_object_get_class(instance);
        std::string setterName = std::string("set_") + propertyName;
        const MethodInfo* setter = il2cpp_class_get_method_from_name(klass, setterName.c_str(), 1);

        if (!setter) return;

        Il2CppClass* valKlass = nullptr;
        if constexpr (std::is_same_v<T, app::Vector3>) valKlass = FindClass("UnityEngine", "Vector3");
        else if constexpr (std::is_same_v<T, float>) valKlass = FindClass("System", "Single");

        void* boxedVal = il2cpp_value_box(valKlass, &value);
        void* params[] = { boxedVal };
        Resolver::Protection::SafeRuntimeInvoke(setter, instance, params);
    }

	namespace Helpers {
		void OpenURL(const char* url);
		std::string GetSceneName(Il2CppObject* gameObject);
        void CopyToClipboard(const char* text);
	}
}