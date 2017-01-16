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
#pragma OPENCL EXTENSION cl_khr_int64_base_atomics : enable\n\
\n\
__kernel\n\
void test_permutations(\n\
    /* 0 */ __global ulong * args,\n\
    /* 1 */ __constant const char * perm_table,\n\
    /* 2 */ __global const uint * fsm,\n\
    /* 3 */ __global const ulong * data,\n\
    /* 4 */ __global short * restrict report\n\
)\n\
{\n\
    const uint n = args[1];\n\
    const uint start_state = args[2];\n\
    const uint qparts = args[3];\n\
    __global ulong * restrict qerror = args + 4;\n\
\n\
    const uint idx_report = get_global_id(0);\n\
    const uint idx_data = qparts * get_global_id(0);\n\
\n\
    report[idx_report] = 0;\n\
\n\
    uint cards0[12];\n\
    for (uint i=0; i<qparts; ++i) {\n\
        ulong mask = data[idx_data + i];\n\
        int j = 0;\n\
        while (mask != 0) {\n\
            const ulong one_bit = mask & (-mask);\n\
            mask ^= one_bit;\n\
            cards0[j++] = 63 - clz(one_bit);\n\
        }\n\
    }\n\
\n\
    uint rank0 = start_state;\n\
    for (uint j=0; j<n; ++j) {\n\
        rank0 = fsm[rank0 + cards0[j]];\n\
    }\n\
\n\
    if (rank0 == 0) {\n\
        atom_inc(qerror);\n\
        report[idx_report] = 0xFFFF;\n\
        return;\n\
    }\n\
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
            atom_inc(qerror);\n\
            report[idx_report] = perm_index;\n\
            return;\n\
        }\n\
\n\
        perm += n;\n\
    }\n\
}\n\
";

int opencl__test_permutations(
    cl_ulong * restrict args,
    const cl_char * const perm_table, const cl_long perm_table_sz,
    const cl_uint * const fsm, const cl_long fsm_sz,
    const cl_ulong * const data, const cl_long data_sz,
    cl_ushort * restrict const report, const cl_long report_sz)
{
    cl_ulong qdata = args[0];
    size_t args_sz = 5 * sizeof(cl_ulong);

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

    cl_mem args_mem = clCreateBuffer(opencl_state.context, CL_MEM_READ_WRITE, args_sz, NULL, &status);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  args_mem = clCreateBuffer(opencl_state.context, CL_MEM_READ_WRITE, %lu, NULL, &status) fails with code %d.\n", args_sz, status);
        goto release_cmd_queue;
    }

    cl_mem perm_table_mem = clCreateBuffer(opencl_state.context, CL_MEM_READ_ONLY, perm_table_sz, NULL, &status);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  perm_table_mem = clCreateBuffer(context, CL_MEM_READ_ONLY, %lu, NULL, &status) fails with code %d.\n", perm_table_sz, status);
        goto release_args_mem;
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

    status = clEnqueueWriteBuffer(cmd_queue, args_mem, CL_FALSE, 0, args_sz, args, 0, NULL, NULL);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  clEnqueueWriteBuffer(cmd_queue, args_mem, CL_FALSE, 0, %lu, %p, 0, NULL, NULL) fails with code %d.\n", args_sz, args, status);
        goto release_report_mem;
    }

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

    status = clSetKernelArg(kernel, 0, sizeof(cl_mem), &args_mem);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  clSetKernelArg(kernel, 0, %lu, &args_mem) fails with code %d.\n", sizeof(cl_mem), status);
        goto release_kernel;
    }

    status = clSetKernelArg(kernel, 1, sizeof(cl_mem), &perm_table_mem);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  clSetKernelArg(kernel, 1, %lu, &perm_table_mem) fails with code %d.\n", sizeof(cl_mem), status);
        goto release_kernel;
    }

    status = clSetKernelArg(kernel, 2, sizeof(cl_mem), &fsm_mem);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  clSetKernelArg(kernel, 2, %lu, &fsm_mem) fails with code %d.\n", sizeof(cl_mem), status);
        goto release_kernel;
    }

    status = clSetKernelArg(kernel, 3, sizeof(cl_mem), &data_mem);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  clSetKernelArg(kernel, 3, %lu, &data_mem) fails with code %d.\n", sizeof(cl_mem), status);
        goto release_kernel;
    }

    status = clSetKernelArg(kernel, 4, sizeof(cl_mem), &report_mem);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  clSetKernelArg(kernel, 4, %lu, &report_mem) fails with code %d.\n", sizeof(cl_mem), status);
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
    release_args_mem:
        clReleaseMemObject(args_mem);
    release_cmd_queue:
        clReleaseCommandQueue(cmd_queue);
    exit_from_test:
        return result;
}

#else

#include <stdint.h>

int init_opencl(FILE * err)
{
    return -1;
}

void free_opencl(void)
{
}

int opencl__test_permutations(
    const int32_t n, const uint32_t start_state, const uint32_t qdata,
    const int8_t * const perm_table, const uint64_t perm_table_sz,
    const uint32_t * const fsm, const uint64_t fsm_sz,
    const uint64_t * const data, const uint64_t data_sz,
    uint16_t * restrict const report, const uint64_t report_sz
)
{
    printf("[FAIL]\n");
    printf("  Not implemented.\n");
    return 1;
}

#endif
