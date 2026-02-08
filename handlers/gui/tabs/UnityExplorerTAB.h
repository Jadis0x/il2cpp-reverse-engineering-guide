#pragma once

#include <string>
#include <Windows.h>

namespace UnityExplorerTAB {

	void Render();
	void Reset();

	namespace Helpers {
		void DrawInspector();
		void DrawObjectNode(Il2CppObject* gameObject);
        void InspectObject(Il2CppObject* obj);
		void DrawField(Il2CppObject* obj, FieldInfo* field);
        void DrawProperty(Il2CppObject* obj, const PropertyInfo* prop);
        void DrawMethods(Il2CppObject* obj, Il2CppClass* klass);
		void DrawClassSearch();
		void InspectClass(Il2CppClass* klass); 
		void DrawVector3(const char* label, Il2CppObject* transform, const char* propName);
	}
}