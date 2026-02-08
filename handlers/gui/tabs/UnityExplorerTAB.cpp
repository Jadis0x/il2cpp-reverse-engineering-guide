#include "pch-il2cpp.h"

#define IMGUI_DEFINE_MATH_OPERATORS

#include <iostream>
#include <vector> 
#include <map>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include "gui/tabs/UnityExplorerTAB.h"
#include "gui/GUITheme.h" 
#include <helpers.h>
#include <algorithm>
#include <stack>
#include <hooks/DirectX.h>

#include "Il2CppResolver.h"

struct HierarchyNode {
	Il2CppObject* ptr;
	std::string name;
	bool isActive;
	std::vector<HierarchyNode> children;
};

struct InspectorHistoryItem {
	Il2CppObject* ptr;
	bool isClassView;
};

static std::stack<InspectorHistoryItem> inspectionHistory;

static Il2CppObject* selectedGameObject = nullptr;
static bool isRawClassView = false; //
static int explorerMode = 0; // 0: Hierarchy, 1: Class Search

static std::map<std::string, std::vector<HierarchyNode>> cachedTree;
static char searchFilter[128] = "";
static bool needsRefresh = true;
static bool showInactive = false;

static char classSearchFilter[128] = "";
static std::vector<Il2CppClass*> searchResults;

static std::map<const MethodInfo*, std::vector<std::string>> methodParamBuffers;
static std::map<const MethodInfo*, std::string> methodLastResults;

void UnityExplorerTAB::Reset()
{
	cachedTree.clear();
	searchResults.clear();

	methodParamBuffers.clear();
	methodLastResults.clear();

	while (!inspectionHistory.empty()) inspectionHistory.pop();

	selectedGameObject = nullptr;
	isRawClassView = false;
	needsRefresh = true;

	memset(searchFilter, 0, sizeof(searchFilter));
	memset(classSearchFilter, 0, sizeof(classSearchFilter));
}

HierarchyNode BuildCache(Il2CppObject* go, int depth = 0) {

	if (depth > 50 || !Resolver::Protection::IsAlive(go)) return { nullptr, "Dead/Too Deep", false, {} };

	static Il2CppClass* goClass = Resolver::FindClass("UnityEngine", "GameObject");
	static Il2CppClass* trClass = Resolver::FindClass("UnityEngine", "Transform");

	static const MethodInfo* getName = il2cpp_class_get_method_from_name(goClass, "get_name", 0);
	static const MethodInfo* getActive = il2cpp_class_get_method_from_name(goClass, "get_activeInHierarchy", 0);
	static const MethodInfo* getTransform = il2cpp_class_get_method_from_name(goClass, "get_transform", 0);
	static const MethodInfo* getChildCount = il2cpp_class_get_method_from_name(trClass, "get_childCount", 0);
	static const MethodInfo* getChild = il2cpp_class_get_method_from_name(trClass, "GetChild", 1);
	static const MethodInfo* getGO = il2cpp_class_get_method_from_name(trClass, "get_gameObject", 0);

	HierarchyNode node;
	node.ptr = go;

	Il2CppString* nStr = (Il2CppString*)Resolver::Protection::SafeRuntimeInvoke(getName, go, nullptr);
	node.name = nStr ? il2cppi_to_string(nStr) : "Unnamed";

	Il2CppObject* resActive = Resolver::Protection::SafeRuntimeInvoke(getActive, go, nullptr);
	node.isActive = resActive ? *static_cast<bool*>(il2cpp_object_unbox(resActive)) : true;

	Il2CppObject* transform = Resolver::Protection::SafeRuntimeInvoke(getTransform, go, nullptr);
	if (transform) {
		Il2CppObject* resCount = Resolver::Protection::SafeRuntimeInvoke(getChildCount, transform, nullptr);
		int count = resCount ? *static_cast<int*>(il2cpp_object_unbox(resCount)) : 0;

		for (int i = 0; i < count; i++) {
			void* p[] = { &i };
			Il2CppObject* childTr = Resolver::Protection::SafeRuntimeInvoke(getChild, transform, p);
			if (childTr) {
				Il2CppObject* childGO = Resolver::Protection::SafeRuntimeInvoke(getGO, childTr, nullptr);
				if (childGO && Resolver::Protection::IsAlive(childGO)) {
					node.children.push_back(BuildCache(childGO, depth + 1));
				}
			}
		}
	}
	return node;
}

void DrawCachedNode(const HierarchyNode& node) {
	if (!node.isActive && !showInactive) return;

	if (strlen(searchFilter) > 0) {
		std::string nName = node.name;
		std::string sFilter = searchFilter;
		std::transform(nName.begin(), nName.end(), nName.begin(), ::tolower);
		std::transform(sFilter.begin(), sFilter.end(), sFilter.begin(), ::tolower);
		if (nName.find(sFilter) == std::string::npos && node.children.empty()) return;
	}

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (node.children.empty()) flags |= ImGuiTreeNodeFlags_Leaf;
	if (selectedGameObject == node.ptr) flags |= ImGuiTreeNodeFlags_Selected;

	if (!node.isActive) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

	ImGui::PushID(node.ptr);
	bool opened = ImGui::TreeNodeEx("##node", flags, "%s", node.name.c_str());
	ImGui::PopID();

	if (ImGui::IsItemClicked()) {
		if (Resolver::Protection::IsAlive(node.ptr)) {
			UnityExplorerTAB::Helpers::InspectObject(node.ptr);
		}
		else {
			needsRefresh = true;
		}
	}

	if (opened) {
		for (const auto& child : node.children) {
			DrawCachedNode(child);
		}
		ImGui::TreePop();
	}

	if (!node.isActive) ImGui::PopStyleColor();
}

void UnityExplorerTAB::Render() {
	il2cpp_thread_attach(il2cpp_domain_get());

	if (ImGui::Button("Refresh")) needsRefresh = true;
	ImGui::SameLine();

	ImGui::Text("| Mode:"); ImGui::SameLine();
	ImGui::RadioButton("Hierarchy", &explorerMode, 0); ImGui::SameLine();
	ImGui::RadioButton("Class Browser", &explorerMode, 1);

	ImGui::Separator();

	ImGui::Columns(2, "UnityExplorerLayout", true);

	ImGui::BeginChild("LeftPanelView");

	if (explorerMode == 0) {
		ImGui::Checkbox("Show Inactive", &showInactive);
		ImGui::SetNextItemWidth(-1);
		ImGui::InputTextWithHint("##hSearch", "Filter objects...", searchFilter, 128);
		ImGui::Separator();

		if (needsRefresh) {
			cachedTree.clear();
			static Il2CppClass* resClass = Resolver::FindClass("UnityEngine", "Resources");
			static Il2CppClass* goClass = Resolver::FindClass("UnityEngine", "GameObject");
			static const MethodInfo* findObjects = il2cpp_class_get_method_from_name(resClass, "FindObjectsOfTypeAll", 1);

			Il2CppReflectionType* goType = (Il2CppReflectionType*)il2cpp_type_get_object(il2cpp_class_get_type(goClass));
			void* params[] = { goType };
			Il2CppArray* all = (Il2CppArray*)Resolver::Protection::SafeRuntimeInvoke(findObjects, nullptr, params);

			if (all) {
				static const MethodInfo* getTransform = il2cpp_class_get_method_from_name(goClass, "get_transform", 0);
				static Il2CppClass* trClass = Resolver::FindClass("UnityEngine", "Transform");
				static const MethodInfo* getParent = il2cpp_class_get_method_from_name(trClass, "get_parent", 0);

				for (uint32_t i = 0; i < il2cpp_array_length(all); i++) {
					Il2CppObject* go = GET_ARRAY_ELEMENT(all, i);
					if (!Resolver::Protection::IsAlive(go)) continue;

					Il2CppObject* tr = Resolver::Protection::SafeRuntimeInvoke(getTransform, go, nullptr);
					Il2CppObject* parent = tr ? Resolver::Protection::SafeRuntimeInvoke(getParent, tr, nullptr) : nullptr;

					if (!parent) {
						std::string sName = Resolver::Helpers::GetSceneName(go);
						cachedTree[sName].push_back(BuildCache(go));
					}
				}
			}
			needsRefresh = false;
		}

		for (auto const& [sceneName, nodes] : cachedTree) {
			if (ImGui::TreeNodeEx(sceneName.c_str(), ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) {
				for (const auto& node : nodes) DrawCachedNode(node);
				ImGui::TreePop();
			}
		}
	}
	else {
		ImGui::SetNextItemWidth(-50);
		ImGui::InputTextWithHint("##cSearch", "Search Assembly...", classSearchFilter, 128);
		ImGui::SameLine();

		if (ImGui::Button("Go")) {
			searchResults.clear();
			std::string filter = classSearchFilter;
			std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);

			auto domain = il2cpp_domain_get();
			size_t assembly_count = 0;
			const Il2CppAssembly** assemblies = il2cpp_domain_get_assemblies(domain, &assembly_count);

			if (assemblies) {
				for (size_t i = 0; i < assembly_count; ++i) {
					const Il2CppImage* image = il2cpp_assembly_get_image(assemblies[i]);
					if (!image) continue;

					size_t class_count = il2cpp_image_get_class_count(image);
					for (size_t j = 0; j < class_count; ++j) {
						Il2CppClass* klass = (Il2CppClass*)il2cpp_image_get_class(image, j);
						if (!klass) continue;

						std::string name = il2cpp_class_get_name(klass);
						std::string lowName = name;
						std::transform(lowName.begin(), lowName.end(), lowName.begin(), ::tolower);

						if (lowName.find(filter) != std::string::npos) {
							searchResults.push_back(klass);
						}
					}
				}
			}
		}

		ImGui::Separator();
		for (auto klass : searchResults) {
			std::string name = il2cpp_class_get_name(klass);
			std::string ns = il2cpp_class_get_namespace(klass);
			if (ImGui::Selectable((ns + "::" + name).c_str())) {
				Helpers::InspectClass(klass);
			}
		}
	}
	ImGui::EndChild();

	ImGui::NextColumn();

	ImGui::BeginChild("InspectorView");
	Helpers::DrawInspector();
	ImGui::EndChild();

	ImGui::Columns(1);
}

void UnityExplorerTAB::Helpers::DrawInspector()
{
	if (!selectedGameObject || !Resolver::Protection::IsValidIl2CppObject(selectedGameObject)) {
		ImGui::TextDisabled("Select an object to inspect.");
		if (selectedGameObject) selectedGameObject = nullptr;
		return;
	}

	Il2CppClass* klass = nullptr;

	bool readSuccess = Resolver::Protection::safe_call([&]() {
		if (isRawClassView) {
			klass = (Il2CppClass*)selectedGameObject;
		}
		else {
			if (Resolver::Protection::IsAlive(selectedGameObject)) {
				klass = il2cpp_object_get_class(selectedGameObject);
			}
		}
		});

	if (!readSuccess || !klass) {
		ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error: Invalid Object or Class ptr.");
		if (ImGui::Button("Clear Selection")) selectedGameObject = nullptr;
		return;
	}

	if (!inspectionHistory.empty()) {
		if (ImGui::Button("< Back")) {
			bool foundValid = false;
			while (!inspectionHistory.empty()) {
				InspectorHistoryItem item = inspectionHistory.top();
				inspectionHistory.pop();

				if (!Resolver::Protection::IsValidIl2CppObject(item.ptr)) continue;

				if (!item.isClassView && !Resolver::Protection::IsAlive(item.ptr)) continue;

				selectedGameObject = item.ptr;
				isRawClassView = item.isClassView;

				methodParamBuffers.clear();
				methodLastResults.clear();
				foundValid = true;
				break;
			}

			if (!foundValid && inspectionHistory.empty()) {
				if (!Resolver::Protection::IsValidIl2CppObject(selectedGameObject)) {
					selectedGameObject = nullptr;
				}
			}
		}
		ImGui::SameLine();
	}

	if (ImGui::Button("Refresh")) needsRefresh = true;
	ImGui::SameLine();

	char addrBuf[64];
	sprintf_s(addrBuf, sizeof(addrBuf), "0x%p", selectedGameObject);
	if (ImGui::Button("Copy Addr")) Resolver::Helpers::CopyToClipboard(addrBuf);
	ImGui::SameLine();
	ImGui::TextDisabled("[%s]", addrBuf);

	if (isRawClassView) {
		static Il2CppClass* mbClass = Resolver::FindClass("UnityEngine", "MonoBehaviour");
		bool isMonoBehaviour = false;

		Resolver::Protection::safe_call([&]() {
			if (mbClass && klass) isMonoBehaviour = il2cpp_class_is_assignable_from(mbClass, klass);
			});

		ImGui::SameLine();
		if (!isMonoBehaviour) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.5f, 0.1f, 1.0f));
			if (ImGui::Button("Create Instance")) {
				Il2CppObject* newObj = il2cpp_object_new(klass);
				if (newObj) {
					il2cpp_runtime_object_init(newObj);
					InspectObject(newObj);
				}
			}
			ImGui::PopStyleColor();
		}
		else {
			ImGui::TextDisabled("(MonoBehaviour)");
		}
	}

	ImGui::Separator();

	static Il2CppClass* goClass = Resolver::FindClass("UnityEngine", "GameObject");
	static Il2CppClass* compClass = Resolver::FindClass("UnityEngine", "Component");
	bool isGameObject = false;

	Resolver::Protection::safe_call([&]() {
		if (!isRawClassView && klass && goClass) {
			isGameObject = il2cpp_class_is_assignable_from(goClass, klass);
		}
		});

	if (isGameObject) {
		static const MethodInfo* getName = il2cpp_class_get_method_from_name(goClass, "get_name", 0);
		Il2CppString* nStr = (Il2CppString*)Resolver::Protection::SafeRuntimeInvoke(getName, selectedGameObject, nullptr);
		ImGui::TextColored(ImVec4(1, 1, 0, 1), "GameObject: %s", nStr ? il2cppi_to_string(nStr).c_str() : "Unnamed");
		ImGui::Separator();

		ImGui::BeginChild("CompScroll");
		static const MethodInfo* getComps = il2cpp_class_get_method_from_name(goClass, "GetComponents", 1);
		Il2CppReflectionType* compTypeObj = (Il2CppReflectionType*)il2cpp_type_get_object(il2cpp_class_get_type(compClass));
		void* pComps[] = { compTypeObj };
		Il2CppArray* components = (Il2CppArray*)Resolver::Protection::SafeRuntimeInvoke(getComps, selectedGameObject, pComps);

		if (components) {
			for (uint32_t i = 0; i < il2cpp_array_length(components); i++) {
				Il2CppObject* comp = GET_ARRAY_ELEMENT(components, i);
				if (!comp) continue;
				Il2CppClass* cKlass = il2cpp_object_get_class(comp);

				ImGui::PushID(comp);

				const MethodInfo* getEn = il2cpp_class_get_method_from_name(cKlass, "get_enabled", 0);
				if (getEn) {
				}

				if (ImGui::CollapsingHeader(il2cpp_class_get_name(cKlass))) {
					ImGui::Indent();
					void* fIter = nullptr;
					while (FieldInfo* f = il2cpp_class_get_fields(cKlass, &fIter)) DrawField(comp, f);
					ImGui::Spacing(); ImGui::TextDisabled("Properties");
					void* pIter = nullptr;
					while (const PropertyInfo* p = il2cpp_class_get_properties(cKlass, &pIter)) DrawProperty(comp, p);
					ImGui::Spacing();
					DrawMethods(comp, cKlass);
					ImGui::Unindent();
				}
				ImGui::PopID();
			}
		}
		ImGui::EndChild();
	}
	else {
		ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Class: %s", il2cpp_class_get_name(klass));
		ImGui::TextDisabled("Namespace: %s", il2cpp_class_get_namespace(klass));

		const char* singletonNames[] = { "instance", "Instance", "singleton", "_instance", "SharedInstance" };
		for (const char* sName : singletonNames) {
			FieldInfo* field = il2cpp_class_get_field_from_name(klass, sName);
			if (field && (field->type->attrs & 0x0010)) {
				Il2CppObject* val = nullptr;
				il2cpp_field_static_get_value(field, &val);
				if (val && Resolver::Protection::IsValidIl2CppObject(val)) {
					ImGui::Separator();
					ImGui::TextColored(ImVec4(1, 0.8f, 0, 1), "[!] Found Singleton: %s", sName);
					ImGui::SameLine();
					if (ImGui::Button("Inspect Singleton")) InspectObject(val);
				}
			}
		}

		ImGui::Separator();
		static std::vector<Il2CppObject*> foundInstances;
		static bool showInstanceList = false;
		static Il2CppClass* lastSearchedClass = nullptr;

		if (lastSearchedClass != klass) {
			foundInstances.clear();
			showInstanceList = false;
			lastSearchedClass = klass;
		}

		if (ImGui::Button("Find Active Objects in Scene")) {
			foundInstances = Resolver::FindObjectsByType(klass);
			showInstanceList = true;
		}

		if (showInstanceList) {
			ImGui::Text("Found %d objects.", (int)foundInstances.size());
			if (!foundInstances.empty()) {
				ImGui::BeginChild("InstList", ImVec2(0, 120), true);
				for (size_t i = 0; i < foundInstances.size(); i++) {
					if (!Resolver::Protection::IsAlive(foundInstances[i])) continue;
					ImGui::PushID((int)i);
					if (ImGui::Button("Inspect")) InspectObject(foundInstances[i]);
					ImGui::SameLine();
					ImGui::Text("0x%p", foundInstances[i]);
					ImGui::PopID();
				}
				ImGui::EndChild();
			}
		}

		ImGui::Separator();
		ImGui::BeginChild("RawScroll");

		void* fIter = nullptr;
		while (FieldInfo* f = il2cpp_class_get_fields(klass, &fIter))
			DrawField(isRawClassView ? nullptr : selectedGameObject, f);

		ImGui::Spacing(); ImGui::TextDisabled("Properties");
		void* pIter = nullptr;
		while (const PropertyInfo* p = il2cpp_class_get_properties(klass, &pIter))
			DrawProperty(isRawClassView ? nullptr : selectedGameObject, p);

		ImGui::Spacing();
		DrawMethods(isRawClassView ? nullptr : selectedGameObject, klass);

		ImGui::EndChild();
	}
}

void UnityExplorerTAB::Helpers::DrawObjectNode(Il2CppObject* gameObject)
{
	if (!Resolver::Protection::IsAlive(gameObject)) return;

	static Il2CppClass* goClass = Resolver::FindClass("UnityEngine", "GameObject");
	static const MethodInfo* getName = il2cpp_class_get_method_from_name(goClass, "get_name", 0);
	static const MethodInfo* getActive = il2cpp_class_get_method_from_name(goClass, "get_activeInHierarchy", 0);
	static const MethodInfo* getTransform = il2cpp_class_get_method_from_name(goClass, "get_transform", 0);

	Il2CppString* nameIl2Cpp = (Il2CppString*)il2cpp_runtime_invoke(getName, gameObject, nullptr, nullptr);
	bool isActive = Resolver::Protection::SafeUnbox<bool>(il2cpp_runtime_invoke(getActive, gameObject, nullptr, nullptr), false);

	if (!isActive) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

	Il2CppObject* transform = il2cpp_runtime_invoke(getTransform, gameObject, nullptr, nullptr);
	int childCount = 0;
	if (transform) {
		static Il2CppClass* trClass = Resolver::FindClass("UnityEngine", "Transform");
		childCount = Resolver::Protection::SafeUnbox<int>(il2cpp_runtime_invoke(il2cpp_class_get_method_from_name(trClass, "get_childCount", 0), transform, nullptr, nullptr), 0);
	}

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (childCount == 0) flags |= ImGuiTreeNodeFlags_Leaf;
	if (selectedGameObject == gameObject) flags |= ImGuiTreeNodeFlags_Selected;

	bool opened = ImGui::TreeNodeEx((void*)gameObject, flags, "%s", nameIl2Cpp ? il2cppi_to_string(nameIl2Cpp).c_str() : "Unnamed");
	if (ImGui::IsItemClicked()) {
		InspectObject(gameObject);
	}

	if (opened) {
		if (childCount > 0) {
			static Il2CppClass* trClass = Resolver::FindClass("UnityEngine", "Transform");
			static const MethodInfo* getChild = il2cpp_class_get_method_from_name(trClass, "GetChild", 1);
			static const MethodInfo* getGO = il2cpp_class_get_method_from_name(trClass, "get_gameObject", 0);
			for (int i = 0; i < childCount; i++) {
				void* p[] = { &i };
				Il2CppObject* cTr = il2cpp_runtime_invoke(getChild, transform, p, nullptr);
				if (cTr) DrawObjectNode((Il2CppObject*)il2cpp_runtime_invoke(getGO, cTr, nullptr, nullptr));
			}
		}
		ImGui::TreePop();
	}
	if (!isActive) ImGui::PopStyleColor();
}

void UnityExplorerTAB::Helpers::InspectObject(Il2CppObject* obj)
{
	if (!obj || !Resolver::Protection::IsValidIl2CppObject(obj)) return;
	if (selectedGameObject == obj) return;

	if (selectedGameObject) {
		inspectionHistory.push({ selectedGameObject, isRawClassView });
	}

	selectedGameObject = obj;
	isRawClassView = false; 

	methodParamBuffers.clear();
	methodLastResults.clear();
}

void UnityExplorerTAB::Helpers::DrawField(Il2CppObject* obj, FieldInfo* field)
{
	if (!field) return;

	const char* fieldName = il2cpp_field_get_name(field);
	const Il2CppType* type = il2cpp_field_get_type(field);
	int typeEnum = type->type;
	bool isStatic = (type->attrs & 0x0010);
	bool hasValueAccess = (obj != nullptr) || isStatic;

	ImGui::PushID(field);

	ImGui::Columns(2, "FieldCols", true);
	static bool initWidth = true;
	if (initWidth) {
		ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.45f);
		initWidth = false;
	}

	char* typeName = il2cpp_type_get_name(type);
	ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "[%s]", typeName ? typeName : "???");
	if (typeName) il2cpp_free(typeName);

	ImGui::SameLine();
	if (isStatic) ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.5f, 1.0f), "(S) %s", fieldName);
	else ImGui::Text("%s", fieldName);

	ImGui::NextColumn();
	ImGui::PushItemWidth(-1);

	if (!hasValueAccess) {
		ImGui::TextDisabled("null (Instance only)");
		ImGui::PopItemWidth();
		ImGui::Columns(1);
		ImGui::PopID();
		return;
	}

	switch (typeEnum) {
	case IL2CPP_TYPE_BOOLEAN: {
		bool val = false;
		Resolver::GetFieldValue(obj, field, &val);
		if (ImGui::Checkbox("##v", &val)) {
			if (isStatic) il2cpp_field_static_set_value(field, &val);
			else il2cpp_field_set_value(obj, field, &val);
		}
		break;
	}
	case IL2CPP_TYPE_I4: {
		int32_t val = 0;
		Resolver::GetFieldValue(obj, field, &val);
		if (ImGui::DragInt("##v", &val)) {
			if (isStatic) il2cpp_field_static_set_value(field, &val);
			else il2cpp_field_set_value(obj, field, &val);
		}
		break;
	}
	case IL2CPP_TYPE_R4: {
		float val = 0;
		Resolver::GetFieldValue(obj, field, &val);
		if (ImGui::DragFloat("##v", &val, 0.05f)) {
			if (isStatic) il2cpp_field_static_set_value(field, &val);
			else il2cpp_field_set_value(obj, field, &val);
		}
		break;
	}
	case IL2CPP_TYPE_VALUETYPE: {
		Il2CppClass* structKlass = il2cpp_class_from_type(type);
		if (!structKlass) break;
		std::string sName = structKlass->name;

		if (sName == "Vector3") {
			app::Vector3 v;
			Resolver::GetFieldValue(obj, field, &v);
			if (ImGui::DragFloat3("##v3", &v.x, 0.1f)) {
				if (isStatic) il2cpp_field_static_set_value(field, &v);
				else il2cpp_field_set_value(obj, field, &v);
			}
		}
		else if (sName == "Color" || sName == "Color32") {
			float c[4];
			Resolver::GetFieldValue(obj, field, &c);
			if (ImGui::ColorEdit4("##clr", c, ImGuiColorEditFlags_NoInputs)) {
				if (isStatic) il2cpp_field_static_set_value(field, &c);
				else il2cpp_field_set_value(obj, field, &c);
			}
		}
		else {
			ImGui::TextDisabled("{Struct: %s}", sName.c_str());
		}
		break;
	}
	case IL2CPP_TYPE_STRING: {
		Il2CppString* strObj = nullptr;
		Resolver::GetFieldValue(obj, field, &strObj);
		if (strObj && strObj->chars) {
			ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.2f, 1.0f), "\"%s\"", il2cppi_to_string(strObj).c_str());
		}
		else ImGui::TextDisabled("null");
		break;
	}
	case IL2CPP_TYPE_CLASS:
	case IL2CPP_TYPE_OBJECT: {
		Il2CppObject* val = nullptr;
		Resolver::GetFieldValue(obj, field, &val);
		if (val && Resolver::Protection::IsValidIl2CppObject(val)) {
			if (ImGui::SmallButton("Inspect")) {
				InspectObject(val);
			}
			ImGui::SameLine();
			ImGui::TextDisabled("0x%p", val);
		}
		else ImGui::TextDisabled("null");
		break;
	}
	default:
		ImGui::TextDisabled("?");
		break;
	}

	ImGui::PopItemWidth();
	ImGui::Columns(1);
	ImGui::PopID();
}

void UnityExplorerTAB::Helpers::DrawProperty(Il2CppObject* obj, const PropertyInfo* prop)
{
	if (!prop || !prop->get) return;

	const char* propName = prop->name;
	const Il2CppType* returnType = prop->get->return_type;
	int typeEnum = returnType->type;

	ImGui::PushID(prop);

	ImGui::Columns(2, "PropCols", true);
	static bool initPropWidth = true;
	if (initPropWidth) {
		ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.35f);
		initPropWidth = false;
	}

	ImGui::TextColored(ImVec4(0.3f, 0.6f, 1.0f, 1.0f), "[P] %s", propName);

	ImGui::NextColumn();
	ImGui::PushItemWidth(-1);

	Il2CppObject* boxedVal = Resolver::Protection::SafeRuntimeInvoke(prop->get, obj, nullptr);

	switch (typeEnum) {
	case IL2CPP_TYPE_BOOLEAN: {
		bool val = Resolver::Protection::SafeUnbox<bool>(boxedVal, false);
		if (ImGui::Checkbox("##v", &val) && prop->set) {
			void* p[] = { &val };
			Resolver::Protection::SafeRuntimeInvoke(prop->set, obj, p);
		}
		break;
	}
	case IL2CPP_TYPE_I4: {
		int32_t val = Resolver::Protection::SafeUnbox<int32_t>(boxedVal, 0);
		if (ImGui::DragInt("##v", &val) && prop->set) {
			void* p[] = { &val };
			Resolver::Protection::SafeRuntimeInvoke(prop->set, obj, p);
		}
		break;
	}
	case IL2CPP_TYPE_R4: {
		float val = Resolver::Protection::SafeUnbox<float>(boxedVal, 0.0f);
		if (ImGui::DragFloat("##v", &val, 0.05f) && prop->set) {
			void* p[] = { &val };
			Resolver::Protection::SafeRuntimeInvoke(prop->set, obj, p);
		}
		break;
	}
	case IL2CPP_TYPE_STRING: {
		if (boxedVal) {
			ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.2f, 1.0f), "\"%s\"", il2cppi_to_string((Il2CppString*)boxedVal).c_str());
		}
		else ImGui::TextDisabled("null");
		break;
	}
	case IL2CPP_TYPE_CLASS:
	case IL2CPP_TYPE_OBJECT: {
		if (boxedVal && Resolver::Protection::IsValidIl2CppObject(boxedVal)) {
			if (ImGui::SmallButton("Inspect")) InspectObject(boxedVal);
			ImGui::SameLine();
			ImGui::TextDisabled("0x%p", boxedVal);
		}
		else ImGui::TextDisabled("null");
		break;
	}
	}

	ImGui::PopItemWidth();
	ImGui::Columns(1);
	ImGui::PopID();
}

void UnityExplorerTAB::Helpers::DrawMethods(Il2CppObject* obj, Il2CppClass* klass)
{
	if (ImGui::CollapsingHeader("Methods (Advanced)", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Indent();
		void* mIter = nullptr;

		while (const MethodInfo* method = il2cpp_class_get_methods(klass, &mIter)) {
			const char* mName = method->name;
			if (strstr(mName, "get_") || strstr(mName, "set_") || strstr(mName, ".ctor")) continue;

			int paramCount = il2cpp_method_get_param_count(method);
			ImGui::PushID(method);
			ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "[M] %s", mName);

			auto& buffers = methodParamBuffers[method];
			if (buffers.size() != paramCount) buffers.resize(paramCount, "");

			std::vector<void*> params(paramCount);
			std::vector<int32_t> iValues(paramCount);
			std::vector<float> fValues(paramCount);
			std::vector<uint8_t> bValues(paramCount);
			std::vector<Il2CppObject*> objValues(paramCount);

			if (paramCount > 0) ImGui::Indent();
			for (int i = 0; i < paramCount; i++) {
				const Il2CppType* pType = il2cpp_method_get_param(method, i);
				char* pTypeName = il2cpp_type_get_name(pType);
				const char* pName = il2cpp_method_get_param_name(method, i);

				ImGui::PushID(i);

				char label[128];
				sprintf_s(label, sizeof(label), "Arg %d", i);

				char buf[512];
				if (buffers[i].length() < 512) strcpy_s(buf, buffers[i].c_str());
				else buf[0] = '\0';

				ImGui::SetNextItemWidth(200.0f);
				if (ImGui::InputText(label, buf, sizeof(buf))) {
					buffers[i] = buf;
				}
				ImGui::SameLine();
				ImGui::TextDisabled("%s (%s)", pName ? pName : "?", pTypeName ? pTypeName : "?");

				try {
					if (pType->type == IL2CPP_TYPE_I4 || pType->type == IL2CPP_TYPE_U4) {
						iValues[i] = std::atoi(buffers[i].c_str());
						params[i] = &iValues[i];
					}
					else if (pType->type == IL2CPP_TYPE_R4) {
						fValues[i] = (float)std::atof(buffers[i].c_str());
						params[i] = &fValues[i];
					}
					else if (pType->type == IL2CPP_TYPE_BOOLEAN) {
						std::string s = buffers[i];
						bValues[i] = (s == "1" || s == "true" || s == "True") ? 1 : 0;
						params[i] = &bValues[i];
					}
					else if (pType->type == IL2CPP_TYPE_STRING) {
						params[i] = il2cpp_string_new(buffers[i].c_str());
					}
					else if (pType->type == IL2CPP_TYPE_CLASS || pType->type == IL2CPP_TYPE_OBJECT) {
						if (buffers[i].find("0x") == 0) {
							uintptr_t addr = std::stoull(buffers[i], nullptr, 16);
							objValues[i] = (Il2CppObject*)addr;
						}
						else objValues[i] = nullptr;
						params[i] = objValues[i];
					}
				}
				catch (...) { params[i] = nullptr; }

				if (pTypeName) il2cpp_free(pTypeName);
				ImGui::PopID();
			}
			if (paramCount > 0) ImGui::Unindent();

			if (ImGui::Button("Invoke")) {
				Il2CppObject* res = Resolver::Protection::SafeRuntimeInvoke(method, obj, params.data());
				if (!res) methodLastResults[method] = "Null/Void";
				else {
					if (method->return_type->type == IL2CPP_TYPE_STRING)
						methodLastResults[method] = il2cppi_to_string((Il2CppString*)res);
					else {
						char hx[64]; sprintf_s(hx, "0x%p (Object)", res);
						methodLastResults[method] = hx;
					}
				}
			}

			if (methodLastResults.count(method)) {
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(0, 1, 0, 1), "-> %s", methodLastResults[method].c_str());
				if (methodLastResults[method].find("0x") == 0) {
					ImGui::SameLine();
					if (ImGui::SmallButton("Inspect")) {
						try {
							size_t sp = methodLastResults[method].find(' ');
							std::string hex = methodLastResults[method].substr(0, sp);
							InspectObject((Il2CppObject*)std::stoull(hex, nullptr, 16));
						}
						catch (...) {}
					}
				}
			}

			ImGui::PopID();
			ImGui::Separator();
		}
		ImGui::Unindent();
	}
}

void UnityExplorerTAB::Helpers::DrawClassSearch()
{
	ImGui::TextColored(ImVec4(1, 1, 0, 1), "Assembly Class Browser");
	ImGui::Separator();

	ImGui::SetNextItemWidth(-100);
	ImGui::InputTextWithHint("##ClassSearch", "Search Class Name...", classSearchFilter, 128);
	ImGui::SameLine();

	if (ImGui::Button("Search", ImVec2(80, 0))) {
		searchResults.clear();
		std::string filter = classSearchFilter;
		std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);

		auto domain = il2cpp_domain_get();
		size_t assembly_count;
		const Il2CppAssembly** assemblies = il2cpp_domain_get_assemblies(domain, &assembly_count);

		for (size_t i = 0; i < assembly_count; ++i) {
			const Il2CppImage* image = il2cpp_assembly_get_image(assemblies[i]);
			size_t class_count = il2cpp_image_get_class_count(image);
			for (size_t j = 0; j < class_count; ++j) {
				Il2CppClass* klass = (Il2CppClass*)il2cpp_image_get_class(image, j);
				std::string name = il2cpp_class_get_name(klass);
				std::string lower_name = name;
				std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);

				if (lower_name.find(filter) != std::string::npos) {
					searchResults.push_back(klass);
				}
			}
		}
	}

	ImGui::BeginChild("SearchList", ImVec2(0, 0), true);
	for (auto klass : searchResults) {
		std::string name = il2cpp_class_get_name(klass);
		std::string ns = il2cpp_class_get_namespace(klass);
		if (ImGui::Selectable((ns + "::" + name).c_str())) {
			InspectClass(klass); 
		}
	}
	ImGui::EndChild();
}

void UnityExplorerTAB::Helpers::InspectClass(Il2CppClass* klass)
{
	if (!klass) return;

	if (selectedGameObject) {
		inspectionHistory.push({ selectedGameObject, isRawClassView });
	}

	selectedGameObject = (Il2CppObject*)klass;
	isRawClassView = true; 
}

void UnityExplorerTAB::Helpers::DrawVector3(const char* label, Il2CppObject* transform, const char* propName)
{
	app::Vector3 vec = Resolver::GetProperty<app::Vector3>(transform, propName);

	ImGui::Text("%s", label); ImGui::SameLine(80);
	ImGui::PushItemWidth(60.0f);

	bool changed = false;
	char idX[64], idY[64], idZ[64];
	sprintf_s(idX, "X##%s", label);
	sprintf_s(idY, "Y##%s", label);
	sprintf_s(idZ, "Z##%s", label);

	if (ImGui::DragFloat(idX, &vec.x, 0.1f)) changed = true; ImGui::SameLine();
	if (ImGui::DragFloat(idY, &vec.y, 0.1f)) changed = true; ImGui::SameLine();
	if (ImGui::DragFloat(idZ, &vec.z, 0.1f)) changed = true;

	if (changed) {
		Resolver::SetProperty<app::Vector3>(transform, propName, vec);
	}

	ImGui::PopItemWidth();
}
