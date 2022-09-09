# MIT License
#
# Copyright(c) 2022 Matthieu Bucchianeri
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this softwareand associated documentation files(the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions :
#
# The above copyright noticeand this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import os
import re
import sys

# Import dependencies from the OpenXR SDK.
cur_dir = os.path.abspath(os.path.dirname(__file__))
base_dir = os.path.abspath(os.path.join(cur_dir, '..', '..'))
sdk_dir = os.path.join(base_dir, 'external', 'OpenXR-SDK-Source')
sys.path.append(os.path.join(sdk_dir, 'specification', 'scripts'))
sys.path.append(os.path.join(sdk_dir, 'src', 'scripts'))

from automatic_source_generator import AutomaticSourceOutputGenerator, AutomaticSourceGeneratorOptions
from reg import Registry
from generator import write
from xrconventions import OpenXRConventions

# Things we can configure.
EXCLUDED_API = ['xrGetInstanceProcAddr', 'xrEnumerateApiLayerProperties']
EXTENSIONS = ['XR_KHR_D3D11_enable', 'XR_KHR_D3D12_enable', 'XR_KHR_vulkan_enable', 'XR_KHR_vulkan_enable2', 'XR_KHR_opengl_enable',
              'XR_KHR_composition_layer_depth', 'XR_KHR_visibility_mask', 'XR_KHR_win32_convert_performance_counter_time']

class DispatchGenOutputGenerator(AutomaticSourceOutputGenerator):
    '''Common generator utilities and formatting.'''
    def outputGeneratedHeaderWarning(self):
        warning = '''// *********** THIS FILE IS GENERATED - DO NOT EDIT ***********'''
        write(warning, file=self.outFile)

    def outputCopywriteHeader(self):
        copyright = '''// MIT License
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
'''
        write(copyright, file=self.outFile)

    def outputGeneratedAuthorNote(self):
        pass

    def makeParametersList(self, cmd):
        parameters_list = ""
        for param in cmd.params:
            if parameters_list:
                parameters_list += ', '
            parameters_list += param.cdecl.strip()

        return parameters_list

    def makeArgumentsList(self, cmd):
        arguments_list = ""
        for param in cmd.params:
            if arguments_list:
                arguments_list += ', '
            arguments_list += param.name

        return arguments_list

class DispatchGenCppOutputGenerator(DispatchGenOutputGenerator):
    '''Generator for dispatch.gen.cpp.'''
    def beginFile(self, genOpts):
        DispatchGenOutputGenerator.beginFile(self, genOpts)
        preamble = '''#include "pch.h"

#include <runtime.h>

#include "dispatch.h"
#include "log.h"

#ifndef RUNTIME_NAMESPACE
#error Must define RUNTIME_NAMESPACE
#endif

namespace RUNTIME_NAMESPACE {
    using namespace RUNTIME_NAMESPACE::log;
'''
        write(preamble, file=self.outFile)

    def endFile(self):
        generated_wrappers = self.genWrappers()
        generated_get_instance_proc_addr = self.genGetInstanceProcAddr()

        postamble = '''} // namespace RUNTIME_NAMESPACE
'''

        contents = f'''
	// Auto-generated wrappers for the APIs.
{generated_wrappers}

	// Auto-generated dispatcher handler.
{generated_get_instance_proc_addr}

{postamble}'''

        write(contents, file=self.outFile)
        DispatchGenOutputGenerator.endFile(self)

    def genWrappers(self):
        generated = ''

        for cur_cmd in self.core_commands + self.ext_commands:
            if cur_cmd.name not in EXCLUDED_API + ['xrDestroyInstance']:
                parameters_list = self.makeParametersList(cur_cmd)
                arguments_list = self.makeArgumentsList(cur_cmd)

                if cur_cmd.return_type is not None:
                    generated += f'''
	XrResult XRAPI_CALL {cur_cmd.name}({parameters_list}) {{
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "{cur_cmd.name}");

		XrResult result;
		try {{
			result = RUNTIME_NAMESPACE::GetInstance()->{cur_cmd.name}({arguments_list});
		}} catch (std::exception& exc) {{
			TraceLoggingWriteTagged(local, "{cur_cmd.name}_Error", TLArg(exc.what(), "Error"));
			ErrorLog("{cur_cmd.name}: %s\\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}}

		TraceLoggingWriteStop(local, "{cur_cmd.name}", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {{
			ErrorLog("{cur_cmd.name} failed with %s\\n", xr::ToCString(result));
		}}

		return result;
	}}
'''
                else:
                    generated += f'''
	void XRAPI_CALL {cur_cmd.name}({parameters_list}) {{
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "{cur_cmd.name}");

		try {{
			RUNTIME_NAMESPACE::GetInstance()->{cur_cmd.name}({arguments_list});
		}} catch (std::exception& exc) {{
			TraceLoggingWriteTagged(local, "{cur_cmd.name}_Error", TLArg(exc.what(), "Error"));
			ErrorLog("{cur_cmd.name}: %s\\n", exc.what());
		}}

		TraceLoggingWriteStop(local, "{cur_cmd.name}");
	}}
'''
                
        return generated

    def genGetInstanceProcAddr(self):
        generated = '''	XrResult OpenXrApi::xrGetInstanceProcAddr(XrInstance instance, const char* name, PFN_xrVoidFunction* function) {
		const std::string apiName(name);

		if (apiName == "xrGetInstanceProcAddr") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetInstanceProcAddr);
		}
'''

        for cur_cmd in self.core_commands + self.ext_commands:
            if cur_cmd.name not in EXCLUDED_API:
                generated += f'''		else if (apiName == "{cur_cmd.name}") {{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::{cur_cmd.name});
		}}
'''

        generated += f'''		else {{
			return XR_ERROR_FUNCTION_UNSUPPORTED;
		}}

		return XR_SUCCESS;
	}}'''

        return generated


class DispatchGenHOutputGenerator(DispatchGenOutputGenerator):
    '''Generator for dispatch.gen.h.'''
    def beginFile(self, genOpts):
        DispatchGenOutputGenerator.beginFile(self, genOpts)
        preamble = '''#pragma once

#ifndef RUNTIME_NAMESPACE
#error Must define RUNTIME_NAMESPACE
#endif

namespace RUNTIME_NAMESPACE {

	class OpenXrApi {
	protected:
		OpenXrApi() = default;

	public:
		virtual ~OpenXrApi() = default;

		// Specially-handled by the auto-generated code.
		virtual XrResult xrGetInstanceProcAddr(XrInstance instance, const char* name, PFN_xrVoidFunction* function);
'''
        write(preamble, file=self.outFile)

    def endFile(self):
        generated_virtual_methods = self.genVirtualMethods()

        postamble = '''
	};

} // namespace RUNTIME_NAMESPACE
'''

        contents = f'''
		// Auto-generated entries for the requested APIs.
{generated_virtual_methods}

{postamble}'''

        write(contents, file=self.outFile)

        DispatchGenOutputGenerator.endFile(self)

    def genVirtualMethods(self):
        generated = ''

        for cur_cmd in self.core_commands + self.ext_commands:
            if cur_cmd.name not in EXCLUDED_API:
                parameters_list = self.makeParametersList(cur_cmd)
                arguments_list = self.makeArgumentsList(cur_cmd)

                if cur_cmd.return_type is not None:
                    generated += f'''		virtual XrResult {cur_cmd.name}({parameters_list}) = 0;
'''
                else:
                    generated += f'''		virtual void {cur_cmd.name}({parameters_list}) = 0;
'''

        return generated

def makeREstring(strings, default=None):
    """Turn a list of strings into a regexp string matching exactly those strings."""
    if strings or default is None:
        return '^(' + '|'.join((re.escape(s) for s in strings)) + ')$'
    return default

if __name__ == '__main__':
    registry = Registry()
    registry.loadFile(os.path.join(sdk_dir, 'specification', 'registry', 'xr.xml'))

    conventions = OpenXRConventions()
    featuresPat = '.*'
    extensionsPat = makeREstring(EXTENSIONS)

    registry.setGenerator(DispatchGenCppOutputGenerator(diagFile=None))
    registry.apiGen(AutomaticSourceGeneratorOptions(
            conventions       = conventions,
            filename          = 'dispatch.gen.cpp',
            directory         = cur_dir,
            apiname           = 'openxr',
            profile           = None,
            versions          = featuresPat,
            emitversions      = featuresPat,
            defaultExtensions = 'openxr',
            addExtensions     = None,
            removeExtensions  = None,
            emitExtensions    = extensionsPat))

    registry.setGenerator(DispatchGenHOutputGenerator(diagFile=None))
    registry.apiGen(AutomaticSourceGeneratorOptions(
            conventions       = conventions,
            filename          = 'dispatch.gen.h',
            directory         = cur_dir,
            apiname           = 'openxr',
            profile           = None,
            versions          = featuresPat,
            emitversions      = featuresPat,
            defaultExtensions = 'openxr',
            addExtensions     = None,
            removeExtensions  = None,
            emitExtensions    = extensionsPat))
