#include "pch-il2cpp.h"

#include "Il2CppResolver.h"
#include "helpers.h"

namespace Resolver::Protection {
	Il2CppObject* SafeRuntimeInvoke(const MethodInfo* method, Il2CppObject* obj, void** params)
	{
		if (!method || (!obj && !(method->flags & 0x0010))) return nullptr;
		Il2CppObject* result = nullptr;
		Il2CppException* exc = nullptr;
		bool success = Protection::safe_call([&]() {
			result = il2cpp_runtime_invoke(method, obj, params, &exc);
			});
		return (success && !exc) ? result : nullptr;
	}

	bool IsAlive(Il2CppObject* obj)
	{
		if (!IsValidIl2CppObject(obj)) return false;
		static Il2CppClass* objectClass = Resolver::FindClass("UnityEngine", "Object");
		static const MethodInfo* op_Implicit = il2cpp_class_get_method_from_name(objectClass, "op_Implicit", 1);
		void* params[] = { obj };
		Il2CppObject* result = SafeRuntimeInvoke(op_Implicit, obj, params);
		return result && *(bool*)il2cpp_object_unbox(result);
	}
}

namespace Resolver {
	Il2CppClass* GetClass(const char* namespaze, const char* name)
	{
		auto domain = il2cpp_domain_get();
		size_t size;
		const Il2CppAssembly** assemblies = il2cpp_domain_get_assemblies(domain, &size);
		for (size_t i = 0; i < size; ++i) {
			auto image = il2cpp_assembly_get_image(assemblies[i]);
			auto klass = il2cpp_class_from_name(image, namespaze, name);
			if (klass) return klass;
		}
		return nullptr;
	}


	Il2CppClass* FindClass(const char* namespaze, const char* name)
	{
		auto domain = il2cpp_domain_get();
		size_t size;
		const Il2CppAssembly** assemblies = il2cpp_domain_get_assemblies(domain, &size);

		for (size_t i = 0; i < size; ++i) {
			auto image = il2cpp_assembly_get_image(assemblies[i]);
			auto klass = il2cpp_class_from_name(image, namespaze, name);
			if (klass) return klass;
		}
		return nullptr;
	}

	std::vector<Il2CppObject*> FindObjectsByType(Il2CppClass* targetClass)
	{
		std::vector<Il2CppObject*> foundObjects;
		if (!targetClass) return foundObjects;

		static Il2CppClass* unityObjectClass = Resolver::FindClass("UnityEngine", "Object");
		static const MethodInfo* findMethod = il2cpp_class_get_method_from_name(unityObjectClass, "FindObjectsOfType", 1);

		if (!findMethod) return foundObjects;

		const Il2CppType* type = il2cpp_class_get_type(targetClass);
		Il2CppReflectionType* reflectionType = (Il2CppReflectionType*)il2cpp_type_get_object(type);

		void* params[] = { reflectionType };
		Il2CppArray* results = (Il2CppArray*)Resolver::Protection::SafeRuntimeInvoke(findMethod, nullptr, params);

		if (results) {
			uint32_t count = il2cpp_array_length(results);
			for (uint32_t i = 0; i < count; i++) {
				Il2CppObject* obj = GET_ARRAY_ELEMENT(results, i);
				if (Resolver::Protection::IsAlive(obj)) {
					foundObjects.push_back(obj);
				}
			}
		}
		return foundObjects;
	}

	void GetFieldValue(Il2CppObject* obj, FieldInfo* field, void* outValue)
	{
		if (field->type->attrs & 0x0010) {
			il2cpp_field_static_get_value(field, outValue);
		}
		else if (obj) {
			il2cpp_field_get_value(obj, field, outValue);
		}
	}
}


namespace Resolver::Helpers{
	void OpenURL(const char* url) {
		ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
	}

	std::string GetSceneName(Il2CppObject* gameObject) {
		if (!Resolver::Protection::IsAlive(gameObject)) return "Unknown";

		std::string resultName = "Unknown Scene";

		Resolver::Protection::safe_call([&]() {

			static Il2CppClass* goClass = Resolver::FindClass("UnityEngine", "GameObject");
			static const MethodInfo* getScene = goClass ? il2cpp_class_get_method_from_name(goClass, "get_scene", 0) : nullptr;

			if (!getScene) return;

			Il2CppObject* sceneBoxed = Resolver::Protection::SafeRuntimeInvoke(getScene, gameObject, nullptr);

			if (!sceneBoxed) {
				resultName = "DontDestroyOnLoad";
				return;
			}

			Il2CppClass* sceneClass = il2cpp_object_get_class(sceneBoxed);
			static const MethodInfo* getSceneName = sceneClass ? il2cpp_class_get_method_from_name(sceneClass, "get_name", 0) : nullptr;

			if (getSceneName) {
				Il2CppString* sName = (Il2CppString*)Resolver::Protection::SafeRuntimeInvoke(getSceneName, sceneBoxed, nullptr);
				if (sName) {
					resultName = il2cppi_to_string(sName);
				}
			}
			});

		return resultName;
	}

	void Helpers::CopyToClipboard(const char* text)
	{
		if (OpenClipboard(NULL)) {
			EmptyClipboard();
			HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, strlen(text) + 1);
			if (hg) {
				memcpy(GlobalLock(hg), text, strlen(text) + 1);
				GlobalUnlock(hg);
				SetClipboardData(CF_TEXT, hg);
			}
			CloseClipboard();
		}
	}
}

