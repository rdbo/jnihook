#include "jnihook.h"
#include <functional>
#include <expected>

namespace jnihook {
        typedef jnihook_result_t result_t;

        inline result_t
        init(JavaVM *jvm)
        {
                return JNIHook_Init(jvm);
        }

        template <typename T>
        inline std::expected<jmethodID, result_t>
        attach(jmethodID method, T *native_hook_method)
        {
                jmethodID orig_method;
                result_t result = JNIHook_Attach(method,
                                                 reinterpret_cast<void *>(native_hook_method),
                                                 &orig_method);

                if (result != JNIHOOK_OK)
                        return std::unexpected(result);

                return orig_method;
        }

        inline result_t
        detach(jmethodID method)
        {
                return JNIHook_Detach(method);
        }

        inline result_t
        shutdown()
        {
                return JNIHook_Shutdown();
        }
}
