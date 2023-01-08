// MIT License
//
// Copyright(c) 2022-2023 Matthieu Bucchianeri
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this softwareand associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright noticeand this permission notice shall be included in all
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

#include "pch.h"

namespace pimax_openxr::appinsights {

    class AppInsights {
      public:
        AppInsights();
        ~AppInsights();

        void initialize();

        void logMetric(const std::string& metric, double value);

        void logVersion(const std::string& version);
        void logApplicationInfo(const std::string& name, const std::string& engine);
        void logScenario(const std::string& gfxApi, bool useLighthouse, int fovLevel, bool useParallelProjection);
        void logFeature(const std::string& feature);
        void logUnimplemented(const std::string& feature);
        void logUsage(double sessionTime, uint64_t frameCount);
        void logProduct(const std::string& product);
        void logError(const std::string& error);

        void tick();

      private:
#ifndef NOCURL
        void transact(const std::string& messageType, const std::string& data);

        CURLM* m_multiHandle{nullptr};
        
        std::mutex m_poolLock;
        std::deque<CURL*> m_pool;
        std::set<CURL*> m_inflight;

        struct curl_slist* m_headers{nullptr};

        std::string m_applicationName;
        std::string m_machineUuid;
#endif
    };

} // namespace pimax_openxr::appinsights
