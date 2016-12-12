#include <stdio.h>

#ifdef HAS_OPENCL

#include <CL/cl.h>

#include <yoo-stdlib.h>

struct opencl_state
{
    int is_init;
    cl_device_id device;
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
        msg(err, "OpenCL: clGetPlatformIDs(0, NULL, &qplatforms) fails with code %d.", status);
        return 1;
    }

    if (qplatforms == 0) {
        msg(err, "OpenCL: Platform is not found.");
        return 1;
    }

    cl_platform_id platforms[qplatforms];

    status = clGetPlatformIDs(qplatforms, platforms, NULL);
    if (status != CL_SUCCESS) {
        msg(err, "clGetPlatformIDs(%u, platforms, NULL) fails with code %d.", qplatforms, status);
    }

    cl_uint qdevices = 0;
    status = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, 0, NULL, &qdevices);
    if (status != CL_SUCCESS) {
        msg(err, "clGetDeviceIDs(id, CL_DEVICE_TYPE_GPU, 0, NULL, &qdevices) fails with code %d.", status);
        return 1;
    }

    if (qdevices == 0) {
        msg(err, "OpenCL: No GPU devices was found.");
        return 1;
    }

    cl_device_id devices[qdevices];

    status = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, qdevices, devices, NULL);
    if (status != CL_SUCCESS) {
        msg(err, "clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, %u, devices, NULL) fails with code %d.", qdevices, status);
        return 1;
    }

    opencl_state.device = devices[0];
    opencl_state.context = clCreateContext(NULL, 1, devices, NULL, NULL, &status);
    if (status != CL_SUCCESS) {
        msg(err, "clCreateContext(NULL, 1, devices, NULL, NULL, &status) fails with code %d.", status);
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

const char * test_permutations_source = "\
__kernel\n\
void test_permutations(\n\
    const uint n, const uint start_state,\n\
    __constant const char * perm_table,\n\
    __global const uint * fsm,\n\
    __global const ulong * data,\n\
    __global short * restrict report\n\
)\n\
{\n\
    const uint i = get_global_id(0);\n\
\n\
    report[i] = 0;\n\
\n\
    ulong mask = data[i];\n\
    uint cards0[12];\n\
    for (uint j=0; j<n; ++j) {\n\
        const ulong one_bit = mask & (-mask);\n\
        mask ^= one_bit;\n\
        cards0[j] = 63 - clz(one_bit);\n\
    }\n\
\n\
    uint rank0 = start_state;\n\
    for (uint j=0; j<n; ++j) {\n\
        rank0 = fsm[rank0 + cards0[j]];\n\
    }\n\
\n\
    if (rank0 == 0) {\n\
        report[i] = 0xFFFF;\n\
        return;\n\
    }\n\
\n\
\n\
    int perm_index = 1;\n\
    __constant const char * perm = perm_table + n;\n\
    uint cards[12];\n\
    for (;; ++perm_index) {\n\
\n\
        if (*perm < 0) {\n\
            break;\n\
        }\n\
\n\
        for (uint j=0; j<n; ++j) {\n\
            cards[j] = cards0[perm[j]];\n\
        }\n\
\n\
        uint rank = start_state;\n\
        for (uint j=0; j<n; ++j) {\n\
            rank = fsm[rank + cards[j]];\n\
        }\n\
\n\
        if (rank != rank0) {\n\
            report[i] = perm_index;\n\
        }\n\
\n\
        perm += n;\n\
    }\n\
}\n\
";

int opencl__test_permutations(
    const cl_uint n, const cl_uint start_state, const cl_uint qdata,
    const cl_char * const perm_table, const cl_long perm_table_sz,
    const cl_uint * const fsm, const cl_long fsm_sz,
    const cl_ulong * const data, const cl_long data_sz,
    cl_short * restrict const report, const cl_long report_sz)
{
    if (!opencl_state.is_init) {
        printf("[FAIL] (OpenCL)\n");
        printf("  OpenCL is not init.\n");
        return 1;
    }

    int result = 1;
    cl_int status;

    cl_command_queue cmd_queue = clCreateCommandQueueWithProperties(opencl_state.context, opencl_state.device, NULL, &status);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  clCreateCommandQueueWithProperties(context, device, NULL, &status) fails with code %d.\n", status);
        goto exit_from_test;
    }

    /* Allocate memory on device */

    cl_mem perm_table_mem = clCreateBuffer(opencl_state.context, CL_MEM_READ_ONLY, perm_table_sz, NULL, &status);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  perm_table_mem = clCreateBuffer(context, CL_MEM_READ_ONLY, %lu, NULL, &status) fails with code %d.\n", perm_table_sz, status);
        goto release_cmd_queue;
    }

    cl_mem fsm_mem = clCreateBuffer(opencl_state.context, CL_MEM_READ_ONLY, fsm_sz, NULL, &status);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  fsm_mem = clCreateBuffer(context, CL_MEM_READ_ONLY, %lu, NULL, &status) fails with code %d.\n", fsm_sz, status);
        goto release_perm_table_mem;
    }

    cl_mem data_mem = clCreateBuffer(opencl_state.context, CL_MEM_READ_ONLY, data_sz, NULL, &status);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  data_mem = clCreateBuffer(context, CL_MEM_READ_ONLY, %lu, NULL, &status) fails with code %d.\n", data_sz, status);
        goto release_fsm_mem;
    }

    cl_mem report_mem = clCreateBuffer(opencl_state.context, CL_MEM_WRITE_ONLY, report_sz, NULL, &status);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  report_mem = clCreateBuffer(context, CL_MEM_WRITE_ONLY, %lu, NULL, &status) fails with code %d.\n", report_sz, status);
        goto release_data_mem;
    }

    /* Copy memory from host to device */

    status = clEnqueueWriteBuffer(cmd_queue, perm_table_mem, CL_FALSE, 0, perm_table_sz, perm_table, 0, NULL, NULL);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  clEnqueueWriteBuffer(cmd_queue, perm_table_mem, CL_FALSE, 0, %lu, %p, 0, NULL, NULL) fails with code %d.\n", perm_table_sz, perm_table, status);
        goto release_report_mem;
    }

    status = clEnqueueWriteBuffer(cmd_queue, fsm_mem, CL_FALSE, 0, fsm_sz, fsm, 0, NULL, NULL);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  clEnqueueWriteBuffer(cmd_queue, fsm_mem, CL_FALSE, 0, %lu, %p, 0, NULL, NULL) fails with code %d.\n", fsm_sz, fsm, status);
        goto release_report_mem;
    }

    status = clEnqueueWriteBuffer(cmd_queue, data_mem, CL_FALSE, 0, data_sz, data, 0, NULL, NULL);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  clEnqueueWriteBuffer(cmd_queue, data_mem, CL_FALSE, 0, %lu, %p, 0, NULL, NULL) fails with code %d.\n", data_sz, data, status);
        goto release_report_mem;
    }

    /* Compile kernel */

    cl_program program = clCreateProgramWithSource(opencl_state.context, 1, &test_permutations_source, NULL, &status);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  clCreateProgramWithSource(context, 1, src, NULL, &status) failed with code %d.\n", status);
        printf("Source:\n%s\n", test_permutations_source);
        goto release_report_mem;
    }

    status = clBuildProgram(program, 1, &opencl_state.device, NULL, NULL, NULL);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("Source:\n%s\n", test_permutations_source);
        printf("  clBuildProgram failed: %d\n", status);

        char msg[0x10000];
        status = clGetProgramBuildInfo(program, opencl_state.device, CL_PROGRAM_BUILD_LOG, 0x10000, msg, NULL);
        if (status == CL_SUCCESS) {
            printf("\n%s\n", msg);
        } else {
            printf("  clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0x10000, msg, NULL) fails with code %d.", status);
        }

        goto release_program;
    }

    const char * kernel_name = "test_permutations";
    cl_kernel kernel = clCreateKernel(program, kernel_name, &status);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  clCreateKernel(program, “%s”, &status) fails with code %d.\n", kernel_name, status);
        goto release_program;
    }

    /* Set arguments */

    status = clSetKernelArg(kernel, 0, sizeof(cl_uint), &n);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  clSetKernelArg(kernel, 0, %lu, &n = [%u]) fails with code %d.\n", sizeof(cl_uint), n, status);
        goto release_kernel;
    }

    status = clSetKernelArg(kernel, 1, sizeof(cl_uint), &start_state);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  clSetKernelArg(kernel, 1, %lu, &n = [%u]) fails with code %d.\n", sizeof(cl_uint), start_state, status);
        goto release_kernel;
    }

    status = clSetKernelArg(kernel, 2, sizeof(cl_mem), &perm_table_mem);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  clSetKernelArg(kernel, 2, %lu, &perm_table_mem) fails with code %d.\n", sizeof(cl_mem), status);
        goto release_kernel;
    }

    status = clSetKernelArg(kernel, 3, sizeof(cl_mem), &fsm_mem);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  clSetKernelArg(kernel, 3, %lu, &fsm_mem) fails with code %d.\n", sizeof(cl_mem), status);
        goto release_kernel;
    }

    status = clSetKernelArg(kernel, 4, sizeof(cl_mem), &data_mem);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  clSetKernelArg(kernel, 4, %lu, &data_mem) fails with code %d.\n", sizeof(cl_mem), status);
        goto release_kernel;
    }

    status = clSetKernelArg(kernel, 5, sizeof(cl_mem), &report_mem);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  clSetKernelArg(kernel, 5, %lu, &report_mem) fails with code %d.\n", sizeof(cl_mem), status);
        goto release_kernel;
    }

    /* Run */

    cl_ulong global_work_sz[1] = { qdata };
    status = clEnqueueNDRangeKernel(cmd_queue, kernel, 1, NULL, global_work_sz, NULL, 0, NULL, NULL);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  clEnqueueNDRangeKernel(cmd_queue, kernel, 1, NULL, { %lu }, NULL, 0, NULL, NULL) fails with code %d.\n", global_work_sz[0], status);;
        goto release_kernel;
    }

    /* Get data */

    status = clEnqueueReadBuffer(cmd_queue, report_mem, CL_TRUE, 0, report_sz, report, 0, NULL, NULL);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  clEnqueueReadBuffer(cmd_queue, report_mem, CL_TRUE, 0, %lu, report, 0, NULL, NULL) fails with code %d.\n", report_sz, status);
        goto release_kernel;
    }

    result = 0;

    release_kernel:
        clReleaseKernel(kernel);
    release_program:
        clReleaseProgram(program);
    release_report_mem:
        clReleaseMemObject(report_mem);
    release_data_mem:
        clReleaseMemObject(data_mem);
    release_fsm_mem:
        clReleaseMemObject(fsm_mem);
    release_perm_table_mem:
        clReleaseMemObject(perm_table_mem);
    release_cmd_queue:
        clReleaseCommandQueue(cmd_queue);
    exit_from_test:
        return result;
}

#else

int init_opencl(FILE * err)
{
    return -1;
}

void free_opencl(void)
{
}

int opencl__test_permutations(
    const cl_uint n, const cl_uint start_state, const cl_uint qdata,
    const cl_int * const perm_table, const cl_long perm_table_sz,
    const cl_uint * const fsm, const cl_long fsm_sz,
    const cl_ulong * const data, const cl_long data_sz,
    cl_short * restrict const report, const cl_long report_sz)
{
    printf("[FAIL]\n");
    printf("  Not implemented.\n");
    return 1;
}

#endif
