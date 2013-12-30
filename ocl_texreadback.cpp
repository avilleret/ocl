////////////////////////////////////////////////////////
//
// GEM - Graphics Environment for Multimedia
//
// zmoelnig@iem.kug.ac.at
//
// Implementation file
//
//    Copyright (c) 1997-2000 Mark Danks.
//    Copyright (c) Günther Geiger.
//    Copyright (c) 2001-2011 IOhannes m zmölnig. forum::für::umläute. IEM. zmoelnig@iem.at
//    For information on usage and redistribution, and for a DISCLAIMER OF ALL
//    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.
//
/////////////////////////////////////////////////////////

const int ARRAY_SIZE = 100;

#include "ocl_texreadback.hpp"

CPPEXTERN_NEW_WITH_ONE_ARG(ocl_texreadback, t_floatarg, A_DEFFLOAT);

///
//  Create an OpenCL context on the first available platform using
//  either a GPU or CPU depending on what is available.
//
///
//  Create an OpenCL context on the first available platform using
//  either a GPU or CPU depending on what is available.
//
cl_context ocl_texreadback :: CreateContext()
{
    cl_int errNum;
    cl_uint numPlatforms;
    cl_platform_id firstPlatformId;
    cl_context context = NULL;

    // First, select an OpenCL platform to run on.  For this example, we
    // simply choose the first available platform.  Normally, you would
    // query for all available platforms and select the most appropriate one.
    errNum = clGetPlatformIDs(1, &firstPlatformId, &numPlatforms);
    if (errNum != CL_SUCCESS || numPlatforms <= 0)
    {
        std::cerr << "Failed to find any OpenCL platforms." << std::endl;
        return NULL;
    }

    // Next, create an OpenCL context on the platform.  Attempt to
    // create a GPU-based context, and if that fails, try to create
    // a CPU-based context.
    cl_context_properties contextProperties[] =
    {
#ifdef _WIN32
        CL_CONTEXT_PLATFORM,
        (cl_context_properties)firstPlatformId,
		CL_GL_CONTEXT_KHR,
		(cl_context_properties)wglGetCurrentContext(),
		CL_WGL_HDC_KHR,
		(cl_context_properties)wglGetCurrentDC(),
#elif defined( __GNUC__)
		CL_CONTEXT_PLATFORM, (cl_context_properties)firstPlatformId, 
		CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext(), 
		CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay(), 
#elif defined(__APPLE__) 
		//todo
#endif
        0



    };
	cl_uint uiDevCount;
    cl_device_id* cdDevices;
	// Get the number of GPU devices available to the platform
    errNum = clGetDeviceIDs(firstPlatformId, CL_DEVICE_TYPE_GPU, 0, NULL, &uiDevCount);

    // Create the device list
    cdDevices = new cl_device_id [uiDevCount];
    errNum = clGetDeviceIDs(firstPlatformId, CL_DEVICE_TYPE_GPU, uiDevCount, cdDevices, NULL);


    context = clCreateContext(contextProperties, 1, &cdDevices[0], NULL, NULL, &errNum);
	//// alternate:
    //context = clCreateContextFromType(contextProperties, CL_DEVICE_TYPE_GPU,
    //                                  NULL, NULL, &errNum);

    if (errNum != CL_SUCCESS)
    {
        std::cout << "Could not create GPU context, trying CPU..." << std::endl;
        context = clCreateContextFromType(contextProperties, CL_DEVICE_TYPE_CPU,
                                          NULL, NULL, &errNum);
        if (errNum != CL_SUCCESS)
        {
            std::cerr << "Failed to create an OpenCL GPU or CPU context." << std::endl;
            return NULL;
        }
    }

    return context;
}

///
//  Create a command queue on the first device available on the
//  context
//
cl_command_queue ocl_texreadback :: CreateCommandQueue(cl_context context, cl_device_id *device)
{
    cl_int errNum;
    cl_device_id *devices;
    cl_command_queue commandQueue = NULL;
    size_t deviceBufferSize = -1;

    // First get the size of the devices buffer
    errNum = clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, NULL, &deviceBufferSize);
    if (errNum != CL_SUCCESS)
    {
        std::cerr << "Failed call to clGetContextInfo(...,GL_CONTEXT_DEVICES,...)";
        return NULL;
    }

    if (deviceBufferSize <= 0)
    {
        std::cerr << "No devices available.";
        return NULL;
    }

    // Allocate memory for the devices buffer
    devices = new cl_device_id[deviceBufferSize / sizeof(cl_device_id)];
    errNum = clGetContextInfo(context, CL_CONTEXT_DEVICES, deviceBufferSize, devices, NULL);
    if (errNum != CL_SUCCESS)
    {
        delete [] devices;
        std::cerr << "Failed to get device IDs";
        return NULL;
    }

    // In this example, we just choose the first available device.  In a
    // real program, you would likely use all available devices or choose
    // the highest performance device based on OpenCL device queries
    commandQueue = clCreateCommandQueue(context, devices[0], 0, NULL);
    if (commandQueue == NULL)
    {
        delete [] devices;
        std::cerr << "Failed to create commandQueue for device 0";
        return NULL;
    }

    *device = devices[0];
    delete [] devices;
    return commandQueue;
}

///
//  Create an OpenCL program from the kernel source file
//
cl_program ocl_texreadback :: CreateProgram(cl_context context, cl_device_id device, const char* fileName)
{
    cl_int errNum;
    cl_program program;

    std::ifstream kernelFile(fileName, std::ios::in);
    if (!kernelFile.is_open())
    {
        std::cerr << "Failed to open file for reading: " << fileName << std::endl;
        return NULL;
    }

    std::ostringstream oss;
    oss << kernelFile.rdbuf();

    std::string srcStdStr = oss.str();
    const char *srcStr = srcStdStr.c_str();
    program = clCreateProgramWithSource(context, 1,
                                        (const char**)&srcStr,
                                        NULL, NULL);
    if (program == NULL)
    {
        std::cerr << "Failed to create CL program from source." << std::endl;
        return NULL;
    }

    errNum = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (errNum != CL_SUCCESS)
    {
        // Determine the reason for the error
        char buildLog[16384];
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
                              sizeof(buildLog), buildLog, NULL);

        std::cerr << "Error in kernel: " << std::endl;
        std::cerr << buildLog;
        clReleaseProgram(program);
        return NULL;
    }

    return program;
}

///
//  Create memory objects used as the arguments to the kernel
//  The kernel takes three arguments: result (output), a (input),
//  and b (input)
//
bool ocl_texreadback :: CreateMemObjects(cl_context context, cl_mem memObjects[3],
                      float *a, float *b)
{
    memObjects[0] = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                   sizeof(float) * ARRAY_SIZE, a, NULL);
    memObjects[1] = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                   sizeof(float) * ARRAY_SIZE, b, NULL);
    memObjects[2] = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                   sizeof(float) * ARRAY_SIZE, NULL, NULL);

    if (memObjects[0] == NULL || memObjects[1] == NULL || memObjects[2] == NULL)
    {
        std::cerr << "Error creating memory objects." << std::endl;
        return false;
    }

    return true;
}

///
//  Cleanup any created OpenCL resources
//
void ocl_texreadback :: Cleanup(cl_context context, cl_command_queue commandQueue,
             cl_program program, cl_kernel kernel, cl_mem memObjects[3])
{
    for (int i = 0; i < 3; i++)
    {
        if (memObjects[i] != 0)
            clReleaseMemObject(memObjects[i]);
    }
    if (commandQueue != 0)
        clReleaseCommandQueue(commandQueue);

    if (kernel != 0)
        clReleaseKernel(kernel);

    if (program != 0)
        clReleaseProgram(program);

    if (context != 0)
        clReleaseContext(context);

}

/////////////////////////////////////////////////////////
//
// ocl_texreadback
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////
ocl_texreadback :: ocl_texreadback(t_floatarg size)
        : GemShape(size),
        context(0),
        commandQueue(0),
        program(0),
        device(0),
        kernel(0),
        memObjects({0,0,0})
{}

void ocl_texreadback :: startRendering(void)
{
    cl_int errNum;

    // Create an OpenCL context on first available platform
    // Create an OpenCL context on first available platform
    context = CreateContext();
    if (context == NULL)
    {
        throw(GemException("Failed to create OpenCL context."));
    }

    // Create a command-queue on the first device available
    // on the created context
    commandQueue = CreateCommandQueue(context, &device);
    if (commandQueue == NULL)
    {
        Cleanup(context, commandQueue, program, kernel, memObjects);
        throw(GemException("Failed to create command cue."));
    }

    // Create OpenCL program from HelloWorld.cl kernel source
    program = CreateProgram(context, device, "HelloWorld.cl");
    if (program == NULL)
    {
        Cleanup(context, commandQueue, program, kernel, memObjects);
        throw(GemException("Failed to create OpenCL program."));
    }

    // Create OpenCL kernel
    kernel = clCreateKernel(program, "hello_kernel", NULL);
    if (kernel == NULL)
    {
        Cleanup(context, commandQueue, program, kernel, memObjects);
        throw(GemException("Failed to create kernel"));
    }
    
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        a[i] = (float)i;
        b[i] = (float)(i * 2);
    }
}

void ocl_texreadback :: stopRendering(void){
  Cleanup(context, commandQueue, program, kernel, memObjects);
}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
ocl_texreadback :: ~ocl_texreadback()
{
  Cleanup(context, commandQueue, program, kernel, memObjects);
}

/////////////////////////////////////////////////////////
// renderShape
//
/////////////////////////////////////////////////////////
void ocl_texreadback :: renderShape(GemState *state)
{
  if(m_drawType==GL_DEFAULT_GEM)m_drawType=GL_QUADS;
    glNormal3f(0.0f, 0.0f, 1.0f);
    if (m_drawType == GL_LINE_LOOP)
        glLineWidth(m_linewidth);

    glBegin(m_drawType);

    SetVertex(state, -m_size,  -m_size, 0.0f,0.,0.,0);
    SetVertex(state, m_size,  -m_size, 0.0f,1.,0.,1);
    SetVertex(state, m_size,  m_size, 0.0f,1.,1.,2);
    SetVertex(state, -m_size,  m_size, 0.0f,0.,1.,3);

    glEnd();
    
    if (!CreateMemObjects(context, memObjects, a, b))
    {
      error("can't create Mem Object");
      return;
    }
    
    cl_int errNum;

    // Set the kernel arguments (result, a, b)
    errNum = clSetKernelArg(kernel, 0, sizeof(cl_mem), &memObjects[0]);
    errNum |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &memObjects[1]);
    errNum |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &memObjects[2]);
    if (errNum != CL_SUCCESS)
    {
        error("Error setting kernel arguments.");
        return;
    }

    size_t globalWorkSize[1] = { ARRAY_SIZE };
    size_t localWorkSize[1] = { 1 };

    // Queue the kernel up for execution across the array
    errNum = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL,
                                    globalWorkSize, localWorkSize,
                                    0, NULL, NULL);
    if (errNum != CL_SUCCESS)
    {
        error("Error queuing kernel for execution.");
        return;
    }

    // Read the output buffer back to the Host
    errNum = clEnqueueReadBuffer(commandQueue, memObjects[2], CL_TRUE,
                                 0, ARRAY_SIZE * sizeof(float), result,
                                 0, NULL, NULL);
    if (errNum != CL_SUCCESS)
    {
        error("Error reading result buffer.");
        return;
    }

    // Output the result buffer
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        std::cout << result[i] << " ";
    }
    std::cout << std::endl;
    std::cout << "Executed program succesfully." << std::endl;
}

void ocl_texreadback :: obj_setupCallback(t_class *classPtr){}
