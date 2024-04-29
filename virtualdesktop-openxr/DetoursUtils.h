// MIT License
//
// Copyright(c) 2024 Matthieu Bucchianeri
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this softwareand associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

namespace DetoursUtils {

#define DECLARE_DETOUR_FUNCTION(ReturnType, Callconv, FunctionName, ...)                                               \
    ReturnType(Callconv* original_##FunctionName)(##__VA_ARGS__) = nullptr;                                            \
    ReturnType Callconv hooked_##FunctionName(##__VA_ARGS__)

    template <class T, typename TMethod>
    void DetourMethodAttach(T* instance, unsigned int methodOffset, TMethod hooked, TMethod& original) {
        if (original) {
            // Already hooked.
            return;
        }

        LPVOID* vtable = *((LPVOID**)instance);
        LPVOID target = vtable[methodOffset];

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());

        original = (TMethod)target;
        DetourAttach((PVOID*)&original, hooked);

        DetourTransactionCommit();
    }

    template <class T, typename TMethod>
    void DetourMethodDetach(T* instance, unsigned int methodOffset, TMethod hooked, TMethod& original) {
        if (!original) {
            // Not hooked.
            return;
        }

        LPVOID* vtable = *((LPVOID**)instance);
        LPVOID target = vtable[methodOffset];

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());

        DetourDetach((PVOID*)&original, hooked);

        DetourTransactionCommit();

        original = nullptr;
    }

} // namespace DetoursUtils
