#include <stdio.h>

#ifdef HAS_OPENCL

#include <CL/cl.h>

#include <yoo-stdlib.h>

struct opencl_state
{
    int is_init;
    cl_context context;
};

static struct opencl_state opencl_state;

int init_opencl(FILE * err)
{
    if (opencl_state.is_init) {
        return 0;
    }

    cl_int status;

    cl_uint qplatforms = 0;
    status = clGetPlatformIDs(0, NULL, &qplatforms);
    if (status != CL_SUCCESS) {
        msg(err, "OpenCL: clGetPlatformIDs(0, NULL, &qplatforms) fails with code %u.", status);
        return 1;
    }

    if (qplatforms == 0) {
        msg(err, "OpenCL: Platform is not found.");
        return 1;
    }

    cl_platform_id platforms[qplatforms];

    status = clGetPlatformIDs(qplatforms, platforms, NULL);
    if (status != CL_SUCCESS) {
        msg(err, "clGetPlatformIDs(%u, platforms, NULL) fails with code %u.", qplatforms, status);
    }

    cl_uint qdevices = 0;
    status = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, 0, NULL, &qdevices);
    if (status != CL_SUCCESS) {
        msg(err, "clGetDeviceIDs(id, CL_DEVICE_TYPE_GPU, 0, NULL, &qdevices) fails with code %u.", status);
        return 1;
    }

    if (qdevices == 0) {
        msg(err, "OpenCL: No GPU devices was found.");
        return 1;
    }

    cl_device_id devices[qdevices];

    status = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, qdevices, devices, NULL);
    if (status != CL_SUCCESS) {
        msg(err, "clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, %u, devices, NULL) fails with code %u.", qdevices, status);
        return 1;
    }

    opencl_state.context = clCreateContext(NULL, 1, devices, NULL, NULL, &status);
    if (status != CL_SUCCESS) {
        msg(err, "clCreateContext(NULL, 1, devices, NULL, NULL, &status) fails with code %u.", status);
        return 1;
    }

    opencl_state.is_init = 1;
    msg(stdout, "Open CL initialization complete.");
    return 0;
}

void free_opencl(void)
{
    if (opencl_state.is_init) {
        clReleaseContext(opencl_state.context);
    }

    opencl_state.is_init = 0;
}

#else

int init_opencl(FILE * err)
{
    return -1;
}

#endif
