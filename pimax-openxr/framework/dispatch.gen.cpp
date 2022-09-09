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

namespace RUNTIME_NAMESPACE {
    using namespace RUNTIME_NAMESPACE::log;


	// Auto-generated wrappers for the APIs.

	XrResult XRAPI_CALL xrEnumerateInstanceExtensionProperties(const char* layerName, uint32_t propertyCapacityInput, uint32_t* propertyCountOutput, XrExtensionProperties* properties) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrEnumerateInstanceExtensionProperties");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrEnumerateInstanceExtensionProperties(layerName, propertyCapacityInput, propertyCountOutput, properties);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrEnumerateInstanceExtensionProperties_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrEnumerateInstanceExtensionProperties: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrEnumerateInstanceExtensionProperties", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrEnumerateInstanceExtensionProperties failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrCreateInstance(const XrInstanceCreateInfo* createInfo, XrInstance* instance) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrCreateInstance");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrCreateInstance(createInfo, instance);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrCreateInstance_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrCreateInstance: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrCreateInstance", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrCreateInstance failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrGetInstanceProperties(XrInstance instance, XrInstanceProperties* instanceProperties) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrGetInstanceProperties");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetInstanceProperties(instance, instanceProperties);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrGetInstanceProperties_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrGetInstanceProperties: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrGetInstanceProperties", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrGetInstanceProperties failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrPollEvent(XrInstance instance, XrEventDataBuffer* eventData) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrPollEvent");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrPollEvent(instance, eventData);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrPollEvent_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrPollEvent: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrPollEvent", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrPollEvent failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrResultToString(XrInstance instance, XrResult value, char buffer[XR_MAX_RESULT_STRING_SIZE]) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrResultToString");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrResultToString(instance, value, buffer);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrResultToString_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrResultToString: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrResultToString", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrResultToString failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrStructureTypeToString(XrInstance instance, XrStructureType value, char buffer[XR_MAX_STRUCTURE_NAME_SIZE]) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrStructureTypeToString");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrStructureTypeToString(instance, value, buffer);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrStructureTypeToString_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrStructureTypeToString: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrStructureTypeToString", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrStructureTypeToString failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrGetSystem(XrInstance instance, const XrSystemGetInfo* getInfo, XrSystemId* systemId) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrGetSystem");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetSystem(instance, getInfo, systemId);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrGetSystem_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrGetSystem: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrGetSystem", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrGetSystem failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrGetSystemProperties(XrInstance instance, XrSystemId systemId, XrSystemProperties* properties) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrGetSystemProperties");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetSystemProperties(instance, systemId, properties);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrGetSystemProperties_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrGetSystemProperties: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrGetSystemProperties", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrGetSystemProperties failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrEnumerateEnvironmentBlendModes(XrInstance instance, XrSystemId systemId, XrViewConfigurationType viewConfigurationType, uint32_t environmentBlendModeCapacityInput, uint32_t* environmentBlendModeCountOutput, XrEnvironmentBlendMode* environmentBlendModes) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrEnumerateEnvironmentBlendModes");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrEnumerateEnvironmentBlendModes(instance, systemId, viewConfigurationType, environmentBlendModeCapacityInput, environmentBlendModeCountOutput, environmentBlendModes);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrEnumerateEnvironmentBlendModes_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrEnumerateEnvironmentBlendModes: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrEnumerateEnvironmentBlendModes", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrEnumerateEnvironmentBlendModes failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrCreateSession(XrInstance instance, const XrSessionCreateInfo* createInfo, XrSession* session) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrCreateSession");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrCreateSession(instance, createInfo, session);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrCreateSession_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrCreateSession: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrCreateSession", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrCreateSession failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrDestroySession(XrSession session) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrDestroySession");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrDestroySession(session);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrDestroySession_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrDestroySession: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrDestroySession", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrDestroySession failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrEnumerateReferenceSpaces(XrSession session, uint32_t spaceCapacityInput, uint32_t* spaceCountOutput, XrReferenceSpaceType* spaces) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrEnumerateReferenceSpaces");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrEnumerateReferenceSpaces(session, spaceCapacityInput, spaceCountOutput, spaces);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrEnumerateReferenceSpaces_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrEnumerateReferenceSpaces: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrEnumerateReferenceSpaces", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrEnumerateReferenceSpaces failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrCreateReferenceSpace(XrSession session, const XrReferenceSpaceCreateInfo* createInfo, XrSpace* space) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrCreateReferenceSpace");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrCreateReferenceSpace(session, createInfo, space);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrCreateReferenceSpace_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrCreateReferenceSpace: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrCreateReferenceSpace", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrCreateReferenceSpace failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrGetReferenceSpaceBoundsRect(XrSession session, XrReferenceSpaceType referenceSpaceType, XrExtent2Df* bounds) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrGetReferenceSpaceBoundsRect");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetReferenceSpaceBoundsRect(session, referenceSpaceType, bounds);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrGetReferenceSpaceBoundsRect_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrGetReferenceSpaceBoundsRect: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrGetReferenceSpaceBoundsRect", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrGetReferenceSpaceBoundsRect failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrCreateActionSpace(XrSession session, const XrActionSpaceCreateInfo* createInfo, XrSpace* space) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrCreateActionSpace");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrCreateActionSpace(session, createInfo, space);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrCreateActionSpace_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrCreateActionSpace: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrCreateActionSpace", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrCreateActionSpace failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrLocateSpace(XrSpace space, XrSpace baseSpace, XrTime time, XrSpaceLocation* location) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrLocateSpace");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrLocateSpace(space, baseSpace, time, location);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrLocateSpace_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrLocateSpace: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrLocateSpace", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrLocateSpace failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrDestroySpace(XrSpace space) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrDestroySpace");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrDestroySpace(space);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrDestroySpace_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrDestroySpace: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrDestroySpace", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrDestroySpace failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrEnumerateViewConfigurations(XrInstance instance, XrSystemId systemId, uint32_t viewConfigurationTypeCapacityInput, uint32_t* viewConfigurationTypeCountOutput, XrViewConfigurationType* viewConfigurationTypes) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrEnumerateViewConfigurations");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrEnumerateViewConfigurations(instance, systemId, viewConfigurationTypeCapacityInput, viewConfigurationTypeCountOutput, viewConfigurationTypes);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrEnumerateViewConfigurations_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrEnumerateViewConfigurations: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrEnumerateViewConfigurations", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrEnumerateViewConfigurations failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrGetViewConfigurationProperties(XrInstance instance, XrSystemId systemId, XrViewConfigurationType viewConfigurationType, XrViewConfigurationProperties* configurationProperties) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrGetViewConfigurationProperties");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetViewConfigurationProperties(instance, systemId, viewConfigurationType, configurationProperties);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrGetViewConfigurationProperties_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrGetViewConfigurationProperties: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrGetViewConfigurationProperties", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrGetViewConfigurationProperties failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrEnumerateViewConfigurationViews(XrInstance instance, XrSystemId systemId, XrViewConfigurationType viewConfigurationType, uint32_t viewCapacityInput, uint32_t* viewCountOutput, XrViewConfigurationView* views) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrEnumerateViewConfigurationViews");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrEnumerateViewConfigurationViews(instance, systemId, viewConfigurationType, viewCapacityInput, viewCountOutput, views);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrEnumerateViewConfigurationViews_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrEnumerateViewConfigurationViews: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrEnumerateViewConfigurationViews", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrEnumerateViewConfigurationViews failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrEnumerateSwapchainFormats(XrSession session, uint32_t formatCapacityInput, uint32_t* formatCountOutput, int64_t* formats) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrEnumerateSwapchainFormats");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrEnumerateSwapchainFormats(session, formatCapacityInput, formatCountOutput, formats);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrEnumerateSwapchainFormats_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrEnumerateSwapchainFormats: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrEnumerateSwapchainFormats", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrEnumerateSwapchainFormats failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrCreateSwapchain(XrSession session, const XrSwapchainCreateInfo* createInfo, XrSwapchain* swapchain) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrCreateSwapchain");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrCreateSwapchain(session, createInfo, swapchain);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrCreateSwapchain_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrCreateSwapchain: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrCreateSwapchain", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrCreateSwapchain failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrDestroySwapchain(XrSwapchain swapchain) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrDestroySwapchain");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrDestroySwapchain(swapchain);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrDestroySwapchain_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrDestroySwapchain: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrDestroySwapchain", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrDestroySwapchain failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrEnumerateSwapchainImages(XrSwapchain swapchain, uint32_t imageCapacityInput, uint32_t* imageCountOutput, XrSwapchainImageBaseHeader* images) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrEnumerateSwapchainImages");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrEnumerateSwapchainImages(swapchain, imageCapacityInput, imageCountOutput, images);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrEnumerateSwapchainImages_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrEnumerateSwapchainImages: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrEnumerateSwapchainImages", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrEnumerateSwapchainImages failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrAcquireSwapchainImage(XrSwapchain swapchain, const XrSwapchainImageAcquireInfo* acquireInfo, uint32_t* index) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrAcquireSwapchainImage");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrAcquireSwapchainImage(swapchain, acquireInfo, index);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrAcquireSwapchainImage_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrAcquireSwapchainImage: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrAcquireSwapchainImage", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrAcquireSwapchainImage failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrWaitSwapchainImage(XrSwapchain swapchain, const XrSwapchainImageWaitInfo* waitInfo) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrWaitSwapchainImage");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrWaitSwapchainImage(swapchain, waitInfo);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrWaitSwapchainImage_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrWaitSwapchainImage: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrWaitSwapchainImage", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrWaitSwapchainImage failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrReleaseSwapchainImage(XrSwapchain swapchain, const XrSwapchainImageReleaseInfo* releaseInfo) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrReleaseSwapchainImage");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrReleaseSwapchainImage(swapchain, releaseInfo);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrReleaseSwapchainImage_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrReleaseSwapchainImage: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrReleaseSwapchainImage", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrReleaseSwapchainImage failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrBeginSession(XrSession session, const XrSessionBeginInfo* beginInfo) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrBeginSession");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrBeginSession(session, beginInfo);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrBeginSession_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrBeginSession: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrBeginSession", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrBeginSession failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrEndSession(XrSession session) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrEndSession");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrEndSession(session);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrEndSession_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrEndSession: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrEndSession", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrEndSession failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrRequestExitSession(XrSession session) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrRequestExitSession");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrRequestExitSession(session);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrRequestExitSession_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrRequestExitSession: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrRequestExitSession", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrRequestExitSession failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrWaitFrame(XrSession session, const XrFrameWaitInfo* frameWaitInfo, XrFrameState* frameState) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrWaitFrame");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrWaitFrame(session, frameWaitInfo, frameState);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrWaitFrame_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrWaitFrame: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrWaitFrame", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrWaitFrame failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrBeginFrame(XrSession session, const XrFrameBeginInfo* frameBeginInfo) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrBeginFrame");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrBeginFrame(session, frameBeginInfo);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrBeginFrame_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrBeginFrame: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrBeginFrame", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrBeginFrame failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrEndFrame(XrSession session, const XrFrameEndInfo* frameEndInfo) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrEndFrame");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrEndFrame(session, frameEndInfo);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrEndFrame_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrEndFrame: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrEndFrame", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrEndFrame failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrLocateViews(XrSession session, const XrViewLocateInfo* viewLocateInfo, XrViewState* viewState, uint32_t viewCapacityInput, uint32_t* viewCountOutput, XrView* views) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrLocateViews");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrLocateViews(session, viewLocateInfo, viewState, viewCapacityInput, viewCountOutput, views);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrLocateViews_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrLocateViews: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrLocateViews", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrLocateViews failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrStringToPath(XrInstance instance, const char* pathString, XrPath* path) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrStringToPath");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrStringToPath(instance, pathString, path);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrStringToPath_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrStringToPath: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrStringToPath", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrStringToPath failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrPathToString(XrInstance instance, XrPath path, uint32_t bufferCapacityInput, uint32_t* bufferCountOutput, char* buffer) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrPathToString");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrPathToString(instance, path, bufferCapacityInput, bufferCountOutput, buffer);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrPathToString_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrPathToString: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrPathToString", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrPathToString failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrCreateActionSet(XrInstance instance, const XrActionSetCreateInfo* createInfo, XrActionSet* actionSet) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrCreateActionSet");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrCreateActionSet(instance, createInfo, actionSet);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrCreateActionSet_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrCreateActionSet: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrCreateActionSet", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrCreateActionSet failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrDestroyActionSet(XrActionSet actionSet) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrDestroyActionSet");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrDestroyActionSet(actionSet);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrDestroyActionSet_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrDestroyActionSet: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrDestroyActionSet", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrDestroyActionSet failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrCreateAction(XrActionSet actionSet, const XrActionCreateInfo* createInfo, XrAction* action) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrCreateAction");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrCreateAction(actionSet, createInfo, action);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrCreateAction_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrCreateAction: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrCreateAction", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrCreateAction failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrDestroyAction(XrAction action) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrDestroyAction");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrDestroyAction(action);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrDestroyAction_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrDestroyAction: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrDestroyAction", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrDestroyAction failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrSuggestInteractionProfileBindings(XrInstance instance, const XrInteractionProfileSuggestedBinding* suggestedBindings) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrSuggestInteractionProfileBindings");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrSuggestInteractionProfileBindings(instance, suggestedBindings);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrSuggestInteractionProfileBindings_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrSuggestInteractionProfileBindings: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrSuggestInteractionProfileBindings", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrSuggestInteractionProfileBindings failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrAttachSessionActionSets(XrSession session, const XrSessionActionSetsAttachInfo* attachInfo) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrAttachSessionActionSets");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrAttachSessionActionSets(session, attachInfo);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrAttachSessionActionSets_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrAttachSessionActionSets: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrAttachSessionActionSets", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrAttachSessionActionSets failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrGetCurrentInteractionProfile(XrSession session, XrPath topLevelUserPath, XrInteractionProfileState* interactionProfile) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrGetCurrentInteractionProfile");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetCurrentInteractionProfile(session, topLevelUserPath, interactionProfile);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrGetCurrentInteractionProfile_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrGetCurrentInteractionProfile: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrGetCurrentInteractionProfile", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrGetCurrentInteractionProfile failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrGetActionStateBoolean(XrSession session, const XrActionStateGetInfo* getInfo, XrActionStateBoolean* state) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrGetActionStateBoolean");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetActionStateBoolean(session, getInfo, state);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrGetActionStateBoolean_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrGetActionStateBoolean: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrGetActionStateBoolean", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrGetActionStateBoolean failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrGetActionStateFloat(XrSession session, const XrActionStateGetInfo* getInfo, XrActionStateFloat* state) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrGetActionStateFloat");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetActionStateFloat(session, getInfo, state);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrGetActionStateFloat_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrGetActionStateFloat: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrGetActionStateFloat", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrGetActionStateFloat failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrGetActionStateVector2f(XrSession session, const XrActionStateGetInfo* getInfo, XrActionStateVector2f* state) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrGetActionStateVector2f");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetActionStateVector2f(session, getInfo, state);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrGetActionStateVector2f_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrGetActionStateVector2f: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrGetActionStateVector2f", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrGetActionStateVector2f failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrGetActionStatePose(XrSession session, const XrActionStateGetInfo* getInfo, XrActionStatePose* state) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrGetActionStatePose");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetActionStatePose(session, getInfo, state);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrGetActionStatePose_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrGetActionStatePose: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrGetActionStatePose", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrGetActionStatePose failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrSyncActions(XrSession session, const XrActionsSyncInfo* syncInfo) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrSyncActions");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrSyncActions(session, syncInfo);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrSyncActions_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrSyncActions: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrSyncActions", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrSyncActions failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrEnumerateBoundSourcesForAction(XrSession session, const XrBoundSourcesForActionEnumerateInfo* enumerateInfo, uint32_t sourceCapacityInput, uint32_t* sourceCountOutput, XrPath* sources) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrEnumerateBoundSourcesForAction");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrEnumerateBoundSourcesForAction(session, enumerateInfo, sourceCapacityInput, sourceCountOutput, sources);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrEnumerateBoundSourcesForAction_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrEnumerateBoundSourcesForAction: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrEnumerateBoundSourcesForAction", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrEnumerateBoundSourcesForAction failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrGetInputSourceLocalizedName(XrSession session, const XrInputSourceLocalizedNameGetInfo* getInfo, uint32_t bufferCapacityInput, uint32_t* bufferCountOutput, char* buffer) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrGetInputSourceLocalizedName");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetInputSourceLocalizedName(session, getInfo, bufferCapacityInput, bufferCountOutput, buffer);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrGetInputSourceLocalizedName_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrGetInputSourceLocalizedName: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrGetInputSourceLocalizedName", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrGetInputSourceLocalizedName failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrApplyHapticFeedback(XrSession session, const XrHapticActionInfo* hapticActionInfo, const XrHapticBaseHeader* hapticFeedback) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrApplyHapticFeedback");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrApplyHapticFeedback(session, hapticActionInfo, hapticFeedback);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrApplyHapticFeedback_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrApplyHapticFeedback: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrApplyHapticFeedback", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrApplyHapticFeedback failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrStopHapticFeedback(XrSession session, const XrHapticActionInfo* hapticActionInfo) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrStopHapticFeedback");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrStopHapticFeedback(session, hapticActionInfo);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrStopHapticFeedback_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrStopHapticFeedback: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrStopHapticFeedback", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrStopHapticFeedback failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrGetOpenGLGraphicsRequirementsKHR(XrInstance instance, XrSystemId systemId, XrGraphicsRequirementsOpenGLKHR* graphicsRequirements) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrGetOpenGLGraphicsRequirementsKHR");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetOpenGLGraphicsRequirementsKHR(instance, systemId, graphicsRequirements);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrGetOpenGLGraphicsRequirementsKHR_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrGetOpenGLGraphicsRequirementsKHR: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrGetOpenGLGraphicsRequirementsKHR", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrGetOpenGLGraphicsRequirementsKHR failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrGetVulkanInstanceExtensionsKHR(XrInstance instance, XrSystemId systemId, uint32_t bufferCapacityInput, uint32_t* bufferCountOutput, char* buffer) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrGetVulkanInstanceExtensionsKHR");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetVulkanInstanceExtensionsKHR(instance, systemId, bufferCapacityInput, bufferCountOutput, buffer);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrGetVulkanInstanceExtensionsKHR_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrGetVulkanInstanceExtensionsKHR: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrGetVulkanInstanceExtensionsKHR", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrGetVulkanInstanceExtensionsKHR failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrGetVulkanDeviceExtensionsKHR(XrInstance instance, XrSystemId systemId, uint32_t bufferCapacityInput, uint32_t* bufferCountOutput, char* buffer) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrGetVulkanDeviceExtensionsKHR");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetVulkanDeviceExtensionsKHR(instance, systemId, bufferCapacityInput, bufferCountOutput, buffer);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrGetVulkanDeviceExtensionsKHR_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrGetVulkanDeviceExtensionsKHR: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrGetVulkanDeviceExtensionsKHR", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrGetVulkanDeviceExtensionsKHR failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrGetVulkanGraphicsDeviceKHR(XrInstance instance, XrSystemId systemId, VkInstance vkInstance, VkPhysicalDevice* vkPhysicalDevice) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrGetVulkanGraphicsDeviceKHR");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetVulkanGraphicsDeviceKHR(instance, systemId, vkInstance, vkPhysicalDevice);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrGetVulkanGraphicsDeviceKHR_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrGetVulkanGraphicsDeviceKHR: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrGetVulkanGraphicsDeviceKHR", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrGetVulkanGraphicsDeviceKHR failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrGetVulkanGraphicsRequirementsKHR(XrInstance instance, XrSystemId systemId, XrGraphicsRequirementsVulkanKHR* graphicsRequirements) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrGetVulkanGraphicsRequirementsKHR");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetVulkanGraphicsRequirementsKHR(instance, systemId, graphicsRequirements);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrGetVulkanGraphicsRequirementsKHR_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrGetVulkanGraphicsRequirementsKHR: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrGetVulkanGraphicsRequirementsKHR", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrGetVulkanGraphicsRequirementsKHR failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrGetD3D11GraphicsRequirementsKHR(XrInstance instance, XrSystemId systemId, XrGraphicsRequirementsD3D11KHR* graphicsRequirements) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrGetD3D11GraphicsRequirementsKHR");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetD3D11GraphicsRequirementsKHR(instance, systemId, graphicsRequirements);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrGetD3D11GraphicsRequirementsKHR_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrGetD3D11GraphicsRequirementsKHR: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrGetD3D11GraphicsRequirementsKHR", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrGetD3D11GraphicsRequirementsKHR failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrGetD3D12GraphicsRequirementsKHR(XrInstance instance, XrSystemId systemId, XrGraphicsRequirementsD3D12KHR* graphicsRequirements) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrGetD3D12GraphicsRequirementsKHR");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetD3D12GraphicsRequirementsKHR(instance, systemId, graphicsRequirements);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrGetD3D12GraphicsRequirementsKHR_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrGetD3D12GraphicsRequirementsKHR: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrGetD3D12GraphicsRequirementsKHR", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrGetD3D12GraphicsRequirementsKHR failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrGetVisibilityMaskKHR(XrSession session, XrViewConfigurationType viewConfigurationType, uint32_t viewIndex, XrVisibilityMaskTypeKHR visibilityMaskType, XrVisibilityMaskKHR* visibilityMask) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrGetVisibilityMaskKHR");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetVisibilityMaskKHR(session, viewConfigurationType, viewIndex, visibilityMaskType, visibilityMask);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrGetVisibilityMaskKHR_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrGetVisibilityMaskKHR: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrGetVisibilityMaskKHR", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrGetVisibilityMaskKHR failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrConvertWin32PerformanceCounterToTimeKHR(XrInstance instance, const LARGE_INTEGER* performanceCounter, XrTime* time) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrConvertWin32PerformanceCounterToTimeKHR");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrConvertWin32PerformanceCounterToTimeKHR(instance, performanceCounter, time);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrConvertWin32PerformanceCounterToTimeKHR_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrConvertWin32PerformanceCounterToTimeKHR: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrConvertWin32PerformanceCounterToTimeKHR", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrConvertWin32PerformanceCounterToTimeKHR failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrConvertTimeToWin32PerformanceCounterKHR(XrInstance instance, XrTime time, LARGE_INTEGER* performanceCounter) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrConvertTimeToWin32PerformanceCounterKHR");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrConvertTimeToWin32PerformanceCounterKHR(instance, time, performanceCounter);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrConvertTimeToWin32PerformanceCounterKHR_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrConvertTimeToWin32PerformanceCounterKHR: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrConvertTimeToWin32PerformanceCounterKHR", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrConvertTimeToWin32PerformanceCounterKHR failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrCreateVulkanInstanceKHR(XrInstance instance, const XrVulkanInstanceCreateInfoKHR* createInfo, VkInstance* vulkanInstance, VkResult* vulkanResult) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrCreateVulkanInstanceKHR");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrCreateVulkanInstanceKHR(instance, createInfo, vulkanInstance, vulkanResult);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrCreateVulkanInstanceKHR_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrCreateVulkanInstanceKHR: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrCreateVulkanInstanceKHR", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrCreateVulkanInstanceKHR failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrCreateVulkanDeviceKHR(XrInstance instance, const XrVulkanDeviceCreateInfoKHR* createInfo, VkDevice* vulkanDevice, VkResult* vulkanResult) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrCreateVulkanDeviceKHR");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrCreateVulkanDeviceKHR(instance, createInfo, vulkanDevice, vulkanResult);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrCreateVulkanDeviceKHR_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrCreateVulkanDeviceKHR: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrCreateVulkanDeviceKHR", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrCreateVulkanDeviceKHR failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrGetVulkanGraphicsDevice2KHR(XrInstance instance, const XrVulkanGraphicsDeviceGetInfoKHR* getInfo, VkPhysicalDevice* vulkanPhysicalDevice) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrGetVulkanGraphicsDevice2KHR");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetVulkanGraphicsDevice2KHR(instance, getInfo, vulkanPhysicalDevice);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrGetVulkanGraphicsDevice2KHR_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrGetVulkanGraphicsDevice2KHR: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrGetVulkanGraphicsDevice2KHR", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrGetVulkanGraphicsDevice2KHR failed with %s\n", xr::ToCString(result));
		}

		return result;
	}

	XrResult XRAPI_CALL xrGetVulkanGraphicsRequirements2KHR(XrInstance instance, XrSystemId systemId, XrGraphicsRequirementsVulkanKHR* graphicsRequirements) {
		TraceLocalActivity(local);
		TraceLoggingWriteStart(local, "xrGetVulkanGraphicsRequirements2KHR");

		XrResult result;
		try {
			result = RUNTIME_NAMESPACE::GetInstance()->xrGetVulkanGraphicsRequirements2KHR(instance, systemId, graphicsRequirements);
		} catch (std::exception& exc) {
			TraceLoggingWriteTagged(local, "xrGetVulkanGraphicsRequirements2KHR_Error", TLArg(exc.what(), "Error"));
			ErrorLog("xrGetVulkanGraphicsRequirements2KHR: %s\n", exc.what());
			result = XR_ERROR_RUNTIME_FAILURE;
		}

		TraceLoggingWriteStop(local, "xrGetVulkanGraphicsRequirements2KHR", TLArg(xr::ToCString(result), "Result"));
		if (XR_FAILED(result)) {
			ErrorLog("xrGetVulkanGraphicsRequirements2KHR failed with %s\n", xr::ToCString(result));
		}

		return result;
	}


	// Auto-generated dispatcher handler.
	XrResult OpenXrApi::xrGetInstanceProcAddr(XrInstance instance, const char* name, PFN_xrVoidFunction* function) {
		const std::string apiName(name);

		if (apiName == "xrGetInstanceProcAddr") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetInstanceProcAddr);
		}
		else if (apiName == "xrEnumerateInstanceExtensionProperties") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrEnumerateInstanceExtensionProperties);
		}
		else if (apiName == "xrCreateInstance") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrCreateInstance);
		}
		else if (apiName == "xrDestroyInstance") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrDestroyInstance);
		}
		else if (apiName == "xrGetInstanceProperties") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetInstanceProperties);
		}
		else if (apiName == "xrPollEvent") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrPollEvent);
		}
		else if (apiName == "xrResultToString") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrResultToString);
		}
		else if (apiName == "xrStructureTypeToString") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrStructureTypeToString);
		}
		else if (apiName == "xrGetSystem") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetSystem);
		}
		else if (apiName == "xrGetSystemProperties") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetSystemProperties);
		}
		else if (apiName == "xrEnumerateEnvironmentBlendModes") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrEnumerateEnvironmentBlendModes);
		}
		else if (apiName == "xrCreateSession") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrCreateSession);
		}
		else if (apiName == "xrDestroySession") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrDestroySession);
		}
		else if (apiName == "xrEnumerateReferenceSpaces") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrEnumerateReferenceSpaces);
		}
		else if (apiName == "xrCreateReferenceSpace") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrCreateReferenceSpace);
		}
		else if (apiName == "xrGetReferenceSpaceBoundsRect") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetReferenceSpaceBoundsRect);
		}
		else if (apiName == "xrCreateActionSpace") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrCreateActionSpace);
		}
		else if (apiName == "xrLocateSpace") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrLocateSpace);
		}
		else if (apiName == "xrDestroySpace") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrDestroySpace);
		}
		else if (apiName == "xrEnumerateViewConfigurations") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrEnumerateViewConfigurations);
		}
		else if (apiName == "xrGetViewConfigurationProperties") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetViewConfigurationProperties);
		}
		else if (apiName == "xrEnumerateViewConfigurationViews") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrEnumerateViewConfigurationViews);
		}
		else if (apiName == "xrEnumerateSwapchainFormats") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrEnumerateSwapchainFormats);
		}
		else if (apiName == "xrCreateSwapchain") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrCreateSwapchain);
		}
		else if (apiName == "xrDestroySwapchain") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrDestroySwapchain);
		}
		else if (apiName == "xrEnumerateSwapchainImages") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrEnumerateSwapchainImages);
		}
		else if (apiName == "xrAcquireSwapchainImage") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrAcquireSwapchainImage);
		}
		else if (apiName == "xrWaitSwapchainImage") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrWaitSwapchainImage);
		}
		else if (apiName == "xrReleaseSwapchainImage") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrReleaseSwapchainImage);
		}
		else if (apiName == "xrBeginSession") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrBeginSession);
		}
		else if (apiName == "xrEndSession") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrEndSession);
		}
		else if (apiName == "xrRequestExitSession") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrRequestExitSession);
		}
		else if (apiName == "xrWaitFrame") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrWaitFrame);
		}
		else if (apiName == "xrBeginFrame") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrBeginFrame);
		}
		else if (apiName == "xrEndFrame") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrEndFrame);
		}
		else if (apiName == "xrLocateViews") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrLocateViews);
		}
		else if (apiName == "xrStringToPath") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrStringToPath);
		}
		else if (apiName == "xrPathToString") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrPathToString);
		}
		else if (apiName == "xrCreateActionSet") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrCreateActionSet);
		}
		else if (apiName == "xrDestroyActionSet") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrDestroyActionSet);
		}
		else if (apiName == "xrCreateAction") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrCreateAction);
		}
		else if (apiName == "xrDestroyAction") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrDestroyAction);
		}
		else if (apiName == "xrSuggestInteractionProfileBindings") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrSuggestInteractionProfileBindings);
		}
		else if (apiName == "xrAttachSessionActionSets") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrAttachSessionActionSets);
		}
		else if (apiName == "xrGetCurrentInteractionProfile") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetCurrentInteractionProfile);
		}
		else if (apiName == "xrGetActionStateBoolean") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetActionStateBoolean);
		}
		else if (apiName == "xrGetActionStateFloat") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetActionStateFloat);
		}
		else if (apiName == "xrGetActionStateVector2f") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetActionStateVector2f);
		}
		else if (apiName == "xrGetActionStatePose") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetActionStatePose);
		}
		else if (apiName == "xrSyncActions") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrSyncActions);
		}
		else if (apiName == "xrEnumerateBoundSourcesForAction") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrEnumerateBoundSourcesForAction);
		}
		else if (apiName == "xrGetInputSourceLocalizedName") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetInputSourceLocalizedName);
		}
		else if (apiName == "xrApplyHapticFeedback") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrApplyHapticFeedback);
		}
		else if (apiName == "xrStopHapticFeedback") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrStopHapticFeedback);
		}
		else if (apiName == "xrGetOpenGLGraphicsRequirementsKHR") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetOpenGLGraphicsRequirementsKHR);
		}
		else if (apiName == "xrGetVulkanInstanceExtensionsKHR") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetVulkanInstanceExtensionsKHR);
		}
		else if (apiName == "xrGetVulkanDeviceExtensionsKHR") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetVulkanDeviceExtensionsKHR);
		}
		else if (apiName == "xrGetVulkanGraphicsDeviceKHR") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetVulkanGraphicsDeviceKHR);
		}
		else if (apiName == "xrGetVulkanGraphicsRequirementsKHR") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetVulkanGraphicsRequirementsKHR);
		}
		else if (apiName == "xrGetD3D11GraphicsRequirementsKHR") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetD3D11GraphicsRequirementsKHR);
		}
		else if (apiName == "xrGetD3D12GraphicsRequirementsKHR") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetD3D12GraphicsRequirementsKHR);
		}
		else if (apiName == "xrGetVisibilityMaskKHR") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetVisibilityMaskKHR);
		}
		else if (apiName == "xrConvertWin32PerformanceCounterToTimeKHR") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrConvertWin32PerformanceCounterToTimeKHR);
		}
		else if (apiName == "xrConvertTimeToWin32PerformanceCounterKHR") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrConvertTimeToWin32PerformanceCounterKHR);
		}
		else if (apiName == "xrCreateVulkanInstanceKHR") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrCreateVulkanInstanceKHR);
		}
		else if (apiName == "xrCreateVulkanDeviceKHR") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrCreateVulkanDeviceKHR);
		}
		else if (apiName == "xrGetVulkanGraphicsDevice2KHR") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetVulkanGraphicsDevice2KHR);
		}
		else if (apiName == "xrGetVulkanGraphicsRequirements2KHR") {
			*function = reinterpret_cast<PFN_xrVoidFunction>(RUNTIME_NAMESPACE::xrGetVulkanGraphicsRequirements2KHR);
		}
		else {
			return XR_ERROR_FUNCTION_UNSUPPORTED;
		}

		return XR_SUCCESS;
	}

} // namespace RUNTIME_NAMESPACE

