#include <stdio.h>

#include <yoo-stdlib.h>
#include <poker.h>

#ifdef HAS_OPENCL

#include <CL/cl.h>

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
    __global ulong * restrict debug = args + 5;\n\
\n\
    const uint idx_report = get_global_id(0);\n\
    const uint idx_data = qparts * get_global_id(0);\n\
\n\
    report[idx_report] = 0;\n\
\n\
    uint cards0[12];\n\
    int j = 0;\n\
    for (uint i=0; i<qparts; ++i) {\n\
        ulong mask = data[idx_data + i];\n\
        while (mask != 0) {\n\
            const ulong one_bit = mask & (-mask);\n\
            mask ^= one_bit;\n\
            cards0[j++] = 63 - clz(one_bit);\n\
        }\n\
    }\n\
\n\
    uint rank0 = start_state;\n\
    for (uint i=0; i<n; ++i) {\n\
        rank0 = fsm[rank0 + cards0[i]];\n\
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
        for (uint i=0; i<n; ++i) {\n\
            cards[i] = cards0[perm[i]];\n\
        }\n\
\n\
        uint rank = start_state;\n\
        for (uint i=0; i<n; ++i) {\n\
            rank = fsm[rank + cards[i]];\n\
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



#define IF__CMD_QUEUE       1
#define IF__SCALAR_MEM      2
#define IF__PERM_TABLE_MEM  4
#define IF__FSM_MEM         8
#define IF__PROGRAM        16
#define IF__KERNEL         32

struct opencl_permutations
{
    const struct opencl_permunation_args * args;

    cl_ulong scalars[5];
    size_t scalar_sz;

    int init_mask;
    cl_command_queue cmd_queue;
    cl_mem scalar_mem;
    cl_mem perm_table_mem;
    cl_mem fsm_mem;
    cl_program program;
    cl_kernel kernel;
};

cl_int opencl_permutations_init(struct opencl_permutations * restrict const me)
{
    cl_int status;

    me->cmd_queue = clCreateCommandQueueWithProperties(opencl_state.context, opencl_state.device, NULL, &status);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  clCreateCommandQueueWithProperties(context, device, NULL, &status) fails with code %d.\n", status);
        return 1;
    }

    me->init_mask |= IF__CMD_QUEUE;

    /* Allocate memory on device */

    me->scalar_sz = sizeof(me->scalars);
    me->scalar_mem = clCreateBuffer(opencl_state.context, CL_MEM_READ_WRITE, me->scalar_sz, NULL, &status);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  scalar_mem = clCreateBuffer(opencl_state.context, CL_MEM_READ_WRITE, %lu, NULL, &status) fails with code %d.\n", me->scalar_sz, status);
        return 1;
    }

    me->init_mask |= IF__SCALAR_MEM;

    me->perm_table_mem = clCreateBuffer(opencl_state.context, CL_MEM_READ_ONLY, me->args->perm_table_sz, NULL, &status);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  perm_table_mem = clCreateBuffer(context, CL_MEM_READ_ONLY, %lu, NULL, &status) fails with code %d.\n", me->args->perm_table_sz, status);
        return 1;
    }

    me->init_mask |= IF__PERM_TABLE_MEM;

    me->fsm_mem = clCreateBuffer(opencl_state.context, CL_MEM_READ_ONLY, me->args->fsm_sz, NULL, &status);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  fsm_mem = clCreateBuffer(context, CL_MEM_READ_ONLY, %lu, NULL, &status) fails with code %d.\n", me->args->fsm_sz, status);
        return 1;
    }

    me->init_mask |= IF__FSM_MEM;

    /* Copy memory from host to device */

    status = clEnqueueWriteBuffer(me->cmd_queue, me->scalar_mem, CL_FALSE, 0, me->scalar_sz, me->scalars, 0, NULL, NULL);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  clEnqueueWriteBuffer(cmd_queue, args_mem, CL_FALSE, 0, %lu, %p, 0, NULL, NULL) fails with code %d.\n", me->scalar_sz, me->scalars, status);
        return 1;
    }

    status = clEnqueueWriteBuffer(me->cmd_queue, me->perm_table_mem, CL_FALSE, 0, me->args->perm_table_sz, me->args->perm_table, 0, NULL, NULL);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  clEnqueueWriteBuffer(cmd_queue, perm_table_mem, CL_FALSE, 0, %lu, %p, 0, NULL, NULL) fails with code %d.\n", me->args->perm_table_sz, me->args->perm_table, status);
        return 1;
    }

    status = clEnqueueWriteBuffer(me->cmd_queue, me->fsm_mem, CL_FALSE, 0, me->args->fsm_sz, me->args->fsm, 0, NULL, NULL);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  clEnqueueWriteBuffer(cmd_queue, fsm_mem, CL_FALSE, 0, %lu, %p, 0, NULL, NULL) fails with code %d.\n", me->args->fsm_sz, me->args->fsm, status);
        return 1;
    }

    /* Compile kernel */

    me->program = clCreateProgramWithSource(opencl_state.context, 1, &test_permutations_source, NULL, &status);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  clCreateProgramWithSource(context, 1, src, NULL, &status) failed with code %d.\n", status);
        printf("Source:\n%s\n", test_permutations_source);
        return 1;
    }

    me->init_mask |= IF__PROGRAM;

    status = clBuildProgram(me->program, 1, &opencl_state.device, NULL, NULL, NULL);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("Source:\n%s\n", test_permutations_source);
        printf("  clBuildProgram failed: %d\n", status);

        char msg[0x10000];
        status = clGetProgramBuildInfo(me->program, opencl_state.device, CL_PROGRAM_BUILD_LOG, 0x10000, msg, NULL);
        if (status == CL_SUCCESS) {
            printf("\n%s\n", msg);
        } else {
            printf("  clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0x10000, msg, NULL) fails with code %d.", status);
        }

        return 1;
    }

    const char * kernel_name = "test_permutations";
    me->kernel = clCreateKernel(me->program, kernel_name, &status);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  clCreateKernel(program, “%s”, &status) fails with code %d.\n", kernel_name, status);
        return 1;
    }

    return 0;
}

void * create_opencl_permutations(const struct opencl_permunation_args * const args)
{
    if (!opencl_state.is_init) {
        printf("[FAIL] (OpenCL)\n");
        printf("  OpenCL is not init.\n");
        return NULL;
    }

    size_t sz = sizeof(struct opencl_permutations);
    void * ptr = malloc(sz);
    if (ptr == NULL) {
        printf("[FAIL] (OpenCL)\n");
        printf("  malloc(%lu) failed with NULL as a result value.\n", sz);
        return NULL;
    }

    struct opencl_permutations * restrict const me = ptr;
    me->args = args;

    me->scalars[0] = args->qdata;
    me->scalars[1] = args->n;
    me->scalars[2] = args->start_state;
    me->scalars[3] = args->qparts;
    me->scalars[4] = 0;

    int status = opencl_permutations_init(me);
    if (status != 0) {
        free_opencl_permutations(me);
        return NULL;
    }

    return me;

}

void free_opencl_permutations(void * handle)
{
    struct opencl_permutations * restrict const me = handle;

    if (me->init_mask & IF__CMD_QUEUE) {
        clReleaseCommandQueue(me->cmd_queue);
    }

    if (me->init_mask & IF__SCALAR_MEM) {
        clReleaseMemObject(me->scalar_mem);
    }

    if (me->init_mask & IF__PERM_TABLE_MEM) {
        clReleaseMemObject(me->perm_table_mem);
    }

    if (me->init_mask & IF__FSM_MEM) {
        clReleaseMemObject(me->fsm_mem);
    }

    if (me->init_mask & IF__PROGRAM) {
        clReleaseProgram(me->program);
    }

    if (me->init_mask & IF__KERNEL) {
        clReleaseKernel(me->kernel);
    }

    free(handle);
}

int run_opencl_permutations(
    void * handle,
    uint64_t qdata,
    const uint64_t * const data,
    uint16_t * restrict const report,
    uint64_t * restrict const qerrors)
{
    struct opencl_permutations * restrict const me = handle;

    const uint64_t report_sz = qdata * sizeof(report[0]);
    const uint64_t data_sz = me->args->qparts * qdata * sizeof(data[0]);

    cl_int status;

    cl_mem data_mem = clCreateBuffer(opencl_state.context, CL_MEM_READ_ONLY, data_sz, NULL, &status);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  data_mem = clCreateBuffer(context, CL_MEM_READ_ONLY, %lu, NULL, &status) fails with code %d.\n", data_sz, status);
        return 1;
    }

    cl_mem report_mem = clCreateBuffer(opencl_state.context, CL_MEM_WRITE_ONLY, report_sz, NULL, &status);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  report_mem = clCreateBuffer(context, CL_MEM_WRITE_ONLY, %lu, NULL, &status) fails with code %d.\n", report_sz, status);
        clReleaseMemObject(data_mem);
        return 1;
    }

    status = clEnqueueWriteBuffer(me->cmd_queue, data_mem, CL_FALSE, 0, data_sz, data, 0, NULL, NULL);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  clEnqueueWriteBuffer(cmd_queue, data_mem, CL_FALSE, 0, %lu, %p, 0, NULL, NULL) fails with code %d.\n", data_sz, data, status);
        clReleaseMemObject(data_mem);
        clReleaseMemObject(report_mem);
        return 1;
    }

    /* Set arguments */

    status = clSetKernelArg(me->kernel, 0, sizeof(cl_mem), &me->scalar_mem);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  clSetKernelArg(kernel, 0, %lu, &args_mem) fails with code %d.\n", sizeof(cl_mem), status);
        clReleaseMemObject(data_mem);
        clReleaseMemObject(report_mem);
        return 1;
    }

    status = clSetKernelArg(me->kernel, 1, sizeof(cl_mem), &me->perm_table_mem);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  clSetKernelArg(kernel, 1, %lu, &perm_table_mem) fails with code %d.\n", sizeof(cl_mem), status);
        clReleaseMemObject(data_mem);
        clReleaseMemObject(report_mem);
        return 1;
    }

    status = clSetKernelArg(me->kernel, 2, sizeof(cl_mem), &me->fsm_mem);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  clSetKernelArg(kernel, 2, %lu, &fsm_mem) fails with code %d.\n", sizeof(cl_mem), status);
        clReleaseMemObject(data_mem);
        clReleaseMemObject(report_mem);
        return 1;
    }

    status = clSetKernelArg(me->kernel, 3, sizeof(cl_mem), &data_mem);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  clSetKernelArg(kernel, 3, %lu, &data_mem) fails with code %d.\n", sizeof(cl_mem), status);
        clReleaseMemObject(data_mem);
        clReleaseMemObject(report_mem);
        return 1;
    }

    status = clSetKernelArg(me->kernel, 4, sizeof(cl_mem), &report_mem);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  clSetKernelArg(kernel, 4, %lu, &report_mem) fails with code %d.\n", sizeof(cl_mem), status);
        clReleaseMemObject(data_mem);
        clReleaseMemObject(report_mem);
        return 1;
    }

    /* Run */

    cl_ulong global_work_sz[1] = { qdata };
    status = clEnqueueNDRangeKernel(me->cmd_queue, me->kernel, 1, NULL, global_work_sz, NULL, 0, NULL, NULL);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  clEnqueueNDRangeKernel(cmd_queue, kernel, 1, NULL, { %lu }, NULL, 0, NULL, NULL) fails with code %d.\n", global_work_sz[0], status);;
        clReleaseMemObject(data_mem);
        clReleaseMemObject(report_mem);
        return 1;
    }

    /* Get data */

    status = clEnqueueReadBuffer(me->cmd_queue, me->scalar_mem, CL_TRUE, 0, me->scalar_sz, me->scalars, 0, NULL, NULL);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  clEnqueueReadBuffer(cmd_queue, scalar_mem, CL_TRUE, 0, %lu, scalars, 0, NULL, NULL) fails with code %d.\n", me->scalar_sz, status);
        clReleaseMemObject(data_mem);
        clReleaseMemObject(report_mem);
        return 1;
    }

    status = clEnqueueReadBuffer(me->cmd_queue, report_mem, CL_TRUE, 0, report_sz, report, 0, NULL, NULL);
    if (status != CL_SUCCESS) {
        printf("[FAIL] (OpenCL)\n");
        printf("  clEnqueueReadBuffer(cmd_queue, report_mem, CL_TRUE, 0, %lu, report, 0, NULL, NULL) fails with code %d.\n", report_sz, status);
        clReleaseMemObject(data_mem);
        clReleaseMemObject(report_mem);
        return 1;
    }

    *qerrors = me->scalars[4];
    clReleaseMemObject(data_mem);
    clReleaseMemObject(report_mem);

    return 0;
}



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

void * create_opencl_permutations(const struct opencl_permunation_args * const args)
{
    return NULL;
}

void free_opencl_permutations(void * handle)
{
}

int run_opencl_permutations(
    void * handle,
    uint64_t qdata,
    const uint64_t * const data,
    uint16_t * restrict const report,
    uint64_t * restrict const qerrors
)
{
    return 1;
}

int opencl__test_permutations(
    uint64_t * restrict args,
    const int8_t * const perm_table, const int64_t perm_table_sz,
    const uint32_t * const fsm, const int64_t fsm_sz,
    const uint64_t * const data, const int64_t data_sz,
    uint16_t * restrict const report, const int64_t report_sz
)
{
    printf("[FAIL]\n");
    printf("  Not implemented.\n");
    return 1;
}

#endif
