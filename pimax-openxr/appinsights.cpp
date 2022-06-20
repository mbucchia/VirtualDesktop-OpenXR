// MIT License
//
// Copyright(c) 2022 Matthieu Bucchianeri
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

#include "pch.h"

#include "appinsights.h"
#include "log.h"
#include "utils.h"

namespace {

    using namespace pimax_openxr::log;

#if !defined(NOCURL) && defined(_DEBUG)
    // https://curl.se/libcurl/c/multi-debugcallback.html
    int curlTrace(CURL* handle, curl_infotype type, unsigned char* data, size_t size, void* userp) {
        const char* text;

        switch (type) {
        case CURLINFO_TEXT:
            Log("== Info: %s\n", data);
            [[fallthrough]];
        default: /* in case a new one is introduced to shock us */
            return 0;

        case CURLINFO_HEADER_OUT:
            text = "=> Send header";
            break;
        case CURLINFO_DATA_OUT:
            text = "=> Send data";
            break;
        case CURLINFO_HEADER_IN:
            text = "<= Recv header";
            break;
        case CURLINFO_DATA_IN:
            text = "<= Recv data";
            break;
        }

        Log("%s: %s", data);
        return 0;
    }
#endif

    // https://stackoverflow.com/questions/7724448/simple-json-string-escape-for-c
    std::string escapeJson(const std::string& s) {
        std::ostringstream o;
        for (auto c = s.cbegin(); c != s.cend(); c++) {
            if (*c == '"' || *c == '\\' || ('\x00' <= *c && *c <= '\x1f')) {
                o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(*c);
            } else {
                o << *c;
            }
        }
        return o.str();
    }

} // namespace

namespace pimax_openxr::appinsights {

    using namespace pimax_openxr::utils;

#ifndef NOCURL

    const std::string appInsightsUrl = "https://dc.services.visualstudio.com/v2/track";
    const std::string iKey = "dac89167-4187-4e65-af2a-cd5542addd69"; // PimaxXR

    // Application Insights does not have an SDK for pure Win32 C++ apps.
    // We will do POST requests by hand. Here is some useful documentation:
    // https://apmtips.com/posts/2017-10-27-send-metric-to-application-insights/
    // https://github.com/microsoft/ApplicationInsights-dotnet-server/tree/develop/WEB/Schema/PublicSchema
    // https://github.com/microsoft/ApplicationInsights-node.js/blob/develop/Library/EnvelopeFactory.ts
    AppInsights::AppInsights() {
    }

    AppInsights::~AppInsights() {
        // Wait for all transactions to complete before cleanup.
        int tries = 20;
        while (tries && !m_inflight.empty()) {
            tick();
            std::this_thread::sleep_for(100ms);
            tries--;
        }

        while (!m_pool.empty()) {
            CURL* handle = m_pool.front();
            m_pool.pop_front();

            curl_easy_cleanup(handle);
        }

        if (m_headers) {
            curl_slist_free_all(m_headers);
        }

        if (m_multiHandle) {
            curl_multi_cleanup(m_multiHandle);
        }
    }

    void AppInsights::initialize() {
        m_multiHandle = curl_multi_init();

        m_headers = curl_slist_append(m_headers, "Expect:");
        m_headers = curl_slist_append(m_headers, "Content-Type: application/json");

        for (int i = 0; i < 10; i++) {
            CURL* handle = curl_easy_init();
            if (handle) {
                // Initialize the common parameters for the transaction.
                curl_easy_setopt(handle, CURLOPT_URL, appInsightsUrl.c_str());
                curl_easy_setopt(handle, CURLOPT_HTTPHEADER, m_headers);
                curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, 5);
                curl_easy_setopt(handle, CURLOPT_TIMEOUT, 5);

#ifdef _DEBUG
                curl_easy_setopt(handle, CURLOPT_DEBUGFUNCTION, curlTrace);
#endif

                m_pool.push_back(handle);
            }
        }

        m_machineUuid = getMachineUuid();
    }

    void AppInsights::transact(const std::string& messageType, const std::string& data) {
        // Format the message for Application Insights.
        const std::time_t now = std::time(nullptr);
        char iso8601[sizeof("0000-00-00T00:00:00Z")];
        strftime(iso8601, sizeof(iso8601), "%FT%TZ", gmtime(&now));

        const auto document = fmt::format(R"_({{
  "name": "{}",
  "time": "{}",
  "iKey": "{}",
  "data": {{
    "baseType": "{}",
    "baseData": {{
{}
    }}
  }}
}})_",
                                          messageType,
                                          iso8601,
                                          iKey,
                                          messageType,
                                          data);

        {
            std::unique_lock lock(m_poolLock);

            // Try to get a transaction handle from the pool.
            if (m_pool.empty()) {
                // We drop this transaction.
                return;
            }

            CURL* handle = m_pool.front();
            m_pool.pop_front();

            curl_easy_setopt(handle, CURLOPT_COPYPOSTFIELDS, document.c_str());

            // Submit the transaction.
            curl_multi_add_handle(m_multiHandle, handle);
            int running;
            curl_multi_perform(m_multiHandle, &running);

            m_inflight.insert(handle);
        }
    }

    void AppInsights::logMetric(const std::string& metric, double value) {
        const auto data = fmt::format(R"_(
      "metrics": [
        {{
          "name": "{}",
          "value": {},
          "count": 1
        }}
      ],
      "properties": {{
        "machineUuid": "{}",
        "applicationName": "{}"
      }})_",
                                      escapeJson(metric),
                                      value,
                                      escapeJson(m_machineUuid),
                                      escapeJson(m_applicationName));
        transact("MetricData", data);
    }

    void AppInsights::logVersion(const std::string& version) {
        const auto data = fmt::format(R"_(
      "name": "VersionInfo",
      "properties": {{
        "machineUuid": "{}",
        "version": "{}"
      }})_",
                                      escapeJson(m_machineUuid),
                                      escapeJson(version));
        transact("EventData", data);
    }

    void AppInsights::logApplicationInfo(const std::string& name, const std::string& engine) {
        m_applicationName = name;

        const auto data = fmt::format(R"_(
      "name": "ApplicationInfo",
      "properties": {{
        "machineUuid": "{}",
        "applicationName": "{}",
        "engineName": "{}"
      }})_",
                                      escapeJson(m_machineUuid),
                                      escapeJson(name),
                                      escapeJson(engine));
        transact("EventData", data);
    }

    void
    AppInsights::logScenario(const std::string& gfxApi, bool useLighthouse, int fovLevel, bool useParallelProjection) {
        const auto data = fmt::format(R"_(
      "name": "ApplicationUserScenario",
      "properties": {{
        "machineUuid": "{}",
        "applicationName": "{}",
        "gfxApi": "{}",
        "useLighthouse": "{}",
        "fovLevel": "{}",
        "useParallelProjection": "{}"
      }})_",
                                      escapeJson(m_machineUuid),
                                      escapeJson(m_applicationName),
                                      escapeJson(gfxApi),
                                      useLighthouse ? 1 : 0,
                                      fovLevel,
                                      useParallelProjection ? 1 : 0);
        transact("EventData", data);
    }

    void AppInsights::logFeature(const std::string& feature) {
        const auto data = fmt::format(R"_(
      "name": "ApplicationFeature",
      "properties": {{
        "machineUuid": "{}",
        "applicationName": "{}",
        "feature": "{}"
      }})_",
                                      escapeJson(m_machineUuid),
                                      escapeJson(m_applicationName),
                                      escapeJson(feature));
        transact("EventData", data);
    }

    void AppInsights::logUnimplemented(const std::string& feature) {
        const auto data = fmt::format(R"_(
      "name": "UnimplementedFeature",
      "properties": {{
        "machineUuid": "{}",
        "applicationName": "{}",
        "feature": "{}"
      }})_",
                                      escapeJson(m_machineUuid),
                                      escapeJson(m_applicationName),
                                      escapeJson(feature));
        transact("EventData", data);
    }

    void AppInsights::logUsage(double sessionTime, uint64_t frameCount) {
        logMetric("SessionTime", sessionTime);
        logMetric("SessionFrameCount", (double)frameCount);
    }

    void AppInsights::logProduct(const std::string& product) {
        const auto data = fmt::format(R"_(
      "name": "ProductName",
      "properties": {{
        "machineUuid": "{}",
        "productName": "{}"
      }})_",
                                      escapeJson(m_machineUuid),
                                      escapeJson(product));
        transact("EventData", data);
    }

    void AppInsights::logError(const std::string& error) {
        const auto data = fmt::format(R"_(
      "message": "{}",
      "properties": {{
        "machineUuid": "{}",
        "applicationName": "{}"
      }})_",
                                      escapeJson(error),
                                      escapeJson(m_machineUuid),
                                      escapeJson(m_applicationName));
        transact("MessageData", data);
    }

    void AppInsights::tick() {
        int running;
        curl_multi_perform(m_multiHandle, &running);
        curl_multi_poll(m_multiHandle, NULL, 0, 0, NULL);

        // Process completion of transactions.
        {
            std::unique_lock lock(m_poolLock);

            int msgLeft;
            while (auto m = curl_multi_info_read(m_multiHandle, &msgLeft)) {
                if (m && (m->msg == CURLMSG_DONE)) {
                    DebugLog("Application Insight transaction result: %d\n", m->data.result);
                    m_inflight.erase(m->easy_handle);
                    m_pool.push_back(m->easy_handle);
                }
            }
        }
    }

#else

    AppInsights::AppInsights() {
    }

    AppInsights::~AppInsights() {
    }

    void AppInsights::logMetric(const std::string& metric, double value) {
    }

    void AppInsights::logVersion(const std::string& version) {
    }

    void AppInsights::logApplicationInfo(const std::string& name, const std::string& engine) {
    }

    void
    AppInsights::logScenario(const std::string& gfxApi, bool useLighthouse, int fovLevel, bool useParallelProjection) {
    }

    void AppInsights::logFeature(const std::string& feature) {
    }

    void AppInsights::logUnimplemented(const std::string& feature) {
    }

    void AppInsights::logUsage(double sessionTime, uint64_t frameCount) {
    }

    void AppInsights::logProduct(const std::string& product) {
    }

    void AppInsights::logError(const std::string& error) {
    }

    void AppInsights::tick() {
    }

#endif

} // namespace pimax_openxr::appinsights
