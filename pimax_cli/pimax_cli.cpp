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

#include <iostream>
#include <string>

#include <PVR.h>
#include <PVR_API.h>

int main(int argc, char** argv) {
    int retval = 1;
    if (argc != 3 && argc != 4) {
        std::cerr << "usage: " << argv[0] << "<-int|-decimal|-string> <key> [<value>]\n";
        return 1;
    }

    pvrResult status;

    pvrEnvHandle handle = nullptr;
    pvrSessionHandle session = nullptr;

    status = pvr_initialise(&handle);
    if (status != pvr_success) {
        std::cerr << "pvr_initialise() failed with: " << status << "\n";
        retval = 1;
        goto exit;
    }

    status = pvr_createSession(handle, &session);
    if (status != pvr_success) {
        std::cerr << "pvr_createSession() failed with: " << status << "\n";
        retval = 1;
        goto exit;
    }

    {
        std::string type(argv[1]);
        std::string key(argv[2]);

        // Set the value if needed.
        if (argc == 4) {
            std::string value(argv[3]);
            if (type == "-int") {
                status = pvr_setIntConfig(session, key.c_str(), std::stoi(value));
            } else if (type == "-decimal") {
                status = pvr_setFloatConfig(session, key.c_str(), std::stof(value));
            } else if (type == "-string") {
                status = pvr_setStringConfig(session, key.c_str(), value.c_str());
            } else {
                std::cerr << "unknown type: " << type << "\n";
                retval = 1;
                goto exit;
            }

            if (status != pvr_success) {
                std::cerr << "pvr_setConfig() failed with: " << status << "\n";
                retval = 1;
                goto exit;
            }
        }

        // Readback through the service.
        if (type == "-int") {
            std::cout << key << "=" << pvr_getIntConfig(session, key.c_str(), INT_MIN) << "\n";
        } else if (type == "-decimal") {
            std::cout << key << "=" << pvr_getFloatConfig(session, key.c_str(), NAN) << "\n";
        } else if (type == "-string") {
            char buf[256]{};
            pvr_getStringConfig(session, key.c_str(), buf, sizeof(buf));
            std::cout << key << "=" << buf << "\n";
        } else {
            std::cerr << "unknown type: " << type << "\n";
            retval = 1;
            goto exit;
        }
    }

    retval = 0;
exit:
    if (session) {
        pvr_destroySession(session);
    }
    if (handle) {
        pvr_shutdown(handle);
    }

    return 0;
}
