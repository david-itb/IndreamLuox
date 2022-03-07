/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "device_resource_if.h"
#include "gpio_if.h"
#include "hdf_base.h"
#include "hdf_device_desc.h"
#include "osal_mem.h"
#include "securec.h"
#include "vibrator_driver.h"
#include "vibrator_driver_type.h"
#include "vibrator_linear_driver.h"

#define HDF_LOG_TAG    vibrator_linear_driver_c

struct VibratorLinearDriverData *g_linearVibratorData = NULL;
static struct VibratorLinearDriverData *GetLinearVibratorData(void)
{
    return g_linearVibratorData;
}

static int32_t StartLinearVibrator()
{
//    int32_t ret;
    struct VibratorLinearDriverData *drvData = GetLinearVibratorData();
    CHECK_VIBRATOR_NULL_PTR_RETURN_VALUE(drvData, HDF_FAILURE);

    if (drvData->busType != VIBRATOR_BUS_GPIO) {
        HDF_LOGE("%s: vibrator bus type not gpio", __func__);
        return HDF_FAILURE;
    }
        HDF_LOGE("%s: pull gpio high", __func__);
#if 0
    ret = GpioWrite(drvData->gpioNum, GPIO_VAL_HIGH);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: pull gpio%d to %d level failed, ret %d", __func__, drvData->gpioNum, GPIO_VAL_HIGH, ret);
        return HDF_FAILURE;
    }
#endif
    return HDF_SUCCESS;
}

static int32_t StartEffectLinearVibrator(uint32_t effectType)
{
    (void)effectType;
    return HDF_SUCCESS;
}

static int32_t StopLinearVibrator()
{
//    int32_t ret;
    struct VibratorLinearDriverData *drvData = GetLinearVibratorData();
    CHECK_VIBRATOR_NULL_PTR_RETURN_VALUE(drvData, HDF_FAILURE);

    if (drvData->busType != VIBRATOR_BUS_GPIO) {
        HDF_LOGE("%s: vibrator bus type not gpio", __func__);
        return HDF_FAILURE;
    }
    HDF_LOGE("%s: pull gpio low", __func__);
#if 0
    ret = GpioWrite(drvData->gpioNum, GPIO_VAL_LOW);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: pull gpio%d to %d level failed, ret %d", __func__, drvData->gpioNum, GPIO_VAL_LOW, ret);
        return HDF_FAILURE;
    }
#endif
    return HDF_SUCCESS;
}

static int32_t DispatchLinearVibrator(struct HdfDeviceIoClient *client,
    int32_t cmd, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    (void)client;
    (void)cmd;
    (void)data;
    (void)reply;

    return HDF_SUCCESS;
}

int32_t BindLinearVibratorDriver(struct HdfDeviceObject *device)
{
    CHECK_VIBRATOR_NULL_PTR_RETURN_VALUE(device, HDF_FAILURE);

    struct VibratorLinearDriverData *drvData = (struct VibratorLinearDriverData *)OsalMemCalloc(sizeof(*drvData));
    CHECK_VIBRATOR_NULL_PTR_RETURN_VALUE(drvData, HDF_ERR_MALLOC_FAIL);

    drvData->ioService.Dispatch = DispatchLinearVibrator;
    drvData->device = device;
    device->service = &drvData->ioService;
    g_linearVibratorData = drvData;

    return HDF_SUCCESS;
}

static int32_t ParserLinearConfig(const struct DeviceResourceNode *node, struct VibratorLinearDriverData *drvData)
{
    int32_t ret;
    struct DeviceResourceIface *parser = NULL;

    CHECK_VIBRATOR_NULL_PTR_RETURN_VALUE(node, HDF_FAILURE);
    CHECK_VIBRATOR_NULL_PTR_RETURN_VALUE(drvData, HDF_FAILURE);

    parser = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    CHECK_VIBRATOR_NULL_PTR_RETURN_VALUE(parser, HDF_ERR_INVALID_PARAM);
    CHECK_VIBRATOR_NULL_PTR_RETURN_VALUE(parser->GetChildNode, HDF_ERR_INVALID_PARAM);

    const struct DeviceResourceNode *configNode = parser->GetChildNode(node, "vibratorChipConfig");
    ret = parser->GetUint32(configNode, "busType", &drvData->busType, 0);
    CHECK_VIBRATOR_PARSER_RESULT_RETURN_VALUE(ret, "busType");
    if (drvData->busType == VIBRATOR_BUS_GPIO) {
        ret = parser->GetUint32(configNode, "gpioNum", &drvData->gpioNum, 0);
        CHECK_VIBRATOR_PARSER_RESULT_RETURN_VALUE(ret, "gpioNum");
    }

    ret = parser->GetUint32(configNode, "startReg", &drvData->startReg, 0);
    CHECK_VIBRATOR_PARSER_RESULT_RETURN_VALUE(ret, "startReg");
    ret = parser->GetUint32(configNode, "stopReg", &drvData->stopReg, 0);
    CHECK_VIBRATOR_PARSER_RESULT_RETURN_VALUE(ret, "stopReg");
    ret = parser->GetUint32(configNode, "startMask", &drvData->mask, 0);
    CHECK_VIBRATOR_PARSER_RESULT_RETURN_VALUE(ret, "startMask");

    return HDF_SUCCESS;
}

int32_t InitLinearVibratorDriver(struct HdfDeviceObject *device)
{
    static struct VibratorOps ops;

    CHECK_VIBRATOR_NULL_PTR_RETURN_VALUE(device, HDF_FAILURE);

    struct VibratorLinearDriverData *drvData = (struct VibratorLinearDriverData *)device->service;
    CHECK_VIBRATOR_NULL_PTR_RETURN_VALUE(drvData, HDF_FAILURE);

    ops.Start = StartLinearVibrator;
    ops.StartEffect = StartEffectLinearVibrator;
    ops.Stop = StopLinearVibrator;

    if (RegisterVibrator(&ops) != HDF_SUCCESS) {
        HDF_LOGE("%s: register vibrator ops fail", __func__);
        return HDF_FAILURE;
    }

    if (ParserLinearConfig(device->property, drvData) != HDF_SUCCESS) {
        HDF_LOGE("%s: parser vibrator cfg fail", __func__);
        return HDF_FAILURE;
    }

    if (GpioSetDir(drvData->gpioNum, GPIO_DIR_OUT) != HDF_SUCCESS) {
        HDF_LOGE("%s: set vibrator gpio fail", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

void ReleaseLinearVibratorDriver(struct HdfDeviceObject *device)
{
    struct VibratorLinearDriverData *drvData = (struct VibratorLinearDriverData *)device->service;
    if (device == NULL || drvData == NULL) {
        HDF_LOGE("%s: pointer is null and return errno", __func__);
        return;
    }

    (void)OsalMemFree(drvData);
    g_linearVibratorData = NULL;
}

struct HdfDriverEntry g_linearVibratorDriverEntry = {
    .moduleVersion = 1,
    .moduleName = "HDF_LINEAR_VIBRATOR",
    .Bind = BindLinearVibratorDriver,
    .Init = InitLinearVibratorDriver,
    .Release = ReleaseLinearVibratorDriver,
};

HDF_INIT(g_linearVibratorDriverEntry);
