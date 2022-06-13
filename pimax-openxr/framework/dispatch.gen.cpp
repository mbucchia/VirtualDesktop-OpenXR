// *********** THIS FILE IS GENERATED - DO NOT EDIT ***********
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

#include <runtime.h>

#include "dispatch.h"
#include "log.h"

#ifndef RUNTIME_NAMESPACE
#error Must define RUNTIME_NAMESPACE
#endif

using namespace RUNTIME_NAMESPACE::log;

namespace RUNTIME_NAMESPACE
{

	// Auto-generated wrappers for the APIs.

	XrResult XRAPI_CALL xrEnumerateInstanceExtensionProperties(const char* layerName, uint32_t propertyCapacityInput, uint32_t* propertyCountOutput, XrExtensionProperties* properties)
	{
		TraceLoggingWrite(g_traceProvider, "xrEnumerateInstanceExtensionProperties");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrEnumerateInstanceExtensionProperties(layerName, propertyCapacityInput, propertyCountOutput, properties);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrEnumerateInstanceExtensionProperties_Error", TLArg(exc.what(), "Error"));
			Log("xrEnumerateInstanceExtensionProperties: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrEnumerateInstanceExtensionProperties_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrCreateInstance(const XrInstanceCreateInfo* createInfo, XrInstance* instance)
	{
		TraceLoggingWrite(g_traceProvider, "xrCreateInstance");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrCreateInstance(createInfo, instance);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrCreateInstance_Error", TLArg(exc.what(), "Error"));
			Log("xrCreateInstance: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrCreateInstance_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrGetInstanceProperties(XrInstance instance, XrInstanceProperties* instanceProperties)
	{
		TraceLoggingWrite(g_traceProvider, "xrGetInstanceProperties");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetInstanceProperties(instance, instanceProperties);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrGetInstanceProperties_Error", TLArg(exc.what(), "Error"));
			Log("xrGetInstanceProperties: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrGetInstanceProperties_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrPollEvent(XrInstance instance, XrEventDataBuffer* eventData)
	{
		TraceLoggingWrite(g_traceProvider, "xrPollEvent");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrPollEvent(instance, eventData);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrPollEvent_Error", TLArg(exc.what(), "Error"));
			Log("xrPollEvent: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrPollEvent_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrResultToString(XrInstance instance, XrResult value, char buffer[XR_MAX_RESULT_STRING_SIZE])
	{
		TraceLoggingWrite(g_traceProvider, "xrResultToString");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrResultToString(instance, value, buffer);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrResultToString_Error", TLArg(exc.what(), "Error"));
			Log("xrResultToString: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrResultToString_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrStructureTypeToString(XrInstance instance, XrStructureType value, char buffer[XR_MAX_STRUCTURE_NAME_SIZE])
	{
		TraceLoggingWrite(g_traceProvider, "xrStructureTypeToString");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrStructureTypeToString(instance, value, buffer);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrStructureTypeToString_Error", TLArg(exc.what(), "Error"));
			Log("xrStructureTypeToString: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrStructureTypeToString_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrGetSystem(XrInstance instance, const XrSystemGetInfo* getInfo, XrSystemId* systemId)
	{
		TraceLoggingWrite(g_traceProvider, "xrGetSystem");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetSystem(instance, getInfo, systemId);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrGetSystem_Error", TLArg(exc.what(), "Error"));
			Log("xrGetSystem: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrGetSystem_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrGetSystemProperties(XrInstance instance, XrSystemId systemId, XrSystemProperties* properties)
	{
		TraceLoggingWrite(g_traceProvider, "xrGetSystemProperties");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetSystemProperties(instance, systemId, properties);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrGetSystemProperties_Error", TLArg(exc.what(), "Error"));
			Log("xrGetSystemProperties: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrGetSystemProperties_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrEnumerateEnvironmentBlendModes(XrInstance instance, XrSystemId systemId, XrViewConfigurationType viewConfigurationType, uint32_t environmentBlendModeCapacityInput, uint32_t* environmentBlendModeCountOutput, XrEnvironmentBlendMode* environmentBlendModes)
	{
		TraceLoggingWrite(g_traceProvider, "xrEnumerateEnvironmentBlendModes");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrEnumerateEnvironmentBlendModes(instance, systemId, viewConfigurationType, environmentBlendModeCapacityInput, environmentBlendModeCountOutput, environmentBlendModes);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrEnumerateEnvironmentBlendModes_Error", TLArg(exc.what(), "Error"));
			Log("xrEnumerateEnvironmentBlendModes: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrEnumerateEnvironmentBlendModes_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrCreateSession(XrInstance instance, const XrSessionCreateInfo* createInfo, XrSession* session)
	{
		TraceLoggingWrite(g_traceProvider, "xrCreateSession");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrCreateSession(instance, createInfo, session);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrCreateSession_Error", TLArg(exc.what(), "Error"));
			Log("xrCreateSession: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrCreateSession_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrDestroySession(XrSession session)
	{
		TraceLoggingWrite(g_traceProvider, "xrDestroySession");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrDestroySession(session);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrDestroySession_Error", TLArg(exc.what(), "Error"));
			Log("xrDestroySession: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrDestroySession_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrEnumerateReferenceSpaces(XrSession session, uint32_t spaceCapacityInput, uint32_t* spaceCountOutput, XrReferenceSpaceType* spaces)
	{
		TraceLoggingWrite(g_traceProvider, "xrEnumerateReferenceSpaces");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrEnumerateReferenceSpaces(session, spaceCapacityInput, spaceCountOutput, spaces);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrEnumerateReferenceSpaces_Error", TLArg(exc.what(), "Error"));
			Log("xrEnumerateReferenceSpaces: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrEnumerateReferenceSpaces_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrCreateReferenceSpace(XrSession session, const XrReferenceSpaceCreateInfo* createInfo, XrSpace* space)
	{
		TraceLoggingWrite(g_traceProvider, "xrCreateReferenceSpace");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrCreateReferenceSpace(session, createInfo, space);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrCreateReferenceSpace_Error", TLArg(exc.what(), "Error"));
			Log("xrCreateReferenceSpace: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrCreateReferenceSpace_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrGetReferenceSpaceBoundsRect(XrSession session, XrReferenceSpaceType referenceSpaceType, XrExtent2Df* bounds)
	{
		TraceLoggingWrite(g_traceProvider, "xrGetReferenceSpaceBoundsRect");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetReferenceSpaceBoundsRect(session, referenceSpaceType, bounds);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrGetReferenceSpaceBoundsRect_Error", TLArg(exc.what(), "Error"));
			Log("xrGetReferenceSpaceBoundsRect: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrGetReferenceSpaceBoundsRect_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrCreateActionSpace(XrSession session, const XrActionSpaceCreateInfo* createInfo, XrSpace* space)
	{
		TraceLoggingWrite(g_traceProvider, "xrCreateActionSpace");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrCreateActionSpace(session, createInfo, space);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrCreateActionSpace_Error", TLArg(exc.what(), "Error"));
			Log("xrCreateActionSpace: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrCreateActionSpace_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrLocateSpace(XrSpace space, XrSpace baseSpace, XrTime time, XrSpaceLocation* location)
	{
		TraceLoggingWrite(g_traceProvider, "xrLocateSpace");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrLocateSpace(space, baseSpace, time, location);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrLocateSpace_Error", TLArg(exc.what(), "Error"));
			Log("xrLocateSpace: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrLocateSpace_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrDestroySpace(XrSpace space)
	{
		TraceLoggingWrite(g_traceProvider, "xrDestroySpace");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrDestroySpace(space);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrDestroySpace_Error", TLArg(exc.what(), "Error"));
			Log("xrDestroySpace: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrDestroySpace_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrEnumerateViewConfigurations(XrInstance instance, XrSystemId systemId, uint32_t viewConfigurationTypeCapacityInput, uint32_t* viewConfigurationTypeCountOutput, XrViewConfigurationType* viewConfigurationTypes)
	{
		TraceLoggingWrite(g_traceProvider, "xrEnumerateViewConfigurations");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrEnumerateViewConfigurations(instance, systemId, viewConfigurationTypeCapacityInput, viewConfigurationTypeCountOutput, viewConfigurationTypes);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrEnumerateViewConfigurations_Error", TLArg(exc.what(), "Error"));
			Log("xrEnumerateViewConfigurations: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrEnumerateViewConfigurations_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrGetViewConfigurationProperties(XrInstance instance, XrSystemId systemId, XrViewConfigurationType viewConfigurationType, XrViewConfigurationProperties* configurationProperties)
	{
		TraceLoggingWrite(g_traceProvider, "xrGetViewConfigurationProperties");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetViewConfigurationProperties(instance, systemId, viewConfigurationType, configurationProperties);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrGetViewConfigurationProperties_Error", TLArg(exc.what(), "Error"));
			Log("xrGetViewConfigurationProperties: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrGetViewConfigurationProperties_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrEnumerateViewConfigurationViews(XrInstance instance, XrSystemId systemId, XrViewConfigurationType viewConfigurationType, uint32_t viewCapacityInput, uint32_t* viewCountOutput, XrViewConfigurationView* views)
	{
		TraceLoggingWrite(g_traceProvider, "xrEnumerateViewConfigurationViews");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrEnumerateViewConfigurationViews(instance, systemId, viewConfigurationType, viewCapacityInput, viewCountOutput, views);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrEnumerateViewConfigurationViews_Error", TLArg(exc.what(), "Error"));
			Log("xrEnumerateViewConfigurationViews: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrEnumerateViewConfigurationViews_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrEnumerateSwapchainFormats(XrSession session, uint32_t formatCapacityInput, uint32_t* formatCountOutput, int64_t* formats)
	{
		TraceLoggingWrite(g_traceProvider, "xrEnumerateSwapchainFormats");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrEnumerateSwapchainFormats(session, formatCapacityInput, formatCountOutput, formats);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrEnumerateSwapchainFormats_Error", TLArg(exc.what(), "Error"));
			Log("xrEnumerateSwapchainFormats: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrEnumerateSwapchainFormats_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrCreateSwapchain(XrSession session, const XrSwapchainCreateInfo* createInfo, XrSwapchain* swapchain)
	{
		TraceLoggingWrite(g_traceProvider, "xrCreateSwapchain");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrCreateSwapchain(session, createInfo, swapchain);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrCreateSwapchain_Error", TLArg(exc.what(), "Error"));
			Log("xrCreateSwapchain: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrCreateSwapchain_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrDestroySwapchain(XrSwapchain swapchain)
	{
		TraceLoggingWrite(g_traceProvider, "xrDestroySwapchain");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrDestroySwapchain(swapchain);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrDestroySwapchain_Error", TLArg(exc.what(), "Error"));
			Log("xrDestroySwapchain: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrDestroySwapchain_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrEnumerateSwapchainImages(XrSwapchain swapchain, uint32_t imageCapacityInput, uint32_t* imageCountOutput, XrSwapchainImageBaseHeader* images)
	{
		TraceLoggingWrite(g_traceProvider, "xrEnumerateSwapchainImages");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrEnumerateSwapchainImages(swapchain, imageCapacityInput, imageCountOutput, images);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrEnumerateSwapchainImages_Error", TLArg(exc.what(), "Error"));
			Log("xrEnumerateSwapchainImages: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrEnumerateSwapchainImages_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrAcquireSwapchainImage(XrSwapchain swapchain, const XrSwapchainImageAcquireInfo* acquireInfo, uint32_t* index)
	{
		TraceLoggingWrite(g_traceProvider, "xrAcquireSwapchainImage");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrAcquireSwapchainImage(swapchain, acquireInfo, index);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrAcquireSwapchainImage_Error", TLArg(exc.what(), "Error"));
			Log("xrAcquireSwapchainImage: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrAcquireSwapchainImage_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrWaitSwapchainImage(XrSwapchain swapchain, const XrSwapchainImageWaitInfo* waitInfo)
	{
		TraceLoggingWrite(g_traceProvider, "xrWaitSwapchainImage");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrWaitSwapchainImage(swapchain, waitInfo);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrWaitSwapchainImage_Error", TLArg(exc.what(), "Error"));
			Log("xrWaitSwapchainImage: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrWaitSwapchainImage_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrReleaseSwapchainImage(XrSwapchain swapchain, const XrSwapchainImageReleaseInfo* releaseInfo)
	{
		TraceLoggingWrite(g_traceProvider, "xrReleaseSwapchainImage");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrReleaseSwapchainImage(swapchain, releaseInfo);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrReleaseSwapchainImage_Error", TLArg(exc.what(), "Error"));
			Log("xrReleaseSwapchainImage: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrReleaseSwapchainImage_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrBeginSession(XrSession session, const XrSessionBeginInfo* beginInfo)
	{
		TraceLoggingWrite(g_traceProvider, "xrBeginSession");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrBeginSession(session, beginInfo);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrBeginSession_Error", TLArg(exc.what(), "Error"));
			Log("xrBeginSession: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrBeginSession_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrEndSession(XrSession session)
	{
		TraceLoggingWrite(g_traceProvider, "xrEndSession");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrEndSession(session);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrEndSession_Error", TLArg(exc.what(), "Error"));
			Log("xrEndSession: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrEndSession_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrRequestExitSession(XrSession session)
	{
		TraceLoggingWrite(g_traceProvider, "xrRequestExitSession");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrRequestExitSession(session);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrRequestExitSession_Error", TLArg(exc.what(), "Error"));
			Log("xrRequestExitSession: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrRequestExitSession_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrWaitFrame(XrSession session, const XrFrameWaitInfo* frameWaitInfo, XrFrameState* frameState)
	{
		TraceLoggingWrite(g_traceProvider, "xrWaitFrame");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrWaitFrame(session, frameWaitInfo, frameState);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrWaitFrame_Error", TLArg(exc.what(), "Error"));
			Log("xrWaitFrame: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrWaitFrame_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrBeginFrame(XrSession session, const XrFrameBeginInfo* frameBeginInfo)
	{
		TraceLoggingWrite(g_traceProvider, "xrBeginFrame");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrBeginFrame(session, frameBeginInfo);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrBeginFrame_Error", TLArg(exc.what(), "Error"));
			Log("xrBeginFrame: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrBeginFrame_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrEndFrame(XrSession session, const XrFrameEndInfo* frameEndInfo)
	{
		TraceLoggingWrite(g_traceProvider, "xrEndFrame");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrEndFrame(session, frameEndInfo);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrEndFrame_Error", TLArg(exc.what(), "Error"));
			Log("xrEndFrame: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrEndFrame_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrLocateViews(XrSession session, const XrViewLocateInfo* viewLocateInfo, XrViewState* viewState, uint32_t viewCapacityInput, uint32_t* viewCountOutput, XrView* views)
	{
		TraceLoggingWrite(g_traceProvider, "xrLocateViews");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrLocateViews(session, viewLocateInfo, viewState, viewCapacityInput, viewCountOutput, views);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrLocateViews_Error", TLArg(exc.what(), "Error"));
			Log("xrLocateViews: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrLocateViews_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrStringToPath(XrInstance instance, const char* pathString, XrPath* path)
	{
		TraceLoggingWrite(g_traceProvider, "xrStringToPath");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrStringToPath(instance, pathString, path);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrStringToPath_Error", TLArg(exc.what(), "Error"));
			Log("xrStringToPath: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrStringToPath_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrPathToString(XrInstance instance, XrPath path, uint32_t bufferCapacityInput, uint32_t* bufferCountOutput, char* buffer)
	{
		TraceLoggingWrite(g_traceProvider, "xrPathToString");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrPathToString(instance, path, bufferCapacityInput, bufferCountOutput, buffer);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrPathToString_Error", TLArg(exc.what(), "Error"));
			Log("xrPathToString: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrPathToString_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrCreateActionSet(XrInstance instance, const XrActionSetCreateInfo* createInfo, XrActionSet* actionSet)
	{
		TraceLoggingWrite(g_traceProvider, "xrCreateActionSet");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrCreateActionSet(instance, createInfo, actionSet);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrCreateActionSet_Error", TLArg(exc.what(), "Error"));
			Log("xrCreateActionSet: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrCreateActionSet_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrDestroyActionSet(XrActionSet actionSet)
	{
		TraceLoggingWrite(g_traceProvider, "xrDestroyActionSet");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrDestroyActionSet(actionSet);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrDestroyActionSet_Error", TLArg(exc.what(), "Error"));
			Log("xrDestroyActionSet: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrDestroyActionSet_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrCreateAction(XrActionSet actionSet, const XrActionCreateInfo* createInfo, XrAction* action)
	{
		TraceLoggingWrite(g_traceProvider, "xrCreateAction");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrCreateAction(actionSet, createInfo, action);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrCreateAction_Error", TLArg(exc.what(), "Error"));
			Log("xrCreateAction: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrCreateAction_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrDestroyAction(XrAction action)
	{
		TraceLoggingWrite(g_traceProvider, "xrDestroyAction");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrDestroyAction(action);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrDestroyAction_Error", TLArg(exc.what(), "Error"));
			Log("xrDestroyAction: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrDestroyAction_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrSuggestInteractionProfileBindings(XrInstance instance, const XrInteractionProfileSuggestedBinding* suggestedBindings)
	{
		TraceLoggingWrite(g_traceProvider, "xrSuggestInteractionProfileBindings");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrSuggestInteractionProfileBindings(instance, suggestedBindings);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrSuggestInteractionProfileBindings_Error", TLArg(exc.what(), "Error"));
			Log("xrSuggestInteractionProfileBindings: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrSuggestInteractionProfileBindings_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrAttachSessionActionSets(XrSession session, const XrSessionActionSetsAttachInfo* attachInfo)
	{
		TraceLoggingWrite(g_traceProvider, "xrAttachSessionActionSets");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrAttachSessionActionSets(session, attachInfo);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrAttachSessionActionSets_Error", TLArg(exc.what(), "Error"));
			Log("xrAttachSessionActionSets: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrAttachSessionActionSets_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrGetCurrentInteractionProfile(XrSession session, XrPath topLevelUserPath, XrInteractionProfileState* interactionProfile)
	{
		TraceLoggingWrite(g_traceProvider, "xrGetCurrentInteractionProfile");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetCurrentInteractionProfile(session, topLevelUserPath, interactionProfile);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrGetCurrentInteractionProfile_Error", TLArg(exc.what(), "Error"));
			Log("xrGetCurrentInteractionProfile: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrGetCurrentInteractionProfile_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrGetActionStateBoolean(XrSession session, const XrActionStateGetInfo* getInfo, XrActionStateBoolean* state)
	{
		TraceLoggingWrite(g_traceProvider, "xrGetActionStateBoolean");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetActionStateBoolean(session, getInfo, state);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrGetActionStateBoolean_Error", TLArg(exc.what(), "Error"));
			Log("xrGetActionStateBoolean: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrGetActionStateBoolean_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrGetActionStateFloat(XrSession session, const XrActionStateGetInfo* getInfo, XrActionStateFloat* state)
	{
		TraceLoggingWrite(g_traceProvider, "xrGetActionStateFloat");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetActionStateFloat(session, getInfo, state);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrGetActionStateFloat_Error", TLArg(exc.what(), "Error"));
			Log("xrGetActionStateFloat: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrGetActionStateFloat_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrGetActionStateVector2f(XrSession session, const XrActionStateGetInfo* getInfo, XrActionStateVector2f* state)
	{
		TraceLoggingWrite(g_traceProvider, "xrGetActionStateVector2f");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetActionStateVector2f(session, getInfo, state);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrGetActionStateVector2f_Error", TLArg(exc.what(), "Error"));
			Log("xrGetActionStateVector2f: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrGetActionStateVector2f_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrGetActionStatePose(XrSession session, const XrActionStateGetInfo* getInfo, XrActionStatePose* state)
	{
		TraceLoggingWrite(g_traceProvider, "xrGetActionStatePose");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetActionStatePose(session, getInfo, state);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrGetActionStatePose_Error", TLArg(exc.what(), "Error"));
			Log("xrGetActionStatePose: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrGetActionStatePose_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrSyncActions(XrSession session, const XrActionsSyncInfo* syncInfo)
	{
		TraceLoggingWrite(g_traceProvider, "xrSyncActions");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrSyncActions(session, syncInfo);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrSyncActions_Error", TLArg(exc.what(), "Error"));
			Log("xrSyncActions: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrSyncActions_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrEnumerateBoundSourcesForAction(XrSession session, const XrBoundSourcesForActionEnumerateInfo* enumerateInfo, uint32_t sourceCapacityInput, uint32_t* sourceCountOutput, XrPath* sources)
	{
		TraceLoggingWrite(g_traceProvider, "xrEnumerateBoundSourcesForAction");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrEnumerateBoundSourcesForAction(session, enumerateInfo, sourceCapacityInput, sourceCountOutput, sources);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrEnumerateBoundSourcesForAction_Error", TLArg(exc.what(), "Error"));
			Log("xrEnumerateBoundSourcesForAction: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrEnumerateBoundSourcesForAction_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrGetInputSourceLocalizedName(XrSession session, const XrInputSourceLocalizedNameGetInfo* getInfo, uint32_t bufferCapacityInput, uint32_t* bufferCountOutput, char* buffer)
	{
		TraceLoggingWrite(g_traceProvider, "xrGetInputSourceLocalizedName");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetInputSourceLocalizedName(session, getInfo, bufferCapacityInput, bufferCountOutput, buffer);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrGetInputSourceLocalizedName_Error", TLArg(exc.what(), "Error"));
			Log("xrGetInputSourceLocalizedName: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrGetInputSourceLocalizedName_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrApplyHapticFeedback(XrSession session, const XrHapticActionInfo* hapticActionInfo, const XrHapticBaseHeader* hapticFeedback)
	{
		TraceLoggingWrite(g_traceProvider, "xrApplyHapticFeedback");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrApplyHapticFeedback(session, hapticActionInfo, hapticFeedback);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrApplyHapticFeedback_Error", TLArg(exc.what(), "Error"));
			Log("xrApplyHapticFeedback: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrApplyHapticFeedback_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrStopHapticFeedback(XrSession session, const XrHapticActionInfo* hapticActionInfo)
	{
		TraceLoggingWrite(g_traceProvider, "xrStopHapticFeedback");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrStopHapticFeedback(session, hapticActionInfo);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrStopHapticFeedback_Error", TLArg(exc.what(), "Error"));
			Log("xrStopHapticFeedback: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrStopHapticFeedback_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrGetVulkanInstanceExtensionsKHR(XrInstance instance, XrSystemId systemId, uint32_t bufferCapacityInput, uint32_t* bufferCountOutput, char* buffer)
	{
		TraceLoggingWrite(g_traceProvider, "xrGetVulkanInstanceExtensionsKHR");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetVulkanInstanceExtensionsKHR(instance, systemId, bufferCapacityInput, bufferCountOutput, buffer);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrGetVulkanInstanceExtensionsKHR_Error", TLArg(exc.what(), "Error"));
			Log("xrGetVulkanInstanceExtensionsKHR: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrGetVulkanInstanceExtensionsKHR_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrGetVulkanDeviceExtensionsKHR(XrInstance instance, XrSystemId systemId, uint32_t bufferCapacityInput, uint32_t* bufferCountOutput, char* buffer)
	{
		TraceLoggingWrite(g_traceProvider, "xrGetVulkanDeviceExtensionsKHR");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetVulkanDeviceExtensionsKHR(instance, systemId, bufferCapacityInput, bufferCountOutput, buffer);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrGetVulkanDeviceExtensionsKHR_Error", TLArg(exc.what(), "Error"));
			Log("xrGetVulkanDeviceExtensionsKHR: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrGetVulkanDeviceExtensionsKHR_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrGetVulkanGraphicsDeviceKHR(XrInstance instance, XrSystemId systemId, VkInstance vkInstance, VkPhysicalDevice* vkPhysicalDevice)
	{
		TraceLoggingWrite(g_traceProvider, "xrGetVulkanGraphicsDeviceKHR");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetVulkanGraphicsDeviceKHR(instance, systemId, vkInstance, vkPhysicalDevice);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrGetVulkanGraphicsDeviceKHR_Error", TLArg(exc.what(), "Error"));
			Log("xrGetVulkanGraphicsDeviceKHR: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrGetVulkanGraphicsDeviceKHR_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrGetVulkanGraphicsRequirementsKHR(XrInstance instance, XrSystemId systemId, XrGraphicsRequirementsVulkanKHR* graphicsRequirements)
	{
		TraceLoggingWrite(g_traceProvider, "xrGetVulkanGraphicsRequirementsKHR");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetVulkanGraphicsRequirementsKHR(instance, systemId, graphicsRequirements);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrGetVulkanGraphicsRequirementsKHR_Error", TLArg(exc.what(), "Error"));
			Log("xrGetVulkanGraphicsRequirementsKHR: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrGetVulkanGraphicsRequirementsKHR_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrGetD3D11GraphicsRequirementsKHR(XrInstance instance, XrSystemId systemId, XrGraphicsRequirementsD3D11KHR* graphicsRequirements)
	{
		TraceLoggingWrite(g_traceProvider, "xrGetD3D11GraphicsRequirementsKHR");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetD3D11GraphicsRequirementsKHR(instance, systemId, graphicsRequirements);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrGetD3D11GraphicsRequirementsKHR_Error", TLArg(exc.what(), "Error"));
			Log("xrGetD3D11GraphicsRequirementsKHR: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrGetD3D11GraphicsRequirementsKHR_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrGetD3D12GraphicsRequirementsKHR(XrInstance instance, XrSystemId systemId, XrGraphicsRequirementsD3D12KHR* graphicsRequirements)
	{
		TraceLoggingWrite(g_traceProvider, "xrGetD3D12GraphicsRequirementsKHR");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetD3D12GraphicsRequirementsKHR(instance, systemId, graphicsRequirements);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrGetD3D12GraphicsRequirementsKHR_Error", TLArg(exc.what(), "Error"));
			Log("xrGetD3D12GraphicsRequirementsKHR: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrGetD3D12GraphicsRequirementsKHR_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrGetVisibilityMaskKHR(XrSession session, XrViewConfigurationType viewConfigurationType, uint32_t viewIndex, XrVisibilityMaskTypeKHR visibilityMaskType, XrVisibilityMaskKHR* visibilityMask)
	{
		TraceLoggingWrite(g_traceProvider, "xrGetVisibilityMaskKHR");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetVisibilityMaskKHR(session, viewConfigurationType, viewIndex, visibilityMaskType, visibilityMask);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrGetVisibilityMaskKHR_Error", TLArg(exc.what(), "Error"));
			Log("xrGetVisibilityMaskKHR: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrGetVisibilityMaskKHR_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrConvertWin32PerformanceCounterToTimeKHR(XrInstance instance, const LARGE_INTEGER* performanceCounter, XrTime* time)
	{
		TraceLoggingWrite(g_traceProvider, "xrConvertWin32PerformanceCounterToTimeKHR");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrConvertWin32PerformanceCounterToTimeKHR(instance, performanceCounter, time);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrConvertWin32PerformanceCounterToTimeKHR_Error", TLArg(exc.what(), "Error"));
			Log("xrConvertWin32PerformanceCounterToTimeKHR: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrConvertWin32PerformanceCounterToTimeKHR_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrConvertTimeToWin32PerformanceCounterKHR(XrInstance instance, XrTime time, LARGE_INTEGER* performanceCounter)
	{
		TraceLoggingWrite(g_traceProvider, "xrConvertTimeToWin32PerformanceCounterKHR");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrConvertTimeToWin32PerformanceCounterKHR(instance, time, performanceCounter);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrConvertTimeToWin32PerformanceCounterKHR_Error", TLArg(exc.what(), "Error"));
			Log("xrConvertTimeToWin32PerformanceCounterKHR: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrConvertTimeToWin32PerformanceCounterKHR_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrCreateVulkanInstanceKHR(XrInstance instance, const XrVulkanInstanceCreateInfoKHR* createInfo, VkInstance* vulkanInstance, VkResult* vulkanResult)
	{
		TraceLoggingWrite(g_traceProvider, "xrCreateVulkanInstanceKHR");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrCreateVulkanInstanceKHR(instance, createInfo, vulkanInstance, vulkanResult);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrCreateVulkanInstanceKHR_Error", TLArg(exc.what(), "Error"));
			Log("xrCreateVulkanInstanceKHR: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrCreateVulkanInstanceKHR_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrCreateVulkanDeviceKHR(XrInstance instance, const XrVulkanDeviceCreateInfoKHR* createInfo, VkDevice* vulkanDevice, VkResult* vulkanResult)
	{
		TraceLoggingWrite(g_traceProvider, "xrCreateVulkanDeviceKHR");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrCreateVulkanDeviceKHR(instance, createInfo, vulkanDevice, vulkanResult);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrCreateVulkanDeviceKHR_Error", TLArg(exc.what(), "Error"));
			Log("xrCreateVulkanDeviceKHR: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrCreateVulkanDeviceKHR_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrGetVulkanGraphicsDevice2KHR(XrInstance instance, const XrVulkanGraphicsDeviceGetInfoKHR* getInfo, VkPhysicalDevice* vulkanPhysicalDevice)
	{
		TraceLoggingWrite(g_traceProvider, "xrGetVulkanGraphicsDevice2KHR");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetVulkanGraphicsDevice2KHR(instance, getInfo, vulkanPhysicalDevice);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrGetVulkanGraphicsDevice2KHR_Error", TLArg(exc.what(), "Error"));
			Log("xrGetVulkanGraphicsDevice2KHR: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrGetVulkanGraphicsDevice2KHR_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}

	XrResult XRAPI_CALL xrGetVulkanGraphicsRequirements2KHR(XrInstance instance, XrSystemId systemId, XrGraphicsRequirementsVulkanKHR* graphicsRequirements)
	{
		TraceLoggingWrite(g_traceProvider, "xrGetVulkanGraphicsRequirements2KHR");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetVulkanGraphicsRequirements2KHR(instance, systemId, graphicsRequirements);
		}
		catch (std::exception& exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrGetVulkanGraphicsRequirements2KHR_Error", TLArg(exc.what(), "Error"));
			Log("xrGetVulkanGraphicsRequirements2KHR: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrGetVulkanGraphicsRequirements2KHR_Result", TLArg(xr::ToCString(result), "Result"));

		return result;
	}


	// Auto-generated dispatcher handler.
	XrResult OpenXrApi::xrGetInstanceProcAddr(XrInstance instance, const char* name, PFN_xrVoidFunction* function)
	{
		const std::string apiName(name);

		if (apiName == "xrGetInstanceProcAddr")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetInstanceProcAddr);
		}
		else if (apiName == "xrEnumerateInstanceExtensionProperties")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrEnumerateInstanceExtensionProperties);
		}
		else if (apiName == "xrCreateInstance")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrCreateInstance);
		}
		else if (apiName == "xrDestroyInstance")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrDestroyInstance);
		}
		else if (apiName == "xrGetInstanceProperties")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetInstanceProperties);
		}
		else if (apiName == "xrPollEvent")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrPollEvent);
		}
		else if (apiName == "xrResultToString")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrResultToString);
		}
		else if (apiName == "xrStructureTypeToString")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrStructureTypeToString);
		}
		else if (apiName == "xrGetSystem")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetSystem);
		}
		else if (apiName == "xrGetSystemProperties")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetSystemProperties);
		}
		else if (apiName == "xrEnumerateEnvironmentBlendModes")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrEnumerateEnvironmentBlendModes);
		}
		else if (apiName == "xrCreateSession")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrCreateSession);
		}
		else if (apiName == "xrDestroySession")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrDestroySession);
		}
		else if (apiName == "xrEnumerateReferenceSpaces")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrEnumerateReferenceSpaces);
		}
		else if (apiName == "xrCreateReferenceSpace")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrCreateReferenceSpace);
		}
		else if (apiName == "xrGetReferenceSpaceBoundsRect")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetReferenceSpaceBoundsRect);
		}
		else if (apiName == "xrCreateActionSpace")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrCreateActionSpace);
		}
		else if (apiName == "xrLocateSpace")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrLocateSpace);
		}
		else if (apiName == "xrDestroySpace")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrDestroySpace);
		}
		else if (apiName == "xrEnumerateViewConfigurations")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrEnumerateViewConfigurations);
		}
		else if (apiName == "xrGetViewConfigurationProperties")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetViewConfigurationProperties);
		}
		else if (apiName == "xrEnumerateViewConfigurationViews")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrEnumerateViewConfigurationViews);
		}
		else if (apiName == "xrEnumerateSwapchainFormats")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrEnumerateSwapchainFormats);
		}
		else if (apiName == "xrCreateSwapchain")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrCreateSwapchain);
		}
		else if (apiName == "xrDestroySwapchain")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrDestroySwapchain);
		}
		else if (apiName == "xrEnumerateSwapchainImages")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrEnumerateSwapchainImages);
		}
		else if (apiName == "xrAcquireSwapchainImage")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrAcquireSwapchainImage);
		}
		else if (apiName == "xrWaitSwapchainImage")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrWaitSwapchainImage);
		}
		else if (apiName == "xrReleaseSwapchainImage")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrReleaseSwapchainImage);
		}
		else if (apiName == "xrBeginSession")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrBeginSession);
		}
		else if (apiName == "xrEndSession")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrEndSession);
		}
		else if (apiName == "xrRequestExitSession")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrRequestExitSession);
		}
		else if (apiName == "xrWaitFrame")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrWaitFrame);
		}
		else if (apiName == "xrBeginFrame")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrBeginFrame);
		}
		else if (apiName == "xrEndFrame")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrEndFrame);
		}
		else if (apiName == "xrLocateViews")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrLocateViews);
		}
		else if (apiName == "xrStringToPath")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrStringToPath);
		}
		else if (apiName == "xrPathToString")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrPathToString);
		}
		else if (apiName == "xrCreateActionSet")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrCreateActionSet);
		}
		else if (apiName == "xrDestroyActionSet")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrDestroyActionSet);
		}
		else if (apiName == "xrCreateAction")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrCreateAction);
		}
		else if (apiName == "xrDestroyAction")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrDestroyAction);
		}
		else if (apiName == "xrSuggestInteractionProfileBindings")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrSuggestInteractionProfileBindings);
		}
		else if (apiName == "xrAttachSessionActionSets")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrAttachSessionActionSets);
		}
		else if (apiName == "xrGetCurrentInteractionProfile")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetCurrentInteractionProfile);
		}
		else if (apiName == "xrGetActionStateBoolean")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetActionStateBoolean);
		}
		else if (apiName == "xrGetActionStateFloat")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetActionStateFloat);
		}
		else if (apiName == "xrGetActionStateVector2f")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetActionStateVector2f);
		}
		else if (apiName == "xrGetActionStatePose")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetActionStatePose);
		}
		else if (apiName == "xrSyncActions")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrSyncActions);
		}
		else if (apiName == "xrEnumerateBoundSourcesForAction")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrEnumerateBoundSourcesForAction);
		}
		else if (apiName == "xrGetInputSourceLocalizedName")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetInputSourceLocalizedName);
		}
		else if (apiName == "xrApplyHapticFeedback")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrApplyHapticFeedback);
		}
		else if (apiName == "xrStopHapticFeedback")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrStopHapticFeedback);
		}
		else if (apiName == "xrGetVulkanInstanceExtensionsKHR")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetVulkanInstanceExtensionsKHR);
		}
		else if (apiName == "xrGetVulkanDeviceExtensionsKHR")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetVulkanDeviceExtensionsKHR);
		}
		else if (apiName == "xrGetVulkanGraphicsDeviceKHR")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetVulkanGraphicsDeviceKHR);
		}
		else if (apiName == "xrGetVulkanGraphicsRequirementsKHR")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetVulkanGraphicsRequirementsKHR);
		}
		else if (apiName == "xrGetD3D11GraphicsRequirementsKHR")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetD3D11GraphicsRequirementsKHR);
		}
		else if (apiName == "xrGetD3D12GraphicsRequirementsKHR")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetD3D12GraphicsRequirementsKHR);
		}
		else if (apiName == "xrGetVisibilityMaskKHR")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetVisibilityMaskKHR);
		}
		else if (apiName == "xrConvertWin32PerformanceCounterToTimeKHR")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrConvertWin32PerformanceCounterToTimeKHR);
		}
		else if (apiName == "xrConvertTimeToWin32PerformanceCounterKHR")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrConvertTimeToWin32PerformanceCounterKHR);
		}
		else if (apiName == "xrCreateVulkanInstanceKHR")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrCreateVulkanInstanceKHR);
		}
		else if (apiName == "xrCreateVulkanDeviceKHR")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrCreateVulkanDeviceKHR);
		}
		else if (apiName == "xrGetVulkanGraphicsDevice2KHR")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetVulkanGraphicsDevice2KHR);
		}
		else if (apiName == "xrGetVulkanGraphicsRequirements2KHR")
		{
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetVulkanGraphicsRequirements2KHR);
		}
		else
		{
			return XR_ERROR_FUNCTION_UNSUPPORTED;
		}

		return XR_SUCCESS;
	}

} // namespace RUNTIME_NAMESPACE

