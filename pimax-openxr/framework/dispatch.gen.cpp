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

	XrResult xrEnumerateInstanceExtensionProperties(const char* layerName, uint32_t propertyCapacityInput, uint32_t* propertyCountOutput, XrExtensionProperties* properties)
	{
		DebugLog("--> xrEnumerateInstanceExtensionProperties\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrEnumerateInstanceExtensionProperties(layerName, propertyCapacityInput, propertyCountOutput, properties);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrEnumerateInstanceExtensionProperties_Error", TLArg(exc.what(), "Error"));
			Log("xrEnumerateInstanceExtensionProperties: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrEnumerateInstanceExtensionProperties_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrEnumerateInstanceExtensionProperties %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrCreateInstance(const XrInstanceCreateInfo* createInfo, XrInstance* instance)
	{
		DebugLog("--> xrCreateInstance\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrCreateInstance(createInfo, instance);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrCreateInstance_Error", TLArg(exc.what(), "Error"));
			Log("xrCreateInstance: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrCreateInstance_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrCreateInstance %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrGetInstanceProperties(XrInstance instance, XrInstanceProperties* instanceProperties)
	{
		DebugLog("--> xrGetInstanceProperties\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetInstanceProperties(instance, instanceProperties);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrGetInstanceProperties_Error", TLArg(exc.what(), "Error"));
			Log("xrGetInstanceProperties: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrGetInstanceProperties_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrGetInstanceProperties %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrPollEvent(XrInstance instance, XrEventDataBuffer* eventData)
	{
		DebugLog("--> xrPollEvent\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrPollEvent(instance, eventData);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrPollEvent_Error", TLArg(exc.what(), "Error"));
			Log("xrPollEvent: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrPollEvent_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrPollEvent %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrGetSystem(XrInstance instance, const XrSystemGetInfo* getInfo, XrSystemId* systemId)
	{
		DebugLog("--> xrGetSystem\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetSystem(instance, getInfo, systemId);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrGetSystem_Error", TLArg(exc.what(), "Error"));
			Log("xrGetSystem: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrGetSystem_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrGetSystem %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrGetSystemProperties(XrInstance instance, XrSystemId systemId, XrSystemProperties* properties)
	{
		DebugLog("--> xrGetSystemProperties\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetSystemProperties(instance, systemId, properties);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrGetSystemProperties_Error", TLArg(exc.what(), "Error"));
			Log("xrGetSystemProperties: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrGetSystemProperties_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrGetSystemProperties %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrEnumerateEnvironmentBlendModes(XrInstance instance, XrSystemId systemId, XrViewConfigurationType viewConfigurationType, uint32_t environmentBlendModeCapacityInput, uint32_t* environmentBlendModeCountOutput, XrEnvironmentBlendMode* environmentBlendModes)
	{
		DebugLog("--> xrEnumerateEnvironmentBlendModes\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrEnumerateEnvironmentBlendModes(instance, systemId, viewConfigurationType, environmentBlendModeCapacityInput, environmentBlendModeCountOutput, environmentBlendModes);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrEnumerateEnvironmentBlendModes_Error", TLArg(exc.what(), "Error"));
			Log("xrEnumerateEnvironmentBlendModes: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrEnumerateEnvironmentBlendModes_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrEnumerateEnvironmentBlendModes %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrCreateSession(XrInstance instance, const XrSessionCreateInfo* createInfo, XrSession* session)
	{
		DebugLog("--> xrCreateSession\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrCreateSession(instance, createInfo, session);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrCreateSession_Error", TLArg(exc.what(), "Error"));
			Log("xrCreateSession: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrCreateSession_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrCreateSession %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrDestroySession(XrSession session)
	{
		DebugLog("--> xrDestroySession\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrDestroySession(session);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrDestroySession_Error", TLArg(exc.what(), "Error"));
			Log("xrDestroySession: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrDestroySession_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrDestroySession %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrEnumerateReferenceSpaces(XrSession session, uint32_t spaceCapacityInput, uint32_t* spaceCountOutput, XrReferenceSpaceType* spaces)
	{
		DebugLog("--> xrEnumerateReferenceSpaces\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrEnumerateReferenceSpaces(session, spaceCapacityInput, spaceCountOutput, spaces);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrEnumerateReferenceSpaces_Error", TLArg(exc.what(), "Error"));
			Log("xrEnumerateReferenceSpaces: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrEnumerateReferenceSpaces_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrEnumerateReferenceSpaces %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrCreateReferenceSpace(XrSession session, const XrReferenceSpaceCreateInfo* createInfo, XrSpace* space)
	{
		DebugLog("--> xrCreateReferenceSpace\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrCreateReferenceSpace(session, createInfo, space);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrCreateReferenceSpace_Error", TLArg(exc.what(), "Error"));
			Log("xrCreateReferenceSpace: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrCreateReferenceSpace_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrCreateReferenceSpace %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrGetReferenceSpaceBoundsRect(XrSession session, XrReferenceSpaceType referenceSpaceType, XrExtent2Df* bounds)
	{
		DebugLog("--> xrGetReferenceSpaceBoundsRect\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetReferenceSpaceBoundsRect(session, referenceSpaceType, bounds);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrGetReferenceSpaceBoundsRect_Error", TLArg(exc.what(), "Error"));
			Log("xrGetReferenceSpaceBoundsRect: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrGetReferenceSpaceBoundsRect_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrGetReferenceSpaceBoundsRect %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrCreateActionSpace(XrSession session, const XrActionSpaceCreateInfo* createInfo, XrSpace* space)
	{
		DebugLog("--> xrCreateActionSpace\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrCreateActionSpace(session, createInfo, space);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrCreateActionSpace_Error", TLArg(exc.what(), "Error"));
			Log("xrCreateActionSpace: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrCreateActionSpace_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrCreateActionSpace %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrLocateSpace(XrSpace space, XrSpace baseSpace, XrTime time, XrSpaceLocation* location)
	{
		DebugLog("--> xrLocateSpace\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrLocateSpace(space, baseSpace, time, location);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrLocateSpace_Error", TLArg(exc.what(), "Error"));
			Log("xrLocateSpace: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrLocateSpace_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrLocateSpace %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrDestroySpace(XrSpace space)
	{
		DebugLog("--> xrDestroySpace\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrDestroySpace(space);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrDestroySpace_Error", TLArg(exc.what(), "Error"));
			Log("xrDestroySpace: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrDestroySpace_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrDestroySpace %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrEnumerateViewConfigurations(XrInstance instance, XrSystemId systemId, uint32_t viewConfigurationTypeCapacityInput, uint32_t* viewConfigurationTypeCountOutput, XrViewConfigurationType* viewConfigurationTypes)
	{
		DebugLog("--> xrEnumerateViewConfigurations\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrEnumerateViewConfigurations(instance, systemId, viewConfigurationTypeCapacityInput, viewConfigurationTypeCountOutput, viewConfigurationTypes);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrEnumerateViewConfigurations_Error", TLArg(exc.what(), "Error"));
			Log("xrEnumerateViewConfigurations: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrEnumerateViewConfigurations_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrEnumerateViewConfigurations %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrGetViewConfigurationProperties(XrInstance instance, XrSystemId systemId, XrViewConfigurationType viewConfigurationType, XrViewConfigurationProperties* configurationProperties)
	{
		DebugLog("--> xrGetViewConfigurationProperties\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetViewConfigurationProperties(instance, systemId, viewConfigurationType, configurationProperties);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrGetViewConfigurationProperties_Error", TLArg(exc.what(), "Error"));
			Log("xrGetViewConfigurationProperties: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrGetViewConfigurationProperties_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrGetViewConfigurationProperties %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrEnumerateViewConfigurationViews(XrInstance instance, XrSystemId systemId, XrViewConfigurationType viewConfigurationType, uint32_t viewCapacityInput, uint32_t* viewCountOutput, XrViewConfigurationView* views)
	{
		DebugLog("--> xrEnumerateViewConfigurationViews\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrEnumerateViewConfigurationViews(instance, systemId, viewConfigurationType, viewCapacityInput, viewCountOutput, views);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrEnumerateViewConfigurationViews_Error", TLArg(exc.what(), "Error"));
			Log("xrEnumerateViewConfigurationViews: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrEnumerateViewConfigurationViews_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrEnumerateViewConfigurationViews %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrEnumerateSwapchainFormats(XrSession session, uint32_t formatCapacityInput, uint32_t* formatCountOutput, int64_t* formats)
	{
		DebugLog("--> xrEnumerateSwapchainFormats\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrEnumerateSwapchainFormats(session, formatCapacityInput, formatCountOutput, formats);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrEnumerateSwapchainFormats_Error", TLArg(exc.what(), "Error"));
			Log("xrEnumerateSwapchainFormats: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrEnumerateSwapchainFormats_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrEnumerateSwapchainFormats %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrCreateSwapchain(XrSession session, const XrSwapchainCreateInfo* createInfo, XrSwapchain* swapchain)
	{
		DebugLog("--> xrCreateSwapchain\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrCreateSwapchain(session, createInfo, swapchain);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrCreateSwapchain_Error", TLArg(exc.what(), "Error"));
			Log("xrCreateSwapchain: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrCreateSwapchain_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrCreateSwapchain %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrDestroySwapchain(XrSwapchain swapchain)
	{
		DebugLog("--> xrDestroySwapchain\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrDestroySwapchain(swapchain);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrDestroySwapchain_Error", TLArg(exc.what(), "Error"));
			Log("xrDestroySwapchain: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrDestroySwapchain_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrDestroySwapchain %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrEnumerateSwapchainImages(XrSwapchain swapchain, uint32_t imageCapacityInput, uint32_t* imageCountOutput, XrSwapchainImageBaseHeader* images)
	{
		DebugLog("--> xrEnumerateSwapchainImages\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrEnumerateSwapchainImages(swapchain, imageCapacityInput, imageCountOutput, images);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrEnumerateSwapchainImages_Error", TLArg(exc.what(), "Error"));
			Log("xrEnumerateSwapchainImages: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrEnumerateSwapchainImages_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrEnumerateSwapchainImages %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrAcquireSwapchainImage(XrSwapchain swapchain, const XrSwapchainImageAcquireInfo* acquireInfo, uint32_t* index)
	{
		DebugLog("--> xrAcquireSwapchainImage\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrAcquireSwapchainImage(swapchain, acquireInfo, index);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrAcquireSwapchainImage_Error", TLArg(exc.what(), "Error"));
			Log("xrAcquireSwapchainImage: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrAcquireSwapchainImage_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrAcquireSwapchainImage %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrWaitSwapchainImage(XrSwapchain swapchain, const XrSwapchainImageWaitInfo* waitInfo)
	{
		DebugLog("--> xrWaitSwapchainImage\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrWaitSwapchainImage(swapchain, waitInfo);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrWaitSwapchainImage_Error", TLArg(exc.what(), "Error"));
			Log("xrWaitSwapchainImage: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrWaitSwapchainImage_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrWaitSwapchainImage %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrReleaseSwapchainImage(XrSwapchain swapchain, const XrSwapchainImageReleaseInfo* releaseInfo)
	{
		DebugLog("--> xrReleaseSwapchainImage\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrReleaseSwapchainImage(swapchain, releaseInfo);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrReleaseSwapchainImage_Error", TLArg(exc.what(), "Error"));
			Log("xrReleaseSwapchainImage: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrReleaseSwapchainImage_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrReleaseSwapchainImage %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrBeginSession(XrSession session, const XrSessionBeginInfo* beginInfo)
	{
		DebugLog("--> xrBeginSession\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrBeginSession(session, beginInfo);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrBeginSession_Error", TLArg(exc.what(), "Error"));
			Log("xrBeginSession: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrBeginSession_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrBeginSession %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrEndSession(XrSession session)
	{
		DebugLog("--> xrEndSession\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrEndSession(session);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrEndSession_Error", TLArg(exc.what(), "Error"));
			Log("xrEndSession: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrEndSession_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrEndSession %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrRequestExitSession(XrSession session)
	{
		DebugLog("--> xrRequestExitSession\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrRequestExitSession(session);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrRequestExitSession_Error", TLArg(exc.what(), "Error"));
			Log("xrRequestExitSession: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrRequestExitSession_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrRequestExitSession %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrWaitFrame(XrSession session, const XrFrameWaitInfo* frameWaitInfo, XrFrameState* frameState)
	{
		DebugLog("--> xrWaitFrame\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrWaitFrame(session, frameWaitInfo, frameState);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrWaitFrame_Error", TLArg(exc.what(), "Error"));
			Log("xrWaitFrame: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrWaitFrame_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrWaitFrame %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrBeginFrame(XrSession session, const XrFrameBeginInfo* frameBeginInfo)
	{
		DebugLog("--> xrBeginFrame\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrBeginFrame(session, frameBeginInfo);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrBeginFrame_Error", TLArg(exc.what(), "Error"));
			Log("xrBeginFrame: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrBeginFrame_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrBeginFrame %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrEndFrame(XrSession session, const XrFrameEndInfo* frameEndInfo)
	{
		DebugLog("--> xrEndFrame\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrEndFrame(session, frameEndInfo);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrEndFrame_Error", TLArg(exc.what(), "Error"));
			Log("xrEndFrame: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrEndFrame_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrEndFrame %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrLocateViews(XrSession session, const XrViewLocateInfo* viewLocateInfo, XrViewState* viewState, uint32_t viewCapacityInput, uint32_t* viewCountOutput, XrView* views)
	{
		DebugLog("--> xrLocateViews\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrLocateViews(session, viewLocateInfo, viewState, viewCapacityInput, viewCountOutput, views);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrLocateViews_Error", TLArg(exc.what(), "Error"));
			Log("xrLocateViews: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrLocateViews_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrLocateViews %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrStringToPath(XrInstance instance, const char* pathString, XrPath* path)
	{
		DebugLog("--> xrStringToPath\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrStringToPath(instance, pathString, path);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrStringToPath_Error", TLArg(exc.what(), "Error"));
			Log("xrStringToPath: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrStringToPath_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrStringToPath %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrPathToString(XrInstance instance, XrPath path, uint32_t bufferCapacityInput, uint32_t* bufferCountOutput, char* buffer)
	{
		DebugLog("--> xrPathToString\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrPathToString(instance, path, bufferCapacityInput, bufferCountOutput, buffer);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrPathToString_Error", TLArg(exc.what(), "Error"));
			Log("xrPathToString: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrPathToString_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrPathToString %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrCreateActionSet(XrInstance instance, const XrActionSetCreateInfo* createInfo, XrActionSet* actionSet)
	{
		DebugLog("--> xrCreateActionSet\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrCreateActionSet(instance, createInfo, actionSet);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrCreateActionSet_Error", TLArg(exc.what(), "Error"));
			Log("xrCreateActionSet: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrCreateActionSet_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrCreateActionSet %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrDestroyActionSet(XrActionSet actionSet)
	{
		DebugLog("--> xrDestroyActionSet\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrDestroyActionSet(actionSet);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrDestroyActionSet_Error", TLArg(exc.what(), "Error"));
			Log("xrDestroyActionSet: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrDestroyActionSet_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrDestroyActionSet %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrCreateAction(XrActionSet actionSet, const XrActionCreateInfo* createInfo, XrAction* action)
	{
		DebugLog("--> xrCreateAction\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrCreateAction(actionSet, createInfo, action);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrCreateAction_Error", TLArg(exc.what(), "Error"));
			Log("xrCreateAction: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrCreateAction_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrCreateAction %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrDestroyAction(XrAction action)
	{
		DebugLog("--> xrDestroyAction\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrDestroyAction(action);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrDestroyAction_Error", TLArg(exc.what(), "Error"));
			Log("xrDestroyAction: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrDestroyAction_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrDestroyAction %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrSuggestInteractionProfileBindings(XrInstance instance, const XrInteractionProfileSuggestedBinding* suggestedBindings)
	{
		DebugLog("--> xrSuggestInteractionProfileBindings\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrSuggestInteractionProfileBindings(instance, suggestedBindings);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrSuggestInteractionProfileBindings_Error", TLArg(exc.what(), "Error"));
			Log("xrSuggestInteractionProfileBindings: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrSuggestInteractionProfileBindings_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrSuggestInteractionProfileBindings %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrAttachSessionActionSets(XrSession session, const XrSessionActionSetsAttachInfo* attachInfo)
	{
		DebugLog("--> xrAttachSessionActionSets\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrAttachSessionActionSets(session, attachInfo);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrAttachSessionActionSets_Error", TLArg(exc.what(), "Error"));
			Log("xrAttachSessionActionSets: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrAttachSessionActionSets_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrAttachSessionActionSets %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrGetCurrentInteractionProfile(XrSession session, XrPath topLevelUserPath, XrInteractionProfileState* interactionProfile)
	{
		DebugLog("--> xrGetCurrentInteractionProfile\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetCurrentInteractionProfile(session, topLevelUserPath, interactionProfile);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrGetCurrentInteractionProfile_Error", TLArg(exc.what(), "Error"));
			Log("xrGetCurrentInteractionProfile: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrGetCurrentInteractionProfile_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrGetCurrentInteractionProfile %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrGetActionStateBoolean(XrSession session, const XrActionStateGetInfo* getInfo, XrActionStateBoolean* state)
	{
		DebugLog("--> xrGetActionStateBoolean\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetActionStateBoolean(session, getInfo, state);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrGetActionStateBoolean_Error", TLArg(exc.what(), "Error"));
			Log("xrGetActionStateBoolean: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrGetActionStateBoolean_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrGetActionStateBoolean %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrGetActionStateFloat(XrSession session, const XrActionStateGetInfo* getInfo, XrActionStateFloat* state)
	{
		DebugLog("--> xrGetActionStateFloat\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetActionStateFloat(session, getInfo, state);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrGetActionStateFloat_Error", TLArg(exc.what(), "Error"));
			Log("xrGetActionStateFloat: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrGetActionStateFloat_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrGetActionStateFloat %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrGetActionStateVector2f(XrSession session, const XrActionStateGetInfo* getInfo, XrActionStateVector2f* state)
	{
		DebugLog("--> xrGetActionStateVector2f\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetActionStateVector2f(session, getInfo, state);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrGetActionStateVector2f_Error", TLArg(exc.what(), "Error"));
			Log("xrGetActionStateVector2f: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrGetActionStateVector2f_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrGetActionStateVector2f %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrGetActionStatePose(XrSession session, const XrActionStateGetInfo* getInfo, XrActionStatePose* state)
	{
		DebugLog("--> xrGetActionStatePose\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetActionStatePose(session, getInfo, state);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrGetActionStatePose_Error", TLArg(exc.what(), "Error"));
			Log("xrGetActionStatePose: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrGetActionStatePose_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrGetActionStatePose %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrSyncActions(XrSession session, const XrActionsSyncInfo* syncInfo)
	{
		DebugLog("--> xrSyncActions\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrSyncActions(session, syncInfo);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrSyncActions_Error", TLArg(exc.what(), "Error"));
			Log("xrSyncActions: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrSyncActions_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrSyncActions %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrEnumerateBoundSourcesForAction(XrSession session, const XrBoundSourcesForActionEnumerateInfo* enumerateInfo, uint32_t sourceCapacityInput, uint32_t* sourceCountOutput, XrPath* sources)
	{
		DebugLog("--> xrEnumerateBoundSourcesForAction\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrEnumerateBoundSourcesForAction(session, enumerateInfo, sourceCapacityInput, sourceCountOutput, sources);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrEnumerateBoundSourcesForAction_Error", TLArg(exc.what(), "Error"));
			Log("xrEnumerateBoundSourcesForAction: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrEnumerateBoundSourcesForAction_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrEnumerateBoundSourcesForAction %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrGetInputSourceLocalizedName(XrSession session, const XrInputSourceLocalizedNameGetInfo* getInfo, uint32_t bufferCapacityInput, uint32_t* bufferCountOutput, char* buffer)
	{
		DebugLog("--> xrGetInputSourceLocalizedName\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetInputSourceLocalizedName(session, getInfo, bufferCapacityInput, bufferCountOutput, buffer);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrGetInputSourceLocalizedName_Error", TLArg(exc.what(), "Error"));
			Log("xrGetInputSourceLocalizedName: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrGetInputSourceLocalizedName_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrGetInputSourceLocalizedName %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrApplyHapticFeedback(XrSession session, const XrHapticActionInfo* hapticActionInfo, const XrHapticBaseHeader* hapticFeedback)
	{
		DebugLog("--> xrApplyHapticFeedback\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrApplyHapticFeedback(session, hapticActionInfo, hapticFeedback);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrApplyHapticFeedback_Error", TLArg(exc.what(), "Error"));
			Log("xrApplyHapticFeedback: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrApplyHapticFeedback_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrApplyHapticFeedback %s\n", xr::ToCString(result));

		return result;
	}

	XrResult xrStopHapticFeedback(XrSession session, const XrHapticActionInfo* hapticActionInfo)
	{
		DebugLog("--> xrStopHapticFeedback\n");

		XrResult result;
		try
		{
			result = RUNTIME_NAMESPACE::GetInstance()->xrStopHapticFeedback(session, hapticActionInfo);
		}
		catch (std::exception exc)
		{
			TraceLoggingWrite(g_traceProvider, "xrStopHapticFeedback_Error", TLArg(exc.what(), "Error"));
			Log("xrStopHapticFeedback: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWrite(g_traceProvider, "xrStopHapticFeedback_Result", TLArg(xr::ToCString(result), "Result"));
		DebugLog("<-- xrStopHapticFeedback %s\n", xr::ToCString(result));

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
		else 
		{
			return XR_ERROR_FUNCTION_UNSUPPORTED;
		}

		return XR_SUCCESS;
	}

} // namespace RUNTIME_NAMESPACE

