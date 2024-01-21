# MIT License
#
# Copyright(c) 2022-2024 Matthieu Bucchianeri
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
# We rewrite the trampoline for these
SPECIAL_API = ['xrDestroyInstance']
# We rewrite the trampoline and prototype for these
VERY_SPECIAL_API = ['xrGetInstanceProperties']
EXTENSIONS = ['XR_KHR_D3D11_enable', 'XR_KHR_D3D12_enable', 'XR_KHR_vulkan_enable', 'XR_KHR_vulkan_enable2', 'XR_KHR_opengl_enable',
              'XR_KHR_composition_layer_depth', 'XR_KHR_composition_layer_cylinder', 'XR_KHR_composition_layer_cube', 'XR_KHR_visibility_mask',
              'XR_KHR_win32_convert_performance_counter_time', 'XR_FB_display_refresh_rate', 'XR_EXT_hand_tracking', 'XR_EXT_hand_joints_motion_range',
              'XR_EXT_eye_gaze_interaction', 'XR_EXT_uuid', 'XR_META_headset_id', 'XR_OCULUS_audio_device_guid', 'XR_MND_headless',
              'XR_FB_eye_tracking_social', 'XR_FB_face_tracking', 'XR_FB_face_tracking2']

SILENT_ERRORS = {
    'xrSuggestInteractionProfileBindings': ['XR_ERROR_PATH_UNSUPPORTED'],
    'xrGetAudioInputDeviceGuidOculus': ['XR_ERROR_FEATURE_UNSUPPORTED'],
    'xrGetAudioOutputDeviceGuidOculus': ['XR_ERROR_FEATURE_UNSUPPORTED'],
}

class DispatchGenOutputGenerator(AutomaticSourceOutputGenerator):
    '''Common generator utilities and formatting.'''
    def outputGeneratedHeaderWarning(self):
        warning = '''// *********** THIS FILE IS GENERATED - DO NOT EDIT ***********'''
        write(warning, file=self.outFile)

    def outputCopywriteHeader(self):
        copyright = '''// MIT License
//
// Copyright(c) 2022-2024 Matthieu Bucchianeri
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
        generated_register_instance_extension = self.genRegisterInstanceExtension()

        postamble = '''} // namespace RUNTIME_NAMESPACE
'''

        contents = f'''
	// Auto-generated wrappers for the APIs.
{generated_wrappers}

	// Auto-generated dispatcher handler.
{generated_get_instance_proc_addr}

	// Auto-generated extension registration handler.
{generated_register_instance_extension}

{postamble}'''

        write(contents, file=self.outFile)
        DispatchGenOutputGenerator.endFile(self)

    def genWrappers(self):
        generated = ''

        for cur_cmd in self.core_commands + self.ext_commands:
            if cur_cmd.name not in EXCLUDED_API + SPECIAL_API + VERY_SPECIAL_API:
                parameters_list = self.makeParametersList(cur_cmd)
                arguments_list = self.makeArgumentsList(cur_cmd)

                if cur_cmd.return_type is not None:
                    silentErrors = ' && '.join([''] + [f'result != {err}' for err in SILENT_ERRORS[cur_cmd.name]]) if cur_cmd.name in SILENT_ERRORS else ''

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
		if (XR_FAILED(result){silentErrors}) {{
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
		const std::string_view apiName(name);

		if (apiName == "xrGetInstanceProcAddr") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetInstanceProcAddr);
		}
'''

        for cur_cmd in self.core_commands:
            if cur_cmd.name not in EXCLUDED_API:
                generated += f'''		else if (apiName == "{cur_cmd.name}") {{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::{cur_cmd.name});
		}}
'''

        for cur_cmd in self.ext_commands:
            if cur_cmd.name not in EXCLUDED_API:
                requirements = " && ".join([f"has_{required_ext}" for required_ext in cur_cmd.required_exts])
                generated += f'''		else if ({requirements} && apiName == "{cur_cmd.name}") {{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::{cur_cmd.name});
		}}
'''

        generated += f'''		else {{
			return XR_ERROR_FUNCTION_UNSUPPORTED;
		}}

		return XR_SUCCESS;
	}}'''

        return generated

    def genRegisterInstanceExtension(self):
        generated = '''	void OpenXrApi::registerInstanceExtension(const std::string& extensionName) {
		if (false) {
		}
'''

        for extension in EXTENSIONS:
                generated += f'''		else if (extensionName == "{extension}") {{
			has_{extension} = true;
		}}
'''

        generated += f'''
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
		virtual XrResult xrGetInstanceProperties(XrInstance instance, XrInstanceProperties* instanceProperties, void *returnAddress) = 0;
'''
        write(preamble, file=self.outFile)

    def endFile(self):
        generated_virtual_methods = self.genVirtualMethods()

        generated_extensions_properties = "\n".join([f'''		bool has_{extension}{{false}};''' for extension in EXTENSIONS])

        postamble = '''
	};

} // namespace RUNTIME_NAMESPACE
'''

        contents = f'''
		// Auto-generated entries for the requested APIs.
{generated_virtual_methods}

	protected:
		// Specially-handled by the auto-generated code.
		virtual void registerInstanceExtension(const std::string& extensionName);

		// Auto-generated extension properties.
{generated_extensions_properties}

{postamble}'''

        write(contents, file=self.outFile)

        DispatchGenOutputGenerator.endFile(self)

    def genVirtualMethods(self):
        generated = ''

        for cur_cmd in self.core_commands + self.ext_commands:
            if cur_cmd.name not in EXCLUDED_API + VERY_SPECIAL_API:
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
    conventions = OpenXRConventions()
    featuresPat = '.*'
    extensionsPat = makeREstring(EXTENSIONS)

    registry = Registry(DispatchGenCppOutputGenerator(diagFile=None),
                        AutomaticSourceGeneratorOptions(conventions       = conventions,
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
    registry.loadFile(os.path.join(sdk_dir, 'specification', 'registry', 'xr.xml'))
    registry.apiGen()

    registry = Registry(DispatchGenHOutputGenerator(diagFile=None),
                        AutomaticSourceGeneratorOptions(conventions       = conventions,
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
    registry.loadFile(os.path.join(sdk_dir, 'specification', 'registry', 'xr.xml'))
    registry.apiGen()
