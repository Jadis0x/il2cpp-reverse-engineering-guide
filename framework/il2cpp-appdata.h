// Generated C++ file by Il2CppInspectorPro - http://www.djkaty.com - https://github.com/djkaty
      // IL2CPP application data

      #pragma once

      #include <cstdint>

      // Application-specific types
      #include "il2cpp-types.h"

      // IL2CPP API function pointers
      #include "il2cpp-api-functions-ptr.h"

      // IL2CPP APIs
      #define DO_API(r, n, p) extern r (*n) p
      #include "il2cpp-api-functions.h"
      #undef DO_API

      // Application-specific functions
      #define DO_APP_FUNC(a, r, n, p) extern r (*n) p
      #define DO_APP_FUNC_METHODINFO(a, n) extern struct MethodInfo ** n
      namespace app {
      #include "il2cpp-functions.h"
      }
      #undef DO_APP_FUNC
      #undef DO_APP_FUNC_METHODINFO

      // TypeInfo pointers
      #define DO_TYPEDEF(a, n) extern n ## __Class** n ## __TypeInfo
      namespace app {
      #include "il2cpp-types-ptr.h"
      }
      #undef DO_TYPEDEF